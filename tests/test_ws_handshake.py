#!/usr/bin/env python3
import argparse
import base64
import hashlib
import os
import socket

def parse_headers(raw: str):
    lines = raw.split("\r\n")
    status = lines[0]
    headers = {}
    for line in lines[1:]:
        if not line:
            break
        if ":" not in line:
            continue
        key, value = line.split(":", 1)
        headers[key.strip().lower()] = value.strip()
    return status, headers


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--path", required=True)
    args = parser.parse_args()

    key = base64.b64encode(os.urandom(16)).decode("ascii")
    accept_seed = key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
    expected_accept = base64.b64encode(hashlib.sha1(accept_seed.encode()).digest()).decode("ascii")

    request = (
        f"GET {args.path} HTTP/1.1\r\n"
        f"Host: {args.host}:{args.port}\r\n"
        "Upgrade: websocket\r\n"
        "Connection: Upgrade\r\n"
        f"Sec-WebSocket-Key: {key}\r\n"
        "Sec-WebSocket-Version: 13\r\n"
        "\r\n"
    )

    with socket.create_connection((args.host, args.port), timeout=2.0) as sock:
        sock.sendall(request.encode("utf-8"))
        sock.settimeout(2.0)
        data = b""
        while b"\r\n\r\n" not in data:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data += chunk

    response = data.decode("utf-8", errors="replace")
    status, headers = parse_headers(response)

    if "101" not in status:
        raise SystemExit(f"expected 101 response, got '{status}'")

    accept = headers.get("sec-websocket-accept")
    if accept != expected_accept:
        raise SystemExit(
            f"invalid Sec-WebSocket-Accept: expected {expected_accept} got {accept}"
        )

if __name__ == "__main__":
    main()
