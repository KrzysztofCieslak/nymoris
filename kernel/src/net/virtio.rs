use core::ptr::{read_volatile, write_volatile};
use crate::memory::virt_to_phys;
use crate::pci::PciDevice;

// VirtIO PCI constants
const VIRTIO_VENDOR_ID: u16 = 0x1AF4;
const VIRTIO_NET_DEVICE_ID_MIN: u16 = 0x1000;
const VIRTIO_NET_DEVICE_ID_MAX: u16 = 0x107F;

// VirtIO PCI capability types
const VIRTIO_PCI_CAP_COMMON_CFG: u8 = 1;
const VIRTIO_PCI_CAP_NOTIFY_CFG: u8 = 2;
const VIRTIO_PCI_CAP_ISR_CFG: u8 = 3;
const VIRTIO_PCI_CAP_DEVICE_CFG: u8 = 4;

// Device status bits
const VIRTIO_STATUS_ACKNOWLEDGE: u8 = 1;
const VIRTIO_STATUS_DRIVER: u8 = 2;
const VIRTIO_STATUS_FEATURES_OK: u8 = 8;
const VIRTIO_STATUS_DRIVER_OK: u8 = 4;
const VIRTIO_STATUS_FAILED: u8 = 128;

// Feature bits
const VIRTIO_F_VERSION_1: u64 = 1 << 32;
const VIRTIO_NET_F_MAC: u64 = 1 << 5;

// Queue constants
const QUEUE_SIZE: u16 = 16;
const RX_QUEUE_INDEX: u16 = 0;
const TX_QUEUE_INDEX: u16 = 1;

// Descriptor flags
const VRING_DESC_F_NEXT: u16 = 1;
const VRING_DESC_F_WRITE: u16 = 2;

#[repr(C)]
#[derive(Clone, Copy)]
struct VringDesc {
    addr: u64,
    len: u32,
    flags: u16,
    next: u16,
}

#[repr(C)]
struct VringAvail {
    flags: u16,
    idx: u16,
    ring: [u16; QUEUE_SIZE as usize],
    used_event: u16,
}

#[repr(C)]
#[derive(Clone, Copy)]
struct VringUsedElem {
    id: u32,
    len: u32,
}

#[repr(C)]
struct VringUsed {
    flags: u16,
    idx: u16,
    ring: [VringUsedElem; QUEUE_SIZE as usize],
    avail_event: u16,
}

#[repr(C, align(4096))]
struct Queue {
    desc: [VringDesc; QUEUE_SIZE as usize],
    avail: VringAvail,
    _padding1: [u8; 64],
    used: VringUsed,
    _padding2: [u8; 64],
}

// Statically allocated queues
static mut RX_QUEUE: Queue = Queue {
    desc: [VringDesc { addr: 0, len: 0, flags: 0, next: 0 }; QUEUE_SIZE as usize],
    avail: VringAvail { flags: 0, idx: 0, ring: [0; QUEUE_SIZE as usize], used_event: 0 },
    _padding1: [0; 64],
    used: VringUsed { flags: 0, idx: 0, ring: [VringUsedElem { id: 0, len: 0 }; QUEUE_SIZE as usize], avail_event: 0 },
    _padding2: [0; 64],
};

static mut TX_QUEUE: Queue = Queue {
    desc: [VringDesc { addr: 0, len: 0, flags: 0, next: 0 }; QUEUE_SIZE as usize],
    avail: VringAvail { flags: 0, idx: 0, ring: [0; QUEUE_SIZE as usize], used_event: 0 },
    _padding1: [0; 64],
    used: VringUsed { flags: 0, idx: 0, ring: [VringUsedElem { id: 0, len: 0 }; QUEUE_SIZE as usize], avail_event: 0 },
    _padding2: [0; 64],
};

// Packet buffers
const PACKET_SIZE: usize = 2048;
static mut RX_BUFFERS: [[u8; PACKET_SIZE]; QUEUE_SIZE as usize] = [[0; PACKET_SIZE]; QUEUE_SIZE as usize];
static mut TX_BUFFER: [u8; PACKET_SIZE] = [0; PACKET_SIZE];

// Driver state
static mut DEVICE: Option<VirtioNetDevice> = None;
static mut MAC_ADDR: [u8; 6] = [0; 6];

