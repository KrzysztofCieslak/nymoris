use crate::{keyboard, println, print, commands, usb};

const MAX_LINE_LENGTH: usize = 256;

static mut SHELL_BUFFER: [u8; MAX_LINE_LENGTH] = [0; MAX_LINE_LENGTH];

/// Read a byte from COM1 serial port if data is available.
fn serial_get_char() -> Option<char> {
    unsafe {
        let lsr: u8;
        core::arch::asm!(
            "in al, dx",
            out("al") lsr,
            in("dx") 0x3FDu16,
            options(nomem, nostack)
        );
        // Bit 0 of LSR = data ready
        if lsr & 0x01 != 0 {
            let data: u8;
            core::arch::asm!(
                "in al, dx",
                out("al") data,
                in("dx") 0x3F8u16,
                options(nomem, nostack)
            );
            return Some(data as char);
        }
    }
    None
}

pub fn run() -> ! {
    let buffer = unsafe { &mut SHELL_BUFFER };
    let mut pos: usize = 0;

    print!("$ ");

    loop {
        let c = usb::hid::get_char()
            .or_else(|| usb::uhci::get_char())
            .or_else(|| keyboard::get_char())
            .or_else(|| serial_get_char());

        if let Some(c) = c {
            match c {
                '\n' | '\r' => {
                    println!();
                    let line = unsafe { core::str::from_utf8_unchecked(&buffer[..pos]) };
                    commands::execute(line);
                    pos = 0;
                    for b in buffer.iter_mut() {
                        *b = 0;
                    }
                    print!("$ ");
                }
                '\u{8}' | '\u{7f}' => {
                    if pos > 0 {
                        pos -= 1;
                        buffer[pos] = 0;
                        print!("\u{8} \u{8}");
                    }
                }
                c => {
                    if pos < MAX_LINE_LENGTH - 1 && c.is_ascii() {
                        buffer[pos] = c as u8;
                        pos += 1;
                        print!("{}", c);
                    }
                }
            }
        }

        x86_64::instructions::hlt();
    }
}
