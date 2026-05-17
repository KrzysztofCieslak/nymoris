use core::alloc::{GlobalAlloc, Layout};
use core::ptr::null_mut;
use spin::Mutex;

/// Free block header in the heap's free list.
struct FreeBlock {
    size: usize,
    next: *mut FreeBlock,
}

unsafe impl Send for FreeBlock {}

unsafe impl Send for HeapAllocator {}

unsafe impl Sync for LockedHeap {}

pub struct HeapAllocator {
    free_list: *mut FreeBlock,
    total_size: usize,
    used_size: usize,
}

impl HeapAllocator {
    pub const fn new() -> Self {
        HeapAllocator {
            free_list: null_mut(),
            total_size: 0,
            used_size: 0,
        }
    }

    /// Add a memory region to the heap. The region must be page-aligned.
    pub unsafe fn add_region(&mut self, start: usize, size: usize) {
        let block = start as *mut FreeBlock;
        (*block).size = size;
        (*block).next = self.free_list;
        self.free_list = block;
        self.total_size += size;
    }

    unsafe fn alloc_from_list(&mut self, size: usize, align: usize) -> *mut u8 {
        let mut current = &mut self.free_list;

        while !(*current).is_null() {
            let block_ptr = *current;
            let block_size = (*block_ptr).size;
            let block_next = (*block_ptr).next;
            let block_start = block_ptr as usize;
            let aligned_start = align_up(block_start + core::mem::size_of::<FreeBlock>(), align);
            let end = block_start + block_size;

            if end >= aligned_start + size {
                let remaining = end.saturating_sub(aligned_start + size);

                if remaining >= core::mem::size_of::<FreeBlock>() + 8 {
                    let new_block = (aligned_start + size) as *mut FreeBlock;
                    (*new_block).size = remaining;
                    (*new_block).next = block_next;
                    *current = new_block;
                } else {
                    *current = block_next;
                }

                self.used_size += aligned_start + size - block_start;
                return aligned_start as *mut u8;
            }

            current = &mut (*block_ptr).next;
        }

        null_mut()
    }

    pub unsafe fn alloc(&mut self, size: usize, align: usize) -> *mut u8 {
        self.alloc_from_list(size, align)
    }

    pub unsafe fn free(&mut self, ptr: *mut u8, size: usize) {
        if ptr.is_null() {
            return;
        }

        let block = ptr as *mut FreeBlock;
        (*block).size = size.max(core::mem::size_of::<FreeBlock>());
        (*block).next = self.free_list;
        self.free_list = block;
        self.used_size = self.used_size.saturating_sub(size);
    }

    pub fn stats(&self) -> (usize, usize) {
        (self.used_size, self.total_size)
    }
}

fn align_up(addr: usize, align: usize) -> usize {
    (addr + align - 1) & !(align - 1)
}

/// Wrapper that provides GlobalAlloc via a Mutex.
pub struct LockedHeap {
    inner: Mutex<HeapAllocator>,
}

impl LockedHeap {
    pub const fn new() -> Self {
        LockedHeap {
            inner: Mutex::new(HeapAllocator::new()),
        }
    }

    pub unsafe fn init(&self) {
        // Allocate an initial 64KB region (16 pages) for the heap
        const INITIAL_PAGES: usize = 16;
        let mut pages = [0u64; INITIAL_PAGES];
        let mut allocated = 0;

        for i in 0..INITIAL_PAGES {
            match super::allocator::alloc_page() {
                Some(p) => {
                    pages[i] = p;
                    allocated += 1;
                }
                None => break,
            }
        }

        if allocated == 0 {
            crate::println!("[HEAP] Failed to allocate any heap pages!");
            return;
        }

        // For MVP, use only the first page; free the rest back
        // In a real impl we'd use contiguous allocation
        for i in 1..allocated {
            super::allocator::free_page(pages[i]);
        }

        let start = pages[0] as usize;
        let size = 4096;

        self.inner.lock().add_region(start, size);

        crate::println!("[HEAP] Initialized at {:x}, size {} KB", start, size / 1024);
    }
}

unsafe impl GlobalAlloc for LockedHeap {
    unsafe fn alloc(&self, layout: Layout) -> *mut u8 {
        self.inner.lock().alloc(layout.size(), layout.align())
    }

    unsafe fn dealloc(&self, ptr: *mut u8, layout: Layout) {
        self.inner.lock().free(ptr, layout.size());
    }
}

/// Global heap allocator instance.
#[global_allocator]
pub static HEAP: LockedHeap = LockedHeap::new();
