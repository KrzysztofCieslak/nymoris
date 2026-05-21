# Nymoris — Agentic AI Operating System

Nymoris (pronounced "nemoris") is a minimal Linux-based operating system designed to host and run autonomous AI agents. It boots a Linux kernel with a custom initramfs containing a lightweight agent runtime.

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

- [x] Boots Linux kernel + custom initramfs in QEMU (serial & GUI)
- [x] Interactive agent shell with 40+ built-in commands
- [x] Basic filesystem operations (cat, ls, mkdir, cp, mv, rm, touch, hexdump, stat, base64, ln, cmp, write, append, replace, sort)
- [x] HTTP client via raw sockets (GET + POST)
- [x] ICMP ping via raw sockets
- [x] System control (reboot, poweroff, free, uptime, ps, kill)
- [x] Ctrl+C interrupts long-running commands (sleep, agent, background jobs)
- [x] Environment variables, aliases, command history, job control
- [x] Semicolon command separators (`cmd1; cmd2; cmd3`)
- [x] Filesystem persistence — auto-mount block devices (ext4/ext2/vfat)
- [x] Startup scripts (`/data/nymoris.rc`, `/nymoris.rc`)
- [x] Interactive agent loop (`agent` command)
- [x] AI API integration — `ask` command calls OpenAI-compatible API
- [x] Configurable API endpoint, model, Bearer auth, and system prompt
- [x] **Agent output capture** — auto mode feeds command output back to AI
- [x] Agent conversation history (16 messages)
- [x] Minimal JSON parser for API responses
- [x] Local LLM inference (llm.c + convert.py)

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
x86_64-elf-gcc -nostdlib -static -O2 -fno-inline -o /tmp/nymoris-init init.c llm.c

# Create initramfs
mkdir -p initramfs/{dev,proc,sys,tmp,bin}
cp /tmp/nymoris-init initramfs/init
chmod +x initramfs/init
cp -f test_model.nymollm initramfs/model.nymollm 2>/dev/null || true
cd initramfs && find . | cpio -o -H newc | gzip > ../initramfs.cpio.gz

# Boot in QEMU (serial)
qemu-system-x86_64 -kernel vmlinuz -initrd initramfs.cpio.gz \
  -m 512M -nographic -append "console=ttyS0 panic=1"

# Boot in QEMU (GUI window)
qemu-system-x86_64 -kernel vmlinuz -initrd initramfs.cpio.gz \
  -m 512M -append "console=tty0 panic=1 quiet loglevel=0"
```

## Project Structure

```
nymoris/
├── vmlinuz              # Linux kernel (Alpine LTS)
├── initramfs.cpio.gz    # Gzipped cpio initramfs
├── init.c               # Custom init program (PID 1)
├── llm.c                # Local LLM inference engine
├── llm.h                # LLM engine header
├── Makefile             # Build automation
├── CLAUDE.md            # Developer guidance
├── convert.py           # Model conversion script
└── agent/               # Experimental Rust agent
```

### Boot Sequence

1. Linux kernel boots and mounts the initramfs
2. Kernel executes `/init` as PID 1
3. `init` mounts proc, sysfs, devtmpfs, tmpfs (on `/tmp` and `/data`)
4. `init` opens `/dev/console` for stdin/stdout/stderr
5. Interactive agent shell starts

### Writable Storage

`/tmp` and `/data` are mounted as tmpfs (writable in RAM). Files persist during the session but are lost on reboot.

Use `/data/bin/` for installing external binaries — the shell searches this path when running commands via `run` or directly.

```bash
install 10.0.2.2 /hello /data/bin/hello
hello
```

### Filesystem Persistence

Nymoris automatically detects and mounts persistent storage on boot. It probes common block devices (`/dev/sda1`, `/dev/vda1`, `/dev/hda1`, `/dev/nvme0n1p1`) and tries `ext4`, `ext2`, and `vfat` filesystems, mounting the first working one to `/data`.

If no block device is found, `/data` falls back to tmpfs (RAM-only, lost on reboot).

**Manual mount:**
```bash
lsblk                          # List available block devices
mount /dev/sda1 /data ext4     # Mount a specific device
umount /data                   # Unmount
```

**Testing in QEMU with a disk image:**
```bash
# Create a disk image
dd if=/dev/zero of=disk.img bs=1M count=100
mkfs.ext4 disk.img

