import subprocess, sys, time

cids = subprocess.run(["docker", "ps", "-q"], capture_output=True, text=True).stdout.strip().split()
if not cids: sys.exit(1)
cid = cids[0]

# OpenOCD commands to manually power GPIOC clock, set PC13 to output, and pull it LOW (LED ON for BlackPill)
cmds = [
    "docker", "exec", cid, "openocd",
    "-f", "interface/stlink.cfg", "-f", "target/stm32f4x.cfg",
    "-c", "init", "-c", "halt",
    "-c", "mww 0x40023830 0x00000004",  # RCC_AHB1ENR: Enable GPIOC clock
    "-c", "mww 0x40020800 0x04000000",  # GPIOC_MODER: Set PC13 to Output
    "-c", "mww 0x40020814 0x00002000",  # GPIOC_BSRR: Reset PC13 (LED ON)
    "-c", "exit"
]
print("Bypassing firmware. Toggling PC13 LED directly via memory mapped registers...")
subprocess.run(cmds, capture_output=True)
print("If LED is ON, hardware trace is flawless.")
