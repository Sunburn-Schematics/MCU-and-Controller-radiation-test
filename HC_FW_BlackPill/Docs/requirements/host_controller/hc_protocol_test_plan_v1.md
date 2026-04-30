# Radiation Test Host Controller (HC) Protocol Test Plan v1

## 1. Document Purpose
This document defines a protocol-oriented test plan for the Radiation Test Host Controller (HC) and its interaction with the Test Executive (TE) while supervising radiation testing of:
- DUT1: LTC3901
- DUT2: LT8316

It incorporates the provided LTC3901 and LT8316 functional test concepts and maps them onto the HC/TE command, reporting, fault, and recovery architecture.

This is a BMAD-style draft intended to:
- verify the HC TE protocol supports the required test workflows
- verify HC command/response behavior during normal, degraded, and faulted conditions
- verify HC reporting supports radiation-test observations and recovery decisions
- preserve traceability between DUT functional test objectives and TE/HC protocol behavior

## 2. Scope
### 2.1 In Scope
- TE-to-HC command/response behavior over USB VCP
- HC periodic and event-driven reporting behavior
- protocol support for DUT1 and DUT2 test execution workflows
- protocol-visible fault detection, fault reporting, and fault recovery coordination
- state/mode interactions relevant to test execution
- degraded-link and reconnect behavior

### 2.2 Out of Scope
- direct electrical validation of DUT circuitry itself
- final experimentally derived threshold values
- external HV supply low-level protocol details, unless later integrated into HC/TE protocol
- beamline control protocol details outside HC visibility of Beam On

## 3. Source Inputs
This plan is based on:
- `/a0/usr/projects/bmad_test/docs/hc_prd_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_fault_response_matrix_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_variable_registry_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_architecture_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_te_interface_spec_v1.md`
- user-provided functional test concepts for LTC3901 and LT8316

## 4. Test Objectives
The protocol test plan shall verify that:
1. the TE can control and observe the HC through the required test phases
2. the HC exposes enough protocol-visible information to determine DUT normal/faulted behavior
3. the HC reports DUT-specific radiation-induced fault events in a machine-usable way
4. the HC supports recovery workflows through protocol commands and reporting
5. link degradation and reconnect behavior do not corrupt HC supervisory behavior
6. protocol semantics remain consistent across Normal, High-Level Fault, and Slave operation

## 5. Assumptions and Constraints
- USB VCP is the primary TE communication interface.
- USB absence after successful stack initialization is a degraded condition only.
- LLFs are reset-only.
- HLFs are latched until explicitly cleared.
- Beam On imposes no restrictions on allowable commands or operational modes.
- unresolved numeric thresholds and timing values are represented as named variables.
- debug may clear faults, but this test plan focuses primarily on TE protocol behavior.

## 6. Protocol Features Under Test
| Feature Area | What is being verified |
|---|---|
| Link establishment | TE can connect, query, disconnect, reconnect |
| Identity and capability | HC reports ID, version, capabilities |
| Status reporting | HC periodic and query-driven status is complete and consistent |
| Mode control | TE can query and request mode changes |
| DUT control | TE can control DUT1/DUT2 relevant test actions |
| Fault visibility | HC reports active faults/warnings clearly |
| Fault recovery | TE can request clear/retry/recovery actions where allowed |
| Event reporting | HC emits protocol-visible events for important transitions |

## 7. DUT1 Test Context: LTC3901
### 7.1 Functional Intent
The LTC3901 shall be biased in a representative operating condition, supplied at approximately 11V, stimulated by HC-generated sync signals, and observed for:
- correct ME/MF behavior
- timeout/fault behavior under sync interruption
- current changes associated with radiation-induced effects
- possible SEE and DSEE behavior

### 7.2 DUT1 Protocol-Relevant Observables
The TE shall be able to obtain or infer through HC protocol:
- DUT1 power state
- DUT1 sync generation state
- DUT1 ME activity state
- DUT1 MF activity state
- DUT1 ME/MF timing summaries
- DUT1 input current summary
- DUT1 relevant faults/warnings
- DUT1 recovery attempt outcomes

### 7.3 DUT1 Protocol-Relevant Controls
The TE should be able to request or coordinate through HC protocol:
- DUT1 power enable / disable
- DUT1 sync enable / disable
- status queries during irradiation
- fault detail retrieval
- fault clear after recovery preconditions are met

