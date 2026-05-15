# Programming and Target Verification Notes

Goal: reliably program the STM32F411 BlackPill target and verify that the flashed firmware is running.

Current hardware path:
- Programmer/debug probe: ST-Link over SWD
- Runtime verification transport: USB CDC / VCP
- Runtime protocol: JSON command/response over USB CDC/VCP

Complete programming verification loop:
1. Use the guarded build process in `Tools/build_notes.md` to produce `build\Debug\HC_FW_BlackPill.elf`.
2. Run:
   - `powershell -NoProfile -ExecutionPolicy Bypass -File Tools\program.ps1`
3. Treat OpenOCD `Verified OK` as flash-content verification.
4. Treat the USB CDC `GET sw_version` response as runtime firmware communication verification.
5. For physical outputs, sensors, timing, and power behavior, request user/hardware confirmation after programming.

Confirmed hardware result:
- The complete build/program/runtime-check path was used after changing the blue LED heartbeat period from `500U` to `250U`.
- The target programmed and verified successfully.
- USB CDC runtime response succeeded.
- The user confirmed the actual hardware BLUE LED flash rate doubled.

Project clues used:
- `.vscode/launch.json` uses an ST-Link GDB target.
- `HC_FW_BlackPill_F411CCU6.ioc` assigns PA13/PA14 to SWD and enables USB Device CDC FS.
- `Docs/architecture.md` and requirements docs define USB VCP as the primary TE interface.
- `Services/CommandHandler/README.md` documents `GET args ["sw_version"]`.
- `Logging/launch.ps1` shows the expected JSON command style and response parsing.

Codex programming procedure:
1. Build or confirm the ELF exists:
   - `build\Debug\HC_FW_BlackPill.elf`
2. Run the guarded programming workflow:
   - `powershell -NoProfile -ExecutionPolicy Bypass -File Tools\program.ps1`
3. Let `Tools\program.ps1` discover the USB CDC/VCP port automatically. Do not assume it will always be `COM7`.
4. Use `-PortName COMx` only when multiple USB serial devices are connected and the script asks for an explicit port.
5. Expected successful results:
   - ST-Link detected
   - target voltage reported near 3.3 V
   - Cortex-M4 detected
   - `DBGMCU_IDCODE` reports `0x10006431`
   - OpenOCD reports `Programming Finished`
   - OpenOCD reports `Verified OK`
   - serial response includes `{"type":"RSP",...,"args":{"sw_version":"..."}}`

Useful commands:

Identify target only:
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\program.ps1 -IdentifyOnly -SkipSerialVerify
```

Program and verify flash, but skip runtime serial verification:
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\program.ps1 -SkipSerialVerify
```

Program and verify using an explicit serial port:
```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\program.ps1 -PortName COM7
```

OpenOCD details:
- Interface config: `interface/stlink.cfg`
- Target config: `target/stm32f4x.cfg`
- SWD adapter speed: `1800 kHz`
- Flash command: `program "<elf>" verify reset exit`

Runtime verification details:
- The script discovers USB serial ports and prefers:
  - USB VID/PID `0483:5740` (`STM32 Virtual COM Port`)
  - product string containing `SBS RadTest CDC`
  - product string containing `STM32`
- The verification command is:
  - `{"type":"GET","msg":101,"args":["sw_version"]}`
- The current verified response after programming was:
  - `{"type":"RSP","hc":4,"msg":101,"ts":"20000502 05:24:22","args":{"sw_version":"0.1.1"}}`

Failure handling:
- If OpenOCD cannot connect, check ST-Link USB connection, SWD wiring, target power, and whether another debug session is using the probe.
- If OpenOCD verifies flash but serial verification fails, wait for USB re-enumeration and confirm the VCP device is present in Device Manager.
- If multiple USB serial devices are connected, re-run with `-PortName COMx`.
- If a port is open in another terminal/logger, close that session before running serial verification.
- After an interrupted programming attempt, check for leftover `openocd` or `arm-none-eabi-gdb` processes before retrying.
