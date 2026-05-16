use crate::println;

pub const ETHERTYPE_ARP: u16 = 0x0806;
pub const ETHERTYPE_IP: u16 = 0x0800;

#[derive(Debug, Clone, Copy)]
pub struct EthernetFrame<'a> {
    pub dst_mac: [u8; 6],
    pub src_mac: [u8; 6],
    pub ethertype: u16,
    pub payload: &'a [u8],
}

pub fn parse_frame(data: &[u8]) -> Option<EthernetFrame> {
    if data.len() < 14 {
        return None;
    }

    let mut dst_mac = [0u8; 6];
    let mut src_mac = [0u8; 6];
    dst_mac.copy_from_slice(&data[0..6]);
    src_mac.copy_from_slice(&data[6..12]);
    let ethertype = u16::from_be_bytes([data[12], data[13]]);

    Some(EthernetFrame {
        dst_mac,
        src_mac,
        ethertype,
        payload: &data[14..],
    })
}

pub fn build_frame(buffer: &mut [u8], dst_mac: &[u8; 6], src_mac: &[u8; 6], ethertype: u16, payload: &[u8]) -> usize {
    let total_len = 14 + payload.len();
    if buffer.len() < total_len {
        panic!("Ethernet frame buffer too small");
    }

    buffer[0..6].copy_from_slice(dst_mac);
    buffer[6..12].copy_from_slice(src_mac);
    buffer[12..14].copy_from_slice(&ethertype.to_be_bytes());
    buffer[14..total_len].copy_from_slice(payload);

    total_len
}

pub fn is_broadcast(mac: &[u8; 6]) -> bool {
    mac == &[0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF]
}

pub fn handle_frame(frame: &EthernetFrame) {
    match frame.ethertype {
        ETHERTYPE_ARP => {
            crate::net::arp::handle_arp_packet(frame);
        }
        ETHERTYPE_IP => {
            crate::net::ip::handle_ip_packet(frame);
        }
        _ => {
            // Unknown ethertype, ignore
        }
    }
}
