use crate::println;

/// A file in the ramdisk.
#[derive(Clone, Copy)]
pub struct RamdiskFile {
    pub name: &'static str,
    pub data: &'static [u8],
}

const MAX_FILES: usize = 8;

static mut FILES: [Option<RamdiskFile>; MAX_FILES] = [None; MAX_FILES];
static mut FILE_COUNT: usize = 0;

pub unsafe fn init() {
    FILE_COUNT = 0;
    println!("[RAMDISK] Initialized (empty)");
}

/// Register a file in the ramdisk.
pub unsafe fn register(name: &'static str, data: &'static [u8]) {
    if FILE_COUNT >= MAX_FILES {
        println!("[RAMDISK] Warning: too many files, skipping '{}'", name);
        return;
    }
    FILES[FILE_COUNT] = Some(RamdiskFile { name, data });
    FILE_COUNT += 1;
    println!("[RAMDISK] Registered '{}' ({} bytes)", name, data.len());
}

/// Find a file by name.
pub fn find(name: &str) -> Option<&'static RamdiskFile> {
    unsafe {
        for i in 0..FILE_COUNT {
            if let Some(ref file) = FILES[i] {
                if file.name == name {
                    return Some(file);
                }
            }
        }
        None
    }
}

/// List all files in the ramdisk.
pub unsafe fn list() {
    if FILE_COUNT == 0 {
        println!("Ramdisk is empty");
        return;
    }
    println!("Ramdisk files:");
    for i in 0..FILE_COUNT {
        if let Some(ref file) = FILES[i] {
            println!("  {} ({} bytes)", file.name, file.data.len());
        }
    }
}
