# Radiation Test Host Controller (HC) Verification & Traceability Matrix v1

## 1. Document Purpose
This document provides the verification and traceability matrix for the Radiation Test Host Controller (HC).

It links together:
- product requirements from the PRD
- fault behaviors from the fault response matrix
- deferred numeric variables from the variable registry
- structural decisions from the firmware architecture
- TE-visible behavior from the TE interface specification
- state behavior from the state machine specification
- protocol-oriented verification from the protocol test plan

Its purpose is to ensure each requirement is:
- traceable to one or more design artifacts
- traceable to one or more verification methods
- traceable to one or more planned test cases
- visible as either fully specified, partially specified, or still open

## 2. Source Documents
This matrix is derived from:
- `/a0/usr/projects/bmad_test/docs/hc_prd_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_fault_response_matrix_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_variable_registry_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_architecture_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_te_interface_spec_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_protocol_test_plan_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_state_machine_spec_v1.md`

## 3. Verification Method Codes
| Code | Meaning |
|---|---|
| INS | Inspection / document review |
| ANL | Analysis |
| TST | Test |
| DEM | Demonstration |
| SIM | Simulation |

## 4. Status Codes
| Code | Meaning |
|---|---|
| SPEC | Sufficiently specified for implementation planning |
| PARTIAL | Partially specified; implementation possible with assumptions |
| OPEN | Not fully resolved yet |

