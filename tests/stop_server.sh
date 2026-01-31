#!/usr/bin/env bash
set -euo pipefail

PID_FILE="/tmp/hx_test_server.pid"

if [[ ! -f "$PID_FILE" ]]; then
  exit 0
fi

SERVER_PID="$(cat "$PID_FILE")"

if kill -0 "$SERVER_PID" >/dev/null 2>&1; then
  kill "$SERVER_PID"
  for _ in {1..50}; do
    if ! kill -0 "$SERVER_PID" >/dev/null 2>&1; then
      rm -f "$PID_FILE"
      exit 0
    fi
    sleep 0.1
  done
  kill -9 "$SERVER_PID" >/dev/null 2>&1 || true
fi

rm -f "$PID_FILE"
