use crate::println;

const PAGE_SIZE: usize = 4096;

#[repr(C, align(4096))]
struct Page {
    next: *mut Page,
}

static mut FREE_LIST: *mut Page = core::ptr::null_mut();
static mut TOTAL_PAGES: usize = 0;
static mut USED_PAGES: usize = 0;

fn align_up(addr: u64, align: u64) -> u64 {
    (addr + align - 1) & !(align - 1)
}

pub unsafe fn init() {
    let memmap_response = match crate::boot::MEMMAP_REQUEST.response() {
        Some(resp) => resp,
        None => {
            println!("[ALLOC] No memory map available!");
            return;
        }
    };

    for entry in memmap_response.entries() {
        if entry.type_ == limine::memmap::MEMMAP_USABLE {
            let start = align_up(entry.base, PAGE_SIZE as u64);
            let end = (entry.base + entry.length) & !(PAGE_SIZE as u64 - 1);

            if start >= end {
                continue;
            }

            for page in (start..end).step_by(PAGE_SIZE) {
                free_page(page);
                TOTAL_PAGES += 1;
            }
        }
    }

    println!("[ALLOC] Page frame allocator initialized: {} pages ({} MB) free",
        TOTAL_PAGES, (TOTAL_PAGES * PAGE_SIZE) / (1024 * 1024));
}

/// Allocate a single physical page. Returns physical address or None.
pub fn alloc_page() -> Option<u64> {
    unsafe {
        if FREE_LIST.is_null() {
            return None;
        }

        let page = FREE_LIST;
        FREE_LIST = (*page).next;
        USED_PAGES += 1;

        // Zero the page before returning
        core::ptr::write_bytes(page as *mut u8, 0, PAGE_SIZE);

        Some(page as u64)
    }
}

/// Free a physical page back to the allocator.
pub fn free_page(page: u64) {
    unsafe {
        let ptr = page as *mut Page;

        // Sanity check: page must be page-aligned
        if page as usize & (PAGE_SIZE - 1) != 0 {
            println!("[ALLOC] WARNING: freeing unaligned page {:x}", page);
            return;
        }

        (*ptr).next = FREE_LIST;
        FREE_LIST = ptr;
        USED_PAGES -= 1;
    }
}

/// Allocate `count` contiguous physical pages.
pub fn alloc_pages(count: usize) -> Option<u64> {
    // For contiguous allocation, we'd need a more sophisticated allocator.
    // For now, just fail if count > 1 since free-list doesn't guarantee contiguity.
    if count == 1 {
        alloc_page()
    } else {
        None
    }
}

pub fn get_stats() -> (usize, usize) {
    unsafe { (USED_PAGES, TOTAL_PAGES) }
}
