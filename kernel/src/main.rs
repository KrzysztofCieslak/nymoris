#![no_std]
#![no_main]

use core::panic::PanicInfo;
use nymoris::{gdt, idt, interrupts, keyboard, shell, framebuffer, usb, memory, net, agent, scheduler, fs, elf, commands, serial};
use nymoris::println;

#[panic_handler]
fn panic(info: &PanicInfo) -> ! {
    framebuffer::WRITER.lock().set_color(framebuffer::Color::White, framebuffer::Color::Red);
    println!("\n\nKERNEL PANIC: {}", info);
    interrupts::hlt_loop();
}

unsafe fn outb(port: u16, value: u8) {
    core::arch::asm!("out dx, al", in("dx") port, in("al") value, options(nomem, nostack));
}

unsafe fn inb(port: u16) -> u8 {
    let value: u8;
    core::arch::asm!("in al, dx", out("al") value, in("dx") port, options(nomem, nostack));
    value
}

unsafe fn serial_write(s: &[u8]) {
    for b in s {
        outb(0x3F8, *b);
    }
}

#[repr(C, align(16))]
struct Stack([u8; 128 * 1024]);

static mut STACK: Stack = Stack([0; 128 * 1024]);

#[no_mangle]
#[link_section = ".text._start"]
pub extern "C" fn _start() -> ! {
    unsafe {
        // Set up our own stack - Limine's provided stack may be small
        let stack_top = (core::ptr::addr_of_mut!(STACK.0) as *mut u8).add(128 * 1024);
        core::arch::asm!(
            "mov rsp, {}",
            "push 0",
            in(reg) stack_top,
            options(nomem, preserves_flags)
        );
    }

    // Enable SSE - the compiler may auto-vectorize with SSE instructions
    unsafe {
        let mut cr0: u64;
        core::arch::asm!("mov {}, cr0", out(reg) cr0);
        cr0 &= !(1 << 2); // Clear EM (Coprocessor Emulation)
        cr0 |= 1 << 1;    // Set MP (Monitor Coprocessor)
        core::arch::asm!("mov cr0, {}", in(reg) cr0);

        let mut cr4: u64;
        core::arch::asm!("mov {}, cr4", out(reg) cr4);
        cr4 |= 1 << 9;  // Set OSFXSR
        cr4 |= 1 << 10; // Set OSXMMEXCPT
        core::arch::asm!("mov cr4, {}", in(reg) cr4);
    }

    unsafe {
        outb(0x3F8 + 1, 0x00);
        outb(0x3F8 + 3, 0x80);
        outb(0x3F8 + 0, 0x03);
        outb(0x3F8 + 1, 0x00);
        outb(0x3F8 + 3, 0x03);
        outb(0x3F8 + 2, 0x07); // FCR: enable FIFOs, 1-byte trigger level
        outb(0x3F8 + 4, 0x0B);

        for b in b"Nymoris kernel _start() reached!\n" {
            outb(0x3F8, *b);
        }
    }

    framebuffer::init();

    println!("Nymoris v0.1 booting...");
    println!("============================");

    gdt::init();
    println!("[OK] GDT initialized");

    idt::init();
    println!("[OK] IDT initialized");

    unsafe { interrupts::init() };
    println!("[OK] PIC initialized");

    keyboard::init();
    println!("[OK] Keyboard initialized");

    serial::init();
    println!("[OK] Serial interrupts enabled");

    memory::print_memmap();
    unsafe { memory::allocator::init(); }
    unsafe { memory::heap::HEAP.init(); }
    unsafe { memory::paging::init(); }

    unsafe {
        let stack_top = (core::ptr::addr_of_mut!(STACK.0) as *mut u8).add(128 * 1024) as u64;
        scheduler::init(stack_top);
    }
    println!("[OK] Scheduler initialized");

    unsafe { fs::ramdisk::init(); }
    static HELLO_ELF: &[u8] = include_bytes!("../../userspace/hello/hello.elf");
    unsafe { fs::ramdisk::register("hello", HELLO_ELF); }

    unsafe { usb::init(); }
    println!("[OK] USB initialized");

    net::init();
    println!("[OK] Network initialized");

    agent::init();
    agent::start();

    println!("\nWelcome to Nymoris! Type 'help' for available commands.\n");

    shell::run();
}
