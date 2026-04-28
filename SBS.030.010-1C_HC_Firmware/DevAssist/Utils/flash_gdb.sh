#!/usr/bin/env bash
set -euo pipefail

OPENOCD_LOG=/tmp/devassist_openocd.log
GDB_LOG=/tmp/devassist_gdb.log
READINESS_HELPER="DevAssist/Utils/openocd_readiness_probe.sh"
ELF="${1:-build/SBS.030.010-1C_HC_Firmware.elf}"

GDB_BIN=""
if command -v arm-none-eabi-gdb >/dev/null 2>&1; then
  GDB_BIN="arm-none-eabi-gdb"
elif command -v gdb-multiarch >/dev/null 2>&1; then
  GDB_BIN="gdb-multiarch"
elif command -v gdb >/dev/null 2>&1; then
  GDB_BIN="gdb"
else
  echo "ERROR: no suitable GDB binary found"
  exit 127
fi

cleanup() {
  if [[ -n "${OPENOCD_PID:-}" ]]; then
    kill "$OPENOCD_PID" >/dev/null 2>&1 || true
  fi
  pkill -x openocd >/dev/null 2>&1 || true
}
trap cleanup EXIT
pkill -x openocd >/dev/null 2>&1 || true
rm -f "$OPENOCD_LOG" "$GDB_LOG"

TMP_READINESS_HELPER=/tmp/devassist_openocd_readiness_probe.sh
tr -d '\r' < "$READINESS_HELPER" > "$TMP_READINESS_HELPER"
chmod +x "$TMP_READINESS_HELPER"

if ! bash "$TMP_READINESS_HELPER" "$OPENOCD_LOG" >/tmp/devassist_readiness_stdout.log 2>&1; then
  echo "ERROR: OpenOCD readiness probe failed"
  echo "__DEVASSIST_OPENOCD_LOG__"
  cat /tmp/devassist_readiness_stdout.log || true
  exit 1
fi
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg >"$OPENOCD_LOG" 2>&1 &
OPENOCD_PID=$!

for _ in $(seq 1 20); do
  if grep -q "Listening on port 3333" "$OPENOCD_LOG" 2>/dev/null; then
    break
  fi
  if grep -q "open failed" "$OPENOCD_LOG" 2>/dev/null; then
    echo "ERROR: OpenOCD readiness probe failed"
    echo "__DEVASSIST_OPENOCD_LOG__"
    cat "$OPENOCD_LOG" || true
    exit 1
  fi
  sleep 1
done

if ! grep -q "Listening on port 3333" "$OPENOCD_LOG" 2>/dev/null; then
  echo "ERROR: OpenOCD did not start GDB server"
  echo "__DEVASSIST_OPENOCD_LOG__"
  cat "$OPENOCD_LOG" || true
  exit 1
fi

"$GDB_BIN" -batch "$ELF" \
  -ex "target extended-remote :3333" \
  -ex "monitor reset halt" \
  -ex "load" \
  -ex "monitor reset run" \
  -ex "quit" >"$GDB_LOG" 2>&1 || {
    rc=$?
    echo "ERROR: GDB load failed with code $rc"
    echo "__DEVASSIST_GDB_LOG__"
    cat "$GDB_LOG" || true
    echo "__DEVASSIST_OPENOCD_LOG__"
    cat "$OPENOCD_LOG" || true
    exit "$rc"
  }

if ! grep -Eqi "Loading section|Transfer rate|Start address" "$GDB_LOG"; then
  echo "ERROR: GDB completed without expected load markers"
  echo "__DEVASSIST_GDB_LOG__"
  cat "$GDB_LOG" || true
  echo "__DEVASSIST_OPENOCD_LOG__"
  cat "$OPENOCD_LOG" || true
  exit 1
fi

echo "FLASH_OK"
echo "__DEVASSIST_GDB_LOG__"
cat "$GDB_LOG"
echo "__DEVASSIST_OPENOCD_LOG__"
cat "$OPENOCD_LOG"
