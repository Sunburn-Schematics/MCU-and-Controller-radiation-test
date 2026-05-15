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

## Current Dependency Shape

The intended architectural direction remains:

```text
App -> Services -> Drivers_Local -> Bsp -> HAL/Cube
```

The current implementation has one deliberate command-facing exception: the JSON command handler in `Services/CommandHandler` calls the `fw_app_*` application facade to apply user requests and query application-owned debug/status behavior. This keeps command parsing in Services while leaving hardware policy in App.

Current high-level source dependencies are:

- `Core/Src/main.c` captures reset reason during early user init, initializes CubeMX peripherals, calls `fw_app_init()` once, and calls `fw_app_run()` from the infinite loop.
- `App/fw_app.c` coordinates BSP, ADC, PWM capture, USB-VCP, command processing, debug telemetry, status formatting, sync generation, and the DUT-local managers.
- `Services/CommandHandler` parses JSON commands and calls `fw_app_*` facade functions for application-owned settings.
- `Drivers_Local` owns HAL-backed ADC, PWM capture, RTC, reset-reason, sync, and USB-VCP wrappers.
- `Bsp/bsp_board.*` owns named board GPIO controls and board status reads.

## Execution model

The firmware currently uses a simple superloop:

1. CubeMX initializes the MCU and configured peripherals.
2. `fw_app_init()` performs user-owned startup.
3. `fw_app_run()` is called repeatedly from the infinite loop.

Current `fw_app_run()` order is:

1. service ADC acquisition
2. service USB-VCP RX/TX
3. service PWM capture bursts
4. process JSON commands
5. refresh the application status snapshot from BSP/ADC/PWM sources
6. run the LTC3901 manager and apply its output intents
7. run the LT8316 manager and apply its output intents
8. service debug telemetry
9. toggle the blue heartbeat LED when due
10. emit periodic `STS` when due
11. delay for 10 ms

The firmware is cooperative and non-RTOS. ADC and PWM DMA callbacks only mark completion/error state; processing and rearming are performed from the superloop.

## Current Application Behavior

The application currently:

- applies BSP safe state during startup
- initializes ADC, PWM capture, USB-VCP, command processing, debug telemetry, status reporting, and sync generation
- configures the sync timer but leaves sync disabled until the LTC3901 manager enables it
- periodically toggles the blue LED as a heartbeat
- continuously captures ADC and PWM measurements
- accepts JSON `GET` / `SET` commands over USB-VCP
- emits asynchronous JSON `EVT` records for manager events
- emits periodic JSON `STS` records at a configurable interval
- runs the LTC3901 manager as the owner of LTC3901 power and sync policy
- runs the LT8316 manager as the owner of LT8316 HV power policy
- can optionally auto-issue one `RUN` request to each DUT manager after startup when compile-time `AUTOSTART_ENABLE` is non-zero

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

### Current Drivers_Local modules

The current `Drivers_Local` partition is:

- `Drivers_Local/adc_sense_drv.*`
- `Drivers_Local/pwm_capture_drv.*`
- `Drivers_Local/sync_drv.*`
- `Drivers_Local/rtc_drv.*`
- `Drivers_Local/reset_reason_drv.*`
- `Drivers_Local/usb_vcp_drv.*`

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

`hc_app_status` publishes `STS` `vsupply` and `vshunt` from the ADC engineering-unit path for `VUpstream_Anlg` and `LTC3901_Vcc_Anlg` when those channel calibrations are valid. If an engineering calibration is not valid, it falls back to nominal pin-level millivolts. It derives the `STS` `isupply` field from those published values as `(vsupply - vshunt) / 10 ohms`, reported in milliamps after fixed-point rounding. If either source voltage is invalid, or the computed shunt voltage would be negative, `isupply` is reported as unavailable.

The default ADC engineering calibration for `VUpstream_Anlg` and `LTC3901_Vcc_Anlg` assumes a 100 k high-side / 37.4 k low-side resistor divider. The default conversion scales raw ADC counts directly to circuit sense-point millivolts using a divider multiplier of approximately 3.6738.

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
- `LT8316_Gate_Tmr` uses `TIM4_CH3` rising-edge DMA only, so frequency is reported while duty-cycle capture is intentionally not measured
- TIM2 and TIM4 use a common capture prescaler, producing a 10.5 MHz capture tick from the 84 MHz APB1 timer clock. This preserves useful resolution for hundreds-of-kHz LTC3901 timing while keeping low-kHz LT8316 gate periods inside TIM4's 16-bit wrap interval.

#### `sync_drv.*`
Owns synchronized output generation for:

- `SDRA`
- `SDRB`

Responsibilities:

- configure TIM3 output-compare toggle generation for SDRA/SDRB using raw timer values
- force SDRA/SDRB inactive while applying a new raw timer configuration
- enable and disable the synchronized outputs
- hide timer output implementation details from upper layers

Public API model:

