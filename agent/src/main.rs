use std::io::{self, BufRead, Read, Write};
use std::net::TcpStream;
use std::process::{Command, Stdio};
use std::thread;
use std::time::Duration;

fn main() {
    mount_fs();
    setup_stdio();
    setup_loopback();
    spawn_zombie_reaper();

    println!("[KACOS] Agentic AI Operating System init starting...");
    println!("[KACOS] System ready. Starting agent shell.");
    println!("Type 'help' for commands, 'agent' to start AI agent loop.\n");

    shell_loop();
}

fn setup_stdio() {
    unsafe {
        // Ensure /dev/console exists. In initramfs it may not exist yet.
        if libc::access(c"/dev/console".as_ptr(), libc::F_OK) != 0 {
            let _ = libc::mknod(
                c"/dev/console".as_ptr(),
                libc::S_IFCHR | 0o660,
                libc::makedev(5, 1),
            );
        }

        let fd = libc::open(c"/dev/console".as_ptr(), libc::O_RDWR);
        if fd < 0 {
            libc::exit(1);
        }
        libc::dup2(fd, 0);
        libc::dup2(fd, 1);
        libc::dup2(fd, 2);
        if fd > 2 {
            libc::close(fd);
        }
    }
}

fn mount_fs() {
    let _ = std::fs::create_dir_all("/proc");
    let _ = std::fs::create_dir_all("/sys");
    let _ = std::fs::create_dir_all("/dev");
    let _ = std::fs::create_dir_all("/tmp");

    unsafe {
        libc::mount(
            c"proc".as_ptr(),
            c"/proc".as_ptr(),
            c"proc".as_ptr(),
            0,
            std::ptr::null(),
        );
        libc::mount(
            c"sysfs".as_ptr(),
            c"/sys".as_ptr(),
            c"sysfs".as_ptr(),
            0,
            std::ptr::null(),
        );
        libc::mount(
            c"devtmpfs".as_ptr(),
            c"/dev".as_ptr(),
            c"devtmpfs".as_ptr(),
            0,
            std::ptr::null(),
        );
        libc::mount(
            c"tmpfs".as_ptr(),
            c"/tmp".as_ptr(),
            c"tmpfs".as_ptr(),
            0,
            std::ptr::null(),
        );
    }
}

fn setup_loopback() {
    unsafe {
        let sock = libc::socket(libc::AF_INET, libc::SOCK_DGRAM, 0);
        if sock >= 0 {
            let mut ifr: libc::ifreq = std::mem::zeroed();
            c"lo".as_ptr().copy_to_nonoverlapping(
                ifr.ifr_name.as_mut_ptr(),
                2,
            );

            libc::ioctl(sock, libc::SIOCGIFFLAGS as libc::c_int, &mut ifr);
            ifr.ifr_ifru.ifru_flags |= (libc::IFF_UP | libc::IFF_RUNNING) as i16;
            libc::ioctl(sock, libc::SIOCSIFFLAGS as libc::c_int, &mut ifr);
            libc::close(sock);
        }
    }
}

fn spawn_zombie_reaper() {
    unsafe {
        let mut sa: libc::sigaction = std::mem::zeroed();
        sa.sa_sigaction = SIG_IGN;
        libc::sigaction(libc::SIGCHLD, &sa, std::ptr::null_mut());
    }
}

const SIG_IGN: usize = 1;

fn shell_loop() {
    let stdin = io::stdin();
    let mut stdout = io::stdout();

    loop {
        print!("$ ");
        let _ = stdout.flush();

        let mut line = String::new();
        if stdin.lock().read_line(&mut line).unwrap_or(0) == 0 {
            break;
        }

        let trimmed = line.trim();
        if trimmed.is_empty() {
            continue;
        }

        let parts: Vec<&str> = trimmed.split_whitespace().collect();
        let cmd = parts[0];
        let args = &parts[1..];

        match cmd {
            "help" => print_help(),
            "exit" | "quit" => {
                println!("[KACOS] Shutting down...");
                unsafe {
                    libc::reboot(libc::LINUX_REBOOT_CMD_POWER_OFF);
                }
            }
            "reboot" => {
                unsafe {
                    libc::reboot(libc::LINUX_REBOOT_CMD_RESTART);
                }
            }
            "ps" => cmd_ps(),
            "cat" => {
                if args.is_empty() {
                    println!("Usage: cat <file>");
                } else {
                    cmd_cat(args[0]);
                }
            }
            "ls" => {
                let path = args.first().unwrap_or(&".");
                cmd_ls(path);
            }
            "mkdir" => {
                if args.is_empty() {
                    println!("Usage: mkdir <dir>");
                } else {
                    let _ = std::fs::create_dir_all(args[0]);
                }
            }
            "echo" => println!("{}", args.join(" ")),
            "agent" => agent_loop(),
            "http" => {
                if args.len() < 1 {
                    println!("Usage: http <host> [path]");
                } else {
                    let host = args[0];
                    let path = args.get(1).unwrap_or(&"/");
                    cmd_http_get(host, path);
                }
            }
            _ => {
                let mut command = Command::new(cmd);
                command.args(args);
                command.stdout(Stdio::inherit());
                command.stderr(Stdio::inherit());
                command.stdin(Stdio::inherit());
                let _ = command.status();
            }
        }
    }
}

