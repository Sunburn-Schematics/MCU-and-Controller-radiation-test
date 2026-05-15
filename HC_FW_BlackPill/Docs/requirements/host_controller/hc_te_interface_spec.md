# Radiation Test Host Controller (HC) TE Interface / Command Specification v1

## Current Implementation Status - 2026-05-15

The implemented TE interface is USB CDC/VCP with JSON request/response objects.
Implemented TE-originated packet types are `SET` and `GET`. HC-originated packet
types are periodic `STS`, periodic `DBG`, and asynchronous `EVT`.

The implemented command surface is the one described by `Docs/JSON_Tests.md` and
exercised by `Test/hw_test_cases.json`. Retired verb-style command names are not
part of the intended protocol direction. Their useful functionality is served by
the simpler JSON `GET` / `SET` command construct and by periodic `STS`, `DBG`,
and `EVT` records.

Implemented `SET args` fields:

| Field | Implemented behavior |
|---|---|
| `date_time` | Sets RTC-backed date/time in `YYYYMMDD HH:MM:SS` format. |
| `sts_period_ms` | Configures periodic `STS`; `0` disables periodic `STS`. |
| `dbg_period_ms` | Configures periodic `DBG`; valid values are `0` or `100..60000`. |
| `dbg_signals` array | Selects periodic debug telemetry signals. |
| `hc_cmd` | Applies host-controller commands. Currently supports `RESET`. |
| direct signal fields | Set supported digital outputs |
| direct `adc.<channel>.<field>` calibration fields | Set RAM-resident ADC calibration for a supported channel name. |
| `ltc3901_cmd` | Applies `RUN`, `HALT`, or `RESET` to the LTC3901 manager. |
| `lt8316_cmd` | Applies `RUN` or `RESET` to the LT8316 manager. |
| direct `ltc3901.<field>` config fields | Partially update RAM-resident LTC3901 manager configuration. |
| direct `lt8316.<field>` config fields | Partially update RAM-resident LT8316 manager configuration. |

Implemented `GET args` fields:

| Field | Implemented behavior |
|---|---|
| `date_time` | Returns current RTC-backed date/time. |
| `sw_version` | Returns compiled firmware version string. |
| `dbg_period_ms` / `dbg_signals` | Returns debug telemetry configuration or a requested signal sample. |
| direct debug signal names | Return one-shot debug samples, including ADC samples such as `adc.vupstream.raw`. |
| direct `adc.<channel>.<field>` calibration fields | Return RAM-resident ADC calibration fields by supported channel name. |
| `ltc3901` | Returns LTC3901 manager configuration. |
| `lt8316` | Returns LT8316 manager configuration. |

## 1. Document Purpose
This document defines the interface between the Test Executive (TE) and the Radiation Test Host Controller (HC).

It describes:
- the transport and session model
- message framing approach
- JSON `GET` / `SET` command and response semantics
- periodic status reporting behavior
- error handling expectations
- interaction rules consistent with the HC PRD, Fault Response Matrix, Variable Registry, and Firmware Architecture

This document has been realigned to the implemented JSON request/response model.
The field set may grow over time, but new TE-originated actions should normally
be added as `GET args` field names or `SET args.<field>` fields rather than new
top-level verb commands.

## 2. Interface Summary
| Item | Definition |
|---|---|
| Primary TE interface | USB Virtual COM Port (VCP) |
| Primary interaction model | JSON request / response over USB VCP |
| HC periodic behavior | In Normal operation, HC sends a summary status message to TE once per second |
| TE command behavior | TE sends `GET` or `SET` JSON objects to HC |
| Beam On restrictions | None |
| Debug relation to TE link | Debug may report whether USB VCP link is active |

## 3. Interface Goals
The TE interface shall:
- provide deterministic supervisory control of the HC
- allow the TE to observe HC state, DUT state, and fault state
- allow the TE to control implemented DUT manager actions subject to HC policy
- support stable machine parsing
- preserve traceability between protocol elements and HC requirements

## 4. Transport Model
### 4.1 Physical / Logical Transport
The TE and HC communicate over USB using a Virtual COM Port (VCP) interface.

