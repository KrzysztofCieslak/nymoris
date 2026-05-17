#!/usr/bin/env python3
"""Test API server for Nymoris agent.

Run this on the host machine before starting QEMU.
The agent connects to 10.0.2.2:8765 (QEMU gateway = host).
"""

import json
import socket
import threading

HOST = "0.0.0.0"
PORT = 8765

# Simple command queue for the agent
COMMANDS = [
    "echo Hello from the AI agent!",
    "about",
    "meminfo",
    "ping 10.0.2.2",
    "none",  # no action
]

cmd_index = 0


def handle_request(client: socket.socket, addr: tuple):
    global cmd_index
    try:
        data = b""
        while b"\r\n\r\n" not in data:
            chunk = client.recv(4096)
            if not chunk:
                break
            data += chunk

        if not data:
            client.close()
            return

        # Check for Content-Length and read body
        headers, _, body = data.partition(b"\r\n\r\n")
        content_length = 0
        for line in headers.split(b"\r\n"):
            if line.lower().startswith(b"content-length:"):
                content_length = int(line.split(b":", 1)[1].strip())
                break

        while len(body) < content_length:
            chunk = client.recv(4096)
            if not chunk:
                break
            body += chunk

        request_body = body.decode("utf-8", errors="replace")
        print(f"[{addr[0]}:{addr[1]}] Request body: {request_body[:200]}")

        # Pick next command
        command = COMMANDS[cmd_index % len(COMMANDS)]
        cmd_index += 1

        if command == "none":
            content = "none"
        else:
            content = f"COMMAND: {command}"

        response_body = json.dumps({"content": content})

        response = (
            "HTTP/1.1 200 OK\r\n"
            "Content-Type: application/json\r\n"
            f"Content-Length: {len(response_body)}\r\n"
            "Connection: close\r\n"
            "\r\n"
            f"{response_body}"
        )

        client.sendall(response.encode())
        print(f"[{addr[0]}:{addr[1]}] Sent: {response_body}")

    except Exception as e:
        print(f"[{addr[0]}:{addr[1]}] Error: {e}")
    finally:
        client.close()


def main():
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    server.bind((HOST, PORT))
    server.listen(5)

    print(f"Nymoris test API server listening on {HOST}:{PORT}")
    print("Press Ctrl+C to stop\n")

    try:
        while True:
            client, addr = server.accept()
            threading.Thread(target=handle_request, args=(client, addr), daemon=True).start()
    except KeyboardInterrupt:
        print("\nShutting down...")
    finally:
        server.close()


if __name__ == "__main__":
    main()
