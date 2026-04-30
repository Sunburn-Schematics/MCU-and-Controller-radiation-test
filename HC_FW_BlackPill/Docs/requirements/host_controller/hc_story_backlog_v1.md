# Radiation Test Host Controller (HC) Story Backlog v1

## 1. Document Purpose
This document defines the initial implementation story backlog for the Radiation Test Host Controller (HC).

It decomposes the approved requirements, fault policy, architecture, TE interface, protocol test plan, state machine, and verification matrix into executable firmware-focused work items.

The backlog is intended to:
- organize implementation into manageable increments
- preserve traceability to HC requirements and verification artifacts
- separate foundational platform work from DUT-specific features
- support later sprint planning and task breakdown

## 2. Source Documents
This backlog is derived from:
- `/a0/usr/projects/bmad_test/docs/hc_prd_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_fault_response_matrix_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_variable_registry_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_architecture_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_te_interface_spec_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_protocol_test_plan_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_state_machine_spec_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_verification_traceability_matrix_v1.md`

## 3. Backlog Structure
Stories are grouped into these implementation themes:
1. platform and safe boot foundation
2. top-level state and fault management
3. TE communications and reporting
4. DUT1 control and monitoring
5. DUT2 control and monitoring
6. diagnostics, logging, and verification support

## 4. Story Format
Each story includes:
- story ID
- title
- objective
- key scope
- dependencies
- primary traceability references
- acceptance focus

## 5. Backlog Stories
## Epic A: Platform Foundation

### HC-STORY-001 — Establish project firmware skeleton and module layout
**Objective**
Create the base firmware structure matching the approved architecture.

**Key Scope**
- create module directories/files consistent with architecture
- define top-level init flow
- define shared headers, types, fault IDs, and state enums
- define variable placeholder integration points

**Dependencies**
- architecture v1

**Primary Traceability**
- HC-REQ-001
- HC-REQ-029
- HC-REQ-030

**Acceptance Focus**
- module layout matches architecture document
- shared enums and interfaces exist for state/fault/reporting use
- no application logic yet required beyond scaffolding

### HC-STORY-002 — Implement safe-default boot and output-safeing behavior
**Objective**
Ensure the HC enters a safe, non-energized condition during boot and reset handling.

**Key Scope**
- set all controllable outputs to safe defaults
- ensure DUT1 power OFF
- ensure DUT2 power/HV-related enable OFF
- ensure DUT1 sync outputs OFF
- establish boot sequencing entry behavior

**Dependencies**
- HC-STORY-001

**Primary Traceability**
- HC-REQ-001
- HC-REQ-021

**Acceptance Focus**
- safe outputs are asserted before operational enable
- boot path reflects state-machine `BOOT` requirements

### HC-STORY-003 — Implement hardware ID readout service
**Objective**
Read and expose the 6-bit hardware ID.

**Key Scope**
- initialize switch inputs
- read hardware ID deterministically
- expose ID to reporting/query layers

**Dependencies**
- HC-STORY-001
- HC-STORY-002

**Primary Traceability**
- HC-REQ-002

**Acceptance Focus**
- hardware ID is readable and queryable

## Epic B: State and Fault Core

### HC-STORY-010 — Implement authoritative top-level state manager
**Objective**
Implement the formal top-level HC state machine.

**Key Scope**
- implement `BOOT`, `LOW_LEVEL_FAULT`, `HIGH_LEVEL_FAULT`, `NORMAL`, `SLAVE`
- implement state transition priority rules
- implement entry handlers and exit validation
- centralize top-level state ownership in `sys_state`

**Dependencies**
- HC-STORY-001
- HC-STORY-002

**Primary Traceability**
- HC-REQ-015
- HC-REQ-016
- HC-REQ-026
- HC-REQ-027
- HC-REQ-030

**Acceptance Focus**
- transitions follow state-machine spec
- LLF takes priority over all lower-priority transitions
- Normal/Slave transitions obey command legality

### HC-STORY-011 — Implement fault manager and latching model
**Objective**
Implement LLF/HLF/WRN storage, latching, prioritization, and clear rules.

**Key Scope**
- represent LLFs, HLFs, WRNs and their metadata
- latch LLFs reset-only
- latch HLFs until explicit clear
- auto-clear warnings where required
- record occurrence count and timestamps if supported

**Dependencies**
- HC-STORY-010

