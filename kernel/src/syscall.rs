use crate::{println, print};

/// Syscall numbers.
pub const SYS_EXIT: u64 = 0;
pub const SYS_WRITE: u64 = 1;
pub const SYS_YIELD: u64 = 2;

/// Called from the int 0x80 assembly stub.
/// Arguments are in registers per System V AMD64 ABI:
/// RAX = syscall number
/// RDI = arg1
/// RSI = arg2
/// RDX = arg3
#[no_mangle]
pub extern "C" fn syscall_handler(rax: u64, rdi: u64, rsi: u64, rdx: u64) -> u64 {
    match rax {
        SYS_EXIT => {
            println!("[SYSCALL] exit({})", rdi);
            unsafe {
                crate::scheduler::exit_current_task()
            }
        }
        SYS_WRITE => {
            // write(fd, buf, len)
            // fd 1 = stdout
            if rdi == 1 && rsi != 0 && rdx > 0 {
                unsafe {
                    let buf = core::slice::from_raw_parts(rsi as *const u8, rdx as usize);
                    // Print as UTF-8 lossy
                    for b in buf {
                        print!("{}", *b as char);
                    }
                }
            }
            rdx // return number of bytes written
        }
        SYS_YIELD => {
            unsafe {
                crate::scheduler::yield_cpu();
            }
            0
        }
        _ => {
            println!("[SYSCALL] Unknown syscall: {}", rax);
            0
        }
    }
}
