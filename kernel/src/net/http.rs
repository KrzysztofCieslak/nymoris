use crate::net::tcp;
use crate::println;
use crate::print;

fn parse_ip(ip_str: &str) -> Option<[u8; 4]> {
    let mut ip = [0u8; 4];
    let mut octet_idx = 0;
    let mut current = 0u16;

    for b in ip_str.bytes() {
        if b == b'.' {
            if octet_idx >= 4 || current > 255 {
                return None;
            }
            ip[octet_idx] = current as u8;
            octet_idx += 1;
            current = 0;
        } else if b >= b'0' && b <= b'9' {
            current = current * 10 + (b - b'0') as u16;
        } else {
            return None;
        }
    }

    if octet_idx != 3 || current > 255 {
        return None;
    }
    ip[3] = current as u8;
    Some(ip)
}

fn send_http_request(method: &[u8], host: &str, path: &str, body: Option<&str>) -> bool {
    let mut request = [0u8; 2048];
    let mut pos = 0;

    // Method and path
    for b in method {
        request[pos] = *b;
        pos += 1;
    }
    request[pos] = b' ';
    pos += 1;
    for b in path.bytes() {
        request[pos] = b;
        pos += 1;
    }
    request[pos] = b' ';
    pos += 1;
    for b in b"HTTP/1.1\r\n" {
        request[pos] = *b;
        pos += 1;
    }

    // Host header
    for b in b"Host: " {
        request[pos] = *b;
        pos += 1;
    }
    for b in host.bytes() {
        request[pos] = b;
        pos += 1;
    }
    for b in b"\r\n" {
        request[pos] = *b;
        pos += 1;
    }

    // User-Agent
    for b in b"User-Agent: Nymoris/0.1\r\n" {
        request[pos] = *b;
        pos += 1;
    }

    // Content-Type for POST
    if body.is_some() {
        for b in b"Content-Type: application/json\r\n" {
            request[pos] = *b;
            pos += 1;
        }
    }

    // Connection close
    for b in b"Connection: close\r\n" {
        request[pos] = *b;
        pos += 1;
    }

    // Content-Length if body present
    if let Some(b) = body {
        for byte in b"Content-Length: " {
            request[pos] = *byte;
            pos += 1;
        }
        let mut len_buf = [0u8; 10];
        let mut len_val = b.len();
        let mut len_idx = 0;
        if len_val == 0 {
            len_buf[0] = b'0';
            len_idx = 1;
        } else {
            let mut temp = len_val;
            while temp > 0 {
                len_idx += 1;
                temp /= 10;
            }
            let mut j = len_idx;
            while len_val > 0 {
                j -= 1;
                len_buf[j] = b'0' + (len_val % 10) as u8;
                len_val /= 10;
            }
        }
        for i in 0..len_idx {
            request[pos] = len_buf[i];
            pos += 1;
        }
        for byte in b"\r\n" {
            request[pos] = *byte;
            pos += 1;
        }
    }

    // Empty line
    for b in b"\r\n" {
        request[pos] = *b;
        pos += 1;
    }

    // Body
    if let Some(b) = body {
        for byte in b.bytes() {
            request[pos] = byte;
            pos += 1;
        }
    }

    tcp::send(&request[..pos])
}

pub fn http_get(ip_str: &str, port: u16, path: &str) {
    let ip = match parse_ip(ip_str) {
        Some(ip) => ip,
        None => {
            println!("[HTTP] Invalid IP address");
            return;
        }
    };

    let host = ip_str;

    if !tcp::connect(&ip, port) {
        println!("[HTTP] Failed to connect to {}:{}", ip_str, port);
        return;
    }

    println!("[HTTP] Connected, sending GET {}...", path);

    if !send_http_request(b"GET", host, path, None) {
        println!("[HTTP] Failed to send request");
        tcp::close();
        return;
    }

    // Receive response
    println!("[HTTP] Waiting for response...");
    receive_and_print_response();

    tcp::close();
}

pub fn http_post(ip_str: &str, port: u16, path: &str, body: &str) {
    let ip = match parse_ip(ip_str) {
        Some(ip) => ip,
        None => {
            println!("[HTTP] Invalid IP address");
            return;
        }
    };

    let host = ip_str;

    if !tcp::connect(&ip, port) {
        println!("[HTTP] Failed to connect to {}:{}", ip_str, port);
        return;
    }

    println!("[HTTP] Connected, sending POST {}...", path);

    if !send_http_request(b"POST", host, path, Some(body)) {
        println!("[HTTP] Failed to send request");
        tcp::close();
        return;
    }

    println!("[HTTP] Waiting for response...");
    receive_and_print_response();

    tcp::close();
}

fn receive_and_print_response() {
    let mut response_buf = [0u8; 4096];
    let mut total = 0usize;
    let mut header_done = false;
    let mut status_printed = false;

    // Poll for data
    for _ in 0..100000 {
        let len = tcp::recv_bytes(&mut response_buf[total..]);
        if len > 0 {
            total += len;

            if !header_done {
                // Look for end of headers
                let buf = &response_buf[..total];
                for i in 3..buf.len() {
                    if buf[i - 3] == b'\r' && buf[i - 2] == b'\n' && buf[i - 1] == b'\r' && buf[i] == b'\n' {
                        header_done = true;

                        // Print status line and headers
                        if !status_printed {
                            println!("[HTTP] === Response ===");
                            for j in 0..=i {
                                print!("{}", buf[j] as char);
                            }
                            status_printed = true;
                        }
                        break;
                    }
                }
            }
        }

        // Small delay
        for _ in 0..100 {
            crate::net::poll();
        }

        // If connection closed and we have data, stop
        if !tcp::is_connected() && total > 0 {
            break;
        }
    }

    if total > 0 && header_done {
        // Print body
        let buf = &response_buf[..total];
        for i in 3..buf.len() {
            if buf[i - 3] == b'\r' && buf[i - 2] == b'\n' && buf[i - 1] == b'\r' && buf[i] == b'\n' {
                println!("[HTTP] === Body ===");
                for j in (i + 1)..total {
                    print!("{}", buf[j] as char);
                }
                println!();
                break;
            }
        }
    } else if total > 0 {
        println!("[HTTP] === Raw Response ===");
        for i in 0..total {
            print!("{}", response_buf[i] as char);
        }
        println!();
    } else {
        println!("[HTTP] No response received");
    }
}
