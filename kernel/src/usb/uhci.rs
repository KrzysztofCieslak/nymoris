use core::ptr::{read_volatile, write_volatile};

fn serial_putc(c: u8) {
    unsafe {
        core::arch::asm!("out dx, al", in("dx") 0x3F8u16, in("al") c, options(nomem, nostack));
    }
}

fn serial_puts(s: &str) {
    for b in s.bytes() {
        serial_putc(b);
    }
}

#[repr(C, align(4096))]
struct FrameList {
    entries: [u32; 1024],
}

#[repr(C, align(16))]
struct Td {
    link: u32,
    cs: u32,
    token: u32,
    buffer: u32,
}

#[repr(C, align(16))]
struct Qh {
    head_link: u32,
    element_link: u32,
}

static mut FRAME_LIST: FrameList = FrameList { entries: [1; 1024] };
static mut CONTROL_QH: Qh = Qh { head_link: 1, element_link: 1 };
static mut SETUP_TD: Td = Td { link: 1, cs: 0, token: 0, buffer: 0 };
static mut DATA_TD: Td = Td { link: 1, cs: 0, token: 0, buffer: 0 };
static mut STATUS_TD: Td = Td { link: 1, cs: 0, token: 0, buffer: 0 };
static mut REPORT_TD: Td = Td { link: 1, cs: 0, token: 0, buffer: 0 };
static mut SETUP_BUF: [u8; 8] = [0; 8];
static mut DATA_BUF: [u8; 256] = [0; 256];
static mut REPORT_BUF: [u8; 8] = [0; 8];
static mut PREV_REPORT: [u8; 8] = [0; 8];

static mut UHCI_PORT: u16 = 0;
static mut KB_DEVICE_ADDR: u8 = 1;
static mut KB_MAX_PACKET0: u16 = 8;
static mut KB_ENDPOINT: u8 = 0;
static mut KB_MAX_PACKET: u16 = 0;
static mut KB_CONFIGURED: bool = false;
static mut CURRENT_LOW_SPEED: bool = false;
static mut REPORT_TOGGLE: bool = false;

static mut KB_BUFFER: [char; 64] = ['\0'; 64];
static mut KB_HEAD: usize = 0;
static mut KB_TAIL: usize = 0;

fn virt_to_phys(virt: u64) -> u64 {
    let exec_addr = crate::boot::EXEC_ADDRESS_REQUEST.response();
    match exec_addr {
        Some(resp) => {
            let vbase = resp.virtual_base;
            let pbase = resp.physical_base;
            virt - vbase + pbase
        }
        None => {
            // Fallback to legacy assumption
            virt - 0xffffffff80000000 + 0x100000
        }
    }
}

struct UhciRegs {
    base: u16,
}

impl UhciRegs {
    fn new(base: u16) -> Self {
        Self { base }
    }

    unsafe fn readw(&self, offset: u16) -> u16 {
        let mut port = x86_64::instructions::port::Port::new(self.base + offset);
        port.read()
    }

    unsafe fn writew(&self, offset: u16, value: u16) {
        let mut port = x86_64::instructions::port::Port::new(self.base + offset);
        port.write(value);
    }

    unsafe fn readl(&self, offset: u16) -> u32 {
        let low = self.readw(offset) as u32;
        let high = self.readw(offset + 2) as u32;
        low | (high << 16)
    }

    unsafe fn writel(&self, offset: u16, value: u32) {
        self.writew(offset, (value & 0xFFFF) as u16);
        self.writew(offset + 2, ((value >> 16) & 0xFFFF) as u16);
    }
}

/// Build a UHCI TD token using QEMU's non-standard bit layout.
/// QEMU's UHCI emulation extracts fields differently from the UHCI spec:
///   PID:         bits [7:0]   (actual USB PID byte value)
///   Device Addr: bits [14:8]   (7 bits)
///   Endpoint:    bits [18:15]  (4 bits)
///   Data Toggle: bit 19
///   MaxLen:      bits [31:21]  (value = actual_length - 1)
unsafe fn make_token(pid: u8, device: u8, endpoint: u8, toggle: bool, len: u16) -> u32 {
    let maxlen = if len == 0 { 0x7FF } else { (len as u32) - 1 };
    let mut token = pid as u32;                          // bits [7:0]
    token |= ((device as u32) & 0x7F) << 8;              // bits [14:8]
    token |= ((endpoint as u32) & 0xF) << 15;            // bits [18:15]
    token |= ((toggle as u32) & 1) << 19;                // bit 19
    token |= (maxlen & 0x7FF) << 21;                     // bits [31:21]
    token
}

