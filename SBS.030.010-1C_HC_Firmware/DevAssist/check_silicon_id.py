import subprocess, re, sys

print("=== Interrogating Silicon Device ID ===")
cids = subprocess.run(["docker", "ps", "-q"], capture_output=True, text=True).stdout.strip().split()
if not cids:
    print("Dev Container not found!")
    sys.exit(1)
cid = cids[0]

cmds = [
    "docker", "exec", cid, "openocd",
    "-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg",
    "-c", "init", "-c", "halt",
    "-c", "echo "\n--- DBGMCU_IDCODE (0xE0042000) ---"", "-c", "mdw 0xE0042000",
    "-c", "echo "\n--- CPUID Base Register (0xE000ED00) ---"", "-c", "mdw 0xE000ED00",
    "-c", "exit"
]
res = subprocess.run(cmds, capture_output=True, text=True)
output = res.stderr + "\n" + res.stdout

id_match = re.search(r"0x[eE]0042000:\s*([0-9a-fA-F]+)", output)
if id_match:
    val = int(id_match.group(1), 16)
    dev_id = val & 0xFFF
    print(f"Extracted DEV_ID: 0x{dev_id:03X}")
    if dev_id == 0x431: print("IDENTIFIED AS: STM32F411xx")
    elif dev_id in [0x423, 0x433]: print("IDENTIFIED AS: STM32F401xx")
    else: print("IDENTIFIED AS: Unknown variant")
else:
    print("Could not extract DBGMCU_IDCODE. Check ST-Link connection.")
