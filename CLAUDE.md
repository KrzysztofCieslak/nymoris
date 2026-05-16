# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

Nymoris is a minimal x86_64 hobby OS written in Rust (`no_std`), targeting bare metal. It boots via the Limine bootloader and runs on both QEMU and real hardware (MacBook Pro 2010 and similar x86_64 machines).

## Build System

The top-level `Makefile` orchestrates the build. The kernel is built with Rust nightly using `-Zbuild-std`.

**Prerequisites:**
- `rustup` with nightly toolchain
- `llvm-tools-preview` rustup component
- `xorriso` (for ISO creation)
- `qemu` (for testing)
- Limine bootloader binaries in `limine/` (pre-built, do not modify)

**Common commands:**

```bash
# Build kernel + ISO
make iso

# Run in QEMU with serial console (no GUI window)
make run

# Run in QEMU with GUI window + serial output
make run-gui

# Run in QEMU with interrupt debugging
make run-debug

# Clean build artifacts
make clean
```

**Kernel-only build (no ISO):**
```bash
cd kernel
RUSTFLAGS="-C link-arg=-T../linker.ld" cargo +nightly build \
  -Zbuild-std=core,compiler_builtins \
  -Zbuild-std-features=compiler-builtins-mem \
  -Zjson-target-spec --target x86_64-nymoris.json --release
```

The kernel binary is produced at `kernel/target/x86_64-nymoris/release/nymoris`.

**Dependencies** (see `kernel/Cargo.toml`):
- `limine` — Limine bootloader protocol
- `x86_64` — x86_64-specific instructions and structures
- `pic8259` — PIC (Programmable Interrupt Controller)
- `pc-keyboard` — PS/2 keyboard scancode decoding
- `spin`, `lazy_static` — Synchronization primitives for `no_std`
- `volatile` — Volatile memory operations

## Architecture

### Memory Layout

The kernel is a **higher-half kernel**. The linker script (`linker.ld`) places the kernel at virtual address `0xffffffff80000000`. The physical load address is **not fixed** — Limine loads the kernel at whatever physical address it chooses. The `ExecutableAddressRequest` in `kernel/src/boot.rs` provides the actual physical base at runtime. **Always use `virt_to_phys()` (which uses the Limine response) rather than hardcoding `0x100000`.

### Boot Sequence (`kernel/src/main.rs`)

