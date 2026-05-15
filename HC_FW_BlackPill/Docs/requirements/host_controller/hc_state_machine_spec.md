# Host Controller Manager State Machines v1

## Purpose

This document describes the state machines implemented in the current Host
Controller firmware. The firmware does not implement a centralized top-level
`BOOT` / `LOW_LEVEL_FAULT` / `HIGH_LEVEL_FAULT` / `SLAVE` state machine. Runtime
supervision is implemented through:

- the bare-metal `fw_app_run()` loop
- periodic `STS`, `DBG`, and `EVT` reporting
- the `LTC3901_Manager`
- the `LT8316_Manager`

The legacy term `Monitor` has been retired for these DUT-local state machines.
Use `Manager` terminology in requirements, tests, and firmware-facing
documentation.

## Top-Level Application State

The implemented protocol does not expose a top-level application state. Runtime
state visibility is provided through the DUT manager state fields in `STS`.

Startup behavior:
- board support enters safe state during initialization
- LT8316 HV enable is disabled
- LTC3901 power enable is disabled
- LED outputs are off until application logic changes them
- the 6-bit hardware ID is read from GPIO and reported as `hc` / `hc_id`
- `BeamOn` is read as a GPIO input and reported in `STS.beam_on`

Autostart behavior:
- after the application autostart delay expires, managers that are still in
  `RESET` may receive an internal start request
- autostart does not define a separate top-level HC state

## Common Manager Rules

Managers run from the main loop, not from interrupt context.

Runtime commands are accepted through JSON `SET args` fields:

| Scope | JSON command field | Valid commands |
|---|---|---|
| Host Controller | `hc_cmd` | `RESET` |
| `LTC3901_Manager` | `ltc3901_cmd` | `RUN`, `HALT`, `RESET` |
| `LT8316_Manager` | `lt8316_cmd` | `RUN`, `RESET` |

`hc_cmd:"RESET"` acknowledges the request and then performs a deferred MCU
software reset. The RTC date/time is preserved across the restart.

Manager state is reported in:

- `STS.duts.LTC3901.state`
- `STS.duts.LT8316.state`
- optional `DBG` telemetry when configured
- asynchronous `EVT` records when notable transitions or fault-like events occur

Manager configuration is RAM-resident and is changed through:

- `GET args:["ltc3901"]` / direct `SET args.ltc3901.<field>`
- `GET args:["lt8316"]` / direct `SET args.lt8316.<field>`

Configuration changes are not persisted across reset.

## `LTC3901_Manager`

### States

| State | Output intent |
|---|---|
| `RESET` | LTC3901 power disabled; sync outputs disabled; fault counters cleared |
| `HALT` | LTC3901 power disabled; sync outputs disabled; fault counters preserved |
| `POWER_UP` | LTC3901 power enabled; sync outputs disabled |
| `POWER_FAULT` | LTC3901 power disabled; sync outputs disabled; power fault counter incremented on entry |
| `POWERED` | LTC3901 power enabled; sync outputs disabled |
| `POWERED_SYNC_ON` | LTC3901 power enabled; sync outputs enabled |
| `POWERED_SYNC_OFF` | LTC3901 power enabled; sync outputs disabled |
| `POWERED_SYNC_FAULT` | LTC3901 power enabled; sync outputs disabled; sync fault counter incremented on entry |

### External Commands

| Command | Behavior |
|---|---|
| `RUN` | Transitions to `POWER_UP` and begins the normal power qualification flow. |
| `HALT` | Transitions to `HALT`; disables power and sync outputs while preserving fault counters. |
| `RESET` | Transitions to `RESET`; disables power and sync outputs and clears fault counters. |

`HALT` is entered only by external command. Internal fault or timing transitions
do not enter `HALT`.

### Transitions

