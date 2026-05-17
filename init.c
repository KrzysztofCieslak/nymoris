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
#define SYS_getdents64 217
#define SYS_nanosleep 35
#define SYS_wait4    61
#define SYS_fork     57
#define SYS_execve   59

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

static void printn(const char *s) {
    print(s);
    print("\n");
}

static void print_int(int n) {
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

struct linux_dirent64 {
    uint64_t d_ino;
    int64_t d_off;
    unsigned short d_reclen;
    unsigned char d_type;
    char d_name[];
};

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

static void ask_ai(const char *prompt) {
    char body[2048];
    int bl = 0;
    const char *p = "{\"model\":\"gpt-3.5-turbo\",\"messages\":[{\"role\":\"system\",\"content\":\"You are an AI agent running inside Nymoris OS. You can use these tools: run <shell_command>, read <file>, write <file> <content>, http <host> [path]. Respond with the tool call only, no explanation.\"},{\"role\":\"user\",\"content\":\"";
    while (*p) body[bl++] = *p++;
    // Escape the prompt for JSON
    for (int i = 0; prompt[i]; i++) {
        if (prompt[i] == '"' || prompt[i] == '\\') body[bl++] = '\\';
        body[bl++] = prompt[i];
    }
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

        // Auto-execute if it looks like a tool call
        if (starts_with(content, "run ")) {
            printn("[AGENT] Executing: run");
            run_command(content + 4);
        } else if (starts_with(content, "read ")) {
            printn("[AGENT] Executing: read");
            cat_file(content + 5);
        } else if (starts_with(content, "write ")) {
            printn("[AGENT] Executing: write");
            // Parse path and content
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

static void run_command(const char *cmd) {
    int pid = sys_fork();
    if (pid == 0) {
        // Child
        char *argv[4];
        argv[0] = "/bin/sh";
        argv[1] = "-c";
        argv[2] = (char *)cmd;
        argv[3] = NULL;
        char *envp[1] = {NULL};
        sys_execve("/bin/sh", argv, envp);
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

static void agent_loop(void) {
    printn("\n[AGENT] AI Agent loop started.");
    printn("[AGENT] Commands: ask <prompt>, run <cmd>, read <file>, write <file> <data>, http <host> [path], sleep <secs>, done");

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
        } else if (strcmp_(action, "run") == 0) {
            if (arg) {
                run_command(arg);
            } else {
                printn("[AGENT] Usage: run <command>");
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
            printn("  help              Show this help");
            printn("  ls [dir]          List directory");
            printn("  cat <file>        Show file contents");
            printn("  mkdir <dir>       Create directory");
            printn("  echo <text>       Print text");
            printn("  http <host> [p]   HTTP GET");
            printn("  sleep <secs>      Sleep");
            printn("  agent             Start AI agent loop");
            printn("  llm <m> <p>      Run local LLM inference");
            printn("  reboot            Reboot system");
            printn("  exit              Power off");
        } else if (strcmp_(linebuf, "ls") == 0) {
            ls_dir(".");
        } else if (starts_with(linebuf, "ls ")) {
            ls_dir(linebuf + 3);
        } else if (strcmp_(linebuf, "ps") == 0) {
            printn("PID    PPID   CMD");
            printn("(ps not fully implemented)");
        } else if (strcmp_(linebuf, "reboot") == 0) {
            do_reboot();
        } else if (strcmp_(linebuf, "exit") == 0 || strcmp_(linebuf, "quit") == 0) {
            do_poweroff();
        } else if (starts_with(linebuf, "cat ")) {
            cat_file(linebuf + 4);
        } else if (starts_with(linebuf, "mkdir ")) {
            if (sys_mkdir(linebuf + 6, 0755) < 0) {
                printn("mkdir: failed");
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