# Run QEMU with the disk attached
make run QEMU_EXTRA="-drive file=disk.img,format=raw,id=disk -device virtio-blk-pci,drive=disk"
```

### Startup Scripts

On boot, Nymoris automatically sources startup scripts if they exist:

1. `/data/nymoris.rc` — on persistent storage (survives reboot)
2. `/nymoris.rc` — in the initramfs (read-only, baked into the image)

Use this to pre-configure environment variables, aliases, and run initial commands:

```bash
# /data/nymoris.rc
export NYMORIS_API_KEY=sk-your-key-here
export NYMORIS_API_MODEL=gpt-4o
export NYMORIS_API_HOST=10.0.2.2
export NYMORIS_API_PATH=/v1/chat/completions
alias h=history
alias ll='ls -la'
echo "Welcome back, agent."
```

Lines starting with `#` are comments and ignored.

## Shell Commands

### Filesystem

| Command | Description |
|---------|-------------|
| `ls [dir]` | List directory |
| `cat <file>` | Display file contents |
| `head <file> [n]` | Show first n lines |
| `tail <file> [n]` | Show last n lines |
| `hexdump <file>` | Hex dump file contents |
| `base64 <file>` | Base64 encode file |
| `base64 -d <file>` | Base64 decode file |
| `grep <pattern> <file>` | Search for pattern |
| `wc <file>` | Count lines/words/bytes |
| `sort <file>` | Sort lines alphabetically |
| `mkdir <dir>` | Create directory |
| `rmdir <dir>` | Remove empty directory |
| `rm <file>` | Remove file |
| `cp <src> <dst>` | Copy file |
| `mv <src> <dst>` | Move/rename file |
| `touch <file>` | Create empty file |
| `write <file> <data>` | Write content to file (overwrite) |
| `append <file> <data>` | Append content to file |
| `replace <file> <old> <new>` | Replace first occurrence in file |
| `chmod <mode> <file>` | Change permissions |
| `stat <file>` | Show file metadata (size, mode, uid, gid, links) |
| `ln [-s] <target> <link>` | Create hard/symbolic link |
| `cmp <file1> <file2>` | Compare two files |
| `find <dir> <name>` | Find file by name |
| `cd <dir>` | Change directory |
| `pwd` | Print working directory |

### System

| Command | Description |
|---------|-------------|
| `ps` | List processes |
| `kill <pid> [sig]` | Send signal to process |
| `free` | Show memory usage |
| `uptime` | Show system uptime |
| `df` | Show disk usage |
| `mount` | Show mounted filesystems |
| `mount <dev> <dir> <fstype>` | Mount block device (ext4/ext2/vfat) |
| `umount <dir>` | Unmount filesystem |
| `lsblk` | List block devices |
| `netstat` | Show TCP connections |
| `dmesg` | Show kernel messages |
| `uname` | Show system info |
| `hostname` | Show hostname |
| `whoami` | Show current user |
| `id` | Show user/group IDs |
| `date` | Show date and time |
| `clear` | Clear screen |
| `reboot` | Reboot system |
| `exit` | Power off |

### Network

| Command | Description |
|---------|-------------|
| `ping <host>` | Ping host via ICMP |
| `http <host> [path]` | HTTP GET |
| `wget <host> <path> <outfile>` | Download file |
| `install <host> <path> <name>` | Download binary to `/data/bin/` |
| `tar x <file>` | Extract tar archive |

### Environment & Shell

