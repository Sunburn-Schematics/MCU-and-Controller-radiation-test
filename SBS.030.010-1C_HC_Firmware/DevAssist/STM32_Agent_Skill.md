# STM32 Development & Debugging Skill

## 🧠 Session Reflection
### What Worked
- **Containerized environments:** Using `.devcontainer` with ARM GCC and OpenOCD provides a reproducible team workflow.
- **Host-owned USB attach:** The ST-Link path is only reliable after the user runs `DevAssist\setup_usbipd.ps1` with elevated privileges on the Windows host.
- **Readiness-gated flashing:** A flash attempt should only proceed after OpenOCD proves it can open the ST-Link and start the GDB server from the same execution path used for automation.
- **Strict proof of success:** Treat flashing as successful only when explicit markers such as `FLASH_OK`, section loading messages, transfer rate, and start address are present.
- **Low-level diagnostics:** OpenOCD direct register access and silicon-ID reads remain the fastest way to separate hardware problems from firmware/configuration problems.

### Pitfalls to Avoid
- **Assuming silicon identity:** Always verify the actual MCU before trusting the `.ioc` target.
- **Assuming USB attach is enough:** `usbipd` showing `Attached` is necessary but not sufficient; OpenOCD must still prove end-to-end readiness.
- **Assuming a fresh session already has OpenOCD running:** Always kill stale instances and start a fresh probe for the current workflow.
- **Using overly broad kill patterns:** Do not use `pkill -f openocd` in helper scripts because it can kill the helper itself if the filename contains `openocd`; use `pkill -x openocd` instead.
- **Ignoring Windows line endings on mounted shell scripts:** Shell helpers in the Windows-mounted workspace may appear as CRLF inside the container. Normalize them to `/tmp` before execution or otherwise enforce LF handling.
- **Hardcoded workspace assumptions:** Detect active container/workspace paths dynamically where possible.

---

## 🛠️ Agent Standard Operating Procedure (SOP)

### 1. Verify the host USB prerequisite first
Before any hardware-facing preflight, debug, or flash operation:
- confirm the user has run `DevAssist\setup_usbipd.ps1` on the Windows host with the required elevated privileges
- verify the ST-Link is attached via `usbipd list`

If this prerequisite is not satisfied, stop and ask the user to run the setup script before proceeding.

### 2. Verify silicon before trusting generated project settings
Use the silicon-ID tooling or direct OpenOCD reads to confirm the actual MCU variant on the board. Do this before assuming the CubeMX project, clocks, memory sizes, or peripheral setup are valid.

### 3. Use readiness-gated preflight
The preferred host preflight entrypoint is:
- `DevAssist\DevEnv_PreFlight.ps1`

A valid hardware-ready preflight requires OpenOCD proof markers from the actual automation path, not just Docker/container visibility.

### 4. Use readiness-gated flash workflow
Use the standardized flash workflow (`DevAssist\build_and_flash.ps1` or `DevAssist\Utils\flash_gdb.sh`) and require explicit success evidence. Never claim the device was flashed unless the logs clearly contain proof markers such as:
- `FLASH_OK`
- `Loading section .isr_vector`
- `Transfer rate:`
- `Start address 0x...`

### 5. Normalize shell helpers before execution in the container
Because Windows-mounted `.sh` files may present with CRLF inside the container, execute normalized copies from `/tmp` when needed. This applies especially to:
- `DevAssist/Utils/flash_gdb.sh`
- `DevAssist/Utils/openocd_readiness_probe.sh`

### 6. Hardware-first debugging
If firmware behavior is suspicious, do not assume application logic is the first cause. Use OpenOCD/GDB, direct register access, and minimal hardware tests to prove whether the target is running, halted, misclocked, or incorrectly configured.

### 7. Safe firmware editing
When modifying CubeMX-managed sources:
- prefer `/* USER CODE BEGIN ... */` regions
- avoid brittle regex edits
- rebuild immediately after each change
- keep project documentation and support tooling aligned with the actual workflow
