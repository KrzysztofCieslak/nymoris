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
#define SYS_pipe     22
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
#define SYS_setsockopt 54
#define SYS_unshare  272
#define SYS_rt_sigaction 13
#define SYS_umount2  166
#define SYS_stat     4
#define SYS_link     86
#define SYS_symlink  88

#define SIGINT  2
#define SIGTERM 15
#define SIGCHLD 17
#define SIG_IGN 1

#define SOL_SOCKET 1
#define SO_RCVTIMEO 20
#define SO_SNDTIMEO 21

#define CLONE_NEWNS  0x00020000
#define CLONE_NEWPID 0x20000000

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

static int sys_dup2(int oldfd, int newfd) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_dup2), "D"(oldfd), "S"(newfd)
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

static int sys_umount2(const char *target, int flags) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_umount2), "D"(target), "S"(flags)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_stat(const char *path, void *buf) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_stat), "D"(path), "S"(buf)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_link(const char *oldpath, const char *newpath) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_link), "D"(oldpath), "S"(newpath)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_symlink(const char *target, const char *linkpath) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_symlink), "D"(target), "S"(linkpath)
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

static int sys_setsockopt(int fd, int level, int optname, const void *optval, int optlen) {
    int ret;
    register int r10_ asm("r10") = optname;
    register const void *r8_ asm("r8") = optval;
    register int r9_ asm("r9") = optlen;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_setsockopt), "D"(fd), "S"(level), "d"(optname), "r"(r10_), "r"(r8_), "r"(r9_)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_pipe(int pipefd[2]) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_pipe), "D"(pipefd)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_unshare(int flags) {
    int ret;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_unshare), "D"(flags)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static int sys_rt_sigaction(int sig, const void *act, void *oldact, int sigsetsize) {
    int ret;
    register void *r10_ asm("r10") = (void *)(long)sigsetsize;
    asm volatile(
        "syscall"
        : "=a"(ret)
        : "a"(SYS_rt_sigaction), "D"(sig), "S"(act), "d"(oldact), "r"(r10_)
        : "rcx", "r11", "memory"
    );
    return ret;
}

static volatile int interrupted = 0;

static void sigint_handler(int sig) {
    interrupted = 1;
}

static void signal_ignore(int sig) {
    struct {
        void *handler;
        unsigned long flags;
        void *restorer;
        unsigned long mask;
    } sa = { (void *)SIG_IGN, 0, NULL, 0 };
    sys_rt_sigaction(sig, &sa, NULL, 8);
}

static void signal_default(int sig) {
    struct {
        void *handler;
        unsigned long flags;
        void *restorer;
        unsigned long mask;
    } sa = { (void *)0, 0, NULL, 0 };
    sys_rt_sigaction(sig, &sa, NULL, 8);
}

static void signal_catch(int sig) {
    struct {
        void *handler;
        unsigned long flags;
        void *restorer;
        unsigned long mask;
    } sa = { (void *)sigint_handler, 0x04000000, NULL, 0 };
    sys_rt_sigaction(sig, &sa, NULL, 8);
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

// Reap any stray zombie children not in the job table
static void reap_stray_children(void) {
    int status;
    while (sys_wait4(-1, &status, 1, NULL) > 0) {}
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
        if (c == 3) { // Ctrl+C
            linepos = 0;
            linebuf[0] = '\0';
            printn("^C");
            return;
        }
        if (c == 127 || c == 8) { // Backspace / DEL
            if (linepos > 0) {
                linepos--;
                print("\b \b");
            }
            continue;
        }
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

static void print_hex_byte(unsigned char b) {
    const char *hex = "0123456789abcdef";
    char buf[2];
    buf[0] = hex[b >> 4];
    buf[1] = hex[b & 0x0f];
    sys_write(1, buf, 2);
}

static void do_hexdump(const char *path) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("hexdump: cannot open file");
        return;
    }
    unsigned char buf[16];
    long offset = 0;
    int n;
    while ((n = sys_read(fd, (char*)buf, 16)) > 0) {
        // Print offset in hex
        char offbuf[9];
        long tmp = offset;
        for (int i = 7; i >= 0; i--) {
            offbuf[i] = "0123456789abcdef"[tmp & 0x0f];
            tmp >>= 4;
        }
        offbuf[8] = '\0';
        print(offbuf);
        print("  ");

        // Print hex bytes
        for (int i = 0; i < 16; i++) {
            if (i < n) {
                print_hex_byte(buf[i]);
            } else {
                print("  ");
            }
            if (i == 7) print(" ");
            else print(" ");
        }
        print(" |");
        // Print ASCII
        for (int i = 0; i < n; i++) {
            if (buf[i] >= 32 && buf[i] < 127) {
                char c = (char)buf[i];
                sys_write(1, &c, 1);
            } else {
                print(".");
            }
        }
        printn("|");
        offset += n;
    }
    sys_close(fd);
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

static const char b64_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static void do_base64_enc(const char *path) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("base64: cannot open file");
        return;
    }
    unsigned char buf[48];
    int n;
    while ((n = sys_read(fd, (char*)buf, 48)) > 0) {
        for (int i = 0; i < n; i += 3) {
            int b[3] = {0, 0, 0};
            int len = 0;
            for (int j = 0; j < 3; j++) {
                if (i + j < n) { b[j] = buf[i + j]; len++; }
            }
            char out[4];
            out[0] = b64_chars[(b[0] >> 2) & 0x3f];
            out[1] = b64_chars[((b[0] & 0x03) << 4) | ((b[1] >> 4) & 0x0f)];
            out[2] = len > 1 ? b64_chars[((b[1] & 0x0f) << 2) | ((b[2] >> 6) & 0x03)] : '=';
            out[3] = len > 2 ? b64_chars[b[2] & 0x3f] : '=';
            sys_write(1, out, 4);
        }
    }
    sys_close(fd);
    printn("");
}

static void do_base64_dec(const char *path) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("base64: cannot open file");
        return;
    }
    unsigned char inbuf[64];
    int inpos = 0;
    int n;
    while ((n = sys_read(fd, (char*)inbuf + inpos, sizeof(inbuf) - inpos)) > 0) {
        inpos += n;
        int outpos = 0;
        while (outpos + 4 <= inpos) {
            int b[4] = {-1, -1, -1, -1};
            for (int j = 0; j < 4; j++) {
                char c = (char)inbuf[outpos + j];
                if (c >= 'A' && c <= 'Z') b[j] = c - 'A';
                else if (c >= 'a' && c <= 'z') b[j] = c - 'a' + 26;
                else if (c >= '0' && c <= '9') b[j] = c - '0' + 52;
                else if (c == '+') b[j] = 62;
                else if (c == '/') b[j] = 63;
                else if (c == '=') b[j] = -2;
            }
            if (b[0] >= 0 && b[1] >= 0) {
                char out[3];
                out[0] = (b[0] << 2) | ((b[1] >> 4) & 0x03);
                sys_write(1, out, 1);
                if (b[2] >= 0) {
                    out[1] = ((b[1] & 0x0f) << 4) | ((b[2] >> 2) & 0x0f);
                    sys_write(1, out + 1, 1);
                    if (b[3] >= 0) {
                        out[2] = ((b[2] & 0x03) << 6) | b[3];
                        sys_write(1, out + 2, 1);
                    }
                }
            }
            outpos += 4;
        }
        // Move remaining bytes to front
        for (int i = 0; i < inpos - outpos; i++) {
            inbuf[i] = inbuf[outpos + i];
        }
        inpos -= outpos;
    }
    sys_close(fd);
}

static void do_stat(const char *path) {
    struct {
        unsigned long st_dev;
        unsigned long st_ino;
        unsigned long st_nlink;
        unsigned int st_mode;
        unsigned int st_uid;
        unsigned int st_gid;
        unsigned int __pad0;
        unsigned long st_rdev;
        long st_size;
        long st_blksize;
        long st_blocks;
        unsigned long st_atime;
        unsigned long st_atime_nsec;
        unsigned long st_mtime;
        unsigned long st_mtime_nsec;
        unsigned long st_ctime;
        unsigned long st_ctime_nsec;
        long __unused[3];
    } st;
    if (sys_stat(path, &st) < 0) {
        printn("stat: cannot stat file");
        return;
    }
    print("File: "); printn(path);
    print("Size: "); print_int((int)st.st_size); printn(" bytes");
    print("Mode: 0"); print_int((int)(st.st_mode & 0777)); printn("");
    print("Uid: "); print_int((int)st.st_uid); print("  Gid: "); print_int((int)st.st_gid); printn("");
    print("Links: "); print_int((int)st.st_nlink); printn("");
    print("Blocks: "); print_int((int)st.st_blocks); printn("");
    print("Inode: "); print_int((int)st.st_ino); printn("");
}