// USB PID values (actual 8-bit PID tokens, not UHCI-encoded)
const PID_SETUP: u8 = 0x2D;
const PID_OUT: u8 = 0xE1;
const PID_IN: u8 = 0x69;

const TD_ACTIVE: u32 = 1 << 23;
const TD_IOC: u32 = 1 << 24;
const TD_LS: u32 = 1 << 26;
const TD_STALLED: u32 = 1 << 22;

unsafe fn init_td(td: *mut Td, next_phys: u32, pid: u8, device: u8, endpoint: u8, toggle: bool, len: u16, buf_phys: u32) {
    use core::ptr::write_volatile;
    write_volatile(&mut (*td).link, if next_phys == 1 { 1 } else { next_phys });
    let mut cs = TD_ACTIVE;
    if CURRENT_LOW_SPEED {
        cs |= TD_LS;
    }
    write_volatile(&mut (*td).cs, cs);
    write_volatile(&mut (*td).token, make_token(pid, device, endpoint, toggle, len));
    write_volatile(&mut (*td).buffer, buf_phys);
}

unsafe fn is_td_active(td: *const Td) -> bool {
    use core::ptr::read_volatile;
    (read_volatile(&(*td).cs) & TD_ACTIVE) != 0
}

unsafe fn td_has_error(td: *const Td) -> bool {
    let cs = (*td).cs;
    // Bitstuff, CRC/Timeout, Babble/DataBuffer errors
    (cs & ((1 << 28) | (1 << 29) | (1 << 31))) != 0
}

unsafe fn td_actual_length(td: *const Td) -> u16 {
    let cs = (*td).cs;
    ((cs & 0x7FF) + 1) as u16
}

fn wait_for_td(td: *mut Td, regs: &UhciRegs) -> bool {
    for i in 0..200000 {
        unsafe {
            if !is_td_active(td) {
                let cs = core::ptr::read_volatile(&(*td).cs);
                let has_err = td_has_error(td);
                crate::println!("[UHCI] TD done: cs={:08x} active={} error={}",
                    cs, !is_td_active(td), has_err);
                return !has_err;
            }
        }
        if i > 0 && i % 50000 == 0 {
            unsafe {
                let cs = core::ptr::read_volatile(&(*td).cs);
                let status = regs.readw(0x02);
                crate::println!("[UHCI] TD still active: cs={:08x} status={:04x}", cs, status);
            }
        }
        for _ in 0..100 {
            unsafe { core::arch::asm!("pause", options(nomem, nostack)); }
        }
    }
    unsafe {
        let cs = core::ptr::read_volatile(&(*td).cs);
        let status = regs.readw(0x02);
        crate::println!("[UHCI] TD timeout: cs={:08x} status={:04x}", cs, status);
    }
    false
}

unsafe fn insert_tds(_regs: &UhciRegs, first_td_phys: u32) {
    use core::ptr::write_volatile;
    let qh_phys = virt_to_phys(&CONTROL_QH as *const _ as u64) as u32;
    // QH head_link = 1 (terminate), element_link = first TD
    write_volatile(&mut CONTROL_QH.head_link, 1);
    write_volatile(&mut CONTROL_QH.element_link, first_td_phys);
    // Frame list points to QH (bit 1 = 1 for QH)
    for i in 0..1024 {
        write_volatile(&mut FRAME_LIST.entries[i], qh_phys | 0x2);
    }
    core::arch::asm!("sfence", options(nomem, nostack));
}

unsafe fn remove_tds() {
    use core::ptr::write_volatile;
    for i in 0..1024 {
        write_volatile(&mut FRAME_LIST.entries[i], 1);
    }
    write_volatile(&mut CONTROL_QH.element_link, 1);
    core::arch::asm!("sfence", options(nomem, nostack));
}

