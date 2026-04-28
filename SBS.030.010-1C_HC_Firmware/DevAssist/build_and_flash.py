from __future__ import annotations

import re
import subprocess
import sys
from pathlib import Path

PROJECT_HINT = "SBS.030.010-1C_HC_Firmware"
BIN_PATH = "build/SBS.030.010-1C_HC_Firmware.bin"
ELF_PATH = "build/SBS.030.010-1C_HC_Firmware.elf"
FLASH_ADDR = "0x08000000"
OPENOCD_SUCCESS_MARKERS = [
    "wrote",
    "verified ok",
    "verified",
]


def run(cmd: list[str], check: bool = True, quiet: bool = False) -> subprocess.CompletedProcess[str]:
    result = subprocess.run(cmd, capture_output=True, text=True)
    if not quiet:
        if result.stdout:
            print(result.stdout, end="" if result.stdout.endswith("\n") else "\n")
        if result.stderr:
            print(result.stderr, end="" if result.stderr.endswith("\n") else "\n", file=sys.stderr)
    if check and result.returncode != 0:
        raise RuntimeError(f"Command failed ({result.returncode}): {' '.join(cmd)}")
    return result


def docker_container_name() -> str:
    result = run(["docker", "ps", "--format", "{{.Names}}"], quiet=True)
    names = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    if not names:
        raise RuntimeError("No running devcontainer found.")
    return names[0]


def detect_workspace(container: str) -> str:
    result = run(
        ["docker", "exec", container, "bash", "-lc", "find /workspaces -name Makefile -type f 2>/dev/null"],
        quiet=True,
    )
    makefiles = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    for mf in makefiles:
        if PROJECT_HINT in mf:
            return str(Path(mf).parent).replace("\\", "/")
    if makefiles:
        return str(Path(makefiles[0]).parent).replace("\\", "/")
    raise RuntimeError("Could not locate project workspace in devcontainer.")


def cleanup_openocd(container: str) -> None:
    print("Cleaning up stale OpenOCD sessions...")
    run(["docker", "exec", container, "bash", "-lc", "pkill -f openocd || true; sleep 1"], check=True, quiet=True)


def build(container: str, workdir: str) -> None:
    print(f"Building in {workdir} ...")
    run(["docker", "exec", "-w", workdir, container, "make", "-j4"], check=True)


def assert_artifacts(container: str, workdir: str) -> None:
    print("Validating build artifacts...")
    cmd = f"test -f {ELF_PATH} && test -f {BIN_PATH} && ls -l {ELF_PATH} {BIN_PATH}"
    run(["docker", "exec", "-w", workdir, container, "bash", "-lc", cmd], check=True)


def flash(container: str, workdir: str) -> None:
    print("Flashing target via OpenOCD...")
    openocd_cmd = (
        f"timeout 45s openocd -f interface/stlink.cfg -f target/stm32f4x.cfg "
        f"-c \"init\" "
        f"-c \"reset halt\" "
        f"-c \"flash write_image erase {BIN_PATH} {FLASH_ADDR}\" "
        f"-c \"verify_image {BIN_PATH} {FLASH_ADDR}\" "
        f"-c \"reset run\" "
        f"-c \"shutdown\""
    )
    result = run(["docker", "exec", "-w", workdir, container, "bash", "-lc", openocd_cmd], check=False)
    merged = (result.stdout + "\n" + result.stderr).lower()
    if result.returncode != 0:
        raise RuntimeError("OpenOCD returned a non-zero exit code during flash.")
    if not any(marker in merged for marker in OPENOCD_SUCCESS_MARKERS):
        raise RuntimeError(
            "OpenOCD completed without explicit flash/verify success markers. Treating flash as failed to avoid false positives."
        )
    print("Flash verified successfully.")


def main() -> int:
    try:
        container = docker_container_name()
        print(f"Using devcontainer: {container}")
        workdir = detect_workspace(container)
        cleanup_openocd(container)
        build(container, workdir)
        assert_artifacts(container, workdir)
        cleanup_openocd(container)
        flash(container, workdir)
        print("Build and flash sequence complete.")
        return 0
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