### 4.2 Session Model
- the HC may operate even if no TE connection is currently active
- successful USB/VCP stack initialization is required for the HC to support TE communications
- absence of an active TE connection after successful initialization is a degraded condition only
- the TE should be able to connect, disconnect, and reconnect without requiring HC reset unless a broader system rule later requires otherwise

### 4.3 Connection State Exposure
The HC shall maintain a TE-link-active indication for reporting and debug visibility.

## 5. Message Model
The TE protocol uses complete JSON objects over the USB VCP byte stream. The
current parser frames input by balanced top-level JSON objects rather than by
requiring a newline terminator. Newline-delimited JSON remains useful for human
logs and scripts, but newline is not the semantic frame boundary.

### 5.1 Baseline Framing
For TE-issued commands:
- one complete JSON object per request
- `type` is `GET` or `SET`
- `msg` is a TE-supplied numeric correlation value
- `GET args` is an array containing one or more field names
- `SET args` is an object containing one or more request fields

For HC-issued reports:
- one complete JSON object per line
- newline-delimited output over VCP
- field names should remain stable across firmware revisions where possible

### 5.2 Message Types
| Message Type | Purpose |
|---|---|
| `GET` | TE-originated read/query request |
| `SET` | TE-originated write/action request |
| `RSP` | HC response to a `GET` or `SET` request |
| `EVT` | asynchronous event indication |
| `STS` | periodic status report |
| `DBG` | periodic debug telemetry report |

Errors are reported as `RSP` objects with an error result in `args`; there are no
separate `OK` or `ERR` top-level message types in the implemented protocol.

### 5.3 Request Shape
Canonical request shape:

```json
{"type":"GET","msg":1,"args":["sw_version"]}
```

```json
{"type":"SET","msg":2,"args":{"sts_period_ms":1000}}
```

The command `type` selects the operation class. The `args` field names select the
data or action being requested. This is the canonical command model.

### 5.4 Response Shape
Canonical response shape:

```json
{"type":"RSP","hc":1,"msg":1,"ts":"20260501 10:30:00","args":{"sw_version":"..."}}
```

The `msg` value echoes the TE request for correlation. The `hc` value identifies
the responding controller. `ts` is the HC RTC-backed timestamp string.

### 5.5 Periodic and Asynchronous Reports
The primary routine supervisory message is periodic `STS`. The firmware also
supports periodic `DBG` records when configured and asynchronous `EVT` records
for DUT-manager transitions and fault-like events.

## 6. Command Processing Model
### 6.1 Shared Semantics
TE requests are dispatched from the JSON `type` and `args` fields. Where debug or
internal mechanisms exercise the same hardware action, they should route through
the same core action handlers wherever possible.

This ensures:
- identical safety/policy behavior
- consistent DUT-manager command behavior
- consistent validation and response behavior

### 6.2 Command Outcome Classes
| Outcome | Meaning |
|---|---|
| Accepted | request is valid and action completed |
| Rejected | request is invalid for current state/policy |
| Failed | request was valid but action could not be completed |
| Deferred/Busy | request is recognized but cannot complete immediately |

### 6.3 Command Validation Layers
Each command should be validated in this order:
1. JSON/frame validity
2. top-level `type` validity
3. `msg` and `args` validity
4. requested `args` field validity
5. authority/policy validity
6. manager/state validity
7. execution success/failure

## 7. Core TE Command Set
All TE-originated requests use either `GET` or `SET`. The field names inside
`args` define the command surface.

## 7.1 Implemented `GET args` Fields
| Field | Purpose | Response |
|---|---|---|
| `date_time` | Return the current RTC-backed date/time. | `args.date_time` |
| `sw_version` | Return the compiled firmware version string. | `args.sw_version` |
| `dbg_period_ms` | Return the configured periodic debug telemetry interval. | `args.dbg_period_ms` |
| `dbg_signals` | Return debug telemetry configuration or a requested signal sample. | `args.dbg_signals` |
| direct debug signal name | Return a one-shot debug sample, for example `adc.vupstream.raw`. | `args.dbg_signals.<signal-name>` |
| direct `adc.<channel>.<field>` calibration field | Return RAM-resident ADC calibration by supported channel name. | direct field name under `args` |
| `ltc3901` | Return LTC3901 manager runtime configuration. | `args.ltc3901` |
| `lt8316` | Return LT8316 manager runtime configuration. | `args.lt8316` |

