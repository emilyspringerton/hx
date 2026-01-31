# HX correctness tests

These tests exercise the HX correctness checklist using the same shell-driven harness style used by the Shankpit server.

## Requirements

- A runnable HX server binary or start command.
- Python 3 available on PATH (used for HTTP/WebSocket checks).

## Usage

```sh
HX_SERVER_BIN=./hx-server ./tests/run.sh
```

If your server needs flags or environment configuration, use:

```sh
HX_SERVER_CMD="./hx-server --port 8080" ./tests/run.sh
```

### Environment variables

- `HX_SERVER_BIN`: Path to executable server binary.
- `HX_SERVER_CMD`: Full command to start the server (overrides `HX_SERVER_BIN`).
- `HX_SERVER_ARGS`: Optional arguments appended to `HX_SERVER_BIN`.
- `HX_HOST`: Host to bind against (default: `127.0.0.1`).
- `HX_PORT`: Port to bind against (default: `8080`).

## What is tested

- `GET /` responds with HTTP 200.
- Missing routes respond with HTTP 404.
- `/ws` performs a valid WebSocket RFC 6455 handshake.
