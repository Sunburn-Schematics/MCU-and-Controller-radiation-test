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

The application currently toggles the blue LED through the BSP heartbeat path.

This preserves simple observable behavior while validating the first custom software layer.

## Coding rules

- Keep generated files regenerable.
- Put custom logic only inside `USER CODE` sections of generated files.
- Keep ISRs short.
- Avoid direct HAL use in high-level application logic unless there is a clear reason.
- Keep board and pin ownership in `Bsp/`.
- Prefer explicit APIs over sharing peripheral handles broadly.

## BSP design rules

The BSP layer exists to answer one question:

**How does this firmware talk to this specific board safely and clearly?**

The BSP layer owns only simple, board-specific controls and reads that should be named by board function rather than by raw port/pin.

### BSP owns

- board LEDs
- board power-enable signals
- BeamOn digital input read
- 6-bit board ID read
- deterministic safe-state output control
- board-level polarity handling

### BSP does not own

- ADC acquisition logic
- timer input-capture logic
- waveform generation logic
- logging policy
- fault policy
- protocol behavior
- state-machine behavior
- telemetry formatting

### BSP partitioning principles

- Use product-facing names, not raw GPIO names, in public APIs.
- Hide active-high vs active-low behavior inside BSP.
- Do not expose raw GPIO port/pin ownership upward unless there is a strong reason.
- Keep BSP synchronous and simple.
- Do not place application policy in BSP.
- Do not let App or Services manipulate board-owned GPIO directly when BSP already owns it.
- CubeMX/HAL configures hardware; BSP applies board policy and named access.

### Consolidated BSP module direction

The current BSP direction is a single consolidated board module:

- `Bsp/bsp_board.h`
- `Bsp/bsp_board.c`

This module should own:

- LED control
- power-enable control with normalized polarity
- BeamOn read
- board ID read
- board safe-state handling

The current public API style is:

- `bsp_init()`
- `bsp_enter_safe_state()`
- `bsp_get_id_raw()`
- `bsp_is_beam_on()`
- `bsp_get_status()`
- `bsp_led_*()`
- `bsp_power_*()`

### BSP safe-state rule

BSP must provide a deterministic hardware-safe output state that can be applied at startup and reused later by higher-level fault handling.

Current intended safe-state behavior is:

- LT8316 power disabled
- LTC3901 power disabled
- blue LED off
- red LED off
- green LED off

Higher layers may later assert indicators or change power state intentionally, but the safe-state baseline is owned by BSP.

## Drivers_Local design rules

The `Drivers_Local` layer owns STM32/HAL-based peripheral behavior that is specific to this product but is still below Services and App.

This layer should encapsulate peripheral-function behavior using product-level concepts rather than exposing raw STM32 instance details upward.

### Drivers_Local owns

- ADC sensing and channel grouping
- timer input-capture measurement
- synchronized SDRA/SDRB waveform generation
- product-specific transport wrappers when needed for UART or USB CDC
- local hardware conversions tightly coupled to this design

### Drivers_Local does not own

- fault latching/clearing policy
- event logging policy
- CLI command parsing
- application state transitions
- operator-facing reporting behavior

### Drivers_Local partitioning principles

- Name drivers by product function, not by STM32 peripheral instance.
- Hide HAL handles and timer/ADC channel details from upper layers.
- Return normalized values such as raw counts, timing values, frequency, pulse width, or enable state.
- Do not embed application decisions in drivers.
- A driver may report invalid data, timeout, or missing signal; it should not decide that a fault must latch or that the board must enter safe state.
- Keep the public API stable even if the STM32 peripheral instance changes later.

### Recommended Drivers_Local modules

The preferred initial `Drivers_Local` partition is:

- `Drivers_Local/adc_sense_drv.*`
- `Drivers_Local/pwm_capture_drv.*`
- `Drivers_Local/sync_drv.*`

Optional transport wrappers may be added later if needed:

- `Drivers_Local/debug_uart_drv.*`
- `Drivers_Local/usb_cdc_drv.*`

### Drivers_Local module intent

#### `adc_sense_drv.*`
Owns analog acquisition for:

