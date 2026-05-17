use crate::{memory, println};
use x86_64::structures::paging::PageTableFlags;
use x86_64::{PhysAddr, VirtAddr};

const ELFMAG: [u8; 4] = [0x7f, b'E', b'L', b'F'];
const ELFCLASS64: u8 = 2;
const EM_X86_64: u16 = 62;
const PT_LOAD: u32 = 1;

#[repr(C)]
struct Elf64Ehdr {
    e_ident: [u8; 16],
    e_type: u16,
    e_machine: u16,
    e_version: u32,
    e_entry: u64,
    e_phoff: u64,
    e_shoff: u64,
    e_flags: u32,
    e_ehsize: u16,
    e_phentsize: u16,
    e_phnum: u16,
    e_shentsize: u16,
    e_shnum: u16,
    e_shstrndx: u16,
}

#[repr(C)]
struct Elf64Phdr {
    p_type: u32,
    p_flags: u32,
    p_offset: u64,
    p_vaddr: u64,
    p_paddr: u64,
    p_filesz: u64,
    p_memsz: u64,
    p_align: u64,
}

/// Load an ELF64 executable into memory.
/// Maps PT_LOAD segments at their specified virtual addresses with user-accessible flags.
/// Returns the entry point virtual address on success.
pub unsafe fn load_elf(data: &[u8]) -> Option<u64> {
    if data.len() < core::mem::size_of::<Elf64Ehdr>() {
        println!("[ELF] File too small for header");
        return None;
    }

    let ehdr = &*(data.as_ptr() as *const Elf64Ehdr);

    if ehdr.e_ident[0..4] != ELFMAG {
        println!("[ELF] Bad magic");
        return None;
    }
    if ehdr.e_ident[4] != ELFCLASS64 {
        println!("[ELF] Not 64-bit");
        return None;
    }
    if ehdr.e_machine != EM_X86_64 {
        println!("[ELF] Not x86_64");
        return None;
    }

    println!(
        "[ELF] Loading ELF: entry={:x}, phoff={:x}, phnum={}",
        ehdr.e_entry, ehdr.e_phoff, ehdr.e_phnum
    );

    for i in 0..ehdr.e_phnum {
        let phoff = ehdr.e_phoff as usize + i as usize * core::mem::size_of::<Elf64Phdr>();
        if phoff + core::mem::size_of::<Elf64Phdr>() > data.len() {
            println!("[ELF] Program header {} out of bounds", i);
            return None;
        }
        let phdr = &*(data.as_ptr().add(phoff) as *const Elf64Phdr);

        if phdr.p_type != PT_LOAD {
            continue;
        }

        println!(
            "[ELF] Segment {}: vaddr={:x}, filesz={}, memsz={}, flags={:x}",
            i, phdr.p_vaddr, phdr.p_filesz, phdr.p_memsz, phdr.p_flags
        );

        let offset = phdr.p_offset as usize;
        let vaddr = phdr.p_vaddr;
        let filesz = phdr.p_filesz as usize;
        let memsz = phdr.p_memsz as usize;

        // Allocate and map pages for this segment.
        let page_size = 4096usize;
        let start_page = (vaddr / page_size as u64) as usize;
        let end_page = ((vaddr + memsz as u64 + page_size as u64 - 1) / page_size as u64) as usize;

        for p in start_page..end_page {
            let page_vaddr = p as u64 * page_size as u64;
            let page_phys = memory::allocator::alloc_page()?;

            // Always map as writable during loading so we can copy data and zero BSS.
            // In a proper OS we'd remove WRITABLE for read-only segments afterward.
            let mut flags = PageTableFlags::PRESENT | PageTableFlags::USER_ACCESSIBLE | PageTableFlags::WRITABLE;
            if phdr.p_flags & 2 == 0 {
                // Not writable in final permissions (TODO: fix after loading)
            }

            if !memory::paging::map_page(page_vaddr, page_phys, flags) {
                println!("[ELF] Failed to map page at {:x}", page_vaddr);
                return None;
            }
        }

        // Copy file data into the mapped region.
        let dst = vaddr as *mut u8;
        if filesz > 0 {
            if offset + filesz > data.len() {
                println!("[ELF] Segment data out of bounds");
                return None;
            }
            core::ptr::copy_nonoverlapping(data.as_ptr().add(offset), dst, filesz);
        }

        // Zero BSS (memsz > filesz).
        if memsz > filesz {
            core::ptr::write_bytes(dst.add(filesz), 0, memsz - filesz);
        }
    }

    println!("[ELF] Entry point: {:x}", ehdr.e_entry);
    Some(ehdr.e_entry)
}
