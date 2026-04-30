# Radiation Test Host Controller (HC) Fault Response Matrix v1

## 1. Document Purpose
This document defines the initial fault response matrix for the Radiation Test Host Controller (HC). It classifies faults, identifies their detection sources, defines the intended system response, and captures open items requiring further refinement.

This matrix is intended to complement the PRD:
- /a0/usr/projects/bmad_test/docs/hc_prd_v1.md

This version is a BMAD-style v1 draft and intentionally includes **TBD** items where thresholds, persistence times, and protocol details are not yet defined.

## 2. Fault Classes

| Class | Meaning | Typical Cause | Expected Outcome |
|---|---|---|---|
| Low-Level Fault (LLF) | Internal/platform fault that prevents trusted operation | Peripheral init failure, invalid config, internal self-test failure | Enter Low-Level Fault state, place outputs in safe state |
| High-Level Fault (HLF) | Runtime DUT-related or externally relevant operational fault | DUT overcurrent, invalid gate activity, output voltage anomaly | Enter High-Level Fault state, protect DUT/system as defined |
| Warning / Degraded (WRN) | Non-fatal condition that reduces observability or connectivity but may not invalidate core control | USB not connected, debug UART absent, optional monitor unavailable | Remain operational if safe, report degraded condition |

## 3. Response Policy Assumptions
Unless otherwise specified in a later revision:

1. All controllable outputs shall default to a safe OFF state on entry to Low-Level Fault.
2. DUT-specific runtime faults shall default to entering High-Level Fault.
3. USB absence alone is provisionally treated as a **Warning / Degraded** condition, not a Low-Level Fault.
4. Low-Level Faults are latched and are recoverable only by reset and re-execution of boot-time checks.
5. High-Level Faults are latched until explicitly cleared.
6. Fault clearing is permitted during Beam On, subject to all other clear preconditions being satisfied, because Beam On imposes no restrictions on allowable commands or operational modes.
7. Debug is permitted to clear faults.
8. Runtime DUT faults shall isolate the affected DUT only by default, unless a specific fault definition requires broader shutdown.
9. Where thresholds are not yet defined, response intent is captured and exact trigger values remain **TBD**.

## 4. Fault Response Matrix

