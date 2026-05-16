use core::ptr::{read_volatile, write_volatile};

pub struct EhciRegs {
    base: u64,
    cap_length: u8,
}

impl EhciRegs {
    pub unsafe fn new(base: u64) -> Self {
        let cap_length = read_volatile(base as *const u8);
        Self { base, cap_length }
    }

    fn op_offset(&self) -> u64 {
        self.base + self.cap_length as u64
    }

    pub fn read_op(&self, offset: u64) -> u32 {
        unsafe { read_volatile((self.op_offset() + offset) as *const u32) }
    }

    pub fn write_op(&self, offset: u64, value: u32) {
        unsafe { write_volatile((self.op_offset() + offset) as *mut u32, value); }
    }

    pub fn read_cap(&self, offset: u64) -> u32 {
        unsafe { read_volatile((self.base + offset) as *const u32) }
    }

    pub fn num_ports(&self) -> u8 {
        (self.read_cap(0x04) & 0x0F) as u8
    }
}

#[repr(C, align(4096))]
struct FrameList {
    entries: [u32; 1024],
}

#[repr(C, align(32))]
pub struct QueueHead {
    pub horizontal_link: u32,
    pub endpoint_chars: u32,
    pub endpoint_caps: u32,
    pub current_qtd: u32,
    pub next_qtd: u32,
    pub alt_next_qtd: u32,
    pub token: u32,
    pub buffers: [u32; 5],
    pub _padding: [u32; 7],
}

#[repr(C, align(32))]
pub struct Qtd {
    pub next_qtd: u32,
    pub alt_next_qtd: u32,
    pub token: u32,
    pub buffers: [u32; 5],
    pub _padding: [u32; 3],
}

static mut FRAME_LIST: FrameList = FrameList { entries: [1; 1024] };
static mut QH_SETUP: QueueHead = QueueHead {
    horizontal_link: 1,
    endpoint_chars: 0,
    endpoint_caps: 0,
    current_qtd: 0,
    next_qtd: 0,
    alt_next_qtd: 0,
    token: 0,
    buffers: [0; 5],
    _padding: [0; 7],
};
static mut QTD_SETUP: Qtd = Qtd {
    next_qtd: 0,
    alt_next_qtd: 1,
    token: 0,
    buffers: [0; 5],
    _padding: [0; 3],
};
static mut QTD_DATA: Qtd = Qtd {
    next_qtd: 0,
    alt_next_qtd: 1,
    token: 0,
    buffers: [0; 5],
    _padding: [0; 3],
};
static mut QTD_STATUS: Qtd = Qtd {
    next_qtd: 0,
    alt_next_qtd: 1,
    token: 0,
    buffers: [0; 5],
    _padding: [0; 3],
};
static mut SETUP_BUFFER: [u8; 8] = [0; 8];
static mut DATA_BUFFER: [u8; 256] = [0; 256];

static mut EHCI_BAR: u64 = 0;

pub fn get_bar() -> u64 {
    unsafe { EHCI_BAR }
}

pub fn set_bar(bar: u64) {
    unsafe { EHCI_BAR = bar; }
}

pub fn virt_to_phys(virt: u64) -> u64 {
    virt - 0xffffffff80000000 + 0x100000
}

unsafe fn init_qh(qh: *mut QueueHead, device: u8, endpoint: u8, max_packet: u16, head: bool) {
    let _addr = virt_to_phys(qh as u64) as u32;
    (*qh).horizontal_link = 1;
    // Endpoint characteristics:
    // bits 0-6: device address
    // bits 7-10: endpoint number
    // bits 11-12: endpoint speed (2 = high speed)
    // bit 13: head of reclamation list
    // bits 14: data toggle control (1 = use qTD)
    // bits 15-18: max packet length
    let mut chars = (device as u32)
        | ((endpoint as u32) << 7)
        | (2u32 << 12)  // high speed
        | (1u32 << 14)  // dtc
        | ((max_packet as u32) << 16);
    if head {
        chars |= 1u32 << 13; // Head of Reclamation List
    }
    (*qh).endpoint_chars = chars;
    (*qh).endpoint_caps = 0;
    (*qh).current_qtd = 0;
    (*qh).next_qtd = 1;      // Terminate - no active qTD
    (*qh).alt_next_qtd = 1;  // Terminate
    (*qh).token = 0;
}