struct VirtioPciCap {
    cap_vndr: u8,
    cap_next: u8,
    cap_len: u8,
    cfg_type: u8,
    bar: u8,
    _padding: [u8; 3],
    offset: u32,
    length: u32,
}

struct VirtioNetDevice {
    pci: PciDevice,
    common_cfg_bar: u8,
    common_cfg_offset: u32,
    notify_cfg_bar: u8,
    notify_cfg_offset: u32,
    notify_off_multiplier: u32,
    isr_cfg_bar: u8,
    isr_cfg_offset: u32,
}

unsafe fn pci_read_config_word(dev: &PciDevice, offset: u8) -> u16 {
    let addr = 0x80000000u32
        | ((dev.bus as u32) << 16)
        | ((dev.slot as u32) << 11)
        | ((dev.func as u32) << 8)
        | ((offset as u32) & 0xFC);
    let mut port: x86_64::instructions::port::Port<u32> = x86_64::instructions::port::Port::new(0xCF8);
    port.write(addr);
    let mut data_port: x86_64::instructions::port::Port<u32> = x86_64::instructions::port::Port::new(0xCFC);
    let val = data_port.read();
    ((val >> ((offset as u32 & 2) * 8)) & 0xFFFF) as u16
}

unsafe fn pci_read_config_dword(dev: &PciDevice, offset: u8) -> u32 {
    let addr = 0x80000000u32
        | ((dev.bus as u32) << 16)
        | ((dev.slot as u32) << 11)
        | ((dev.func as u32) << 8)
        | ((offset as u32) & 0xFC);
    let mut port: x86_64::instructions::port::Port<u32> = x86_64::instructions::port::Port::new(0xCF8);
    port.write(addr);
    let mut data_port: x86_64::instructions::port::Port<u32> = x86_64::instructions::port::Port::new(0xCFC);
    data_port.read()
}

unsafe fn read_bar_u8(bar: u64, offset: u32) -> u8 {
    let ptr = (bar + offset as u64) as *const u8;
    read_volatile(ptr)
}

unsafe fn read_bar_u16(bar: u64, offset: u32) -> u16 {
    let ptr = (bar + offset as u64) as *const u16;
    read_volatile(ptr)
}

unsafe fn read_bar_u32(bar: u64, offset: u32) -> u32 {
    let ptr = (bar + offset as u64) as *const u32;
    read_volatile(ptr)
}

unsafe fn write_bar_u8(bar: u64, offset: u32, val: u8) {
    let ptr = (bar + offset as u64) as *mut u8;
    write_volatile(ptr, val);
}

unsafe fn write_bar_u16(bar: u64, offset: u32, val: u16) {
    let ptr = (bar + offset as u64) as *mut u16;
    write_volatile(ptr, val);
}

unsafe fn write_bar_u32(bar: u64, offset: u32, val: u32) {
    let ptr = (bar + offset as u64) as *mut u32;
    write_volatile(ptr, val);
}

unsafe fn write_bar_u64(bar: u64, offset: u32, val: u64) {
    let ptr = (bar + offset as u64) as *mut u64;
    write_volatile(ptr, val);
}

fn read_pci_cap(dev: &PciDevice, cap_offset: u8) -> VirtioPciCap {
    unsafe {
        let cap_vndr = pci_read_config_word(dev, cap_offset) as u8;
        let cap_next = (pci_read_config_word(dev, cap_offset) >> 8) as u8;
        let cap_len = pci_read_config_word(dev, cap_offset + 2) as u8;
        let cfg_type = (pci_read_config_word(dev, cap_offset + 2) >> 8) as u8;
        let bar = pci_read_config_word(dev, cap_offset + 4) as u8;
        let offset = pci_read_config_dword(dev, cap_offset + 8);
        let length = pci_read_config_dword(dev, cap_offset + 12);

        VirtioPciCap {
            cap_vndr,
            cap_next,
            cap_len,
            cfg_type,
            bar,
            _padding: [0; 3],
            offset,
            length,
        }
    }
}

fn get_bar_address(dev: &PciDevice, bar_index: u8) -> u64 {
    let bar_val = dev.read_bar(bar_index);
    if bar_val & 1 == 0 {
        // Memory-mapped BAR
        (bar_val & 0xFFFFFFF0) as u64
    } else {
        // I/O BAR - not expected for VirtIO modern devices
        (bar_val & 0xFFFFFFFC) as u64
    }
}

