# Host Controller Implementation Alignment - 2026-05-15

## Purpose

This document records the current implementation state of the Host Controller
firmware so the requirements documents in this folder can be read accurately.

The firmware has moved beyond the early BMAD draft stage. This alignment note is
the current reference for what is implemented in the STM32 project as of
2026-05-15.

## Implemented Firmware Shape

- Target: STM32F411xE on the DFRobot BlackPill hardware.
- Scheduler: bare-metal main loop using HAL tick timing; no RTOS.
- Primary TE interface: USB CDC/VCP carrying JSON request/response objects.
- Programming/debug path: ST-Link over SWD.
- Board support:
  - safe-state entry disables LT8316 HV enable, disables LTC3901 power enable,
    and turns off board LEDs
  - 6-bit hardware ID is read from GPIO and reported as `hc` / `hc_id`
  - `BeamOn` is read as a GPIO input and reported in `STS`
- Periodic outputs:
  - blue heartbeat LED toggles every 500 ms
  - periodic `STS` defaults to 1000 ms and can be disabled with
    `args.sts_period_ms = 0`
  - periodic `DBG` is disabled by default and is configured through
    `dbg_period_ms` and `dbg_signals`
- Asynchronous event reporting:
  - DUT manager transitions and fault-like conditions emit `EVT` messages
    containing a string in `args.msg`

## Implemented Command Protocol

Request packets are JSON objects with:

- `type`: currently `SET` or `GET` for TE-originated commands
- `msg`: numeric request correlation value
- `args`: command-specific object

Response packets are JSON objects with:

- `type:"RSP"`
- `hc`: current hardware-controller ID
- `msg`: mirrored request ID when the request had a valid numeric `msg`
- `ts`: `YYYYMMDD HH:MM:SS`
- `args` on success or `error` on failure

Implemented `SET args` fields:

- `date_time`
- `sts_period_ms`
- `dbg_period_ms`
- `dbg_signals` as an array for debug telemetry selection
- direct `SET args` signal fields for settable digital outputs
- `hc_cmd`
- direct `adc.<channel>.<field>` calibration fields
- `ltc3901_cmd`
- `lt8316_cmd`
- `ltc3901`
- `lt8316`

Implemented `GET args` fields:

- `date_time`
- `sw_version`
- `dbg_period_ms`
- `dbg_signals`
- direct debug signal names, including ADC sample names
- direct `adc.<channel>.<field>` calibration fields
- `ltc3901`
- `lt8316`

Retired verb-style command concepts:

- standalone identity/capability commands
- top-level mode commands
- centralized fault query/clear commands
- explicit TE session/connect-state commands

These should not be treated as command names. Equivalent functionality is
expressed through the existing JSON `GET` / `SET args` model or through
HC-originated `STS`, `DBG`, and `EVT` records.

## Implemented Status and Telemetry Messages

`STS` shape:

```json
{
  "type": "STS",
  "hc_id": 4,
  "ts": "YYYYMMDD HH:MM:SS",
  "beam_on": true,
  "duts": {
    "LTC3901": {
      "state": "RESET",
      "pwr_en": false,
      "sync": false,
      "vsupply": 23,
      "vshunt": 47,
      "isupply": null,
      "me_freq": null,
      "me_ratio": null,
      "me_anlg": 0,
      "mf_freq": null,
      "mf_ratio": null,
      "mf_anlg": 0
    },
    "LT8316": {
      "state": "RESET",
      "pwr_en": false,
      "gate_freq": 1537,
      "gate_anlg": 7,
      "vout": 215
    }
  }
}
```

`DBG` shape:

```json
{
  "type": "DBG",
  "hc_id": 4,
  "ts": "YYYYMMDD HH:MM:SS",
  "signals": {
    "adc.vupstream.raw": 8
  }
}
```

`EVT` shape:

```json
{
  "type": "EVT",
  "hc": 4,
  "ts": "YYYYMMDD HH:MM:SS",
  "args": {
    "msg": "LTC3901: Cycle Sync ON"
  }
}
```

## Implemented DUT Supervision

The firmware currently implements DUT-local managers rather than the full
top-level fault/state model described in the early v1 documents.

LTC3901 manager:

- commands: `RUN`, `HALT`, `RESET`
- states: `RESET`, `HALT`, `POWER_UP`, `POWER_FAULT`, `POWERED`,
  `POWERED_SYNC_ON`, `POWERED_SYNC_OFF`, `POWERED_SYNC_FAULT`
- owns LTC3901 power-enable requests and sync enable/disable
- checks supply/current conditions, power-up timeout, missing ME/MF activity
  after sync stabilization, and sync-cycle timing
- emits scoped `EVT` strings for notable state/fault/retry events

LT8316 manager:

- commands: `RUN`, `RESET`
- states: `RESET`, `POWERED`, `FAULT`
- owns LT8316 HV power-enable requests
- checks gate-frequency validity after the configured stabilization interval
- emits scoped `EVT` strings for notable state/fault/retry events

Both managers expose runtime configuration through JSON `GET` / `SET` command
fields. Configuration is RAM-resident and not persisted across reset.

## Not Implemented

The following concepts are not implemented in the current firmware:

- centralized top-level application state machine
- full fault manager with fault IDs, latching, clear preconditions, and
  fault-detail query responses
- TE command legality matrix by top-level application state
- debug UART command terminal on PA9/PA10
- explicit TE connection-active status model
- capability discovery responses
- durable nonvolatile configuration storage
- complete automated verification of all fault classes

The current runtime does not expose a top-level application state. Status
visibility is provided by the DUT manager states and the other `STS` fields.

## Current Automated Verification

Hardware-in-the-loop JSON protocol tests live in:

- `Test/hw_test_cases.json`
- `Tools/test.ps1`
- generated durable reports under `Test/Reports/`

The latest comprehensive run at the time of this alignment pass exercised 58
cases and produced 56 passes / 2 failures. The failures were intentionally left
for human triage:

- unquoted JSON object key was accepted rather than rejected as `BAD_JSON`
- two back-to-back valid JSON objects in one input stream produced no response

## How to Read the Requirements Documents

- Treat this alignment document and the implementation source as the current
  truth for implemented behavior.
- Treat `hc_te_interface_spec.md`, `Docs/JSON_Tests.md`, and
  `Test/hw_test_cases.json` as the most concrete description of the implemented
  TE protocol.