unsafe fn control_transfer(
    regs: &UhciRegs,
    device: u8,
    setup: &[u8; 8],
    mut data: Option<&mut [u8]>,
    data_out: bool,
    max_packet: u16,
) -> bool {
    let data_len = data.as_ref().map_or(0, |d| d.len() as u16);
    let data_ptr = data.as_mut().map_or(core::ptr::null_mut(), |d| d.as_mut_ptr());

    // Copy setup to setup buffer
    SETUP_BUF.copy_from_slice(setup);
    let setup_phys = virt_to_phys(SETUP_BUF.as_mut_ptr() as u64) as u32;

    // Copy outgoing data to data buffer
    if data_out && data_len > 0 && !data_ptr.is_null() {
        core::ptr::copy_nonoverlapping(data_ptr, DATA_BUF.as_mut_ptr(), data_len as usize);
    }
    let data_phys = virt_to_phys(DATA_BUF.as_mut_ptr() as u64) as u32;

    let setup_td_phys = virt_to_phys(&SETUP_TD as *const _ as u64) as u32;
    let data_td_phys = virt_to_phys(&DATA_TD as *const _ as u64) as u32;
    let status_td_phys = virt_to_phys(&STATUS_TD as *const _ as u64) as u32;

    crate::println!("[UHCI] Control xfer: dev={} setup_phys={:08x} data_phys={:08x}",
        device, setup_phys, data_phys);
    crate::println!("[UHCI] TD phys: setup={:08x} data={:08x} status={:08x}",
        setup_td_phys, data_td_phys, status_td_phys);

    // Build SETUP TD (toggle=0)
    init_td(&mut SETUP_TD, data_td_phys, PID_SETUP, device, 0, false, 8, setup_phys);

    // Build DATA TD (OUT or IN, toggle=1)
    if data_len > 0 {
        let data_pid = if data_out { PID_OUT } else { PID_IN };
        init_td(&mut DATA_TD, status_td_phys, data_pid, device, 0, true, data_len, data_phys);
    } else {
        // No data phase - link setup directly to status
        core::ptr::write_volatile(&mut SETUP_TD.link, status_td_phys);
        init_td(&mut DATA_TD, 1, PID_OUT, device, 0, false, 0, 0);
    }

    // Build STATUS TD:
    // - For OUT data phase: STATUS = IN
    // - For IN data phase: STATUS = OUT
    // - For no-data phase: STATUS = IN (opposite of SETUP which is host-to-device)
    let status_pid = if data_out || data_len == 0 { PID_IN } else { PID_OUT };
    init_td(&mut STATUS_TD, 1, status_pid, device, 0, true, 0, 0);

    crate::println!("[UHCI] TDs built, inserting...");

    // Insert into frame list
    insert_tds(regs, setup_td_phys);

    // Verify writes
    let flbase = regs.readl(0x08);
    let fl_entry = core::ptr::read_volatile(&FRAME_LIST.entries[0]);
    let setup_link = core::ptr::read_volatile(&SETUP_TD.link);
    let setup_cs = core::ptr::read_volatile(&SETUP_TD.cs);
    let setup_token = core::ptr::read_volatile(&SETUP_TD.token);
    crate::println!("[UHCI] Verify: FLBASE={:08x} entry[0]={:08x}", flbase, fl_entry);
    crate::println!("[UHCI] Verify: setup link={:08x} cs={:08x} token={:08x}", setup_link, setup_cs, setup_token);

    // Debug: try reading frame list via identity mapping
    let fl_phys2 = virt_to_phys(&FRAME_LIST as *const _ as u64) as u32;
    let id_addr = fl_phys2 as *const u32;
    let id_val = core::ptr::read_volatile(id_addr);
    crate::println!("[UHCI] Identity read from {:08x}: {:08x}", fl_phys2, id_val);

    crate::println!("[UHCI] Waiting for STATUS TD...");

    // Check if FRNUM is advancing
    let frnum1 = regs.readw(0x06);
    for _ in 0..5000000 { unsafe { core::arch::asm!("pause", options(nomem, nostack)); } }
    let frnum2 = regs.readw(0x06);
    crate::println!("[UHCI] FRNUM: {} -> {} (diff={})", frnum1, frnum2, frnum2.wrapping_sub(frnum1));

    // Wait for completion
    let ok = wait_for_td(&mut STATUS_TD, regs);
    crate::println!("[UHCI] Control xfer result: {}", if ok { "OK" } else { "FAIL" });

    // Remove from frame list
    remove_tds();

    // Copy incoming data back
    if ok && !data_out && data_len > 0 && !data_ptr.is_null() {
        let actual = td_actual_length(&DATA_TD);
        let len = (data_len as usize).min(actual as usize);
        core::ptr::copy_nonoverlapping(DATA_BUF.as_ptr(), data_ptr, len);
    }

    ok
}

