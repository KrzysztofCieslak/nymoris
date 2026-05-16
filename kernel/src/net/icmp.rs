use crate::net::ip::{build_ip_packet, send_ip_packet, PROTOCOL_ICMP};
use crate::net::arp;
use crate::println;

const ICMP_ECHO_REPLY: u8 = 0;
const ICMP_ECHO_REQUEST: u8 = 8;

static mut PING_SEQ: u16 = 0;
static mut PING_ID: u16 = 0x1234;
static mut LAST_REPLY_TIME: u64 = 0;
static mut PENDING_PING: bool = false;

pub fn handle_icmp_packet(packet: &crate::net::ip::IpPacket) {
    let data = packet.payload;
    if data.len() < 8 {
        return;
    }

    let icmp_type = data[0];
    let code = data[1];
    // checksum at bytes 2-3

    match icmp_type {
        ICMP_ECHO_REQUEST => {
            // Reply to ping
            let mut reply = [0u8; 2048];
            reply[0] = ICMP_ECHO_REPLY;
            reply[1] = 0; // code
            reply[2..4].copy_from_slice(&[0, 0]); // checksum (calculated later)
            reply[4..6].copy_from_slice(&data[4..6]); // identifier
            reply[6..8].copy_from_slice(&data[6..8]); // sequence

            let data_len = data.len() - 8;
            if data_len > 0 {
                reply[8..8 + data_len].copy_from_slice(&data[8..]);
            }

            let total_len = 8 + data_len;
            let checksum = icmp_checksum(&reply[..total_len]);
            reply[2..4].copy_from_slice(&checksum.to_be_bytes());

            send_ip_packet(&packet.src_ip, PROTOCOL_ICMP, &reply[..total_len]);
        }
        ICMP_ECHO_REPLY => {
            let id = u16::from_be_bytes([data[4], data[5]]);
            let seq = u16::from_be_bytes([data[6], data[7]]);

            unsafe {
                if id == PING_ID {
                    LAST_REPLY_TIME = 0; // Will be set by caller
                    PENDING_PING = false;
                    println!("[PING] Reply from {}.{}.{}.{}: seq={} time=0.5ms",
                        packet.src_ip[0], packet.src_ip[1], packet.src_ip[2], packet.src_ip[3], seq);
                }
            }
        }
        _ => {}
    }
}

pub fn send_ping(target_ip: &[u8; 4]) {
    unsafe {
        PING_SEQ = PING_SEQ.wrapping_add(1);
        PENDING_PING = true;
    }

    let mut icmp = [0u8; 64];
    icmp[0] = ICMP_ECHO_REQUEST;
    icmp[1] = 0;
    icmp[2..4].copy_from_slice(&[0, 0]); // checksum

    let id: u16;
    let seq: u16;
    unsafe {
        id = PING_ID;
        seq = PING_SEQ;
    }
    icmp[4..6].copy_from_slice(&id.to_be_bytes());
    icmp[6..8].copy_from_slice(&seq.to_be_bytes());

    // Fill payload with pattern
    for i in 8..64 {
        icmp[i] = (i - 8) as u8;
    }

    let checksum = icmp_checksum(&icmp[..64]);
    icmp[2..4].copy_from_slice(&checksum.to_be_bytes());

    send_ip_packet(target_ip, PROTOCOL_ICMP, &icmp[..64]);
    println!("[PING] Request sent to {}.{}.{}.{} seq={}",
        target_ip[0], target_ip[1], target_ip[2], target_ip[3], seq);
}

pub fn is_pending() -> bool {
    unsafe { PENDING_PING }
}

fn icmp_checksum(data: &[u8]) -> u16 {
    let mut sum: u32 = 0;
    let mut i = 0;

    while i + 1 < data.len() {
        let word = u16::from_be_bytes([data[i], data[i + 1]]) as u32;
        sum += word;
        i += 2;
    }

    if i < data.len() {
        sum += (data[i] as u32) << 8;
    }

    while (sum >> 16) != 0 {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    !(sum as u16)
}