- `sync_drv_raw_config_t.ARR` sets the TIM3 auto-reload value
- `sync_drv_raw_config_t.CCR2` sets the SDRB channel compare value
- SDRA uses channel 1 with compare value `0`
- `sync_drv_configure()` applies the raw configuration and leaves the outputs disabled
- `sync_drv_enable()` / `sync_drv_disable()` start and stop both output channels
- `sync_drv_configure_and_enable()` exists, but `fw_app_init()` currently uses `sync_drv_configure()` so startup does not enable sync automatically

Design rules:

- SDRA is the reference toggle waveform.
- SDRB uses the same timer period and toggles at the configured `CCR2` offset.
- `ARR` must be greater than `CCR2`.
- Higher layers own when sync is enabled. The current LTC3901 manager is the application owner of sync enable/disable policy.

#### `rtc_drv.*`

`rtc_drv.*` owns validation and HAL access for RTC date/time values.

#### `reset_reason_drv.*`

`reset_reason_drv.*` captures RCC reset flags into a local summary and clears the hardware flags after capture. The current API exposes whether reset-reason capture is initialized and whether specific reset flags were present.

#### `usb_vcp_drv.*`

`usb_vcp_drv.*` wraps the USB CDC interface with RX/TX ring-buffer handling. `command_processor_task()` consumes bytes from this driver and `hc_comms_tx_send_line()` sends JSON output through it.

## Application Modules

### `fw_app.*`

`fw_app.c` is the current top-level application coordinator. It owns:

- superloop task ordering
- heartbeat and periodic status timing
- compile-time autostart policy using `AUTOSTART_ENABLE` and `AUTOSTART_DELAY_MS`
- LTC3901 and LT8316 manager instances, one-shot request storage, and provisional manager configuration constants
- mapping manager output intents to `bsp_power_write()` and `sync_drv_enable()` / `sync_drv_disable()`
- debug facade functions used by the JSON command handler

When autostart is enabled, `fw_app.c` waits `AUTOSTART_DELAY_MS` after `fw_app_init()` and then issues a single `RUN` request to each DUT manager that is still in `RESET` and does not already have a pending external request. Autostart is one-shot per boot; later `RESET` or `HALT` commands do not retrigger it.

### `hc_app_status.*`

`hc_app_status` owns the application status snapshot and `STS` JSON formatting. It refreshes live fields from BSP, ADC, and PWM capture drivers, including:

- board ID and BeamOn
- LTC3901 and LT8316 power state
- LTC3901 and LT8316 manager states as each DUT `state`
- LTC3901 `vsupply` and `vshunt` from ADC engineering-unit calibration where valid, otherwise nominal pin-level millivolts
- LTC3901 `isupply = (vsupply - vshunt) / 10 ohms`
- ME/MF frequency and ratio
- LT8316 gate frequency
- analog millivolt fields

Unavailable numeric measurements are formatted as JSON `null`.

### `hc_debug_telemetry.*`

`hc_debug_telemetry` owns named debug signal lookup and periodic `DBG` output formatting. It exposes ADC, PWM, board, DUT manager state, and selected digital signals. `ltc3901.pwr_en` and `lt8316.pwr_en` remain settable as compatibility manager requests. Explicit LTC3901 control should use `SET args.ltc3901_cmd`. `sync.enable` is observable but no longer directly settable because the LTC3901 manager owns sync control.

Host-controller-level commands are accepted through `SET args.hc_cmd`. The
implemented `RESET` command acknowledges the request and then performs a
deferred MCU software reset using `NVIC_SystemReset()`. The reset does not
rewrite the RTC backup domain, so RTC date/time is preserved according to the
reset and RTC startup policy.

### `ltc3901_manager.*`

`ltc3901_manager` owns the DUT1 LTC3901 state table and produces hardware output intents. It does not call BSP or drivers directly. `fw_app.c` supplies sampled inputs and applies outputs.

The LTC3901 manager runtime configuration is stored in `fw_app.c`, initialized from compile-time defaults, and consumed by the manager on each superloop pass. The current JSON interface supports partial runtime updates through direct `SET args.ltc3901.<field>` values and full readback through `GET args:["ltc3901"]`.

External LTC3901 commands are accepted through `SET args.ltc3901_cmd`:

- `RUN`: transition from the current state to `POWER_UP`, then continue through normal manager flow
- `HALT`: transition from the current state to `HALT`; outputs are disabled and fault counters are preserved
- `RESET`: transition from the current state to `RESET`; outputs are disabled and manager fault counters are cleared

Current manager states are:

- `RESET`
- `HALT`
- `POWER_UP`
- `POWER_FAULT`
- `POWERED`
- `POWERED_SYNC_ON`
- `POWERED_SYNC_OFF`
- `POWERED_SYNC_FAULT`

`POWERED_SYNC_OFF` keeps LTC3901 power enabled and disables only the sync outputs.

### `lt8316_manager.*`

`lt8316_manager` owns the DUT2 LT8316 state table and produces the HV power-enable output intent. It does not call BSP or drivers directly. `fw_app.c` supplies sampled inputs and applies outputs.

The LT8316 manager runtime configuration is stored in `fw_app.c`, initialized from compile-time defaults, and consumed by the manager on each superloop pass. The current JSON interface supports partial runtime updates through direct `SET args.lt8316.<field>` values and full readback through `GET args:["lt8316"]`.

