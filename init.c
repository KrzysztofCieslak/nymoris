#define SYS_read     0
#define SYS_write    1
#define SYS_open     2
#define SYS_close    3
#define SYS_lseek    8
#define SYS_exit     60
#define SYS_reboot   169
#define SYS_mount    165
#define SYS_mkdir    83
#define SYS_mknod    133
#define SYS_dup2     33
#define SYS_socket   41
#define SYS_connect  42
#define SYS_sendto   44
#define SYS_recvfrom 45
#define SYS_getdents64 217
#define SYS_nanosleep 35
#define SYS_wait4    61
#define SYS_fork     57
#define SYS_execve   59
#define SYS_kill     62
#define SYS_uname    63
#define SYS_gettimeofday 96
#define SYS_getuid   102
#define SYS_getgid   104
#define SYS_statfs   137
#define SYS_getcwd   79
#define SYS_rename   82
#define SYS_unlink   87
#define SYS_chmod    90
#define SYS_chdir    80
#define SYS_rmdir    84
#define SYS_syslog   103

typedef unsigned long size_t;
typedef long ssize_t;
typedef short int16_t;
typedef int int32_t;
typedef long int64_t;
typedef unsigned short uint16_t;
typedef unsigned int uint32_t;
typedef unsigned long uint64_t;
#define NULL ((void*)0)

#include "llm.h"

void sys_write(int fd, const char *buf, size_t len) {
    asm volatile(
        "syscall"
        : : "a"(SYS_write), "D"(fd), "S"(buf), "d"(len)
        : "rcx", "r11", "memory"
    );
}

int sys_open(const char *path, int flags, int mode) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_open), "D"(path), "S"(flags), "d"(mode)
        : "rcx", "r11", "memory"
    );
    return ret;
}

void sys_close(int fd) {
    asm volatile(
        "syscall"
        : : "a"(SYS_close), "D"(fd)
        : "rcx", "r11", "memory"
    );
}

static void sys_exit(int code) {
    asm volatile(
        "syscall"
        : : "a"(SYS_exit), "D"(code)
        : "memory"
    );
    __builtin_unreachable();
}

int sys_read(int fd, char *buf, size_t len) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_read), "D"(fd), "S"(buf), "d"(len)
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

static int sys_sendto(int fd, const void *buf, size_t len, int flags, const void *addr, int addr_len) {
    int ret;
    register int r10_ asm("r10") = flags;
    register const void *r8_ asm("r8") = addr;
    register int r9_ asm("r9") = addr_len;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_sendto), "D"(fd), "S"(buf), "d"(len), "r"(r10_), "r"(r8_), "r"(r9_)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_recvfrom(int fd, void *buf, size_t len, int flags, void *addr, int *addr_len) {
    int ret;
    register int r10_ asm("r10") = flags;
    register void *r8_ asm("r8") = addr;
    register int *r9_ asm("r9") = addr_len;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_recvfrom), "D"(fd), "S"(buf), "d"(len), "r"(r10_), "r"(r8_), "r"(r9_)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_getdents64(int fd, void *buf, int count) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_getdents64), "D"(fd), "S"(buf), "d"(count)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_nanosleep(const void *req, void *rem) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_nanosleep), "D"(req), "S"(rem)
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

static int sys_kill(int pid, int sig) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_kill), "D"(pid), "S"(sig)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_uname(void *buf) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_uname), "D"(buf)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_getuid(void) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_getuid)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_getgid(void) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_getgid)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_gettimeofday(void *tv, void *tz) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_gettimeofday), "D"(tv), "S"(tz)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_statfs(const char *path, void *buf) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_statfs), "D"(path), "S"(buf)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_rmdir(const char *path) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_rmdir), "D"(path)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_syslog(int type, char *buf, int len) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_syslog), "D"(type), "S"(buf), "d"(len)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_chdir(const char *path) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_chdir), "D"(path)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_chmod(const char *path, int mode) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_chmod), "D"(path), "S"(mode)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_unlink(const char *path) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_unlink), "D"(path)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_rename(const char *oldpath, const char *newpath) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_rename), "D"(oldpath), "S"(newpath)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_getcwd(char *buf, size_t size) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_getcwd), "D"(buf), "S"(size)
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

static int starts_with(const char *s, const char *prefix) {
    while (*prefix) {
        if (*s != *prefix) return 0;
        s++; prefix++;
    }
    return 1;
}

static void print(const char *s) {
    sys_write(1, s, strlen_(s));
}

void printn(const char *s) {
    print(s);
    print("\n");
}

void print_int(int n) {
    char buf[16];
    int i = 15;
    buf[i--] = '\0';
    if (n == 0) {
        buf[i--] = '0';
    } else {
        int neg = n < 0;
        if (neg) n = -n;
        while (n > 0) {
            buf[i--] = '0' + (n % 10);
            n /= 10;
        }
        if (neg) buf[i--] = '-';
    }
    print(&buf[i + 1]);
}

// Background jobs
#define MAX_JOBS 16
static struct {
    int pid;
    char cmd[64];
    int active;
} jobs[MAX_JOBS];
static int job_count = 0;

static void jobs_reap(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            int status;
            int r = sys_wait4(jobs[i].pid, &status, 1, NULL); // WNOHANG = 1
            if (r > 0) {
                print("["); print_int(i + 1); print("] Done    ");
                printn(jobs[i].cmd);
                jobs[i].active = 0;
                job_count--;
            }
        }
    }
}

static void jobs_add(int pid, const char *cmd) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (!jobs[i].active) {
            jobs[i].pid = pid;
            jobs[i].active = 1;
            int j = 0;
            while (cmd[j] && j < 63) {
                jobs[i].cmd[j] = cmd[j];
                j++;
            }
            jobs[i].cmd[j] = '\0';
            job_count++;
            print("["); print_int(i + 1); print("] "); print_int(pid); printn("");
            return;
        }
    }
    printn("jobs: job table full");
}

static void jobs_list(void) {
    for (int i = 0; i < MAX_JOBS; i++) {
        if (jobs[i].active) {
            print("["); print_int(i + 1); print("] Running    ");
            printn(jobs[i].cmd);
        }
    }
}

// Environment variables
#define MAX_ENV 32
#define ENV_LEN 128
static char env_vars[MAX_ENV][ENV_LEN];
static int env_count = 0;

static void env_set(const char *key, const char *value) {
    int key_len = 0;
    while (key[key_len]) key_len++;
    // Update existing
    for (int i = 0; i < env_count; i++) {
        int match = 1;
        for (int j = 0; j < key_len; j++) {
            if (env_vars[i][j] != key[j]) { match = 0; break; }
        }
        if (match && env_vars[i][key_len] == '=') {
            int j = 0;
            while (key[j]) { env_vars[i][j] = key[j]; j++; }
            env_vars[i][j++] = '=';
            int k = 0;
            while (value[k] && j < ENV_LEN - 1) { env_vars[i][j++] = value[k++]; }
            env_vars[i][j] = '\0';
            return;
        }
    }
    // Add new
    if (env_count < MAX_ENV) {
        int j = 0;
        while (key[j]) { env_vars[env_count][j] = key[j]; j++; }
        env_vars[env_count][j++] = '=';
        int k = 0;
        while (value[k] && j < ENV_LEN - 1) { env_vars[env_count][j++] = value[k++]; }
        env_vars[env_count][j] = '\0';
        env_count++;
    }
}

static const char* env_get(const char *key) {
    int key_len = 0;
    while (key[key_len]) key_len++;
    for (int i = 0; i < env_count; i++) {
        int match = 1;
        for (int j = 0; j < key_len; j++) {
            if (env_vars[i][j] != key[j]) { match = 0; break; }
        }
        if (match && env_vars[i][key_len] == '=') {
            return &env_vars[i][key_len + 1];
        }
    }
    return NULL;
}

static void env_print(void) {
    for (int i = 0; i < env_count; i++) {
        printn(env_vars[i]);
    }
}

static void dispatch_command(void);

static int parse_ip(const char *s, uint32_t *out) {
    uint32_t ip = 0;
    int octets = 0;
    while (*s && octets < 4) {
        int n = 0;
        int digits = 0;
        while (*s >= '0' && *s <= '9' && digits < 3) {
            n = n * 10 + (*s - '0');
            s++; digits++;
        }
        if (digits == 0 || n > 255) return -1;
        ip = (ip << 8) | n;
        octets++;
        if (*s == '.') s++;
    }
    if (octets != 4 || *s != '\0') return -1;
    *out = ip;
    return 0;
}

static void ip_to_str(uint32_t ip, char *buf) {
    int i = 0;
    for (int octet = 0; octet < 4; octet++) {
        int n = (ip >> (24 - octet * 8)) & 0xFF;
        if (n >= 100) buf[i++] = '0' + n / 100;
        if (n >= 10) buf[i++] = '0' + (n / 10) % 10;
        buf[i++] = '0' + n % 10;
        if (octet < 3) buf[i++] = '.';
    }
    buf[i] = '\0';
}

static char linebuf[1024];
static int linepos = 0;

static int is_var_char(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '_';
}

