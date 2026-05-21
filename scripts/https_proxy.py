#!/usr/bin/env python3
"""
Nymoris HTTPS Proxy

Translates HTTP requests from nymoris (which has no TLS) to HTTPS
requests to AI API endpoints (OpenAI, Anthropic, etc.).

Usage:
    python3 scripts/https_proxy.py --listen 8080 --target api.openai.com:443

Then in nymoris:
    export NYMORIS_API_KEY=sk-...
    export NYMORIS_API_HOST=10.0.2.2
    export NYMORIS_API_PATH=/v1/chat/completions
    agent
    ask hello
"""

import argparse
import http.server
import socketserver
import ssl
import sys
import urllib.parse


class ProxyHandler(http.server.BaseHTTPRequestHandler):
    target_host = None
    target_port = 443

    def log_message(self, format, *args):
        print(f"[proxy] {self.client_address[0]} - {format % args}")

    def do_GET(self):
        self._proxy("GET")

    def do_POST(self):
        self._proxy("POST")

    def do_OPTIONS(self):
        self._proxy("OPTIONS")

    def _proxy(self, method):
        content_length = int(self.headers.get("Content-Length", 0))
        body = self.rfile.read(content_length) if content_length > 0 else b""

        import http.client

        conn = http.client.HTTPSConnection(self.target_host, self.target_port)

        # Forward headers, filtering out hop-by-hop ones
        hop_by_hop = {"host", "connection", "keep-alive", "proxy-authenticate",
                      "proxy-authorization", "te", "trailers", "transfer-encoding", "upgrade"}
        headers = {}
        for key, value in self.headers.items():
            if key.lower() not in hop_by_hop:
                headers[key] = value
        headers["Host"] = self.target_host

        try:
            conn.request(method, self.path, body=body, headers=headers)
            resp = conn.getresponse()

            self.send_response(resp.status)
            for key, value in resp.getheaders():
                if key.lower() not in hop_by_hop:
                    self.send_header(key, value)
            self.end_headers()
            self.wfile.write(resp.read())
        except Exception as e:
            self.send_response(502)
            self.send_header("Content-Type", "text/plain")
            self.end_headers()
            self.wfile.write(f"Proxy error: {e}".encode())
        finally:
            conn.close()


def main():
    parser = argparse.ArgumentParser(description="HTTP-to-HTTPS proxy for nymoris")
    parser.add_argument("--listen", type=int, default=8080, help="Local port to listen on")
    parser.add_argument("--target", required=True, help="Target HTTPS host:port (e.g., api.openai.com:443)")
    args = parser.parse_args()

    target = args.target
    if ":" in target:
        host, port_str = target.rsplit(":", 1)
        port = int(port_str)
    else:
        host = target
        port = 443

    ProxyHandler.target_host = host
    ProxyHandler.target_port = port

    with socketserver.TCPServer(("", args.listen), ProxyHandler) as httpd:
        print(f"[proxy] Listening on http://localhost:{args.listen}")
        print(f"[proxy] Forwarding to https://{host}:{port}")
        print(f"[proxy] Press Ctrl+C to stop")
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[proxy] Shutting down")


if __name__ == "__main__":
    main()
