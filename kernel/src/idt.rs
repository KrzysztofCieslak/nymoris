use lazy_static::lazy_static;
use x86_64::structures::idt::InterruptDescriptorTable;
use x86_64::VirtAddr;

lazy_static! {
    static ref IDT: InterruptDescriptorTable = {
        let mut idt = InterruptDescriptorTable::new();

        // Use assembly stubs for all handlers to avoid SSE stack misalignment
        // in extern "x86-interrupt" prologues. For same-ring interrupts, the CPU
        // pushes 24 bytes (no error code) or 32 bytes (with error code). The
        // compiler's prologue doesn't account for this, causing movaps to fault.
        unsafe {
            idt.breakpoint
                .set_handler_addr(VirtAddr::new(
                    crate::interrupts::breakpoint_stub as *const () as u64
                ));
            idt.double_fault
                .set_handler_addr(VirtAddr::new(
                    crate::interrupts::double_fault_stub as *const () as u64
                ));
            idt.page_fault
                .set_handler_addr(VirtAddr::new(
                    crate::interrupts::page_fault_stub as *const () as u64
                ));
            idt[crate::interrupts::InterruptIndex::Timer.as_u8()]
                .set_handler_addr(VirtAddr::new(
                    crate::interrupts::timer_interrupt_stub as *const () as u64
                ));
            idt[crate::interrupts::InterruptIndex::Keyboard.as_u8()]
                .set_handler_addr(VirtAddr::new(
                    crate::interrupts::keyboard_interrupt_stub as *const () as u64
                ));
        }

        idt
    };
}

pub fn init() {
    IDT.load();
}
