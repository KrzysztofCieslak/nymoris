use crate::net::ethernet::{build_frame, ETHERTYPE_IP};
use crate::net::arp;
use crate::net::virtio;

pub const PROTOCOL_ICMP: u8 = 1;
pub const PROTOCOL_TCP: u8 = 6;
pub const PROTOCOL_UDP: u8 = 17;

#[derive(Debug, Clone, Copy)]
pub struct IpPacket<'a> {
    pub src_ip: [u8; 4],
    pub dst_ip: [u8; 4],
    pub protocol: u8,
    pub ttl: u8,
    pub payload: &'a [u8],
}

pub fn parse_ip_packet(data: &[u8]) -> Option<IpPacket> {
    if data.len() < 20 {
        return None;
    }

    let version_ihl = data[0];
    let version = version_ihl >> 4;
    let ihl = (version_ihl & 0x0F) as usize * 4;

    if version != 4 {
        return None;
    }
    if data.len() < ihl {
        return None;
    }

    let total_len = u16::from_be_bytes([data[2], data[3]]) as usize;
    if data.len() < total_len {
        return None;
    }

    let ttl = data[8];
    let protocol = data[9];
    let mut src_ip = [0u8; 4];
    let mut dst_ip = [0u8; 4];
    src_ip.copy_from_slice(&data[12..16]);
    dst_ip.copy_from_slice(&data[16..20]);

    Some(IpPacket {
        src_ip,
        dst_ip,
        protocol,
        ttl,
        payload: &data[ihl..total_len],
    })
}

pub fn build_ip_packet(
    buffer: &mut [u8],
    src_ip: &[u8; 4],
    dst_ip: &[u8; 4],
    protocol: u8,
    payload: &[u8],
) -> usize {
    let total_len = 20 + payload.len();
    if buffer.len() < total_len {
        panic!("IP packet buffer too small");
    }

    // Version (4) + IHL (5) = 0x45
    buffer[0] = 0x45;
    // DSCP + ECN = 0
    buffer[1] = 0;
    // Total length
    buffer[2..4].copy_from_slice(&(total_len as u16).to_be_bytes());
    // Identification
    buffer[4..6].copy_from_slice(&[0, 0]);
    // Flags + Fragment offset = no fragmentation
    buffer[6..8].copy_from_slice(&[0x40, 0x00]);
    // TTL
    buffer[8] = 64;
    // Protocol
    buffer[9] = protocol;
    // Header checksum (initially 0, calculated later)
    buffer[10..12].copy_from_slice(&[0, 0]);
    // Source IP
    buffer[12..16].copy_from_slice(src_ip);
    // Destination IP
    buffer[16..20].copy_from_slice(dst_ip);
    // Payload
    buffer[20..total_len].copy_from_slice(payload);

    // Calculate checksum
    let checksum = ip_checksum(&buffer[0..20]);
    buffer[10..12].copy_from_slice(&checksum.to_be_bytes());

    total_len
}

fn ip_checksum(data: &[u8]) -> u16 {
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

pub fn handle_ip_packet(frame: &crate::net::ethernet::EthernetFrame) {
    let packet = match parse_ip_packet(frame.payload) {
        Some(p) => p,
        None => return,
    };

    let our_ip = arp::get_our_ip();

    // Check if packet is for us
    if packet.dst_ip != our_ip && packet.dst_ip != [255, 255, 255, 255] {
        return;
    }

    match packet.protocol {
        PROTOCOL_ICMP => {
            crate::net::icmp::handle_icmp_packet(&packet);
        }
        PROTOCOL_TCP => {
            // TCP not yet implemented
        }
        PROTOCOL_UDP => {
            // UDP not yet implemented
        }
        _ => {}
    }
}

pub fn send_ip_packet(dst_ip: &[u8; 4], protocol: u8, payload: &[u8]) {
    let mut ip_buffer = [0u8; 2048];
    let ip_len = build_ip_packet(&mut ip_buffer, &arp::get_our_ip(), dst_ip, protocol, payload);

    // Determine destination MAC
    let dst_mac = if dst_ip == &arp::get_gateway_ip() || !is_local_network(dst_ip) {
        arp::get_gateway_mac()
    } else {
        // For local network, we'd need ARP resolution
        // For MVP, assume gateway
        arp::get_gateway_mac()
    };

    if dst_mac == [0; 6] {
        crate::println!("[IP] Cannot send: destination MAC unknown");
        return;
    }

    let our_mac = virtio::get_mac_address();
    let mut eth_buffer = [0u8; 2048];
    let eth_len = build_frame(&mut eth_buffer, &dst_mac, &our_mac, ETHERTYPE_IP, &ip_buffer[..ip_len]);

    virtio::send_packet(&eth_buffer[..eth_len]);
}

fn is_local_network(ip: &[u8; 4]) -> bool {
    // Simple check: if first 3 octets match our IP's first 3 octets
    let our_ip = arp::get_our_ip();
    ip[0] == our_ip[0] && ip[1] == our_ip[1] && ip[2] == our_ip[2]
}
