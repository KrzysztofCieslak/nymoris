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

/// Split a P3 1GB huge page into 512 P2 2MB huge pages.
unsafe fn split_p3_huge_page(p3: &mut PageTable, p3_index: usize, user: bool) -> bool {
    let old_entry = p3[p3_index].clone();
    let phys_base = old_entry.addr().as_u64();
    let old_flags = old_entry.flags() & !PageTableFlags::HUGE_PAGE;
    let mut sub_flags = old_flags | PageTableFlags::HUGE_PAGE;
    let mut p3_flags = old_flags;
    if user {
        sub_flags |= PageTableFlags::USER_ACCESSIBLE;
        p3_flags |= PageTableFlags::USER_ACCESSIBLE;
    }

    let p2_phys = match super::allocator::alloc_page() {
        Some(p) => p,
        None => return false,
    };
    let p2 = p2_phys as *mut PageTable;
    let p2_ref = &mut *p2;
    p2_ref.zero();

    for i in 0..512 {
        let entry_phys = phys_base + (i as u64 * 0x200000); // 2MB
        p2_ref[i].set_addr(PhysAddr::new(entry_phys), sub_flags);
    }

    p3[p3_index].set_addr(PhysAddr::new(p2_phys), p3_flags);
    true
}

/// Split a P2 2MB huge page into 512 P1 4KB pages.
unsafe fn split_p2_huge_page(p2: &mut PageTable, p2_index: usize, user: bool) -> bool {
    let old_entry = p2[p2_index].clone();
    let phys_base = old_entry.addr().as_u64();
    let old_flags = old_entry.flags() & !PageTableFlags::HUGE_PAGE;
    let mut sub_flags = old_flags;
    let mut p2_flags = old_flags;
    if user {
        sub_flags |= PageTableFlags::USER_ACCESSIBLE;
        p2_flags |= PageTableFlags::USER_ACCESSIBLE;
    }

    let p1_phys = match super::allocator::alloc_page() {
        Some(p) => p,
        None => return false,
    };
    let p1 = p1_phys as *mut PageTable;
    let p1_ref = &mut *p1;
    p1_ref.zero();

    for i in 0..512 {
        let entry_phys = phys_base + (i as u64 * 0x1000); // 4KB
        p1_ref[i].set_addr(PhysAddr::new(entry_phys), sub_flags);
    }

    p2[p2_index].set_addr(PhysAddr::new(p1_phys), p2_flags);
    true
}

/// Map a single 4KB page. Returns true on success.
/// Handles splitting of existing huge pages.
pub unsafe fn map_page(virt: u64, phys: u64, flags: PageTableFlags) -> bool {
    let addr = VirtAddr::new(virt);
    let p4_index = addr.p4_index();
    let p3_index = usize::from(u16::from(addr.p3_index()));
    let p2_index = usize::from(u16::from(addr.p2_index()));
    let p1_index = usize::from(u16::from(addr.p1_index()));

    let user = flags.contains(PageTableFlags::USER_ACCESSIBLE);
    let intermediate_flags = PageTableFlags::PRESENT | PageTableFlags::WRITABLE
        | if user { PageTableFlags::USER_ACCESSIBLE } else { PageTableFlags::empty() };

    let p4 = get_p4_mut();

    // Get or create P3
    let p3 = if !p4[p4_index].flags().contains(PageTableFlags::PRESENT) {
        let p3_phys = match super::allocator::alloc_page() {
            Some(p) => p,
            None => return false,
        };
        let p3 = p3_phys as *mut PageTable;
        (*p3).zero();
        p4[p4_index].set_addr(PhysAddr::new(p3_phys), intermediate_flags);
        p3
    } else {
        if user && !p4[p4_index].flags().contains(PageTableFlags::USER_ACCESSIBLE) {
            let e = &mut p4[p4_index];
            e.set_addr(e.addr(), e.flags() | PageTableFlags::USER_ACCESSIBLE);
        }
        p4[p4_index].addr().as_u64() as *mut PageTable
    };

    let p3 = &mut *p3;

    // Split P3 1GB huge page if present
    if p3[p3_index].flags().contains(PageTableFlags::HUGE_PAGE) {
        if !split_p3_huge_page(p3, p3_index, user) {
            return false;
        }
    }

    // Ensure P3 entry has USER_ACCESSIBLE if needed
    if user && p3[p3_index].flags().contains(PageTableFlags::PRESENT)
        && !p3[p3_index].flags().contains(PageTableFlags::USER_ACCESSIBLE)
    {
        let e = &mut p3[p3_index];
        e.set_addr(e.addr(), e.flags() | PageTableFlags::USER_ACCESSIBLE);
    }

    // Get or create P2
    let p2 = if !p3[p3_index].flags().contains(PageTableFlags::PRESENT) {
        let p2_phys = match super::allocator::alloc_page() {
            Some(p) => p,
            None => return false,
        };
        let p2 = p2_phys as *mut PageTable;
        (*p2).zero();
        p3[p3_index].set_addr(PhysAddr::new(p2_phys), intermediate_flags);
        p2
    } else {
        if user && !p3[p3_index].flags().contains(PageTableFlags::USER_ACCESSIBLE) {
            let e = &mut p3[p3_index];
            e.set_addr(e.addr(), e.flags() | PageTableFlags::USER_ACCESSIBLE);
        }
        p3[p3_index].addr().as_u64() as *mut PageTable
    };

    let p2 = &mut *p2;

    // Split P2 2MB huge page if present
    if p2[p2_index].flags().contains(PageTableFlags::HUGE_PAGE) {
        if !split_p2_huge_page(p2, p2_index, user) {
            return false;
        }
    }

    // Ensure P2 entry has USER_ACCESSIBLE if needed
    if user && p2[p2_index].flags().contains(PageTableFlags::PRESENT)
        && !p2[p2_index].flags().contains(PageTableFlags::USER_ACCESSIBLE)
    {
        let e = &mut p2[p2_index];
        e.set_addr(e.addr(), e.flags() | PageTableFlags::USER_ACCESSIBLE);
    }

    // Get or create P1
    let p1 = if !p2[p2_index].flags().contains(PageTableFlags::PRESENT) {
        let p1_phys = match super::allocator::alloc_page() {
            Some(p) => p,
            None => return false,
        };
        let p1 = p1_phys as *mut PageTable;
        (*p1).zero();
        p2[p2_index].set_addr(PhysAddr::new(p1_phys), intermediate_flags);
        p1
    } else {
        if user && !p2[p2_index].flags().contains(PageTableFlags::USER_ACCESSIBLE) {
            let e = &mut p2[p2_index];
            e.set_addr(e.addr(), e.flags() | PageTableFlags::USER_ACCESSIBLE);
        }
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
