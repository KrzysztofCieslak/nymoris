use core::ptr::{read_volatile, write_volatile};

static mut KB_ENDPOINT: u8 = 0;
static mut KB_MAX_PACKET: u16 = 0;
static mut KB_INTERVAL: u8 = 0;
static mut KB_MAX_PACKET0: u16 = 0;
static mut KB_DEVICE_ADDR: u8 = 1;
static mut KB_CONFIGURED: bool = false;

static mut REPORT_BUFFER: [u8; 8] = [0; 8];
static mut PREV_REPORT: [u8; 8] = [0; 8];
static mut REPORT_QTD: super::ehci::Qtd = super::ehci::Qtd {
    next_qtd: 0,
    alt_next_qtd: 1,
    token: 0,
    buffers: [0; 5],
    _padding: [0; 3],
};
static mut REPORT_QH: super::ehci::QueueHead = super::ehci::QueueHead {
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

static mut KB_BUFFER: [char; 64] = ['\0'; 64];
static mut KB_HEAD: usize = 0;
static mut KB_TAIL: usize = 0;

pub fn init_keyboard_endpoint(addr: u8, max_packet: u16, interval: u8, max_packet0: u16) {
    unsafe {
        KB_ENDPOINT = addr;
        KB_MAX_PACKET = max_packet;
        KB_INTERVAL = interval;
        KB_MAX_PACKET0 = max_packet0;

        crate::println!(
            "[HID] Keyboard endpoint configured: addr={:02x} maxpacket={} interval={}",
            addr, max_packet, interval
        );

        // Set configuration to 1
        let set_config = [0x00, 9, 1, 0, 0, 0, 0, 0];
        if super::ehci::control_transfer(
            &unsafe { super::ehci::EhciRegs::new(get_bar()) },
            KB_DEVICE_ADDR,
            0,
            &set_config,
            None,
            max_packet0,
        ) {
            crate::println!("[HID] Configuration set to 1");
        }

        // Set boot protocol
        let set_protocol = [0x21, 0x0B, 0, 0, 0, 0, 0, 0];
        if super::ehci::control_transfer(
            &unsafe { super::ehci::EhciRegs::new(get_bar()) },
            KB_DEVICE_ADDR,
            0,
            &set_protocol,
            None,
            max_packet0,
        ) {
            crate::println!("[HID] Boot protocol set");
        }

        // Set idle rate to 0 (no repeat)
        let set_idle = [0x21, 0x0A, 0, 0, 0, 0, 0, 0];
        let _ = super::ehci::control_transfer(
            &unsafe { super::ehci::EhciRegs::new(get_bar()) },
            KB_DEVICE_ADDR,
            0,
            &set_idle,
            None,
            max_packet0,
        );

        KB_CONFIGURED = true;
        setup_interrupt_transfer();
    }
}

fn get_bar() -> u64 {
    super::ehci::get_bar()
}

unsafe fn setup_interrupt_transfer() {
    let report_phys = super::ehci::virt_to_phys(REPORT_BUFFER.as_mut_ptr() as u64) as u32;

    // Initialize report qTD (PID = 1 for IN)
    REPORT_QTD.next_qtd = 1;
    REPORT_QTD.alt_next_qtd = 1;
    REPORT_QTD.token = ((8u32) << 16)
        | (0x3u32 << 10)
        | (1u32 << 8)  // PID = IN
        | (1u32 << 7); // ACTIVE
    REPORT_QTD.buffers[0] = report_phys;
    for i in 1..5 {
        REPORT_QTD.buffers[i] = 0;
    }

    // Initialize QH for interrupt IN endpoint
    let qh_addr = super::ehci::virt_to_phys(&REPORT_QH as *const _ as u64
    ) as u32;
    REPORT_QH.horizontal_link = 1;
    // endpoint characteristics:
    // device addr, endpoint number, speed=2 (high), max packet
    let chars = (KB_DEVICE_ADDR as u32)
        | (((KB_ENDPOINT & 0x0F) as u32) << 7)
        | (2u32 << 12)
        | (1u32 << 14)
        | ((KB_MAX_PACKET as u32) << 16);
    REPORT_QH.endpoint_chars = chars;
    REPORT_QH.endpoint_caps = 0;
    REPORT_QH.current_qtd = 0;
    REPORT_QH.next_qtd = super::ehci::virt_to_phys(&REPORT_QTD as *const _ as u64
    ) as u32;

    // For simplicity, add to async list instead of periodic
    // (this is not ideal but works for low-speed polling)
}

pub fn poll_keyboard() {
    unsafe {
        if !KB_CONFIGURED {
            return;
        }

        // Check if previous transfer completed
        let token = read_volatile(&REPORT_QTD.token);
        if token & (1 << 7) != 0 {
            // Still active
            return;
        }

        // Transfer completed - parse report
        if token & 0x7C == 0 {
            // No errors
            parse_boot_report(&REPORT_BUFFER);
        }

        // Re-arm the transfer
        write_volatile(&mut REPORT_QTD.token,
            ((8u32) << 16)
                | (0x3u32 << 10)
                | (1u32 << 8)
                | (1u32 << 7)
        );
    }
}

fn parse_boot_report(report: &[u8; 8]) {
    // Boot protocol report format:
    // [0]: modifier keys (Ctrl, Shift, Alt, GUI)
    // [1]: reserved
    // [2..7]: keycodes (up to 6 simultaneous keys)
    let modifiers = report[0];
    let shift = (modifiers & 0x22) != 0; // left or right shift

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
        0x28 => Some('\n'),  // Enter
        0x29 => Some('\u{1B}'), // Escape
        0x2A => Some('\u{8}'),  // Backspace
        0x2B => Some('\t'),      // Tab
        0x2C => Some(' '),
        0x2D => Some(if shift { '_' } else { '-' }),
        0x2E => Some(if shift { '+' } else { '=' }),
        0x2F => Some(if shift { '{' } else { '[' }),
        0x30 => Some(if shift { '}' } else { ']' }),
        0x31 => Some(if shift { '|' } else { '\\' }),
        0x33 => Some(if shift { ':' } else { ';' }),
        0x34 => Some(if shift { '\"' } else { '\'' }),
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