unsafe fn read_common_cfg_u8(dev: &VirtioNetDevice, offset: u32) -> u8 {
    let bar = get_bar_address(&dev.pci, dev.common_cfg_bar);
    read_bar_u8(bar, dev.common_cfg_offset + offset)
}

unsafe fn read_common_cfg_u16(dev: &VirtioNetDevice, offset: u32) -> u16 {
    let bar = get_bar_address(&dev.pci, dev.common_cfg_bar);
    read_bar_u16(bar, dev.common_cfg_offset + offset)
}

unsafe fn read_common_cfg_u32(dev: &VirtioNetDevice, offset: u32) -> u32 {
    let bar = get_bar_address(&dev.pci, dev.common_cfg_bar);
    read_bar_u32(bar, dev.common_cfg_offset + offset)
}

unsafe fn write_common_cfg_u8(dev: &VirtioNetDevice, offset: u32, val: u8) {
    let bar = get_bar_address(&dev.pci, dev.common_cfg_bar);
    write_bar_u8(bar, dev.common_cfg_offset + offset, val);
}

unsafe fn write_common_cfg_u16(dev: &VirtioNetDevice, offset: u32, val: u16) {
    let bar = get_bar_address(&dev.pci, dev.common_cfg_bar);
    write_bar_u16(bar, dev.common_cfg_offset + offset, val);
}

unsafe fn write_common_cfg_u32(dev: &VirtioNetDevice, offset: u32, val: u32) {
    let bar = get_bar_address(&dev.pci, dev.common_cfg_bar);
    write_bar_u32(bar, dev.common_cfg_offset + offset, val);
}

unsafe fn write_common_cfg_u64(dev: &VirtioNetDevice, offset: u32, val: u64) {
    let bar = get_bar_address(&dev.pci, dev.common_cfg_bar);
    write_bar_u64(bar, dev.common_cfg_offset + offset, val);
}

unsafe fn notify_queue(dev: &VirtioNetDevice, queue_index: u16) {
    let bar = get_bar_address(&dev.pci, dev.notify_cfg_bar);
    // Read queue_notify_off from common cfg for this queue
    write_common_cfg_u16(dev, 0x0E, queue_index); // queue_select
    let notify_off = read_common_cfg_u16(dev, 0x16); // queue_notify_off
    let notify_addr = bar + dev.notify_cfg_offset as u64 + (notify_off as u64 * dev.notify_off_multiplier as u64);
    write_bar_u16(notify_addr, 0, queue_index);
}

fn find_virtio_net_device() -> Option<PciDevice> {
    let mut devices = [PciDevice {
        bus: 0, slot: 0, func: 0,
        vendor_id: 0, device_id: 0,
        class: 0, subclass: 0, prog_if: 0,
    }; 32];
    let count = crate::pci::scan_all(&mut devices);

    for i in 0..count {
        let dev = &devices[i];
        if dev.vendor_id == VIRTIO_VENDOR_ID {
            if dev.device_id >= VIRTIO_NET_DEVICE_ID_MIN && dev.device_id <= VIRTIO_NET_DEVICE_ID_MAX {
                crate::println!("[NET] Found VirtIO device: {:04x}:{:04x} class={:02x}:{:02x}:{:02x}",
                    dev.vendor_id, dev.device_id, dev.class, dev.subclass, dev.prog_if);
                // Check subsystem ID to confirm it's a network device
                return Some(*dev);
            }
        }
    }
    None
}

