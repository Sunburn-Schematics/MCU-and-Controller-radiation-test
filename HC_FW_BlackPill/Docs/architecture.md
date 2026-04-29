# Firmware architecture

## Layering

The project is split into these layers:

1. **Cube-generated platform**
   - `Core/`
   - `Drivers/`
   - `USB_DEVICE/`
2. **Board support**
   - `Bsp/`
3. **Local drivers**
   - `Drivers_Local/`
4. **Services**
   - `Services/`
5. **Application**
   - `App/`

## Dependency direction

```text
App -> Services -> Drivers_Local -> Bsp -> HAL/Cube
```

Do not reverse this dependency direction.

## Execution model

The firmware currently uses a simple superloop:

1. CubeMX initializes the MCU and configured peripherals.
2. `fw_app_init()` performs user-owned startup.
3. `fw_app_run()` is called repeatedly from the infinite loop.

## Current placeholder behavior

The application toggles the blue LED every 500 ms.

This preserves the current observable behavior while moving it out of `main.c`.

## Coding rules

- Keep generated files regenerable.
- Put custom logic only inside `USER CODE` sections of generated files.
- Keep ISRs short.
- Avoid direct HAL use in high-level application logic unless there is a clear reason.
- Keep board and pin ownership in `Bsp/`.
- Prefer explicit APIs over sharing peripheral handles broadly.

## Planned next modules

- `Bsp/bsp_board.*`
- `Services/log.*`
- `Services/cli.*`
- `Services/fault_manager.*`
- `App/app_state_machine.*`