unsafe fn init_qtd(qtd: *mut Qtd, data_phys: u32, len: u16, pid: u8, toggle: bool) {
    (*qtd).next_qtd = 1;
    (*qtd).alt_next_qtd = 1;
    let mut token = ((len as u32) << 16)
        | (0x3u32 << 10)  // CERR = 3
        | ((pid as u32) << 8)
        | (1u32 << 7);    // ACTIVE
    if toggle {
        token |= 1u32 << 31; // data toggle
    }
    (*qtd).token = token;
    // Buffer pointer 0: data_phys with page offset in bits 0-11
    (*qtd).buffers[0] = data_phys;
    for i in 1..5 {
        if data_phys + (i as u32) * 0x1000 > data_phys {
            (*qtd).buffers[i] = (data_phys & !0xFFF) + (i as u32) * 0x1000;
        } else {
            (*qtd).buffers[i] = 0;
        }
    }
}

fn wait_for_qtd(qtd: *mut Qtd) -> bool {
    for _ in 0..1000000 {
        unsafe {
            let token = (*qtd).token;
            if token & (1 << 7) == 0 {
                // Check status
                if token & 0x7C != 0 {
                    return false; // Error
                }
                return true;
            }
        }
        for _ in 0..100 {
            unsafe { core::arch::asm!("pause", options(nomem, nostack)); }
        }
    }
    false
}

fn wait_for_port_enabled(regs: &EhciRegs, port: u8) -> bool {
    // After reset, wait for Port Reset (bit 8) to clear, then check Port Enabled (bit 2)
    for _ in 0..100000 {
        let status = regs.read_op(0x44 + 4 * (port as u64 - 1));
        if status & (1 << 8) == 0 {
            // Reset complete - check if enabled
            return status & (1 << 2) != 0;
        }
    }
    false
}

pub unsafe fn init_controller(bar: u64) {
    set_bar(bar);
    let regs = EhciRegs::new(bar);
    let n_ports = regs.num_ports();
    crate::println!("[EHCI] {} ports, CAPLENGTH={}", n_ports, regs.cap_length);

    // Reset controller
    let mut cmd = regs.read_op(0x00);
    cmd |= 1 << 1; // HCRESET
    regs.write_op(0x00, cmd);
    for _ in 0..100000 {
        if regs.read_op(0x00) & (1 << 1) == 0 {
            break;
        }
    }

    // Set CONFIGFLAG = 1 so EHCI owns the ports (not companions)
    regs.write_op(0x40, 1);

    // Set frame list base
    let fl_phys = virt_to_phys(&FRAME_LIST as *const _ as u64) as u32;
    regs.write_op(0x14, fl_phys); // PERIODICLISTBASE
    regs.write_op(0x10, 0);       // CTRLDSSEGMENT (32-bit)

    for i in 0..1024 {
        FRAME_LIST.entries[i] = 1; // Terminate
    }

    // Set async list base
    let qh_phys = virt_to_phys(&QH_SETUP as *const _ as u64) as u32;
    init_qh(&mut QH_SETUP, 0, 0, 64, true);
    QH_SETUP.horizontal_link = (qh_phys & !0x1F) | 0x02; // Typ=QH, T=0
    regs.write_op(0x18, qh_phys); // ASYNCLISTADDR

    // Clear USB status
    regs.write_op(0x04, 0x3F);

    // Set interrupt threshold, run
    cmd = regs.read_op(0x00);
    cmd |= 1; // RS (Run/Stop)
    cmd |= 1 << 4; // ASE (Async Schedule Enable)
    // Do NOT enable PSE - we don't use periodic schedule
    regs.write_op(0x00, cmd);

    // Wait for halted to clear
    for _ in 0..100000 {
        if regs.read_op(0x04) & (1 << 12) == 0 {
            break;
        }
    }

    crate::println!("[EHCI] Controller running");

    // Scan ports for keyboards
    for port in 1..=n_ports {
        let status = regs.read_op(0x44 + 4 * (port as u64 - 1));
        if status & 1 != 0 {
            crate::println!("[EHCI] Port {}: device connected, resetting...", port);
            // Power on port first
            let s = regs.read_op(0x44 + 4 * (port as u64 - 1));
            if s & (1 << 12) == 0 {
                regs.write_op(0x44 + 4 * (port as u64 - 1), 1 << 12);
                for _ in 0..10000 {
                    let s2 = regs.read_op(0x44 + 4 * (port as u64 - 1));
                    if s2 & (1 << 12) != 0 {
                        break;
                    }
                }
            }
            // Reset port - write only PR bit, nothing else
            regs.write_op(0x44 + 4 * (port as u64 - 1), 1 << 8);
            // Wait for reset to complete (PR bit clears)
            for _ in 0..100000 {
                let s = regs.read_op(0x44 + 4 * (port as u64 - 1));
                if s & (1 << 8) == 0 {
                    break;
                }
            }
            // After reset, wait a bit for hardware to settle
            for _ in 0..100000 {}
            // Check port status after reset
            let s = regs.read_op(0x44 + 4 * (port as u64 - 1));
            crate::println!("[EHCI] Port {} status after reset: {:#x}", port, s);
            if s & (1 << 13) != 0 {
                crate::println!("[EHCI] Port {}: low/full-speed device, handed to companion", port);
            }
            if s & (1 << 2) != 0 {
                crate::println!("[EHCI] Port {} enabled", port);
                enumerate_device(&regs, port);
            } else {
                crate::println!("[EHCI] Port {} failed to enable", port);
            }
        }
    }
}