Example:

```json
{"type":"GET","msg":10,"args":["sw_version"]}
```

## 7.2 Implemented `SET args` Fields
| Field | Purpose | Response |
|---|---|---|
| `date_time` | Set RTC-backed date/time in `YYYYMMDD HH:MM:SS` format. | Echoes `args.date_time` on success. |
| `sts_period_ms` | Configure periodic `STS`; `0` disables periodic `STS`. | Echoes resulting `args.sts_period_ms`. |
| `dbg_period_ms` | Configure periodic `DBG`; valid values are `0` or `100..60000`. | Echoes resulting `args.dbg_period_ms`. |
| `dbg_signals` array | Select periodic debug telemetry signals. | Echoes resulting debug signal configuration. |
| `hc_cmd` | Apply `RESET` to request a deferred MCU software reset. | Echoes `hc_cmd` before reset. |
| direct digital signal fields | Set supported digital outputs, such as `led.blue` or `ltc3901.pwr_en`. | Echoes resulting field values. |
| direct `adc.<channel>.<field>` calibration fields | Set RAM-resident ADC calibration by supported channel name. | Echoes applied direct calibration fields. |
| `ltc3901_cmd` | Apply `RUN`, `HALT`, or `RESET` to the LTC3901 manager. | Returns manager command/status result. |
| `lt8316_cmd` | Apply `RUN` or `RESET` to the LT8316 manager. | Returns manager command/status result. |
| direct `ltc3901.<field>` config fields | Partially update RAM-resident LTC3901 manager configuration. | Echoes applied fields. |
| direct `lt8316.<field>` config fields | Partially update RAM-resident LT8316 manager configuration. | Echoes applied fields. |

Example:

```json
{"type":"SET","msg":11,"args":{"ltc3901_cmd":"RUN"}}
{"type":"SET","msg":12,"args":{"hc_cmd":"RESET"}}
```

## 7.3 Field-Based Protocol Direction
New TE-visible readback behavior should be added as a new field name in the
`GET args` array. New write/action behavior should be added as a new
`SET args.<field>` capability unless there is a compelling reason to add a new
top-level message type. Verb-style aliases are outside the current protocol.

Functionality that might previously have been described as a separate command
maps into the current model as follows:

| Older concept | Preferred direction |
|---|---|
| identity / version query | `GET args:["sw_version"]`; hardware ID is available in `RSP.hc` and `STS.hc_id` |
| status query | periodic `STS`; targeted readback through specific `GET args` fields |
| power, reset, or manager action | `SET args.hc_cmd`, `SET args.ltc3901_cmd`, `SET args.lt8316_cmd`, or direct settable digital signal fields |
| runtime configuration | `GET args:["ltc3901"]`, `GET args:["lt8316"]`, direct `SET args.ltc3901.<field>`, and direct `SET args.lt8316.<field>` |
| debug telemetry | `GET args:["dbg_period_ms","dbg_signals"]`, `SET args.dbg_period_ms`, and `SET args.dbg_signals` |
| ADC calibration and sampling | direct `GET` field names such as `adc.vupstream.raw` and `adc.vupstream.offset`, plus direct `SET` fields such as `adc.vupstream.offset` |

Centralized top-level fault query/clear and mode-control functionality is not
part of the implemented protocol. If equivalent product behavior is later
needed, it should be expressed through explicit `GET args` or `SET args` fields rather
than legacy verb-style commands.

## 8. Response Payload Content
### 8.1 Minimum Common Response Fields
Responses should include, where practical:
- `type:"RSP"`
- `hc`
- echoed `msg`
- `ts`
- `args` containing echoed result fields or an error description

### 8.2 Status Visibility
The periodic `STS` report is the canonical summary status output. It should
include at minimum:
- HC ID
- operational state
- Beam On state
- DUT manager states
- DUT control/output states
- DUT summary measurements
- RTC-backed timestamp

Additional detail should be exposed through specific `GET args` fields rather
than through a monolithic status command. Fault-like DUT manager transitions are
reported through asynchronous `EVT` messages.

