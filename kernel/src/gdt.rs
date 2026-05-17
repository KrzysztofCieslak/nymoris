use lazy_static::lazy_static;
use x86_64::instructions::segmentation::{Segment, CS};
use x86_64::instructions::tables::load_tss;
use x86_64::structures::gdt::{Descriptor, GlobalDescriptorTable, SegmentSelector};
use x86_64::structures::tss::TaskStateSegment;
use x86_64::{PrivilegeLevel, VirtAddr};

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
        let user_data_selector = gdt.append(Descriptor::user_data_segment());
        let user_code_selector = gdt.append(Descriptor::user_code_segment());
        (
            gdt,
            Selectors {
                code_selector,
                tss_selector,
                user_data_selector,
                user_code_selector,
            },
        )
    };
}

struct Selectors {
    code_selector: SegmentSelector,
    tss_selector: SegmentSelector,
    user_data_selector: SegmentSelector,
    user_code_selector: SegmentSelector,
}

unsafe fn outb(port: u16, value: u8) {
    core::arch::asm!("out dx, al", in("dx") port, in("al") value, options(nomem, nostack));
}

unsafe fn serial_write(s: &[u8]) {
    for b in s {
        outb(0x3F8, *b);
    }
}

pub fn user_code_selector() -> SegmentSelector {
    SegmentSelector::new(GDT.1.user_code_selector.0 >> 3, PrivilegeLevel::Ring3)
}

pub fn user_data_selector() -> SegmentSelector {
    SegmentSelector::new(GDT.1.user_data_selector.0 >> 3, PrivilegeLevel::Ring3)
}

pub unsafe fn set_rsp0(addr: u64) {
    let tss = &*TSS as *const TaskStateSegment as *mut TaskStateSegment;
    (*tss).privilege_stack_table[0] = VirtAddr::new(addr);
}

unsafe fn read_seg_reg(name: &[u8], val: u16) {
    serial_write(name);
    serial_write(b"=");
    let hex = b"0123456789ABCDEF";
    serial_write(&[hex[(val >> 12) as usize], hex[((val >> 8) & 0xF) as usize],
                   hex[((val >> 4) & 0xF) as usize], hex[(val & 0xF) as usize]]);
    serial_write(b"\n");
}

pub unsafe fn dump_segments() {
    let cs: u16;
    let ds: u16;
    let es: u16;
    let fs: u16;
    let gs: u16;
    let ss: u16;
    let tr: u16;
    core::arch::asm!("mov {0:x}, cs", out(reg) cs);
    core::arch::asm!("mov {0:x}, ds", out(reg) ds);
    core::arch::asm!("mov {0:x}, es", out(reg) es);
    core::arch::asm!("mov {0:x}, fs", out(reg) fs);
    core::arch::asm!("mov {0:x}, gs", out(reg) gs);
    core::arch::asm!("mov {0:x}, ss", out(reg) ss);
    core::arch::asm!("str {0:x}", out(reg) tr);
    serial_write(b"=== SEGMENT REGISTERS ===\n");
    read_seg_reg(b"CS", cs);
    read_seg_reg(b"DS", ds);
    read_seg_reg(b"ES", es);
    read_seg_reg(b"FS", fs);
    read_seg_reg(b"GS", gs);
    read_seg_reg(b"SS", ss);
    read_seg_reg(b"TR", tr);
    serial_write(b"=========================\n");
}

pub fn init() {
    use x86_64::instructions::segmentation::{DS, ES, FS, GS, SS};
    use x86_64::PrivilegeLevel;

    unsafe { serial_write(b"gdt: about to load\n"); }
    GDT.0.load();
    unsafe { serial_write(b"gdt: loaded\n"); }
    unsafe {
        let null_sel = SegmentSelector::new(0, PrivilegeLevel::Ring0);
        serial_write(b"gdt: about to set CS\n");
        CS::set_reg(GDT.1.code_selector);
        serial_write(b"gdt: CS set\n");
        serial_write(b"gdt: about to set DS\n");
        DS::set_reg(null_sel);
        serial_write(b"gdt: DS set\n");
        serial_write(b"gdt: about to set ES\n");
        ES::set_reg(null_sel);
        serial_write(b"gdt: ES set\n");
        serial_write(b"gdt: about to set FS\n");
        FS::set_reg(null_sel);
        serial_write(b"gdt: FS set\n");
        serial_write(b"gdt: about to set GS\n");
        GS::set_reg(null_sel);
        serial_write(b"gdt: GS set\n");
        serial_write(b"gdt: about to set SS\n");
        SS::set_reg(null_sel);
        serial_write(b"gdt: SS set\n");
        serial_write(b"gdt: about to load TSS\n");
        load_tss(GDT.1.tss_selector);
        serial_write(b"gdt: TSS loaded\n");
        dump_segments();
    }
}
