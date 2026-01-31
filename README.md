# HX

HX is a minimal in-browser terminal server.

## Connect

By default, connect to:

- HTTP UI: `http://127.0.0.1:8080/`
- WebSocket: `ws://127.0.0.1:8080/ws`

If your server supports configurable host/port, update these URLs to match.

## CI artifacts

The GitHub Actions workflow uploads the HX server binary from `HX_APP_BIN` (default `hx-server` / `hx-server.exe`). If your build outputs a different path, set `HX_APP_BIN` or update the workflow.