- `VUpstream_Anlg`
- `LTC3901_Vcc_Anlg`
- `LT8316_Vout_Anlg`
- `LTC3901_ME_Anlg`
- `LTC3901_MF_Anlg`
- `LT8316_Gate_Anlg`
- `VTemp`
- `VRefInt`

Responsibilities:

- trigger or retrieve ADC conversions
- provide stable channel enumeration
- return raw counts, nominal millivolts, and optional per-channel engineering-unit conversion using `y = mx + c`

#### `pwm_capture_drv.*`
Owns timer-based measurement for:

- `LTC3901_ME_Tmr`
- `LTC3901_MF_Tmr`
- `LT8316_Gate_Tmr`

Responsibilities:

- frequency measurement
- pulse-width measurement
- duty-cycle derivation
- hiding timer channel and capture math details

Current implementation notes:

- acquisition runs as repeated per-signal bursts once started and continues until aborted
- each signal owns its own DMA channels, timeout, buffers, processing, invalidation, and re-arm path
- each burst captures up to 16 timestamps per active edge stream and processes the burst offline using the number of samples actually captured
- `LTC3901_ME_Tmr` uses `TIM4_CH1` rising-edge DMA plus paired `TIM4_CH2` falling-edge DMA
- `LTC3901_MF_Tmr` uses `TIM2_CH1` rising-edge DMA plus paired `TIM2_CH2` falling-edge DMA
- `LT8316_Gate_Tmr` currently uses `TIM4_CH3` rising-edge DMA only, so frequency is reported while duty-cycle capture remains deferred

#### `sync_drv.*`
Owns synchronized output generation for:

- `SDRA`
- `SDRB`

Responsibilities:

- validate synchronized square-wave generation requests
- generate a 50% duty-cycle SDRA waveform at the requested base frequency
- generate a 50% duty-cycle SDRB waveform delayed relative to SDRA
- convert requested frequency and delay values into timer realization
- enable and disable the synchronized outputs
- hide timer output implementation details from upper layers

Public API model:

- one shared `frequency_hz`
- one `sdrb_delay_ns` relative to SDRA

Design rules:

- SDRA is the reference waveform.
- SDRB uses the same base frequency and 50% duty cycle as SDRA.
- SDRB timing is defined only by its delay relative to SDRA.
- Invalid or unrealizable requests must be rejected by the driver.

## Layer boundary examples

### Belongs in BSP

- turn red LED on
- disable LT8316 power
- read BeamOn input
- read packed board ID

### Belongs in Drivers_Local

- sample all ADC channels
- convert a capture register set into frequency and duty-cycle data
- configure the SDRA base frequency and SDRB delay
- enable or disable synchronized waveform generation

### Does not belong in BSP or Drivers_Local

- decide whether a measured voltage is a latched fault
- decide whether BeamOn should inhibit a restart sequence
- format a status line for CLI or USB reporting
- coordinate application state transitions

## Planned next modules

Near-term architecture direction is:

- keep `Bsp/bsp_board.*` as the consolidated board abstraction
- build `Drivers_Local/adc_sense_drv.*`
- add `Drivers_Local/pwm_capture_drv.*`
- add `Drivers_Local/sync_drv.*`
- add Services only after the local hardware driver contracts are stable enough to support logging, CLI, telemetry, and fault management

## Current ADC implementation status

The current `Drivers_Local/adc_sense_drv.*` implementation:

- start ADC1 regular conversions using DMA
- retain the latest full 8-channel sample frame
- expose stable channel enumeration for the configured ADC input order
- provide raw sample access and nominal pin-level millivolt conversion
- support per-channel engineering-unit conversion from raw counts using configurable `SlopeScaled` and `Offset` factors

Engineering-unit conversion details:

- conversion uses `engineering = ((SlopeScaled * raw_counts) / ADC_SENSE_CALIBRATION_SLOPE_SCALE) + Offset`
- each ADC channel owns an independent calibration entry
- `Valid = false` disables engineering-unit output for that channel
- calibration storage is currently RAM-only

Not yet implemented in this driver:

- calibrated VDDA compensation using VREFINT
- fault thresholds or policy decisions
- timer-derived frequency or duty-cycle measurements
- non-volatile storage for calibration factors