static void do_ln(const char *target, const char *linkpath, int symlink_) {
    int ret;
    if (symlink_) {
        ret = sys_symlink(target, linkpath);
    } else {
        ret = sys_link(target, linkpath);
    }
    if (ret < 0) {
        printn("ln: failed");
    }
}

static void do_cmp(const char *path1, const char *path2) {
    int fd1 = sys_open(path1, 0, 0);
    int fd2 = sys_open(path2, 0, 0);
    if (fd1 < 0) { printn("cmp: cannot open first file"); if (fd2 >= 0) sys_close(fd2); return; }
    if (fd2 < 0) { printn("cmp: cannot open second file"); sys_close(fd1); return; }
    char buf1[256], buf2[256];
    long offset = 0;
    int n1, n2;
    while (1) {
        n1 = sys_read(fd1, buf1, sizeof(buf1));
        n2 = sys_read(fd2, buf2, sizeof(buf2));
        if (n1 < 0) n1 = 0;
        if (n2 < 0) n2 = 0;
        int min = n1 < n2 ? n1 : n2;
        for (int i = 0; i < min; i++) {
            if (buf1[i] != buf2[i]) {
                print(path1); print(" "); print(path2); print(" differ: byte ");
                print_int((int)(offset + i + 1));
                print(", line ");
                print_int(1); // simplified
                printn("");
                sys_close(fd1); sys_close(fd2);
                return;
            }
        }
        if (n1 != n2) {
            printn("cmp: EOF on one file");
            sys_close(fd1); sys_close(fd2);
            return;
        }
        if (n1 == 0) break;
        offset += n1;
    }
    sys_close(fd1); sys_close(fd2);
    printn("cmp: files are identical");
}

static void do_chmod(const char *path, int mode) {
    if (sys_chmod(path, mode) < 0) {
        printn("chmod: failed");
    }
}

static void do_cp(const char *src, const char *dst) {
    int sfd = sys_open(src, 0, 0);
    if (sfd < 0) {
        printn("cp: cannot open source");
        return;
    }
    int dfd = sys_open(dst, 0x241, 0644);
    if (dfd < 0) {
        printn("cp: cannot create destination");
        sys_close(sfd);
        return;
    }
    char buf[256];
    int n;
    while ((n = sys_read(sfd, buf, sizeof(buf))) > 0) {
        sys_write(dfd, buf, n);
    }
    sys_close(dfd);
    sys_close(sfd);
}