static void expand_vars(void) {
    char out[1024];
    int o = 0;
    int i = 0;
    while (linebuf[i] && o < sizeof(out) - 1) {
        if (linebuf[i] == '$' && is_var_char(linebuf[i + 1])) {
            i++;
            char name[64];
            int n = 0;
            while (is_var_char(linebuf[i]) && n < 63) {
                name[n++] = linebuf[i++];
            }
            name[n] = '\0';
            const char *val = env_get(name);
            if (val) {
                int v = 0;
                while (val[v] && o < sizeof(out) - 1) {
                    out[o++] = val[v++];
                }
            }
        } else {
            out[o++] = linebuf[i++];
        }
    }
    out[o] = '\0';
    int j = 0;
    while (out[j] && j < sizeof(linebuf) - 1) {
        linebuf[j] = out[j];
        j++;
    }
    linebuf[j] = '\0';
}

// Aliases
#define MAX_ALIASES 16
#define ALIAS_NAME_LEN 32
#define ALIAS_VAL_LEN 128
static char alias_names[MAX_ALIASES][ALIAS_NAME_LEN];
static char alias_values[MAX_ALIASES][ALIAS_VAL_LEN];
static int alias_count = 0;

static void alias_set(const char *name, const char *value) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp_(alias_names[i], name) == 0) {
            int v = 0;
            while (value[v] && v < ALIAS_VAL_LEN - 1) {
                alias_values[i][v] = value[v];
                v++;
            }
            alias_values[i][v] = '\0';
            return;
        }
    }
    if (alias_count < MAX_ALIASES) {
        int n = 0;
        while (name[n] && n < ALIAS_NAME_LEN - 1) {
            alias_names[alias_count][n] = name[n];
            n++;
        }
        alias_names[alias_count][n] = '\0';
        int v = 0;
        while (value[v] && v < ALIAS_VAL_LEN - 1) {
            alias_values[alias_count][v] = value[v];
            v++;
        }
        alias_values[alias_count][v] = '\0';
        alias_count++;
    }
}

static void alias_unset(const char *name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp_(alias_names[i], name) == 0) {
            for (int j = i; j < alias_count - 1; j++) {
                int n = 0;
                while (alias_names[j + 1][n]) {
                    alias_names[j][n] = alias_names[j + 1][n];
                    n++;
                }
                alias_names[j][n] = '\0';
                n = 0;
                while (alias_values[j + 1][n]) {
                    alias_values[j][n] = alias_values[j + 1][n];
                    n++;
                }
                alias_values[j][n] = '\0';
            }
            alias_count--;
            return;
        }
    }
}

static const char* alias_get(const char *name) {
    for (int i = 0; i < alias_count; i++) {
        if (strcmp_(alias_names[i], name) == 0) {
            return alias_values[i];
        }
    }
    return NULL;
}

static void alias_print(void) {
    for (int i = 0; i < alias_count; i++) {
        print("alias "); print(alias_names[i]); print("='"); print(alias_values[i]); printn("'");
    }
}

static void expand_alias(void) {
    // Find first word
    char name[ALIAS_NAME_LEN];
    int i = 0;
    while (linebuf[i] == ' ') i++;
    int n = 0;
    while (linebuf[i] && linebuf[i] != ' ' && n < ALIAS_NAME_LEN - 1) {
        name[n++] = linebuf[i++];
    }
    name[n] = '\0';
    const char *val = alias_get(name);
    if (val) {
        char rest[1024];
        int r = 0;
        while (linebuf[i] && r < sizeof(rest) - 1) {
            rest[r++] = linebuf[i++];
        }
        rest[r] = '\0';
        int v = 0;
        while (val[v] && v < sizeof(linebuf) - 1) {
            linebuf[v] = val[v];
            v++;
        }
        int j = 0;
        while (rest[j] && v < sizeof(linebuf) - 1) {
            linebuf[v++] = rest[j++];
        }
        linebuf[v] = '\0';
    }
}

#define HISTORY_SIZE 32
#define HISTORY_LEN 256
static char history[HISTORY_SIZE][HISTORY_LEN];
static int history_count = 0;
static int history_next = 0;

static void history_add(const char *cmd) {
    if (cmd[0] == '\0') return;
    // Don't add if same as last command
    if (history_count > 0) {
        int last = (history_next - 1 + HISTORY_SIZE) % HISTORY_SIZE;
        int same = 1;
        for (int i = 0; i < HISTORY_LEN; i++) {
            if (history[last][i] != cmd[i]) { same = 0; break; }
            if (cmd[i] == '\0') break;
        }
        if (same) return;
    }
    int idx = history_next % HISTORY_SIZE;
    int i = 0;
    while (cmd[i] && i < HISTORY_LEN - 1) {
        history[idx][i] = cmd[i];
        i++;
    }
    history[idx][i] = '\0';
    history_next++;
    if (history_count < HISTORY_SIZE) history_count++;
}

static void history_print(void) {
    int start = history_next - history_count;
    for (int i = 0; i < history_count; i++) {
        int idx = (start + i) % HISTORY_SIZE;
        print_int(i + 1);
        print("  ");
        printn(history[idx]);
    }
}

static const char* history_get(int n) {
    if (n <= 0 || n > history_count) return NULL;
    int start = history_next - history_count;
    int idx = (start + n - 1) % HISTORY_SIZE;
    return history[idx];
}

static void read_line(void) {
    linepos = 0;
    while (linepos < sizeof(linebuf) - 1) {
        char c;
        int n = sys_read(0, &c, 1);
        if (n <= 0) break;
        if (c == '\n' || c == '\r') break;
        linebuf[linepos++] = c;
    }
    linebuf[linepos] = '\0';
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

static void print_env(void) {
    int fd = sys_open("/proc/self/environ", 0, 0);
    if (fd < 0) {
        printn("env: cannot open /proc/self/environ");
        return;
    }
    char buf[1024];
    int n = sys_read(fd, buf, sizeof(buf) - 1);
    sys_close(fd);
    if (n <= 0) {
        printn("env: no environment variables");
        return;
    }
    int i = 0;
    while (i < n) {
        // print until null byte
        int start = i;
        while (i < n && buf[i] != '\0') i++;
        if (i > start) {
            sys_write(1, buf + start, i - start);
            print("\n");
        }
        i++; // skip null byte
    }
    // Also print custom env vars
    env_print();
}

static void print_free(void) {
    int fd = sys_open("/proc/meminfo", 0, 0);
    if (fd < 0) {
        printn("free: cannot open /proc/meminfo");
        return;
    }
    char buf[1024];
    int n = sys_read(fd, buf, sizeof(buf) - 1);
    sys_close(fd);
    if (n <= 0) {
        printn("free: cannot read /proc/meminfo");
        return;
    }
    buf[n] = '\0';

    unsigned long total = 0, free = 0, available = 0;
    char *p = buf;
    while (*p) {
        if (starts_with(p, "MemTotal:")) {
            p += 9;
            while (*p == ' ') p++;
            while (*p >= '0' && *p <= '9') {
                total = total * 10 + (*p - '0');
                p++;
            }
        } else if (starts_with(p, "MemFree:")) {
            p += 8;
            while (*p == ' ') p++;
            while (*p >= '0' && *p <= '9') {
                free = free * 10 + (*p - '0');
                p++;
            }
        } else if (starts_with(p, "MemAvailable:")) {
            p += 13;
            while (*p == ' ') p++;
            while (*p >= '0' && *p <= '9') {
                available = available * 10 + (*p - '0');
                p++;
            }
        }
        while (*p && *p != '\n') p++;
        if (*p == '\n') p++;
    }

    print("MemTotal: "); print_int((int)(total / 1024)); printn(" MB");
    print("MemFree:  "); print_int((int)(free / 1024)); printn(" MB");
    print("MemAvail: "); print_int((int)(available / 1024)); printn(" MB");
}

static void print_uptime(void) {
    int fd = sys_open("/proc/uptime", 0, 0);
    if (fd < 0) {
        printn("uptime: cannot open /proc/uptime");
        return;
    }
    char buf[64];
    int n = sys_read(fd, buf, sizeof(buf) - 1);
    sys_close(fd);
    if (n <= 0) {
        printn("uptime: cannot read /proc/uptime");
        return;
    }
    buf[n] = '\0';

    // Parse seconds (integer part before decimal)
    unsigned long secs = 0;
    char *p = buf;
    while (*p >= '0' && *p <= '9') {
        secs = secs * 10 + (*p - '0');
        p++;
    }

    int days = secs / 86400;
    int hours = (secs % 86400) / 3600;
    int mins = (secs % 3600) / 60;

    if (days > 0) { print_int(days); print("d "); }
    if (hours > 0) { print_int(hours); print("h "); }
    print_int(mins); printn("m");
}

static void do_clear(void) {
    // ANSI escape: clear screen, move cursor to top-left
    print("\x1b[2J\x1b[H");
}

static void grep_file(const char *pattern, const char *path) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("grep: cannot open file");
        return;
    }
    char buf[1024];
    int n;
    char line[256];
    int line_pos = 0;
    int pat_len = 0;
    while (pattern[pat_len]) pat_len++;

    while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
        for (int i = 0; i < n; i++) {
            if (buf[i] == '\n') {
                line[line_pos] = '\0';
                // Search for pattern in line
                int found = 0;
                for (int j = 0; line[j] && !found; j++) {
                    int match = 1;
                    for (int k = 0; k < pat_len; k++) {
                        if (line[j + k] != pattern[k]) { match = 0; break; }
                    }
                    if (match) found = 1;
                }
                if (found) {
                    printn(line);
                }
                line_pos = 0;
            } else if (line_pos < sizeof(line) - 1) {
                line[line_pos++] = buf[i];
            }
        }
    }
    sys_close(fd);
}