| Command | Description |
|---------|-------------|
| `env` | Show environment variables |
| `export KEY=VALUE` | Set environment variable |
| `alias [name[=value]]` | Show/set alias |
| `unalias <name>` | Remove alias |
| `history` | Show command history |
| `sleep <secs>` | Sleep |
| `echo <text>` | Print text |
| `seq [start] [end] [step]` | Print number sequence |
| `source <file>` | Execute commands from file |
| `jobs` | List background jobs |
| `cmd &` | Run command in background |
| `cmd > file` | Redirect output to file |
| `cmd < file` | Redirect input from file |
| `cmd1; cmd2` | Run multiple commands sequentially |

### Agent & LLM

| Command | Description |
|---------|-------------|
| `agent` | Start AI agent loop |
| `llm <model> <prompt>` | Run local LLM inference |

### Agent Loop Commands

Inside the `agent` loop:

| Command | Description |
|---------|-------------|
| `ask <prompt>` | Ask AI (calls remote API) |
| `auto [n] [s]` | Start autonomous mode (n iterations, s seconds interval) |
| `run <cmd>` | Execute binary command |
| `exec <cmd>` | Execute built-in shell command |
| `read <file>` | Read file contents |
| `write <file> <data>` | Write to file (overwrite) |
| `append <file> <data>` | Append to file |
| `http <host> [path]` | HTTP GET |
| `post <host> <path> <body>` | HTTP POST |
| `sleep <secs>` | Sleep |
| `history` | Show conversation history |
| `reset` | Clear conversation history |
| `config` | Show API configuration |
| `done` / `quit` | Exit agent loop |

## Local LLM Inference

Run models entirely offline with the built-in inference engine:

```bash
llm model.nymollm "What is the capital of France?"
```

### Converting Models

Use `convert.py` to convert HuggingFace models to the NYMOLLM format:

```bash
# Full precision (default)
python convert.py --model gpt2 --output model.nymollm

# Half precision — 2x smaller
python convert.py --model gpt2 --output model.nymollm --dtype f16

# 4-bit quantization — 8x smaller
python convert.py --model gpt2 --output model.nymollm --dtype q4_0
```

Supported dtypes: `f32`, `f16`, `q4_0`. The inference engine dequantizes to f32 on load.

## Configuring the AI API

Set environment variables before entering the agent loop:

```bash
export NYMORIS_API_KEY=sk-your-key-here
export NYMORIS_API_MODEL=gpt-4o
export NYMORIS_API_HOST=api.openai.com
export NYMORIS_API_PATH=/v1/chat/completions
agent
```

For local models (e.g., Ollama), use without an API key:

```bash
export NYMORIS_API_MODEL=llama3.2
export NYMORIS_API_HOST=10.0.2.2
export NYMORIS_API_PATH=/v1/chat/completions
agent
ask hello
```

### Custom System Prompt

Change the agent's behavior with a custom system prompt:

```bash
export NYMORIS_SYSTEM_PROMPT="You are a security researcher. Use exec nmap, exec ping, and exec cat to analyze the network."
agent
ask "scan the local network"
```

The default prompt instructs the AI to use available tools (run, exec, read, write, http, post) and respond with tool calls only.

### Using HTTPS APIs (OpenAI, Claude, etc.)

Nymoris has no TLS library. Use the included Python proxy to bridge HTTP → HTTPS:

**1. Start the proxy on your host machine:**

```bash
python3 scripts/https_proxy.py --listen 8080 --target api.openai.com:443
```

**2. Run QEMU with port forwarding:**

```bash
make run QEMU_EXTRA="-netdev user,id=net0,hostfwd=tcp::8080-:80"
```

Or add to the `Makefile` `QEMU_EXTRA` variable permanently.

**3. In nymoris, configure for the proxy:**

```bash
export NYMORIS_API_KEY=sk-your-key-here
export NYMORIS_API_HOST=10.0.2.2
export NYMORIS_API_PATH=/v1/chat/completions
agent
ask "list files in current directory"
```

