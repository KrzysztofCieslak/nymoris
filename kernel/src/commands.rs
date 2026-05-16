use crate::{interrupts, memory, println, print, framebuffer};

pub fn execute(line: &str) {
    let line = line.trim();
    if line.is_empty() {
        return;
    }

    let mut parts = line.split_whitespace();
    let cmd = parts.next().unwrap_or("");
    let args: &str = line[cmd.len()..].trim_start();

    match cmd {
        "help" => cmd_help(),
        "echo" => cmd_echo(args),
        "clear" => cmd_clear(),
        "reboot" => cmd_reboot(),
        "halt" => cmd_halt(),
        "meminfo" => cmd_meminfo(),
        "about" => cmd_about(),
        "panic" => cmd_panic(),
        _ => println!("Unknown command: '{}'. Type 'help' for list.", cmd),
    }
}

fn cmd_help() {
    println!("Available commands:");
    println!("  help     - Show this help message");
    println!("  echo     - Print text to screen");
    println!("  clear    - Clear the screen");
    println!("  reboot   - Reboot the computer");
    println!("  halt     - Halt the CPU");
    println!("  meminfo  - Show memory map information");
    println!("  about    - About KACOS");
    println!("  panic    - Trigger a kernel panic (for testing)");
}

fn cmd_echo(args: &str) {
    println!("{}", args);
}

fn cmd_clear() {
    framebuffer::WRITER.lock().clear_screen();
}

fn cmd_reboot() {
    println!("Rebooting...");
    unsafe {
        let mut port: x86_64::instructions::port::Port<u8> = x86_64::instructions::port::Port::new(0x64);
        port.write(0xFE);
    }
    interrupts::hlt_loop();
}

fn cmd_halt() {
    println!("Halting CPU...");
    interrupts::hlt_loop();
}

fn cmd_meminfo() {
    memory::print_memmap();
}

fn cmd_about() {
    println!("==============================");
    println!(" KACOS - Krzysztof's Amateur");
    println!("        Computer OS");
    println!("==============================");
    println!("Version: 0.1.0");
    println!("Built with: Rust");
    println!("Target: x86_64");
    println!("Bootloader: Limine");
    println!("==============================");
}

fn cmd_panic() {
    panic!("Test panic triggered by user");
}
