# KACOS — Agentic AI Operating System

KACOS is a minimal x86_64 operating system written in Rust, designed from the ground up to host and run autonomous AI agents. It boots on bare metal via the Limine bootloader and runs on both QEMU and real hardware (MacBook Pro 2010 and similar x86_64 machines).

## Vision

KACOS is not just another hobby OS. It is being built as a dedicated runtime for agentic AI — software that perceives, decides, and acts on behalf of users. The system provides:

- Direct hardware control for deterministic agent execution
- A minimal, auditable attack surface
- Network connectivity for remote AI APIs
- Local LLM inference capabilities
- A foundation for running agent frameworks like OpenClaw and Hermes

## Current Status

KACOS is in active development. Current capabilities:

- [x] Boots via Limine bootloader on x86_64
- [x] Framebuffer text output with 8x16 bitmap font
- [x] Interrupt handling (timer, keyboard) via raw assembly stubs
- [x] PS/2 and USB HID keyboard input
- [x] Interactive shell with built-in commands
- [x] PCI bus scanning
- [x] VirtIO-net driver with network stack (ARP, IP, ICMP)
- [x] `ping` command
- [ ] TCP/HTTP stack
- [ ] Agent runtime
- [ ] Userspace / ELF loader
- [ ] Local LLM inference

## Quick Start

### Prerequisites

- `rustup` with nightly toolchain
- `llvm-tools-preview` rustup component
- `xorriso` (for ISO creation)
- `qemu` (for testing)

### Build and Run

```bash
# Build kernel + ISO
make iso

# Run in QEMU with GUI window and network
make run-gui

# Run in QEMU with serial console only
make run

# Clean build artifacts
make clean
```

In the QEMU window, type `help` to see available commands.

## Architecture

### Memory Layout

Higher-half kernel at virtual address `0xffffffff80000000`. Physical load address is determined at runtime by Limine — never hardcode `0x100000`.

### Boot Sequence

1. Limine loads kernel and provides framebuffer, memory map, and executable address
2. Custom 128KB stack setup
3. SSE enable for compiler auto-vectorization
4. COM1 serial port init (9600 baud, 8N1)
5. Framebuffer text output init
6. IDT and PIC initialization
7. USB controller scan and keyboard init
8. Network device init (VirtIO-net)
9. Interactive shell

### Network Stack

The network stack is kernel-embedded for the MVP:

- **VirtIO-net PCI driver** (`kernel/src/net/virtio.rs`) — QEMU device detection, queue management, TX/RX
- **Ethernet** (`kernel/src/net/ethernet.rs`) — framing and parsing
- **ARP** (`kernel/src/net/arp.rs`) — MAC address resolution
- **IPv4** (`kernel/src/net/ip.rs`) — packet construction and routing
- **ICMP** (`kernel/src/net/icmp.rs`) — ping support
- **TCP/HTTP** — in development

### Keyboard Input Chain

The shell reads from multiple sources in priority order:
1. USB HID keyboard (EHCI/UHCI)
2. PS/2 keyboard
3. COM1 serial input

### Project Structure

```
kacos/
├── kernel/
│   ├── src/
│   │   ├── main.rs          # Entry point
│   │   ├── lib.rs           # Module declarations
│   │   ├── boot.rs          # Limine requests
│   │   ├── framebuffer.rs   # Framebuffer + serial output
│   │   ├── interrupts.rs    # PIC + interrupt stubs
│   │   ├── idt.rs           # Interrupt descriptor table
│   │   ├── keyboard.rs      # PS/2 keyboard driver
│   │   ├── memory.rs        # Memory map + virt_to_phys
│   │   ├── pci.rs           # PCI bus scanner
│   │   ├── shell.rs         # Command interpreter
│   │   ├── commands.rs      # Built-in commands
│   │   ├── usb/             # USB stack
│   │   │   ├── mod.rs
│   │   │   ├── ehci.rs
│   │   │   ├── uhci.rs
│   │   │   └── hid.rs
│   │   └── net/             # Network stack
│   │       ├── mod.rs
│   │       ├── virtio.rs
│   │       ├── ethernet.rs
│   │       ├── arp.rs
│   │       ├── ip.rs
│   │       └── icmp.rs
│   ├── Cargo.toml
│   └── x86_64-kacos.json    # Custom Rust target
├── limine/                  # Bootloader binaries
├── linker.ld                # Linker script
├── limine.conf              # Bootloader config
├── Makefile
└── CLAUDE.md                # Developer guidance for Claude Code
```

## Security

KACOS is designed with security as a core principle:

- **No `alloc` in kernel** (for now) — prevents heap-based attacks
- **Volatile memory access** for all hardware descriptors — prevents compiler optimization bugs
- **Assembly interrupt stubs** — avoids SSE alignment crashes from `extern "x86-interrupt"`
- **Minimal attack surface** — only necessary drivers and protocols

See `SECURITY.md` for detailed security practices.

## Roadmap

### Phase 1: Agent MVP (Weeks)
- [x] VirtIO-net driver
- [x] Ethernet, ARP, IP, ICMP
- [ ] TCP + HTTP client
- [ ] JSON parser
- [ ] Kernel-space agent loop calling remote AI APIs

### Phase 2: Userspace Foundation (Months)
- [ ] Physical page allocator
- [ ] Kernel heap + `alloc` crate
- [ ] Paging / virtual memory
- [ ] Process scheduler + context switching
- [ ] System calls (`int 0x80`)
- [ ] ELF loader
- [ ] Ramdisk filesystem

### Phase 3: Local Inference (Months)
- [ ] GGUF model loader
- [ ] Transformer inference engine
- [ ] BPE tokenizer
- [ ] TinyLlama 1.1B support

### Phase 4: Real Agent Frameworks (Months → Years)
- [ ] musl libc port
- [ ] MicroPython / QuickJS runtime
- [ ] FAT32 / ext2 filesystem
- [ ] lwIP TCP/IP stack
- [ ] Package manager (`.kpkg`)

## License

MIT License — see `LICENSE` file.

## Acknowledgments

- [Limine](https://github.com/limine-bootloader/limine) bootloader
- [OSDev Wiki](https://wiki.osdev.org/) for x86_64 architecture references
- QEMU for excellent virtualization and debugging support