static void head_file(const char *path, int nlines) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("head: cannot open file");
        return;
    }
    char buf[256];
    int n;
    int lines = 0;
    while ((n = sys_read(fd, buf, sizeof(buf))) > 0 && lines < nlines) {
        for (int i = 0; i < n && lines < nlines; i++) {
            sys_write(1, &buf[i], 1);
            if (buf[i] == '\n') lines++;
        }
    }
    sys_close(fd);
}

static void tail_file(const char *path, int nlines) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("tail: cannot open file");
        return;
    }
    // Read entire file into a buffer
    char buf[4096];
    int total = 0;
    int n;
    while ((n = sys_read(fd, buf + total, sizeof(buf) - total - 1)) > 0 && total < sizeof(buf) - 1) {
        total += n;
    }
    sys_close(fd);
    buf[total] = '\0';

    if (nlines <= 0) nlines = 10;
    // Count newlines from the end
    int lines = 0;
    int start = total;
    for (int i = total - 1; i >= 0; i--) {
        if (buf[i] == '\n') {
            lines++;
            if (lines == nlines) {
                start = i + 1;
                break;
            }
        }
    }
    // If we didn't find enough lines, start from beginning
    if (lines < nlines) start = 0;
    sys_write(1, buf + start, total - start);
}

static void wc_file(const char *path) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("wc: cannot open file");
        return;
    }
    char buf[256];
    int n;
    int lines = 0, words = 0, bytes = 0;
    int in_word = 0;
    while ((n = sys_read(fd, buf, sizeof(buf))) > 0) {
        bytes += n;
        for (int i = 0; i < n; i++) {
            if (buf[i] == '\n') lines++;
            if (buf[i] == ' ' || buf[i] == '\n' || buf[i] == '\t') {
                in_word = 0;
            } else if (!in_word) {
                in_word = 1;
                words++;
            }
        }
    }
    sys_close(fd);
    print_int(lines); print(" ");
    print_int(words); print(" ");
    print_int(bytes); print(" ");
    printn(path);
}

static void print_uname(void) {
    struct {
        char sysname[65];
        char nodename[65];
        char release[65];
        char version[65];
        char machine[65];
        char domainname[65];
    } buf;
    if (sys_uname(&buf) < 0) {
        printn("uname: failed");
        return;
    }
    print(buf.sysname); print(" ");
    print(buf.nodename); print(" ");
    print(buf.release); print(" ");
    print(buf.version); print(" ");
    printn(buf.machine);
}

static void print_date(void) {
    struct { long sec; long usec; } tv;
    if (sys_gettimeofday(&tv, NULL) < 0) {
        printn("date: failed");
        return;
    }
    // Simple conversion: seconds since 1970-01-01
    // This is a rough approximation, good enough for basic usage
    long t = tv.sec;
    long days = t / 86400;
    int year = 1970;
    // Very rough year calculation (not accounting for leap years precisely)
    while (days >= 365) {
        int leap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
        if (days >= 365 + leap) {
            days -= 365 + leap;
            year++;
        } else {
            break;
        }
    }
    int month_days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0)) month_days[1] = 29;
    int month = 0;
    while (month < 12 && days >= month_days[month]) {
        days -= month_days[month];
        month++;
    }
    int day = (int)days + 1;
    int hour = (t % 86400) / 3600;
    int min = (t % 3600) / 60;
    int sec = t % 60;

    print_int(year); print("-");
    if (month + 1 < 10) print("0"); print_int(month + 1); print("-");
    if (day < 10) print("0"); print_int(day); print(" ");
    if (hour < 10) print("0"); print_int(hour); print(":");
    if (min < 10) print("0"); print_int(min); print(":");
    if (sec < 10) print("0"); print_int(sec); printn(" UTC");
}

static void print_df(void) {
    struct {
        long type;
        long bsize;
        long blocks;
        long bfree;
        long bavail;
        long files;
        long ffree;
        long fsid;
        long namelen;
        long frsize;
        long flags;
        long spare[4];
    } stat;
    if (sys_statfs("/", &stat) < 0) {
        printn("df: failed");
        return;
    }
    long total = stat.blocks * stat.bsize / 1024;
    long free = stat.bfree * stat.bsize / 1024;
    long used = total - free;
    print("Filesystem: /\nTotal: "); print_int((int)(total / 1024)); printn(" MB");
    print("Used:  "); print_int((int)(used / 1024)); printn(" MB");
    print("Free:  "); print_int((int)(free / 1024)); printn(" MB");
}

static void print_dmesg(void) {
    char buf[8192];
    int n = sys_syslog(3, buf, sizeof(buf) - 1);
    if (n < 0) {
        printn("dmesg: failed");
        return;
    }
    if (n > 0) {
        buf[n] = '\0';
        sys_write(1, buf, n);
    }
}

static void print_whoami(void) {
    int uid = sys_getuid();
    if (uid == 0) {
        printn("root");
    } else {
        print_int(uid);
        printn("");
    }
}

static void print_id(void) {
    int uid = sys_getuid();
    int gid = sys_getgid();
    print("uid="); print_int(uid); print(" gid="); print_int(gid); printn("");
}

static void print_hostname(void) {
    int fd = sys_open("/proc/sys/kernel/hostname", 0, 0);
    if (fd < 0) {
        printn("hostname: failed");
        return;
    }
    char buf[64];
    int n = sys_read(fd, buf, sizeof(buf) - 1);
    sys_close(fd);
    if (n > 0) {
        // Strip trailing newline
        if (buf[n-1] == '\n') n--;
        buf[n] = '\0';
        printn(buf);
    }
}

static void source_file(const char *path) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("source: cannot open file");
        return;
    }
    char buf[4096];
    int n = sys_read(fd, buf, sizeof(buf) - 1);
    sys_close(fd);
    if (n <= 0) return;
    buf[n] = '\0';

    char cmdbuf[256];
    int cmdpos = 0;
    for (int i = 0; i <= n; i++) {
        if (buf[i] == '\n' || buf[i] == '\0') {
            if (cmdpos > 0) {
                cmdbuf[cmdpos] = '\0';
                // Copy to linebuf and execute
                int j = 0;
                while (cmdbuf[j] && j < sizeof(linebuf) - 1) {
                    linebuf[j] = cmdbuf[j];
                    j++;
                }
                linebuf[j] = '\0';
                linepos = j;
                // Skip comments and empty lines
                if (linebuf[0] != '#' && linebuf[0] != '\0') {
                    history_add(linebuf);
                    dispatch_command();
                }
                cmdpos = 0;
            }
        } else if (cmdpos < sizeof(cmdbuf) - 1) {
            cmdbuf[cmdpos++] = buf[i];
        }
    }
}

static void do_chmod(const char *path, int mode) {
    if (sys_chmod(path, mode) < 0) {
        printn("chmod: failed");
    }
}

static int parse_octal(const char *s) {
    int n = 0;
    while (*s >= '0' && *s <= '7') {
        n = n * 8 + (*s - '0');
        s++;
    }
    return n;
}

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

static void find_file(const char *dir, const char *name) {
    int fd = sys_open(dir, 0, 0);
    if (fd < 0) {
        printn("find: cannot open directory");
        return;
    }
    char buf[1024];
    int n;
    while ((n = sys_getdents64(fd, buf, sizeof(buf))) > 0) {
        int pos = 0;
        while (pos < n) {
            struct linux_dirent64 *de = (struct linux_dirent64 *)(buf + pos);
            if (de->d_name[0] != '.') {
                // Check if name matches
                int match = 1;
                int i = 0;
                while (name[i]) {
                    if (de->d_name[i] != name[i]) { match = 0; break; }
                    i++;
                }
                if (match && de->d_name[i] == '\0') {
                    print(dir); print("/"); printn(de->d_name);
                }
                // Recurse into subdirectories
                if (de->d_type == 4) {
                    char subdir[256];
                    int j = 0;
                    while (dir[j]) { subdir[j] = dir[j]; j++; }
                    subdir[j++] = '/';
                    int k = 0;
                    while (de->d_name[k]) { subdir[j++] = de->d_name[k++]; }
                    subdir[j] = '\0';
                    find_file(subdir, name);
                }
            }
            pos += de->d_reclen;
        }
    }
    sys_close(fd);
}

static int is_numeric(const char *s) {
    while (*s) {
        if (*s < '0' || *s > '9') return 0;
        s++;
    }
    return 1;
}