| Current state | Event / condition | Next state | Event report |
|---|---|---|---|
| `RESET` | `RUN` command | `POWER_UP` | `Entering POWER_UP` |
| `HALT` | `RUN` command | `POWER_UP` | `Entering POWER_UP` |
| `HALT` | `RESET` command | `RESET` | none |
| `POWER_UP` | `isupply_ma >= isupply_ma_max` | `POWER_FAULT` | `Isupply Current too high` |
| `POWER_UP` | `power_up_timeout_ms` elapsed | `POWER_FAULT` | `Power Up Timeout` |
| `POWER_UP` | upstream voltage, LTC3901 VCC, and current checks pass | `POWERED` | `Powered` |
| `POWER_FAULT` | `RESET` command | `RESET` | none |
| `POWER_FAULT` | `RUN` command | `POWER_UP` | `Entering POWER_UP` |
| `POWER_FAULT` | retry delay elapsed and retry allowed | `POWER_UP` | `Retrying Power Up` |
| `POWERED` | current, upstream voltage, or LTC3901 VCC check fails | `POWER_FAULT` | measured fault event |
| `POWERED` | `sync_on_delay_ms` elapsed | `POWERED_SYNC_ON` | `Cycle Sync ON` |
| `POWERED_SYNC_ON` | current, upstream voltage, or LTC3901 VCC check fails | `POWER_FAULT` | measured fault event |
| `POWERED_SYNC_ON` | ME activity missing after stabilization | `POWERED_SYNC_FAULT` | `ME Stopped` |
| `POWERED_SYNC_ON` | MF activity missing after stabilization | `POWERED_SYNC_FAULT` | `MF Stopped` |
| `POWERED_SYNC_ON` | `sync_hold_on_time_ms` elapsed | `POWERED_SYNC_OFF` | `Cycle Sync OFF` |
| `POWERED_SYNC_OFF` | current, upstream voltage, or LTC3901 VCC check fails | `POWER_FAULT` | measured fault event |
| `POWERED_SYNC_OFF` | `sync_hold_off_time_ms` elapsed | `POWERED_SYNC_ON` | `Cycle Sync ON` |
| `POWERED_SYNC_FAULT` | current, upstream voltage, or LTC3901 VCC check fails | `POWER_FAULT` | measured fault event |
| `POWERED_SYNC_FAULT` | `sync_fault_delay_ms` elapsed | `POWERED_SYNC_ON` | `Cycle Sync ON` |

### Retry Rule

`power_fault_max = 0` disables the power retry-count limit. In that
configuration, `POWER_FAULT` retries indefinitely after each
`power_retry_delay_ms` interval until power-up succeeds or a command changes the
state.

## `LT8316_Manager`

### States

| State | Output intent |
|---|---|
| `RESET` | LT8316 HV power disabled; fault counter cleared |
| `POWERED` | LT8316 HV power enabled |
| `FAULT` | LT8316 HV power disabled; fault counter incremented on entry |

### External Commands

| Command | Behavior |
|---|---|
| `RUN` | Transitions to `POWERED`. |
| `RESET` | Transitions to `RESET`; disables HV power and clears fault counter. |

### Transitions

| Current state | Event / condition | Next state | Event report |
|---|---|---|---|
| `RESET` | `RUN` command | `POWERED` | `Entering POWERED` |
| `FAULT` | `RESET` command | `RESET` | none |
| `FAULT` | `RUN` command | `POWERED` | `Entering POWERED` |
| `FAULT` | retry delay elapsed and retry allowed | `POWERED` | `Retrying Power Up` |
| `POWERED` | gate frequency missing after stabilization | `FAULT` | `GATE Stopped` |

### Retry Rule

`power_fault_max = 0` disables the LT8316 retry-count limit. In that
configuration, `FAULT` retries indefinitely after each
`power_retry_delay_ms` interval until gate activity is detected or a command
changes state.

## Reporting

Manager states are visible in the canonical `STS` payload:

```json
{
  "type": "STS",
  "hc_id": 4,
  "ts": "YYYYMMDD HH:MM:SS",
  "beam_on": true,
  "duts": {
    "LTC3901": { "state": "RESET" },
    "LT8316": { "state": "RESET" }
  }
}
```

Manager events are emitted as `EVT` records:

```json
{"type":"EVT","hc":1,"ts":"20260501 10:30:00","args":{"msg":"LTC3901: Cycle Sync ON"}}
```

## Source Alignment

This document incorporates the previous LTC3901 and LT8316 state-transition
tables using the current `Manager` nomenclature. The older source tables were
removed after their implemented behavior was merged here.
