use lazy_static::lazy_static;
use x86_64::instructions::segmentation::{Segment, CS};
use x86_64::instructions::tables::load_tss;
use x86_64::structures::gdt::{Descriptor, GlobalDescriptorTable, SegmentSelector};
use x86_64::structures::tss::TaskStateSegment;
use x86_64::VirtAddr;

pub const DOUBLE_FAULT_IST_INDEX: u16 = 0;

lazy_static! {
    static ref TSS: TaskStateSegment = {
        let mut tss = TaskStateSegment::new();
        tss.interrupt_stack_table[DOUBLE_FAULT_IST_INDEX as usize] = {
            const STACK_SIZE: usize = 4096 * 5;
            static mut STACK: [u8; STACK_SIZE] = [0; STACK_SIZE];
            let stack_start = VirtAddr::from_ptr(unsafe { &STACK });
            let stack_end = stack_start + STACK_SIZE as u64;
            stack_end
        };
        tss
    };

    static ref GDT: (GlobalDescriptorTable, Selectors) = {
        let mut gdt = GlobalDescriptorTable::new();
        let code_selector = gdt.append(Descriptor::kernel_code_segment());
        let tss_selector = gdt.append(Descriptor::tss_segment(&TSS));
        (
            gdt,
            Selectors {
                code_selector,
                tss_selector,
            },
        )
    };
}

struct Selectors {
    code_selector: SegmentSelector,
    tss_selector: SegmentSelector,
}

unsafe fn outb(port: u16, value: u8) {
    core::arch::asm!("out dx, al", in("dx") port, in("al") value, options(nomem, nostack));
}

unsafe fn serial_write(s: &[u8]) {
    for b in s {
        outb(0x3F8, *b);
    }
}

pub fn init() {
    use x86_64::instructions::segmentation::DS;
    use x86_64::PrivilegeLevel;

    unsafe { serial_write(b"gdt: about to load\n"); }
    GDT.0.load();
    unsafe { serial_write(b"gdt: loaded\n"); }
    unsafe {
        serial_write(b"gdt: about to set CS\n");
        CS::set_reg(GDT.1.code_selector);
        serial_write(b"gdt: CS set\n");
        serial_write(b"gdt: about to set DS\n");
        DS::set_reg(SegmentSelector::new(0, PrivilegeLevel::Ring0));
        serial_write(b"gdt: DS set\n");
        serial_write(b"gdt: about to load TSS\n");
        load_tss(GDT.1.tss_selector);
        serial_write(b"gdt: TSS loaded\n");
    }
}