### 8.3 Recommended Once-Per-Second `STS` Message in `NORMAL`
When the HC is in `NORMAL`, it should send a summary `STS` message to the TE at a nominal 1 Hz rate on a best-effort basis.

This message is the primary routine supervisory message from HC to TE.

No further detailed cadence definition is required for v1.

The `STS` message shall be emitted as one JSON object per line by device-output convention and should include, at minimum:
- `type` = `STS`
- `hc_id`
- `ts`
- `state` (`NORMAL` in the current implementation)
- `beam_on`
- `duts`

Each emitted line should remain focused on directly useful DUT status rather than unrelated transport/session metadata.

Detailed measurements, counters, and fault evidence that are not part of the agreed `STS` shape should be exposed through specific `GET args` fields as needed.

#### 8.3.1 Required Top-Level JSON Keys
The v1 `STS` object shall use these top-level keys:
- `type`
- `hc_id`
- `ts`
- `state`
- `beam_on`
- `duts`

#### 8.3.2 Required `duts` Object Shape
For v1, `duts` shall be a JSON object keyed by DUT name.

The initial required DUT keys are:
- `LTC3901`
- `LT8316`

Each DUT object shall be present in every emitted `STS` line, even if the DUT is powered off or faulted.

#### 8.3.3 Required `LTC3901` Object Keys
The `duts.LTC3901` object shall include:
- `state`
- `pwr_en`
- `sync`
- `vsupply`
- `vshunt`
- `isupply`
- `me_freq`
- `me_ratio`
- `me_anlg`
- `mf_freq`
- `mf_ratio`
- `mf_anlg`

#### 8.3.4 Required `LT8316` Object Keys
The `duts.LT8316` object shall include:
- `state`
- `pwr_en`
- `gate_freq`
- `gate_anlg`
- `vout`

#### 8.3.5 Recommended Value Conventions
For v1, the following conventions are recommended:
- `type` uses a stable string enum, with `STS` required for this record type
- `duts.LTC3901.state` uses the LTC3901 manager state-table names: `RESET`, `HALT`, `POWER_UP`, `POWER_FAULT`, `POWERED`, `POWERED_SYNC_ON`, `POWERED_SYNC_OFF`, or `POWERED_SYNC_FAULT`
- `duts.LT8316.state` uses the LT8316 manager state-table names: `RESET`, `FAULT`, or `POWERED`
- `beam_on` uses a JSON boolean
- `hc_id` uses a JSON integer representing the HC hardware ID
- `ts` uses the HC RTC-backed timestamp string format `YYYYMMDD HH:MM:SS`
- `vsupply` uses millivolts (`mV`)
- `vshunt` uses millivolts (`mV`)
- `isupply` uses milliamps (`mA`)
- fields ending in `_freq` use hertz (`Hz`)
- populated fields ending in `_ratio` use percent over the range `0` to `100`
- fields ending in `_anlg` use millivolts (`mV`)
- `vout` uses millivolts (`mV`)
#### 8.3.6 Frequency and Analog Capture / Scaling Rules
For v1, frequency and analog measurement fields reported in `STS` should follow these general rules:
- reported values shall be scaled into engineering units before transmission in `STS`; raw ADC counts, timer counts, or other unscaled internal values should not be used in the periodic `STS` payload
- `vsupply` and `vshunt` should represent the circuit sense-point voltage rather than only the MCU ADC pin voltage
- the default `vsupply` and `vshunt` scaling assumes 100 k / 37.4 k input dividers unless overridden by calibration
- fields ending in `_freq` should represent the measured signal frequency in hertz after applying the relevant timer/counter scaling and any required averaging or qualification logic
- fields ending in `_anlg` should represent the measured analog signal level in millivolts after applying the relevant ADC scaling, reference conversion, divider or gain correction, and any required averaging or qualification logic
- populated fields ending in `_ratio` should represent a derived duty, activity, or proportion metric scaled to the range `0` to `100`
- capture and scaling behavior shall be deterministic for a given firmware build and shall use the same interpretation for all emitted `STS` lines
- any averaging, filtering, debounce, or qualification used before publishing measurement values shall be controlled by named HC variables rather than fixed numeric values in this specification
- if a measurement is not valid because the DUT is powered off, isolated, restarting, or otherwise not in a condition where the measurement is meaningful, the firmware shall still emit the field and shall use `null` as the preferred invalid or unavailable value representation; if `null` cannot be accommodated by the protocol implementation, the value `-1` shall be used instead