## 8. DUT2 Test Context: LT8316
### 8.1 Functional Intent
The LT8316 shall be tested under representative HV input conditions while the HC monitors protocol-visible evidence of regulation and failure behavior.

The TE should be able to supervise tests across increasing HV input conditions and detect conditions consistent with:
- normal 12V regulation
- output overvoltage
- output undervoltage
- regulation loss
- possible SEE / DSEE behavior

### 8.2 DUT2 Protocol-Relevant Observables
The TE shall be able to obtain or infer through HC protocol:
- DUT2 power/HV-enable state as seen by the HC
- DUT2 output-voltage summary
- DUT2 gate-activity summary if monitored through the HC design path
- DUT2 relevant faults/warnings
- DUT2 recovery attempt outcomes

### 8.3 DUT2 Protocol-Relevant Controls
The TE should be able to request or coordinate through HC protocol:
- DUT2 power/HV enable / disable actions available to the HC
- status queries during irradiation
- fault detail retrieval
- fault clear after recovery preconditions are met

## 9. Protocol Test Categories
### 9.1 Link and Session Tests
| Test ID | Title | Objective |
|---|---|---|
| PTP-001 | TE initial connect | Verify HC accepts TE link establishment and query traffic |
| PTP-002 | TE disconnect/reconnect | Verify degraded-only handling of link loss after successful init |
| PTP-003 | Query without prior activity | Verify HC responds correctly to initial `GET_*` queries |

### 9.2 Identity and Capability Tests
| Test ID | Title | Objective |
|---|---|---|
| PTP-010 | Get HC ID | Verify `GET_ID` behavior |
| PTP-011 | Get version | Verify `GET_VERSION` behavior |
| PTP-012 | Get capabilities | Verify `GET_CAPABILITIES` behavior |

### 9.3 Status Reporting Tests
| Test ID | Title | Objective |
|---|---|---|
| PTP-020 | Get status in Normal | Verify `GET_STATUS` completeness in Normal |
| PTP-021 | Periodic status emission | Verify `STS` periodic reporting while TE link is active |
| PTP-022 | Event-driven reporting | Verify `EVT` records on major transitions if enabled |
| PTP-023 | Link status reporting | Verify TE-link-active visibility in reports |

### 9.4 Mode and State Interaction Tests
| Test ID | Title | Objective |
|---|---|---|
| PTP-030 | Get mode | Verify `GET_MODE` behavior |
| PTP-031 | Enter Slave | Verify `ENTER_SLAVE` command behavior |
| PTP-032 | Exit Slave | Verify `EXIT_SLAVE` command behavior |
| PTP-033 | Commands during Beam On | Verify Beam On imposes no command restrictions |

### 9.5 DUT1 Workflow Tests
| Test ID | Title | Objective |
|---|---|---|
| PTP-100 | DUT1 power enable workflow | Verify TE can command and observe DUT1 power state |
| PTP-101 | DUT1 sync enable workflow | Verify TE can command and observe DUT1 sync state |
| PTP-102 | DUT1 normal operation reporting | Verify HC reports DUT1 nominal behavior |
| PTP-103 | DUT1 sync fault injection workflow | Verify HC/TE workflow for sync interruption testing |
| PTP-104 | DUT1 ME/MF cease-operation fault reporting | Verify protocol-visible fault behavior when ME/MF stop |
| PTP-105 | DUT1 overcurrent fault reporting | Verify protocol-visible fault behavior for current rise |
| PTP-106 | DUT1 recovery by power-cycle workflow | Verify protocol-visible recovery behavior after fault |

### 9.6 DUT2 Workflow Tests
| Test ID | Title | Objective |
|---|---|---|
| PTP-200 | DUT2 power enable workflow | Verify TE can command and observe DUT2/HV path state |
| PTP-201 | DUT2 normal regulation reporting | Verify HC reports DUT2 nominal output-voltage behavior |
| PTP-202 | DUT2 overvoltage fault reporting | Verify protocol-visible fault behavior for output high |
| PTP-203 | DUT2 undervoltage fault reporting | Verify protocol-visible fault behavior for output low |
| PTP-204 | DUT2 recovery workflow | Verify protocol-visible recovery coordination after fault |

