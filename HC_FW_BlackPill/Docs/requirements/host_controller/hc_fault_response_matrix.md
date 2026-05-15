# Host Controller Implemented Fault and Event Responses v1

## Purpose

This document records fault-like responses that are implemented in the current
firmware. It intentionally excludes proposed LLF/HLF/WRN taxonomies, centralized
fault IDs, fault-query commands, and fault-clear policy because those mechanisms
are not implemented.

Current fault-like behavior is owned by the DUT-local managers:

- `LTC3901_Manager`
- `LT8316_Manager`

Manager state is visible in `STS`. Notable transitions are emitted as `EVT`
records with a text message in `args.msg`.

## Implemented Safe Defaults

On board safe-state entry:

- LT8316 HV enable is disabled
- LTC3901 power enable is disabled
- board LEDs are turned off

## LTC3901 Manager Responses

| Condition | Detection | Manager response | Output action | Reported visibility | Recovery behavior |
|---|---|---|---|---|---|
| Supply current too high | `isupply_ma >= isupply_ma_max` | transition to `POWER_FAULT` | disable LTC3901 power and sync outputs | `EVT` message and `STS.duts.LTC3901.state` | retry after `power_retry_delay_ms` while retry is allowed |
| Power-up timeout | `POWER_UP` exceeds `power_up_timeout_ms` | transition to `POWER_FAULT` | disable LTC3901 power and sync outputs | `EVT` message and `STS.duts.LTC3901.state` | retry after `power_retry_delay_ms` while retry is allowed |
| Upstream voltage too low | powered state and `vupstream_mv < vupstream_mv_min` or invalid | transition to `POWER_FAULT` | disable LTC3901 power and sync outputs | `EVT` message and `STS.duts.LTC3901.state` | retry after `power_retry_delay_ms` while retry is allowed |
| LTC3901 VCC too low | powered state and `ltc3901_vcc_mv < ltc3901_vcc_mv_min` or invalid | transition to `POWER_FAULT` | disable LTC3901 power and sync outputs | `EVT` message and `STS.duts.LTC3901.state` | retry after `power_retry_delay_ms` while retry is allowed |
| ME activity missing | `POWERED_SYNC_ON` and ME frequency unavailable after `sync_stabilization_time_ms` | transition to `POWERED_SYNC_FAULT` | keep LTC3901 power enabled; disable sync outputs | `EVT` message and `STS.duts.LTC3901.state` | retry sync after `sync_fault_delay_ms` |
| MF activity missing | `POWERED_SYNC_ON` and MF frequency unavailable after `sync_stabilization_time_ms` | transition to `POWERED_SYNC_FAULT` | keep LTC3901 power enabled; disable sync outputs | `EVT` message and `STS.duts.LTC3901.state` | retry sync after `sync_fault_delay_ms` |

`power_fault_max = 0` disables the LTC3901 power retry-count limit.

## LT8316 Manager Responses

| Condition | Detection | Manager response | Output action | Reported visibility | Recovery behavior |
|---|---|---|---|---|---|
| Gate activity missing | `POWERED` and gate frequency unavailable after `power_on_stabilization_time_ms` | transition to `FAULT` | disable LT8316 HV power | `EVT` message and `STS.duts.LT8316.state` | retry after `power_retry_delay_ms` while retry is allowed |

`power_fault_max = 0` disables the LT8316 retry-count limit.

## Commanded Recovery

| Manager | Command | Effect |
|---|---|---|
| Host Controller | `SET args.hc_cmd:"RESET"` | acknowledge command, then perform MCU software reset without resetting RTC date/time |
| `LTC3901_Manager` | `SET args.ltc3901_cmd:"RESET"` | transition to `RESET`, disable outputs, clear manager fault counters |
| `LTC3901_Manager` | `SET args.ltc3901_cmd:"HALT"` | transition to `HALT`, disable outputs, preserve manager fault counters |
| `LTC3901_Manager` | `SET args.ltc3901_cmd:"RUN"` | transition to `POWER_UP` and restart normal manager flow |
| `LT8316_Manager` | `SET args.lt8316_cmd:"RESET"` | transition to `RESET`, disable HV output, clear manager fault counter |
| `LT8316_Manager` | `SET args.lt8316_cmd:"RUN"` | transition to `POWERED` |

## Not Implemented

The following concepts are not part of the current firmware fault response
implementation:

- centralized LLF / HLF / WRN fault classes
- stable fault IDs
- fault query commands
- fault clear commands
- top-level fault latching
- LED fault-code mapping
- top-level `LOW_LEVEL_FAULT` or `HIGH_LEVEL_FAULT` states