For v1, the intended field-specific interpretation is:
- `me_freq`, `mf_freq`, and `gate_freq`: frequency-like measurements reported in `Hz`
- `me_anlg`, `mf_anlg`, `gate_anlg`, `vsupply`, `vshunt`, and `vout`: analog-derived measurements reported in `mV`
- `isupply`: derived LTC3901 supply current reported in `mA` as `(vsupply - vshunt) / 10 ohms`
- `me_ratio`: derived LTC3901 ME duty ratio reported over the range `0` to `100`
- `mf_ratio`: derived LTC3901 MF duty ratio reported over the range `0` to `100`

Applicable scaling and qualification variables may include, for example:
- ADC conversion and analog front-end variables for reference voltage, gain, and divider correction
- timer or counter scaling variables for frequency conversion
- moving-average, debounce, persistence, or qualification variables for publication stability

#### 8.3.7 DUT-Local `state` Meanings
For v1, DUT-local `state` values in `duts.<name>.state` are the active DUT manager states.

LTC3901 states:
- `RESET`: LTC3901 power and sync outputs are disabled and fault counters are cleared.
- `HALT`: LTC3901 power and sync outputs are disabled and fault counters are preserved.
- `POWER_UP`: LTC3901 power is enabled while voltage/current startup checks are being qualified.
- `POWER_FAULT`: LTC3901 power is disabled after a power/startup fault.
- `POWERED`: LTC3901 power is enabled and sync is not yet enabled.
- `POWERED_SYNC_ON`: LTC3901 power and SDRA/SDRB sync outputs are enabled.
- `POWERED_SYNC_OFF`: LTC3901 power is enabled and sync outputs are disabled during the off portion of the sync cycle.
- `POWERED_SYNC_FAULT`: LTC3901 power is enabled, sync outputs are disabled, and the manager is handling a sync activity fault.

LT8316 states:
- `RESET`: LT8316 HV power is disabled and fault counters are cleared.
- `FAULT`: LT8316 HV power is disabled after a gate/startup fault.
- `POWERED`: LT8316 HV power is enabled.

#### 8.3.8 DUT Recovery / Restart Policy
For v1, DUT-local recovery and restart behavior is owned by each DUT manager:
- LTC3901 power/startup failures transition to `POWER_FAULT`, disable LTC3901 power and sync outputs, increment the LTC3901 power fault count, and retry by entering `POWER_UP` while the retry count remains below the configured maximum. A configured LTC3901 `power_fault_max` value of `0` disables the retry-count limit and permits indefinite retries.
- LTC3901 sync activity failures transition to `POWERED_SYNC_FAULT`, keep LTC3901 power enabled, disable sync outputs, increment the sync fault count, and retry sync by entering `POWERED_SYNC_ON` after the configured sync fault delay.
- LT8316 gate/startup failures transition to `FAULT`, disable LT8316 HV power, increment the LT8316 fault count, and retry by entering `POWERED` while the retry count remains below the configured maximum. A configured LT8316 `power_fault_max` value of `0` disables the retry-count limit and permits indefinite retries.
- `RESET` commands return the affected manager to `RESET` and clear that manager's fault counters.
- LTC3901 `HALT` returns the LTC3901 manager to `HALT` without clearing its fault counters.
- Recovery actions apply to the affected DUT manager only unless a broader policy is added later.

#### 8.3.9 DUT Restart Qualification Criteria
For v1, a DUT restart attempt should be considered successful only when all applicable qualification conditions are satisfied for that DUT:
- no DUT-local fault remains active for that DUT
- the DUT is not isolated
- the DUT power-enable state is asserted as required for that DUT
- required DUT-local telemetry fields indicate operation within expected limits or expected activity for that DUT, using the applicable HC variables and fault logic
- the DUT remains free of renewed DUT-local fault detection for at least the required qualification interval

