pub mod allocator;

use crate::println;

/// Convert a kernel virtual address to a physical address using Limine's
/// ExecutableAddressResponse. The kernel is loaded at a higher-half virtual
/// address but the physical load address is chosen by Limine at boot time.
pub fn virt_to_phys(virt: u64) -> u64 {
    let exec_addr = crate::boot::EXEC_ADDRESS_REQUEST.response();
    match exec_addr {
        Some(resp) => {
            let vbase = resp.virtual_base;
            let pbase = resp.physical_base;
            virt - vbase + pbase
        }
        None => {
            // Fallback to legacy assumption
            virt - 0xffffffff80000000 + 0x100000
        }
    }
}

/// Convert a physical address to a kernel virtual address (higher-half mapping).
pub fn phys_to_virt(phys: u64) -> u64 {
    let exec_addr = crate::boot::EXEC_ADDRESS_REQUEST.response();
    match exec_addr {
        Some(resp) => {
            let vbase = resp.virtual_base;
            let pbase = resp.physical_base;
            phys - pbase + vbase
        }
        None => {
            // Fallback to legacy assumption
            phys - 0x100000 + 0xffffffff80000000
        }
    }
}

pub fn print_memmap() {
    println!("Memory map information:");

    let memmap_response = {
        let ptr = crate::boot::MEMMAP_REQUEST.response();
        match ptr {
            Some(resp) => resp,
            None => {
                println!("  No memory map available from bootloader.");
                return;
            }
        }
    };

    let entries = memmap_response.entries();
    println!("  Found {} memory regions:", entries.len());
    println!("  {:<20} {:<16} {}", "Base", "Length", "Type");

    for entry in entries {
        let type_str = match entry.type_ {
            limine::memmap::MEMMAP_USABLE => "Usable",
            limine::memmap::MEMMAP_RESERVED => "Reserved",
            limine::memmap::MEMMAP_ACPI_RECLAIMABLE => "ACPI Reclaimable",
            limine::memmap::MEMMAP_ACPI_NVS => "ACPI NVS",
            limine::memmap::MEMMAP_BAD_MEMORY => "Bad Memory",
            limine::memmap::MEMMAP_BOOTLOADER_RECLAIMABLE => "Bootloader Reclaimable",
            limine::memmap::MEMMAP_EXECUTABLE_AND_MODULES => "Kernel/Modules",
            limine::memmap::MEMMAP_FRAMEBUFFER => "Framebuffer",
            limine::memmap::MEMMAP_MAPPED_RESERVED => "Mapped Reserved",
            _ => "Unknown",
        };

        println!(
            "  0x{:016X} 0x{:014X} {}",
            entry.base,
            entry.length,
            type_str
        );
    }
}
