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
- [x] Basic filesystem operations (cat, ls, mkdir, cp, mv, rm, touch)
- [x] HTTP client via raw sockets (GET + POST)
- [x] ICMP ping via raw sockets
- [x] System control (reboot, poweroff, free, uptime, ps, kill)
- [x] Environment variables, aliases, command history, job control
- [x] Interactive agent loop (`agent` command)
- [x] AI API integration — `ask` command calls OpenAI-compatible API
- [x] Configurable API endpoint, model, and Bearer auth
- [x] **Agent output capture** — auto mode feeds command output back to AI
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

## Shell Commands

### Filesystem

| Command | Description |
|---------|-------------|
| `ls [dir]` | List directory |
| `cat <file>` | Display file contents |
| `head <file> [n]` | Show first n lines |
| `tail <file> [n]` | Show last n lines |
| `grep <pattern> <file>` | Search for pattern |
| `wc <file>` | Count lines/words/bytes |
| `mkdir <dir>` | Create directory |
| `rmdir <dir>` | Remove empty directory |
| `rm <file>` | Remove file |
| `cp <src> <dst>` | Copy file |
| `mv <src> <dst>` | Move/rename file |
| `touch <file>` | Create empty file |
| `chmod <mode> <file>` | Change permissions |
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
| `write <file> <data>` | Write to file |
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
- [ ] File system persistence (ext4/FAT driver)
- [ ] ELF Loader

### Phase 2: Advanced Agent
- [x] HTTPS proxy for AI APIs (`scripts/https_proxy.py`)
- [x] Better HTTP client (redirects, chunked encoding, timeouts)
- [x] Writable `/data` tmpfs mount
- [x] Agent `post` tool for HTTP POST
- [x] Agent runs in forked child (crash isolation)
- [x] Container-style isolation (mount + PID namespaces)
- [x] Install command + `/data/bin` executable path
- [x] Better local LLM (f16, Q4_0 quantization)
- [ ] File system persistence (ext4/FAT driver)
- [ ] ELF Loader

### Phase 3: Production
- [ ] Full agent framework support
- [ ] Package manager
- [ ] Real hardware deployment
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