Qualification timing and acceptance thresholds shall be controlled by named variables rather than fixed numeric values in this specification. Applicable variables may include, for example:
- `VAR_HC_DUT_RECOVERY_QUALIFY_TIME_MS`
- `VAR_HC_DUT1_VSUPPLY_MIN_MV`
- `VAR_HC_DUT1_VSUPPLY_MAX_MV`
- `VAR_HC_DUT1_ISUPPLY_MIN_MA`
- `VAR_HC_DUT1_ISUPPLY_MAX_MA`
- `VAR_HC_DUT1_ME_FREQ_MIN_HZ`
- `VAR_HC_DUT1_ME_FREQ_MAX_HZ`
- `VAR_HC_DUT1_MF_FREQ_MIN_HZ`
- `VAR_HC_DUT1_MF_FREQ_MAX_HZ`
- `VAR_HC_DUT2_VOUT_MIN_MV`
- `VAR_HC_DUT2_VOUT_MAX_MV`
- `VAR_HC_DUT2_GATE_FREQ_MIN_HZ`
- `VAR_HC_DUT2_GATE_FREQ_MAX_HZ`

If the applicable qualification conditions are satisfied after restart, the DUT manager transitions to its normal powered state sequence: LTC3901 proceeds from `POWER_UP` to `POWERED` and then through the sync states, while LT8316 remains in `POWERED`. If qualification fails, the manager remains in or returns to its fault handling state according to the DUT recovery / restart policy.

#### 8.3.10 DUT Restart Action Sequence
For v1, when the HC performs a DUT-local restart attempt, the action sequence should follow this order:
- detect one or more DUT-local faults and enter the relevant manager fault state
- record or update DUT fault status and any related evidence required for reporting
- de-assert the affected DUT power-enable or other DUT-local enable path as required for safe restart
- wait the required restart delay interval controlled by `VAR_HC_DUT_RESTART_DELAY_MS`
- re-assert the affected DUT power-enable or other DUT-local enable path
- allow the DUT to reinitialize for any required startup or settle interval defined by the applicable HC variables and DUT-specific logic
- observe DUT-local telemetry and fault logic during the qualification interval
- if qualification succeeds, transition through the applicable manager powered state sequence
- if qualification fails, either begin another permitted restart attempt or remain in the applicable manager fault state after retry attempts are exhausted

The HC should apply this restart sequence to the affected DUT only, unless another HC policy explicitly requires broader action.

Manager events may emit asynchronous `EVT` records for significant DUT-local transitions. Periodic `STS` reporting carries the current manager state for ongoing visibility.

#### 8.3.11 Canonical Example `STS` JSON Object
Example single emitted JSON line:
- `{ "type": "STS", "hc_id": 63, "ts": "20260501 10:30:00", "beam_on": true, "duts": { "LTC3901": { "state": "POWERED_SYNC_ON", "pwr_en": true, "sync": true, "vsupply": 12345, "vshunt": 12345, "isupply": 12345, "me_freq": 12345, "me_ratio": 50, "me_anlg": 12345, "mf_freq": 12345, "mf_ratio": 50, "mf_anlg": 12345 }, "LT8316": { "state": "POWERED", "pwr_en": true, "gate_freq": 12345, "gate_anlg": 12345, "vout": 12345 } } }`

Field ordering should be kept stable in firmware where practical, even though JSON object ordering is not semantically significant.


### 8.4 Fault and Event Detail Direction
The current firmware does not expose a centralized top-level fault query/clear
protocol. DUT-local fault-like transitions are visible through DUT manager state
in `STS` and through `EVT` text records.

If structured fault detail is added later, it should be exposed through one or
more explicit `GET args` fields rather than a new verb-style command.

## 9. Periodic Status Reporting
The HC shall transmit periodic status reports to the TE while TE communications are active.

### 9.1 Periodic Report Interval
The reporting interval shall be represented by a named variable, such as:
- `VAR_TE_STATUS_REPORT_INTERVAL`

### 9.2 Periodic Report Content
Periodic reports should include:
- HC ID
- current state
- Beam On status
- DUT1 status summary
- DUT2 status summary
- timestamp

### 9.3 Event-Driven Reporting
In addition to periodic reports, the HC should be capable of issuing event-style indications for significant transitions, such as:
- state change
- fault asserted
- fault cleared
- warning asserted
- warning cleared
- TE link became active/inactive

