#!/usr/bin/env bash
set -euo pipefail

if [[ $# -lt 3 ]]; then
  echo "usage: start_server.sh <cmd> <host> <port>" >&2
  exit 1
fi

CMD="$1"
HOST="$2"
PORT="$3"

PID_FILE="/tmp/hx_test_server.pid"
LOG_FILE="/tmp/hx_test_server.log"

if [[ -f "$PID_FILE" ]]; then
  echo "existing pid file found, refusing to start" >&2
  exit 1
fi

bash -c "$CMD" >"$LOG_FILE" 2>&1 &
SERVER_PID=$!

echo "$SERVER_PID" >"$PID_FILE"

for _ in {1..50}; do
  if (echo >"/dev/tcp/$HOST/$PORT") >/dev/null 2>&1; then
    exit 0
  fi
  sleep 0.1
  if ! kill -0 "$SERVER_PID" >/dev/null 2>&1; then
    echo "server process exited early" >&2
    exit 1
  fi
  done

echo "server did not become ready on $HOST:$PORT" >&2
exit 1
