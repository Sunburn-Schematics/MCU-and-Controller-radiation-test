# Radiation Test Host Controller (HC) TE Interface / Command Specification v1

## 1. Document Purpose
This document defines the proposed interface between the Test Executive (TE) and the Radiation Test Host Controller (HC).

It describes:
- the transport and session model
- message framing approach
- command and response semantics
- periodic status reporting behavior
- error handling expectations
- interaction rules consistent with the HC PRD, Fault Response Matrix, Variable Registry, and Firmware Architecture

This is a BMAD-style v1 draft and intentionally leaves some low-level protocol encoding details open where the project has not yet finalized them.

## 2. Interface Summary
| Item | Definition |
|---|---|
| Primary TE interface | USB Virtual COM Port (VCP) |
| Primary interaction model | Serial-style command / response |
| HC periodic behavior | In Normal operation, HC sends a summary status message to TE once per second |
| TE command behavior | TE sends ad hoc commands to HC |
| Beam On restrictions | None |
| Debug relation to TE link | Debug may report whether USB VCP link is active |

## 3. Interface Goals
The TE interface shall:
- provide deterministic supervisory control of the HC
- allow the TE to observe HC state, DUT state, and fault state
- allow the TE to request operational mode changes
- allow the TE to control DUT power and related actions subject to HC policy
- support explicit fault clear operations for clearable HLFs
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

## 5. Recommended Message Model
A line-oriented ASCII command protocol remains acceptable for TE-issued commands in v1, but the HC once-per-second Normal-operation summary message should use JSON Lines (JSONL) so it is both machine-parseable and human-readable.

### 5.1 Recommended Baseline Framing
For TE-issued commands:
- one command per line
- arguments separated by spaces or key-value tokens
- newline-terminated

For HC-issued periodic status in `NORMAL`:
- one complete JSON object per line
- newline-delimited transport over VCP
- this is JSONL framing
- field names should remain stable across firmware revisions where possible

### 5.2 Recommended Response Categories
| Response Type | Purpose |
|---|---|
| `OK` | command accepted and completed |
| `ERR` | command rejected or failed |
| `RSP` | structured query/command response payload |
| `EVT` | asynchronous event indication |
| `STS` | periodic status report |

### 5.3 Priority Message for v1
The highest-priority HC-to-TE message for v1 is the periodic Normal-operation summary status message.

This message shall:
- be emitted by the HC once per second while the HC is in `NORMAL`
- be encoded as JSONL, with one JSON object per line
- provide summary DUT information for both DUT1 and DUT2
- be machine-parseable and human-readable
- avoid requiring the TE to poll constantly for routine supervisory information

### 5.4 Recommended Normal-Operation Summary Form
The periodic message should use the `STS` response category and be emitted as a single JSON object on each line.

Recommended abstract form:
- `{ "type": "STS", "state": "NORMAL", "tsb": <value>, "dut1": { ... }, "dut2": { ... }, "faults": { ... }, "warnings": { ... } }`

This JSONL `STS` message should be treated as the primary supervisory heartbeat from HC to TE.

### 5.5 Example Abstract Forms
These examples are conceptual and not yet the final syntax:

- `GET_STATUS`
- `GET_ID`
- `SET_DUT1_POWER ON`
- `SET_DUT2_POWER OFF`
- `CLEAR_FAULT HLF-001`
- `ENTER_SLAVE`
- `EXIT_SLAVE`

Corresponding example response patterns:
- `OK cmd=SET_DUT1_POWER state=accepted`
- `ERR cmd=CLEAR_FAULT reason=precondition_failed`
- `RSP cmd=GET_ID id=0x15`
- `{ "type": "STS", "state": "NORMAL", "dut1": { "power": "ON" }, "dut2": { "power": "OFF" } }`
## 6. Command Processing Model
### 6.1 Shared Semantics
TE and debug commands should route through the same core action handlers wherever possible.

This ensures:
- identical safety/policy behavior
- consistent fault clear logic
- consistent state transition rules