The current JSON implementation uses asynchronous `EVT` records for DUT manager events. `EVT` records are HC-originated and are not responses to a TE request, so they do not include a request `msg` field.

The current first-slice `EVT` shape is:

- `{ "type": "EVT", "hc": 1, "ts": "20260501 10:30:00", "args": { "msg": "LTC3901: Entering POWER_UP" } }`

The current payload is intentionally limited to `args.msg`, with manager-generated messages prefixed by the related device name. Future revisions may add structured event identifiers, scopes, severity, or fault references without changing the top-level `type` / `hc` / `ts` / `args` pattern.

For manager-generated events, the current `args.msg` text starts with the related device name. Fault events include the measured value and comparison value where available. Retry events include retry count and maximum retry count, for example:

- `{ "type": "EVT", "hc": 1, "ts": "20260501 10:30:00", "args": { "msg": "LTC3901: Isupply Current too high: measured 123 mA >= limit 100 mA" } }`
- `{ "type": "EVT", "hc": 1, "ts": "20260501 10:30:00", "args": { "msg": "LTC3901: Retrying 1/3 Power Up" } }`

## 10. Error Handling Model
### 10.1 Error Categories
| Error Category | Meaning |
|---|---|
| syntax error | malformed JSON or incomplete frame |
| unknown type | top-level `type` is unsupported |
| invalid argument | argument missing or invalid |
| unknown field | requested `args` field is unsupported |
| invalid state | command not allowed in current state |
| policy violation | command conflicts with fault/safety policy |
| execution failure | attempted action failed |
| not supported | recognized concept not available through implemented fields |

### 10.2 Error Response Expectations
An error `RSP` should include:
- `type:"RSP"`
- echoed `msg` when available
- error category
- concise reason token/string
- optional detail field

### 10.3 State Violation Policy
Requests that violate current manager/state rules should be rejected with an error response or ignored according to the implemented command handler behavior.

## 11. TE Interface Authority and Safety Rules
### 11.1 Authority Model
- TE is the primary supervisory interface
- debug may also invoke shared action handlers
- shared policy logic must determine whether a requested action is allowed

### 11.2 Safety Rule Alignment
- LLFs remain reset-only
- Beam On imposes no command restrictions
- runtime DUT faults isolate the affected DUT by default
- catastrophic initialization failures render normal TE interaction moot

## 12. Variable and Protocol Traceability
Where protocol behavior depends on command-visible numeric values, named fields shall be used.

Examples:
- `sts_period_ms`
- `dbg_period_ms`
- possible event-throttle variables if report rate limiting is later added

All such variables should ultimately be recorded in:
- `hc_variable_registry.md`

## 13. JSON Protocol Examples
The following examples use the implemented field-based protocol style.

### Query Examples
```json
{"type":"GET","msg":20,"args":["date_time"]}
```

```json
{"type":"GET","msg":21,"args":["ltc3901"]}
```

### Action Examples
```json
{"type":"SET","msg":22,"args":{"date_time":"20260501 10:30:00"}}
```

```json
{"type":"SET","msg":23,"args":{"lt8316_cmd":"RUN"}}
```

### Example Success Response Style
```json
{"type":"RSP","hc":1,"msg":21,"ts":"20260501 10:30:00","args":{"ltc3901":{}}}
```

### Example Error Response Style
```json
{"type":"RSP","hc":1,"msg":23,"ts":"20260501 10:30:00","args":{"err":"invalid_argument"}}
```

## 14. Open Interface Decisions
The following remain open for refinement:
- whether asynchronous `EVT` records are always enabled or configurable
- whether additional structured fault/event fields are needed
- whether additional runtime variables should become TE-visible `GET args` or `SET args` fields
- whether a binary protocol is needed in a later revision

## 15. Recommended Next BMAD Artifacts
This TE interface spec should be followed by:
1. formal state machine specification
2. story backlog for implementation
3. verification and traceability matrix
4. protocol test plan

## 16. Revision Notes
- v1: Initial HC TE interface / command specification created from PRD v1, Fault Response Matrix v1.4, Variable Registry v1, and Architecture v1.
