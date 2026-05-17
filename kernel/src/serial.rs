//! COM1 serial port driver with interrupt-driven input buffer.

static mut SERIAL_BUFFER: [char; 256] = ['\0'; 256];
static mut SERIAL_HEAD: usize = 0;
static mut SERIAL_TAIL: usize = 0;

/// Push a character into the serial input ring buffer.
pub unsafe fn push_char(c: char) {
    let next = (SERIAL_HEAD + 1) % SERIAL_BUFFER.len();
    if next != SERIAL_TAIL {
        SERIAL_BUFFER[SERIAL_HEAD] = c;
        SERIAL_HEAD = next;
    }
}

/// Pop a character from the serial input ring buffer.
pub fn get_char() -> Option<char> {
    unsafe {
        if SERIAL_HEAD == SERIAL_TAIL {
            return None;
        }
        let c = SERIAL_BUFFER[SERIAL_TAIL];
        SERIAL_TAIL = (SERIAL_TAIL + 1) % SERIAL_BUFFER.len();
        Some(c)
    }
}

/// Enable COM1 receiver interrupts (IER bit 0).
pub fn init() {
    unsafe {
        let mut port: x86_64::instructions::port::Port<u8> =
            x86_64::instructions::port::Port::new(0x3F8 + 1);
        port.write(0x01);
    }
}
