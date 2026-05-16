use core::fmt;
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

const VGA_COLORS: [(u8, u8, u8); 16] = [
    (0x00, 0x00, 0x00), // Black
    (0x00, 0x00, 0xAA), // Blue
    (0x00, 0xAA, 0x00), // Green
    (0x00, 0xAA, 0xAA), // Cyan
    (0xAA, 0x00, 0x00), // Red
    (0xAA, 0x00, 0xAA), // Magenta
    (0xAA, 0x55, 0x00), // Brown
    (0xAA, 0xAA, 0xAA), // LightGray
    (0x55, 0x55, 0x55), // DarkGray
    (0x55, 0x55, 0xFF), // LightBlue
    (0x55, 0xFF, 0x55), // LightGreen
    (0x55, 0xFF, 0xFF), // LightCyan
    (0xFF, 0x55, 0x55), // LightRed
    (0xFF, 0x55, 0xFF), // Pink
    (0xFF, 0xFF, 0x55), // Yellow
    (0xFF, 0xFF, 0xFF), // White
];

const FONT_WIDTH: usize = 8;
const FONT_HEIGHT: usize = 16;
const FONT: &[u8; 4096] = include_bytes!("font8x16.bin");

pub struct FramebufferWriter {
    buffer: *mut u8,
    width: u64,
    height: u64,
    pitch: u64,
    bpp: u16,
    cursor_x: usize,
    cursor_y: usize,
    foreground: (u8, u8, u8),
    background: (u8, u8, u8),
    red_shift: u8,
    green_shift: u8,
    blue_shift: u8,
}

unsafe impl Send for FramebufferWriter {}

impl FramebufferWriter {
    pub const fn empty() -> Self {
        FramebufferWriter {
            buffer: core::ptr::null_mut(),
            width: 0,
            height: 0,
            pitch: 0,
            bpp: 0,
            cursor_x: 0,
            cursor_y: 0,
            foreground: (0x00, 0xFF, 0x00),
            background: (0x00, 0x00, 0x00),
            red_shift: 0,
            green_shift: 0,
            blue_shift: 0,
        }
    }

    fn write_pixel(&mut self, x: usize, y: usize, r: u8, g: u8, b: u8) {
        if self.buffer.is_null() {
            return;
        }
        if x >= self.width as usize || y >= self.height as usize {
            return;
        }
        let offset = y * self.pitch as usize + x * (self.bpp as usize / 8);
        let pixel = ((r as u32) << self.red_shift)
                  | ((g as u32) << self.green_shift)
                  | ((b as u32) << self.blue_shift);
        let bytes = pixel.to_le_bytes();
        unsafe {
            let ptr = self.buffer.add(offset);
            match self.bpp {
                32 => {
                    core::ptr::write_volatile(ptr as *mut u32, pixel);
                }
                24 => {
                    core::ptr::write_volatile(ptr, bytes[0]);
                    core::ptr::write_volatile(ptr.add(1), bytes[1]);
                    core::ptr::write_volatile(ptr.add(2), bytes[2]);
                }
                16 => {
                    core::ptr::write_volatile(ptr as *mut u16, pixel as u16);
                }
                _ => {}
            }
        }
    }

    fn draw_char(&mut self, c: char, x: usize, y: usize, fg: (u8, u8, u8), bg: (u8, u8, u8)) {
        let c = c as usize;
        let font_offset = c * FONT_HEIGHT;
        for row in 0..FONT_HEIGHT {
            let font_byte = FONT[font_offset + row];
            for col in 0..FONT_WIDTH {
                let bit = 7 - col;
                let pixel_on = (font_byte >> bit) & 1;
                let (r, g, b) = if pixel_on != 0 { fg } else { bg };
                self.write_pixel(x + col, y + row, r, g, b);
            }
        }
    }