fn print_help() {
    println!("KACOS Agent Shell Commands:");
    println!("  help       Show this help");
    println!("  ps         List processes");
    println!("  cat <file> Show file contents");
    println!("  ls [dir]   List directory");
    println!("  mkdir <d>  Create directory");
    println!("  echo <txt> Print text");
    println!("  http <h>   Simple HTTP GET");
    println!("  agent      Start AI agent loop");
    println!("  reboot     Reboot system");
    println!("  exit       Power off");
    println!("  <cmd>      Run any binary in PATH");
}

fn cmd_ps() {
    if let Ok(entries) = std::fs::read_dir("/proc") {
        println!("{:>6} {:>6} {}", "PID", "PPID", "CMD");
        for entry in entries.flatten() {
            let name = entry.file_name();
            let name_str = name.to_string_lossy();
            if name_str.parse::<u64>().is_ok() {
                let status_file = format!("/proc/{}/stat", name_str);
                if let Ok(content) = std::fs::read_to_string(&status_file) {
                    let fields: Vec<&str> = content.split(' ').collect();
                    if fields.len() > 3 {
                        let cmd = fields[1].trim_start_matches('(').trim_end_matches(')');
                        println!("{:>6} {:>6} {}", fields[0], fields[3], cmd);
                    }
                }
            }
        }
    }
}

fn cmd_cat(path: &str) {
    match std::fs::read_to_string(path) {
        Ok(content) => print!("{}", content),
        Err(e) => println!("cat: {}", e),
    }
}

fn cmd_ls(path: &str) {
    match std::fs::read_dir(path) {
        Ok(entries) => {
            for entry in entries.flatten() {
                let meta = entry.metadata();
                let prefix = match meta {
                    Ok(m) if m.is_dir() => "d",
                    Ok(m) if m.is_symlink() => "l",
                    _ => "-",
                };
                println!("{} {}", prefix, entry.file_name().to_string_lossy());
            }
        }
        Err(e) => println!("ls: {}", e),
    }
}

fn cmd_http_get(host: &str, path: &str) {
    let port = 80;
    let addr = format!("{}:{}", host, port);

    match TcpStream::connect(&addr) {
        Ok(mut stream) => {
            let request = format!(
                "GET {} HTTP/1.1\r\nHost: {}\r\nConnection: close\r\n\r\n",
                path, host
            );
            let _ = stream.write_all(request.as_bytes());
            let _ = stream.flush();

            let mut response = String::new();
            let mut buf_reader = std::io::BufReader::new(&stream);
            let mut line = String::new();

            while buf_reader.read_line(&mut line).unwrap_or(0) > 0 {
                if line == "\r\n" {
                    break;
                }
                response.push_str(&line);
                line.clear();
            }

            let mut body = String::new();
            let _ = buf_reader.read_to_string(&mut body);

            println!("{}", response);
            println!("{}", body);
        }
        Err(e) => println!("http: connection failed: {}", e),
    }
}

fn agent_loop() {
    println!("[AGENT] AI Agent loop started.");
    println!("[AGENT] Commands: run <cmd>, read <file>, write <file> <data>, done");

    let stdin = io::stdin();
    let mut stdout = io::stdout();

    loop {
        print!("[AGENT] > ");
        let _ = stdout.flush();

        let mut line = String::new();
        if stdin.lock().read_line(&mut line).unwrap_or(0) == 0 {
            break;
        }

        let trimmed = line.trim();
        if trimmed.is_empty() {
            continue;
        }

        let parts: Vec<&str> = trimmed.splitn(2, ' ').collect();
        let action = parts[0];
        let arg = parts.get(1).unwrap_or(&"").trim();

        match action {
            "done" | "quit" => {
                println!("[AGENT] Exiting agent loop.");
                break;
            }
            "run" => {
                let output = Command::new("sh")
                    .arg("-c")
                    .arg(arg)
                    .output();
                match output {
                    Ok(out) => {
                        println!(
                            "[AGENT] stdout:\n{}",
                            String::from_utf8_lossy(&out.stdout)
                        );
                        if !out.stderr.is_empty() {
                            println!(
                                "[AGENT] stderr:\n{}",
                                String::from_utf8_lossy(&out.stderr)
                            );
                        }
                    }
                    Err(e) => println!("[AGENT] Failed to run: {}", e),
                }
            }
            "read" => cmd_cat(arg),
            "write" => {
                let write_parts: Vec<&str> = arg.splitn(2, ' ').collect();
                if write_parts.len() == 2 {
                    match std::fs::write(write_parts[0], write_parts[1]) {
                        Ok(_) => println!("[AGENT] Written to {}", write_parts[0]),
                        Err(e) => println!("[AGENT] Write failed: {}", e),
                    }
                } else {
                    println!("[AGENT] Usage: write <file> <content>");
                }
            }
            "http" => {
                let url_parts: Vec<&str> = arg.splitn(2, ' ').collect();
                let host = url_parts[0];
                let path = url_parts.get(1).unwrap_or(&"/");
                cmd_http_get(host, path);
            }
            "sleep" => {
                if let Ok(secs) = arg.parse::<u64>() {
                    thread::sleep(Duration::from_secs(secs));
                }
            }
            _ => println!("[AGENT] Unknown action: {}", action),
        }
    }
}