**Primary Traceability**
- HC-REQ-015
- HC-REQ-016
- HC-REQ-019
- HC-REQ-024

**Acceptance Focus**
- LLFs cannot be cleared by TE/debug command path
- HLF clear preconditions are enforceable
- warnings do not force illegal top-level state transitions

### HC-STORY-012 — Implement catastrophic boot-failure handling
**Objective**
Handle catastrophic initialization failures as non-operational safe-off conditions.

**Key Scope**
- detect catastrophic UART/USB init failures
- route failures to low-level fault/non-operational behavior
- preserve safe outputs and boot failure visibility as feasible

**Dependencies**
- HC-STORY-002
- HC-STORY-010
- HC-STORY-011

**Primary Traceability**
- HC-REQ-003
- HC-REQ-020
- HC-REQ-021

**Acceptance Focus**
- catastrophic init failure prevents normal operation
- safe-off behavior preserved

## Epic C: TE Communications and Reporting

### HC-STORY-020 — Implement USB VCP transport and session handling
**Objective**
Bring up the primary TE communications path over USB VCP.

**Key Scope**
- initialize USB VCP transport
- detect active/inactive TE connection state
- expose link-active status to higher layers
- handle disconnect/reconnect behavior as degraded-only after successful init

**Dependencies**
- HC-STORY-001
- HC-STORY-012

**Primary Traceability**
- HC-REQ-003
- HC-REQ-004
- HC-REQ-020

**Acceptance Focus**
- TE can connect and reconnect without false LLF behavior
- link-active status is observable

### HC-STORY-021 — Implement TE command parser and response framework
**Objective**
Implement the TE command/response core defined by the interface specification.

**Key Scope**
- line-oriented command parsing
- response categories (`OK`, `ERR`, `RSP`, `STS`, `EVT` as applicable)
- argument validation and error responses
- command dispatch routing

**Dependencies**
- HC-STORY-020
- HC-STORY-010
- HC-STORY-011

**Primary Traceability**
- HC-REQ-022
- HC-REQ-025
- HC-REQ-031
- HC-REQ-032

**Acceptance Focus**
- valid commands route correctly
- invalid or malformed commands are rejected consistently

### HC-STORY-022 — Implement identity, capability, and status query commands
**Objective**
Expose required HC observability to the TE.

**Key Scope**
- `GET_ID`
- `GET_VERSION`
- `GET_CAPABILITIES`
- `GET_STATUS`
- state/fault/warning/link-active fields

**Dependencies**
- HC-STORY-021
- HC-STORY-003
- HC-STORY-010
- HC-STORY-011

**Primary Traceability**
- HC-REQ-002
- HC-REQ-004
- HC-REQ-022
- HC-REQ-028

**Acceptance Focus**
- TE can query core HC identity and supervisory status

### HC-STORY-023 — Implement periodic and event-driven reporting
**Objective**
Provide protocol-visible status and transition reporting.

**Key Scope**
- periodic `STS` generation
- event reporting for major transitions/faults if enabled
- use `VAR_TE_STATUS_REPORT_INTERVAL`
- expose top-level state, faults, warnings, DUT enables, TE-link-active

**Dependencies**
- HC-STORY-022

**Primary Traceability**
- HC-REQ-023
- HC-REQ-024
- HC-REQ-028

**Acceptance Focus**
- periodic reports occur at configured interval placeholder
- transition/fault visibility supports TE supervision

### HC-STORY-024 — Implement fault query and clear command set
**Objective**
Expose fault visibility and controlled HLF clearing through TE commands.

**Key Scope**
- `GET_FAULTS`
- `GET_FAULT_DETAIL`
- `CLEAR_FAULT`
- LLF clear rejection
- HLF clear precondition validation
- clear allowed during Beam On

**Dependencies**
- HC-STORY-021
- HC-STORY-011
- HC-STORY-010

**Primary Traceability**
- HC-REQ-015
- HC-REQ-016
- HC-REQ-017
- HC-REQ-024
- HC-REQ-025

**Acceptance Focus**
- LLFs are never clearable by command
- HLF clear succeeds only when allowed
- Beam On is not used as a rejection reason

### HC-STORY-025 — Implement mode control commands for Normal and Slave
**Objective**
Support protocol-visible transitions between Normal and Slave operation.