### 6.2 Command Outcome Classes
| Outcome | Meaning |
|---|---|
| Accepted | command is valid and action completed |
| Rejected | command is invalid for current state/policy |
| Failed | command was valid but action could not be completed |
| Deferred/Busy | command recognized but cannot complete immediately |

### 6.3 Command Validation Layers
Each command should be validated in this order:
1. transport/frame validity
2. command name validity
3. argument validity
4. authority/policy validity
5. state validity
6. execution success/failure

## 7. Core TE Command Set
The following command families are recommended for v1.

## 7.1 Discovery and Identity Commands
| Command | Purpose | Response |
|---|---|---|
| `GET_ID` | return the HC hardware ID | `RSP` with HC ID |
| `GET_VERSION` | return firmware/protocol version information | `RSP` |
| `GET_CAPABILITIES` | return supported command families / features | `RSP` |
| `PING` | verify responsiveness | `OK` or `RSP` |

### Notes
- `GET_CAPABILITIES` is useful because requirements and protocol may evolve over time
- version reporting should support later compatibility management

## 7.2 Status and Telemetry Commands
| Command | Purpose | Response |
|---|---|---|
| `GET_STATUS` | return current HC summary status | `RSP` |
| `GET_LINK_STATUS` | return TE-link-active status | `RSP` |
| `GET_DUT1_STATUS` | return DUT1-specific status and measurements | `RSP` |
| `GET_DUT2_STATUS` | return DUT2-specific status and measurements | `RSP` |
| `GET_BEAM_STATUS` | return Beam On input state | `RSP` |
| `GET_VARIABLE <name>` | return current value/status of a named variable if supported | `RSP` or `ERR` |

### Status Reporting Priority Note
The TE should rely primarily on the HC periodic once-per-second `STS` message during `NORMAL` operation, and use `GET_*` commands for ad hoc detail or recovery workflows.

## 7.3 Fault and Warning Commands
| Command | Purpose | Response |
|---|---|---|
| `GET_FAULTS` | return active and/or latched faults | `RSP` |
| `GET_WARNINGS` | return active warnings/degraded conditions | `RSP` |
| `GET_FAULT_DETAIL <fault_id>` | return detailed record for one fault | `RSP` |
| `CLEAR_FAULT <fault_id>` | clear a specific clearable HLF | `OK` or `ERR` |
| `CLEAR_ALL_CLEARABLE_FAULTS` | attempt to clear all eligible HLFs | `OK` / `RSP` / `ERR` |

### Fault-Clearing Policy Alignment
- LLFs are not TE-clearable
- HLFs are clearable only when all preconditions are satisfied
- warnings auto-clear and generally do not require explicit clear
- Beam On does not block fault-clearing commands

## 7.4 Mode Control Commands
| Command | Purpose | Response |
|---|---|---|
| `GET_MODE` | return current HC operational mode | `RSP` |
| `ENTER_SLAVE` | request transition to Slave mode | `OK` or `ERR` |
| `EXIT_SLAVE` | request return from Slave mode | `OK` or `ERR` |
| `RESET_HC` | request HC reset | `OK` then reset behavior |

### Mode Policy Notes
- Beam On imposes no restrictions on mode-control commands
- mode transitions remain subject to state-machine rules
- invalid mode requests should be rejected with clear error reasons

## 7.5 DUT Power and Control Commands
| Command | Purpose | Response |
|---|---|---|
| `SET_DUT1_POWER ON|OFF` | control DUT1 power path | `OK` or `ERR` |
| `SET_DUT2_POWER ON|OFF` | control DUT2 HV path | `OK` or `ERR` |
| `GET_DUT1_POWER` | return commanded DUT1 power state | `RSP` |
| `GET_DUT2_POWER` | return commanded DUT2 power state | `RSP` |
| `SET_SYNC_ENABLE ON|OFF` | enable/disable DUT1 sync generation if exposed as a separate control | `OK` or `ERR` |
| `GET_SYNC_STATUS` | return DUT1 sync generation status | `RSP` |

### Control Policy Notes
- acceptance of power or sync commands depends on current HC state and safety policy
- affected-DUT-only isolation policy means unrelated DUT controls should remain available unless a broader system rule blocks them

