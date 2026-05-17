use lazy_static::lazy_static;
use pic8259::ChainedPics;
use spin::Mutex;

pub const PIC_1_OFFSET: u8 = 32;
pub const PIC_2_OFFSET: u8 = PIC_1_OFFSET + 8;

#[derive(Debug, Clone, Copy)]
#[repr(u8)]
pub enum InterruptIndex {
    Timer = PIC_1_OFFSET,
    Keyboard,
}

impl InterruptIndex {
    pub fn as_u8(self) -> u8 {
        self as u8
    }

    pub fn as_usize(self) -> usize {
        usize::from(self.as_u8())
    }
}

lazy_static! {
    pub static ref PICS: Mutex<ChainedPics> =
        Mutex::new(unsafe { ChainedPics::new(PIC_1_OFFSET, PIC_2_OFFSET) });
}

pub unsafe fn init() {
    let mut pics = PICS.lock();
    pics.initialize();
    let mut master_mask = 0xFF;
    let slave_mask = 0xFF;
    master_mask &= !(1 << 0); // Unmask timer on master PIC
    master_mask &= !(1 << 1); // Unmask keyboard on master PIC
    pics.write_masks(master_mask, slave_mask);
    drop(pics);
    x86_64::instructions::interrupts::enable();
}

pub fn hlt_loop() -> ! {
    loop {
        x86_64::instructions::interrupts::disable();
        x86_64::instructions::hlt();
    }
}

// Assembly stubs for all interrupt/exception handlers.
// extern "x86-interrupt" generates SSE saves (movaps) in the prologue that
// fault when the stack is misaligned. For same-ring interrupts, the CPU pushes
// 24 bytes (no error code) or 32 bytes (with error code). The compiler's
// prologue doesn't account for this properly, causing crashes.
// These assembly stubs save registers, call normal extern "C" Rust functions
// (which have correct stack alignment), then restore and iretq.

// Helper macro to generate a stub that calls a Rust function with no args.
macro_rules! interrupt_stub {
    ($name:ident, $rust_fn:ident) => {
        core::arch::global_asm!(
            concat!(".globl ", stringify!($name)),
            concat!(stringify!($name), ":"),
            "push rax",
            "push rcx",
            "push rdx",
            "push rsi",
            "push rdi",
            "push r8",
            "push r9",
            "push r10",
            "push r11",
            concat!("call ", stringify!($rust_fn)),
            "pop r11",
            "pop r10",
            "pop r9",
            "pop r8",
            "pop rdi",
            "pop rsi",
            "pop rdx",
            "pop rcx",
            "pop rax",
            "iretq",
        );
    };
}

// Stubs for exceptions without error code (CPU pushes 24 bytes).
interrupt_stub!(breakpoint_stub, breakpoint_rust);
interrupt_stub!(timer_interrupt_stub, timer_interrupt_rust);
interrupt_stub!(keyboard_interrupt_stub, keyboard_interrupt_rust);

// Stubs for exceptions with error code (CPU pushes 32 bytes).
// Error code is at [rsp+72] after our 9 pushes; pass it in RDI.
macro_rules! exception_stub_with_errcode {
    ($name:ident, $rust_fn:ident) => {
        core::arch::global_asm!(
            concat!(".globl ", stringify!($name)),
            concat!(stringify!($name), ":"),
            "push rax",
            "push rcx",
            "push rdx",
            "push rsi",
            "push rdi",
            "push r8",
            "push r9",
            "push r10",
            "push r11",
            "mov rdi, [rsp + 72]",
            concat!("call ", stringify!($rust_fn)),
            "pop r11",
            "pop r10",
            "pop r9",
            "pop r8",
            "pop rdi",
            "pop rsi",
            "pop rdx",
            "pop rcx",
            "pop rax",
            "iretq",
        );
    };
}

exception_stub_with_errcode!(double_fault_stub, double_fault_rust);
exception_stub_with_errcode!(page_fault_stub, page_fault_rust);

extern "C" {
    pub fn breakpoint_stub();
    pub fn timer_interrupt_stub();
    pub fn keyboard_interrupt_stub();
    pub fn double_fault_stub();
    pub fn page_fault_stub();
}

#[no_mangle]
pub extern "C" fn breakpoint_rust() {
    crate::println!("EXCEPTION: BREAKPOINT");
}

#[no_mangle]
pub extern "C" fn timer_interrupt_rust() {
    unsafe {
        crate::usb::hid::poll_keyboard();
        crate::usb::uhci::poll_keyboard();
        crate::agent::tick();

        // Send EOI before potentially switching away.
        PICS.lock()
            .notify_end_of_interrupt(InterruptIndex::Timer.as_u8());

        // Preemptive scheduling: switch to next ready task.
        crate::scheduler::schedule();
    }
}

#[no_mangle]
pub extern "C" fn keyboard_interrupt_rust() {
    unsafe {
        let mut status_port = x86_64::instructions::port::Port::new(0x64);
        let status: u8 = status_port.read();

        if status & 0x01 != 0 {
            let mut data_port = x86_64::instructions::port::Port::new(0x60);
            let scancode: u8 = data_port.read();
            crate::keyboard::handle_scancode(scancode);
        }

        PICS.lock()
            .notify_end_of_interrupt(InterruptIndex::Keyboard.as_u8());
    }
}

#[no_mangle]
pub extern "C" fn double_fault_rust(_error_code: u64) -> ! {
    crate::framebuffer::WRITER.lock().set_color(crate::framebuffer::Color::White, crate::framebuffer::Color::Red);
    crate::println!("EXCEPTION: DOUBLE FAULT");
    x86_64::instructions::interrupts::disable();
    loop {
        x86_64::instructions::hlt();
    }
}

#[no_mangle]
pub extern "C" fn page_fault_rust(error_code: u64) -> ! {
    use x86_64::registers::control::Cr2;
    crate::framebuffer::WRITER.lock().set_color(crate::framebuffer::Color::White, crate::framebuffer::Color::Red);
    crate::println!("EXCEPTION: PAGE FAULT");
    crate::println!("Accessed Address: {:?}", Cr2::read());
    crate::println!("Error Code: {:#x}", error_code);
    x86_64::instructions::interrupts::disable();
    loop {
        x86_64::instructions::hlt();
    }
}
