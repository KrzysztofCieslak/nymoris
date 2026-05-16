use crate::{interrupts, memory, println, print, framebuffer, net, agent};

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
        "httpget" => cmd_httpget(args),
        "httptest" => cmd_httptest(),
        "agent" => cmd_agent(args),
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
    println!("  httpget  - HTTP GET request (e.g., httpget 10.0.2.2 80 /)");
    println!("  httptest - HTTP GET to example.com via gateway");
    println!("  agent    - Agent control: agent start | agent stop | agent status");
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

fn cmd_httpget(args: &str) {
    let parts: &str = args.trim();
    if parts.is_empty() {
        println!("Usage: httpget <ip> <port> <path>");
        println!("Example: httpget 10.0.2.2 80 /");
        return;
    }

    // Simple parsing: ip port path
    let mut tokens = parts.split_whitespace();
    let ip = tokens.next().unwrap_or("");
    let port_str = tokens.next().unwrap_or("80");
    let path = tokens.next().unwrap_or("/");

    let port = match parse_port(port_str) {
        Some(p) => p,
        None => {
            println!("Invalid port");
            return;
        }
    };

    // Resolve gateway MAC first if needed
    if !net::arp::is_gateway_mac_resolved() {
        net::arp::send_arp_request(&net::arp::get_gateway_ip());
        for _ in 0..10000 {
            net::poll();
            if net::arp::is_gateway_mac_resolved() {
                break;
            }
        }
    }

    if !net::arp::is_gateway_mac_resolved() {
        println!("[HTTP] Gateway MAC not resolved");
        return;
    }

    net::http::http_get(ip, port, path);
}

fn cmd_httptest() {
    // Test HTTP GET to QEMU's built-in gateway (10.0.2.2) on port 80
    // In QEMU user networking, 10.0.2.2 is the host machine
    println!("[HTTP] Testing HTTP GET to 10.0.2.2:80...");

    // Resolve gateway MAC first if needed
    if !net::arp::is_gateway_mac_resolved() {
        net::arp::send_arp_request(&net::arp::get_gateway_ip());
        for _ in 0..10000 {
            net::poll();
            if net::arp::is_gateway_mac_resolved() {
                break;
            }
        }
    }

    if !net::arp::is_gateway_mac_resolved() {
        println!("[HTTP] Gateway MAC not resolved");
        return;
    }

    net::http::http_get("10.0.2.2", 80, "/");
}

fn parse_port(s: &str) -> Option<u16> {
    let mut port = 0u16;
    for b in s.bytes() {
        if b >= b'0' && b <= b'9' {
            port = port * 10 + (b - b'0') as u16;
        } else {
            return None;
        }
    }
    if port > 0 && port <= 65535 {
        Some(port)
    } else {
        None
    }
}

fn cmd_agent(args: &str) {
    let action = args.trim();
    match action {
        "start" => agent::start(),
        "stop" => agent::stop(),
        "status" => {
            if agent::is_running() {
                println!("[AGENT] Running, state: {:?}", agent::get_state());
            } else {
                println!("[AGENT] Stopped");
            }
        }
        _ => {
            println!("Usage: agent start | agent stop | agent status");
        }
    }
}