## 7.6 Diagnostics and Event Commands
| Command | Purpose | Response |
|---|---|---|
| `GET_EVENT_LOG` | return recent event/fault summary entries if supported | `RSP` |
| `GET_COUNTERS` | return selected counters such as fault counts or uptime | `RSP` |
| `GET_BOOT_REASON` | return reset / boot reason if available | `RSP` |

## 8. Response Payload Content
### 8.1 Minimum Common Response Fields
Responses should include, where practical:
- command name
- result code
- timestamp or TSB
- current HC state when relevant
- reason field on rejection/failure

### 8.2 Recommended `GET_STATUS` Fields
A `GET_STATUS` response and periodic `STS` report should include at minimum:
- HC ID
- firmware/protocol version
- operational state
- Beam On state
- TE link active state
- DUT1 commanded power state
- DUT2 commanded power state
- DUT1 summary measurements
- DUT2 summary measurements
- active HLF summary
- active LLF summary if representable
- active warnings summary
- TSB

### 8.3 Recommended Once-Per-Second `STS` Message in `NORMAL`
When the HC is in `NORMAL`, it should send a summary `STS` message to the TE at a nominal 1 Hz rate on a best-effort basis.

This message is the primary routine supervisory message from HC to TE.

No further detailed cadence definition is required for v1.

The `STS` message shall use JSONL framing and should include, at minimum, one JSON object per emitted line containing:
- `type` = `STS`
- `version`
- `hc_id`
- `tsb`
- `state` (`BOOT`, `FAULT`, `NORMAL`, or `SLAVE`)
- `beam_on`
- `duts`

Each emitted line should remain focused on directly useful DUT status rather than unrelated transport/session metadata.

Detailed measurements, counters, and fault evidence that are not part of the agreed `STS` shape should remain available through `GET_STATUS`, `GET_DUT1_STATUS`, `GET_DUT2_STATUS`, and `GET_FAULT_DETAIL` as appropriate.

#### 8.3.1 Required v1 Top-Level JSONL Keys
The v1 `STS` object shall use these top-level keys:
- `type`
- `version`
- `hc_id`
- `tsb`
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
- `faults`

#### 8.3.4 Required `LT8316` Object Keys
The `duts.LT8316` object shall include:
- `state`
- `pwr_en`
- `gate_freq`
- `gate_ratio`
- `gate_anlg`
- `vout`
- `faults`

#### 8.3.5 Required DUT-Local `faults` Object Shape
For v1, each DUT-local `faults` object shall use this shape:
- `count`
- `summary`
- `ids`

Where:
- `count` is the number of active faults currently associated with that DUT
- `summary` is a short human-readable roll-up string such as `NONE`, `OVERCURRENT`, or `MULTIPLE`
- `ids` is an array of active fault IDs for that DUT, which may be empty

An empty-fault case should therefore appear as:
- `{ "count": 0, "summary": "NONE", "ids": [] }`

