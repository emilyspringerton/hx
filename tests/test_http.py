#!/usr/bin/env python3
import argparse
import socket

def read_response(sock):
    sock.settimeout(2.0)
    data = b""
    while b"\r\n\r\n" not in data:
        chunk = sock.recv(4096)
        if not chunk:
            break
        data += chunk
    return data


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--host", required=True)
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--path", required=True)
    parser.add_argument("--expect-status", type=int, required=True)
    args = parser.parse_args()

    with socket.create_connection((args.host, args.port), timeout=2.0) as sock:
        request = (
            f"GET {args.path} HTTP/1.1\r\n"
            f"Host: {args.host}:{args.port}\r\n"
            "Connection: close\r\n"
            "\r\n"
        )
        sock.sendall(request.encode("utf-8"))
        response = read_response(sock).decode("utf-8", errors="replace")

    if not response.startswith("HTTP/1.1"):
        raise SystemExit(f"invalid HTTP response: {response.splitlines()[:1]}")

    status_line = response.splitlines()[0]
    try:
        status = int(status_line.split()[1])
    except (IndexError, ValueError) as exc:
        raise SystemExit(f"unable to parse status from '{status_line}'") from exc

    if status != args.expect_status:
        raise SystemExit(f"expected status {args.expect_status}, got {status}")

if __name__ == "__main__":
    main()