**Key Scope**
- enter Slave command
- exit Slave command
- legality enforcement by state
- mode/state reporting updates

**Dependencies**
- HC-STORY-021
- HC-STORY-010

**Primary Traceability**
- HC-REQ-018
- HC-REQ-026
- HC-REQ-027

**Acceptance Focus**
- mode transitions obey state-machine rules
- Beam On does not block mode commands

## Epic D: DUT1 Control and Monitoring

### HC-STORY-030 — Implement DUT1 power control path
**Objective**
Control DUT1 power-enable output under state/fault policy.

**Key Scope**
- enable/disable DUT1 power
- honor safe-off requirements in boot and LLF
- isolate DUT1 under required HLFs

**Dependencies**
- HC-STORY-010
- HC-STORY-011

**Primary Traceability**
- HC-REQ-009
- HC-REQ-014

**Acceptance Focus**
- DUT1 power behavior matches state and fault policy

### HC-STORY-031 — Implement DUT1 sync generation control
**Objective**
Generate and control DUT1 quadrature sync outputs.

**Key Scope**
- initialize timer/output resources
- generate sync outputs using variable-based timing placeholders
- allow sync enable/disable control
- disable sync in safe-off states and when DUT1 isolation requires it

**Dependencies**
- HC-STORY-030

**Primary Traceability**
- HC-REQ-006
- HC-REQ-031

**Acceptance Focus**
- sync generation is controllable and state-aware
- exact numeric timing remains variable-driven

### HC-STORY-032 — Implement DUT1 measurement acquisition and summaries
**Objective**
Acquire the DUT1 measurements required for supervision and reporting.

**Key Scope**
- ME/MF activity capture or summary generation
- DUT1 current measurement path
- status summary outputs for TE queries/reports
- variable-driven timeouts and envelopes

**Dependencies**
- HC-STORY-001
- HC-STORY-023

**Primary Traceability**
- HC-REQ-005
- HC-REQ-007
- HC-REQ-008
- HC-REQ-031

**Acceptance Focus**
- TE-visible DUT1 status is available
- measurement pipeline supports fault logic inputs

### HC-STORY-033 — Implement DUT1 fault detection and recovery workflow
**Objective**
Detect DUT1 runtime faults and support controlled recovery attempts.

**Key Scope**
- ME/MF cease-operation fault detection
- overcurrent fault detection
- affected-DUT isolation behavior
- support TE-visible power-cycle/recovery workflow

**Dependencies**
- HC-STORY-011
- HC-STORY-030
- HC-STORY-031
- HC-STORY-032
- HC-STORY-024

**Primary Traceability**
- HC-REQ-007
- HC-REQ-008
- HC-REQ-009
- HC-REQ-010
- HC-REQ-014
- HC-REQ-031

**Acceptance Focus**
- DUT1 faults latch correctly as HLFs
- DUT2 remains unaffected by default
- recovery path is TE-visible and policy-compliant

## Epic E: DUT2 Control and Monitoring

### HC-STORY-040 — Implement DUT2 power/HV enable control path
**Objective**
Control the DUT2-related enable path available to the HC.

**Key Scope**
- DUT2 enable/disable path control
- honor safe-off requirements in boot and LLF
- isolate DUT2 under required HLFs

**Dependencies**
- HC-STORY-010
- HC-STORY-011

**Primary Traceability**
- HC-REQ-013
- HC-REQ-014

**Acceptance Focus**
- DUT2 power/HV-related enable obeys state/fault policy

### HC-STORY-041 — Implement DUT2 measurement acquisition and summaries
**Objective**
Acquire the DUT2 measurements required for supervision and reporting.

**Key Scope**
- output-voltage measurement
- gate-activity summary if supported by final hardware path
- status summary outputs for TE queries/reports
- variable-driven thresholds and debounce integration

**Dependencies**
- HC-STORY-001
- HC-STORY-023

**Primary Traceability**
- HC-REQ-011
- HC-REQ-012
- HC-REQ-032

**Acceptance Focus**
- TE-visible DUT2 status is available
- measurement pipeline supports regulation-loss fault logic

### HC-STORY-042 — Implement DUT2 fault detection and recovery workflow
**Objective**
Detect DUT2 runtime faults and support controlled recovery attempts.

**Key Scope**
- overvoltage detection
- undervoltage/regulation-loss detection
- affected-DUT isolation behavior
- support TE-visible recovery workflow

