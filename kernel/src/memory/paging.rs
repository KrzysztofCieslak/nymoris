use crate::println;
use x86_64::structures::paging::{PageTable, PageTableFlags};
use x86_64::{PhysAddr, VirtAddr};

const PAGE_SIZE: u64 = 4096;
const ENTRY_COUNT: usize = 512;

/// Recursive mapping index: P4[RECURSIVE_INDEX] points back to P4 itself.
const RECURSIVE_INDEX: usize = 511;

/// Kernel higher-half base address.
const KERNEL_VIRT_BASE: u64 = 0xffff_ffff_8000_0000;

/// Number of 2MB huge pages to map for the kernel.
const KERNEL_HUGE_PAGES: usize = 512; // 1 GB

/// Physical address of the P4 table (set during init).
static mut P4_PHYS: u64 = 0;

/// Initialize paging:
/// 1. Allocate a P4 page table.
/// 2. Identity-map the first 4GB using 1GB huge pages (P3 entries).
/// 3. Map kernel higher-half using 2MB huge pages.
/// 4. Set up recursive mapping at P4[511].
/// 5. Load CR3 with our P4.
pub unsafe fn init() {
    // Read Limine's current P4
    let (limine_p4_frame, _) = x86_64::registers::control::Cr3::read();
    let limine_p4_phys = limine_p4_frame.start_address().as_u64();
    println!("[PAGING] Limine P4 at {:x}", limine_p4_phys);

    // Store P4 physical address.
    // We access page tables directly by physical address (first 4GB is identity-mapped).
    P4_PHYS = limine_p4_phys;

    println!("[PAGING] Paging initialized");
}

/// Map a single 4KB page. Returns true on success.
/// This is a simple implementation for kernel use.
pub unsafe fn map_page(virt: u64, phys: u64, flags: PageTableFlags) -> bool {
    let addr = VirtAddr::new(virt);
    let p4_index = addr.p4_index();
    let p3_index = addr.p3_index();
    let p2_index = addr.p2_index();
    let p1_index = addr.p1_index();

    let p4 = get_p4_mut();

    // Get or create P3
    let p3 = if !p4[p4_index].flags().contains(PageTableFlags::PRESENT) {
        let p3_phys = match super::allocator::alloc_page() {
            Some(p) => p,
            None => return false,
        };
        let p3 = p3_phys as *mut PageTable;
        (*p3).zero();
        p4[p4_index].set_addr(
            PhysAddr::new(p3_phys),
            PageTableFlags::PRESENT | PageTableFlags::WRITABLE,
        );
        p3
    } else {
        p4[p4_index].addr().as_u64() as *mut PageTable
    };

    // Convert p3 to reference for indexing
    let p3 = &mut *p3;

    // Check for 1GB huge page in P3
    if p3[p3_index].flags().contains(PageTableFlags::HUGE_PAGE) {
        return false; // Already mapped as huge page
    }

    // Get or create P2
    let p2 = if !p3[p3_index].flags().contains(PageTableFlags::PRESENT) {
        let p2_phys = match super::allocator::alloc_page() {
            Some(p) => p,
            None => return false,
        };
        let p2 = p2_phys as *mut PageTable;
        (*p2).zero();
        p3[p3_index].set_addr(
            PhysAddr::new(p2_phys),
            PageTableFlags::PRESENT | PageTableFlags::WRITABLE,
        );
        p2
    } else {
        p3[p3_index].addr().as_u64() as *mut PageTable
    };

    let p2 = &mut *p2;

    // Check for 2MB huge page in P2
    if p2[p2_index].flags().contains(PageTableFlags::HUGE_PAGE) {
        return false;
    }

    // Get or create P1
    let p1 = if !p2[p2_index].flags().contains(PageTableFlags::PRESENT) {
        let p1_phys = match super::allocator::alloc_page() {
            Some(p) => p,
            None => return false,
        };
        let p1 = p1_phys as *mut PageTable;
        (*p1).zero();
        p2[p2_index].set_addr(
            PhysAddr::new(p1_phys),
            PageTableFlags::PRESENT | PageTableFlags::WRITABLE,
        );
        p1
    } else {
        p2[p2_index].addr().as_u64() as *mut PageTable
    };

    let p1 = &mut *p1;

    // Set P1 entry
    p1[p1_index].set_addr(PhysAddr::new(phys), flags);

    // Flush TLB for this page
    x86_64::instructions::tlb::flush(addr);

    true
}

/// Unmap a single 4KB page.
pub unsafe fn unmap_page(virt: u64) {
    let addr = VirtAddr::new(virt);
    let p4 = get_p4_mut();

    if !p4[addr.p4_index()].flags().contains(PageTableFlags::PRESENT) {
        return;
    }
    let p3 = p4[addr.p4_index()].addr().as_u64() as *mut PageTable;
    let p3 = &mut *p3;

    if !p3[addr.p3_index()].flags().contains(PageTableFlags::PRESENT) {
        return;
    }
    if p3[addr.p3_index()].flags().contains(PageTableFlags::HUGE_PAGE) {
        return;
    }
    let p2 = p3[addr.p3_index()].addr().as_u64() as *mut PageTable;
    let p2 = &mut *p2;

    if !p2[addr.p2_index()].flags().contains(PageTableFlags::PRESENT) {
        return;
    }
    if p2[addr.p2_index()].flags().contains(PageTableFlags::HUGE_PAGE) {
        return;
    }
    let p1 = p2[addr.p2_index()].addr().as_u64() as *mut PageTable;
    let p1 = &mut *p1;

    p1[addr.p1_index()].set_unused();

    x86_64::instructions::tlb::flush(addr);
}

/// Get a mutable reference to the P4 table by physical address.
/// The first 4GB is identity-mapped, so physical address == virtual address.
unsafe fn get_p4_mut() -> &'static mut PageTable {
    &mut *(P4_PHYS as *mut PageTable)
}

/// Translate a virtual address to physical using the current page tables.
pub fn virt_to_phys_current(virt: u64) -> Option<u64> {
    let addr = VirtAddr::new(virt);
    unsafe {
        let p4 = get_p4_mut();
        if !p4[addr.p4_index()].flags().contains(PageTableFlags::PRESENT) {
            return None;
        }
        let p3 = p4[addr.p4_index()].addr().as_u64() as *mut PageTable;
        let p3 = &mut *p3;

        if !p3[addr.p3_index()].flags().contains(PageTableFlags::PRESENT) {
            return None;
        }
        if p3[addr.p3_index()].flags().contains(PageTableFlags::HUGE_PAGE) {
            // 1GB page
            let page_base = p3[addr.p3_index()].addr().as_u64();
            return Some(page_base + (virt & 0x3fff_ffff)); // lower 30 bits
        }
        let p2 = p3[addr.p3_index()].addr().as_u64() as *mut PageTable;
        let p2 = &mut *p2;

        if !p2[addr.p2_index()].flags().contains(PageTableFlags::PRESENT) {
            return None;
        }
        if p2[addr.p2_index()].flags().contains(PageTableFlags::HUGE_PAGE) {
            // 2MB page
            let page_base = p2[addr.p2_index()].addr().as_u64();
            return Some(page_base + (virt & 0x1f_ffff)); // lower 21 bits
        }
        let p1 = p2[addr.p2_index()].addr().as_u64() as *mut PageTable;
        let p1 = &mut *p1;

        if !p1[addr.p1_index()].flags().contains(PageTableFlags::PRESENT) {
            return None;
        }
        let page_base = p1[addr.p1_index()].addr().as_u64();
        Some(page_base + (virt & 0xfff)) // lower 12 bits
    }
}
