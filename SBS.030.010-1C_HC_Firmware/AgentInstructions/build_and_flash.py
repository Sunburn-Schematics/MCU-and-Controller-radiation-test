import subprocess, sys

cids = subprocess.run(["docker", "ps", "-q"], capture_output=True, text=True).stdout.strip().split()
if not cids:
    print("Dev Container down.")
    sys.exit(1)
cid = cids[0]

print("Locating Makefile...")
res = subprocess.run(["docker", "exec", cid, "find", "/workspaces", "-name", "Makefile"], capture_output=True, text=True)
makefiles = res.stdout.strip().split('\n')
workdir = None
for mf in makefiles:
    if 'SBS.030' in mf: workdir = mf.rsplit('/', 1)[0]; break
if not workdir:
    print("Could not locate project workspace dynamically.")
    sys.exit(1)

print(f"Compiling in {workdir}...")
make_res = subprocess.run(["docker", "exec", "-w", workdir, cid, "make", "-j4"], capture_output=True, text=True)
if make_res.returncode != 0:
    print("Build failed:\n" + make_res.stderr)
    sys.exit(1)

print("Flashing...")
subprocess.run(["docker", "exec", cid, "killall", "openocd"], capture_output=True)
flash_cmd = [
    "docker", "exec", "-w", workdir, cid,
    "openocd", "-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg",
    "-c", "program build/SBS.030.010-1C_HC_Firmware.elf verify reset exit"
]
subprocess.run(flash_cmd, capture_output=True)
print("Sequence Complete.")