The proxy listens on HTTP port 8080, forwards requests to HTTPS on the real API, and returns the response. No TLS code needed inside nymoris.

In autonomous mode, the agent:
1. Sends a prompt to the AI API
2. Receives a tool call (`exec`, `run`, `read`, `write`)
3. Executes it and **captures the output**
4. Feeds the captured output back as a "system" message
5. Repeats

## Why Linux?

Nymoris was previously built as a custom kernel from scratch. While educational, that approach required years of work to achieve basic functionality. By building on Linux:

- **Proven hardware support** — drivers for virtually all x86_64 hardware
- **Network stack** — TCP/IP, routing, firewalling
- **Security** — mature security model, namespaces, cgroups
- **Focus** — we can build the agent layer instead of kernel infrastructure

The custom initramfs approach keeps the system minimal and purpose-built while leveraging Linux's robust foundation.

## Real Hardware Deployment

Nymoris can be deployed to physical x86_64 hardware via bootable ISO, USB, or PXE.

### Bootable ISO

On a Linux machine (or Docker container):

```bash
make build
bash scripts/deploy/mkiso.sh
# Creates nymoris.iso
qemu-system-x86_64 -cdrom nymoris.iso -m 512M
```

Write to USB:
```bash
sudo dd if=nymoris.iso of=/dev/sdX bs=4M status=progress
```

See `scripts/deploy/README.md` for GRUB, syslinux, and PXE setup details.

## Roadmap

### Phase 1: Agent MVP (Done)
- [x] Linux kernel + custom initramfs boot
- [x] Minimal init with shell (40+ commands)
- [x] TCP networking (via Linux kernel sockets)
- [x] HTTP client with JSON parsing
- [x] Interactive agent loop
- [x] Agent calling remote AI APIs
- [x] Agent output capture (auto mode)
- [x] Configurable API endpoint + Bearer auth
- [x] Local LLM inference (llm.c)

### Phase 2: Advanced Agent
- [x] HTTPS proxy for AI APIs (`scripts/https_proxy.py`)
- [x] Better HTTP client (redirects, chunked encoding, timeouts)
- [x] Writable `/data` tmpfs mount
- [x] Agent `post` tool for HTTP POST
- [x] Agent runs in forked child (crash isolation)
- [x] Container-style isolation (mount + PID namespaces)
- [x] Install command + `/data/bin` executable path
- [x] Better local LLM (f16, Q4_0 quantization)
- [x] Tar archive extractor
- [x] Netstat command
- [x] Semicolon command separators (`cmd1; cmd2`)
- [x] Filesystem persistence — auto-mount block devices (ext4/ext2/vfat)
- [x] Startup scripts (`/data/nymoris.rc`)
- [x] Hexdump command
- [x] Base64 encode/decode
- [x] Ctrl+C interrupts long-running commands
- [x] Custom agent system prompt (NYMORIS_SYSTEM_PROMPT)
- [x] Expanded agent conversation history (16 messages)
- [x] Stat command (file metadata)
- [x] Ln command (hard/symbolic links)
- [x] Cmp command (compare files)
- [x] Write/append commands for file editing
- [x] Replace command (find/replace in files)
- [x] Sort command (alphabetical line sort)
- [ ] ELF Loader

### Phase 3: Production
- [ ] Full agent framework support
- [x] Package manager precursor (`install`, `tar x`)
- [x] Real hardware deployment scripts (ISO, GRUB, PXE)
- [ ] Distributed agent clusters

## Security

- Minimal initramfs — only essential binaries
- No network services listening by default
- Agent runs as root (intentional for full system control)
- Agent is isolated in mount + PID namespaces (changes in `/tmp` and `/data` don't affect host)
- Agent crashes don't panic the system (runs in forked child process)
- All operations auditable via shell history

## License

MIT License — see `LICENSE` file.
