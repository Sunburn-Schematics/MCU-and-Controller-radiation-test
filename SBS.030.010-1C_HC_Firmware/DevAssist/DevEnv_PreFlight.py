import os, subprocess, sys

def check():
    print("=== DevAssist Startup Environment Pre-Flight Check ===")
    proj_dir = r"C:\Users\marty\Code\STM32\SBS.030.010-1C_HC_Firmware"
    all_passed = True

    if os.path.exists(proj_dir):
        print("[PASS] Host Project Workspace located.")
    else:
        print(f"[FAIL] Host Project Workspace missing at {proj_dir}.")
        all_passed = False

    cids = subprocess.run(["docker", "ps", "-q"], capture_output=True, text=True).stdout.strip().split()
    if not cids:
        print("[FAIL] Dev Container is NOT running. Open VS Code and 'Reopen in Container'.")
        sys.exit(1)
    cid = cids[0]
    print(f"[PASS] Dev Container running (ID: {cid[:12]})")

    gcc = subprocess.run(["docker", "exec", cid, "arm-none-eabi-gcc", "--version"], capture_output=True, text=True)
    if gcc.returncode == 0:
        print("[PASS] ARM GCC Compiler is installed and responding.")
    else:
        print("[FAIL] ARM GCC missing inside container.")
        all_passed = False

    make_res = subprocess.run(["docker", "exec", cid, "make", "--version"], capture_output=True, text=True)
    if make_res.returncode == 0:
        print("[PASS] GNU Make is ready.")
    else:
        print("[FAIL] GNU Make missing.")
        all_passed = False

    subprocess.run(["docker", "exec", cid, "killall", "-q", "openocd"], capture_output=True)
    oocd = subprocess.run([
        "docker", "exec", cid, "openocd",
        "-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg",
        "-c", "init", "-c", "shutdown"
    ], capture_output=True, text=True)
    out = oocd.stdout + oocd.stderr

    if "Error: open failed" in out or "No device found" in out or "init_reset" in out:
        print("[FAIL] ST-Link NOT found in container. Need to run .devcontainer/setup_usbipd.ps1")
        all_passed = False
    elif "Target voltage" in out or "hardware has" in out:
        print("[PASS] ST-Link connected and STM32 target found via SWD.")
    else:
        print("[WARN] ST-Link state unclear:\n" + out.strip())
        all_passed = False

    print("========================================================")
    if all_passed:
        print(">>> ALL CHECKS PASSED. Ready for Development. <<<")
        sys.exit(0)
    else:
        print(">>> PRE-FLIGHT CHECKS FAILED. Please resolve issues before continuing. <<<")
        sys.exit(1)

if __name__ == '__main__':
    check()