**Dependencies**
- HC-STORY-011
- HC-STORY-040
- HC-STORY-041
- HC-STORY-024

**Primary Traceability**
- HC-REQ-011
- HC-REQ-012
- HC-REQ-013
- HC-REQ-014
- HC-REQ-032

**Acceptance Focus**
- DUT2 faults latch correctly as HLFs
- DUT1 remains unaffected by default
- recovery path is TE-visible and policy-compliant

## Epic F: Diagnostics, Logging, and Verification Support

### HC-STORY-050 — Implement internal event/fault evidence capture hooks
**Objective**
Capture enough evidence to support fault-detail reporting and later verification.

**Key Scope**
- fault timestamps
- occurrence counters
- last measured value snapshots where practical
- event hooks for major state/fault transitions

**Dependencies**
- HC-STORY-011
- HC-STORY-023

**Primary Traceability**
- HC-REQ-024
- HC-REQ-028

**Acceptance Focus**
- fault-detail queries return meaningful context where supported

### HC-STORY-051 — Implement protocol test hooks and harness support
**Objective**
Support execution of the HC protocol test plan.

**Key Scope**
- test-friendly command/status observability
- deterministic reporting behavior
- support for DUT1 sync-fault-injection workflows
- support for DUT2 verification workflows

**Dependencies**
- HC-STORY-021
- HC-STORY-023
- HC-STORY-033
- HC-STORY-042

**Primary Traceability**
- HC-REQ-031
- HC-REQ-032

**Acceptance Focus**
- protocol test plan can be executed against the firmware implementation

### HC-STORY-052 — Align variable placeholders with compile-time/runtime configuration model
**Objective**
Decide and implement where named variables live in firmware configuration.

**Key Scope**
- classify variables as compile-time, calibration-time, or runtime-configurable
- define configuration ownership and interfaces
- maintain naming alignment with variable registry

**Dependencies**
- HC-STORY-001
- HC-STORY-032
- HC-STORY-041

**Primary Traceability**
- HC-REQ-023
- HC-REQ-029
- HC-REQ-031
- HC-REQ-032

**Acceptance Focus**
- registry variables map cleanly to firmware configuration points

## 6. Suggested Implementation Order
| Order | Story ID | Reason |
|---|---|---|
| 1 | HC-STORY-001 | foundation |
| 2 | HC-STORY-002 | safe boot first |
| 3 | HC-STORY-003 | low-risk observability |
| 4 | HC-STORY-010 | top-level control backbone |
| 5 | HC-STORY-011 | fault policy backbone |
| 6 | HC-STORY-012 | catastrophic boot handling |
| 7 | HC-STORY-020 | TE transport |
| 8 | HC-STORY-021 | command framework |
| 9 | HC-STORY-022 | basic observability |
| 10 | HC-STORY-023 | status/event reporting |
| 11 | HC-STORY-024 | fault query/clear |
| 12 | HC-STORY-025 | mode control |
| 13 | HC-STORY-030 | DUT1 power |
| 14 | HC-STORY-031 | DUT1 sync |
| 15 | HC-STORY-032 | DUT1 measurement |
| 16 | HC-STORY-033 | DUT1 fault/recovery |
| 17 | HC-STORY-040 | DUT2 enable path |
| 18 | HC-STORY-041 | DUT2 measurement |
| 19 | HC-STORY-042 | DUT2 fault/recovery |
| 20 | HC-STORY-050 | evidence capture |
| 21 | HC-STORY-051 | protocol test support |
| 22 | HC-STORY-052 | variable/config alignment |

## 7. Open Backlog Questions
The following should be refined in future backlog revisions:
- exact story point sizing or effort estimation
- hardware dependencies and board bring-up prerequisites per story
- which stories require host-side TE tooling changes
- whether DUT2 HV supply orchestration belongs in HC firmware scope or TE-only scope
- whether some stories should be split into driver-level and application-level sub-stories

## 8. Recommended Next Steps
This backlog should next be used to produce either:
1. a sprint-ready implementation plan
2. command-level protocol test vectors
3. a task-level decomposition for the first implementation stories

## 9. Revision Notes
- v1: Initial HC implementation story backlog created from the approved PRD, fault policy, architecture, TE interface, protocol test plan, state machine, and verification traceability matrix.
