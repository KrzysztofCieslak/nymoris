pub mod arp;
pub mod ethernet;
pub mod icmp;
pub mod ip;
pub mod virtio;

use crate::println;

pub fn init() {
    println!("[NET] Initializing network stack...");

    unsafe {
        virtio::init_controller();
    }

    if virtio::is_initialized() {
        println!("[NET] Network device initialized");

        // Send ARP request for gateway
        arp::send_arp_request(&arp::get_gateway_ip());
    } else {
        println!("[NET] No network device available");
    }
}

pub fn handle_packet(data: &[u8]) {
    if let Some(frame) = ethernet::parse_frame(data) {
        ethernet::handle_frame(&frame);
    }
}

pub fn poll() {
    virtio::poll_packets();
}
