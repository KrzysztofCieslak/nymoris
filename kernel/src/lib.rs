#![no_std]
#![feature(alloc_error_handler)]

extern crate alloc;

pub mod boot;
pub mod commands;
pub mod gdt;
pub mod idt;
pub mod interrupts;
pub mod keyboard;
pub mod memory;
pub mod net;
pub mod pci;
pub mod shell;
pub mod usb;
pub mod framebuffer;
pub mod agent;

#[alloc_error_handler]
fn alloc_error_handler(layout: alloc::alloc::Layout) -> ! {
    panic!("allocation error: {:?}", layout);
}