unsafe fn parse_virtio_caps(dev: &PciDevice) -> Option<VirtioNetDevice> {
    // Read status register to find capability pointer
    let status = pci_read_config_word(dev, 0x06);
    if status & (1 << 4) == 0 {
        crate::println!("[NET] No PCI capabilities found");
        return None;
    }

    let cap_ptr = (pci_read_config_dword(dev, 0x34) & 0xFF) as u8;
    if cap_ptr == 0 {
        crate::println!("[NET] Capability pointer is 0");
        return None;
    }

    let mut net_dev = VirtioNetDevice {
        pci: *dev,
        common_cfg_bar: 0,
        common_cfg_offset: 0,
        notify_cfg_bar: 0,
        notify_cfg_offset: 0,
        notify_off_multiplier: 0,
        isr_cfg_bar: 0,
        isr_cfg_offset: 0,
    };

    let mut cap_offset = cap_ptr;
    let mut found_common = false;
    let mut found_notify = false;
    let mut found_isr = false;

    while cap_offset != 0 {
        let cap = read_pci_cap(dev, cap_offset);

        if cap.cap_vndr == 0x09 {
            match cap.cfg_type {
                VIRTIO_PCI_CAP_COMMON_CFG => {
                    net_dev.common_cfg_bar = cap.bar;
                    net_dev.common_cfg_offset = cap.offset;
                    found_common = true;
                    crate::println!("[NET] Common cfg: bar={}, offset={}", cap.bar, cap.offset);
                }
                VIRTIO_PCI_CAP_NOTIFY_CFG => {
                    net_dev.notify_cfg_bar = cap.bar;
                    net_dev.notify_cfg_offset = cap.offset;
                    // Read notify_off_multiplier from the capability data
                    let bar = get_bar_address(dev, cap.bar);
                    net_dev.notify_off_multiplier = read_bar_u32(bar, cap.offset + cap.length - 4);
                    found_notify = true;
                    crate::println!("[NET] Notify cfg: bar={}, offset={}, multiplier={}",
                        cap.bar, cap.offset, net_dev.notify_off_multiplier);
                }
                VIRTIO_PCI_CAP_ISR_CFG => {
                    net_dev.isr_cfg_bar = cap.bar;
                    net_dev.isr_cfg_offset = cap.offset;
                    found_isr = true;
                    crate::println!("[NET] ISR cfg: bar={}, offset={}", cap.bar, cap.offset);
                }
                VIRTIO_PCI_CAP_DEVICE_CFG => {
                    crate::println!("[NET] Device cfg: bar={}, offset={}", cap.bar, cap.offset);
                }
                _ => {}
            }
        }

        cap_offset = cap.cap_next;
    }

    if found_common && found_notify && found_isr {
        Some(net_dev)
    } else {
        crate::println!("[NET] Missing capabilities: common={}, notify={}, isr={}",
            found_common, found_notify, found_isr);
        None
    }
}

unsafe fn negotiate_features(dev: &VirtioNetDevice) -> bool {
    // Reset device
    write_common_cfg_u8(dev, 0x14, 0);
    while read_common_cfg_u8(dev, 0x14) != 0 {
        core::arch::asm!("pause", options(nomem, nostack));
    }

    // Set ACKNOWLEDGE
    write_common_cfg_u8(dev, 0x14, VIRTIO_STATUS_ACKNOWLEDGE);

    // Set DRIVER
    write_common_cfg_u8(dev, 0x14, VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER);

    // Read device features
    write_common_cfg_u32(dev, 0x00, 1); // device_feature_select = high 32 bits
    let device_features_hi = read_common_cfg_u32(dev, 0x04);
    write_common_cfg_u32(dev, 0x00, 0); // device_feature_select = low 32 bits
    let device_features_lo = read_common_cfg_u32(dev, 0x04);
    let device_features = ((device_features_hi as u64) << 32) | (device_features_lo as u64);
    crate::println!("[NET] Device features: {:016x}", device_features);

    // Set driver features: VERSION_1 + MAC
    let driver_features = VIRTIO_F_VERSION_1 | VIRTIO_NET_F_MAC;
    write_common_cfg_u32(dev, 0x08, 1); // driver_feature_select = high
    write_common_cfg_u32(dev, 0x0C, (driver_features >> 32) as u32);
    write_common_cfg_u32(dev, 0x08, 0); // driver_feature_select = low
    write_common_cfg_u32(dev, 0x0C, driver_features as u32);

    // Set FEATURES_OK
    write_common_cfg_u8(dev, 0x14,
        VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK);

    // Verify FEATURES_OK is still set
    let status = read_common_cfg_u8(dev, 0x14);
    if status & VIRTIO_STATUS_FEATURES_OK == 0 {
        crate::println!("[NET] Device rejected features");
        return false;
    }

    crate::println!("[NET] Features negotiated successfully");
    true
}