static void do_mv(const char *src, const char *dst) {
    if (sys_rename(src, dst) < 0) {
        printn("mv: failed");
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

static void http_set_timeout(int fd, int secs) {
    struct { uint64_t sec; uint64_t usec; } tv = { secs, 0 };
    sys_setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
    sys_setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
}

// Parse a hex string, returns value and advances pointer.
static int parse_hex(const char *s, const char **end) {
    int val = 0;
    while (*s) {
        char c = *s;
        int digit = -1;
        if (c >= '0' && c <= '9') digit = c - '0';
        else if (c >= 'a' && c <= 'f') digit = 10 + (c - 'a');
        else if (c >= 'A' && c <= 'F') digit = 10 + (c - 'A');
        else break;
        val = val * 16 + digit;
        s++;
    }
    if (end) *end = s;
    return val;
}

// Reads HTTP response from fd, extracts body into `out`.
// Returns body length, sets *status to HTTP status code.
// If redirect is non-NULL and redirect_max > 0, copies Location header into redirect on 3xx.
// Handles Content-Length and chunked transfer encoding.
static int http_parse_response(int fd, char *out, int out_max, int *status, char *redirect, int redirect_max) {
    char hdr[2048];
    int hdr_len = 0;
    int found_end = 0;

    // Read headers byte by byte until \r\n\r\n
    while (hdr_len < sizeof(hdr) - 1) {
        char c;
        int n = sys_read(fd, &c, 1);
        if (n <= 0) break;
        hdr[hdr_len++] = c;
        if (hdr_len >= 4 && hdr[hdr_len-4] == '\r' && hdr[hdr_len-3] == '\n' &&
            hdr[hdr_len-2] == '\r' && hdr[hdr_len-1] == '\n') {
            found_end = 1;
            break;
        }
    }
    hdr[hdr_len] = '\0';
    if (!found_end) return -1;

    // Parse status code
    *status = 0;
    const char *p = hdr;
    while (*p && *p != ' ') p++; // skip "HTTP/1.x"
    if (*p == ' ') {
        p++;
        while (*p >= '0' && *p <= '9') {
            *status = *status * 10 + (*p - '0');
            p++;
        }
    }

    // Look for Content-Length and Transfer-Encoding
    int content_len = -1;
    int is_chunked = 0;

    const char *line = hdr;
    while (*line) {
        // Find end of line
        const char *eol = line;
        while (*eol && !(*eol == '\r' && *(eol+1) == '\n')) eol++;
        if (!*eol) break;

        if (starts_with(line, "Content-Length: ") || starts_with(line, "content-length: ")) {
            content_len = 0;
            const char *v = line + 16;
            while (*v >= '0' && *v <= '9') {
                content_len = content_len * 10 + (*v - '0');
                v++;
            }
        }
        if (starts_with(line, "Transfer-Encoding: chunked") ||
            starts_with(line, "transfer-encoding: chunked")) {
            is_chunked = 1;
        }
        if (redirect && redirect_max > 0) {
            if (starts_with(line, "Location: ") || starts_with(line, "location: ")) {
                const char *v = line + 10;
                int ri = 0;
                while (*v && *v != '\r' && *v != '\n' && ri < redirect_max - 1) {
                    redirect[ri++] = *v++;
                }
                redirect[ri] = '\0';
            }
        }

        line = eol + 2;
        if (*line == '\r' && *(line+1) == '\n') break; // end of headers
    }

    int out_pos = 0;

    if (is_chunked) {
        // Read chunk size line, then chunk data, repeat
        while (out_pos < out_max - 1) {
            char chunk_hdr[32];
            int ch_len = 0;
            // Read chunk size line
            while (ch_len < sizeof(chunk_hdr) - 1) {
                char c;
                int n = sys_read(fd, &c, 1);
                if (n <= 0) return -1;
                chunk_hdr[ch_len++] = c;
                if (ch_len >= 2 && chunk_hdr[ch_len-2] == '\r' && chunk_hdr[ch_len-1] == '\n')
                    break;
            }
            chunk_hdr[ch_len] = '\0';
            int chunk_size = parse_hex(chunk_hdr, NULL);
            if (chunk_size == 0) {
                // Read trailing \r\n
                char c[2];
                sys_read(fd, c, 2);
                break;
            }
            // Read chunk data
            int to_read = chunk_size;
            if (out_pos + to_read > out_max - 1) to_read = out_max - 1 - out_pos;
            while (to_read > 0) {
                int n = sys_read(fd, out + out_pos, to_read);
                if (n <= 0) return -1;
                out_pos += n;
                to_read -= n;
            }
            // Read trailing \r\n after chunk
            char c[2];
            sys_read(fd, c, 2);
        }
    } else if (content_len >= 0) {
        int to_read = content_len;
        if (to_read > out_max - 1) to_read = out_max - 1;
        while (to_read > 0) {
            int n = sys_read(fd, out + out_pos, to_read);
            if (n <= 0) break;
            out_pos += n;
            to_read -= n;
        }
    } else {
        // No content length, read until close
        while (out_pos < out_max - 1) {
            int n = sys_read(fd, out + out_pos, out_max - 1 - out_pos);
            if (n <= 0) break;
            out_pos += n;
        }
    }

    out[out_pos] = '\0';
    return out_pos;
}

static void do_http_get(const char *host, const char *path) {
    char cur_host[64];
    char cur_path[256];
    int i = 0;
    while (host[i] && i < sizeof(cur_host) - 1) { cur_host[i] = host[i]; i++; }
    cur_host[i] = '\0';
    i = 0;
    while (path[i] && i < sizeof(cur_path) - 1) { cur_path[i] = path[i]; i++; }
    cur_path[i] = '\0';

    for (int redirect_count = 0; redirect_count < 5; redirect_count++) {
        uint32_t ip = dns_resolve(cur_host);
        if (ip == 0) {
            printn("http: cannot resolve host");
            return;
        }

        int fd = sys_socket(2, 1, 0);
        if (fd < 0) {
            printn("http: socket failed");
            return;
        }

        struct {
            uint16_t family;
            uint16_t port;
            uint32_t addr;
            char pad[8];
        } sa = {2, 0x5000, ip, {0}};

        http_set_timeout(fd, 10);

        if (sys_connect(fd, &sa, sizeof(sa)) < 0) {
            printn("http: connect failed");
            sys_close(fd);
            return;
        }

        char req[512];
        int len = 0;
        const char *p = "GET ";
        while (*p) req[len++] = *p++;
        p = cur_path;
        while (*p) req[len++] = *p++;
        p = " HTTP/1.1\r\nHost: ";
        while (*p) req[len++] = *p++;
        p = cur_host;
        while (*p) req[len++] = *p++;
        p = "\r\nConnection: close\r\n\r\n";
        while (*p) req[len++] = *p++;

        sys_write(fd, req, len);

        char resp[4096];
        int status;
        char location[256];
        location[0] = '\0';
        int body_len = http_parse_response(fd, resp, sizeof(resp), &status, location, sizeof(location));
        sys_close(fd);

        if (body_len < 0) {
            printn("http: failed to read response");
            return;
        }

        if (status >= 300 && status < 400 && location[0]) {
            // Follow redirect
            if (starts_with(location, "http://")) {
                const char *h = location + 7;
                const char *sl = h;
                while (*sl && *sl != '/') sl++;
                int hl = sl - h;
                if (hl >= sizeof(cur_host)) hl = sizeof(cur_host) - 1;
                for (int j = 0; j < hl; j++) cur_host[j] = h[j];
                cur_host[hl] = '\0';
                if (*sl) {
                    int pl = 0;
                    while (sl[pl] && pl < sizeof(cur_path) - 1) { cur_path[pl] = sl[pl]; pl++; }
                    cur_path[pl] = '\0';
                } else {
                    cur_path[0] = '/';
                    cur_path[1] = '\0';
                }
            } else if (location[0] == '/') {
                int pl = 0;
                while (location[pl] && pl < sizeof(cur_path) - 1) { cur_path[pl] = location[pl]; pl++; }
                cur_path[pl] = '\0';
            }
            continue;
        }

        sys_write(1, resp, body_len);
        return;
    }
    printn("http: too many redirects");
}

static void do_wget(const char *host, const char *path, const char *outfile) {
    char cur_host[64];
    char cur_path[256];
    int i = 0;
    while (host[i] && i < sizeof(cur_host) - 1) { cur_host[i] = host[i]; i++; }
    cur_host[i] = '\0';
    i = 0;
    while (path[i] && i < sizeof(cur_path) - 1) { cur_path[i] = path[i]; i++; }
    cur_path[i] = '\0';

    for (int redirect_count = 0; redirect_count < 5; redirect_count++) {
        uint32_t ip = dns_resolve(cur_host);
        if (ip == 0) {
            printn("wget: cannot resolve host");
            return;
        }

        int fd = sys_socket(2, 1, 0);
        if (fd < 0) {
            printn("wget: socket failed");
            return;
        }

        struct {
            uint16_t family;
            uint16_t port;
            uint32_t addr;
            char pad[8];
        } sa = {2, 0x5000, ip, {0}};

        http_set_timeout(fd, 30);

        if (sys_connect(fd, &sa, sizeof(sa)) < 0) {
            printn("wget: connect failed");
            sys_close(fd);
            return;
        }

        char req[512];
        int len = 0;
        const char *p = "GET ";
        while (*p) req[len++] = *p++;
        p = cur_path;
        while (*p) req[len++] = *p++;
        p = " HTTP/1.1\r\nHost: ";
        while (*p) req[len++] = *p++;
        p = cur_host;
        while (*p) req[len++] = *p++;
        p = "\r\nConnection: close\r\n\r\n";
        while (*p) req[len++] = *p++;

        sys_write(fd, req, len);

        char resp_buf[8192];
        int status;
        char location[256];
        location[0] = '\0';
        int body_len = http_parse_response(fd, resp_buf, sizeof(resp_buf), &status, location, sizeof(location));
        sys_close(fd);

        if (body_len < 0) {
            printn("wget: failed to read response");
            return;
        }

        if (status >= 300 && status < 400 && location[0]) {
            if (starts_with(location, "http://")) {
                const char *h = location + 7;
                const char *sl = h;
                while (*sl && *sl != '/') sl++;
                int hl = sl - h;
                if (hl >= sizeof(cur_host)) hl = sizeof(cur_host) - 1;
                for (int j = 0; j < hl; j++) cur_host[j] = h[j];
                cur_host[hl] = '\0';
                if (*sl) {
                    int pl = 0;
                    while (sl[pl] && pl < sizeof(cur_path) - 1) { cur_path[pl] = sl[pl]; pl++; }
                    cur_path[pl] = '\0';
                } else {
                    cur_path[0] = '/';
                    cur_path[1] = '\0';
                }
            } else if (location[0] == '/') {
                int pl = 0;
                while (location[pl] && pl < sizeof(cur_path) - 1) { cur_path[pl] = location[pl]; pl++; }
                cur_path[pl] = '\0';
            }
            continue;
        }

        int outfd = sys_open(outfile, 0x241, 0644);
        if (outfd < 0) {
            printn("wget: cannot create output file");
            return;
        }
        sys_write(outfd, resp_buf, body_len);
        sys_close(outfd);
        print("wget: saved ");
        print_int(body_len);
        print(" bytes to ");
        printn(outfile);
        return;
    }
    printn("wget: too many redirects");
}

static char api_key[256];

static int do_http_post_body(const char *host, const char *path, const char *body, char *resp, int resp_max) {
    uint32_t ip = dns_resolve(host);
    if (ip == 0) return -1;

    int fd = sys_socket(2, 1, 0);
    if (fd < 0) return -1;

    struct {
        uint16_t family;
        uint16_t port;
        uint32_t addr;
        char pad[8];
    } sa = {2, 0x5000, ip, {0}};

    http_set_timeout(fd, 30);

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
    p = "\r\nContent-Type: application/json";
    while (*p) req[len++] = *p++;

    if (api_key[0]) {
        p = "\r\nAuthorization: Bearer ";
        while (*p) req[len++] = *p++;
        int k = 0;
        while (api_key[k]) req[len++] = api_key[k++];
    }

    p = "\r\nContent-Length: ";
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

    int status;
    int rlen = http_parse_response(fd, resp, resp_max, &status, NULL, 0);
    sys_close(fd);
    return rlen;
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
static char api_key[256] = "";
static char api_model[64] = "gpt-3.5-turbo";

static void agent_load_config(void) {
    const char *key = env_get("NYMORIS_API_KEY");
    if (key) {
        int i = 0;
        while (key[i] && i < sizeof(api_key) - 1) {
            api_key[i] = key[i];
            i++;
        }
        api_key[i] = '\0';
    }
    const char *host = env_get("NYMORIS_API_HOST");
    if (host) {
        int i = 0;
        while (host[i] && i < sizeof(api_host) - 1) {
            api_host[i] = host[i];
            i++;
        }
        api_host[i] = '\0';
    }
    const char *model = env_get("NYMORIS_API_MODEL");
    if (model) {
        int i = 0;
        while (model[i] && i < sizeof(api_model) - 1) {
            api_model[i] = model[i];
            i++;
        }
        api_model[i] = '\0';
    }
    const char *path = env_get("NYMORIS_API_PATH");
    if (path) {
        int i = 0;
        while (path[i] && i < sizeof(api_path) - 1) {
            api_path[i] = path[i];
            i++;
        }
        api_path[i] = '\0';
    }
}

static void run_command(const char *cmd);
static void write_file(const char *path, char *content);
static void append_file(const char *path, char *content);

#define MAX_AGENT_HISTORY 16
#define AGENT_MSG_LEN 512
static char agent_roles[MAX_AGENT_HISTORY][16];
static char agent_msgs[MAX_AGENT_HISTORY][AGENT_MSG_LEN];
static int agent_history_count = 0;
static int agent_history_next = 0;
static int agent_auto_mode = 0;

static void agent_history_add(const char *role, const char *content) {
    int idx = agent_history_next % MAX_AGENT_HISTORY;
    int r = 0;
    while (role[r] && r < 15) {
        agent_roles[idx][r] = role[r];
        r++;
    }
    agent_roles[idx][r] = '\0';
    int len = 0;
    while (content[len]) len++;
    int c = 0;
    if (len > AGENT_MSG_LEN - 20) {
        while (c < AGENT_MSG_LEN - 20) {
            agent_msgs[idx][c] = content[c];
            c++;
        }
        const char *trunc = " ... (truncated)";
        int t = 0;
        while (trunc[t] && c < AGENT_MSG_LEN - 1) {
            agent_msgs[idx][c++] = trunc[t++];
        }
    } else {
        while (c < len && c < AGENT_MSG_LEN - 1) {
            agent_msgs[idx][c] = content[c];
            c++;
        }
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

static void agent_save_history(const char *path) {
    int fd = sys_open(path, 0x241, 0644);
    if (fd < 0) {
        printn("[AGENT] Failed to save history");
        return;
    }
    int start = agent_history_next - agent_history_count;
    for (int i = 0; i < agent_history_count; i++) {
        int idx = (start + i) % MAX_AGENT_HISTORY;
        sys_write(fd, agent_roles[idx], strlen_(agent_roles[idx]) + 1);
        sys_write(fd, agent_msgs[idx], strlen_(agent_msgs[idx]) + 1);
    }
    char end = '\0';
    sys_write(fd, &end, 1);
    sys_close(fd);
    printn("[AGENT] History saved.");
}

static void agent_load_history(const char *path) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) return;
    agent_history_clear();
    char role[16];
    char content[AGENT_MSG_LEN];
    while (1) {
        int ri = 0;
        char c;
        while (ri < 15 && sys_read(fd, &c, 1) == 1 && c != '\0') {
            role[ri++] = c;
        }
        if (ri == 0 && c == '\0') break;
        role[ri] = '\0';
        int ci = 0;
        while (ci < AGENT_MSG_LEN - 1 && sys_read(fd, &c, 1) == 1 && c != '\0') {
            content[ci++] = c;
        }
        content[ci] = '\0';
        agent_history_add(role, content);
    }
    sys_close(fd);
    printn("[AGENT] History loaded.");
}

static int capture_setup(int pipefd[2]) {
    if (sys_pipe(pipefd) < 0) return -1;
    sys_dup2(1, 61);
    sys_dup2(2, 60);
    sys_dup2(pipefd[1], 1);
    sys_dup2(pipefd[1], 2);
    sys_close(pipefd[1]);
    return 0;
}

static int capture_finish(int pipefd[2], char *out, int outlen) {
    sys_dup2(61, 1);
    sys_dup2(60, 2);
    sys_close(61);
    sys_close(60);
    int n = sys_read(pipefd[0], out, outlen - 1);
    sys_close(pipefd[0]);
    if (n < 0) n = 0;
    out[n] = '\0';
    return n;
}

static void append_json_string(char *buf, int *pos, int max, const char *s) {
    for (int i = 0; s[i] && *pos < max - 1; i++) {
        if (s[i] == '"' || s[i] == '\\') buf[(*pos)++] = '\\';
        buf[(*pos)++] = s[i];
    }
}

static char *extract_tool_call(char *text) {
    // Strip markdown code blocks
    if (starts_with(text, "```")) {
        char *end = text;
        while (*end) end++;
        // Find closing ```
        for (char *p = text + 3; *p; p++) {
            if (p[0] == '`' && p[1] == '`' && p[2] == '`') {
                *p = '\0';
                text = text + 3;
                // Skip optional language tag
                while (*text == ' ' || *text == '\n' || *text == '\r') text++;
                while (*text && *text != '\n' && *text != '\r') {
                    if (*text == ' ') { text++; break; }
                    text++;
                }
                while (*text == '\n' || *text == '\r' || *text == ' ') text++;
                break;
            }
        }
    }

    const char *tools[] = {"run ", "exec ", "read ", "write ", "append ", "replace ", "find ", "grep ", "mkdir ", "rm ", "ls ", "cp ", "mv ", "chmod ", "http ", "post ", "sleep ", NULL};
    char *best = NULL;
    int best_pos = 4096;

    for (int t = 0; tools[t]; t++) {
        char *found = text;
        while (*found) {
            if (starts_with(found, tools[t])) {
                int pos = found - text;
                if (pos < best_pos) {
                    best_pos = pos;
                    best = found;
                }
                break;
            }
            found++;
        }
    }
    return best ? best : text;
}

static void do_sleep(int secs);
static void do_replace(const char *path, const char *old, const char *new_);

static void ask_ai(const char *prompt) {
    char body[16384];
    int bl = 0;
    const char *p = "{\"model\":\"";
    while (*p) body[bl++] = *p++;
    int m = 0;
    while (api_model[m]) body[bl++] = api_model[m++];
    p = "\",\"messages\":[{\"role\":\"system\",\"content\":\"";
    while (*p) body[bl++] = *p++;
    const char *sys_prompt = env_get("NYMORIS_SYSTEM_PROMPT");
    if (!sys_prompt || !sys_prompt[0]) {
        sys_prompt = "You are an AI agent running inside Nymoris OS. Available tools: run <cmd>, exec <cmd>, read <file>, write <file> <content>, append <file> <content>, replace <file> <old> <new>, find <dir> <name>, grep <pattern> <file>, mkdir <dir>, rm <file>, ls [dir], cp <src> <dst>, mv <src> <dst>, chmod <mode> <file>, http <host> [path], post <host> <path> <body>, sleep <secs>. Use 'exec' for built-in shell commands. Respond with the tool call only, no explanation.";
    }
    append_json_string(body, &bl, sizeof(body) - 1, sys_prompt);
    p = "\"}";
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

    char resp[8192];
    int resp_len = do_http_post_body(api_host, api_path, body, resp, sizeof(resp));
    if (resp_len < 0) {
        printn("[AGENT] API request failed");
        return;
    }

    char content[4096];
    if (json_extract_string(resp, "content", content, sizeof(content))) {
        printn("[AGENT] AI response:");
        printn(content);

        // Save to conversation history
        agent_history_add("user", prompt);
        agent_history_add("assistant", content);

        // Auto-execute if it looks like a tool call
        char *tc = extract_tool_call(content);
        if (starts_with(tc, "run ")) {
            printn("[AGENT] Executing: run");
            if (agent_auto_mode) {
                int pipefd[2];
                int cap = capture_setup(pipefd);
                run_command(tc + 4);
                if (cap == 0) {
                    char out[2048];
                    capture_finish(pipefd, out, sizeof(out));
                    agent_history_add("system", out);
                }
            } else {
                run_command(tc + 4);
            }
        } else if (starts_with(tc, "exec ")) {
            printn("[AGENT] Executing: exec");
            char saved[1024];
            int i = 0;
            while (linebuf[i] && i < sizeof(saved) - 1) {
                saved[i] = linebuf[i];
                i++;
            }
            saved[i] = '\0';
            char *cmd = tc + 5;
            i = 0;
            while (cmd[i] && i < sizeof(linebuf) - 1) {
                linebuf[i] = cmd[i];
                i++;
            }
            linebuf[i] = '\0';
            linepos = i;
            if (agent_auto_mode) {
                int pipefd[2];
                int cap = capture_setup(pipefd);
                dispatch_command();
                if (cap == 0) {
                    char out[2048];
                    capture_finish(pipefd, out, sizeof(out));
                    agent_history_add("system", out);
                }
            } else {
                dispatch_command();
            }
            i = 0;
            while (saved[i]) {
                linebuf[i] = saved[i];
                i++;
            }
            linebuf[i] = '\0';
            linepos = i;
        } else if (starts_with(tc, "read ")) {
            printn("[AGENT] Executing: read");
            if (agent_auto_mode) {
                int pipefd[2];
                int cap = capture_setup(pipefd);
                cat_file(tc + 5);
                if (cap == 0) {
                    char out[2048];
                    capture_finish(pipefd, out, sizeof(out));
                    agent_history_add("system", out);
                }
            } else {
                cat_file(tc + 5);
            }
        } else if (starts_with(tc, "write ")) {
            printn("[AGENT] Executing: write");
            char *wp = tc + 6;
            char *wpath = wp;
            char *wcontent = NULL;
            for (int i = 0; wp[i]; i++) {
                if (wp[i] == ' ') {
                    wp[i] = '\0';
                    wcontent = &wp[i + 1];
                    break;
                }
            }
            if (wcontent) {
                write_file(wpath, wcontent);
                if (agent_auto_mode) {
                    agent_history_add("system", "File written successfully.");
                }
            }
        } else if (starts_with(tc, "append ")) {
            printn("[AGENT] Executing: append");
            char *ap = tc + 7;
            char *apath = ap;
            char *acontent = NULL;
            for (int i = 0; ap[i]; i++) {
                if (ap[i] == ' ') {
                    ap[i] = '\0';
                    acontent = &ap[i + 1];
                    break;
                }
            }
            if (acontent) {
                append_file(apath, acontent);
                if (agent_auto_mode) {
                    agent_history_add("system", "File appended successfully.");
                }
            }
        } else if (starts_with(tc, "replace ")) {
            printn("[AGENT] Executing: replace");
            char *rp = tc + 8;
            char *rpath = rp;
            char *rold = NULL;
            char *rnew = NULL;
            for (int i = 0; rp[i]; i++) {
                if (rp[i] == ' ') {
                    rp[i] = '\0';
                    rold = &rp[i + 1];
                    break;
                }
            }
            if (rold) {
                for (int i = 0; rold[i]; i++) {
                    if (rold[i] == ' ') {
                        rold[i] = '\0';
                        rnew = &rold[i + 1];
                        break;
                    }
                }
            }
            if (rpath && rold && rnew) {
                do_replace(rpath, rold, rnew);
                if (agent_auto_mode) {
                    agent_history_add("system", "File replaced successfully.");
                }
            }
        } else if (starts_with(tc, "post ")) {
            printn("[AGENT] Executing: post");
            char *pp = tc + 5;
            char *phost = pp;
            char *ppath = NULL;
            char *pbody = NULL;
            for (int i = 0; pp[i]; i++) {
                if (pp[i] == ' ') {
                    pp[i] = '\0';
                    ppath = &pp[i + 1];
                    break;
                }
            }
            if (ppath) {
                for (int i = 0; ppath[i]; i++) {
                    if (ppath[i] == ' ') {
                        ppath[i] = '\0';
                        pbody = &ppath[i + 1];
                        break;
                    }
                }
            }
            if (phost && ppath && pbody) {
                char post_resp[4096];
                int prlen = do_http_post_body(phost, ppath, pbody, post_resp, sizeof(post_resp));
                if (agent_auto_mode) {
                    if (prlen > 0) {
                        agent_history_add("system", post_resp);
                    } else {
                        agent_history_add("system", "POST request failed.");
                    }
                }
            }
        } else if (starts_with(tc, "http ")) {
            printn("[AGENT] Executing: http");
            char *hp = tc + 5;
            char *hhost = hp;
            char *hpath = NULL;
            for (int i = 0; hp[i]; i++) {
                if (hp[i] == ' ') {
                    hp[i] = '\0';
                    hpath = &hp[i + 1];
                    break;
                }
            }
            if (agent_auto_mode) {
                int pipefd[2];
                int cap = capture_setup(pipefd);
                do_http_get(hhost, hpath ? hpath : "/");
                if (cap == 0) {
                    char out[4096];
                    capture_finish(pipefd, out, sizeof(out));
                    agent_history_add("system", out);
                }
            } else {
                do_http_get(hhost, hpath ? hpath : "/");
            }
        } else if (starts_with(tc, "sleep ")) {
            printn("[AGENT] Executing: sleep");
            int secs = 0;
            char *sp = tc + 6;
            while (*sp >= '0' && *sp <= '9') {
                secs = secs * 10 + (*sp - '0');
                sp++;
            }
            do_sleep(secs);
            if (agent_auto_mode) {
                agent_history_add("system", "Sleep completed.");
            }
        } else if (starts_with(tc, "find ")) {
            printn("[AGENT] Executing: find");
            char *fp = tc + 5;
            char *fdir = fp;
            char *fname = NULL;
            for (int i = 0; fp[i]; i++) {
                if (fp[i] == ' ') {
                    fp[i] = '\0';
                    fname = &fp[i + 1];
                    break;
                }
            }
            if (fname) {
                if (agent_auto_mode) {
                    int pipefd[2];
                    int cap = capture_setup(pipefd);
                    find_file(fdir, fname);
                    if (cap == 0) {
                        char out[2048];
                        capture_finish(pipefd, out, sizeof(out));
                        agent_history_add("system", out);
                    }
                } else {
                    find_file(fdir, fname);
                }
            }
        } else if (starts_with(tc, "grep ")) {
            printn("[AGENT] Executing: grep");
            char *gp = tc + 5;
            char *gpat = gp;
            char *gpath = NULL;
            for (int i = 0; gp[i]; i++) {
                if (gp[i] == ' ') {
                    gp[i] = '\0';
                    gpath = &gp[i + 1];
                    break;
                }
            }
            if (gpath) {
                if (agent_auto_mode) {
                    int pipefd[2];
                    int cap = capture_setup(pipefd);
                    grep_file(gpat, gpath);
                    if (cap == 0) {
                        char out[4096];
                        capture_finish(pipefd, out, sizeof(out));
                        agent_history_add("system", out);
                    }
                } else {
                    grep_file(gpat, gpath);
                }
            }
        } else if (starts_with(tc, "mkdir ")) {
            printn("[AGENT] Executing: mkdir");
            if (sys_mkdir(tc + 6, 0755) < 0) {
                printn("[AGENT] mkdir failed");
                if (agent_auto_mode) {
                    agent_history_add("system", "mkdir failed.");
                }
            } else {
                if (agent_auto_mode) {
                    agent_history_add("system", "Directory created.");
                }
            }
        } else if (starts_with(tc, "rm ")) {
            printn("[AGENT] Executing: rm");
            if (sys_unlink(tc + 3) < 0) {
                printn("[AGENT] rm failed");
                if (agent_auto_mode) {
                    agent_history_add("system", "rm failed.");
                }
            } else {
                if (agent_auto_mode) {
                    agent_history_add("system", "File removed.");
                }
            }
        } else if (starts_with(tc, "ls ")) {
            printn("[AGENT] Executing: ls");
            if (agent_auto_mode) {
                int pipefd[2];
                int cap = capture_setup(pipefd);
                ls_dir(tc + 3);
                if (cap == 0) {
                    char out[4096];
                    capture_finish(pipefd, out, sizeof(out));
                    agent_history_add("system", out);
                }
            } else {
                ls_dir(tc + 3);
            }
        } else if (starts_with(tc, "cp ")) {
            printn("[AGENT] Executing: cp");
            char *cp_src = tc + 3;
            char *cp_dst = NULL;
            for (int i = 0; cp_src[i]; i++) {
                if (cp_src[i] == ' ') {
                    cp_src[i] = '\0';
                    cp_dst = &cp_src[i + 1];
                    break;
                }
            }
            if (cp_dst) {
                do_cp(cp_src, cp_dst);
                if (agent_auto_mode) {
                    agent_history_add("system", "File copied.");
                }
            }
        } else if (starts_with(tc, "mv ")) {
            printn("[AGENT] Executing: mv");
            char *mv_src = tc + 3;
            char *mv_dst = NULL;
            for (int i = 0; mv_src[i]; i++) {
                if (mv_src[i] == ' ') {
                    mv_src[i] = '\0';
                    mv_dst = &mv_src[i + 1];
                    break;
                }
            }
            if (mv_dst) {
                do_mv(mv_src, mv_dst);
                if (agent_auto_mode) {
                    agent_history_add("system", "File moved.");
                }
            }
        } else if (starts_with(tc, "chmod ")) {
            printn("[AGENT] Executing: chmod");
            char *cpath = tc + 6;
            char *cmode_str = NULL;
            for (int i = 0; cpath[i]; i++) {
                if (cpath[i] == ' ') {
                    cpath[i] = '\0';
                    cmode_str = &cpath[i + 1];
                    break;
                }
            }
            if (cmode_str) {
                int cmode = parse_octal(cmode_str);
                do_chmod(cpath, cmode);
                if (agent_auto_mode) {
                    agent_history_add("system", "Permissions changed.");
                }
            }
        }
    } else {
        printn("[AGENT] Could not parse AI response");
        printn(resp);
    }
}

static void do_sleep(int secs) {
    for (int i = 0; i < secs && !interrupted; i++) {
        struct { uint64_t sec; uint64_t nsec; } req = { 1, 0 };
        sys_nanosleep(&req, NULL);
    }
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
        // Child: reset SIGINT so Ctrl+C can interrupt us
        signal_default(SIGINT);
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
            // Try /data/bin/ prefix
            char datapath[128] = "/data/bin/";
            bp = datapath + 10;
            ap = argv[0];
            while (*ap && bp < datapath + sizeof(datapath) - 1) {
                *bp++ = *ap++;
            }
            *bp = '\0';
            sys_execve(datapath, argv, envp);
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

static void unescape(char *s) {
    int r = 0, w = 0;
    while (s[r]) {
        if (s[r] == '\\' && s[r + 1]) {
            if (s[r + 1] == 'n') { s[w++] = '\n'; r += 2; }
            else if (s[r + 1] == 't') { s[w++] = '\t'; r += 2; }
            else if (s[r + 1] == '\\') { s[w++] = '\\'; r += 2; }
            else if (s[r + 1] == 'r') { s[w++] = '\r'; r += 2; }
            else { s[w++] = s[r++]; }
        } else {
            s[w++] = s[r++];
        }
    }
    s[w] = '\0';
}

static void write_file(const char *path, char *content) {
    unescape(content);
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

static void append_file(const char *path, char *content) {
    unescape(content);
    int fd = sys_open(path, 0x441, 0644); // O_WRONLY|O_CREAT|O_APPEND
    if (fd < 0) {
        printn("append: cannot open file");
        return;
    }
    sys_write(fd, content, strlen_(content));
    sys_close(fd);
    print("Appended to ");
    printn(path);
}

static void do_replace(const char *path, const char *old, const char *new_) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("replace: cannot open file");
        return;
    }
    static char rbuf[65536];
    int n = sys_read(fd, rbuf, sizeof(rbuf) - 1);
    sys_close(fd);
    if (n < 0) {
        printn("replace: cannot read file");
        return;
    }
    rbuf[n] = '\0';

    int oldlen = strlen_(old);
    int newlen = strlen_(new_);
    if (oldlen == 0) {
        printn("replace: empty search string");
        return;
    }

    // Find first occurrence
    char *pos = NULL;
    for (int i = 0; i <= n - oldlen; i++) {
        int match = 1;
        for (int j = 0; j < oldlen; j++) {
            if (rbuf[i + j] != old[j]) {
                match = 0;
                break;
            }
        }
        if (match) {
            pos = rbuf + i;
            break;
        }
    }

    if (!pos) {
        printn("replace: pattern not found");
        return;
    }

    // Build new content: before + new + after
    int before_len = pos - rbuf;
    int after_len = n - before_len - oldlen;
    int total = before_len + newlen + after_len;
    if (total >= sizeof(rbuf)) {
        printn("replace: result too large");
        return;
    }

    // Shift after part to make room
    if (newlen != oldlen) {
        for (int i = after_len - 1; i >= 0; i--) {
            rbuf[before_len + newlen + i] = rbuf[before_len + oldlen + i];
        }
    }
    // Copy new string
    for (int i = 0; i < newlen; i++) {
        rbuf[before_len + i] = new_[i];
    }

    fd = sys_open(path, 0x241, 0644);
    if (fd < 0) {
        printn("replace: cannot write file");
        return;
    }
    sys_write(fd, rbuf, total);
    sys_close(fd);
    printn("replace: done");
}

static void do_sort(const char *path) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("sort: cannot open file");
        return;
    }
    static char sbuf[65536];
    static char *lines[1024];
    int n = sys_read(fd, sbuf, sizeof(sbuf) - 1);
    sys_close(fd);
    if (n < 0) {
        printn("sort: cannot read file");
        return;
    }
    sbuf[n] = '\0';

    int line_count = 0;
    lines[0] = sbuf;
    for (int i = 0; i < n && line_count < 1023; i++) {
        if (sbuf[i] == '\n') {
            sbuf[i] = '\0';
            line_count++;
            lines[line_count] = sbuf + i + 1;
        }
    }
    // Remove empty trailing line
    if (line_count > 0 && lines[line_count][0] == '\0') {
        line_count--;
    }

    // Bubble sort
    for (int i = 0; i < line_count - 1; i++) {
        for (int j = 0; j < line_count - i - 1; j++) {
            int cmp = 0;
            char *a = lines[j];
            char *b = lines[j + 1];
            while (*a && *b) {
                if (*a != *b) {
                    cmp = (unsigned char)*a - (unsigned char)*b;
                    break;
                }
                a++;
                b++;
            }
            if (cmp == 0) cmp = (unsigned char)*a - (unsigned char)*b;
            if (cmp > 0) {
                char *tmp = lines[j];
                lines[j] = lines[j + 1];
                lines[j + 1] = tmp;
            }
        }
    }

    for (int i = 0; i <= line_count; i++) {
        printn(lines[i]);
    }
}

static void do_uniq(const char *path) {
    int fd = sys_open(path, 0, 0);
    if (fd < 0) {
        printn("uniq: cannot open file");
        return;
    }
    static char ubuf[65536];
    static char *lines[1024];
    int n = sys_read(fd, ubuf, sizeof(ubuf) - 1);
    sys_close(fd);
    if (n < 0) {
        printn("uniq: cannot read file");
        return;
    }
    ubuf[n] = '\0';

    int line_count = 0;
    lines[0] = ubuf;
    for (int i = 0; i < n && line_count < 1023; i++) {
        if (ubuf[i] == '\n') {
            ubuf[i] = '\0';
            line_count++;
            lines[line_count] = ubuf + i + 1;
        }
    }
    if (line_count > 0 && lines[line_count][0] == '\0') {
        line_count--;
    }

    char *prev = NULL;
    for (int i = 0; i <= line_count; i++) {
        int same = 0;
        if (prev) {
            char *a = prev;
            char *b = lines[i];
            same = 1;
            while (*a || *b) {
                if (*a != *b) { same = 0; break; }
                a++;
                b++;
            }
        }
        if (!same) {
            printn(lines[i]);
            prev = lines[i];
        }
    }
}

// Minimal tar extractor. Supports regular files and directories.
// Format: 512-byte header blocks, then file data padded to 512 bytes.
static int octal_to_int(const char *s, int len) {
    int val = 0;
    for (int i = 0; i < len; i++) {
        if (s[i] >= '0' && s[i] <= '7') {
            val = val * 8 + (s[i] - '0');
        }
    }
    return val;
}

static void do_tar_extract(const char *tarfile) {
    int fd = sys_open(tarfile, 0, 0);
    if (fd < 0) {
        printn("tar: cannot open archive");
        return;
    }

    char header[512];
    int n;
    int files = 0;

    while ((n = sys_read(fd, header, 512)) == 512) {
        // Check for end-of-archive (two zero blocks)
        int all_zero = 1;
        for (int i = 0; i < 512; i++) {
            if (header[i] != 0) { all_zero = 0; break; }
        }
        if (all_zero) {
            // Read second zero block
            char zero[512];
            int z = sys_read(fd, zero, 512);
            if (z == 512) {
                int all_zero2 = 1;
                for (int i = 0; i < 512; i++) {
                    if (zero[i] != 0) { all_zero2 = 0; break; }
                }
                if (all_zero2) break;
            }
            // Not end of archive, continue
        }

        // Parse header
        char name[101];
        for (int i = 0; i < 100; i++) name[i] = header[i];
        name[100] = '\0';

        // Trim trailing spaces/nulls from name
        int name_len = 99;
        while (name_len >= 0 && (name[name_len] == ' ' || name[name_len] == '\0')) name_len--;
        name[name_len + 1] = '\0';

        if (name[0] == '\0') continue;

        int size = octal_to_int(header + 124, 12);
        char typeflag = header[156];

        if (typeflag == '5') {
            // Directory
            sys_mkdir(name, 0755);
            files++;
        } else if (typeflag == '0' || typeflag == '\0') {
            // Regular file
            int outfd = sys_open(name, 0x241, 0644);
            if (outfd < 0) {
                printn("tar: cannot create file");
                // Skip file data
                int blocks = (size + 511) / 512;
                for (int b = 0; b < blocks; b++) {
                    char skip[512];
                    sys_read(fd, skip, 512);
                }
                continue;
            }
            int remaining = size;
            while (remaining > 0) {
                char buf[512];
                int to_read = remaining > 512 ? 512 : remaining;
                int r = sys_read(fd, buf, 512);
                if (r <= 0) break;
                sys_write(outfd, buf, to_read);
                remaining -= to_read;
            }
            sys_close(outfd);
            files++;
        } else {
            // Unknown type, skip data
            int blocks = (size + 511) / 512;
            for (int b = 0; b < blocks; b++) {
                char skip[512];
                sys_read(fd, skip, 512);
            }
        }
    }

    sys_close(fd);
    print("tar: extracted ");
    print_int(files);
    printn(" entries");
}

static void agent_auto_loop(int max_iter, int interval_secs) {
    if (max_iter <= 0) max_iter = 10;
    if (interval_secs <= 0) interval_secs = 5;
    agent_auto_mode = 1;
    interrupted = 0;
    printn("\n[AGENT] Autonomous mode started.");
    print("[AGENT] Max iterations: "); print_int(max_iter); printn("");
    print("[AGENT] Interval: "); print_int(interval_secs); printn(" seconds");
    printn("[AGENT] Press Ctrl+C to stop.");

    char last_tool[64] = "";
    int repeat_count = 0;

    for (int iter = 0; iter < max_iter && !interrupted; iter++) {
        print("\n[AGENT] === Iteration "); print_int(iter + 1); print(" / "); print_int(max_iter); printn(" ===");

        char prompt[512];
        int p = 0;
        const char *s = "You are running autonomously on Nymoris OS. Decide what to do next. ";
        while (*s) prompt[p++] = *s++;
        s = "Available tools: run, exec, read, write, append, replace, find, grep, mkdir, rm, ls, cp, mv, chmod, http, post, sleep. ";
        while (*s) prompt[p++] = *s++;
        s = "Be concise. Execute one tool per response.";
        while (*s) prompt[p++] = *s++;
        prompt[p] = '\0';

        int prev_count = agent_history_count;
        ask_ai(prompt);

        // Loop detection: check if the last assistant message repeats
        if (agent_history_count > prev_count) {
            int idx = (agent_history_next - 1) % MAX_AGENT_HISTORY;
            if (strcmp_(agent_roles[idx], "assistant") == 0) {
                if (strcmp_(agent_msgs[idx], last_tool) == 0) {
                    repeat_count++;
                    if (repeat_count >= 2) {
                        printn("[AGENT] Detected repeating tool calls. Stopping auto mode.");
                        break;
                    }
                } else {
                    repeat_count = 0;
                    int i = 0;
                    while (agent_msgs[idx][i] && i < 63) {
                        last_tool[i] = agent_msgs[idx][i];
                        i++;
                    }
                    last_tool[i] = '\0';
                }
            }
        }

        if (iter < max_iter - 1 && !interrupted) {
            do_sleep(interval_secs);
        }
    }

    agent_auto_mode = 0;
    if (interrupted) {
        printn("\n[AGENT] Autonomous mode interrupted.");
    } else {
        printn("\n[AGENT] Autonomous mode completed.");
    }
}

static void agent_loop(void) {
    agent_load_config();
    agent_load_history("/data/agent.history");

    printn("\n[AGENT] AI Agent loop started.");
    printn("[AGENT] Commands: ask <prompt>, auto [n] [s], history, reset, save [path], load [path], config, exec <cmd>, run <cmd>, read <file>, write <file> <data>, append <file> <data>, replace <file> <old> <new>, find <dir> <name>, grep <p> <file>, mkdir <dir>, rm <file>, ls [dir], cp <src> <dst>, mv <src> <dst>, chmod <mode> <file>, http <host> [path], post <host> <path> <body>, sleep <secs>, done");
    const char *sp = env_get("NYMORIS_SYSTEM_PROMPT");
    if (sp && sp[0]) {
        printn("[AGENT] Custom system prompt active.");
    }

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
            agent_save_history("/data/agent.history");
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
        } else if (strcmp_(action, "save") == 0) {
            const char *path = arg ? arg : "/data/agent.history";
            agent_save_history(path);
        } else if (strcmp_(action, "load") == 0) {
            const char *path = arg ? arg : "/data/agent.history";
            agent_load_history(path);
        } else if (strcmp_(action, "config") == 0) {
            printn("[AGENT] Configuration:");
            print("  API_HOST:  "); printn(api_host);
            print("  API_PATH:  "); printn(api_path);
            print("  API_MODEL: "); printn(api_model);
            print("  API_KEY:   ");
            if (api_key[0]) {
                printn("***set***");
            } else {
                printn("(not set)");
            }
            const char *sp = env_get("NYMORIS_SYSTEM_PROMPT");
            print("  SYSTEM:    ");
            if (sp && sp[0]) {
                printn("custom");
            } else {
                printn("default");
            }
            printn("[AGENT] Set via env: NYMORIS_API_KEY, NYMORIS_API_HOST, NYMORIS_API_MODEL, NYMORIS_API_PATH, NYMORIS_SYSTEM_PROMPT");
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
        printn("  hexdump <file>    Hex dump file");
        printn("  base64 <file>     Base64 encode file");
        printn("  base64 -d <file>  Base64 decode file");
        printn("  grep <p> <file>   Search for pattern");
        printn("  wc <file>         Count lines/words/bytes");
        printn("  sort <file>       Sort lines alphabetically");
        printn("  uniq <file>       Remove duplicate adjacent lines");
        printn("  mkdir <dir>       Create directory");
        printn("  rmdir <dir>       Remove empty directory");
        printn("  echo <text>       Print text");
        printn("  pwd               Print working directory");
        printn("  cd <dir>          Change directory");
        printn("  rm <file>         Remove file");
        printn("  cp <src> <dst>    Copy file");
        printn("  mv <src> <dst>    Move/rename file");
        printn("  touch <file>      Create empty file");
        printn("  write <f> <d>    Write content to file");
        printn("  append <f> <d>   Append content to file");
        printn("  replace <f> <o> <n> Replace first occurrence in file");
        printn("  ping <host>       Ping host");
        printn("  wget <h> <p> <f>  Download file via HTTP");
        printn("  install <h> <p> <n> Download binary to /data/bin/");
        printn("  tar x <file>      Extract tar archive");
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
        printn("  mount <d> <p> <t> Mount block device");
        printn("  umount <dir>      Unmount filesystem");
        printn("  lsblk             List block devices");
        printn("  netstat           Show TCP connections");
        printn("  hostname          Show hostname");
        printn("  whoami            Show current user");
        printn("  id                Show user/group IDs");
        printn("  chmod <m> <f>    Change file permissions");
        printn("  stat <file>       Show file metadata");
        printn("  ln [-s] <t> <l>  Create hard/symbolic link");
        printn("  cmp <f1> <f2>     Compare two files");
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
    } else if (starts_with(linebuf, "mount ")) {
        char *rest = linebuf + 6;
        while (*rest == ' ') rest++;
        char *device = rest;
        char *dir = NULL;
        char *fstype = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                dir = &rest[i + 1];
                break;
            }
        }
        if (dir) {
            while (*dir == ' ') dir++;
            for (int i = 0; dir[i]; i++) {
                if (dir[i] == ' ') {
                    dir[i] = '\0';
                    fstype = &dir[i + 1];
                    break;
                }
            }
        }
        if (fstype) {
            while (*fstype == ' ') fstype++;
            // Remove trailing spaces from fstype
            int ftlen = 0;
            while (fstype[ftlen]) ftlen++;
            while (ftlen > 0 && fstype[ftlen - 1] == ' ') ftlen--;
            fstype[ftlen] = '\0';
            int ret = sys_mount(device, dir, fstype, 0, NULL);
            if (ret == 0) {
                print("mounted ");
                print(device);
                print(" on ");
                printn(dir);
            } else {
                print("mount failed: ");
                print_int(ret);
                printn("");
            }
        } else {
            printn("Usage: mount <device> <dir> <fstype>");
        }
    } else if (strcmp_(linebuf, "lsblk") == 0) {
        printn("Block devices:");
        printn("MAJOR MINOR  BLOCKS  NAME");
        cat_file("/proc/partitions");
    } else if (starts_with(linebuf, "umount ")) {
        char *rest = linebuf + 7;
        while (*rest == ' ') rest++;
        int ret = sys_umount2(rest, 0);
        if (ret == 0) {
            print("umounted ");
            printn(rest);
        } else {
            print("umount failed: ");
            print_int(ret);
            printn("");
        }
    } else if (starts_with(linebuf, "hexdump ")) {
        char *path = linebuf + 8;
        while (*path == ' ') path++;
        do_hexdump(path);
    } else if (starts_with(linebuf, "base64 ")) {
        char *rest = linebuf + 7;
        while (*rest == ' ') rest++;
        if (starts_with(rest, "-d ")) {
            char *path = rest + 3;
            while (*path == ' ') path++;
            do_base64_dec(path);
        } else {
            do_base64_enc(rest);
        }
    } else if (strcmp_(linebuf, "netstat") == 0) {
        cat_file("/proc/net/tcp");
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
    } else if (starts_with(linebuf, "stat ")) {
        char *path = linebuf + 5;
        while (*path == ' ') path++;
        do_stat(path);
    } else if (starts_with(linebuf, "ln ")) {
        char *rest = linebuf + 3;
        while (*rest == ' ') rest++;
        int symlink_ = 0;
        if (starts_with(rest, "-s ")) {
            symlink_ = 1;
            rest += 3;
            while (*rest == ' ') rest++;
        }
        char *target = rest;
        char *linkpath = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                linkpath = &rest[i + 1];
                break;
            }
        }
        if (linkpath) {
            while (*linkpath == ' ') linkpath++;
            do_ln(target, linkpath, symlink_);
        } else {
            printn("Usage: ln [-s] <target> <link>");
        }
    } else if (starts_with(linebuf, "cmp ")) {
        char *rest = linebuf + 4;
        while (*rest == ' ') rest++;
        char *path1 = rest;
        char *path2 = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                path2 = &rest[i + 1];
                break;
            }
        }
        if (path2) {
            while (*path2 == ' ') path2++;
            do_cmp(path1, path2);
        } else {
            printn("Usage: cmp <file1> <file2>");
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
    } else if (starts_with(linebuf, "sort ")) {
        char *path = linebuf + 5;
        while (*path == ' ') path++;
        do_sort(path);
    } else if (starts_with(linebuf, "uniq ")) {
        char *path = linebuf + 5;
        while (*path == ' ') path++;
        do_uniq(path);
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
            do_cp(src, dst);
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
            do_mv(src, dst);
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
    } else if (starts_with(linebuf, "write ")) {
        char *rest = linebuf + 6;
        while (*rest == ' ') rest++;
        char *path = rest;
        char *content = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                content = &rest[i + 1];
                break;
            }
        }
        if (content) {
            write_file(path, content);
        } else {
            printn("Usage: write <file> <content>");
        }
    } else if (starts_with(linebuf, "append ")) {
        char *rest = linebuf + 7;
        while (*rest == ' ') rest++;
        char *path = rest;
        char *content = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                content = &rest[i + 1];
                break;
            }
        }
        if (content) {
            append_file(path, content);
        } else {
            printn("Usage: append <file> <content>");
        }
    } else if (starts_with(linebuf, "replace ")) {
        char *rest = linebuf + 8;
        while (*rest == ' ') rest++;
        char *path = rest;
        char *old = NULL;
        char *new_ = NULL;
        for (int i = 0; rest[i]; i++) {
            if (rest[i] == ' ') {
                rest[i] = '\0';
                old = &rest[i + 1];
                break;
            }
        }
        if (old) {
            while (*old == ' ') old++;
            for (int i = 0; old[i]; i++) {
                if (old[i] == ' ') {
                    old[i] = '\0';
                    new_ = &old[i + 1];
                    break;
                }
            }
        }
        if (new_) {
            while (*new_ == ' ') new_++;
            do_replace(path, old, new_);
        } else {
            printn("Usage: replace <file> <old> <new>");
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
    } else if (starts_with(linebuf, "install ")) {
        char *rest = linebuf + 8;
        while (*rest == ' ') rest++;
        char *host = rest;
        char *path = NULL;
        char *name = NULL;
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
                    name = &path[i + 1];
                    break;
                }
            }
        }
        if (host && path && name) {
            char outpath[128] = "/data/bin/";
            char *op = outpath + 10;
            char *np = name;
            while (*np && op < outpath + sizeof(outpath) - 1) {
                *op++ = *np++;
            }
            *op = '\0';
            do_wget(host, path, outpath);
            sys_chmod(outpath, 0755);
            printn("install: made executable");
        } else {
            printn("Usage: install <host> <path> <name>");
        }
    } else if (starts_with(linebuf, "tar x ")) {
        do_tar_extract(linebuf + 6);
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
        int pid = sys_fork();
        if (pid == 0) {
            // Isolate agent in its own mount and PID namespace
            if (sys_unshare(CLONE_NEWNS | CLONE_NEWPID) == 0) {
                // In the new mount namespace, re-mount /tmp and /data
                // so agent changes are isolated from the host
                sys_mount("tmpfs", "/tmp", "tmpfs", 0, NULL);
                sys_mount("tmpfs", "/data", "tmpfs", 0, NULL);
                sys_mkdir("/data/bin", 0755);
            }
            agent_loop();
            sys_exit(0);
        } else if (pid > 0) {
            int status;
            sys_wait4(pid, &status, 0, NULL);
        } else {
            printn("agent: fork failed");
        }
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
    // Clear screen and move cursor to home
    print("\x1b[2J\x1b[H");
    printn("");
    printn("========================================");
    printn("  Nymoris Agentic AI Operating System");
    printn("========================================");
    printn("");

    while (1) {
        interrupted = 0;
        jobs_reap();
        reap_stray_children();

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

        // Split line into commands separated by ';'
        char cmd_segments[8][512];
        int seg_count = 0;
        char *seg_start = linebuf;
        while (*seg_start == ' ') seg_start++;

        while (*seg_start && seg_count < 8) {
            char *seg_end = seg_start;
            while (*seg_end && *seg_end != ';') seg_end++;

            int seg_len = seg_end - seg_start;
            while (seg_len > 0 && seg_start[seg_len - 1] == ' ') seg_len--;

            if (seg_len > 0 && seg_len < 512) {
                for (int i = 0; i < seg_len; i++) {
                    cmd_segments[seg_count][i] = seg_start[i];
                }
                cmd_segments[seg_count][seg_len] = '\0';
                seg_count++;
            }

            if (*seg_end == ';') {
                seg_start = seg_end + 1;
                while (*seg_start == ' ') seg_start++;
            } else {
                break;
            }
        }

        // Execute each segment sequentially
        for (int seg = 0; seg < seg_count; seg++) {
            // Copy segment into linebuf for dispatch_command
            int i = 0;
            while (cmd_segments[seg][i] && i < sizeof(linebuf) - 1) {
                linebuf[i] = cmd_segments[seg][i];
                i++;
            }
            linebuf[i] = '\0';
            linepos = i;

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
                sys_dup2(1, saved_stdout);
                sys_dup2(fd, 1);
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
                sys_dup2(0, saved_stdin);
                sys_dup2(fd, 0);
                sys_close(fd);
            }

            // Fork for background execution
            if (bg) {
                int pid = sys_fork();
                if (pid > 0) {
                    jobs_add(pid, linebuf);
                    if (saved_stdout >= 0) {
                        sys_dup2(saved_stdout, 1);
                        sys_close(saved_stdout);
                    }
                    if (saved_stdin >= 0) {
                        sys_dup2(saved_stdin, 0);
                        sys_close(saved_stdin);
                    }
                    continue;
                } else if (pid < 0) {
                    printn("fork failed");
                    if (saved_stdout >= 0) {
                        sys_dup2(saved_stdout, 1);
                        sys_close(saved_stdout);
                    }
                    if (saved_stdin >= 0) {
                        sys_dup2(saved_stdin, 0);
                        sys_close(saved_stdin);
                    }
                    continue;
                }
                // Child: reset SIGINT so Ctrl+C can interrupt us
                signal_default(SIGINT);
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
                sys_dup2(saved_stdout, 1);
                sys_close(saved_stdout);
            }
            if (saved_stdin >= 0) {
                sys_dup2(saved_stdin, 0);
                sys_close(saved_stdin);
            }
        }
    }
}