#### 8.3.6 Recommended Value Conventions
For v1, the following conventions are recommended:
- `type` uses a stable string enum, with `STS` required for this record type
- top-level `state` uses one of: `BOOT`, `FAULT`, `NORMAL`, `SLAVE`
- `duts.<name>.state` uses one of: `NORMAL`, `RECOVERED`, `ISOLATED`, `FAULT`
- `beam_on` uses a JSON boolean
- `hc_id` uses a JSON integer representing the HC hardware ID
- `tsb` uses a monotonic integer time-base value from HC firmware
- `vsupply` uses millivolts (`mV`)
- `vshunt` uses millivolts (`mV`)
- `isupply` uses milliamps (`mA`)
- fields ending in `_freq` use hertz (`Hz`)
- fields ending in `_ratio` use percent over the range `0` to `100`
- fields ending in `_anlg` use millivolts (`mV`)
- `vout` uses millivolts (`mV`)
- `faults.count` uses a JSON integer
- `faults.summary` uses one of: `NONE`, `SINGLE`, `MULTIPLE`
- `faults.ids` uses a JSON array of strings
#### 8.3.7 Frequency and Analog Capture / Scaling Rules
For v1, frequency and analog measurement fields reported in `STS` should follow these general rules:
- reported values shall be scaled into engineering units before transmission in `STS`; raw ADC counts, timer counts, or other unscaled internal values should not be used in the periodic `STS` payload
- fields ending in `_freq` should represent the measured signal frequency in hertz after applying the relevant timer/counter scaling and any required averaging or qualification logic
- fields ending in `_anlg` should represent the measured analog signal level in millivolts after applying the relevant ADC scaling, reference conversion, divider or gain correction, and any required averaging or qualification logic
- fields ending in `_ratio` should represent a derived duty, activity, or proportion metric scaled to the range `0` to `100`
- capture and scaling behavior shall be deterministic for a given firmware build and shall use the same interpretation for all emitted `STS` lines
- any averaging, filtering, debounce, or qualification used before publishing measurement values shall be controlled by named HC variables rather than fixed numeric values in this specification
- if a measurement is not valid because the DUT is powered off, isolated, restarting, or otherwise not in a condition where the measurement is meaningful, the firmware shall still emit the field and shall use `null` as the preferred invalid or unavailable value representation; if `null` cannot be accommodated by the protocol implementation, the value `-1` shall be used instead

For v1, the intended field-specific interpretation is:
- `me_freq`, `mf_freq`, and `gate_freq`: frequency-like measurements reported in `Hz`
- `me_anlg`, `mf_anlg`, `gate_anlg`, `vsupply`, `vshunt`, and `vout`: analog-derived measurements reported in `mV`
- `me_ratio`, `mf_ratio`, and `gate_ratio`: derived ratio measurements reported over the range `0` to `100`

Applicable scaling and qualification variables may include, for example:
- ADC conversion and analog front-end variables for reference voltage, gain, and divider correction
- timer or counter scaling variables for frequency conversion
- moving-average, debounce, persistence, or qualification variables for publication stability

#### 8.3.8 DUT-Local `state` Meanings
For v1, DUT-local `state` values in `duts.<name>.state` should be interpreted as follows:
- `NORMAL`: the DUT is not isolated, no DUT-local fault is active, and the HC considers the DUT to be operating within expected conditions based on the available summary measurements and fault logic.
- `FAULT`: one or more DUT-local faults have been detected for the DUT and the HC is attempting to restart the device.
- `RECOVERED`: after restarting the device due to a `FAULT`, the device appears to be operating within `NORMAL` parameters again.
- `ISOLATED`: recovery failed and the DUT has been switched off or otherwise isolated so as not to damage other circuits or corrupt test results.

#### 8.3.8 DUT-Local `state` Transition Rules
For v1, DUT-local state transitions should follow these rules:
- `NORMAL` to `FAULT`: enter when one or more DUT-local faults are detected for that DUT and the HC begins a recovery restart attempt.
- `FAULT` to `RECOVERED`: enter after the HC restart attempt completes and the DUT appears to be operating within `NORMAL` parameters again, with no currently active DUT-local fault.
- `FAULT` to `ISOLATED`: enter when the HC determines that recovery has failed and switches off or otherwise isolates the DUT to protect hardware integrity or test validity.
- `RECOVERED` to `NORMAL`: enter after the DUT continues operating within `NORMAL` parameters for the required observation or qualification interval, if such an interval is implemented.
- `RECOVERED` to `FAULT`: enter if one or more DUT-local faults are detected again during or after the post-restart recovery period and the HC begins another recovery restart attempt.
- `NORMAL` to `ISOLATED`: allowed when the HC intentionally isolates the DUT by command or protective action without first attempting recovery through the `FAULT` state.
- `ISOLATED` to `NORMAL`: allowed only after the DUT is intentionally re-enabled and the HC determines that the DUT is again operating within `NORMAL` parameters.
- `ISOLATED` to `FAULT`: allowed if re-enable or restart is attempted from the isolated condition and a DUT-local fault is immediately detected during the restart attempt.