## 5. Traceability Matrix
| Req ID | Requirement Summary | Source PRD Area | Related Fault IDs / Variables | Architecture / State / Interface References | Verification Method | Planned Verification References | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| HC-REQ-001 | HC shall initialize into a safe state on power-up/reset | Boot, initialization, safe defaults | LLF boot failures; catastrophic init handling | Architecture boot sequencing; State: `BOOT` | INS, TST | State boot tests; protocol connect tests after successful boot | SPEC | Safe OFF before operational enable |
| HC-REQ-002 | HC shall read hardware ID from 6-bit switches | Identification / configuration | N/A | Architecture identification path; TE `GET_ID`; State reporting | INS, TST | PTP-010 | SPEC | Value exposed via TE |
| HC-REQ-003 | HC shall provide primary TE communications over USB VCP | Communications | USB init catastrophic; USB absence degraded-only | TE interface transport; State reporting of TE-link-active | INS, TST | PTP-001, PTP-002, PTP-023 | SPEC | Distinguish init failure vs absence after init |
| HC-REQ-004 | HC shall report whether TE USB VCP link is active | Communications / debug visibility | WRN degraded link | TE interface status model; State reporting | INS, TST | PTP-023 | SPEC | Debug view limited to link-active indication |
| HC-REQ-005 | HC shall monitor DUT1 representative operating behavior | DUT1 monitoring | HLF DUT1 activity/current faults; DUT1 variables | DUT1 measurement modules; `NORMAL` / `HIGH_LEVEL_FAULT`; TE status commands | INS, TST, ANL | PTP-102, PTP-104, PTP-105 | PARTIAL | Numeric thresholds deferred to variables |
| HC-REQ-006 | HC shall generate DUT1 quadrature sync outputs | DUT1 control | DUT1 sync/activity variables | DUT1 sync generator; commandable control; `NORMAL` / `SLAVE` | INS, TST | PTP-101, PTP-103 | PARTIAL | Exact timing values deferred |
| HC-REQ-007 | HC shall detect DUT1 ME/MF cease-operation faults during expected operation | DUT1 fault detection | HLF DUT1 gate/activity faults; `VAR_DUT1_ME_ACTIVITY_TIMEOUT`, `VAR_DUT1_MF_ACTIVITY_TIMEOUT` | Fault manager; DUT1 monitor path; `HIGH_LEVEL_FAULT` | INS, TST, ANL | PTP-103, PTP-104 | PARTIAL | Threshold logic named, numeric values pending |
| HC-REQ-008 | HC shall detect DUT1 input overcurrent conditions | DUT1 fault detection | DUT1 overcurrent HLF; `VAR_DUT1_OVERCURRENT_LIMIT`, `VAR_DUT1_OVERCURRENT_DEBOUNCE` | Measurement pipeline; fault manager; affected-DUT isolation policy | INS, TST, ANL | PTP-105 | PARTIAL | Numeric trip point experimental |
| HC-REQ-009 | HC shall isolate DUT1 power when DUT1 runtime faults require isolation | DUT1 protection | DUT1 HLF rows | ctrl_power; fault manager; State output behavior table | INS, TST | PTP-105, PTP-106 | SPEC | Default unaffected DUT preservation applies |
| HC-REQ-010 | HC shall allow DUT1 recovery attempts by re-enable / power cycling when permitted | DUT1 recovery | clearable HLFs; recovery dwell variables if later defined | TE clear commands; state exit from `HIGH_LEVEL_FAULT` | INS, TST | PTP-106, PTP-302 | PARTIAL | Final recovery sequencing may need refinement |
| HC-REQ-011 | HC shall monitor DUT2 output-voltage behavior | DUT2 monitoring | DUT2 voltage HLFs; `VAR_DUT2_VOUT_LOW_LIMIT`, `VAR_DUT2_VOUT_HIGH_LIMIT`, debounce vars | DUT2 measurement module; fault manager; TE status model | INS, TST, ANL | PTP-201, PTP-202, PTP-203 | PARTIAL | Final limits deferred |
| HC-REQ-012 | HC shall detect DUT2 regulation loss conditions | DUT2 fault detection | DUT2 undervoltage / overvoltage HLFs | DUT2 monitor logic; `HIGH_LEVEL_FAULT` transitions | INS, TST | PTP-202, PTP-203 | PARTIAL | Final numeric envelopes deferred |
| HC-REQ-013 | HC shall isolate DUT2 when DUT2 runtime faults require isolation | DUT2 protection | DUT2 HLF rows | ctrl_power; fault manager; State output behavior table | INS, TST | PTP-202, PTP-204 | SPEC | Default unaffected DUT preservation applies |
| HC-REQ-014 | HC shall preserve operation of an unaffected DUT by default when the other DUT faults | Fault policy / isolation | HLF runtime policy | Fault manager policy; State: `HIGH_LEVEL_FAULT`; output behavior table | INS, TST | PTP-105, PTP-202 | SPEC | Broader shutdown only for fault-specific override |
| HC-REQ-015 | HC shall latch LLFs and require reset-only recovery | Fault policy | All LLFs | Fault matrix; State: `LOW_LEVEL_FAULT`; command legality matrix | INS, TST | State LLF tests; PTP-303 | SPEC | No TE/debug clear |
| HC-REQ-016 | HC shall latch HLFs until explicitly cleared | Fault policy | All clearable HLFs | Fault manager; State: `HIGH_LEVEL_FAULT`; HLF clear preconditions | INS, TST | PTP-300, PTP-301, PTP-302, PTP-304 | SPEC | No auto-clear |
| HC-REQ-017 | HC shall allow HLF clear during Beam On if preconditions are satisfied | Fault policy / Beam On | HLF clear preconditions | TE interface safety rules; State HLF behavior | INS, TST | PTP-305 | SPEC | Beam On itself is not a restriction |
| HC-REQ-018 | Beam On shall impose no restrictions on allowable commands or operational modes | Operational policy | HLF-017 not used | TE interface; State command legality notes | INS, TST | PTP-033, PTP-305 | SPEC | Explicitly finalized policy |
| HC-REQ-019 | Warnings/degraded conditions shall not create a separate top-level state | Warning handling | WRN entries; TE-link absence degraded | State warning behavior; reporting requirements | INS, TST | PTP-002, PTP-023 | SPEC | Warnings coexist with Normal/Slave/HLF |
| HC-REQ-020 | USB absence after successful initialization shall be degraded-only | Communications policy | WRN USB absence | State warning behavior; TE status reporting | INS, TST | PTP-002, PTP-023 | SPEC | Distinct from catastrophic USB init failure |
| HC-REQ-021 | Catastrophic UART/USB initialization failure shall prevent normal operation | Boot/platform trust | catastrophic init handling | `BOOT` to `LOW_LEVEL_FAULT`; architecture boot sequencing | INS, TST | Boot-failure verification | SPEC | Runtime management distinction moot |
| HC-REQ-022 | HC shall provide queryable identity, version, status, and capabilities | TE interface | N/A | TE interface command families; reporting layer | INS, TST | PTP-010, PTP-011, PTP-012, PTP-020 | SPEC | Core TE observability |
| HC-REQ-023 | HC shall provide periodic status reporting over the TE interface | TE interface / reporting | `VAR_TE_STATUS_REPORT_INTERVAL` | Reporting module; `NORMAL`/`SLAVE`/`HIGH_LEVEL_FAULT` reporting requirements | INS, TST | PTP-021 | PARTIAL | Report cadence variable unresolved numerically |
| HC-REQ-024 | HC shall provide active fault and fault-detail visibility to the TE | TE interface / fault reporting | fault IDs + evidence variables | fault_manager; reporting; TE interface fault queries | INS, TST | PTP-300, PTP-301 | SPEC | Supports supervisory decisions |
| HC-REQ-025 | HC shall reject invalid or illegal commands consistently | TE interface robustness | N/A | parser/validator; state command legality matrix | INS, TST | PTP-400, PTP-401, PTP-402, PTP-403 | SPEC | Error semantics defined at interface level |
| HC-REQ-026 | HC shall support mode transitions between Normal and Slave | Operational modes | N/A | State transitions; TE commands | INS, TST | PTP-030, PTP-031, PTP-032 | PARTIAL | Some detailed Slave semantics still open |
| HC-REQ-027 | HC shall continue protection and monitoring while in Slave | Operational modes / protection | LLF/HLF policy remains active | State: `SLAVE`; architecture ownership model | INS, TST | Slave mode state tests | SPEC | Slave is not an unprotected mode |
| HC-REQ-028 | HC shall expose top-level state, active faults, warnings, DUT enables, and TE-link-active in reporting | Reporting | warning/fault variables and IDs | State reporting requirements; TE status/report schema | INS, TST | PTP-020, PTP-021, PTP-023, PTP-300 | PARTIAL | Final report field schema still open |
| HC-REQ-029 | HC shall use named variables instead of unresolved numeric TBDs | Documentation/config policy | all `VAR_*` placeholders | Variable registry; PRD; fault matrix; architecture integration | INS | document review against PRD/fault matrix/registry | SPEC | Numeric assignment deferred experimentally |
| HC-REQ-030 | HC shall maintain a deterministic firmware control model with centralized top-level state ownership | Architecture / implementation | N/A | Architecture scheduling rules; `sys_state` ownership; ISR capture only | INS, ANL, TST | architecture review; implementation unit/integration tests later | SPEC | Supports verification and maintainability |
| HC-REQ-031 | HC shall provide enough protocol support to supervise LTC3901 sync-fault-injection testing | DUT1 protocol support | DUT1 timing/current variables | TE interface + protocol test plan + state legality | INS, TST | PTP-103, PTP-104, PTP-105, PTP-106 | PARTIAL | Command grammar and exact names still open |
| HC-REQ-032 | HC shall provide enough protocol support to supervise LT8316 regulation-fault testing | DUT2 protocol support | DUT2 VOUT variables | TE interface + protocol test plan + state legality | INS, TST | PTP-201, PTP-202, PTP-203, PTP-204 | PARTIAL | External HV orchestration boundary still open |

