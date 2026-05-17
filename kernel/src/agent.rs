use crate::net::http;
use crate::println;
use crate::commands;
use crate::net::arp;
use crate::net;

// Agent configuration
const POLL_INTERVAL_TICKS: u64 = 600; // ~600 ticks (~30s at default 18Hz PIT)
const API_IP: [u8; 4] = [10, 0, 2, 2];
const API_PORT: u16 = 8765;
const API_PATH: &str = "/api/chat";

#[derive(Debug, Clone, Copy, PartialEq)]
pub enum AgentState {
    Idle,
    Polling,
    Executing,
    Reporting,
}

static mut AGENT_STATE: AgentState = AgentState::Idle;
static mut AGENT_ENABLED: bool = false;
static mut LAST_POLL_TICKS: u64 = 0;
static mut TICKS: u64 = 0;

// Request/response buffers
const BUFFER_SIZE: usize = 4096;
static mut REQUEST_BUF: [u8; BUFFER_SIZE] = [0; BUFFER_SIZE];
static mut RESPONSE_BUF: [u8; BUFFER_SIZE] = [0; BUFFER_SIZE];

// System prompt describing available tools
const SYSTEM_PROMPT: &str = "You are an agent running inside Nymoris, an agentic AI operating system. \
You can execute shell commands to interact with the system. \
Available commands: help, echo, clear, reboot, halt, meminfo, about, panic, ping, httpget, httptest. \
Respond with a single command to execute, or 'none' if no action is needed. \
Format your response as: COMMAND: <command>";

pub fn init() {
    println!("[AGENT] Agent subsystem initialized");
    println!("[AGENT] Use 'agent start' to begin polling");
}

pub fn start() {
    unsafe {
        if AGENT_ENABLED {
            println!("[AGENT] Already running");
            return;
        }
        AGENT_ENABLED = true;
        AGENT_STATE = AgentState::Idle;
        LAST_POLL_TICKS = TICKS;
        println!("[AGENT] Started - polling every {} ticks (~{}s)", POLL_INTERVAL_TICKS, POLL_INTERVAL_TICKS / 18);
    }
}

pub fn stop() {
    unsafe {
        AGENT_ENABLED = false;
        AGENT_STATE = AgentState::Idle;
        println!("[AGENT] Stopped");
    }
}

pub fn is_running() -> bool {
    unsafe { AGENT_ENABLED }
}

pub fn get_state() -> AgentState {
    unsafe { AGENT_STATE }
}

// Called from timer interrupt or main loop
pub fn tick() {
    unsafe {
        TICKS += 1;

        if !AGENT_ENABLED {
            return;
        }

        // Simple timing: each tick is roughly 1ms (based on timer interrupt)
        // In reality this depends on timer frequency, but for MVP this is fine
        if TICKS - LAST_POLL_TICKS < POLL_INTERVAL_TICKS {
            return;
        }
        LAST_POLL_TICKS = TICKS;

        match AGENT_STATE {
            AgentState::Idle => {
                AGENT_STATE = AgentState::Polling;
                poll_api();
                AGENT_STATE = AgentState::Idle;
            }
            _ => {
                // If we're in the middle of something, skip this cycle
                println!("[AGENT] Skipping poll - still processing previous request");
            }
        }
    }
}

fn poll_api() {
    println!("[AGENT] Polling API...");

    // Build JSON request body
    let body = build_request_body();

    // Ensure gateway MAC is resolved
    if !arp::is_gateway_mac_resolved() {
        arp::send_arp_request(&arp::get_gateway_ip());
        for _ in 0..20000 {
            net::poll();
            if arp::is_gateway_mac_resolved() {
                break;
            }
        }
    }

    if !arp::is_gateway_mac_resolved() {
        println!("[AGENT] Cannot reach API - gateway MAC not resolved");
        return;
    }

    // Convert IP to string for HTTP client
    let ip_str = ip_to_str(&API_IP);

    // Connect and send request
    use crate::net::tcp;
    if !tcp::connect(&API_IP, API_PORT) {
        println!("[AGENT] Failed to connect to API");
        return;
    }

    // Build HTTP POST request manually
    let mut request = [0u8; BUFFER_SIZE];
    let mut pos = 0;

    pos += append_bytes(&mut request[pos..], b"POST ");
    pos += append_bytes(&mut request[pos..], API_PATH.as_bytes());
    pos += append_bytes(&mut request[pos..], b" HTTP/1.1\r\n");
    pos += append_bytes(&mut request[pos..], b"Host: ");
    pos += append_bytes(&mut request[pos..], &ip_str[..]);
    pos += append_bytes(&mut request[pos..], b"\r\n");
    pos += append_bytes(&mut request[pos..], b"Content-Type: application/json\r\n");
    pos += append_bytes(&mut request[pos..], b"Connection: close\r\n");
    pos += append_bytes(&mut request[pos..], b"Content-Length: ");
    pos += append_usize(&mut request[pos..], body.len());
    pos += append_bytes(&mut request[pos..], b"\r\n\r\n");
    pos += append_bytes(&mut request[pos..], body.as_bytes());

    if !tcp::send(&request[..pos]) {
        println!("[AGENT] Failed to send request");
        tcp::close();
        return;
    }

    // Receive response
    let mut response = [0u8; BUFFER_SIZE];
    let mut total = 0;
    let mut header_done = false;
    let mut header_end = 0;

    for _ in 0..200000 {
        let len = tcp::recv_bytes(&mut response[total..]);
        if len > 0 {
            total += len;

            if !header_done {
                // Find end of headers (\r\n\r\n)
                for i in 3..total {
                    if response[i-3] == b'\r' && response[i-2] == b'\n'
                        && response[i-1] == b'\r' && response[i] == b'\n' {
                        header_done = true;
                        header_end = i + 1;
                        break;
                    }
                }
            }
        }

        if !tcp::is_connected() && total > 0 {
            break;
        }

        for _ in 0..100 {
            net::poll();
        }
    }

    tcp::close();

    if total == 0 {
        println!("[AGENT] No response from API");
        return;
    }

    // Extract body
    let body_data = if header_done && header_end < total {
        &response[header_end..total]
    } else {
        &response[..total]
    };

    // Parse response - look for COMMAND: prefix
    let response_str = unsafe { core::str::from_utf8_unchecked(body_data) };
    println!("[AGENT] API response: {}", response_str);

    // Try to extract command from JSON "content" field first
    if let Some(content) = extract_json_field(response_str, "content") {
        execute_command_from_response(content);
    } else {
        // Fall back to plain text COMMAND: prefix
        execute_command_from_response(response_str);
    }
}

