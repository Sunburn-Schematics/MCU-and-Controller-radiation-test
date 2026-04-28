#!/usr/bin/env bash
set -eu

OPENOCD_LOG="${1:-/tmp/devassist_openocd.log}"

cleanup() {
  if [[ -n "${OPENOCD_PID:-}" ]]; then
    kill "$OPENOCD_PID" >/dev/null 2>&1 || true
  fi
  pkill -x openocd >/dev/null 2>&1 || true
}
trap cleanup EXIT

pkill -x openocd >/dev/null 2>&1 || true
rm -f "$OPENOCD_LOG"

openocd -f interface/stlink.cfg -f target/stm32f4x.cfg >"$OPENOCD_LOG" 2>&1 &
OPENOCD_PID=$!

for _ in $(seq 1 20); do
  if grep -q "Listening on port 3333" "$OPENOCD_LOG" 2>/dev/null; then
    break
  fi
  if grep -q "open failed" "$OPENOCD_LOG" 2>/dev/null; then
    cat "$OPENOCD_LOG" || true
    exit 2
  fi
  sleep 1
done

if ! grep -q "Listening on port 3333" "$OPENOCD_LOG" 2>/dev/null; then
  cat "$OPENOCD_LOG" || true
  exit 3
fi

for marker in "VID:PID 0483:3748" "Target voltage:" "Listening on port 3333"; do
  if ! grep -q "$marker" "$OPENOCD_LOG" 2>/dev/null; then
    cat "$OPENOCD_LOG" || true
    exit 4
  fi
done

cat "$OPENOCD_LOG"
