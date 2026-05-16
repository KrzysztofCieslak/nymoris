use crate::net::ip::{build_ip_packet, PROTOCOL_TCP};
use crate::net::arp;
use crate::net::ethernet::{build_frame, ETHERTYPE_IP};
use crate::net::virtio;
use crate::println;

// TCP flags
const TCP_FLAG_FIN: u8 = 0x01;
const TCP_FLAG_SYN: u8 = 0x02;
const TCP_FLAG_RST: u8 = 0x04;
const TCP_FLAG_PSH: u8 = 0x08;
const TCP_FLAG_ACK: u8 = 0x10;

// TCP states
#[derive(Debug, Clone, Copy, PartialEq)]
enum TcpState {
    Closed,
    SynSent,
    Established,
    FinWait1,
    FinWait2,
    Closing,
    TimeWait,
}

#[derive(Debug, Clone, Copy)]
struct TcpConnection {
    state: TcpState,
    src_port: u16,
    dst_port: u16,
    dst_ip: [u8; 4],
    local_seq: u32,
    remote_seq: u32,
    remote_window: u16,
}

// Receive buffer for incoming data
const RX_BUFFER_SIZE: usize = 8192;
static mut RX_BUFFER: [u8; RX_BUFFER_SIZE] = [0; RX_BUFFER_SIZE];
static mut RX_READ_POS: usize = 0;
static mut RX_WRITE_POS: usize = 0;

static mut CONNECTION: TcpConnection = TcpConnection {
    state: TcpState::Closed,
    src_port: 0,
    dst_port: 0,
    dst_ip: [0; 4],
    local_seq: 0,
    remote_seq: 0,
    remote_window: 4096,
};

static mut NEXT_PORT: u16 = 40000;

#[derive(Debug, Clone, Copy)]
pub struct TcpHeader {
    pub src_port: u16,
    pub dst_port: u16,
    pub seq_num: u32,
    pub ack_num: u32,
    pub data_offset: u8,
    pub flags: u8,
    pub window: u16,
    pub checksum: u16,
    pub urgent: u16,
}

pub fn parse_tcp_header(data: &[u8]) -> Option<TcpHeader> {
    if data.len() < 20 {
        return None;
    }

    let src_port = u16::from_be_bytes([data[0], data[1]]);
    let dst_port = u16::from_be_bytes([data[2], data[3]]);
    let seq_num = u32::from_be_bytes([data[4], data[5], data[6], data[7]]);
    let ack_num = u32::from_be_bytes([data[8], data[9], data[10], data[11]]);
    let data_offset = (data[12] >> 4) * 4;
    let flags = data[13];
    let window = u16::from_be_bytes([data[14], data[15]]);
    let checksum = u16::from_be_bytes([data[16], data[17]]);
    let urgent = u16::from_be_bytes([data[18], data[19]]);

    Some(TcpHeader {
        src_port,
        dst_port,
        seq_num,
        ack_num,
        data_offset,
        flags,
        window,
        checksum,
        urgent,
    })
}

fn build_tcp_header(
    buffer: &mut [u8],
    src_port: u16,
    dst_port: u16,
    seq_num: u32,
    ack_num: u32,
    flags: u8,
    window: u16,
    payload_len: usize,
) -> usize {
    let header_len = 20;
    let total_len = header_len + payload_len;
    if buffer.len() < total_len {
        panic!("TCP buffer too small");
    }

    buffer[0..2].copy_from_slice(&src_port.to_be_bytes());
    buffer[2..4].copy_from_slice(&dst_port.to_be_bytes());
    buffer[4..8].copy_from_slice(&seq_num.to_be_bytes());
    buffer[8..12].copy_from_slice(&ack_num.to_be_bytes());
    // Data offset (5 * 4 = 20 bytes) + reserved (0) = 0x50
    buffer[12] = 0x50;
    buffer[13] = flags;
    buffer[14..16].copy_from_slice(&window.to_be_bytes());
    // Checksum placeholder
    buffer[16..18].copy_from_slice(&[0, 0]);
    buffer[18..20].copy_from_slice(&[0, 0]);

    header_len
}