static void ps_list(void) {
    printn("  PID   PPID  CMD");
    int fd = sys_open("/proc", 0, 0);
    if (fd < 0) {
        printn("ps: cannot open /proc");
        return;
    }
    char buf[1024];
    int n;
    while ((n = sys_getdents64(fd, buf, sizeof(buf))) > 0) {
        int pos = 0;
        while (pos < n) {
            struct linux_dirent64 *de = (struct linux_dirent64 *)(buf + pos);
            if (is_numeric(de->d_name)) {
                char stat_path[64];
                int i = 0;
                const char *prefix = "/proc/";
                while (prefix[i]) { stat_path[i] = prefix[i]; i++; }
                int j = 0;
                while (de->d_name[j]) { stat_path[i++] = de->d_name[j++]; }
                const char *suffix = "/stat";
                j = 0;
                while (suffix[j]) { stat_path[i++] = suffix[j++]; }
                stat_path[i] = '\0';

                int sfd = sys_open(stat_path, 0, 0);
                if (sfd >= 0) {
                    char sbuf[256];
                    int sr = sys_read(sfd, sbuf, sizeof(sbuf) - 1);
                    if (sr > 0) {
                        sbuf[sr] = '\0';
                        // Parse: pid (comm) state ppid ...
                        // Find first space -> end of pid
                        int pid_end = 0;
                        while (sbuf[pid_end] && sbuf[pid_end] != ' ') pid_end++;
                        sbuf[pid_end] = '\0';

                        // Find '(' and ')'
                        char *comm_start = 0;
                        char *comm_end = 0;
                        for (int k = pid_end + 1; sbuf[k]; k++) {
                            if (sbuf[k] == '(') comm_start = &sbuf[k + 1];
                            if (sbuf[k] == ')') { comm_end = &sbuf[k]; break; }
                        }
                        if (comm_start && comm_end) *comm_end = '\0';

                        // Find ppid after ')'
                        char *ppid_ptr = comm_end ? comm_end + 2 : 0;
                        int ppid = 0;
                        if (ppid_ptr) {
                            while (*ppid_ptr == ' ') ppid_ptr++;
                            // skip state char
                            if (*ppid_ptr) ppid_ptr++;
                            while (*ppid_ptr == ' ') ppid_ptr++;
                            while (*ppid_ptr >= '0' && *ppid_ptr <= '9') {
                                ppid = ppid * 10 + (*ppid_ptr - '0');
                                ppid_ptr++;
                            }
                        }

                        // Pad and print
                        print(" ");
                        print(sbuf); // pid
                        int pid_len = 0;
                        while (sbuf[pid_len]) pid_len++;
                        for (int p = 0; p < 6 - pid_len; p++) print(" ");

                        char ppid_str[16];
                        int ppid_len = 0;
                        if (ppid == 0) {
                            ppid_str[ppid_len++] = '0';
                        } else {
                            int tmp = ppid;
                            char rev[16];
                            int rev_len = 0;
                            while (tmp > 0) {
                                rev[rev_len++] = '0' + (tmp % 10);
                                tmp /= 10;
                            }
                            for (int r = rev_len - 1; r >= 0; r--) {
                                ppid_str[ppid_len++] = rev[r];
                            }
                        }
                        ppid_str[ppid_len] = '\0';
                        print(ppid_str);
                        for (int p = 0; p < 6 - ppid_len; p++) print(" ");

                        if (comm_start && comm_end) {
                            printn(comm_start);
                        } else {
                            printn("?");
                        }
                    }
                    sys_close(sfd);
                }
            }
            pos += de->d_reclen;
        }
    }
    sys_close(fd);
}

static void ls_dir(const char *path) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("ls: cannot open directory");
        return;
    }
    char buf[1024];
    int n;
    while ((n = sys_getdents64(fd, buf, sizeof(buf))) > 0) {
        int pos = 0;
        while (pos < n) {
            struct linux_dirent64 *de = (struct linux_dirent64 *)(buf + pos);
            if (de->d_name[0] != '.') {
                const char *type = (de->d_type == 4) ? "d" :
                                   (de->d_type == 10) ? "l" : "-";
                print(type);
                print(" ");
                printn(de->d_name);
            }
            pos += de->d_reclen;
        }
    }
    sys_close(fd);
}

static void do_reboot(void) {
    sys_reboot(0xfee1dead, 672274793, 0x01234567, NULL);
}

static void do_poweroff(void) {
    sys_reboot(0xfee1dead, 672274793, 0x4321fedc, NULL);
}

static uint32_t dns_resolve(const char *host);

static uint16_t icmp_checksum(const void *data, int len) {
    const uint16_t *p = data;
    uint32_t sum = 0;
    while (len > 1) {
        sum += *p++;
        len -= 2;
    }
    if (len == 1) sum += *(const unsigned char *)p;
    while (sum >> 16) sum = (sum & 0xFFFF) + (sum >> 16);
    return ~sum;
}

static void do_ping(const char *host) {
    uint32_t ip = dns_resolve(host);
    if (ip == 0) {
        printn("ping: cannot resolve host");
        return;
    }

    int fd = sys_socket(2, 3, 1); // AF_INET, SOCK_RAW, IPPROTO_ICMP
    if (fd < 0) {
        printn("ping: socket failed (need root)");
        return;
    }

    struct {
        uint16_t family;
        uint16_t port;
        uint32_t addr;
        char pad[8];
    } sa = {2, 0, ip, {0}};

    // Build ICMP echo request
    struct {
        unsigned char type;
        unsigned char code;
        uint16_t checksum;
        uint16_t id;
        uint16_t seq;
        uint64_t data;
    } req = {8, 0, 0, 1, 0, 0x1234567890abcdefULL};
    req.checksum = icmp_checksum(&req, sizeof(req));

    int sent = sys_sendto(fd, &req, sizeof(req), 0, &sa, sizeof(sa));
    if (sent < 0) {
        printn("ping: sendto failed");
        sys_close(fd);
        return;
    }

    // Receive reply with timeout (using non-blocking + poll not available, just try once)
    char resp[256];
    int from_len = sizeof(sa);
    int n = sys_recvfrom(fd, resp, sizeof(resp), 0, &sa, &from_len);
    sys_close(fd);

    if (n < 0) {
        printn("ping: no reply");
        return;
    }

    // Parse IP header (minimum 20 bytes) + ICMP
    if (n < 28) {
        printn("ping: reply too short");
        return;
    }

    // IP header is first, ICMP starts at offset 20
    unsigned char *icmp = (unsigned char *)resp + 20;
    if (icmp[0] == 0 && icmp[1] == 0) {
        print("ping: reply from ");
        char ip_str[16];
        ip_to_str(ip, ip_str);
        print(ip_str);
        printn(" ok");
    } else {
        printn("ping: unexpected reply type");
    }
}

static uint32_t dns_resolve(const char *host) {
    // Simple hardcoded DNS for common hosts
    if (strcmp_(host, "10.0.2.2") == 0) return 0x0202000a;
    if (strcmp_(host, "127.0.0.1") == 0) return 0x0100007f;
    if (strcmp_(host, "localhost") == 0) return 0x0100007f;
    // Try to parse as dotted decimal
    uint32_t ip;
    if (parse_ip(host, &ip) == 0) return ip;
    return 0;
}