unsafe fn read_device_descriptor(regs: &UhciRegs, device: u8, max_packet: u16, buf: &mut [u8; 18]) -> bool {
    let setup = [0x80, 6, 0, 1, 0, 0, 18, 0];
    control_transfer(regs, device, &setup, Some(buf), false, max_packet)
}

unsafe fn set_address(regs: &UhciRegs, device: u8, new_addr: u8, max_packet: u16) -> bool {
    let setup = [0x00, 5, new_addr, 0, 0, 0, 0, 0];
    control_transfer(regs, device, &setup, None, false, max_packet)
}

unsafe fn read_config_descriptor(regs: &UhciRegs, device: u8, max_packet: u16, buf: &mut [u8; 64]) -> bool {
    let setup = [0x80, 6, 0, 2, 0, 0, 64, 0];
    control_transfer(regs, device, &setup, Some(buf), false, max_packet)
}

unsafe fn set_configuration(regs: &UhciRegs, device: u8, config: u8, max_packet: u16) -> bool {
    let setup = [0x00, 9, config, 0, 0, 0, 0, 0];
    control_transfer(regs, device, &setup, None, false, max_packet)
}

unsafe fn set_protocol(regs: &UhciRegs, device: u8, protocol: u8, max_packet: u16) -> bool {
    let setup = [0x21, 0x0B, protocol, 0, 0, 0, 0, 0];
    control_transfer(regs, device, &setup, None, false, max_packet)
}

unsafe fn set_idle(regs: &UhciRegs, device: u8, duration: u8, max_packet: u16) -> bool {
    let setup = [0x21, 0x0A, duration, 0, 0, 0, 0, 0];
    control_transfer(regs, device, &setup, None, false, max_packet)
}

unsafe fn enumerate_device(regs: &UhciRegs, port: u8) {
    crate::println!("[UHCI] Enumerating device on port {}...", port);
    let mut buf = [0u8; 18];
    if read_device_descriptor(regs, 0, 64, &mut buf) {
        let b_device_class = buf[4];
        let b_device_subclass = buf[5];
        let b_device_protocol = buf[6];
        let b_max_packet_size0 = buf[7];
        let id_vendor = (buf[8] as u16) | ((buf[9] as u16) << 8);
        let id_product = (buf[10] as u16) | ((buf[11] as u16) << 8);
        let b_num_configurations = buf[17];
        crate::println!(
            "[USB] Device: vendor={:04x} product={:04x} class={} subclass={} proto={} maxpacket={}",
            id_vendor, id_product,
            b_device_class, b_device_subclass,
            b_device_protocol, b_max_packet_size0
        );

        // Set address to 1
        if set_address(regs, 0, 1, b_max_packet_size0 as u16) {
            // USB spec requires >= 2ms recovery after SET_ADDRESS
            // Wait 10ms to be safe for both QEMU and real hardware
            let fr_before = regs.readw(0x06);
            let mut fr_now = fr_before;
            while fr_now.wrapping_sub(fr_before) < 10 {
                fr_now = regs.readw(0x06);
            }
            crate::println!("[USB] Address set to 1");

            // Get configuration descriptor
            let mut cfg_buf = [0u8; 64];
            if read_config_descriptor(regs, 1, b_max_packet_size0 as u16, &mut cfg_buf) {
                parse_config_descriptor(regs, &cfg_buf, b_max_packet_size0 as u16);
            }
        }
    } else {
        crate::println!("[UHCI] Failed to read device descriptor");
    }
}

unsafe fn parse_config_descriptor(regs: &UhciRegs, buf: &[u8; 64], max_packet: u16) {
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
            let ep_addr = buf[i + 2];
            let ep_attr = buf[i + 3];
            let ep_max_packet = (buf[i + 4] as u16) | ((buf[i + 5] as u16) << 8);
            let ep_interval = buf[i + 6];
            crate::println!(
                "[USB] Endpoint: addr={:02x} attr={:02x} maxpacket={} interval={}",
                ep_addr, ep_attr, ep_max_packet, ep_interval
            );
            if ep_addr & 0x80 != 0 {
                init_keyboard_endpoint(regs, ep_addr, ep_max_packet, max_packet);
            }
        } else if dtype == 0x21 {
            crate::println!("[USB] HID descriptor found");
        }

        i += len as usize;
    }
}