#### 8.3.9 DUT Recovery / Restart Policy
For v1, DUT-local recovery and restart behavior should follow these rules:
- When a DUT-local fault drives a DUT into `FAULT`, the HC should begin a DUT-local recovery attempt for the affected DUT only.
- Recovery attempt count, restart timing, and qualification timing shall be controlled by named variables rather than fixed numeric values in this specification, for example `VAR_HC_DUT_RESTART_MAX_ATTEMPTS`, `VAR_HC_DUT_RESTART_DELAY_MS`, and `VAR_HC_DUT_RECOVERY_QUALIFY_TIME_MS`.
- If restart succeeds and the DUT appears to operate within `NORMAL` parameters with no active DUT-local fault, the DUT-local state shall transition to `RECOVERED`.
- If the DUT remains faulted, immediately re-faults, or exceeds the permitted number of recovery attempts, the HC shall transition that DUT to `ISOLATED`.
- Transition from `RECOVERED` to `NORMAL` shall occur only after the DUT satisfies the required post-restart observation or qualification interval, if implemented.
- Recovery actions shall not, by default, interrupt the unaffected DUT.
- Re-enable or retry from `ISOLATED` shall occur only by explicit supervisory action or other policy defined elsewhere in the HC specification.

#### 8.3.10 DUT Restart Qualification Criteria
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

If all applicable qualification conditions are satisfied after restart, the DUT-local `state` shall transition from `FAULT` to `RECOVERED`. If qualification fails, renewed DUT-local fault detection occurs, or the qualification interval cannot be completed successfully, the DUT shall remain in `FAULT` or transition to `ISOLATED` according to the DUT recovery / restart policy.

#### 8.3.11 DUT Restart Action Sequence
For v1, when the HC performs a DUT-local restart attempt, the action sequence should follow this order:
- detect one or more DUT-local faults and set the DUT-local `state` to `FAULT`
- record or update DUT fault status and any related evidence required for reporting
- de-assert the affected DUT power-enable or other DUT-local enable path as required for safe restart
- wait the required restart delay interval controlled by `VAR_HC_DUT_RESTART_DELAY_MS`
- re-assert the affected DUT power-enable or other DUT-local enable path
- allow the DUT to reinitialize for any required startup or settle interval defined by the applicable HC variables and DUT-specific logic
- observe DUT-local telemetry and fault logic during the qualification interval
- if qualification succeeds, transition the DUT-local `state` to `RECOVERED`
- if qualification fails, either begin another permitted restart attempt or transition the DUT-local `state` to `ISOLATED` according to `VAR_HC_DUT_RESTART_MAX_ATTEMPTS` and the DUT recovery policy

The HC should apply this restart sequence to the affected DUT only, unless another HC policy explicitly requires broader action.

Immediate event emission is not required for DUT-local fault, restart, recovery, or isolation transitions. For v1, periodic `STS` reporting at the nominal 1 Hz rate is sufficient for TE visibility of these state changes.

#### 8.3.12 Canonical Example `STS` JSONL Object
Example single emitted JSONL line:
- `{ "type": "STS", "version": "1.0", "hc_id": 63, "tsb": 123456, "state": "NORMAL", "beam_on": true, "duts": { "LTC3901": { "state": "NORMAL", "pwr_en": true, "sync": true, "vsupply": 12345, "vshunt": 12345, "isupply": 12345, "me_freq": 12345, "me_ratio": 50, "me_anlg": 12345, "mf_freq": 12345, "mf_ratio": 50, "mf_anlg": 12345, "faults": { "count": 0, "summary": "NONE", "ids": [] } }, "LT8316": { "state": "NORMAL", "pwr_en": true, "gate_freq": 12345, "gate_ratio": 50, "gate_anlg": 12345, "vout": 12345, "faults": { "count": 1, "summary": "SINGLE", "ids": ["HLF-010"] } } } }`

Field ordering should be kept stable in firmware where practical, even though JSON object ordering is not semantically significant.