pub unsafe fn control_transfer(
    regs: &EhciRegs,
    device: u8,
    endpoint: u8,
    setup: &[u8; 8],
    mut data: Option<&mut [u8]>,
    max_packet: u16,
) -> bool {
    let data_ptr = data.as_mut().map_or(core::ptr::null_mut(), |d| d.as_mut_ptr());
    let data_len = data.as_ref().map_or(0, |d| d.len() as u16);
    let data_out = setup[0] & 0x80 == 0;
    let data_phys = if data_ptr.is_null() {
        0
    } else {
        virt_to_phys(data_ptr as u64) as u32
    };

    if data_out && !data_ptr.is_null() {
        core::ptr::copy_nonoverlapping(
            data_ptr,
            DATA_BUFFER.as_mut_ptr(),
            data_len as usize,
        );
    }

    let setup_phys = virt_to_phys(SETUP_BUFFER.as_mut_ptr() as u64) as u32;
    SETUP_BUFFER.copy_from_slice(setup);

    // Build SETUP qTD (PID = 2)
    init_qtd(&mut QTD_SETUP, setup_phys, 8, 2, false);
    // Build DATA qTD (PID = 0 for OUT, 1 for IN)
    let data_pid = if setup[0] & 0x80 != 0 { 1u8 } else { 0u8 };
    init_qtd(&mut QTD_DATA, data_phys, data_len, data_pid, true);
    // Build STATUS qTD (PID = opposite of DATA)
    let status_pid = if data_pid == 1 { 0u8 } else { 1u8 };
    init_qtd(&mut QTD_STATUS, 0, 0, status_pid, true);

    // Link qTDs
    let setup_phys = virt_to_phys(&QTD_SETUP as *const _ as u64) as u32;
    let data_phys_qtd = virt_to_phys(&QTD_DATA as *const _ as u64) as u32;
    let status_phys = virt_to_phys(&QTD_STATUS as *const _ as u64) as u32;

    QTD_SETUP.next_qtd = data_phys_qtd;
    QTD_DATA.next_qtd = status_phys;
    QTD_STATUS.next_qtd = 1; // Terminate

    // Re-init QH
    init_qh(&mut QH_SETUP, device, endpoint, max_packet, false);
    QH_SETUP.next_qtd = setup_phys;
    let qh_phys = virt_to_phys(&QH_SETUP as *const _ as u64) as u32;
    QH_SETUP.horizontal_link = (qh_phys & !0x1F) | 0x02;

    // Trigger doorbell
    let mut cmd = regs.read_op(0x00);
    cmd |= 1 << 6; // IAAD (Interrupt on Async Advance Doorbell)
    regs.write_op(0x00, cmd);

    // Wait for completion
    let ok = wait_for_qtd(&mut QTD_STATUS);
    if ok && !data_out && data_len > 0 && !data_ptr.is_null() {
        let actual_len = (QTD_DATA.token >> 16) & 0x7FFF;
        let len = (data_len as usize).min(actual_len as usize);
        core::ptr::copy_nonoverlapping(
            DATA_BUFFER.as_ptr(),
            data_ptr,
            len,
        );
    }

    ok
}

