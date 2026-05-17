#define SYS_read     0
#define SYS_write    1
#define SYS_open     2
#define SYS_close    3
#define SYS_fork     57
#define SYS_exit     60
#define SYS_wait4    61
#define SYS_execve   59
#define SYS_ioctl    16
#define SYS_socket   41
#define SYS_connect  42
#define SYS_reboot   169
#define SYS_mount    165
#define SYS_mkdir    83
#define SYS_mknod    133
#define SYS_dup2     33

typedef unsigned long size_t;
typedef long ssize_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
#define NULL ((void*)0)

static void sys_write(int fd, const char *buf, size_t len) {
    asm volatile(
        "syscall"
        : : "a"(SYS_write), "D"(fd), "S"(buf), "d"(len)
        : "rcx", "r11", "memory"
    );
}

static int sys_open(const char *path, int flags, int mode) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_open), "D"(path), "S"(flags), "d"(mode)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static void sys_close(int fd) {
    asm volatile(
        "syscall"
        : : "a"(SYS_close), "D"(fd)
        : "rcx", "r11", "memory"
    );
}

static int sys_read(int fd, char *buf, size_t len) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_read), "D"(fd), "S"(buf), "d"(len)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_fork(void) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_fork)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_execve(const char *path, char *const argv[], char *const envp[]) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_execve), "D"(path), "S"(argv), "d"(envp)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static void sys_exit(int code) {
    asm volatile(
        "syscall"
        : : "a"(SYS_exit), "D"(code)
        : "memory"
    );
    __builtin_unreachable();
}

static int sys_wait4(int pid, int *status, int options, void *rusage) {
    int ret;
    register void *r10_ asm("r10") = rusage;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_wait4), "D"(pid), "S"(status), "d"(options), "r"(r10_)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_reboot(int magic1, int magic2, int cmd, void *arg) {
    int ret;
    register void *r10_ asm("r10") = arg;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_reboot), "D"(magic1), "S"(magic2), "d"(cmd), "r"(r10_)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_mount(const char *src, const char *tgt, const char *fstype, unsigned long flags, const void *data) {
    int ret;
    register unsigned long r10_ asm("r10") = flags;
    register const void *r8_ asm("r8") = data;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_mount), "D"(src), "S"(tgt), "d"(fstype), "r"(r10_), "r"(r8_)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_mkdir(const char *path, int mode) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_mkdir), "D"(path), "S"(mode)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_mknod(const char *path, int mode, unsigned dev) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_mknod), "D"(path), "S"(mode), "d"(dev)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_socket(int domain, int type, int protocol) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_socket), "D"(domain), "S"(type), "d"(protocol)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_connect(int fd, const void *addr, int len) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_connect), "D"(fd), "S"(addr), "d"(len)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_ioctl(int fd, unsigned long req, void *arg) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_ioctl), "D"(fd), "S"(req), "d"(arg)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static size_t strlen_(const char *s) {
    size_t n = 0;
    while (s[n]) n++;
    return n;
}

static int strcmp_(const char *a, const char *b) {
    while (*a && *a == *b) { a++; b++; }
    return *(unsigned char *)a - *(unsigned char *)b;
}

static void print(const char *s) {
    sys_write(1, s, strlen_(s));
}

static void printn(const char *s) {
    print(s);
    print("\n");
}

static char linebuf[512];
static int linepos = 0;

static void read_line(void) {
    linepos = 0;
    while (linepos < sizeof(linebuf) - 1) {
        char c;
        int n = sys_read(0, &c, 1);
        if (n <= 0) break;
        if (c == '\n') break;
        linebuf[linepos++] = c;
    }
    linebuf[linepos] = '\0';
}

static int starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++; prefix++;
    }
    return 1;
}

static void cat_file(const char *path) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("cat: cannot open file");
        return;
    }
    char buf[256];
    int n;
    while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
        sys_write(1, buf, n);
    }
    sys_close(fd);
}

static void do_ps(void) {
    printn("PID    PPID   CMD");
    // Simplified - just list /proc entries
    printn("(ps not fully implemented)");
}