1. Set up custom 128KB stack (Limine's default stack may be too small)
2. Enable SSE (compiler may auto-vectorize)
3. Initialize COM1 serial port (0x3F8) for debug output
4. `framebuffer::init()` — set up framebuffer text output
5. `idt::init()` — load Interrupt Descriptor Table
6. `interrupts::init()` — initialize and remap PIC (timer + keyboard unmasked)
7. `keyboard::init()` — PS/2 keyboard driver (placeholder, actual init is in `interrupts::init()`)
8. `memory::print_memmap()` — print Limine memory map
9. `usb::init()` — scan PCI for USB controllers and initialize EHCI/UHCI
10. `shell::run()` — enter interactive shell loop

### Output System (`kernel/src/framebuffer.rs`)

All output goes through `println!` / `print!` macros, which write simultaneously to:
- The Limine-provided framebuffer (8x16 bitmap font, RGB pixels)
- COM1 serial port (0x3F8)

The framebuffer writer detects BPP (16/24/32) and shifts from the Limine response. If no framebuffer is available, output falls back to serial only.

The framebuffer handles `\n` (newline) and `\x08` (backspace) by moving the cursor. Backspace erases the pixel area under the cursor position.

### VGA Text Mode (`kernel/src/vga.rs`)

An alternative text-mode output driver for 80x25 VGA text mode at `0xB8000` exists but is **not currently used** in the boot sequence. `framebuffer.rs` is the active output path.

### Interrupt Handling (`kernel/src/interrupts.rs`)

The project does **not** use `extern "x86-interrupt"`. The compiler-generated prologue for that ABI uses `movaps` which crashes when the stack is misaligned (CPU pushes 24–32 bytes for same-ring interrupts). Instead, raw assembly stubs save registers, call normal `extern "C"` Rust handler functions, then restore and `iretq`.

Timer interrupt (IRQ0) polls both EHCI and UHCI keyboards. Keyboard interrupt (IRQ1) reads scancodes from PS/2 port 0x60.

### PCI Scanning (`kernel/src/pci.rs`)

`pci::scan_all()` enumerates PCI devices and returns them in a caller-provided array. `usb::init()` uses this to find USB controllers by class `0x0C` subclass `0x03`.

### USB Stack (`kernel/src/usb/`)

USB controller detection happens via PCI scan (`kernel/src/pci.rs`). The code looks for class `0x0C` subclass `0x03`:
- `prog_if == 0x20` → EHCI controller
- `prog_if == 0x00` → UHCI controller

**UHCI driver (`kernel/src/usb/uhci.rs`)** is the primary working path for QEMU. Critical implementation details:
- QEMU's UHCI emulation uses a **non-standard TD token bit layout**: PID in bits [7:0], device addr in [14:8], endpoint in [18:15], data toggle in bit 19, MaxLen in [31:21]. The `make_token()` function encodes for this layout.
- USB PID values are the actual 8-bit PID bytes: `0x2D` (SETUP), `0xE1` (OUT), `0x69` (IN).
- HCRESET (bit 1 of Command register) must be issued before writing FLBASEADD to force QEMU to drop cached SeaBIOS frame list state.
- Queue Heads (QH) are required in the frame list; direct TD links cause QEMU HC Process Error.
- `virt_to_phys()` must use Limine's `ExecutableAddressResponse` to compute the correct physical address.
- Control transfer STATUS direction: for no-data transfers (e.g., SET_ADDRESS), STATUS must be IN, not OUT.
- All TD field writes must use `write_volatile`; direct struct field assignments (e.g., `SETUP_TD.link = ...`) may be optimized away.
- HID keyboard input is **de-duplicated** against the previous report: `parse_boot_report()` only emits characters for keycodes that were not present in the previous report, preventing key repeat on every poll.

**EHCI driver (`kernel/src/usb/ehci.rs`)** exists but is less tested. The HID keyboard module (`kernel/src/usb/hid.rs`) targets EHCI structures and also uses report de-duplication.

### Keyboard Input (`kernel/src/shell.rs`)

The shell reads characters from a priority chain:
1. `usb::hid::get_char()` — EHCI HID keyboard
2. `usb::uhci::get_char()` — UHCI HID keyboard
3. `keyboard::get_char()` — PS/2 keyboard
4. Serial input from COM1

The timer interrupt handler calls `poll_keyboard()` for both EHCI and UHCI, which checks if interrupt IN transfers have completed and re-arms them.

### Shell Commands (`kernel/src/commands.rs`)

Available built-in commands:
- `help` — list commands
- `echo <text>` — print text
- `clear` — clear the screen
- `reboot` — reboot via PS/2 controller port 0x64
- `halt` — halt the CPU
- `meminfo` — show Limine memory map
- `about` — show system info
- `panic` — trigger a kernel panic (for testing)

### Serial Debug

COM1 (0x3F8) is initialized in `_start()` at 9600 baud, 8N1. Serial output is used for:
- Early boot debug (before framebuffer is ready)
- Kernel panic messages
- Dual output alongside framebuffer

## Git Workflow

This repository is tracked in Git and pushed to GitHub:
- **Remote**: `git@github.com:KrzysztofCieslak/Nymoris.git`
- **Default branch**: `main`

**Always push changes to GitHub after making edits:**

```bash
git add -A
git commit -m "Describe the change"
git push origin main
```

If the remote has changes you don't have locally, pull first:
```bash
git pull origin main
git push origin main
```

## Important Implementation Notes

- **Static mutable variables** are used extensively for hardware descriptors (TDs, QHs, frame lists). These are in `unsafe` blocks. Always use `write_volatile` and `read_volatile` when accessing them from Rust to prevent compiler optimizations.
- **QEMU testing**: The `pc` machine needs `-usb -device usb-kbd` flags. Without `-usb`, QEMU does not expose the PIIX3 USB controller.
- **Real hardware caveat**: The current UHCI `make_token()` uses QEMU's non-standard bit layout. Real UHCI controllers (e.g., PIIX3 on MacBook Pro 2010) use the standard UHCI spec layout. A runtime or compile-time switch will be needed for real hardware.
- **Frame number timing**: Use the UHCI `FRNUM` register (offset 0x06) for millisecond-scale delays. It increments every 1ms (one USB frame).
- **Framebuffer resolution**: The QEMU window size is controlled by the framebuffer resolution. Add `resolution: WIDTHxHEIGHT` (e.g., `resolution: 640x480`) to the boot entry in `limine.conf` to change the window size.