unsafe fn setup_queue(dev: &VirtioNetDevice, queue_index: u16, queue: &mut Queue) -> bool {
    // Select queue
    write_common_cfg_u16(dev, 0x0E, queue_index);

    // Read queue size
    let queue_size = read_common_cfg_u16(dev, 0x10);
    crate::println!("[NET] Queue {} size: {}", queue_index, queue_size);

    if queue_size < QUEUE_SIZE {
        crate::println!("[NET] Queue size {} too small, need {}", queue_size, QUEUE_SIZE);
        return false;
    }

    // Set queue size
    write_common_cfg_u16(dev, 0x10, QUEUE_SIZE);

    // Clear queue_enable
    write_common_cfg_u16(dev, 0x18, 0);

    // Set descriptor table physical address
    let desc_phys = virt_to_phys(queue.desc.as_ptr() as u64);
    write_common_cfg_u64(dev, 0x20, desc_phys);

    // Set available ring physical address
    let avail_phys = virt_to_phys(core::ptr::addr_of!(queue.avail) as u64);
    write_common_cfg_u64(dev, 0x28, avail_phys);

    // Set used ring physical address
    let used_phys = virt_to_phys(core::ptr::addr_of!(queue.used) as u64);
    write_common_cfg_u64(dev, 0x30, used_phys);

    // Enable queue
    write_common_cfg_u16(dev, 0x18, 1);

    crate::println!("[NET] Queue {} setup: desc={:x} avail={:x} used={:x}",
        queue_index, desc_phys, avail_phys, used_phys);

    true
}

unsafe fn read_mac_address(dev: &VirtioNetDevice) -> [u8; 6] {
    // Device-specific config follows common config in BAR
    // MAC is at offset 0 in device config
    let mut mac = [0u8; 6];

    // We need to find the device cfg capability to know the exact offset
    // For now, try reading from common_cfg_offset + common_cfg_length
    // Actually, let's search for device cfg capability again
    let status = pci_read_config_word(&dev.pci, 0x06);
    if status & (1 << 4) != 0 {
        let cap_ptr = (pci_read_config_dword(&dev.pci, 0x34) & 0xFF) as u8;
        let mut cap_offset = cap_ptr;
        while cap_offset != 0 {
            let cap = read_pci_cap(&dev.pci, cap_offset);
            if cap.cap_vndr == 0x09 && cap.cfg_type == VIRTIO_PCI_CAP_DEVICE_CFG {
                let bar = get_bar_address(&dev.pci, cap.bar);
                for i in 0..6 {
                    mac[i] = read_bar_u8(bar, cap.offset + i as u32);
                }
                break;
            }
            cap_offset = cap.cap_next;
        }
    }
    mac
}

unsafe fn add_rx_buffer(queue: &mut Queue, buf: &mut [u8; PACKET_SIZE], index: usize) {
    let phys = virt_to_phys(buf.as_mut_ptr() as u64);

    queue.desc[index] = VringDesc {
        addr: phys,
        len: PACKET_SIZE as u32,
        flags: VRING_DESC_F_WRITE,
        next: 0,
    };

    let avail_idx = queue.avail.idx % QUEUE_SIZE;
    queue.avail.ring[avail_idx as usize] = index as u16;
    queue.avail.idx = queue.avail.idx.wrapping_add(1);
}

pub unsafe fn init_controller() {
    crate::println!("[NET] Scanning for VirtIO-net device...");

    let pci_dev = match find_virtio_net_device() {
        Some(dev) => dev,
        None => {
            crate::println!("[NET] No VirtIO-net device found");
            return;
        }
    };

    crate::println!("[NET] Found at {:02x}:{:02x}.{:x}", pci_dev.bus, pci_dev.slot, pci_dev.func);

    // Enable bus mastering and memory space
    pci_dev.enable_bus_mastering();

    let net_dev = match parse_virtio_caps(&pci_dev) {
        Some(dev) => dev,
        None => {
            crate::println!("[NET] Failed to parse VirtIO capabilities");
            return;
        }
    };

    if !negotiate_features(&net_dev) {
        return;
    }

    // Read MAC address
    MAC_ADDR = read_mac_address(&net_dev);
    crate::println!("[NET] MAC address: {:02x}:{:02x}:{:02x}:{:02x}:{:02x}:{:02x}",
        MAC_ADDR[0], MAC_ADDR[1], MAC_ADDR[2],
        MAC_ADDR[3], MAC_ADDR[4], MAC_ADDR[5]);

    // Setup RX queue
    if !setup_queue(&net_dev, RX_QUEUE_INDEX, &mut RX_QUEUE) {
        return;
    }

    // Add buffers to RX queue
    for i in 0..QUEUE_SIZE {
        add_rx_buffer(&mut RX_QUEUE, &mut RX_BUFFERS[i as usize], i as usize);
    }

    // Notify device about RX buffers
    notify_queue(&net_dev, RX_QUEUE_INDEX);

    // Setup TX queue
    if !setup_queue(&net_dev, TX_QUEUE_INDEX, &mut TX_QUEUE) {
        return;
    }

    // Set DRIVER_OK
    write_common_cfg_u8(&net_dev, 0x14,
        VIRTIO_STATUS_ACKNOWLEDGE | VIRTIO_STATUS_DRIVER | VIRTIO_STATUS_FEATURES_OK | VIRTIO_STATUS_DRIVER_OK);

    crate::println!("[NET] VirtIO-net initialized successfully");

    DEVICE = Some(net_dev);
}

