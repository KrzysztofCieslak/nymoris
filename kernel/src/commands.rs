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
        "alloctest" => cmd_alloctest(),
        "heaptest" => cmd_heaptest(),
        "pagetest" => cmd_pagetest(),
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
    println!("  alloctest - Test page frame allocator");
    println!("  heaptest  - Test kernel heap allocator");
    println!("  pagetest  - Test paging and virtual memory");
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

fn cmd_heaptest() {
    println!("[HEAPTEST] Testing kernel heap allocator...");

    use alloc::vec::Vec;
    use alloc::string::String;

    // Test 1: Vec allocation
    {
        let mut v = Vec::new();
        for i in 0..10 {
            v.push(i * i);
        }
        println!("[HEAPTEST] Vec: {:?}", v);
    }

    // Test 2: String allocation
    {
        let mut s = String::from("Hello from ");
        s.push_str("the heap!");
        println!("[HEAPTEST] String: {}", s);
    }

    // Test 3: Box
    {
        let b = alloc::boxed::Box::new(42);
        println!("[HEAPTEST] Box: {}", *b);
    }

    println!("[HEAPTEST] PASS: all heap allocations succeeded");
}

fn cmd_alloctest() {
    println!("[ALLOCTEST] Testing page frame allocator...");

    let (used_before, total) = memory::allocator::get_stats();
    println!("[ALLOCTEST] Before: {}/{} pages used", used_before, total);

    // Allocate a few pages
    let mut pages = [0u64; 5];
    for (i, page) in pages.iter_mut().enumerate() {
        match memory::allocator::alloc_page() {
            Some(p) => {
                println!("[ALLOCTEST] Allocated page {}: {:x}", i, p);
                *page = p;
            }
            None => {
                println!("[ALLOCTEST] Failed to allocate page {}", i);
            }
        }
    }

    let (used_mid, _) = memory::allocator::get_stats();
    println!("[ALLOCTEST] After alloc: {}/{} pages used", used_mid, total);

    // Free them in reverse order
    for (i, page) in pages.iter().enumerate().rev() {
        if *page != 0 {
            memory::allocator::free_page(*page);
            println!("[ALLOCTEST] Freed page {}: {:x}", i, page);
        }
    }

    let (used_after, _) = memory::allocator::get_stats();
    println!("[ALLOCTEST] After free: {}/{} pages used", used_after, total);

    if used_before == used_after {
        println!("[ALLOCTEST] PASS: all pages freed correctly");
    } else {
        println!("[ALLOCTEST] FAIL: page leak detected");
    }
}

fn cmd_pagetest() {
    use x86_64::structures::paging::PageTableFlags;

    println!("[PAGETEST] Testing paging...");

    // Allocate a physical page
    let phys = match memory::allocator::alloc_page() {
        Some(p) => p,
        None => {
            println!("[PAGETEST] FAIL: could not allocate physical page");
            return;
        }
    };

    // Choose a test virtual address in the lower half (not used by kernel)
    let test_virt: u64 = 0x1000; // first page after null

    println!("[PAGETEST] Mapping virt {:x} -> phys {:x}", test_virt, phys);

    unsafe {
        // Write a known value to the physical page
        core::ptr::write_bytes(phys as *mut u8, 0xAB, 4096);

        // Map the page
        let flags = PageTableFlags::PRESENT | PageTableFlags::WRITABLE;
        if !memory::paging::map_page(test_virt, phys, flags) {
            println!("[PAGETEST] FAIL: map_page returned false");
            memory::allocator::free_page(phys);
            return;
        }

        // Verify translation
        match memory::paging::virt_to_phys_current(test_virt) {
            Some(translated) => {
                if translated == phys {
                    println!("[PAGETEST] Translation OK: {:x} -> {:x}", test_virt, translated);
                } else {
                    println!("[PAGETEST] FAIL: expected phys {:x}, got {:x}", phys, translated);
                }
            }
            None => {
                println!("[PAGETEST] FAIL: virt_to_phys_current returned None");
            }
        }

        // Verify we can read the mapped virtual address
        let val = core::ptr::read_volatile(test_virt as *const u8);
        if val == 0xAB {
            println!("[PAGETEST] Readback OK: 0xAB at virt {:x}", test_virt);
        } else {
            println!("[PAGETEST] FAIL: expected 0xAB, got 0x{:x}", val);
        }

        // Unmap
        memory::paging::unmap_page(test_virt);
        println!("[PAGETEST] Unmapped");

        // Verify translation is gone
        match memory::paging::virt_to_phys_current(test_virt) {
            Some(p) => println!("[PAGETEST] FAIL: still mapped to {:x}", p),
            None => println!("[PAGETEST] Translation removed OK"),
        }
    }

    memory::allocator::free_page(phys);
    println!("[PAGETEST] PASS");
}
