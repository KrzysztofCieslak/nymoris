use x86_64::instructions::port::Port;

const CONFIG_ADDRESS: u16 = 0xCF8;
const CONFIG_DATA: u16 = 0xCFC;

#[derive(Debug, Clone, Copy)]
pub struct PciDevice {
    pub bus: u8,
    pub slot: u8,
    pub func: u8,
    pub vendor_id: u16,
    pub device_id: u16,
    pub class: u8,
    pub subclass: u8,
    pub prog_if: u8,
}

impl PciDevice {
    pub fn read_bar(&self, bar: u8) -> u64 {
        let offset = 0x10 + (bar as u8) * 4;
        let low = self.read_dword(offset) as u64;
        if (low & 0x1) != 0 {
            // I/O space BAR: mask lower 2 bits
            low & 0xFFFF_FFFF_FFFF_FFFC
        } else if bar < 5 && (low & 0x6) == 0x4 {
            // 64-bit memory BAR
            let high = self.read_dword(offset + 4) as u64;
            (high << 32) | (low & 0xFFFF_FFFF_FFFF_FFF0)
        } else {
            // 32-bit memory BAR
            low & 0xFFFF_FFFF_FFFF_FFF0
        }
    }

    pub fn read_dword(&self, offset: u8) -> u32 {
        let address = 0x80000000
            | ((self.bus as u32) << 16)
            | ((self.slot as u32) << 11)
            | ((self.func as u32) << 8)
            | ((offset as u32) & 0xFC);
        unsafe {
            let mut addr_port: Port<u32> = Port::new(CONFIG_ADDRESS);
            let mut data_port: Port<u32> = Port::new(CONFIG_DATA);
            addr_port.write(address);
            data_port.read()
        }
    }

    pub fn write_dword(&self, offset: u8, value: u32) {
        let address = 0x80000000
            | ((self.bus as u32) << 16)
            | ((self.slot as u32) << 11)
            | ((self.func as u32) << 8)
            | ((offset as u32) & 0xFC);
        unsafe {
            let mut addr_port: Port<u32> = Port::new(CONFIG_ADDRESS);
            let mut data_port: Port<u32> = Port::new(CONFIG_DATA);
            addr_port.write(address);
            data_port.write(value);
        }
    }

    pub fn enable_bus_mastering(&self) {
        let cmd = self.read_dword(0x04);
        self.write_dword(0x04, cmd | 0x04 | 0x02 | 0x01); // Bus Master + Memory Space + I/O Space
    }
}

fn serial_putc_pci(c: u8) {
    unsafe {
        core::arch::asm!("out dx, al", in("dx") 0x3F8u16, in("al") c, options(nomem, nostack));
    }
}

fn serial_puts_pci(s: &str) {
    for b in s.bytes() {
        serial_putc_pci(b);
    }
}

pub fn scan_all(devices: &mut [PciDevice; 32]) -> usize {
    serial_puts_pci("PCI: scan_all start\n");
    let mut count = 0;
    serial_puts_pci("PCI: entering bus loop\n");
    for bus in 0..=255u8 {
        for slot in 0..32u8 {
            let vendor = read_config_word(bus, slot, 0, 0x00);
            if vendor == 0xFFFF {
                continue;
            }
            for func in 0..8u8 {
                let vendor = read_config_word(bus, slot, func, 0x00);
                if vendor == 0xFFFF {
                    continue;
                }
                let device = read_config_word(bus, slot, func, 0x02);
                let class_revision = read_config_dword(bus, slot, func, 0x08);
                let class = ((class_revision >> 24) & 0xFF) as u8;
                let subclass = ((class_revision >> 16) & 0xFF) as u8;
                let prog_if = ((class_revision >> 8) & 0xFF) as u8;
                serial_puts_pci("PCI: found device ");
                // Simple hex output for vendor:device
                fn hex_nibble(n: u8) -> u8 { if n < 10 { b'0' + n } else { b'A' + (n - 10) } }
                fn put_hex_byte(b: u8) {
                    serial_putc_pci(hex_nibble(b >> 4));
                    serial_putc_pci(hex_nibble(b & 0xF));
                }
                put_hex_byte((vendor >> 8) as u8);
                put_hex_byte((vendor & 0xFF) as u8);
                serial_puts_pci(":");
                put_hex_byte((device >> 8) as u8);
                put_hex_byte((device & 0xFF) as u8);
                serial_puts_pci(" class=");
                put_hex_byte(class);
                serial_puts_pci(":");
                put_hex_byte(subclass);
                serial_puts_pci(":");
                put_hex_byte(prog_if);
                serial_puts_pci("\n");
                if count < devices.len() {
                    devices[count] = PciDevice {
                        bus,
                        slot,
                        func,
                        vendor_id: vendor,
                        device_id: device,
                        class,
                        subclass,
                        prog_if,
                    };
                    count += 1;
                }
            }
        }
    }
    serial_puts_pci("PCI: scan_all done\n");
    count
}

fn read_config_word(bus: u8, slot: u8, func: u8, offset: u8) -> u16 {
    let dword = read_config_dword(bus, slot, func, offset);
    if (offset & 2) != 0 {
        (dword >> 16) as u16
    } else {
        (dword & 0xFFFF) as u16
    }
}

fn read_config_dword(bus: u8, slot: u8, func: u8, offset: u8) -> u32 {
    let address = 0x80000000
        | ((bus as u32) << 16)
        | ((slot as u32) << 11)
        | ((func as u32) << 8)
        | ((offset as u32) & 0xFC);
    unsafe {
        let mut addr_port: Port<u32> = Port::new(CONFIG_ADDRESS);
        let mut data_port: Port<u32> = Port::new(CONFIG_DATA);
        addr_port.write(address);
        data_port.read()
    }
}