fn build_request_body() -> &'static str {
    // For MVP, use a simple hardcoded request
    // In production this would be dynamically built with conversation history
    r#"{"messages":[{"role":"system","content":"You are an agent inside Nymoris OS. Respond with COMMAND: <cmd> or none."},{"role":"user","content":"What should I do?"}]}"#
}

fn execute_command_from_response(response: &str) {
    // Look for "COMMAND: " prefix
    const PREFIX: &str = "COMMAND:";

    if let Some(idx) = response.find(PREFIX) {
        let after = &response[idx + PREFIX.len()..];
        // Trim whitespace
        let trimmed = trim_whitespace(after);

        if trimmed.is_empty() || trimmed == "none" {
            println!("[AGENT] No command to execute");
            return;
        }

        println!("[AGENT] Executing: {}", trimmed);
        commands::execute(trimmed);
    } else {
        println!("[AGENT] No COMMAND: prefix found in response");
    }
}

fn extract_json_field<'a>(json: &'a str, field: &str) -> Option<&'a str> {
    // Look for "field": "value" pattern
    let pattern = format_field_pattern(field);

    if let Some(idx) = json.find(&pattern) {
        let after = &json[idx + pattern.len()..];
        // Find closing quote
        if let Some(end) = find_unescaped_quote(after) {
            return Some(&after[..end]);
        }
    }

    None
}

fn format_field_pattern(field: &str) -> &'static str {
    static mut BUF: [u8; 64] = [0; 64];
    unsafe {
        let field_bytes = field.as_bytes();
        let mut pos = 0;
        BUF[pos] = b'"';
        pos += 1;
        for b in field_bytes {
            if pos < 60 {
                BUF[pos] = *b;
                pos += 1;
            }
        }
        BUF[pos] = b'"';
        pos += 1;
        BUF[pos] = b':';
        pos += 1;
        BUF[pos] = b' ';
        pos += 1;
        BUF[pos] = b'"';
        pos += 1;
        core::str::from_utf8_unchecked(&BUF[..pos])
    }
}

fn find_unescaped_quote(s: &str) -> Option<usize> {
    let bytes = s.as_bytes();
    for (i, &b) in bytes.iter().enumerate() {
        if b == b'"' {
            // Check if escaped
            if i > 0 && bytes[i - 1] == b'\\' {
                continue;
            }
            return Some(i);
        }
    }
    None
}

fn trim_whitespace(s: &str) -> &str {
    let bytes = s.as_bytes();
    let mut start = 0;
    let mut end = bytes.len();

    while start < end && (bytes[start] == b' ' || bytes[start] == b'\n' || bytes[start] == b'\r' || bytes[start] == b'\t') {
        start += 1;
    }

    while end > start && (bytes[end - 1] == b' ' || bytes[end - 1] == b'\n' || bytes[end - 1] == b'\r' || bytes[end - 1] == b'\t') {
        end -= 1;
    }

    unsafe { core::str::from_utf8_unchecked(&bytes[start..end]) }
}

fn ip_to_str(ip: &[u8; 4]) -> [u8; 16] {
    let mut buf = [0u8; 16];
    let mut pos = 0;

    for (i, octet) in ip.iter().enumerate() {
        if *octet >= 100 {
            buf[pos] = b'0' + (octet / 100);
            pos += 1;
            buf[pos] = b'0' + ((octet % 100) / 10);
            pos += 1;
            buf[pos] = b'0' + (octet % 10);
            pos += 1;
        } else if *octet >= 10 {
            buf[pos] = b'0' + (octet / 10);
            pos += 1;
            buf[pos] = b'0' + (octet % 10);
            pos += 1;
        } else {
            buf[pos] = b'0' + *octet;
            pos += 1;
        }

        if i < 3 {
            buf[pos] = b'.';
            pos += 1;
        }
    }

    buf
}

fn append_bytes(dst: &mut [u8], src: &[u8]) -> usize {
    let len = src.len().min(dst.len());
    dst[..len].copy_from_slice(&src[..len]);
    len
}

fn append_usize(dst: &mut [u8], mut val: usize) -> usize {
    if val == 0 {
        if !dst.is_empty() {
            dst[0] = b'0';
            return 1;
        }
        return 0;
    }

    let mut buf = [0u8; 20];
    let mut pos = 0;
    while val > 0 {
        buf[pos] = b'0' + (val % 10) as u8;
        pos += 1;
        val /= 10;
    }

    let mut written = 0;
    for i in (0..pos).rev() {
        if written < dst.len() {
            dst[written] = buf[i];
            written += 1;
        }
    }
    written
}