| Fault ID | Class | Fault Name | Trigger Condition | Detection Source | Persistence / Debounce | State Result | Output Action | TE Reporting | LED Indication | Recovery Method | Notes / Open Items |
|---|---|---|---|---|---|---|---|---|---|---|---|
| LLF-001 | LLF | GPIO Initialization Failure | Required GPIO configuration fails during boot | Boot self-test / init return code | None | Low-Level Fault | Force all controllable outputs to safe OFF states | Report when possible after comms available | LLF flash code TBD | Reset required, unless retry policy added later | Need exact failure detection mechanism |
| LLF-002 | LLF | Debug UART Initialization Failure | Debug UART init fails during boot | Boot init return code | None | Fatal boot failure outside normal operational fault management | Safe OFF outputs; no attempt at runtime management beyond halt/reset behavior | Report unavailable except through any surviving interface | LLF flash code TBD | Treat as catastrophic initialization failure; classification as LLF vs WRN is operationally moot | Initialization failure implies catastrophic hardware and/or firmware failure |
| LLF-003 | LLF | Timer/Input Capture Initialization Failure | Required timer or input capture resource fails to initialize | Boot init return code | None | Low-Level Fault | Safe OFF outputs | Report when possible | LLF flash code TBD | Reset required or init retry TBD | Timers are required for DUT monitoring |
| LLF-004 | LLF | ADC Initialization Failure | ADC init/calibration fails during boot | Boot init/calibration result | None | Low-Level Fault | Safe OFF outputs | Report when possible | LLF flash code TBD | Reset required or recalibration retry TBD | ADC required for DUT voltage/current monitoring |
| LLF-005 | LLF | USB Stack Initialization Failure | USB Virtual COM Port peripheral or stack fails to initialize | Boot init result | None | Fatal boot failure outside normal operational fault management | Safe OFF outputs; no attempt at runtime management beyond halt/reset behavior | Report unavailable until corrected | LLF flash code TBD | Treat as catastrophic initialization failure; classification as LLF vs WRN is operationally moot | USB/VCP init failure implies catastrophic hardware and/or firmware failure |
| LLF-006 | LLF | Internal Configuration Invalid | Required config constants, tables, or build-time assumptions invalid | Boot self-check | None | Low-Level Fault | Safe OFF outputs | Report when possible | LLF flash code TBD | Reset after firmware/config correction | Add concrete config validation list |
| LLF-007 | LLF | ID Read Invalid | Hardware ID read is invalid or illegal combination detected | GPIO read / validation | TBD | Low-Level Fault or Warning/TBD | No DUT power enable until resolved | Report invalid ID | LLF or warning pattern TBD | Reset or ignore invalid combinations TBD | Requires definition of invalid ID combinations |
| WRN-001 | WRN | USB Not Connected / Not Enumerated | USB VCP link not established with TE | USB link supervision | Continuous poll, timeout TBD | Remain in current state unless policy changes | No change to DUT outputs solely due to missing TE link | No TE report possible while disconnected | Degraded indication TBD | Auto-clear on connection | USB VCP is primary, but lack of an active connection is not itself a fault if the stack initialized correctly |
| WRN-002 | WRN | Debug Terminal Absent | No debug terminal connected | Operational observation | None | No state change | No action | No TE impact unless reported in status | None or optional indicator TBD | Auto-clear when terminal attached if detectable | Debug terminal is optional |
| HLF-001 | HLF | DUT1 Input Overcurrent | DUT1 input current exceeds limit | ADC-derived shunt current measurement | `VAR_DUT1_OVERCURRENT_DEBOUNCE` | High-Level Fault | Disable DUT1 power via `nLTC3901_Pwr_Enable`; DUT2 remains unaffected by default | Report fault ID, measured current, timestamp/TSB | HLF indication TBD | Latched; clearable by TE or debug after current returns normal and clear preconditions are met | Threshold variable: `VAR_DUT1_OVERCURRENT_LIMIT`; debounce variable: `VAR_DUT1_OVERCURRENT_DEBOUNCE` |
| HLF-002 | HLF | DUT1 Current Sense Invalid | Current sense inputs inconsistent, saturated, or implausible | ADC plausibility check | `VAR_DUT1_CURRENT_SENSE_INVALID_DEBOUNCE` | High-Level Fault or Warning/TBD | Disable DUT1 power TBD; DUT2 remains unaffected by default | Report degraded or fault condition | TBD | Latched if treated as HLF; clearable by TE or debug after condition normal and preconditions are met | Plausibility-rule variables TBD; debounce variable: `VAR_DUT1_CURRENT_SENSE_INVALID_DEBOUNCE` |
| HLF-003 | HLF | DUT1 ME Gate Activity Missing | No valid ME activity detected while DUT1 is expected to operate | Timer input capture timeout | `VAR_DUT1_ME_ACTIVITY_TIMEOUT` | High-Level Fault | Disable DUT1 power; DUT2 remains unaffected by default | Report fault with channel ME | HLF indication TBD | Latched; clearable by TE or debug after valid activity returns or DUT1 is intentionally disabled and clear preconditions are met | Activity-timeout variable: `VAR_DUT1_ME_ACTIVITY_TIMEOUT`; expected-active condition definition still required |
| HLF-004 | HLF | DUT1 MF Gate Activity Missing | No valid MF activity detected while DUT1 is expected to operate | Timer input capture timeout | `VAR_DUT1_MF_ACTIVITY_TIMEOUT` | High-Level Fault | Disable DUT1 power; DUT2 remains unaffected by default | Report fault with channel MF | HLF indication TBD | Latched; clearable by TE or debug after valid activity returns or DUT1 is intentionally disabled and clear preconditions are met | Activity-timeout variable: `VAR_DUT1_MF_ACTIVITY_TIMEOUT` |
| HLF-005 | HLF | DUT1 ME Gate Out of Range | ME frequency or duty cycle outside valid range | Timer input capture analysis | `VAR_DUT1_ME_RANGE_DEBOUNCE` | High-Level Fault | Disable DUT1 power; DUT2 remains unaffected by default | Report measured values and limits | HLF indication TBD | Latched; clearable by TE or debug after measurements return normal and clear preconditions are met | Range variables: `VAR_DUT1_ME_FREQ_MIN`, `VAR_DUT1_ME_FREQ_MAX`, `VAR_DUT1_ME_DUTY_MIN`, `VAR_DUT1_ME_DUTY_MAX`; debounce variable: `VAR_DUT1_ME_RANGE_DEBOUNCE` |
| HLF-006 | HLF | DUT1 MF Gate Out of Range | MF frequency or duty cycle outside valid range | Timer input capture analysis | `VAR_DUT1_MF_RANGE_DEBOUNCE` | High-Level Fault | Disable DUT1 power; DUT2 remains unaffected by default | Report measured values and limits | HLF indication TBD | Latched; clearable by TE or debug after measurements return normal and clear preconditions are met | Range variables: `VAR_DUT1_MF_FREQ_MIN`, `VAR_DUT1_MF_FREQ_MAX`, `VAR_DUT1_MF_DUTY_MIN`, `VAR_DUT1_MF_DUTY_MAX`; debounce variable: `VAR_DUT1_MF_RANGE_DEBOUNCE` |
| HLF-007 | HLF | DUT1 Filtered ME ADC Out of Range | Filtered ME monitor ADC outside expected range | ADC threshold monitor | `VAR_DUT1_FILTERED_ME_RANGE_DEBOUNCE` | High-Level Fault or Warning/TBD | Disable DUT1 power TBD; DUT2 remains unaffected by default | Report analog monitor anomaly | HLF/WRN indication TBD | If treated as HLF: latched; clearable by TE or debug after condition normal and clear preconditions are met | Envelope variables: `VAR_DUT1_FILTERED_ME_MIN`, `VAR_DUT1_FILTERED_ME_MAX`; debounce variable: `VAR_DUT1_FILTERED_ME_RANGE_DEBOUNCE` |
| HLF-008 | HLF | DUT1 Filtered MF ADC Out of Range | Filtered MF monitor ADC outside expected range | ADC threshold monitor | `VAR_DUT1_FILTERED_MF_RANGE_DEBOUNCE` | High-Level Fault or Warning/TBD | Disable DUT1 power TBD; DUT2 remains unaffected by default | Report analog monitor anomaly | HLF/WRN indication TBD | If treated as HLF: latched; clearable by TE or debug after condition normal and clear preconditions are met | Envelope variables: `VAR_DUT1_FILTERED_MF_MIN`, `VAR_DUT1_FILTERED_MF_MAX`; debounce variable: `VAR_DUT1_FILTERED_MF_RANGE_DEBOUNCE` |
| HLF-009 | HLF | DUT1 Sync Generation Failure | SyncA/SyncB generation cannot be started or maintained as configured | Timer/PWM supervision or internal status | `VAR_DUT1_SYNC_FAILURE_DEBOUNCE` or none, per architecture | High-Level Fault or Low-Level Fault/TBD | Disable DUT1 power or inhibit DUT1 start; DUT2 remains unaffected by default | Report inability to stimulate DUT1 | TBD | If treated as HLF: latched; clearable by TE or debug after configuration/operation is restored and clear preconditions are met | Supervision/debounce variable: `VAR_DUT1_SYNC_FAILURE_DEBOUNCE` if debounce is used |
| HLF-010 | HLF | DUT1 Unexpected Activity While Power Disabled | ME/MF activity present when DUT1 power is expected OFF | Input capture / ADC monitor | `VAR_DUT1_UNEXPECTED_ACTIVITY_DEBOUNCE` | High-Level Fault or Warning/TBD | Keep DUT1 power disabled, flag anomaly; DUT2 remains unaffected by default | Report unexpected activity | TBD | If treated as HLF: latched; clearable by TE or debug after condition normal and clear preconditions are met | Detection variables TBD; debounce variable: `VAR_DUT1_UNEXPECTED_ACTIVITY_DEBOUNCE` |
| HLF-011 | HLF | DUT2 Output Voltage Out of Range High | DUT2 output voltage exceeds upper limit | ADC threshold monitor | `VAR_DUT2_VOUT_HIGH_DEBOUNCE` | High-Level Fault | Disable HV via `HV_Pwr_Enable`; DUT1 remains unaffected by default | Report measured voltage and limit | HLF indication TBD | Latched; clearable by TE or debug after voltage returns normal and clear preconditions are met | Threshold variable: `VAR_DUT2_VOUT_HIGH_LIMIT`; debounce variable: `VAR_DUT2_VOUT_HIGH_DEBOUNCE` |
| HLF-012 | HLF | DUT2 Output Voltage Out of Range Low | DUT2 output voltage below required operating range while enabled | ADC threshold monitor | `VAR_DUT2_VOUT_LOW_DEBOUNCE` | High-Level Fault or Warning/TBD | Disable HV TBD; DUT1 remains unaffected by default | Report measured voltage and limit | TBD | If treated as HLF: latched; clearable by TE or debug after voltage returns normal and clear preconditions are met | Threshold variable: `VAR_DUT2_VOUT_LOW_LIMIT`; debounce variable: `VAR_DUT2_VOUT_LOW_DEBOUNCE` |
| HLF-013 | HLF | DUT2 Gate Activity Missing | No valid DUT2 gate activity while HV enabled and DUT2 expected active | Timer input capture timeout | `VAR_DUT2_GATE_ACTIVITY_TIMEOUT` | High-Level Fault | Disable HV; DUT1 remains unaffected by default | Report missing gate activity | HLF indication TBD | Latched; clearable by TE or debug after valid activity returns or DUT2 is intentionally disabled and clear preconditions are met | Activity-timeout variable: `VAR_DUT2_GATE_ACTIVITY_TIMEOUT`; expected-active condition definition still required |
| HLF-014 | HLF | DUT2 Gate Out of Range | DUT2 gate period/duty outside valid range | Timer input capture analysis | `VAR_DUT2_GATE_RANGE_DEBOUNCE` | High-Level Fault | Disable HV TBD; DUT1 remains unaffected by default | Report measured timing values | HLF indication TBD | Latched; clearable by TE or debug after measurements return normal and clear preconditions are met | Range variables: `VAR_DUT2_GATE_FREQ_MIN`, `VAR_DUT2_GATE_FREQ_MAX`, `VAR_DUT2_GATE_DUTY_MIN`, `VAR_DUT2_GATE_DUTY_MAX`; debounce variable: `VAR_DUT2_GATE_RANGE_DEBOUNCE` |
| HLF-015 | HLF | HV Enable Feedback Mismatch | Commanded HV enable state does not match observed behavior if feedback exists | Control/monitor cross-check | `VAR_HV_ENABLE_FEEDBACK_MISMATCH_DEBOUNCE` | High-Level Fault or Warning/TBD | Force safe disable of DUT2 path; DUT1 remains unaffected by default | Report control mismatch | TBD | If treated as HLF: latched; clearable by TE or debug after mismatch is resolved and clear preconditions are met | Feedback-source variables TBD; debounce variable: `VAR_HV_ENABLE_FEEDBACK_MISMATCH_DEBOUNCE` |
| HLF-016 | HLF | DUT1 Power Enable Feedback Mismatch | Commanded DUT1 power state does not match observed behavior if feedback exists | Control/monitor cross-check | `VAR_DUT1_ENABLE_FEEDBACK_MISMATCH_DEBOUNCE` | High-Level Fault or Warning/TBD | Force safe disable of DUT1 path; DUT2 remains unaffected by default | Report control mismatch | TBD | If treated as HLF: latched; clearable by TE or debug after mismatch is resolved and clear preconditions are met | Feedback-source variables TBD; debounce variable: `VAR_DUT1_ENABLE_FEEDBACK_MISMATCH_DEBOUNCE` |
| HLF-017 | HLF | Beam-On Prohibited Override | This fault is not used in the current policy because Beam On imposes no restrictions on allowable commands or operational modes | Command processor with Beam On status | N/A | Not applicable under current policy | No action based solely on Beam On state | No TE report required for this condition under current policy | None | Not applicable | Retained only for traceability in case Beam On restrictions are introduced later |
| HLF-018 | HLF | Fault During Slave Override | Runtime DUT fault occurs while in Slave mode | Fault manager | Per underlying fault variable set | High-Level Fault | Remove unsafe overrides; disable affected power path(s) only by default | Report fault and active override context | HLF indication TBD | Latched; clear per underlying fault by TE or debug when clear preconditions are met | Inherits threshold/debounce variables from the underlying fault plus override precedence rules |
| HLF-019 | HLF | Measurement Acquisition Stalled | ADC or capture updates stop arriving during runtime | Acquisition watchdog | `VAR_MEASUREMENT_ACQUISITION_STALL_TIMEOUT` | High-Level Fault or Low-Level Fault/TBD | Disable affected DUT power path(s) only by default | Report acquisition stall | TBD | If treated as HLF: latched; clearable by TE or debug after acquisition is restored and clear preconditions are met | Timeout variable: `VAR_MEASUREMENT_ACQUISITION_STALL_TIMEOUT`; runtime peripheral-failure policy still required |
| HLF-020 | HLF | TE Command Violates Current State Rules | Command requests action not allowed in current state | Command processor / state machine | Immediate | No state change or High-Level Fault/TBD | Reject command; outputs unchanged unless unsafe request in progress | Return error response; optional event report | None/TBD | Auto-clear if treated as protocol rejection; if elevated to HLF, latched and clearable by TE or debug | Likely protocol error rather than system fault |