## 6. Fault-to-Verification Matrix
| Fault Class / Example | Required Behavior | Verification Focus | Planned References | Status |
|---|---|---|---|---|
| LLF boot/init failure | safe OFF, latched, reset-only | boot fault path, command rejection, safe outputs | boot-failure tests; state LLF tests | SPEC |
| DUT1 overcurrent HLF | isolate DUT1, latch fault, preserve DUT2 by default | threshold crossing, fault visibility, isolation | PTP-105 | PARTIAL |
| DUT1 ME/MF cease operation | latch fault when expected operation absent | activity timeout handling, status/fault detail | PTP-103, PTP-104 | PARTIAL |
| DUT2 overvoltage HLF | isolate DUT2, latch fault, preserve DUT1 by default | output-high detection and reporting | PTP-202 | PARTIAL |
| DUT2 undervoltage HLF | isolate DUT2 or report per policy, latch fault | output-low detection and reporting | PTP-203 | PARTIAL |
| USB absent after successful init | degraded only, auto-clear | no false state escalation, status visibility | PTP-002, PTP-023 | SPEC |
| HLF clear during Beam On | allowed if preconditions met | clear success path with Beam On active | PTP-305 | SPEC |

## 7. Variable-Dependent Verification Items
The following requirement areas depend on deferred variable assignment and therefore remain partially specified until experimental values are available:

| Variable Area | Example Variables | Affected Requirements / Tests |
|---|---|---|
| DUT1 overcurrent limits | `VAR_DUT1_OVERCURRENT_LIMIT`, `VAR_DUT1_OVERCURRENT_DEBOUNCE` | HC-REQ-008, HC-REQ-009, PTP-105 |
| DUT1 activity timing | `VAR_DUT1_ME_ACTIVITY_TIMEOUT`, `VAR_DUT1_MF_ACTIVITY_TIMEOUT` | HC-REQ-007, PTP-103, PTP-104 |
| DUT2 voltage thresholds | `VAR_DUT2_VOUT_LOW_LIMIT`, `VAR_DUT2_VOUT_HIGH_LIMIT` and debounce vars | HC-REQ-011, HC-REQ-012, PTP-202, PTP-203 |
| TE report cadence | `VAR_TE_STATUS_REPORT_INTERVAL` | HC-REQ-023, PTP-021 |
| future recovery dwell values | future `VAR_*_RECOVERY_DWELL` if added | HC-REQ-010, HC-REQ-017, PTP-302, PTP-305 |

## 8. Coverage Gaps / Open Items
The following items are still open or partially open and should be resolved in future revisions:
- final command grammar and exact ASCII framing details
- final TE report field schema
- exact Slave-mode behavioral semantics in some edge cases
- exact mapping of all fault IDs to final command/query payloads
- detailed external HV supply orchestration boundary for DUT2 workflows
- final numeric assignments for all variable-based thresholds, timing windows, and debounce values
- whether any HLF subclasses should remain below top-level state transition scope

## 9. Recommended Next Steps
This matrix should next be used to drive:
1. implementation story creation
2. command-level protocol test vectors
3. verification procedure documents
4. eventual unit, integration, and radiation-test execution records

## 10. Revision Notes
- v1: Initial HC verification and traceability matrix created, linking PRD requirements to fault policy, architecture, TE interface behavior, state machine rules, and planned protocol verification.