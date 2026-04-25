# DevAssist Utilities Compilation Guide

This folder contains the native C# source code for the DevAssist serial infrastructure tools:
- `DiscoverDebugPort`
- `TCPSerialBridge`

It also works alongside the host automation utilities stored in the parent `DevAssist` folder, including:
- `DevEnv_PreFlight.py`
- `check_silicon_id.py`
- `build_and_flash.py`
- `build_and_flash.ps1`

To maintain a strict zero-dependency environment for the Windows serial tools, the C# utilities do **not** require Visual Studio or Python to be installed. They are compiled using the native `.NET Framework` C# compiler (`csc.exe`) that is built into Windows.

## Compilation Instructions

If you make changes to the `.cs` files and need to regenerate the `.exe` binaries, follow these steps:

1. Open a **PowerShell** or **Command Prompt** window.
2. Navigate to the project repository root:

   ```powershell
   cd C:\Users\marty\Code\STM32\SBS.030.010-1C_HC_Firmware
   ```

3. Compile the EXEs directly into the `DevAssist` folder.

### Compile DiscoverDebugPort

```powershell
C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe /nologo /target:exe /out:DevAssist\DiscoverDebugPort.exe DevAssist\Utils\DiscoverDebugPort.cs
```

### Compile TCPSerialBridge

```powershell
C:\Windows\Microsoft.NET\Framework\v4.0.30319\csc.exe /nologo /target:exe /out:DevAssist\TCPSerialBridge.exe DevAssist\Utils\TCPSerialBridge.cs
```

The updated, zero-dependency EXEs are then ready for team use.

## TCPSerialBridge Runtime Behavior

`TCPSerialBridge.exe` opens the physical COM port once and exposes the serial stream through two local interfaces at the same time.

| Interface | Endpoint | Purpose |
|---|---|---|
| Raw TCP | `localhost:5555` | PuTTY, Netcat, simple TCP tools |
| WebSocket | `ws://localhost:5556/` | browser widgets and web-based dashboards |

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

Connect your widget to:

```text
ws://localhost:5556/
```

Browser-side JavaScript example:

## Reliable Build and Flash Workflow

The preferred automated flashing entry point on Windows hosts is:

```powershell
.\DevAssist\build_and_flash.ps1
```

A Python version also exists for reference, but the PowerShell script should be used by default on Windows because it does not depend on Python being installed.

### What it does

1. Detects the active Docker Dev Container dynamically.
2. Locates the STM32 project workspace dynamically under `/workspaces`.
3. Kills stale `openocd` sessions before doing anything else.
4. Builds the firmware with `make -j4` inside the container.
5. Verifies that the build artifacts exist.
6. Runs a **readiness-gated** OpenOCD probe from inside the same container execution path that will be used for flashing.
7. Requires OpenOCD proof markers before it allows flashing:
   - `VID:PID 0483:3748`
   - `Target voltage:`
   - `starting gdb server`
8. Uses `gdb` to `load` the ELF only after the readiness gate passes.
9. Treats any flash attempt without explicit `FLASH_OK` and GDB load markers as a failure.

### Why this workflow exists

We observed that the USB attach state alone was not a sufficient proof that flashing would work. In several cases:
- `usbipd` reported the ST-Link as attached
- `/dev/bus/usb` was present in the container
- but OpenOCD still failed with `Error: open failed`

The correct readiness rule is therefore:
- **do not attempt a flash unless OpenOCD can already open the ST-Link and start the GDB server from the same execution path**.

### Readiness vs success

| State | Meaning |
|---|---|
| `usbipd` says `Attached` | necessary but not sufficient |
| USB device visible in `/dev/bus/usb` | better, but still not sufficient |
| OpenOCD reports VID:PID, target voltage, and GDB server start | ready to flash |
| `FLASH_OK` plus GDB load markers | flash confirmed |

### Required success evidence

A flash is only considered successful if the output includes:
- `FLASH_OK`
- and at least one explicit GDB load marker such as:
  - `Loading section .isr_vector`
  - `Transfer rate:`
  - `Start address 0x...`

If those markers are missing, treat the flash as **failed or unconfirmed**.

We observed that direct host-side `openocd` execution was unreliable because USB access belongs inside the Dev Container. We also observed stale `openocd` processes causing silent failures. This workflow standardizes the recovery, build, and flash sequence so each development session follows the same proven pattern.
