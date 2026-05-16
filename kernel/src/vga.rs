use core::fmt;
use lazy_static::lazy_static;
use spin::Mutex;

#[allow(dead_code)]
#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(u8)]
pub enum Color {
    Black = 0,
    Blue = 1,
    Green = 2,
    Cyan = 3,
    Red = 4,
    Magenta = 5,
    Brown = 6,
    LightGray = 7,
    DarkGray = 8,
    LightBlue = 9,
    LightGreen = 10,
    LightCyan = 11,
    LightRed = 12,
    Pink = 13,
    Yellow = 14,
    White = 15,
}

#[derive(Debug, Clone, Copy, PartialEq, Eq)]
#[repr(transparent)]
struct ColorCode(u8);

impl ColorCode {
    fn new(foreground: Color, background: Color) -> ColorCode {
        ColorCode((background as u8) << 4 | (foreground as u8))
    }
}

const BUFFER_HEIGHT: usize = 25;
const BUFFER_WIDTH: usize = 80;

pub struct Writer {
    column_position: usize,
    color_code: ColorCode,
    buffer: *mut u16,
}

unsafe impl Send for Writer {}

impl Writer {
    fn write_byte(&mut self, byte: u8) {
        match byte {
            b'\n' => self.new_line(),
            byte => {
                if self.column_position >= BUFFER_WIDTH {
                    self.new_line();
                }

                let row = BUFFER_HEIGHT - 1;
                let col = self.column_position;
                let color_code = self.color_code.0;
                let char_data: u16 = (color_code as u16) << 8 | (byte as u16);

                unsafe {
                    let offset = row * BUFFER_WIDTH + col;
                    core::ptr::write_volatile(self.buffer.add(offset), char_data);
                }

                self.column_position += 1;
            }
        }
    }

    pub fn write_string(&mut self, s: &str) {
        for byte in s.bytes() {
            match byte {
                0x20..=0x7e | b'\n' => self.write_byte(byte),
                _ => self.write_byte(0xfe),
            }
        }
    }

    pub fn set_color(&mut self, foreground: Color, background: Color) {
        self.color_code = ColorCode::new(foreground, background);
    }

    pub fn clear_screen(&mut self) {
        let blank: u16 = (self.color_code.0 as u16) << 8 | (b' ' as u16);
        unsafe {
            for i in 0..(BUFFER_HEIGHT * BUFFER_WIDTH) {
                core::ptr::write_volatile(self.buffer.add(i), blank);
            }
        }
        self.column_position = 0;
    }

    fn new_line(&mut self) {
        for row in 1..BUFFER_HEIGHT {
            for col in 0..BUFFER_WIDTH {
                unsafe {
                    let from_offset = row * BUFFER_WIDTH + col;
                    let to_offset = (row - 1) * BUFFER_WIDTH + col;
                    let char_data = core::ptr::read_volatile(self.buffer.add(from_offset));
                    core::ptr::write_volatile(self.buffer.add(to_offset), char_data);
                }
            }
        }
        self.clear_row(BUFFER_HEIGHT - 1);
        self.column_position = 0;
    }

    fn clear_row(&mut self, row: usize) {
        let blank: u16 = (self.color_code.0 as u16) << 8 | (b' ' as u16);
        unsafe {
            for col in 0..BUFFER_WIDTH {
                let offset = row * BUFFER_WIDTH + col;
                core::ptr::write_volatile(self.buffer.add(offset), blank);
            }
        }
    }
}

impl fmt::Write for Writer {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.write_string(s);
        Ok(())
    }
}

lazy_static! {
    pub static ref WRITER: Mutex<Writer> = Mutex::new(Writer {
        column_position: 0,
        color_code: ColorCode::new(Color::LightGreen, Color::Black),
        buffer: 0xb8000 as *mut u16,
    });
}

pub fn init() {
    WRITER.lock().clear_screen();
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::vga::_print(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($($arg:tt)*) => ({
        $crate::print!($($arg)*);
        $crate::print!("\n");
    });
}

/// Write a byte to COM1 serial port for debug output.
fn serial_write_byte(byte: u8) {
    unsafe {
        let port: u16 = 0x3F8;
        core::arch::asm!(
            "out dx, al",
            in("dx") port,
            in("al") byte,
            options(nomem, nostack)
        );
    }
}

/// Write a string to COM1 serial port.
fn serial_write_str(s: &str) {
    for byte in s.bytes() {
        serial_write_byte(byte);
    }
}

struct DualWriter;

impl fmt::Write for DualWriter {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        WRITER.lock().write_string(s);
        serial_write_str(s);
        Ok(())
    }
}

#[doc(hidden)]
pub fn _print(args: fmt::Arguments) {
    use core::fmt::Write;
    DualWriter.write_fmt(args).unwrap();
}