unsafe fn init_keyboard_endpoint(regs: &UhciRegs, addr: u8, max_packet: u16, max_packet0: u16) {
    KB_DEVICE_ADDR = 1;
    KB_MAX_PACKET0 = max_packet0;
    KB_ENDPOINT = addr & 0x0F;
    KB_MAX_PACKET = max_packet;

    crate::println!(
        "[HID] Keyboard endpoint configured: addr={:02x} maxpacket={}",
        addr, max_packet
    );

    if set_configuration(regs, KB_DEVICE_ADDR, 1, KB_MAX_PACKET0) {
        crate::println!("[HID] Configuration set to 1");
    }

    if set_protocol(regs, KB_DEVICE_ADDR, 0, KB_MAX_PACKET0) {
        crate::println!("[HID] Boot protocol set");
    }

    let _ = set_idle(regs, KB_DEVICE_ADDR, 0, KB_MAX_PACKET0);

    KB_CONFIGURED = true;
    UHCI_PORT = regs.base;

    // Setup report TD for interrupt IN
    let report_phys = virt_to_phys(REPORT_BUF.as_mut_ptr() as u64) as u32;
    let report_td_phys = virt_to_phys(&REPORT_TD as *const _ as u64) as u32;
    REPORT_TOGGLE = false;
    init_td(&mut REPORT_TD, 1, PID_IN, KB_DEVICE_ADDR, KB_ENDPOINT, REPORT_TOGGLE, 8, report_phys);
    insert_tds(regs, report_td_phys);
}

pub fn poll_keyboard() {
    unsafe {
        if !KB_CONFIGURED {
            return;
        }

        if !is_td_active(&REPORT_TD) {
            if !td_has_error(&REPORT_TD) {
                parse_boot_report(&REPORT_BUF);
                // Toggle data toggle for next transfer
                REPORT_TOGGLE = !REPORT_TOGGLE;
            }
            // Re-arm TD with current toggle state
            let mut cs = TD_ACTIVE;
            if CURRENT_LOW_SPEED {
                cs |= TD_LS;
            }
            core::ptr::write_volatile(&mut REPORT_TD.cs, cs);
            core::ptr::write_volatile(
                &mut REPORT_TD.token,
                make_token(PID_IN, KB_DEVICE_ADDR, KB_ENDPOINT, REPORT_TOGGLE, 8),
            );
            let report_td_phys = virt_to_phys(&REPORT_TD as *const _ as u64) as u32;
            let regs = UhciRegs::new(UHCI_PORT);
            insert_tds(&regs, report_td_phys);
        }
    }
}

fn parse_boot_report(report: &[u8; 8]) {
    let modifiers = report[0];
    let shift = (modifiers & 0x22) != 0;

    unsafe {
        for i in 2..8 {
            let code = report[i];
            if code == 0 {
                continue;
            }
            // Only emit if this key was NOT in the previous report
            let already_pressed = PREV_REPORT[2..8].contains(&code);
            if !already_pressed {
                if let Some(c) = hid_code_to_ascii(code, shift) {
                    push_char(c);
                }
            }
        }
        // Save current report for next comparison
        PREV_REPORT.copy_from_slice(report);
    }
}

