# LEGACY: Host-Python preflight retained only for reference.
# Preferred entrypoint: DevAssist/DevEnv_PreFlight.ps1
import os
import subprocess
import sys

FIXED_CONTAINER_NAME = "STM32_HC_DevContainer"
WORKSPACE = "/workspaces/STM32/SBS.030.010-1C_HC_Firmware"
ELF_PATH = "build/SBS.030.010-1C_HC_Firmware.elf"
BIN_PATH = "build/SBS.030.010-1C_HC_Firmware.bin"
FLASH_HELPER = "DevAssist/Utils/flash_gdb.sh"
READINESS_CMD = (
    "pkill -f openocd || true; "
    "timeout 10s openocd -f interface/stlink.cfg -f target/stm32f4x.cfg "
    "-c \"transport select hla_swd; init; reset halt; shutdown\" 2>&1 || true"
)
REQUIRED_MARKERS = [
    "VID:PID 0483:3748",
    "Target voltage:",
    "starting gdb server",
]
FAIL_MARKERS = [
    "Error: open failed",
    "No device found",
    "init mode failed",
    "unable to open ftdi device",
]


def run(cmd):
    return subprocess.run(cmd, capture_output=True, text=True)


def fail(msg):
    print(f"[FAIL] {msg}")
    return False


def ok(msg):
    print(f"[PASS] {msg}")
    return True


def main():
    print("=== DevAssist Startup Environment Pre-Flight Check ===")
    all_passed = True

    repo_dir = os.getcwd()
    if os.path.exists(repo_dir):
        ok(f"Host project workspace located: {repo_dir}")
    else:
        all_passed = fail(f"Host project workspace missing at {repo_dir}")

    ps = run(["docker", "ps", "--format", "{{.Names}}"])
    names = [line.strip() for line in ps.stdout.splitlines() if line.strip()]
    if not names:
        print("[FAIL] No devcontainer is running. Open VS Code and rebuild/reopen in container.")
        sys.exit(1)

    if FIXED_CONTAINER_NAME in names:
        container = FIXED_CONTAINER_NAME
        ok(f"Fixed devcontainer running: {container}")
    else:
        container = names[0]
        print(f"[WARN] Fixed container name not found. Falling back to first running container: {container}")
        all_passed = False

    ws = run(["docker", "exec", container, "bash", "-lc", f"cd {WORKSPACE} && pwd"])
    if ws.returncode == 0 and WORKSPACE in ws.stdout:
        ok(f"Workspace available in container: {WORKSPACE}")
    else:
        all_passed = fail(f"Workspace missing or inaccessible in container: {WORKSPACE}")

    files = run([
        "docker", "exec", container, "bash", "-lc",
        f"cd {WORKSPACE} && test -f Makefile && test -f {FLASH_HELPER}"
    ])
    if files.returncode == 0:
        ok("Required project files are present (Makefile, flash_gdb.sh)")
    else:
        all_passed = fail("Required project files are missing")

    gcc = run(["docker", "exec", container, "bash", "-lc", "arm-none-eabi-gcc --version | head -n 1"])
    if gcc.returncode == 0 and gcc.stdout.strip():
        ok(f"ARM GCC available: {gcc.stdout.strip()}")
    else:
        all_passed = fail("ARM GCC missing inside container")

    make_res = run(["docker", "exec", container, "bash", "-lc", "make --version | head -n 1"])
    if make_res.returncode == 0 and make_res.stdout.strip():
        ok(f"GNU Make available: {make_res.stdout.strip()}")
    else:
        all_passed = fail("GNU Make missing inside container")

    ocd = run(["docker", "exec", container, "bash", "-lc", "openocd --version 2>&1 | head -n 1"])
    if ocd.returncode == 0 and ocd.stdout.strip():
        ok(f"OpenOCD available: {ocd.stdout.strip()}")
    else:
        all_passed = fail("OpenOCD missing inside container")

    artifacts = run([
        "docker", "exec", container, "bash", "-lc",
        f"cd {WORKSPACE} && test -f {ELF_PATH} && test -f {BIN_PATH} && ls -l {ELF_PATH} {BIN_PATH}"
    ])
    if artifacts.returncode == 0:
        ok("Firmware artifacts are present (.elf and .bin)")
    else:
        all_passed = fail("Firmware artifacts are missing")

    probe = run(["docker", "exec", container, "bash", "-lc", READINESS_CMD])
    probe_out = (probe.stdout or "") + (probe.stderr or "")

    if any(marker in probe_out for marker in FAIL_MARKERS):
        print("[FAIL] OpenOCD readiness probe failed. ST-Link is not openable from this container path.")
        print(probe_out.strip())
        all_passed = False
    elif all(marker in probe_out for marker in REQUIRED_MARKERS):
        ok("OpenOCD readiness gate passed (VID:PID, target voltage, GDB server startup)")
    else:
        print("[FAIL] OpenOCD readiness proof markers were incomplete.")
        if probe_out.strip():
            print(probe_out.strip())
        all_passed = False

    print("========================================================")
    if all_passed:
        print(">>> ALL CHECKS PASSED. Environment and hardware path are ready. <<<")
        sys.exit(0)
    else:
        print(">>> PRE-FLIGHT CHECKS FAILED. Resolve issues before continuing. <<<")
        sys.exit(1)


if __name__ == "__main__":
    main()