unsafe fn enumerate_device(regs: &EhciRegs, port: u8) {
    let mut buf = [0u8; 18];
    let setup = [0x80, 6, 0, 1, 0, 0, 18, 0]; // GET_DESCRIPTOR (Device)

    if control_transfer(regs, 0, 0, &setup, Some(&mut buf), 64) {
        let ptr = buf.as_ptr();
        let b_device_class = *ptr.add(4);
        let b_device_subclass = *ptr.add(5);
        let b_device_protocol = *ptr.add(6);
        let b_max_packet_size0 = *ptr.add(7);
        let id_vendor = (*(ptr.add(8) as *const u16)).to_le();
        let id_product = (*(ptr.add(10) as *const u16)).to_le();
        let b_num_configurations = *ptr.add(17);
        crate::println!(
            "[USB] Device: vendor={:04x} product={:04x} class={} subclass={} proto={} maxpacket={}",
            id_vendor, id_product,
            b_device_class, b_device_subclass,
            b_device_protocol, b_max_packet_size0
        );

        // Set address to 1
        let set_addr = [0x00, 5, 1, 0, 0, 0, 0, 0];
        if control_transfer(regs, 0, 0, &set_addr, None, b_max_packet_size0 as u16) {
            // Wait after set address
            for _ in 0..100000 {}
            crate::println!("[USB] Address set to 1");

            // Get configuration descriptor
            let mut cfg_buf = [0u8; 64];
            let get_cfg = [0x80, 6, 0, 2, 0, 0, 64, 0]; // GET_DESCRIPTOR (Configuration)
            if control_transfer(regs, 1, 0, &get_cfg, Some(&mut cfg_buf), b_max_packet_size0 as u16) {
                parse_config_descriptor(&cfg_buf, b_max_packet_size0 as u16);
            }
        }
    }
}

unsafe fn parse_config_descriptor(buf: &[u8; 64], max_packet: u16) {
    let total_len = (buf[2] as u16) | ((buf[3] as u16) << 8);
    crate::println!("[USB] Config descriptor total length: {}", total_len);

    let mut i: usize = 9;
    let mut keyboard_found = false;

    while i < total_len as usize && i < buf.len() {
        let len = buf[i];
        let dtype = buf[i + 1];
        if len == 0 {
            break;
        }

        if dtype == 0x04 {
            // Interface descriptor
            let iface_class = buf[i + 5];
            let iface_subclass = buf[i + 6];
            let iface_proto = buf[i + 7];
            crate::println!(
                "[USB] Interface: class={} subclass={} proto={}",
                iface_class, iface_subclass, iface_proto
            );
            if iface_class == 0x03 && iface_subclass == 0x01 && iface_proto == 0x01 {
                crate::println!("[USB] HID Boot Keyboard found!");
                keyboard_found = true;
            }
        } else if dtype == 0x05 && keyboard_found {
            // Endpoint descriptor
            let ep_addr = buf[i + 2];
            let ep_attr = buf[i + 3];
            let ep_max_packet = (buf[i + 4] as u16) | ((buf[i + 5] as u16) << 8);
            let ep_interval = buf[i + 6];
            crate::println!(
                "[USB] Endpoint: addr={:02x} attr={:02x} maxpacket={} interval={}",
                ep_addr, ep_attr, ep_max_packet, ep_interval
            );
            if ep_addr & 0x80 != 0 {
                // IN endpoint
                super::hid::init_keyboard_endpoint(ep_addr, ep_max_packet, ep_interval, max_packet);
            }
        } else if dtype == 0x21 {
            // HID descriptor
            crate::println!("[USB] HID descriptor found");
        }

        i += len as usize;
    }
}