fn hid_code_to_ascii(code: u8, shift: bool) -> Option<char> {
    match code {
        0x04 => Some(if shift { 'A' } else { 'a' }),
        0x05 => Some(if shift { 'B' } else { 'b' }),
        0x06 => Some(if shift { 'C' } else { 'c' }),
        0x07 => Some(if shift { 'D' } else { 'd' }),
        0x08 => Some(if shift { 'E' } else { 'e' }),
        0x09 => Some(if shift { 'F' } else { 'f' }),
        0x0A => Some(if shift { 'G' } else { 'g' }),
        0x0B => Some(if shift { 'H' } else { 'h' }),
        0x0C => Some(if shift { 'I' } else { 'i' }),
        0x0D => Some(if shift { 'J' } else { 'j' }),
        0x0E => Some(if shift { 'K' } else { 'k' }),
        0x0F => Some(if shift { 'L' } else { 'l' }),
        0x10 => Some(if shift { 'M' } else { 'm' }),
        0x11 => Some(if shift { 'N' } else { 'n' }),
        0x12 => Some(if shift { 'O' } else { 'o' }),
        0x13 => Some(if shift { 'P' } else { 'p' }),
        0x14 => Some(if shift { 'Q' } else { 'q' }),
        0x15 => Some(if shift { 'R' } else { 'r' }),
        0x16 => Some(if shift { 'S' } else { 's' }),
        0x17 => Some(if shift { 'T' } else { 't' }),
        0x18 => Some(if shift { 'U' } else { 'u' }),
        0x19 => Some(if shift { 'V' } else { 'v' }),
        0x1A => Some(if shift { 'W' } else { 'w' }),
        0x1B => Some(if shift { 'X' } else { 'x' }),
        0x1C => Some(if shift { 'Y' } else { 'y' }),
        0x1D => Some(if shift { 'Z' } else { 'z' }),
        0x1E => Some(if shift { '!' } else { '1' }),
        0x1F => Some(if shift { '@' } else { '2' }),
        0x20 => Some(if shift { '#' } else { '3' }),
        0x21 => Some(if shift { '$' } else { '4' }),
        0x22 => Some(if shift { '%' } else { '5' }),
        0x23 => Some(if shift { '^' } else { '6' }),
        0x24 => Some(if shift { '&' } else { '7' }),
        0x25 => Some(if shift { '*' } else { '8' }),
        0x26 => Some(if shift { '(' } else { '9' }),
        0x27 => Some(if shift { ')' } else { '0' }),
        0x28 => Some('\n'),
        0x29 => Some('\u{1B}'),
        0x2A => Some('\u{8}'),
        0x2B => Some('\t'),
        0x2C => Some(' '),
        0x2D => Some(if shift { '_' } else { '-' }),
        0x2E => Some(if shift { '+' } else { '=' }),
        0x2F => Some(if shift { '{' } else { '[' }),
        0x30 => Some(if shift { '}' } else { ']' }),
        0x31 => Some(if shift { '|' } else { '\\' }),
        0x33 => Some(if shift { ':' } else { ';' }),
        0x34 => Some(if shift { '"' } else { '\'' }),
        0x35 => Some(if shift { '~' } else { '`' }),
        0x36 => Some(if shift { '<' } else { ',' }),
        0x37 => Some(if shift { '>' } else { '.' }),
        0x38 => Some(if shift { '?' } else { '/' }),
        _ => None,
    }
}

unsafe fn push_char(c: char) {
    let next = (KB_HEAD + 1) % KB_BUFFER.len();
    if next != KB_TAIL {
        KB_BUFFER[KB_HEAD] = c;
        KB_HEAD = next;
    }
}

pub fn get_char() -> Option<char> {
    unsafe {
        if KB_HEAD == KB_TAIL {
            return None;
        }
        let c = KB_BUFFER[KB_TAIL];
        KB_TAIL = (KB_TAIL + 1) % KB_BUFFER.len();
        Some(c)
    }
}

