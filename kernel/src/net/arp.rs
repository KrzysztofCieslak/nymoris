use crate::net::ethernet::{build_frame, ETHERTYPE_ARP, is_broadcast};
use crate::net::virtio;
use crate::println;

const ARP_HW_TYPE_ETHERNET: u16 = 1;
const ARP_PROTOCOL_IP: u16 = 0x0800;
const ARP_OP_REQUEST: u16 = 1;
const ARP_OP_REPLY: u16 = 2;

static mut GATEWAY_IP: [u8; 4] = [10, 0, 2, 2];
static mut GATEWAY_MAC: [u8; 6] = [0x52, 0x55, 0x0a, 0x00, 0x02, 0x02]; // QEMU slirp default gateway MAC
static mut OUR_IP: [u8; 4] = [10, 0, 2, 15];

pub fn set_our_ip(ip: [u8; 4]) {
    unsafe { OUR_IP = ip; }
}

pub fn set_gateway_ip(ip: [u8; 4]) {
    unsafe { GATEWAY_IP = ip; }
}

pub fn get_our_ip() -> [u8; 4] {
    unsafe { OUR_IP }
}

pub fn get_gateway_ip() -> [u8; 4] {
    unsafe { GATEWAY_IP }
}

pub fn get_gateway_mac() -> [u8; 6] {
    unsafe { GATEWAY_MAC }
}

pub fn is_gateway_mac_resolved() -> bool {
    unsafe { GATEWAY_MAC != [0; 6] }
}

pub fn handle_arp_packet(frame: &crate::net::ethernet::EthernetFrame) {
    let data = frame.payload;
    if data.len() < 28 {
        return;
    }

    let hw_type = u16::from_be_bytes([data[0], data[1]]);
    let proto_type = u16::from_be_bytes([data[2], data[3]]);
    let hw_len = data[4];
    let proto_len = data[5];
    let op = u16::from_be_bytes([data[6], data[7]]);

    if hw_type != ARP_HW_TYPE_ETHERNET || proto_type != ARP_PROTOCOL_IP {
        return;
    }
    if hw_len != 6 || proto_len != 4 {
        return;
    }

    let sender_mac = [data[8], data[9], data[10], data[11], data[12], data[13]];
    let sender_ip = [data[14], data[15], data[16], data[17]];
    let target_ip = [data[24], data[25], data[26], data[27]];

    let our_ip = get_our_ip();

    match op {
        ARP_OP_REQUEST => {
            // Someone is asking for our MAC
            if target_ip == our_ip {
                send_arp_reply(&sender_mac, &sender_ip);
            }
        }
        ARP_OP_REPLY => {
            // Someone replied to our request
            if sender_ip == get_gateway_ip() {
                unsafe { GATEWAY_MAC = sender_mac; }
                println!("[ARP] Gateway MAC resolved: {:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
                    sender_mac[0], sender_mac[1], sender_mac[2],
                    sender_mac[3], sender_mac[4], sender_mac[5]);
            }
        }
        _ => {}
    }
}

pub fn send_arp_request(target_ip: &[u8; 4]) {
    let mut buffer = [0u8; 64]; // Min Ethernet frame is 60 bytes + padding room
    let our_mac = virtio::get_mac_address();

    // ARP packet
    buffer[14..16].copy_from_slice(&ARP_HW_TYPE_ETHERNET.to_be_bytes());
    buffer[16..18].copy_from_slice(&ARP_PROTOCOL_IP.to_be_bytes());
    buffer[18] = 6; // hw len
    buffer[19] = 4; // proto len
    buffer[20..22].copy_from_slice(&ARP_OP_REQUEST.to_be_bytes());
    buffer[22..28].copy_from_slice(&our_mac);
    buffer[28..32].copy_from_slice(&get_our_ip());
    buffer[32..38].copy_from_slice(&[0; 6]); // target MAC unknown
    buffer[38..42].copy_from_slice(target_ip);

    // Ethernet frame
    let broadcast = [0xFFu8; 6];
    let mut eth_buffer = [0u8; 64];
    let len = build_frame(&mut eth_buffer, &broadcast, &our_mac, ETHERTYPE_ARP, &buffer[14..42]);

    virtio::send_packet(&eth_buffer[..len]);
    println!("[ARP] Request sent for {}.{}.{}.{}", target_ip[0], target_ip[1], target_ip[2], target_ip[3]);
}

fn send_arp_reply(dst_mac: &[u8; 6], dst_ip: &[u8; 4]) {
    let mut buffer = [0u8; 64];
    let our_mac = virtio::get_mac_address();

    // ARP packet
    buffer[14..16].copy_from_slice(&ARP_HW_TYPE_ETHERNET.to_be_bytes());
    buffer[16..18].copy_from_slice(&ARP_PROTOCOL_IP.to_be_bytes());
    buffer[18] = 6;
    buffer[19] = 4;
    buffer[20..22].copy_from_slice(&ARP_OP_REPLY.to_be_bytes());
    buffer[22..28].copy_from_slice(&our_mac);
    buffer[28..32].copy_from_slice(&get_our_ip());
    buffer[32..38].copy_from_slice(dst_mac);
    buffer[38..42].copy_from_slice(dst_ip);

    // Ethernet frame
    let mut eth_buffer = [0u8; 64];
    let len = build_frame(&mut eth_buffer, dst_mac, &our_mac, ETHERTYPE_ARP, &buffer[14..42]);

    virtio::send_packet(&eth_buffer[..len]);
}