### 9.7 Fault and Recovery Tests
| Test ID | Title | Objective |
|---|---|---|
| PTP-300 | Get active faults | Verify `GET_FAULTS` behavior |
| PTP-301 | Get fault detail | Verify `GET_FAULT_DETAIL` behavior |
| PTP-302 | Clear clearable HLF | Verify `CLEAR_FAULT` success path |
| PTP-303 | Reject LLF clear attempt | Verify LLF clear attempts are rejected |
| PTP-304 | Reject HLF clear with failed preconditions | Verify correct error response |
| PTP-305 | Clear during Beam On | Verify clear command allowed during Beam On |

### 9.8 Error and Robustness Tests
| Test ID | Title | Objective |
|---|---|---|
| PTP-400 | Unknown command | Verify `ERR` handling for unsupported command |
| PTP-401 | Invalid argument | Verify `ERR` handling for bad arguments |
| PTP-402 | Invalid state request | Verify state-rule rejection behavior |
| PTP-403 | Partial or malformed line | Verify syntax error handling |
| PTP-404 | Repeated polling under degraded link | Verify stable behavior under reconnect cycles |

## 10. Detailed Test Procedures
## 10.1 PTP-001 TE Initial Connect
### Objective
Verify the HC supports initial TE interaction over USB VCP.

### Preconditions
- HC powered and initialized successfully
- TE link available

### Procedure
1. Connect TE to HC USB VCP.
2. Issue `PING`.
3. Issue `GET_ID`.
4. Issue `GET_VERSION`.
5. Issue `GET_STATUS`.

### Expected Results
- HC responds on VCP without reset requirement.
- `PING` succeeds.
- identity/version/status data is returned.
- TE-link-active state is reflected consistently.

## 10.2 PTP-021 Periodic Status Emission
### Objective
Verify periodic status reports are emitted while TE communications are active.

### Preconditions
- HC in a reporting-capable state
- TE link active

### Procedure
1. Establish TE connection.
2. Observe incoming reports for at least several intervals.
3. Compare observed report cadence against `VAR_TE_STATUS_REPORT_INTERVAL` once implemented.

### Expected Results
- `STS` reports are emitted periodically.
- report fields are internally consistent.
- report cadence is stable within implementation tolerance.

## 10.3 PTP-103 DUT1 Sync Fault Injection Workflow
### Objective
Verify the protocol supports DUT1 sync interruption testing and observation.

### Preconditions
- DUT1 present and enabled
- sync generation active
- DUT1 in expected normal operation

### Procedure
1. Query baseline DUT1 status.
2. Command DUT1 sync disable or equivalent fault-injection action.
3. Observe subsequent `GET_DUT1_STATUS`, `GET_FAULTS`, and/or `EVT` indications.
4. After the defined fault-injection dwell, command sync re-enable.
5. Query DUT1 status and fault detail again.

### Expected Results
- HC reflects sync disable state.
- DUT1 ME/MF behavior transitions are visible through protocol-visible summaries/faults.
- recovery or non-recovery is visible in DUT1 status/fault detail.
- if fault criteria are met, the corresponding HLF appears latched until cleared.

### Variable Dependencies
- `VAR_DUT1_ME_ACTIVITY_TIMEOUT`
- `VAR_DUT1_MF_ACTIVITY_TIMEOUT`
- any sync fault injection dwell/recovery timing variables later added

## 10.4 PTP-105 DUT1 Overcurrent Fault Reporting
### Objective
Verify DUT1 overcurrent behavior is exposed correctly through the protocol.

### Preconditions
- DUT1 enabled
- overcurrent detection path active

### Procedure
1. Establish baseline DUT1 current reporting.
2. Apply a test condition that causes DUT1 current to exceed the defined threshold.
3. Query status, faults, and fault detail.
4. Verify DUT1 isolation behavior.

### Expected Results
- HC reports fault assertion corresponding to DUT1 overcurrent.
- DUT1 power path is isolated per policy.
- DUT2 remains unaffected by default.
- fault detail includes relevant evidence when supported.

### Variable Dependencies
- `VAR_DUT1_OVERCURRENT_LIMIT`
- `VAR_DUT1_OVERCURRENT_DEBOUNCE`