pub unsafe fn init_controller(base_port: u16) {
    serial_puts("UHCI: enter init_controller\n");
    let regs = UhciRegs::new(base_port);
    serial_puts("UHCI: regs created\n");
    crate::println!("[UHCI] Init at port {:#x}", base_port);

    // Full reset sequence: stop -> HCRESET -> reconfigure
    // HCRESET is required to force QEMU to drop cached SeaBIOS frame list state
    serial_puts("UHCI: stopping controller...\n");
    regs.writew(0x00, 0x0000);
    for _ in 0..100000 {
        if regs.readw(0x02) & (1 << 5) != 0 {
            break;
        }
    }
    serial_puts("UHCI: controller stopped\n");

    // Issue Host Controller Reset (bit 1 of Command)
    serial_puts("UHCI: HCRESET...\n");
    regs.writew(0x00, 0x0002);
    for _ in 0..100000 {
        if regs.readw(0x00) & 0x0002 == 0 {
            break;
        }
    }
    serial_puts("UHCI: HCRESET done\n");

    // HCRESET resets FLBASEADD, FRNUM, and port status - reconfigure everything
    serial_puts("UHCI: setting FLBASEADD...\n");
    let fl_phys = virt_to_phys(&FRAME_LIST as *const _ as u64) as u32;
    serial_puts("UHCI: FL phys computed\n");
    regs.writel(0x08, fl_phys);
    serial_puts("UHCI: FLBASEADD written\n");

    // Verify FLBASEADD readback
    let flbase_readback = regs.readl(0x08);
    crate::println!("[UHCI] FLBASEADD readback: {:08x} (expected {:08x})", flbase_readback, fl_phys);

    // Clear frame list
    serial_puts("UHCI: clearing frame list...\n");
    use core::ptr::write_volatile;
    for i in 0..1024 {
        write_volatile(&mut FRAME_LIST.entries[i], 1);
    }
    core::arch::asm!("sfence", options(nomem, nostack));
    serial_puts("UHCI: frame list cleared\n");

    // Set frame number to 0
    serial_puts("UHCI: setting FRNUM...\n");
    regs.writew(0x06, 0);
    serial_puts("UHCI: FRNUM set\n");

    // Start controller (Run + Configure Flag)
    serial_puts("UHCI: starting controller...\n");
    regs.writew(0x00, 0x0001 | (1 << 6));
    serial_puts("UHCI: controller started\n");

    crate::println!("[UHCI] Controller running");

    // Wait a bit for SOF generation before scanning ports
    for _ in 0..1000000 { core::arch::asm!("pause", options(nomem, nostack)); }

    // Scan ports (UHCI has 2 ports)
    serial_puts("UHCI: scanning ports...\n");
    for port in 1..=2u8 {
        serial_puts("UHCI: reading port status...\n");
        let portsc = regs.readw((0x10u16 + 2 * (port as u16 - 1)));
        serial_puts("UHCI: port status read\n");
        crate::println!("[UHCI] Port {} status: {:#x}", port, portsc);
        if portsc & 1 != 0 {
            let port_offset = 0x10u16 + 2 * (port as u16 - 1);
            let is_enabled = portsc & (1 << 2) != 0;

            // Always reset the port to put device back to default state (address 0)
            // Even if BIOS already enabled it, the device may have a non-zero address
            if is_enabled {
                crate::println!("[UHCI] Port {} already enabled, resetting to default state...", port);
            } else {
                crate::println!("[UHCI] Port {}: device connected, resetting...", port);
            }

            // Reset port: write only bit 9 to start reset
            crate::println!("[UHCI] Port {} starting reset...", port);
            let mut fr_before = regs.readw(0x06);
            regs.writew(port_offset, 1 << 9);
            // Wait for at least 50 frames (~50ms) using FRNUM
            let mut fr_wait = fr_before;
            while fr_wait.wrapping_sub(fr_before) < 50 {
                fr_wait = regs.readw(0x06);
            }
            // Clear reset
            regs.writew(port_offset, 0);
            // Wait another 30 frames for port to enable
            fr_before = regs.readw(0x06);
            fr_wait = fr_before;
            while fr_wait.wrapping_sub(fr_before) < 30 {
                fr_wait = regs.readw(0x06);
            }
            let mut s = regs.readw(port_offset);
            crate::println!("[UHCI] Port {} after reset: {:#x}", port, s);

            // Wait for enable
            let mut enabled = false;
            for i in 0..100000 {
                s = regs.readw(port_offset);
                if s & (1 << 2) != 0 {
                    enabled = true;
                    break;
                }
                if i > 0 && i % 20000 == 0 {
                    crate::println!("[UHCI] Port {} waiting... status={:#x}", port, s);
                }
            }
            crate::println!("[UHCI] Port {} final status: {:#x} enabled={}", port, s, enabled);

            // UHCI spec: bit 7 (Suspend) is 1=Suspend, 0=Resume.
            // If port is enabled but suspended, write enable=1, suspend=0 to resume.
            if !enabled || s & (1 << 7) != 0 {
                crate::println!("[UHCI] Port {} resuming from suspend...", port);
                // Write only bit 2 = 1: enable port, resume (suspend=0)
                regs.writew(port_offset, 1 << 2);
                for _ in 0..1000000 { core::arch::asm!("pause", options(nomem, nostack)); }
                s = regs.readw(port_offset);
                crate::println!("[UHCI] Port {} after resume: {:#x}", port, s);
                enabled = s & (1 << 2) != 0;
            }

            if enabled {
                let is_low_speed = s & (1 << 8) != 0;
                crate::println!("[UHCI] Port {} enabled ({})", port,
                    if is_low_speed { "low-speed" } else { "full-speed" });
                CURRENT_LOW_SPEED = is_low_speed;
                enumerate_device(&regs, port);
            } else {
                crate::println!("[UHCI] Port {} still disabled, skipping", port);
            }
        }
    }
    serial_puts("UHCI: init_controller done\n");
}
