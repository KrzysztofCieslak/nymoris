use lazy_static::lazy_static;
use pc_keyboard::{layouts, DecodedKey, EventDecoder, HandleControl, KeyCode, KeyEvent, KeyState, Keyboard, ScancodeSet1};
use spin::Mutex;

lazy_static! {
    static ref KEYBOARD: Mutex<Keyboard<layouts::Us104Key, ScancodeSet1>> = Mutex::new(
        Keyboard::new(ScancodeSet1::new(), layouts::Us104Key, HandleControl::Ignore)
    );
}

static mut KEYBOARD_BUFFER: [char; 256] = ['\0'; 256];
static mut BUFFER_HEAD: usize = 0;
static mut BUFFER_TAIL: usize = 0;

pub fn init() {}

pub fn handle_scancode(scancode: u8) {
    let mut keyboard = KEYBOARD.lock();
    if let Ok(Some(key_event)) = keyboard.add_byte(scancode) {
        if let Some(key) = keyboard.process_keyevent(key_event) {
            match key {
                DecodedKey::Unicode(character) => {
                    unsafe { push_char(character); }
                }
                DecodedKey::RawKey(_) => {}
            }
        }
    }
}

unsafe fn push_char(c: char) {
    let next = (BUFFER_HEAD + 1) % KEYBOARD_BUFFER.len();
    if next != BUFFER_TAIL {
        KEYBOARD_BUFFER[BUFFER_HEAD] = c;
        BUFFER_HEAD = next;
    }
}

pub fn get_char() -> Option<char> {
    unsafe {
        if BUFFER_HEAD == BUFFER_TAIL {
            return None;
        }
        let c = KEYBOARD_BUFFER[BUFFER_TAIL];
        BUFFER_TAIL = (BUFFER_TAIL + 1) % KEYBOARD_BUFFER.len();
        Some(c)
    }
}