static void do_http_get(const char *host, const char *path) {
    uint32_t ip = dns_resolve(host);
    if (ip == 0) {
        printn("http: cannot resolve host (DNS not implemented, use IP)");
        return;
    }

    int fd = sys_socket(2, 1, 0); // AF_INET, SOCK_STREAM, 0
    if (fd < 0) {
        printn("http: socket failed");
        return;
    }

    struct {
        uint16_t family;
        uint16_t port;
        uint32_t addr;
        char pad[8];
    } sa = {2, 0x5000, ip, {0}}; // port 80

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

static void do_wget(const char *host, const char *path, const char *outfile) {
    uint32_t ip = dns_resolve(host);
    if (ip == 0) {
        printn("wget: cannot resolve host");
        return;
    }

    int fd = sys_socket(2, 1, 0); // AF_INET, SOCK_STREAM, 0
    if (fd < 0) {
        printn("wget: socket failed");
        return;
    }

    struct {
        uint16_t family;
        uint16_t port;
        uint32_t addr;
        char pad[8];
    } sa = {2, 0x5000, ip, {0}}; // port 80

    if (sys_connect(fd, &sa, sizeof(sa)) < 0) {
        printn("wget: connect failed");
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

    // Read full response into a buffer
    char resp_buf[8192];
    int total = 0;
    char buf[256];
    int n;
    while ((n = sys_read(fd, buf, sizeof(buf))) > 0 && total + n < sizeof(resp_buf) - 1) {
        for (int i = 0; i < n; i++) resp_buf[total++] = buf[i];
    }
    resp_buf[total] = '\0';
    sys_close(fd);

    // Find body after \r\n\r\n
    int body_start = -1;
    for (int i = 0; i < total - 3; i++) {
        if (resp_buf[i] == '\r' && resp_buf[i+1] == '\n' && resp_buf[i+2] == '\r' && resp_buf[i+3] == '\n') {
            body_start = i + 4;
            break;
        }
    }
    if (body_start < 0) {
        printn("wget: no body in response");
        return;
    }

    int outfd = sys_open(outfile, 0x241, 0644);
    if (outfd < 0) {
        printn("wget: cannot create output file");
        return;
    }
    sys_write(outfd, resp_buf + body_start, total - body_start);
    sys_close(outfd);
    print("wget: saved ");
    print_int(total - body_start);
    print(" bytes to ");
    printn(outfile);
}

static int do_http_post_body(const char *host, const char *path, const char *body, char *resp, int resp_max) {
    uint32_t ip = dns_resolve(host);
    if (ip == 0) return -1;

    int fd = sys_socket(2, 1, 0); // AF_INET, SOCK_STREAM, 0
    if (fd < 0) return -1;

    struct {
        uint16_t family;
        uint16_t port;
        uint32_t addr;
        char pad[8];
    } sa = {2, 0x5000, ip, {0}}; // port 80

    if (sys_connect(fd, &sa, sizeof(sa)) < 0) {
        sys_close(fd);
        return -1;
    }

    char req[2048];
    int len = 0;
    const char *p = "POST ";
    while (*p) req[len++] = *p++;
    p = path;
    while (*p) req[len++] = *p++;
    p = " HTTP/1.1\r\nHost: ";
    while (*p) req[len++] = *p++;
    p = host;
    while (*p) req[len++] = *p++;
    p = "\r\nContent-Type: application/json\r\nContent-Length: ";
    while (*p) req[len++] = *p++;

    int body_len = 0;
    while (body[body_len]) body_len++;
    char len_str[16];
    int len_digits = 0;
    int tmp = body_len;
    if (tmp == 0) len_str[len_digits++] = '0';
    while (tmp > 0) {
        len_str[len_digits++] = '0' + (tmp % 10);
        tmp /= 10;
    }
    for (int i = len_digits - 1; i >= 0; i--) {
        req[len++] = len_str[i];
    }

    p = "\r\nConnection: close\r\n\r\n";
    while (*p) req[len++] = *p++;

    sys_write(fd, req, len);
    sys_write(fd, body, body_len);

    // Read full response into buffer
    int total = 0;
    char buf[256];
    int n;
    while ((n = sys_read(fd, buf, sizeof(buf))) > 0 && total + n < resp_max - 1) {
        for (int i = 0; i < n; i++) resp[total++] = buf[i];
    }
    resp[total] = '\0';
    sys_close(fd);

    // Find body (after \r\n\r\n)
    for (int i = 0; i < total - 3; i++) {
        if (resp[i] == '\r' && resp[i+1] == '\n' && resp[i+2] == '\r' && resp[i+3] == '\n') {
            // Shift body to start of resp
            int body_start = i + 4;
            int body_len = total - body_start;
            for (int j = 0; j <= body_len; j++) resp[j] = resp[body_start + j];
            return body_len;
        }
    }
    return total;
}

// Minimal JSON string extractor. Finds key and copies string value into out.
// Returns 1 on success, 0 on failure. Handles escaped quotes.
static int json_extract_string(const char *json, const char *key, char *out, int out_len) {
    int key_len = 0;
    while (key[key_len]) key_len++;

    const char *p = json;
    while (*p) {
        if (*p == '"') {
            p++;
            // Check if this is our key
            int match = 1;
            for (int i = 0; i < key_len; i++) {
                if (p[i] != key[i]) { match = 0; break; }
            }
            if (match && p[key_len] == '"') {
                // Found key, skip to value
                p += key_len + 1;
                while (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r' || *p == ':') p++;
                if (*p != '"') return 0;
                p++;
                int i = 0;
                while (*p && *p != '"' && i < out_len - 1) {
                    if (*p == '\\' && *(p+1)) {
                        p++;
                        if (*p == 'n') out[i++] = '\n';
                        else if (*p == 't') out[i++] = '\t';
                        else if (*p == 'r') out[i++] = '\r';
                        else out[i++] = *p;
                        p++;
                    } else {
                        out[i++] = *p++;
                    }
                }
                out[i] = '\0';
                return 1;
            }
            // Skip this string
            while (*p && *p != '"') {
                if (*p == '\\' && *(p+1)) p += 2;
                else p++;
            }
            if (*p == '"') p++;
        } else {
            p++;
        }
    }
    return 0;
}

static char api_host[64] = "10.0.2.2";
static char api_path[128] = "/v1/chat/completions";

static void run_command(const char *cmd);
static void write_file(const char *path, const char *content);

#define MAX_AGENT_HISTORY 8
#define AGENT_MSG_LEN 512
static char agent_roles[MAX_AGENT_HISTORY][16];
static char agent_msgs[MAX_AGENT_HISTORY][AGENT_MSG_LEN];
static int agent_history_count = 0;
static int agent_history_next = 0;

static void agent_history_add(const char *role, const char *content) {
    int idx = agent_history_next % MAX_AGENT_HISTORY;
    int r = 0;
    while (role[r] && r < 15) {
        agent_roles[idx][r] = role[r];
        r++;
    }
    agent_roles[idx][r] = '\0';
    int c = 0;
    while (content[c] && c < AGENT_MSG_LEN - 1) {
        agent_msgs[idx][c] = content[c];
        c++;
    }
    agent_msgs[idx][c] = '\0';
    agent_history_next++;
    if (agent_history_count < MAX_AGENT_HISTORY) agent_history_count++;
}

static void agent_history_clear(void) {
    agent_history_count = 0;
    agent_history_next = 0;
}

static void agent_history_print(void) {
    int start = agent_history_next - agent_history_count;
    for (int i = 0; i < agent_history_count; i++) {
        int idx = (start + i) % MAX_AGENT_HISTORY;
        print("["); print(agent_roles[idx]); print("] ");
        printn(agent_msgs[idx]);
    }
}

static void append_json_string(char *buf, int *pos, int max, const char *s) {
    for (int i = 0; s[i] && *pos < max - 1; i++) {
        if (s[i] == '"' || s[i] == '\\') buf[(*pos)++] = '\\';
        buf[(*pos)++] = s[i];
    }
}

static void ask_ai(const char *prompt) {
    char body[8192];
    int bl = 0;
    const char *p = "{\"model\":\"gpt-3.5-turbo\",\"messages\":[{\"role\":\"system\",\"content\":\"You are an AI agent running inside Nymoris OS. Available tools: run <binary>, exec <shell_command>, read <file>, write <file> <content>, http <host> [path]. Use 'exec' for built-in shell commands (ls, cat, ps, etc.). Respond with the tool call only, no explanation.\"}";
    while (*p) body[bl++] = *p++;

    // Add conversation history
    int start = agent_history_next - agent_history_count;
    for (int i = 0; i < agent_history_count; i++) {
        int idx = (start + i) % MAX_AGENT_HISTORY;
        p = ", {\"role\":\"";
        while (*p) body[bl++] = *p++;
        int r = 0;
        while (agent_roles[idx][r]) body[bl++] = agent_roles[idx][r++];
        p = "\",\"content\":\"";
        while (*p) body[bl++] = *p++;
        append_json_string(body, &bl, sizeof(body) - 1, agent_msgs[idx]);
        p = "\"}";
        while (*p) body[bl++] = *p++;
    }

    // Add current user prompt
    p = ", {\"role\":\"user\",\"content\":\"";
    while (*p) body[bl++] = *p++;
    append_json_string(body, &bl, sizeof(body) - 1, prompt);
    p = "\"}]}";
    while (*p) body[bl++] = *p++;
    body[bl] = '\0';

    char resp[4096];
    int resp_len = do_http_post_body(api_host, api_path, body, resp, sizeof(resp));
    if (resp_len < 0) {
        printn("[AGENT] API request failed");
        return;
    }

    char content[2048];
    if (json_extract_string(resp, "content", content, sizeof(content))) {
        printn("[AGENT] AI response:");
        printn(content);

        // Save to conversation history
        agent_history_add("user", prompt);
        agent_history_add("assistant", content);

        // Auto-execute if it looks like a tool call
        if (starts_with(content, "run ")) {
            printn("[AGENT] Executing: run");
            run_command(content + 4);
        } else if (starts_with(content, "exec ")) {
            printn("[AGENT] Executing: exec");
            char saved[1024];
            int i = 0;
            while (linebuf[i] && i < sizeof(saved) - 1) {
                saved[i] = linebuf[i];
                i++;
            }
            saved[i] = '\0';
            char *cmd = content + 5;
            i = 0;
            while (cmd[i] && i < sizeof(linebuf) - 1) {
                linebuf[i] = cmd[i];
                i++;
            }
            linebuf[i] = '\0';
            linepos = i;
            dispatch_command();
            i = 0;
            while (saved[i]) {
                linebuf[i] = saved[i];
                i++;
            }
            linebuf[i] = '\0';
            linepos = i;
        } else if (starts_with(content, "read ")) {
            printn("[AGENT] Executing: read");
            cat_file(content + 5);
        } else if (starts_with(content, "write ")) {
            printn("[AGENT] Executing: write");
            char *wp = content + 6;
            char *wpath = wp;
            char *wcontent = NULL;
            for (int i = 0; wp[i]; i++) {
                if (wp[i] == ' ') {
                    wp[i] = '\0';
                    wcontent = &wp[i + 1];
                    break;
                }
            }
            if (wcontent) write_file(wpath, wcontent);
        }
    } else {
        printn("[AGENT] Could not parse AI response");
        printn(resp);
    }
}

static void do_sleep(int secs) {
    struct { uint64_t sec; uint64_t nsec; } req = { secs, 0 };
    sys_nanosleep(&req, NULL);
}

static int parse_argv(const char *cmd, char **argv, int max_argc) {
    static char argbuf[256];
    int argc = 0;
    int i = 0;
    int bufpos = 0;
    while (cmd[i] && argc < max_argc) {
        while (cmd[i] == ' ') i++;
        if (!cmd[i]) break;
        argv[argc++] = argbuf + bufpos;
        while (cmd[i] && cmd[i] != ' ' && bufpos < sizeof(argbuf) - 1) {
            argbuf[bufpos++] = cmd[i++];
        }
        if (bufpos < sizeof(argbuf)) argbuf[bufpos++] = '\0';
    }
    return argc;
}

static void run_command(const char *cmd) {
    int pid = sys_fork();
    if (pid == 0) {
        // Child
        char *argv_sh[4];
        argv_sh[0] = "/bin/sh";
        argv_sh[1] = "-c";
        argv_sh[2] = (char *)cmd;
        argv_sh[3] = NULL;
        char *envp[1] = {NULL};
        sys_execve("/bin/sh", argv_sh, envp);
        // Fallback: try to exec directly
        char *argv[16];
        int argc = parse_argv(cmd, argv, 15);
        if (argc > 0) {
            argv[argc] = NULL;
            sys_execve(argv[0], argv, envp);
            // Try /bin/ prefix
            char binpath[128] = "/bin/";
            char *bp = binpath + 5;
            char *ap = argv[0];
            while (*ap && bp < binpath + sizeof(binpath) - 1) {
                *bp++ = *ap++;
            }
            *bp = '\0';
            sys_execve(binpath, argv, envp);
        }
        printn("run: exec failed");
        sys_exit(1);
    } else if (pid > 0) {
        int status;
        sys_wait4(pid, &status, 0, NULL);
    } else {
        printn("run: fork failed");
    }
}

static void write_file(const char *path, const char *content) {
    int fd = sys_open(path, 0x241, 0644); // O_WRONLY|O_CREAT|O_TRUNC
    if (fd < 0) {
        printn("write: cannot create file");
        return;
    }
    sys_write(fd, content, strlen_(content));
    sys_close(fd);
    print("Written to ");
    printn(path);
}

static void agent_auto_loop(int max_iter, int interval_secs) {
    if (max_iter <= 0) max_iter = 10;
    if (interval_secs <= 0) interval_secs = 5;
    printn("\n[AGENT] Autonomous mode started.");
    print("[AGENT] Max iterations: "); print_int(max_iter); printn("");
    print("[AGENT] Interval: "); print_int(interval_secs); printn(" seconds");
    printn("[AGENT] Press Ctrl+C or type 'interrupt' to stop (not implemented).");

    for (int iter = 0; iter < max_iter; iter++) {
        print("\n[AGENT] === Iteration "); print_int(iter + 1); print(" / "); print_int(max_iter); printn(" ===");

        char prompt[512];
        int p = 0;
        const char *s = "You are running autonomously on Nymoris OS. Decide what to do next. ";
        while (*s) prompt[p++] = *s++;
        s = "Available: exec <shell_cmd>, run <binary>, read <file>, write <file> <content>, http <host> [path]. ";
        while (*s) prompt[p++] = *s++;
        s = "Be concise. Execute one tool per response.";
        while (*s) prompt[p++] = *s++;
        prompt[p] = '\0';

        ask_ai(prompt);

        if (iter < max_iter - 1) {
            do_sleep(interval_secs);
        }
    }

    printn("\n[AGENT] Autonomous mode completed.");
}

static void agent_loop(void) {
    printn("\n[AGENT] AI Agent loop started.");
    printn("[AGENT] Commands: ask <prompt>, auto [n] [s], history, reset, exec <cmd>, run <cmd>, read <file>, write <file> <data>, http <host> [path], sleep <secs>, done");

    while (1) {
        print("[AGENT] > ");
        read_line();

        if (linepos == 0) continue;

        // Parse action and argument
        char *action = linebuf;
        char *arg = NULL;
        for (int i = 0; i < linepos; i++) {
            if (linebuf[i] == ' ') {
                linebuf[i] = '\0';
                arg = &linebuf[i + 1];
                break;
            }
        }

        if (strcmp_(action, "done") == 0 || strcmp_(action, "quit") == 0) {
            printn("[AGENT] Exiting agent loop.");
            break;
        } else if (strcmp_(action, "ask") == 0) {
            if (arg) {
                ask_ai(arg);
            } else {
                printn("[AGENT] Usage: ask <prompt>");
            }
        } else if (strcmp_(action, "auto") == 0) {
            int max_iter = 10;
            int interval = 5;
            if (arg) {
                char *p = arg;
                max_iter = 0;
                while (*p >= '0' && *p <= '9') {
                    max_iter = max_iter * 10 + (*p - '0');
                    p++;
                }
                while (*p == ' ') p++;
                interval = 0;
                while (*p >= '0' && *p <= '9') {
                    interval = interval * 10 + (*p - '0');
                    p++;
                }
            }
            agent_auto_loop(max_iter, interval);
        } else if (strcmp_(action, "history") == 0) {
            agent_history_print();
        } else if (strcmp_(action, "reset") == 0) {
            agent_history_clear();
            printn("[AGENT] Conversation history cleared.");
        } else if (strcmp_(action, "run") == 0) {
            if (arg) {
                run_command(arg);
            } else {
                printn("[AGENT] Usage: run <command>");
            }
        } else if (strcmp_(action, "exec") == 0) {
            if (arg) {
                char saved[1024];
                int i = 0;
                while (linebuf[i] && i < sizeof(saved) - 1) {
                    saved[i] = linebuf[i];
                    i++;
                }
                saved[i] = '\0';
                i = 0;
                while (arg[i] && i < sizeof(linebuf) - 1) {
                    linebuf[i] = arg[i];
                    i++;
                }
                linebuf[i] = '\0';
                linepos = i;
                dispatch_command();
                i = 0;
                while (saved[i]) {
                    linebuf[i] = saved[i];
                    i++;
                }
                linebuf[i] = '\0';
                linepos = i;
            } else {
                printn("[AGENT] Usage: exec <shell_command>");
            }
        } else if (strcmp_(action, "read") == 0) {
            if (arg) {
                cat_file(arg);
            } else {
                printn("[AGENT] Usage: read <file>");
            }
        } else if (strcmp_(action, "write") == 0) {
            if (arg) {
                // Find space separating path and content
                char *path = arg;
                char *content = NULL;
                for (int i = 0; arg[i]; i++) {
                    if (arg[i] == ' ') {
                        arg[i] = '\0';
                        content = &arg[i + 1];
                        break;
                    }
                }
                if (content) {
                    write_file(path, content);
                } else {
                    printn("[AGENT] Usage: write <file> <content>");
                }
            } else {
                printn("[AGENT] Usage: write <file> <content>");
            }
        } else if (strcmp_(action, "http") == 0) {
            if (arg) {
                char *host = arg;
                char *path = NULL;
                for (int i = 0; arg[i]; i++) {
                    if (arg[i] == ' ') {
                        arg[i] = '\0';
                        path = &arg[i + 1];
                        break;
                    }
                }
                do_http_get(host, path ? path : "/");
            } else {
                printn("[AGENT] Usage: http <host> [path]");
            }
        } else if (strcmp_(action, "sleep") == 0) {
            if (arg) {
                int secs = 0;
                for (int i = 0; arg[i] >= '0' && arg[i] <= '9'; i++) {
                    secs = secs * 10 + (arg[i] - '0');
                }
                do_sleep(secs);
            }
        } else {
            print("[AGENT] Unknown action: ");
            printn(action);
        }
    }
}

static void dispatch_command(void) {
    if (strcmp_(linebuf, "help") == 0) {
        printn("Commands:");
        printn("  help              Show this help");
        printn("  ls [dir]          List directory");
        printn("  cat <file>        Show file contents");
        printn("  head <file> [n]   Show first n lines");
        printn("  tail <file> [n]   Show last n lines");
        printn("  grep <p> <file>   Search for pattern");
        printn("  wc <file>         Count lines/words/bytes");
        printn("  mkdir <dir>       Create directory");
        printn("  rmdir <dir>       Remove empty directory");
        printn("  echo <text>       Print text");
        printn("  pwd               Print working directory");
        printn("  cd <dir>          Change directory");
        printn("  rm <file>         Remove file");
        printn("  cp <src> <dst>    Copy file");
        printn("  mv <src> <dst>    Move/rename file");
        printn("  touch <file>      Create empty file");
        printn("  ping <host>       Ping host");
        printn("  wget <h> <p> <f>  Download file via HTTP");
        printn("  http <host> [p]   HTTP GET");
        printn("  sleep <secs>      Sleep");
        printn("  read <var>        Read input into variable");
        printn("  seq [a] [b] [s]   Print number sequence");
        printn("  ps                List processes");
        printn("  lsmod             List loaded kernel modules");
        printn("  sysctl [n[=v]]    Read/write kernel parameter");
        printn("  jobs              List background jobs");
        printn("  cmd &            Run command in background");
        printn("  cmd > file       Redirect output to file");
        printn("  cmd < file       Redirect input from file");
        printn("  kill <pid> [sig]  Send signal to process");
        printn("  env               Show environment variables");
        printn("  export K=V        Set environment variable");
        printn("  alias [n[=v]]     Show/set command alias");
        printn("  unalias <name>    Remove alias");
        printn("  free              Show memory usage");
        printn("  uptime            Show system uptime");
        printn("  clear             Clear screen");
        printn("  uname             Show system info");
        printn("  dmesg             Show kernel messages");
        printn("  history           Show command history");
        printn("  date              Show date and time");
        printn("  df                Show disk usage");
        printn("  mount             Show mounted filesystems");
        printn("  hostname          Show hostname");
        printn("  whoami            Show current user");
        printn("  id                Show user/group IDs");
        printn("  chmod <m> <f>    Change file permissions");
        printn("  find <d> <n>     Find file by name");
        printn("  source <file>     Execute commands from file");
        printn("  agent             Start AI agent loop");
        printn("  llm <m> <p>      Run local LLM inference");
        printn("  reboot            Reboot system");
        printn("  exit              Power off");
    } else if (strcmp_(linebuf, "ls") == 0) {
        ls_dir(".");
    } else if (starts_with(linebuf, "ls ")) {
        ls_dir(linebuf + 3);
    } else if (strcmp_(linebuf, "ps") == 0) {
        ps_list();
    } else if (strcmp_(linebuf, "lsmod") == 0) {
        cat_file("/proc/modules");
    } else if (starts_with(linebuf, "sysctl ")) {
        char *rest = linebuf + 7;
        while (*rest == ' ') rest++;
        char *name = rest;
        char *value = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == '=') {
                rest[i] = '\0';
                value = &rest[i + 1];
                break;
            }
        }
        // Build path: replace . with /
        char path[128] = "/proc/sys/";
        int j = 10;
        int k = 0;
        while (name[k] && j < sizeof(path) - 2) {
            if (name[k] == '.') path[j++] = '/';
            else path[j++] = name[k];
            k++;
        }
        path[j] = '\0';
        if (value) {
            int fd = sys_open(path, 0x241, 0644);
            if (fd >= 0) {
                sys_write(fd, value, strlen_(value));
                sys_close(fd);
            } else {
                printn("sysctl: cannot write parameter");
            }
        } else {
            cat_file(path);
        }
    } else if (strcmp_(linebuf, "jobs") == 0) {
        jobs_list();
    } else if (starts_with(linebuf, "kill ")) {
        char *rest = linebuf + 5;
        while (*rest == ' ') rest++;
        int pid = 0;
        while (*rest >= '0' && *rest <= '9') {
            pid = pid * 10 + (*rest - '0');
            rest++;
        }
        int sig = 15; // SIGTERM
        while (*rest == ' ') rest++;
        if (*rest >= '0' && *rest <= '9') {
            sig = 0;
            while (*rest >= '0' && *rest <= '9') {
                sig = sig * 10 + (*rest - '0');
                rest++;
            }
        }
        if (pid > 0) {
            if (sys_kill(pid, sig) < 0) {
                printn("kill: failed");
            }
        } else {
            printn("Usage: kill <pid> [signal]");
        }
    } else if (strcmp_(linebuf, "env") == 0) {
        print_env();
    } else if (starts_with(linebuf, "export ")) {
        char *rest = linebuf + 7;
        while (*rest == ' ') rest++;
        char *key = rest;
        char *value = "";
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == '=') {
                rest[i] = '\0';
                value = &rest[i + 1];
                break;
            }
        }
        env_set(key, value);
    } else if (strcmp_(linebuf, "alias") == 0) {
        alias_print();
    } else if (starts_with(linebuf, "alias ")) {
        char *rest = linebuf + 6;
        while (*rest == ' ') rest++;
        char *name = rest;
        char *value = "";
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == '=') {
                rest[i] = '\0';
                value = &rest[i + 1];
                break;
            }
        }
        alias_set(name, value);
    } else if (starts_with(linebuf, "unalias ")) {
        alias_unset(linebuf + 8);
    } else if (strcmp_(linebuf, "free") == 0) {
        print_free();
    } else if (strcmp_(linebuf, "uptime") == 0) {
        print_uptime();
    } else if (strcmp_(linebuf, "clear") == 0) {
        do_clear();
    } else if (strcmp_(linebuf, "uname") == 0) {
        print_uname();
    } else if (strcmp_(linebuf, "dmesg") == 0) {
        print_dmesg();
    } else if (strcmp_(linebuf, "history") == 0) {
        history_print();
    } else if (strcmp_(linebuf, "date") == 0) {
        print_date();
    } else if (strcmp_(linebuf, "df") == 0) {
        print_df();
    } else if (strcmp_(linebuf, "mount") == 0) {
        cat_file("/proc/mounts");
    } else if (strcmp_(linebuf, "hostname") == 0) {
        print_hostname();
    } else if (strcmp_(linebuf, "whoami") == 0) {
        print_whoami();
    } else if (strcmp_(linebuf, "id") == 0) {
        print_id();
    } else if (starts_with(linebuf, "chmod ")) {
        char *rest = linebuf + 6;
        while (*rest == ' ') rest++;
        char *mode_str = rest;
        char *path = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                path = &rest[i + 1];
                break;
            }
        }
        if (path) {
            int mode = parse_octal(mode_str);
            do_chmod(path, mode);
        } else {
            printn("Usage: chmod <mode> <file>");
        }
    } else if (starts_with(linebuf, "find ")) {
        char *rest = linebuf + 5;
        while (*rest == ' ') rest++;
        char *dir = rest;
        char *name = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                name = &rest[i + 1];
                break;
            }
        }
        if (name) {
            find_file(dir, name);
        } else {
            printn("Usage: find <dir> <name>");
        }
    } else if (starts_with(linebuf, "source ")) {
        source_file(linebuf + 7);
    } else if (strcmp_(linebuf, "reboot") == 0) {
        do_reboot();
    } else if (strcmp_(linebuf, "exit") == 0 || strcmp_(linebuf, "quit") == 0) {
        do_poweroff();
    } else if (starts_with(linebuf, "cat ")) {
        cat_file(linebuf + 4);
    } else if (starts_with(linebuf, "head ")) {
        char *rest = linebuf + 5;
        while (*rest == ' ') rest++;
        char *path = rest;
        int nlines = 10;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                nlines = 0;
                char *p = &rest[i + 1];
                while (*p >= '0' && *p <= '9') {
                    nlines = nlines * 10 + (*p - '0');
                    p++;
                }
                break;
            }
        }
        if (nlines <= 0) nlines = 10;
        head_file(path, nlines);
    } else if (starts_with(linebuf, "tail ")) {
        char *rest = linebuf + 5;
        while (*rest == ' ') rest++;
        char *path = rest;
        int nlines = 10;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                nlines = 0;
                char *p = &rest[i + 1];
                while (*p >= '0' && *p <= '9') {
                    nlines = nlines * 10 + (*p - '0');
                    p++;
                }
                break;
            }
        }
        if (nlines <= 0) nlines = 10;
        tail_file(path, nlines);
    } else if (starts_with(linebuf, "grep ")) {
        char *rest = linebuf + 5;
        while (*rest == ' ') rest++;
        char *pattern = rest;
        char *path = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                path = &rest[i + 1];
                break;
            }
        }
        if (path) {
            grep_file(pattern, path);
        } else {
            printn("Usage: grep <pattern> <file>");
        }
    } else if (starts_with(linebuf, "wc ")) {
        wc_file(linebuf + 3);
    } else if (starts_with(linebuf, "mkdir ")) {
        if (sys_mkdir(linebuf + 6, 0755) < 0) {
            printn("mkdir: failed");
        }
    } else if (starts_with(linebuf, "rmdir ")) {
        if (sys_rmdir(linebuf + 6) < 0) {
            printn("rmdir: failed");
        }
    } else if (strcmp_(linebuf, "pwd") == 0) {
        char cwd[256];
        if (sys_getcwd(cwd, sizeof(cwd)) >= 0) {
            printn(cwd);
        } else {
            printn("pwd: failed");
        }
    } else if (starts_with(linebuf, "cd ")) {
        char *path = linebuf + 3;
        while (*path == ' ') path++;
        if (sys_chdir(path) < 0) {
            printn("cd: failed");
        }
    } else if (strcmp_(linebuf, "cd") == 0) {
        if (sys_chdir("/") < 0) {
            printn("cd: failed");
        }
    } else if (starts_with(linebuf, "rm ")) {
        if (sys_unlink(linebuf + 3) < 0) {
            printn("rm: failed");
        }
    } else if (starts_with(linebuf, "cp ")) {
        char *src = linebuf + 3;
        char *dst = NULL;
        for (int i = 3; linebuf[i]; i++) {
            if (linebuf[i] == ' ') {
                linebuf[i] = '\0';
                dst = &linebuf[i + 1];
                break;
            }
        }
        if (dst) {
            int sfd = sys_open(src, 0, 0);
            if (sfd < 0) {
                printn("cp: cannot open source");
            } else {
                int dfd = sys_open(dst, 0x241, 0644);
                if (dfd < 0) {
                    printn("cp: cannot create destination");
                } else {
                    char buf[256];
                    int n;
                    while ((n = sys_read(sfd, buf, sizeof(buf))) > 0) {
                        sys_write(dfd, buf, n);
                    }
                    sys_close(dfd);
                }
                sys_close(sfd);
            }
        } else {
            printn("Usage: cp <src> <dst>");
        }
    } else if (starts_with(linebuf, "mv ")) {
        char *src = linebuf + 3;
        char *dst = NULL;
        for (int i = 3; linebuf[i]; i++) {
            if (linebuf[i] == ' ') {
                linebuf[i] = '\0';
                dst = &linebuf[i + 1];
                break;
            }
        }
        if (dst) {
            if (sys_rename(src, dst) < 0) {
                printn("mv: failed");
            }
        } else {
            printn("Usage: mv <src> <dst>");
        }
    } else if (starts_with(linebuf, "touch ")) {
        int fd = sys_open(linebuf + 6, 0x241, 0644);
        if (fd < 0) {
            printn("touch: failed");
        } else {
            sys_close(fd);
        }
    } else if (starts_with(linebuf, "ping ")) {
        do_ping(linebuf + 5);
    } else if (starts_with(linebuf, "wget ")) {
        char *rest = linebuf + 5;
        while (*rest == ' ') rest++;
        char *host = rest;
        char *path = NULL;
        char *outfile = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                path = &rest[i + 1];
                break;
            }
        }
        if (path) {
            for (int i = 0; path[i]; i++) {
                if (path[i] == ' ') {
                    path[i] = '\0';
                    outfile = &path[i + 1];
                    break;
                }
            }
        }
        if (host && path && outfile) {
            do_wget(host, path, outfile);
        } else {
            printn("Usage: wget <host> <path> <output_file>");
        }
    } else if (starts_with(linebuf, "echo ")) {
        printn(linebuf + 5);
    } else if (starts_with(linebuf, "http ")) {
        char *host = linebuf + 5;
        char *path = NULL;
        for (int i = 5; linebuf[i]; i++) {
            if (linebuf[i] == ' ') {
                linebuf[i] = '\0';
                path = &linebuf[i + 1];
                break;
            }
        }
        do_http_get(host, path ? path : "/");
    } else if (starts_with(linebuf, "sleep ")) {
        int secs = 0;
        char *p = linebuf + 6;
        while (*p >= '0' && *p <= '9') {
            secs = secs * 10 + (*p - '0');
            p++;
        }
        do_sleep(secs);
    } else if (starts_with(linebuf, "read ")) {
        char varname[64];
        char *p = linebuf + 5;
        while (*p == ' ') p++;
        int i = 0;
        while (*p && *p != ' ' && i < 63) {
            varname[i++] = *p++;
        }
        varname[i] = '\0';
        read_line();
        env_set(varname, linebuf);
    } else if (starts_with(linebuf, "seq ")) {
        char *p = linebuf + 4;
        while (*p == ' ') p++;
        int nums[3] = {1, 1, 1};
        int num_count = 0;
        while (*p && num_count < 3) {
            int n = 0;
            int neg = 0;
            if (*p == '-') { neg = 1; p++; }
            while (*p >= '0' && *p <= '9') {
                n = n * 10 + (*p - '0');
                p++;
            }
            if (neg) n = -n;
            nums[num_count++] = n;
            while (*p == ' ') p++;
        }
        int start, end, step;
        if (num_count == 1) { start = 1; end = nums[0]; step = 1; }
        else if (num_count == 2) { start = nums[0]; end = nums[1]; step = 1; }
        else { start = nums[0]; end = nums[1]; step = nums[2]; }
        if (step == 0) step = 1;
        if (step > 0) {
            for (int i = start; i <= end; i += step) {
                print_int(i); print("\n");
            }
        } else {
            for (int i = start; i >= end; i += step) {
                print_int(i); print("\n");
            }
        }
    } else if (strcmp_(linebuf, "agent") == 0) {
        agent_loop();
    } else if (starts_with(linebuf, "llm ")) {
        char *rest = linebuf + 4;
        while (*rest == ' ') rest++;
        char *model_path = rest;
        char *prompt = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                prompt = &rest[i + 1];
                break;
            }
        }
        if (prompt) {
            printn("[LLM] Loading model...");
            char output[1024];
            int n = llm_generate(model_path, prompt, output, sizeof(output), 64);
            if (n > 0) {
                printn("[LLM] Generated:");
                printn(output);
            } else {
                printn("[LLM] Generation failed");
            }
        } else {
            printn("Usage: llm <model> <prompt>");
        }
    }
}

