# Nymoris — Agentic AI Operating System

Nymoris (pronounced "kay-kos") is a minimal Linux-based operating system designed to host and run autonomous AI agents. It boots a Linux kernel with a custom initramfs containing a lightweight agent runtime.

## Vision

Nymoris is built as a dedicated runtime for agentic AI — software that perceives, decides, and acts on behalf of users. Unlike a general-purpose Linux distribution, Nymoris is purpose-built:

- Minimal attack surface — only what's needed for agents
- Direct control over the system environment
- Network connectivity for remote AI APIs
- Extensible agent shell with tool-calling capabilities
- Foundation for running agent frameworks

## Architecture

Nymoris is built on the Linux kernel (Alpine LTS) with a custom initramfs:

```
Linux Kernel (Alpine LTS) → Custom Initramfs → nymoris-init (PID 1)
                                              → Agent Shell
                                              → Built-in tools
```

The init program (`init.c`) is a minimal C program using raw Linux syscalls — no libc dependency. It runs as PID 1 and provides an interactive shell for agent operations.

## Current Status

- [x] Boots Linux kernel + custom initramfs in QEMU
- [x] Interactive agent shell with built-in commands
- [x] Basic filesystem operations (cat, ls, mkdir)
- [x] HTTP client via raw sockets
- [x] System control (reboot, poweroff)
- [ ] TCP networking stack
- [ ] Agent loop with AI API integration
- [ ] Local LLM inference

## Quick Start

### Prerequisites

- `qemu` (for testing)
- `x86_64-elf-gcc` (for compiling init):
  ```bash
  brew install x86_64-elf-gcc
  ```
- `cpio` (for initramfs creation — usually preinstalled)

### Build and Run

```bash
# Build initramfs + run in QEMU with serial console
make run

# Run in QEMU with GUI window
make run-gui

# Clean build artifacts
make clean
```

In the serial console, type `help` to see available commands.

### Manual Build

```bash
# Compile init program
x86_64-elf-gcc -nostdlib -static -O2 -o /tmp/nymoris-init init.c

# Create initramfs
mkdir -p initramfs/{dev,proc,sys,tmp,bin}
cp /tmp/nymoris-init initramfs/init
chmod +x initramfs/init
cd initramfs && find . | cpio -o -H newc | gzip > ../initramfs.cpio.gz

# Boot in QEMU
qemu-system-x86_64 -kernel vmlinuz -initrd initramfs.cpio.gz \
  -m 512M -nographic -append "console=ttyS0 panic=1"
```

## Project Structure

```
nymoris/
├── vmlinuz              # Linux kernel (Alpine LTS)
├── initramfs.cpio.gz    # Gzipped cpio initramfs
├── init.c               # Custom init program (PID 1)
├── agent/               # Rust agent program (experimental)
├── Makefile             # Build automation
└── CLAUDE.md            # Developer guidance
```

### Boot Sequence

1. Linux kernel boots and mounts the initramfs
2. Kernel executes `/init` as PID 1
3. `init` opens `/dev/console` for stdin/stdout/stderr
4. `init` mounts proc, sysfs, devtmpfs, tmpfs
5. Interactive agent shell starts

### Agent Shell Commands

| Command | Description |
|---------|-------------|
| `help` | Show available commands |
| `cat <file>` | Display file contents |
| `ps` | List processes |
| `http` | HTTP GET to QEMU gateway |
| `reboot` | Reboot the system |
| `exit` | Power off |

## Why Linux?

Nymoris was previously built as a custom kernel ("Nymoris") from scratch. While educational, that approach required years of work to achieve basic functionality. By building on Linux:

- **Proven hardware support** — drivers for virtually all x86_64 hardware
- **Network stack** — TCP/IP, routing, firewalling
- **Security** — mature security model, namespaces, cgroups
- **Focus** — we can build the agent layer instead of kernel infrastructure

The custom initramfs approach keeps the system minimal and purpose-built while leveraging Linux's robust foundation.

## Roadmap

### Phase 1: Agent MVP (Weeks)
- [x] Linux kernel + custom initramfs boot
- [x] Minimal init with shell
- [ ] TCP networking
- [ ] HTTP client with JSON parsing
- [ ] Agent loop calling remote AI APIs

### Phase 2: Advanced Agent (Months)
- [ ] File system persistence
- [ ] Multi-process agent runtime
- [ ] Local LLM inference (GGUF)
- [ ] Container-style isolation

### Phase 3: Production (Months)
- [ ] Full agent framework support
- [ ] Package manager
- [ ] Real hardware deployment
- [ ] Distributed agent clusters

## Security

- Minimal initramfs — only essential binaries
- No network services listening by default
- Agent runs as root (intentional for full system control)
- All operations auditable via shell history

## License

MIT License — see `LICENSE` file.
