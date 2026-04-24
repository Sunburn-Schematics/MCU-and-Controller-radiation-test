# STM32 Development & Debugging Skill

## 🧠 Session Reflection
### What Worked:
- **Containerized Environments:** Using `.devcontainer` with ARM GCC and OpenOCD guarantees environment replication. `usbipd-win` bridges the WSL gap beautifully.
- **Bare-Metal Diagnostics:** Using `openocd` to write directly to registers (`mdw`, `mww`) is the ultimate hardware sanity check, completely bypassing buggy HAL code.
- **Low-Level Sleuthing:** Querying `DBGMCU_IDCODE` (0xE0042000) directly over SWD to discover the silicon was an F401 instead of an F411 solved the core Hard Fault mystery.
- **GDB Backtracing:** Pulling the PC and MSP from the CPU crash state identified the `__libc_init_array` null-pointer trap caused by missing standard libraries.

### What Didn't Work (Pitfalls to Avoid):
- **Assuming Silicon Identity:** Generating code before physically verifying the chip variant led to mismatched clock trees (100MHz vs 84MHz) and immediate Hard Faults.
- **Regex Blind Replacements:** Injecting C code via regex without strictly managing curly braces `}` broke the AST and corrupted compilation.
- **Hardcoded Workspace Paths:** Dockers mount points (`/workspaces/...`) vary. Hardcoding them leads to "No Makefile found" errors.
- **Missing Toolchain Configs:** Forgetting `libnewlib-arm-none-eabi` in the initial Docker image caused bootloader crashes.

---

## 🛠️ Agent Standard Operating Procedure (SOP)

### 1. Verify Silicon Before Coding
**Never trust the project `.ioc` blindly.** Always run `check_silicon_id.py` to read the hardware ID directly from the chip via OpenOCD. Match it against the STM32 Reference Manual.

### 2. Resilient Compilation
Use dynamically located workspace paths. Run `build_and_flash.py` to automatically detect the Makefile, compile `-j4`, and flash the `.elf` via the active Docker container.

### 3. Hardware-First Debugging
If the CPU hits a Hard Fault, do not assume the software logic is wrong. Run `direct_hardware_toggle.py` to bypass the entire stack via raw SWD register writes. If the LED blinks via Direct Memory Access, the hardware is fine; the bug is in the HAL or clock configurations.

### 4. Safe Code Injection
When modifying `.c` files, locate the STM32Cube `/* USER CODE BEGIN X */` blocks. Use robust string manipulation to preserve boundaries and trailing brackets. Always run a clean `make` to catch syntax faults immediately.