static void shell_loop(void) {
    printn("");
    printn("========================================");
    printn("  Nymoris Agentic AI Operating System");
    printn("========================================");
    printn("");

    while (1) {
        jobs_reap();

        print("$ ");
        read_line();

        if (linepos == 0) continue;

        // Handle history expansion: !n or !!
        if (linebuf[0] == '!') {
            const char *prev = NULL;
            if (linebuf[1] == '!') {
                prev = history_get(history_count);
            } else {
                int n = 0;
                char *p = linebuf + 1;
                while (*p >= '0' && *p <= '9') {
                    n = n * 10 + (*p - '0');
                    p++;
                }
                prev = history_get(n);
            }
            if (prev) {
                int i = 0;
                while (prev[i] && i < sizeof(linebuf) - 1) {
                    linebuf[i] = prev[i];
                    i++;
                }
                linebuf[i] = '\0';
                linepos = i;
                printn(linebuf);
            } else {
                printn("history: no such command");
                continue;
            }
        }

        history_add(linebuf);
        expand_vars();
        expand_alias();

        // Parse background '&', output redirection '>', input redirection '<'
        int bg = 0;
        char redirect_file[64];
        char input_file[64];
        int redirect = 0;
        int input_redirect = 0;
        int saved_stdout = -1;
        int saved_stdin = -1;

        // Check for trailing '&'
        int cmd_len = 0;
        while (linebuf[cmd_len]) cmd_len++;
        while (cmd_len > 0 && linebuf[cmd_len - 1] == ' ') cmd_len--;
        if (cmd_len > 0 && linebuf[cmd_len - 1] == '&') {
            bg = 1;
            cmd_len--;
            while (cmd_len > 0 && linebuf[cmd_len - 1] == ' ') cmd_len--;
            linebuf[cmd_len] = '\0';
        }

        // Check for '>' redirection (look for " > ")
        char *gt = NULL;
        for (int i = 1; linebuf[i]; i++) {
            if (linebuf[i] == '>' && linebuf[i-1] == ' ') {
                gt = &linebuf[i];
            }
        }
        if (gt) {
            *gt = '\0';
            gt++;
            while (*gt == ' ') gt++;
            int j = 0;
            while (*gt && *gt != ' ' && *gt != '&' && j < 63) {
                redirect_file[j++] = *gt++;
            }
            redirect_file[j] = '\0';
            redirect = 1;
        }

        // Check for '<' input redirection (look for " < ")
        char *lt = NULL;
        for (int i = 1; linebuf[i]; i++) {
            if (linebuf[i] == '<' && linebuf[i-1] == ' ') {
                lt = &linebuf[i];
            }
        }
        if (lt) {
            *lt = '\0';
            lt++;
            while (*lt == ' ') lt++;
            int j = 0;
            while (*lt && *lt != ' ' && *lt != '&' && *lt != '>' && j < 63) {
                input_file[j++] = *lt++;
            }
            input_file[j] = '\0';
            input_redirect = 1;
        }

        // Set up output redirection
        if (redirect) {
            int fd = sys_open(redirect_file, 0x241, 0644);
            if (fd < 0) {
                print("redirect: cannot open ");
                printn(redirect_file);
                continue;
            }
            saved_stdout = 63;
            asm volatile("syscall" : : "a"(SYS_dup2), "D"(1), "S"(saved_stdout) : "rcx", "r11", "memory");
            asm volatile("syscall" : : "a"(SYS_dup2), "D"(fd), "S"(1) : "rcx", "r11", "memory");
            sys_close(fd);
        }

        // Set up input redirection
        if (input_redirect) {
            int fd = sys_open(input_file, 0, 0);
            if (fd < 0) {
                print("redirect: cannot open ");
                printn(input_file);
                continue;
            }
            saved_stdin = 62;
            asm volatile("syscall" : : "a"(SYS_dup2), "D"(0), "S"(saved_stdin) : "rcx", "r11", "memory");
            asm volatile("syscall" : : "a"(SYS_dup2), "D"(fd), "S"(0) : "rcx", "r11", "memory");
            sys_close(fd);
        }

        // Fork for background execution
        if (bg) {
            int pid = sys_fork();
            if (pid > 0) {
                jobs_add(pid, linebuf);
                if (saved_stdout >= 0) {
                    asm volatile("syscall" : : "a"(SYS_dup2), "D"(saved_stdout), "S"(1) : "rcx", "r11", "memory");
                    sys_close(saved_stdout);
                }
                if (saved_stdin >= 0) {
                    asm volatile("syscall" : : "a"(SYS_dup2), "D"(saved_stdin), "S"(0) : "rcx", "r11", "memory");
                    sys_close(saved_stdin);
                }
                continue;
            } else if (pid < 0) {
                printn("fork failed");
                if (saved_stdout >= 0) {
                    asm volatile("syscall" : : "a"(SYS_dup2), "D"(saved_stdout), "S"(1) : "rcx", "r11", "memory");
                    sys_close(saved_stdout);
                }
                if (saved_stdin >= 0) {
                    asm volatile("syscall" : : "a"(SYS_dup2), "D"(saved_stdin), "S"(0) : "rcx", "r11", "memory");
                    sys_close(saved_stdin);
                }
                continue;
            }
            if (saved_stdout >= 0) {
                sys_close(saved_stdout);
                saved_stdout = -1;
            }
            if (saved_stdin >= 0) {
                sys_close(saved_stdin);
                saved_stdin = -1;
            }
        }

        dispatch_command();

        // Cleanup: child exits or restore stdout/stdin
        if (bg) {
            sys_exit(0);
        }
        if (saved_stdout >= 0) {
            asm volatile("syscall" : : "a"(SYS_dup2), "D"(saved_stdout), "S"(1) : "rcx", "r11", "memory");
            sys_close(saved_stdout);
        }
        if (saved_stdin >= 0) {
            asm volatile("syscall" : : "a"(SYS_dup2), "D"(saved_stdin), "S"(0) : "rcx", "r11", "memory");
            sys_close(saved_stdin);
        }
    }
}

void _start(void) {
    // Align stack to 16 bytes for SSE compatibility
    asm volatile("andq $-16, %%rsp" ::: "memory");

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
