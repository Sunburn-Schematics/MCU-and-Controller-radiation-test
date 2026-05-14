# HC_FW_BlackPill

STM32F411 firmware project targeting a Black Pill development board.

This project is structured for clean inclusion in the repository:

- https://github.com/Sunburn-Schematics/MCU-and-Controller-radiation-test

## Objectives

- Keep STM32CubeMX generated code isolated in `Core/`, `Drivers/`, and `USB_DEVICE/`
- Keep application logic in stable user-owned modules
- Minimize merge pain when CubeMX regenerates code
- Provide a clear starting point for radiation test firmware evolution

## Directory layout

| Path | Purpose |
|---|---|
| `Core/` | STM32CubeMX-generated startup and peripheral init |
| `Drivers/` | CMSIS and STM32 HAL |
| `USB_DEVICE/` | STM USB device stack glue |
| `App/` | Top-level firmware application flow |
| `Bsp/` | Board-specific mappings and simple board abstractions |
| `Drivers_Local/` | User-owned HAL wrapper drivers |
| `Services/` | Logging, protocol, CLI, timing, fault handling |
| `Config/` | Project and board configuration headers |
| `Docs/` | Architecture and integration notes |

## Architectural rules

1. Keep generated files regenerable.
2. Add custom logic only inside `USER CODE` regions in generated files.
3. Prefer calling user-owned modules from `main.c` rather than writing logic directly in generated files.
4. Keep application logic independent from direct HAL calls where practical.
5. Put board mapping in `Bsp/`, not in application modules.
6. Keep interrupts short. Set flags or buffer data there, then process in the main loop.

## Current execution flow

`main.c` performs MCU and peripheral init, then transfers control to:

- `fw_app_init()`
- `fw_app_run()`

This keeps the generated superloop minimal and makes future migration to scheduler or state-machine logic straightforward.

## Build notes

The project uses the STM32CubeMX CMake export plus user source registration in the top-level `CMakeLists.txt`.

Manual builds should use VS Code CMake Tools with the `Debug` or `Release` preset from `CMakePresets.json`.

Automation should mirror the manual CMake Tools workflow rather than call compiler commands directly:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tools/build.ps1 -Preset Debug -DryRun
powershell -NoProfile -ExecutionPolicy Bypass -File Tools/build.ps1 -Preset Debug
```

The wrapper discovers the installed STM32 VS Code extension paths, prepends the same `cube-cmake` and STM32 binary paths used by the STM32 VS Code extensions, and operates on the CMake-generated Ninja graph under `build/<preset>/`.

The default backend is `-Backend Commands`. It asks Ninja for the generated command list with `ninja -t commands <target>`, then executes those generated compile and link commands one at a time with per-command timeouts and captured logs. This keeps automation synchronized with the manual CMake Tools build graph while avoiding the observed Ninja scheduler hang in this environment.

Use `-Backend Ninja` or `-Backend CubeCMake` only when comparing behavior against the normal manual build layers.

## Repository inclusion notes

Before first commit into the target repository, verify:

- board pinout matches actual Black Pill hardware revision
- clock tree values match installed crystal
- UART/USB interfaces match intended radiation test harness
- generated files are from the expected STM32CubeMX / STM32CubeIDE extension version

## Next recommended steps

- add `Bsp/` board mapping module
- add UART logging service
- add explicit application state machine
- add fault/event reporting over UART or USB CDC
- document radiation test command and telemetry protocol in `Docs/`