pub fn get_mac_address() -> [u8; 6] {
    unsafe { MAC_ADDR }
}

pub fn is_initialized() -> bool {
    unsafe { DEVICE.is_some() }
}

pub fn send_packet(data: &[u8]) -> bool {
    unsafe {
        let dev = match &DEVICE {
            Some(d) => d,
            None => return false,
        };

        if data.len() > PACKET_SIZE - 12 {
            crate::println!("[NET] Packet too large: {}", data.len());
            return false;
        }

        // VirtIO-net header: 12 bytes (flags + gso_type + hdr_len + gso_size + csum_start + csum_offset)
        // For simple transmit without offload, all zeros
        let header: [u8; 12] = [0; 12];

        // Copy header + data into TX buffer
        TX_BUFFER[..12].copy_from_slice(&header);
        TX_BUFFER[12..12 + data.len()].copy_from_slice(data);
        let total_len = 12 + data.len();

        // Get next descriptor index (simple: always use index 0 for MVP)
        let desc_idx = 0;
        let phys = virt_to_phys(TX_BUFFER.as_ptr() as u64);

        TX_QUEUE.desc[desc_idx] = VringDesc {
            addr: phys,
            len: total_len as u32,
            flags: 0,
            next: 0,
        };

        let avail_idx = TX_QUEUE.avail.idx % QUEUE_SIZE;
        TX_QUEUE.avail.ring[avail_idx as usize] = desc_idx as u16;
        TX_QUEUE.avail.idx = TX_QUEUE.avail.idx.wrapping_add(1);

        // Notify device
        notify_queue(dev, TX_QUEUE_INDEX);

        true
    }
}

pub fn receive_packet(buffer: &mut [u8]) -> Option<usize> {
    unsafe {
        let dev = match &DEVICE {
            Some(d) => d,
            None => return None,
        };

        // Check used ring
        let used_idx = RX_QUEUE.used.idx;
        let last_used = RX_QUEUE.avail.flags; // Hack: reuse flags as last_used tracker

        // Actually, let's track last_used separately
        static mut LAST_USED_IDX: u16 = 0;

        if LAST_USED_IDX == used_idx {
            return None;
        }

        let used_elem = &RX_QUEUE.used.ring[(LAST_USED_IDX % QUEUE_SIZE) as usize];
        let desc_idx = used_elem.id as usize;
        let len = used_elem.len as usize;

        // Skip virtio-net header (12 bytes)
        let data_len = if len > 12 { len - 12 } else { 0 };
        if data_len > 0 && data_len <= buffer.len() {
            buffer[..data_len].copy_from_slice(&RX_BUFFERS[desc_idx][12..12 + data_len]);
        }

        // Re-add buffer to RX queue
        add_rx_buffer(&mut RX_QUEUE, &mut RX_BUFFERS[desc_idx], desc_idx);
        notify_queue(dev, RX_QUEUE_INDEX);

        LAST_USED_IDX = LAST_USED_IDX.wrapping_add(1);

        if data_len > 0 {
            Some(data_len)
        } else {
            None
        }
    }
}

pub fn poll_packets() {
    unsafe {
        let mut buffer = [0u8; PACKET_SIZE];
        while let Some(len) = receive_packet(&mut buffer) {
            crate::net::handle_packet(&buffer[..len]);
        }
    }
}
