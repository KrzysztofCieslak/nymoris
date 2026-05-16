pub mod ehci;
pub mod hid;
pub mod uhci;

pub fn init() {
    crate::println!("[USB] Scanning PCI for USB controllers...");
    let mut devices = [crate::pci::PciDevice {
        bus: 0, slot: 0, func: 0,
        vendor_id: 0, device_id: 0,
        class: 0, subclass: 0, prog_if: 0,
    }; 32];
    let count = crate::pci::scan_all(&mut devices);
    for i in 0..count {
        let dev = &devices[i];
        if dev.class == 0x0C && dev.subclass == 0x03 {
            crate::println!("[USB] Found USB controller: {:04x}:{:04x} prog_if={:02x}",
                dev.vendor_id, dev.device_id, dev.prog_if);
            dev.enable_bus_mastering();
            if dev.prog_if == 0x20 {
                crate::println!("[USB] Found EHCI controller at {:02x}:{:02x}.{:x}",
                    dev.bus, dev.slot, dev.func);
                let bar = dev.read_bar(0);
                crate::println!("[USB] EHCI BAR0 = {:#x}", bar);
                if bar != 0 {
                    unsafe { ehci::init_controller(bar); }
                }
            } else if dev.prog_if == 0x00 {
                crate::println!("[USB] Found UHCI controller at {:02x}:{:02x}.{:x}",
                    dev.bus, dev.slot, dev.func);
                // UHCI may use any BAR; find first non-zero I/O BAR
                let mut bar = 0;
                for b in 0..6u8 {
                    let v = dev.read_bar(b);
                    if v != 0 {
                        bar = v;
                        crate::println!("[USB] UHCI BAR{} = {:#x}", b, v);
                        break;
                    }
                }
                if bar != 0 {
                    unsafe { uhci::init_controller(bar as u16); }
                } else {
                    crate::println!("[USB] No valid BAR found for UHCI");
                }
            }
        }
    }
}