### 8.4 Recommended Fault Detail Fields
`GET_FAULT_DETAIL <fault_id>` should return, where available:
- fault ID
- class
- active flag
- latched flag
- first occurrence TSB
- latest occurrence TSB
- occurrence count
- related evidence / measurement values
- clearable flag
- current clear eligibility reason

## 9. Periodic Status Reporting
The HC shall transmit periodic status reports to the TE while TE communications are active.

### 9.1 Periodic Report Interval
The reporting interval shall be represented by a named variable, such as:
- `VAR_TE_STATUS_REPORT_INTERVAL`

### 9.2 Recommended Periodic Report Content
Periodic reports should include:
- HC ID
- current mode/state
- Beam On status
- DUT1 status summary
- DUT2 status summary
- active fault/warning summary
- TE link status
- TSB

### 9.3 Event-Driven Reporting
In addition to periodic reports, the HC should be capable of issuing event-style indications for significant transitions, such as:
- state change
- fault asserted
- fault cleared
- warning asserted
- warning cleared
- TE link became active/inactive

## 10. Error Handling Model
### 10.1 Error Categories
| Error Category | Meaning |
|---|---|
| syntax error | malformed command/frame |
| unknown command | command name unsupported |
| invalid argument | argument missing or invalid |
| invalid state | command not allowed in current state |
| policy violation | command conflicts with fault/safety policy |
| execution failure | attempted action failed |
| not supported | recognized concept not implemented in this revision |

### 10.2 Error Response Expectations
An `ERR` response should include:
- originating command
- error category
- concise reason token/string
- optional detail field

### 10.3 State Violation Policy
Commands that violate current state rules should normally be rejected rather than escalated into HLF, unless a future requirement explicitly says otherwise.

## 11. TE Interface Authority and Safety Rules
### 11.1 Authority Model
- TE is the primary supervisory interface
- debug may also invoke shared action handlers
- shared policy logic must determine whether a requested action is allowed

### 11.2 Safety Rule Alignment
- LLFs remain reset-only
- HLF clear operations require all preconditions to be met
- Beam On imposes no command restrictions
- runtime DUT faults isolate the affected DUT by default
- catastrophic initialization failures render normal TE interaction moot

## 12. Variable and Protocol Traceability
Where protocol behavior depends on deferred numeric values, named variables shall be used.

Examples:
- `VAR_TE_STATUS_REPORT_INTERVAL`
- future timeout variables for command parsing or inter-character reception if needed
- possible event-throttle variables if report rate limiting is later added

All such variables should ultimately be recorded in:
- `/a0/usr/projects/bmad_test/docs/hc_variable_registry_v1.md`

## 13. Recommended v1 Text Protocol Style
The following is a recommended style, not yet a locked format:

### Query Examples
- `GET_STATUS`
- `GET_FAULTS`
- `GET_DUT1_STATUS`

### Action Examples
- `SET_DUT1_POWER ON`
- `SET_DUT2_POWER OFF`
- `ENTER_SLAVE`
- `CLEAR_FAULT HLF-001`

### Example Success Response Style
- `OK cmd=SET_DUT1_POWER tsb=<value>`
- `RSP cmd=GET_STATUS state=NORMAL beam=OFF dut1_power=ON dut2_power=OFF tsb=<value>`

### Example Error Response Style
- `ERR cmd=CLEAR_FAULT code=PRECONDITION reason=condition_still_present tsb=<value>`

## 14. Open Interface Decisions
The following remain open for future refinement:
- exact final ASCII grammar
- whether checksums/CRCs are required for line-oriented protocol frames
- whether command names remain verbose text or move to shorter tokens
- whether asynchronous `EVT` records are always enabled or configurable
- whether variable readback is TE-visible for all variables or only a subset
- whether `RESET_HC` is always permitted or restricted by future policy
- whether a binary protocol is needed in a later revision

## 15. Recommended Next BMAD Artifacts
This TE interface spec should be followed by:
1. formal state machine specification
2. story backlog for implementation
3. verification and traceability matrix
4. protocol test plan

## 16. Revision Notes
- v1: Initial HC TE interface / command specification created from PRD v1, Fault Response Matrix v1.4, Variable Registry v1, and Architecture v1.
