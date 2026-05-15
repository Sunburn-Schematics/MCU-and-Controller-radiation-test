# Host Controller Implemented Product Requirements v1

## Purpose

This document captures product requirements that are represented in the current
Host Controller firmware. Obsolete draft content has been removed from this
current requirements view.

## Requirements

| ID | Requirement | Current implementation |
|---|---|---|
| HC-REQ-001 | The firmware shall target the STM32F411xE DFRobot BlackPill hardware. | CMake target and STM32 project configuration. |
| HC-REQ-002 | The firmware shall run as bare-metal firmware without an RTOS. | `fw_app_run()` services cooperative tasks from the main loop. |
| HC-REQ-003 | The board shall enter a safe output state during initialization. | BSP safe state disables LT8316 HV enable, disables LTC3901 power enable, and turns LEDs off. |
| HC-REQ-004 | The firmware shall read the 6-bit hardware ID from GPIO. | Hardware ID is reported as `RSP.hc` and `STS.hc_id`. |
| HC-REQ-005 | The firmware shall read the Beam On input. | Beam On is reported in `STS.beam_on`. |
| HC-REQ-006 | The TE interface shall use USB CDC/VCP with JSON request/response objects. | Implemented through USB VCP driver and command handler. |
| HC-REQ-007 | TE-originated commands shall use JSON `GET` with an `args` field-name array and JSON `SET` with an `args` field object. | Implemented by the command parser and dispatcher. |
| HC-REQ-008 | HC responses shall use `RSP` objects with request correlation. | `RSP.msg` mirrors the request `msg` when valid. |
| HC-REQ-009 | The firmware shall emit periodic `STS` status reports. | Default period is 1000 ms; configurable with `sts_period_ms`; `0` disables reports. |
| HC-REQ-010 | The firmware shall support configurable periodic `DBG` telemetry. | `dbg_period_ms` and `dbg_signals` configure debug telemetry. |
| HC-REQ-011 | The firmware shall emit `EVT` records for notable manager events. | Manager transitions and fault-like events emit `EVT.args.msg`. |
| HC-REQ-012 | The firmware shall support runtime date/time set and readback. | `SET args.date_time` and `GET args:["date_time"]`. |
| HC-REQ-013 | The firmware shall support software version readback. | `GET args:["sw_version"]`. |
| HC-REQ-014 | The firmware shall support ADC sample readback and RAM-resident ADC calibration. | ADC samples are read with direct debug signal names such as `GET args:["adc.vupstream.raw"]`; calibration is read and written with direct fields such as `adc.vupstream.offset`. |
| HC-REQ-015 | The firmware shall supervise LTC3901 behavior through `LTC3901_Manager`. | Implemented in `App/ltc3901_manager.*`. |
| HC-REQ-016 | The firmware shall supervise LT8316 behavior through `LT8316_Manager`. | Implemented in `App/lt8316_manager.*`. |
| HC-REQ-017 | Manager runtime configuration shall be accessible through JSON fields. | `GET args:["ltc3901"]` / direct `SET args.ltc3901.<field>`, `GET args:["lt8316"]` / direct `SET args.lt8316.<field>`. |
| HC-REQ-018 | Runtime configuration changes shall be RAM-resident. | Configuration resets to compile-time defaults after reset/reprogramming. |
| HC-REQ-019 | Reset reason shall be captured before RTC startup policy uses it. | `reset_reason_drv_init()` captures and clears reset flags. |
| HC-REQ-020 | Default RTC date/time initialization shall only occur after power-on reset. | RTC initialization policy branches on retained reset reason. |
| HC-REQ-021 | Hardware test execution shall be data-driven and produce durable reports. | `Tools/test.ps1`, `Test/hw_test_cases.json`, and `Test/Reports/`. |

## Non-Requirements for Current Firmware

The following are outside the current firmware requirements:

- verb-style TE commands
- centralized top-level `BOOT`, `LOW_LEVEL_FAULT`, `HIGH_LEVEL_FAULT`, or `SLAVE`
  state behavior
- centralized LLF/HLF/WRN fault taxonomy
- persisted runtime configuration
- external beamline control
- external HV supply protocol control

If any equivalent behavior is added, it should be captured as a new explicit
requirement and aligned with the existing JSON `GET args` array / `SET args`
object protocol.
