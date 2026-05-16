# KACOS Security Practices

KACOS is designed with security as a foundational principle. As an operating system intended to host autonomous AI agents, a minimal and auditable codebase is essential.

## Core Principles

### 1. No Heap Allocator in Kernel (Phase 1)

The kernel does not use the Rust `alloc` crate or any dynamic heap allocation during the MVP phase. All data structures are statically allocated or stack-reserved. This eliminates an entire class of vulnerabilities:

- Heap overflows corrupting adjacent metadata
- Use-after-free attacks
- Double-free corruption
- Heap spraying and feng shui techniques
- Allocation metadata tampering

**Enforcement:** Do not add `extern crate alloc` or any allocator dependency to kernel code without explicit Phase 2 planning.

### 2. Volatile Memory Access for Hardware

All hardware-mapped memory (PCI BARs, USB descriptors, VirtIO rings, framebuffer) is accessed exclusively through volatile operations. The compiler must not reorder, elide, or cache these accesses.

- Use `core::ptr::read_volatile` and `write_volatile`
- Never use direct struct field assignment for MMIO regions
- Mark descriptor structures with `#[repr(C)]` to prevent Rust from reordering fields

**Example:**
```rust
// Correct
unsafe { td_ptr.add(i).write_volatile(build_td(...)); }

// Incorrect — may be optimized away
unsafe { (*td_ptr.add(i)).link = 0; }
```

### 3. Assembly Interrupt Stubs

The kernel uses hand-written assembly stubs for interrupt handlers instead of `extern "x86-interrupt"`. The compiler-generated prologue for the x86-interrupt ABI uses `movaps`, which crashes when the stack is not 16-byte aligned at interrupt entry (the CPU pushes 24–32 bytes for same-ring interrupts, breaking alignment guarantees).

- Raw assembly saves all GPRs, calls Rust `extern "C"` handlers, restores, and `iretq`
- No reliance on compiler interrupt ABI correctness
- Predictable stack layout for security-critical paths

### 4. Minimal Attack Surface

Only the drivers and protocols strictly necessary for the agent MVP are included:

- VirtIO-net (PCI) — no other network drivers
- UHCI/EHCI USB — only HID keyboard, no mass storage or hub support
- ICMP echo — for basic connectivity testing
- No filesystem driver in Phase 1 — no disk attack surface

Each added driver expands the Trusted Computing Base (TCB). Evaluate every new component against the MVP requirement.

### 5. Static Memory Layout

All major buffers are statically allocated with fixed sizes:

- VirtIO RX/TX descriptor rings: 16 entries each
- USB transfer descriptors and queue heads: fixed pools
- Framebuffer back buffer: determined at boot from Limine response
- Stack: 128KB fixed at link time

No runtime resizing means no integer overflow in size calculations and no allocation failure paths to mishandle.

### 6. Input Validation and Bounds Checking

All external input is treated as untrusted and validated before use:

- **USB HID reports:** Keycodes are bounds-checked against the scancode table before lookup
- **Network packets:** All parsed lengths are checked against the buffer size before indexing
- **IP addresses:** Manual octet parsing with overflow checks (no `atoi`)
- **Shell commands:** Arguments are length-limited and checked for valid characters

**Pattern for packet parsing:**
```rust
if data.len() < MIN_HEADER_SIZE {
    return None; // Drop malformed packet
}
let len = u16::from_be_bytes([data[2], data[3]]) as usize;
if len > data.len() {
    return None; // Drop truncated packet
}
```

### 7. No Unsafe Abstractions Without Audit

`unsafe` blocks are used extensively for hardware access. Every `unsafe` block must be:

- As small as possible (ideally a single volatile read/write)
- Accompanied by a comment explaining the safety invariant
- Not wrapped in a "safe" function that hides unsafety without validation

**Bad:**
```rust
fn write_pci(addr: u32, val: u32) {
    unsafe { core::ptr::write_volatile(PCI_MMIO as *mut u32, val); }
}
```

**Better:**
```rust
/// # Safety
/// `addr` must be a valid PCI configuration space offset.
unsafe fn write_pci_config(addr: u32, val: u32) {
    core::ptr::write_volatile((PCI_MMIO + addr) as *mut u32, val);
}
```

### 8. Serial Output for Security Logging

COM1 serial output (0x3F8) is initialized early and used for:
- Kernel panic messages (unbuffered, always available)
- Security-relevant events (future: agent actions, network connections)

The serial path is simpler than framebuffer rendering and useful for post-mortem analysis.

### 9. No User-Controllable Code Execution (Phase 1)

In the MVP kernel-space agent, there is no userspace, no ELF loader, and no way for external input to execute arbitrary code. The agent loop can only invoke built-in shell commands with bounded arguments. This is a deliberate security boundary — the worst-case exploit is limited to the predefined command set.

### 10. Stack Protection

- Fixed 128KB stack at boot — no stack expansion
- No recursive functions in interrupt handlers or network packet processing
- Careful review of array sizes on stack to prevent stack overflow

## Future Security Work

### Phase 2: Userspace Isolation

When userspace is introduced:

- Separate kernel and user page tables
- User processes run at ring 3 with no I/O port access
- System call interface is the only gateway — audit all syscalls
- Stack canaries for kernel threads
- W^X (write-xor-execute) for user mappings

### Phase 3: Local LLM Sandboxing

When running local LLM inference:

- Model weights loaded into read-only user mappings
- Inference engine runs in isolated process
- Input tokenization is strictly bounded
- No JIT compilation in the model runtime

### Phase 4: Network Security

- TLS 1.3 for all remote API connections
- Certificate pinning for known AI API endpoints
- No plaintext HTTP except for local testing
- Firewall rules limiting outbound connections

## Security Checklist for New Code

Before committing any kernel change, verify:

- [ ] No heap allocations introduced (`alloc`, `Box`, `Vec`, `String`)
- [ ] All MMIO accesses use volatile read/write
- [ ] All input parsing has length and bounds checks
- [ ] `unsafe` blocks are minimal and documented
- [ ] No panics in interrupt handlers (use `unwrap` only in init code)
- [ ] Stack usage is bounded (no unbounded recursion, no large stack arrays)
- [ ] New dependencies are justified and minimal
- [ ] Security-sensitive code has a comment explaining the threat model