External LT8316 commands are accepted through `SET args.lt8316_cmd`. The compatibility debug signal `lt8316.pwr_en` maps `true` to `RUN` and `false` to `RESET`:

- `RUN`: transition from the current state to `POWERED`
- `RESET`: transition from the current state to `RESET`; HV power is disabled and manager fault counters are cleared

Current manager states are:

- `RESET`
- `FAULT`
- `POWERED`

`POWERED` asserts `HV_Pwr_En`. If the LT8316 gate frequency remains unavailable after the configured power-on stabilization time, the manager transitions to `FAULT`, disables `HV_Pwr_En`, increments its fault counter, and retries while the fault count remains below the configured limit. A `power_fault_max` value of `0` disables the retry-count limit and allows indefinite LT8316 retries.

## Services Modules

### `Services/CommandHandler`

The command handler owns JSON object framing, parsing, dispatch, response formatting, and command-time validation. It supports date/time, status period, debug telemetry configuration, ADC calibration, raw/debug signal reads, and debug digital signal writes.

Command handlers do not directly touch BSP or drivers. They call `fw_app_*` facade functions for application-owned behavior.

`hc_datetime.*` lives in this folder and provides command/status-facing timestamp formatting and fallback handling. JSON responses, asynchronous `EVT` output, and periodic `STS` output use the HC datetime string format `YYYYMMDD HH:MM:SS`.

The firmware exposes a compile-time software version string through `GET args:["sw_version"]`. The default is `SW_VERSION_STRING`, which can be overridden by the build system with a compiler definition.

Manager-generated `EVT` records currently use an `args.msg` text payload prefixed with the related device name, for example `LTC3901:` or `LT8316:`. Fault messages include measurement evidence and the configured comparison value where available; retry messages include retry count and maximum retry count.

## Layer boundary examples

### Belongs in BSP

- turn red LED on
- disable LT8316 power
- read BeamOn input
- read packed board ID

### Belongs in Drivers_Local

- sample all ADC channels
- convert a capture register set into frequency and duty-cycle data
- configure TIM3 raw values for SDRA/SDRB generation
- enable or disable synchronized waveform generation

### Does not belong in BSP or Drivers_Local

- decide whether a measured voltage is a latched fault
- decide whether BeamOn should inhibit a restart sequence
- format a status line for CLI or USB reporting
- coordinate application state transitions

## Current Implementation Status

The following custom modules are currently implemented and included in the firmware build:

- `App/fw_app.*`
- `App/hc_app_status.*`
- `App/hc_debug_telemetry.*`
- `App/ltc3901_manager.*`
- `App/lt8316_manager.*`
- `Bsp/bsp_board.*`
- `Drivers_Local/adc_sense_drv.*`
- `Drivers_Local/pwm_capture_drv.*`
- `Drivers_Local/reset_reason_drv.*`
- `Drivers_Local/rtc_drv.*`
- `Drivers_Local/sync_drv.*`
- `Drivers_Local/usb_vcp_drv.*`
- `Services/CommandHandler/*`
- `Services/RingBuffer/*`
- `Services/jsmn/*`

Near-term remaining architecture work is:

- replace provisional LTC3901 and LT8316 manager timing/threshold constants with named configuration variables
- decide whether current DUT manager `EVT` messages should gain structured event IDs, scopes, severity, or fault-manager records
- add a centralized fault manager when fault ID ownership and latching policy are finalized
- add non-volatile storage for ADC calibration if required
- tighten or remove the remaining legacy `fw_app_set_sync_enable()` facade once no callers require it

## Current ADC implementation status

The current `Drivers_Local/adc_sense_drv.*` implementation:

- starts and rearms ADC1 regular conversions using DMA from the superloop
- retain the latest full 8-channel sample frame
- expose stable channel enumeration for the configured ADC input order
- provide raw sample access and nominal pin-level millivolt conversion
- support per-channel engineering-unit conversion from raw counts using configurable `SlopeScaled` and `Offset` factors

ADC acquisition uses CubeMX's normal-mode DMA configuration. `adc_sense_drv_init()` starts the first 8-channel frame. The DMA completion callback only marks the frame valid and requests a rearm; `adc_sense_drv_task()` performs the stop/restart work from `fw_app_run()` so ADC rearming is not done inside the DMA ISR.

Engineering-unit conversion details:

- conversion uses `engineering = ((SlopeScaled * raw_counts) / ADC_SENSE_CALIBRATION_SLOPE_SCALE) + Offset`
- each ADC channel owns an independent calibration entry
- `Valid = false` disables engineering-unit output for that channel
- calibration storage is currently RAM-only
- `VUpstream_Anlg` and `LTC3901_Vcc_Anlg` default to valid engineering calibration for their 100 k / 37.4 k input dividers; clearing those channels restores the divider defaults

Not yet implemented in this driver:

- calibrated VDDA compensation using VREFINT
- fault thresholds or policy decisions
- non-volatile storage for calibration factors