static void try_automount(void) {
    // Try to mount persistent storage from common block devices
    const char *devices[] = {"/dev/sda1", "/dev/vda1", "/dev/hda1", "/dev/nvme0n1p1"};
    const char *fstypes[] = {"ext4", "ext2", "vfat"};

    for (int d = 0; d < 4; d++) {
        int fd = sys_open(devices[d], 0, 0);
        if (fd >= 0) {
            sys_close(fd);
            for (int f = 0; f < 3; f++) {
                // Unmount tmpfs /data first, then mount the block device
                sys_umount2("/data", 0);
                int ret = sys_mount(devices[d], "/data", fstypes[f], 0, NULL);
                if (ret == 0) {
                    sys_mkdir("/data/bin", 0755);
                    print("[Nymoris] mounted persistent storage: ");
                    print(devices[d]);
                    print(" (");
                    print(fstypes[f]);
                    printn(") on /data");
                    return;
                }
            }
        }
    }
    // No block device found, /data stays as tmpfs
    printn("[Nymoris] no persistent storage found, /data is tmpfs");
}

void _start(void) {
    // Align stack to 16 bytes for SSE compatibility
    asm volatile("andq $-16, %%rsp" ::: "memory");

    // Mount basic filesystems first so /dev/console exists
    sys_mkdir("/proc", 0755);
    sys_mkdir("/sys", 0755);
    sys_mkdir("/dev", 0755);
    sys_mkdir("/tmp", 0755);
    sys_mkdir("/data", 0755);
    sys_mkdir("/data/bin", 0755);
    sys_mount("proc", "/proc", "proc", 0, NULL);
    sys_mount("sysfs", "/sys", "sysfs", 0, NULL);
    sys_mount("devtmpfs", "/dev", "devtmpfs", 0, NULL);
    sys_mount("tmpfs", "/tmp", "tmpfs", 0, NULL);
    sys_mount("tmpfs", "/data", "tmpfs", 0, NULL);

    // Open /dev/console for stdin/stdout/stderr
    int fd = sys_open("/dev/console", 2, 0); // O_RDWR
    if (fd >= 0) {
        if (fd != 0) {
            sys_dup2(fd, 0);
        }
        if (fd != 1) {
            sys_dup2(fd, 1);
        }
        if (fd != 2) {
            sys_dup2(fd, 2);
        }
        if (fd > 2) {
            sys_close(fd);
        }
    }

    // Catch SIGINT so we can interrupt long-running commands in PID 1.
    // Ignore SIGTERM — we handle shutdown via explicit commands.
    signal_catch(SIGINT);
    signal_ignore(SIGTERM);

    printn("[Nymoris] init started");

    // Try to mount persistent storage
    try_automount();

    // Source startup scripts if they exist
    int rc_fd = sys_open("/data/nymoris.rc", 0, 0);
    if (rc_fd >= 0) {
        sys_close(rc_fd);
        printn("[Nymoris] sourcing /data/nymoris.rc");
        source_file("/data/nymoris.rc");
    }
    rc_fd = sys_open("/nymoris.rc", 0, 0);
    if (rc_fd >= 0) {
        sys_close(rc_fd);
        printn("[Nymoris] sourcing /nymoris.rc");
        source_file("/nymoris.rc");
    }

    shell_loop();

    sys_exit(0);
}