static void do_reboot(void) {
    sys_reboot(0xfee1dead, 672274793, 0x01234567, NULL);
}

static void do_poweroff(void) {
    sys_reboot(0xfee1dead, 672274793, 0x4321fedc, NULL);
}

static void do_http_get(const char *host, const char *path) {
    int fd = sys_socket(2, 1, 0); // AF_INET, SOCK_STREAM, 0
    if (fd < 0) {
        printn("http: socket failed");
        return;
    }

    // Simple sockaddr_in for 10.0.2.2 (QEMU gateway)
    struct {
        uint16_t family;
        uint16_t port;
        uint32_t addr;
        char pad[8];
    } sa = {2, 0x5000, 0x0202000a, {0}}; // port 80, 10.0.2.2

    if (sys_connect(fd, &sa, sizeof(sa)) < 0) {
        printn("http: connect failed");
        sys_close(fd);
        return;
    }

    char req[512];
    int len = 0;
    const char *p = "GET ";
    while (*p) req[len++] = *p++;
    p = path;
    while (*p) req[len++] = *p++;
    p = " HTTP/1.1\r\nHost: ";
    while (*p) req[len++] = *p++;
    p = host;
    while (*p) req[len++] = *p++;
    p = "\r\nConnection: close\r\n\r\n";
    while (*p) req[len++] = *p++;

    sys_write(fd, req, len);

    char buf[256];
    int n;
    while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
        sys_write(1, buf, n);
    }
    sys_close(fd);
}

static void shell_loop(void) {
    printn("");
    printn("========================================");
    printn("  Nymoris Agentic AI Operating System");
    printn("========================================");
    printn("");

    while (1) {
        print("$ ");
        read_line();

        if (linepos == 0) continue;

        if (strcmp_(linebuf, "help") == 0) {
            printn("Commands:");
            printn("  help     Show this help");
            printn("  ps       List processes");
            printn("  cat      Show file contents");
            printn("  reboot   Reboot system");
            printn("  exit     Power off");
            printn("  http     HTTP GET (hardcoded to 10.0.2.2)");
        } else if (strcmp_(linebuf, "ps") == 0) {
            do_ps();
        } else if (strcmp_(linebuf, "reboot") == 0) {
            do_reboot();
        } else if (strcmp_(linebuf, "exit") == 0 || strcmp_(linebuf, "quit") == 0) {
            do_poweroff();
        } else if (starts_with(linebuf, "cat ")) {
            cat_file(linebuf + 4);
        } else if (starts_with(linebuf, "http ")) {
            do_http_get("10.0.2.2", "/");
        } else {
            printn("Unknown command. Type 'help' for available commands.");
        }
    }
}

void _start(void) {
    // Open /dev/console for stdin/stdout/stderr
    int fd = sys_open("/dev/console", 2, 0); // O_RDWR
    if (fd >= 0) {
        if (fd != 0) {
            asm volatile("syscall" : : "a"(SYS_dup2), "D"(fd), "S"(0) : "rcx", "r11", "memory");
        }
        if (fd != 1) {
            asm volatile("syscall" : : "a"(SYS_dup2), "D"(fd), "S"(1) : "rcx", "r11", "memory");
        }
        if (fd != 2) {
            asm volatile("syscall" : : "a"(SYS_dup2), "D"(fd), "S"(2) : "rcx", "r11", "memory");
        }
        if (fd > 2) {
            sys_close(fd);
        }
    }

    // Mount basic filesystems
    sys_mkdir("/proc", 0755);
    sys_mkdir("/sys", 0755);
    sys_mkdir("/dev", 0755);
    sys_mkdir("/tmp", 0755);
    sys_mount("proc", "/proc", "proc", 0, NULL);
    sys_mount("sysfs", "/sys", "sysfs", 0, NULL);
    sys_mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    sys_mount("tmpfs", "/tmp", "tmpfs", 0, NULL);

    printn("[Nymoris] init started");

    shell_loop();

    sys_exit(0);
}