    fn scroll(&mut self) {
        if self.buffer.is_null() {
            return;
        }
        let row_bytes = self.pitch as usize * FONT_HEIGHT;
        let total_bytes = self.pitch as usize * self.height as usize;
        unsafe {
            core::ptr::copy(
                self.buffer.add(row_bytes),
                self.buffer,
                total_bytes - row_bytes,
            );
            core::ptr::write_bytes(
                self.buffer.add(total_bytes - row_bytes),
                0,
                row_bytes,
            );
        }
    }

    fn new_line(&mut self) {
        self.cursor_x = 0;
        self.cursor_y += FONT_HEIGHT;
        if self.cursor_y + FONT_HEIGHT > self.height as usize {
            self.scroll();
            self.cursor_y = self.cursor_y.saturating_sub(FONT_HEIGHT);
            if self.cursor_y + FONT_HEIGHT > self.height as usize {
                self.cursor_y = self.height as usize - FONT_HEIGHT;
            }
        }
    }

    fn write_byte(&mut self, byte: u8) {
        match byte {
            b'\n' => self.new_line(),
            b'\x08' => {
                // Backspace: move cursor back and clear the character
                if self.cursor_x >= FONT_WIDTH {
                    self.cursor_x -= FONT_WIDTH;
                    let bg = self.background;
                    for row in 0..FONT_HEIGHT {
                        for col in 0..FONT_WIDTH {
                            self.write_pixel(self.cursor_x + col, self.cursor_y + row, bg.0, bg.1, bg.2);
                        }
                    }
                }
            }
            byte => {
                if self.cursor_x + FONT_WIDTH > self.width as usize {
                    self.new_line();
                }
                let fg = self.foreground;
                let bg = self.background;
                self.draw_char(byte as char, self.cursor_x, self.cursor_y, fg, bg);
                self.cursor_x += FONT_WIDTH;
            }
        }
    }

    pub fn write_string(&mut self, s: &str) {
        for byte in s.bytes() {
            match byte {
                0x20..=0x7e | b'\n' | b'\x08' => self.write_byte(byte),
                _ => self.write_byte(0xfe),
            }
        }
    }

    pub fn set_color(&mut self, foreground: Color, background: Color) {
        self.foreground = VGA_COLORS[foreground as usize];
        self.background = VGA_COLORS[background as usize];
    }

    pub fn clear_screen(&mut self) {
        if self.buffer.is_null() {
            self.cursor_x = 0;
            self.cursor_y = 0;
            return;
        }
        let total_bytes = self.pitch as usize * self.height as usize;
        unsafe {
            core::ptr::write_bytes(self.buffer, 0, total_bytes);
        }
        self.cursor_x = 0;
        self.cursor_y = 0;
    }
}

impl fmt::Write for FramebufferWriter {
    fn write_str(&mut self, s: &str) -> fmt::Result {
        self.write_string(s);
        Ok(())
    }
}

pub static WRITER: Mutex<FramebufferWriter> = Mutex::new(FramebufferWriter::empty());

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

pub fn init() {
    let response = crate::boot::FRAMEBUFFER_REQUEST.response();
    if let Some(response) = response {
        let fbs = response.framebuffers();
        if let Some(fb) = fbs.first() {
            let mut writer = WRITER.lock();
            writer.buffer = fb.address() as *mut u8;
            writer.width = fb.width;
            writer.height = fb.height;
            writer.pitch = fb.pitch;
            writer.bpp = fb.bpp;
            writer.red_shift = fb.red_mask_shift;
            writer.green_shift = fb.green_mask_shift;
            writer.blue_shift = fb.blue_mask_shift;
            writer.cursor_x = 0;
            writer.cursor_y = 0;
            writer.foreground = VGA_COLORS[Color::LightGreen as usize];
            writer.background = VGA_COLORS[Color::Black as usize];
            drop(writer);
            return;
        }
    }
}

#[macro_export]
macro_rules! print {
    ($($arg:tt)*) => ($crate::framebuffer::_print(format_args!($($arg)*)));
}

#[macro_export]
macro_rules! println {
    () => ($crate::print!("\n"));
    ($($arg:tt)*) => ({
        $crate::print!($($arg)*);
        $crate::print!("\n");
    });
}