## 10.5 PTP-202 DUT2 Overvoltage Fault Reporting
### Objective
Verify DUT2 output-overvoltage behavior is exposed correctly through the protocol.

### Preconditions
- DUT2 active in a representative operating condition

### Procedure
1. Establish baseline DUT2 output-voltage reporting.
2. Apply a condition that causes DUT2 output voltage to rise beyond the allowed range.
3. Query status, faults, and fault detail.
4. Observe DUT2 isolation/recovery coordination.

### Expected Results
- HC reports DUT2 high-voltage output fault.
- DUT2 power/HV path is disabled or reported according to policy.
- DUT1 remains unaffected by default.
- protocol-visible evidence is sufficient for TE supervisory logic.

### Variable Dependencies
- `VAR_DUT2_VOUT_HIGH_LIMIT`
- `VAR_DUT2_VOUT_HIGH_DEBOUNCE`

## 10.6 PTP-302 Clear Clearable HLF
### Objective
Verify TE can clear a clearable HLF after preconditions are satisfied.

### Preconditions
- a clearable HLF is active and latched
- underlying condition has returned to normal

### Procedure
1. Query `GET_FAULT_DETAIL <fault_id>`.
2. Verify clear eligibility or infer preconditions are satisfied.
3. Issue `CLEAR_FAULT <fault_id>`.
4. Query `GET_FAULTS` and `GET_STATUS`.

### Expected Results
- HC accepts the clear command.
- fault clears from the active/latched set.
- status output reflects post-clear condition.

## 10.7 PTP-305 Clear During Beam On
### Objective
Verify fault clear commands are allowed during Beam On.

### Preconditions
- Beam On active
- a clearable HLF present
- all clear preconditions satisfied

### Procedure
1. Confirm Beam On state using `GET_BEAM_STATUS` or status report.
2. Issue `CLEAR_FAULT <fault_id>`.
3. Query `GET_FAULTS` and `GET_STATUS`.

### Expected Results
- command is not rejected solely due to Beam On.
- clear succeeds if all other preconditions are satisfied.

## 11. DUT-Specific Protocol Traceability
### 11.1 LTC3901 Functional Test Traceability
| DUT1 Functional Intent | Protocol Need |
|---|---|
| Bias and enable DUT1 | commandable DUT1 power control |
| Apply sync | commandable/observable sync control |
| Monitor ME/MF operation | DUT1 status and/or fault reporting |
| Detect loss of operation | fault/event visibility |
| Observe current rise | current summary and fault detail |
| Attempt recovery by power cycle | TE-visible recovery workflow |

### 11.2 LT8316 Functional Test Traceability
| DUT2 Functional Intent | Protocol Need |
|---|---|
| Enable DUT2 test condition | commandable DUT2/HV-related control visible to HC |
| Monitor output voltage | DUT2 status and periodic reporting |
| Detect loss of regulation | fault/event visibility |
| Coordinate shutdown/retry | TE-visible recovery workflow |
| Progress through test conditions | stable command/status/reconnect behavior |

## 12. Pass/Fail Philosophy
A protocol test passes when:
- the HC returns the expected protocol-visible semantics for the scenario
- faults and recoveries are reported consistently with policy
- status information is sufficiently complete for TE supervision
- no prohibited side effects occur on the unaffected DUT when isolation policy says otherwise

A protocol test fails when:
- required commands are unavailable or inconsistent
- expected status/fault information is missing or ambiguous
- command acceptance/rejection conflicts with documented policy
- reporting does not support supervisory decision making

## 13. Open Items
The following still need refinement in later revisions:
- exact command grammar and final ASCII framing
- whether asynchronous `EVT` output is mandatory or configurable
- exact mapping of DUT functional workflows to final HC command names
- whether external HV supply orchestration is directly inside HC scope or TE-only scope
- detailed report field schema
- protocol timing tolerances beyond the variable-based placeholders

## 14. Recommended Next Steps
This test plan should next be paired with:
1. formal state machine specification
2. verification and traceability matrix
3. command-level protocol test vectors
4. story backlog for implementation and test automation

## 15. Revision Notes
- v1: Initial HC protocol test plan created from the provided LTC3901 and LT8316 functional test descriptions and aligned to the HC PRD, fault model, variable registry, architecture, and TE interface spec.
