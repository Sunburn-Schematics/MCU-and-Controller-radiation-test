# Host Controller Firmware Architecture v1

## Purpose

This document describes the architecture implemented by the current STM32 Host
Controller firmware.

## Platform

| Item | Implementation |
|---|---|
| MCU | STM32F411xE |
| Board | DFRobot BlackPill |
| Scheduler | Bare-metal main loop using HAL tick timing |
| RTOS | None |
| Build system | CMake |
| Toolchain | `gcc-arm-none-eabi` |
| Primary programming/debug | ST-Link over SWD |
| Primary TE transport | USB CDC/VCP |

## Main Application Flow

The firmware runs from the generated STM32 startup and `main()` flow into
`fw_app_init()` and repeated `fw_app_run()` calls.

The main loop services:

- board and driver state
- measurement acquisition and scaling
- USB JSON command processing
- `LTC3901_Manager`
- `LT8316_Manager`
- periodic `STS` reporting
- optional periodic `DBG` telemetry
- asynchronous `EVT` messages from manager events
- LED heartbeat behavior

## Layering

| Layer | Current modules / responsibilities |
|---|---|
| BSP | board pin control, safe-state entry, hardware ID, Beam On input |
| Drivers | ADC sensing, PWM/input capture, RTC, reset reason, USB VCP |
| Services | JSON parsing/dispatch/response formatting, date/time helpers, jsmn utilities |
| Application | `fw_app`, debug telemetry, LTC3901 and LT8316 managers |
| Tools | guarded build, ST-Link programming, hardware test automation |

## Safety Defaults

During safe-state entry:

- LT8316 HV enable is disabled
- LTC3901 power enable is disabled
- LEDs are turned off

Manager states also drive output intents so fault-like DUT conditions disable the
affected outputs as documented in `hc_state_machine_spec.md` and
`hc_fault_response_matrix.md`.

## Command Architecture

The TE command interface is USB CDC/VCP carrying JSON objects.

TE-originated message types:

- `GET`
- `SET`

HC-originated message types:

- `RSP`
- `STS`
- `DBG`
- `EVT`

The command surface is field-based. New TE-visible behavior should normally be
added as a new `GET args` field name or `SET args.<field>` capability rather than a
new verb-style command.

## Reporting Architecture

`STS` is the canonical status summary. It uses this top-level shape:

```json
{
  "type": "STS",
  "hc_id": 4,
  "ts": "YYYYMMDD HH:MM:SS",
  "beam_on": true,
  "duts": {
    "LTC3901": {},
    "LT8316": {}
  }
}
```

`DBG` telemetry is disabled by default and is configured with `dbg_period_ms` and
`dbg_signals`.

`EVT` records are emitted for notable DUT-manager transitions and contain a text
message in `args.msg`.

## Manager Architecture

The current firmware uses DUT-local managers rather than a centralized
top-level fault/state manager.

| Manager | Source | Responsibility |
|---|---|---|
| `LTC3901_Manager` | `App/ltc3901_manager.*` | LTC3901 power qualification, sync cycling, fault-like power/sync handling, output intents |
| `LT8316_Manager` | `App/lt8316_manager.*` | LT8316 HV enable, gate-activity supervision, retry behavior, output intents |

The manager state machines are documented in
`hc_state_machine_spec.md`.

## Reset and RTC Architecture

Reset-cause capture is handled by `Drivers_Local/reset_reason_drv.*`.

RTC startup policy is handled in the CubeMX-generated RTC initialization user
sections. The default RTC date/time is only rewritten after a power-on reset.
Runtime date/time is command-visible through `GET args:["date_time"]` and `SET args.date_time`.

## Test Architecture

Hardware tests are data-driven:

- cases live in `Test/hw_test_cases.json`
- execution is handled by `Tools/test.ps1`
- durable reports are written under `Test/Reports/`

The test regime is documented in `hc_protocol_test_plan.md`.

## Not Implemented

The current architecture does not include:

- a centralized application lifecycle manager
- centralized LLF/HLF/WRN fault classes
- stable fault IDs
- fault query/clear commands
- verb-style TE commands
- persisted runtime configuration
