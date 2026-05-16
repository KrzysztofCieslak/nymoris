# Security Skill

This skill governs all security practices when writing or modifying Nymoris kernel code.

## Before Writing Any Code

1. Read SECURITY.md in the project root
2. Identify the threat model for the component being modified

## Mandatory Security Checks for Every Change

### No Heap Allocations in Kernel (Phase 1)
- NEVER introduce `extern crate alloc`, `Box`, `Vec`, `String`, or any allocator-dependent type in kernel code
- Use fixed-size arrays, static buffers, or stack allocations only
- If dynamic sizing is absolutely needed, use a pre-allocated static pool with a free bitmap

### Volatile Memory Access
- ALL hardware-mapped memory (PCI BARs, USB descriptors, VirtIO rings, framebuffer) MUST use `core::ptr::read_volatile` / `write_volatile`
- NEVER use direct struct field assignment for MMIO regions
- All descriptor structures MUST be `#[repr(C)]`

### Input Validation
- ALL external input is untrusted: USB HID reports, network packets, serial input, shell arguments
- Every parsed length MUST be checked against buffer size before indexing
- Return `None` or drop malformed packets — do not attempt to "fix" them
- IP address parsing must manually validate octet ranges (0-255) and format

### Unsafe Blocks
- Keep `unsafe` blocks as small as possible (single volatile operation ideally)
- Every `unsafe` block MUST have a comment explaining the safety invariant
- Do NOT hide `unsafe` inside "safe" functions without input validation

### Bounds and Stack Safety
- No recursive functions in interrupt handlers or packet processing paths
- Stack arrays must have fixed, reasonable sizes (under 1KB preferred)
- Check array indices against fixed bounds before access

### Panic Safety
- No `unwrap()`, `expect()`, or indexing (`arr[i]`) in interrupt handlers or network poll paths
- Use `if let`, `match`, or explicit bounds checks
- Panics in interrupt handlers deadlock the system

## Security Review Checklist (apply before finishing)

- [ ] No heap allocations (`alloc`, `Box`, `Vec`, `String`) introduced
- [ ] All MMIO uses volatile read/write
- [ ] All input parsing has length and bounds checks
- [ ] `unsafe` blocks are minimal and documented
- [ ] No panics in interrupt handlers or network paths
- [ ] Stack usage is bounded (no recursion, no large stack arrays)
- [ ] New dependencies are minimal and justified
- [ ] Security-sensitive code has threat model comment
