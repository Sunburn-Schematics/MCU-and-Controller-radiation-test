# DevAssist Utilities Compilation and Operations Guide

This folder contains the native C# source code for the DevAssist serial infrastructure tools:
- `DiscoverDebugPort`
- `TCPSerialBridge`

It works alongside the host automation utilities stored in the parent `DevAssist` folder, including:
- `DevEnv_PreFlight.ps1` **(primary preflight entrypoint)**
- `DevEnv_PreFlight.py` **(legacy/reference only)**
- `build_and_flash.ps1` **(primary Windows build/flash entrypoint)**
- `build_and_flash.py` **(legacy/reference only)**
- `Utils\flash_gdb.sh`
- `Utils\openocd_readiness_probe.sh`

The C# utilities do **not** require Visual Studio or Python to be installed. They are compiled using the native `.NET Framework` C# compiler (`csc.exe`) available on Windows.

## Compilation Instructions

If you change the `.cs` files and need to rebuild the EXEs:

1. Open **PowerShell** or **Command Prompt**.
2. Navigate to the repository root:

```powershell
cd C:\Users\marty\Code\STM32\SBS.030.010-1C_HC_Firmware
```

### Compile DiscoverDebugPort

```powershell
C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe /nologo /target:exe /out:DevAssist\DiscoverDebugPort.exe DevAssist\Utils\DiscoverDebugPort.cs
```

### Compile TCPSerialBridge

```powershell
C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe /nologo /target:exe /out:DevAssist\TCPSerialBridge.exe DevAssist\Utils\TCPSerialBridge.cs
```

## TCPSerialBridge Runtime Behavior

`TCPSerialBridge.exe` opens the physical COM port once and exposes the serial stream through two local interfaces at the same time.

| Interface | Endpoint | Purpose |
|---|---|---|
| Raw TCP | `localhost:5555` | PuTTY, Netcat, simple TCP tools |
| WebSocket | `ws://localhost:5556/` | browser widgets and web dashboards |

### What it does
- opens the debug COM port saved in `DevAssist\env.json`
- forwards serial RX data to all connected TCP and WebSocket clients
- forwards TCP client payloads back to the serial port
- forwards WebSocket text or binary payloads back to the serial port
- supports WebSocket ping/pong handling

### Running the bridge
From the repository root:

```powershell
.\DevAssist\TCPSerialBridge.exe
```

Expected console output includes endpoints similar to:

```text
=== TCP + WebSocket Serial Bridge Active ===
TCP       : localhost:5555 -> COMx
WebSocket : ws://localhost:5556/ -> COMx
```

### Example clients
#### Raw TCP
- **PuTTY**
  - Host: `localhost`
  - Port: `5555`
  - Connection type: `Raw`
- **Netcat**

```powershell
ncat localhost 5555
```

#### Browser / Web Widget
Connect to:

```text
ws://localhost:5556/
```

## Reliable Preflight Workflow

The preferred preflight entrypoint on Windows hosts is:

```powershell
powershell -ExecutionPolicy Bypass -File ".\DevAssist\DevEnv_PreFlight.ps1"
```

The Python preflight remains in the repo only as a legacy reference.

### Required host-side prerequisite
Before hardware-facing checks can pass, the **user** must run:

```powershell
.\DevAssist\setup_usbipd.ps1
```

with the required elevated privileges on the Windows host.

The PowerShell preflight now treats this as a mandatory prerequisite and checks host attachment state using `usbipd list` before attempting container-side OpenOCD readiness.

### What the PowerShell preflight checks
1. Confirms the host workspace exists.
2. Confirms the ST-Link host prerequisite through `usbipd list`.
3. Verifies the fixed Docker container/workspace path is available.
4. Verifies required files exist:
   - `Makefile`
   - `DevAssist/Utils/flash_gdb.sh`
   - `DevAssist/Utils/openocd_readiness_probe.sh`
5. Verifies key tools are available:
   - `arm-none-eabi-gcc`
   - `make`
   - `openocd`
6. Verifies firmware artifacts exist:
   - `.elf`
   - `.bin`
7. Runs a readiness-gated OpenOCD probe from the same container execution path used by automation.
8. Requires readiness proof markers before passing the hardware path.

### Readiness proof markers
A hardware-ready preflight requires OpenOCD proof such as:
- `VID:PID 0483:3748`
- `Target voltage:`
- `Listening on port 3333`

### Known failure markers
Fail clearly if the logs contain errors such as:
- `Error: open failed`
- `No device found`
- `init mode failed`
- `unable to open ftdi device`

### Important rule
A preflight is **not** hardware-ready just because:
- `usbipd` says `Attached`, or
- the USB device appears inside the container.

It is only hardware-ready when **OpenOCD itself can open the ST-Link and emit the required readiness markers from the same path the automation uses**.

## Reliable Build and Flash Workflow

The preferred automated flashing entrypoint on Windows hosts is:

```powershell
.\DevAssist\build_and_flash.ps1
```

### What it does
1. Detects the active STM32 devcontainer/workspace.
2. Builds the firmware with `make` inside the container.
3. Verifies output artifacts exist.
4. Runs a readiness-gated OpenOCD probe before flashing.
5. Only proceeds to GDB `load` after readiness proof is present.
6. Requires explicit flash success evidence before reporting success.

### Required flash success evidence
A flash is only considered successful if the output includes:
- `FLASH_OK`
- and explicit GDB/OpenOCD load markers such as:
  - `Loading section .isr_vector`
  - `Transfer rate:`
  - `Start address 0x...`

If those markers are missing, treat the flash as **failed or unconfirmed**.

## Shell Script Reliability Notes

### CRLF handling on mounted shell scripts
When `.sh` files are executed from the Windows-mounted workspace inside the devcontainer, they may appear with CRLF line endings and fail with errors such as:
- `set: pipefail: invalid option name`

For reliable execution, normalize shell helpers to a temporary LF-only copy in `/tmp` before running them.

This especially applies to:
- `DevAssist/Utils/flash_gdb.sh`
- `DevAssist/Utils/openocd_readiness_probe.sh`

### Safe OpenOCD process cleanup
Do **not** use:

```bash
pkill -f openocd
```

because it can terminate helper scripts whose filenames contain `openocd`.

Use:

```bash
pkill -x openocd || true
```

instead.

That change is required for both preflight and flash helpers so the readiness probe does not kill itself.

## Operational Summary

| State | Meaning |
|---|---|
| `usbipd` says `Attached` | necessary but not sufficient |
| USB device visible in container | still not sufficient |
| OpenOCD emits readiness markers | ready to flash |
| `FLASH_OK` plus load markers | flash confirmed |

This workflow exists because USB visibility alone was repeatedly insufficient proof that OpenOCD flashing would work. The standardized DevAssist preflight and flash scripts encode the proven recovery/build/flash path so future sessions can use the same validated process.
