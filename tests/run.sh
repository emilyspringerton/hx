#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# shellcheck source=tests/harness.sh
source "$ROOT_DIR/tests/harness.sh"

HX_HOST="${HX_HOST:-127.0.0.1}"
HX_PORT="${HX_PORT:-8080}"
HX_SERVER_BIN="${HX_SERVER_BIN:-}" 
HX_SERVER_CMD="${HX_SERVER_CMD:-}"
HX_SERVER_ARGS="${HX_SERVER_ARGS:-}"

if [[ -z "$HX_SERVER_CMD" && -z "$HX_SERVER_BIN" ]]; then
  echo "[SKIP] HX_SERVER_CMD or HX_SERVER_BIN is required to run tests." >&2
  exit 0
fi

if [[ -z "$HX_SERVER_CMD" ]]; then
  if [[ ! -x "$HX_SERVER_BIN" ]]; then
    echo "[SKIP] HX_SERVER_BIN '$HX_SERVER_BIN' is not executable." >&2
    exit 0
  fi
  HX_SERVER_CMD="$HX_SERVER_BIN $HX_SERVER_ARGS"
fi

"$ROOT_DIR/tests/start_server.sh" "$HX_SERVER_CMD" "$HX_HOST" "$HX_PORT"

start_test "http_root_served"
python3 "$ROOT_DIR/tests/test_http.py" --host "$HX_HOST" --port "$HX_PORT" --path "/" --expect-status 200
pass_test "http_root_served"

start_test "http_404"
python3 "$ROOT_DIR/tests/test_http.py" --host "$HX_HOST" --port "$HX_PORT" --path "/nope" --expect-status 404
pass_test "http_404"

start_test "websocket_handshake"
python3 "$ROOT_DIR/tests/test_ws_handshake.py" --host "$HX_HOST" --port "$HX_PORT" --path "/ws"
pass_test "websocket_handshake"

"$ROOT_DIR/tests/stop_server.sh"

finish_tests
