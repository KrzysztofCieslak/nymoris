use crate::{interrupts, memory, println, print, framebuffer, net};

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
        "ping" => cmd_ping(args),
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
    println!("  about    - About Nymoris");
    println!("  panic    - Trigger a kernel panic (for testing)");
    println!("  ping     - Ping a remote host (e.g., ping 10.0.2.2)");
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
    println!(" Nymoris - Agentic AI Operating System");
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

fn cmd_ping(args: &str) {
    let ip_str = args.trim();
    if ip_str.is_empty() {
        println!("Usage: ping <ip_address>");
        return;
    }

    let mut ip = [0u8; 4];
    let mut octet_idx = 0;
    let mut current = 0u16;
    for b in ip_str.bytes() {
        if b == b'.' {
            if octet_idx >= 4 || current > 255 {
                println!("Invalid IP address");
                return;
            }
            ip[octet_idx] = current as u8;
            octet_idx += 1;
            current = 0;
        } else if b >= b'0' && b <= b'9' {
            current = current * 10 + (b - b'0') as u16;
        } else {
            println!("Invalid IP address");
            return;
        }
    }
    if octet_idx != 3 || current > 255 {
        println!("Invalid IP address");
        return;
    }
    ip[3] = current as u8;

    // Resolve gateway MAC first if needed
    if !net::arp::is_gateway_mac_resolved() && ip != net::arp::get_our_ip() {
        net::arp::send_arp_request(&net::arp::get_gateway_ip());
        // Wait a bit for ARP reply (poll network)
        for _ in 0..10000 {
            net::poll();
            if net::arp::is_gateway_mac_resolved() {
                break;
            }
        }
    }

    if !net::arp::is_gateway_mac_resolved() {
        println!("[PING] Gateway MAC not resolved yet");
        return;
    }

    net::icmp::send_ping(&ip);

    // Poll for reply
    for _ in 0..50000 {
        net::poll();
        if !net::icmp::is_pending() {
            break;
        }
    }

    if net::icmp::is_pending() {
        println!("[PING] Request timed out");
    }
}