fn tcp_checksum(src_ip: &[u8; 4], dst_ip: &[u8; 4], tcp_header: &[u8], payload: &[u8]) -> u16 {
    let tcp_len = tcp_header.len() + payload.len();

    // Pseudo header: src_ip (4) + dst_ip (4) + zero (1) + protocol (1) + tcp_len (2)
    let mut sum: u32 = 0;

    // Add pseudo header
    for i in (0..4).step_by(2) {
        sum += u16::from_be_bytes([src_ip[i], src_ip[i + 1]]) as u32;
    }
    for i in (0..4).step_by(2) {
        sum += u16::from_be_bytes([dst_ip[i], dst_ip[i + 1]]) as u32;
    }
    sum += 6 as u32; // protocol
    sum += (tcp_len as u16) as u32;

    // Add TCP header
    let mut i = 0;
    while i + 1 < tcp_header.len() {
        sum += u16::from_be_bytes([tcp_header[i], tcp_header[i + 1]]) as u32;
        i += 2;
    }
    if i < tcp_header.len() {
        sum += (tcp_header[i] as u32) << 8;
    }

    // Add payload
    i = 0;
    while i + 1 < payload.len() {
        sum += u16::from_be_bytes([payload[i], payload[i + 1]]) as u32;
        i += 2;
    }
    if i < payload.len() {
        sum += (payload[i] as u32) << 8;
    }

    while (sum >> 16) != 0 {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    !(sum as u16)
}

fn send_tcp_packet(flags: u8, payload: &[u8]) {
    unsafe {
        let conn = &CONNECTION;
        if conn.state == TcpState::Closed {
            return;
        }

        let mut tcp_buf = [0u8; 2048];
        let header_len = build_tcp_header(
            &mut tcp_buf,
            conn.src_port,
            conn.dst_port,
            conn.local_seq,
            conn.remote_seq,
            flags,
            4096, // window
            payload.len(),
        );

        if !payload.is_empty() {
            tcp_buf[header_len..header_len + payload.len()].copy_from_slice(payload);
        }

        let total_tcp_len = header_len + payload.len();
        let checksum = tcp_checksum(
            &arp::get_our_ip(),
            &conn.dst_ip,
            &tcp_buf[..header_len],
            payload,
        );
        tcp_buf[16..18].copy_from_slice(&checksum.to_be_bytes());

        let mut ip_buf = [0u8; 2048];
        let ip_len = build_ip_packet(&mut ip_buf, &arp::get_our_ip(), &conn.dst_ip, PROTOCOL_TCP, &tcp_buf[..total_tcp_len]);

        let dst_mac = arp::get_gateway_mac();
        if dst_mac == [0; 6] {
            println!("[TCP] Cannot send: gateway MAC unknown");
            return;
        }

        let our_mac = virtio::get_mac_address();
        let mut eth_buf = [0u8; 2048];
        let eth_len = build_frame(&mut eth_buf, &dst_mac, &our_mac, ETHERTYPE_IP, &ip_buf[..ip_len]);

        virtio::send_packet(&eth_buf[..eth_len]);

        // Update local sequence number
        if !payload.is_empty() {
            CONNECTION.local_seq += payload.len() as u32;
        }
        if flags & TCP_FLAG_SYN != 0 {
            CONNECTION.local_seq += 1;
        }
        if flags & TCP_FLAG_FIN != 0 {
            CONNECTION.local_seq += 1;
        }
    }
}

pub fn handle_tcp_packet(src_ip: &[u8; 4], dst_ip: &[u8; 4], payload: &[u8]) {
    let header = match parse_tcp_header(payload) {
        Some(h) => h,
        None => return,
    };

    unsafe {
        let conn = &mut CONNECTION;

        // Check if this packet belongs to our connection
        if conn.state == TcpState::Closed {
            return;
        }
        if conn.dst_ip != *src_ip || conn.dst_port != header.src_port {
            return;
        }

        let data_offset = header.data_offset as usize;
        let data = if payload.len() > data_offset {
            &payload[data_offset..]
        } else {
            &[]
        };

        match conn.state {
            TcpState::SynSent => {
                if header.flags & TCP_FLAG_SYN != 0 && header.flags & TCP_FLAG_ACK != 0 {
                    // Received SYN-ACK
                    conn.remote_seq = header.seq_num + 1;
                    conn.remote_window = header.window;
                    send_tcp_packet(TCP_FLAG_ACK, &[]);
                    conn.state = TcpState::Established;
                    println!("[TCP] Connection established to {}.{}.{}.{}:{}",
                        src_ip[0], src_ip[1], src_ip[2], src_ip[3], conn.dst_port);
                } else if header.flags & TCP_FLAG_RST != 0 {
                    conn.state = TcpState::Closed;
                    println!("[TCP] Connection reset by peer");
                }
            }
            TcpState::Established => {
                if header.flags & TCP_FLAG_RST != 0 {
                    conn.state = TcpState::Closed;
                    println!("[TCP] Connection reset by peer");
                    return;
                }

                if header.flags & TCP_FLAG_FIN != 0 {
                    conn.remote_seq = header.seq_num + 1;
                    send_tcp_packet(TCP_FLAG_ACK, &[]);
                    conn.state = TcpState::Closed;
                    println!("[TCP] Connection closed by peer");
                    return;
                }

                if !data.is_empty() {
                    // Store data in receive buffer
                    let write_pos = RX_WRITE_POS;
                    let available = RX_BUFFER_SIZE - write_pos;
                    let to_copy = data.len().min(available);
                    if to_copy > 0 {
                        RX_BUFFER[write_pos..write_pos + to_copy].copy_from_slice(&data[..to_copy]);
                        RX_WRITE_POS += to_copy;
                    }
                    conn.remote_seq = header.seq_num + data.len() as u32;
                    send_tcp_packet(TCP_FLAG_ACK, &[]);
                } else if header.flags & TCP_FLAG_ACK != 0 {
                    // Pure ACK, update remote window if needed
                    conn.remote_window = header.window;
                }
            }
            TcpState::FinWait1 => {
                if header.flags & TCP_FLAG_ACK != 0 {
                    if header.flags & TCP_FLAG_FIN != 0 {
                        conn.remote_seq = header.seq_num + 1;
                        send_tcp_packet(TCP_FLAG_ACK, &[]);
                        conn.state = TcpState::Closed;
                        println!("[TCP] Connection fully closed");
                    } else {
                        conn.state = TcpState::FinWait2;
                    }
                }
            }
            TcpState::FinWait2 => {
                if header.flags & TCP_FLAG_FIN != 0 {
                    conn.remote_seq = header.seq_num + 1;
                    send_tcp_packet(TCP_FLAG_ACK, &[]);
                    conn.state = TcpState::Closed;
                    println!("[TCP] Connection fully closed");
                }
            }
            _ => {}
        }
    }
}

pub fn connect(dst_ip: &[u8; 4], dst_port: u16) -> bool {
    unsafe {
        if CONNECTION.state != TcpState::Closed {
            println!("[TCP] Connection already in progress or established");
            return false;
        }

        // Clear receive buffer
        RX_READ_POS = 0;
        RX_WRITE_POS = 0;
        RX_BUFFER = [0; RX_BUFFER_SIZE];

        let src_port = NEXT_PORT;
        NEXT_PORT += 1;

        CONNECTION = TcpConnection {
            state: TcpState::SynSent,
            src_port,
            dst_port,
            dst_ip: *dst_ip,
            local_seq: 1000, // Simplified ISN
            remote_seq: 0,
            remote_window: 4096,
        };

        println!("[TCP] Connecting to {}.{}.{}.{}:{} from port {}",
            dst_ip[0], dst_ip[1], dst_ip[2], dst_ip[3], dst_port, src_port);

        send_tcp_packet(TCP_FLAG_SYN, &[]);

        // Poll for response
        for _ in 0..50000 {
            crate::net::poll();
            if CONNECTION.state == TcpState::Established {
                return true;
            }
        }

        println!("[TCP] Connection timeout");
        CONNECTION.state = TcpState::Closed;
        false
    }
}

pub fn send(data: &[u8]) -> bool {
    unsafe {
        if CONNECTION.state != TcpState::Established {
            println!("[TCP] Not connected");
            return false;
        }

        // Send in chunks if needed
        let chunk_size = 512;
        let mut offset = 0;
        while offset < data.len() {
            let end = (offset + chunk_size).min(data.len());
            send_tcp_packet(TCP_FLAG_PSH | TCP_FLAG_ACK, &data[offset..end]);
            offset = end;

            // Small delay between chunks
            for _ in 0..1000 {
                crate::net::poll();
            }
        }

        true
    }
}

pub fn recv_bytes(buffer: &mut [u8]) -> usize {
    unsafe {
        let available = RX_WRITE_POS - RX_READ_POS;
        let to_copy = available.min(buffer.len());
        if to_copy > 0 {
            buffer[..to_copy].copy_from_slice(&RX_BUFFER[RX_READ_POS..RX_READ_POS + to_copy]);
            RX_READ_POS += to_copy;

            // If buffer is fully consumed, reset pointers
            if RX_READ_POS == RX_WRITE_POS {
                RX_READ_POS = 0;
                RX_WRITE_POS = 0;
            }
        }
        to_copy
    }
}

pub fn recv_line(buffer: &mut [u8]) -> usize {
    unsafe {
        let available = RX_WRITE_POS - RX_READ_POS;
        let mut line_len = 0;
        for i in 0..available {
            let b = RX_BUFFER[RX_READ_POS + i];
            if line_len < buffer.len() {
                buffer[line_len] = b;
                line_len += 1;
            }
            if b == b'\n' {
                RX_READ_POS += i + 1;
                if RX_READ_POS == RX_WRITE_POS {
                    RX_READ_POS = 0;
                    RX_WRITE_POS = 0;
                }
                return line_len;
            }
        }
        0
    }
}

pub fn close() {
    unsafe {
        match CONNECTION.state {
            TcpState::Established => {
                send_tcp_packet(TCP_FLAG_FIN | TCP_FLAG_ACK, &[]);
                CONNECTION.state = TcpState::FinWait1;

                // Wait for close
                for _ in 0..20000 {
                    crate::net::poll();
                    if CONNECTION.state == TcpState::Closed {
                        break;
                    }
                }

                if CONNECTION.state != TcpState::Closed {
                    CONNECTION.state = TcpState::Closed;
                }
            }
            _ => {
                CONNECTION.state = TcpState::Closed;
            }
        }
    }
}

pub fn is_connected() -> bool {
    unsafe { CONNECTION.state == TcpState::Established }
}