## 5. Preliminary LED Indication Strategy
This section is provisional until the LED flash-code table is defined.

| Condition Type | Proposed LED Behavior | Status |
|---|---|---|
| Normal | Green steady or slow blink | TBD |
| Boot | Green blink startup pattern | TBD |
| Low-Level Fault | Red/Green encoded flash code by LLF ID | TBD |
| High-Level Fault | Red steady or repeating pattern, optional fault-specific cadence | TBD |
| Warning / Degraded | Green with intermittent red pulse or alternate pattern | TBD |

## 6. Finalized Recovery Policy
The following recovery policy decisions have been made:

1. **Low-Level Faults (LLFs)** are latched and are recoverable only by reset.
2. **High-Level Faults (HLFs)** are latched until explicitly cleared.
3. **Fault clearing is allowed during Beam On**, provided all clear preconditions are satisfied, because Beam On imposes no restrictions on allowable commands or operational modes.
4. **Debug is allowed to clear faults**.
5. **Runtime DUT faults isolate the affected DUT only by default**; the unaffected DUT remains running unless a specific fault definition requires broader shutdown.
6. **Warnings / Degraded conditions** are non-latched and auto-clear when the underlying condition is removed.

### 6.1 High-Level Fault Clear Preconditions
A latched HLF may be cleared only when all applicable preconditions are satisfied:

1. The fault is categorized as clearable and is not a reset-only LLF.
2. The underlying measured or inferred fault condition is no longer present.
3. Any required debounce or recovery dwell time has elapsed.
4. The affected DUT remains in or has been returned to a safe state prior to re-enable.
5. No higher-priority uncleared fault prevents return to operation.
6. The clear request originates from the TE or debug interface.

## 7. Remaining Open Questions
The following items still require definition before implementation:
- exact voltage/current/timing thresholds
- persistence/debounce values
- exact scope of any fault that should shut down both DUTs

## 8. Revision Notes
- v1: Initial BMAD-style fault response matrix generated from HC PRD v1 with explicit TBD markers.
- v1.1: Updated with finalized latching and recovery policy decisions for LLF, HLF, Beam On clearing, debug fault clearing, and affected-DUT-only isolation.
- v1.2: Updated Beam On policy to state that Beam On imposes no restrictions on allowable commands or operational modes.
- v1.3: Updated USB policy to reflect USB Virtual COM Port as the primary TE interface and to distinguish USB stack initialization failure from mere lack of active connection.
- v1.4: Updated UART and USB initialization failure policy to treat such failures as catastrophic boot failures, making LLF-versus-other runtime handling distinctions operationally moot.
