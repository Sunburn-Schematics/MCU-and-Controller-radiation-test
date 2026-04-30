# Radiation Test Host Controller (HC) State Machine Specification v1

## 1. Document Purpose
This document defines the formal firmware state machine for the Radiation Test Host Controller (HC).

It translates the HC PRD, fault-response policy, architecture, TE interface specification, and protocol test plan into explicit state behavior suitable for implementation and verification.

This specification defines:
- top-level HC states
- entry and exit conditions
- state invariants
- commanded transitions
- fault-driven transitions
- output behavior per state
- command legality by state
- reporting expectations by state

## 2. Source Documents
This specification is derived from:
- `/a0/usr/projects/bmad_test/docs/hc_prd_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_fault_response_matrix_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_variable_registry_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_architecture_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_te_interface_spec_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_protocol_test_plan_v1.md`

## 3. Governing Policy Summary
The state machine shall obey these already-decided policies:
- Low-Level Faults (LLFs) are latched and reset-only.
- High-Level Faults (HLFs) are latched until explicitly cleared.
- Beam On imposes no restrictions on allowable commands or operational modes.
- USB absence after successful initialization is a degraded condition only.
- Runtime DUT faults isolate only the affected DUT by default unless a specific fault requires broader shutdown.
- All unresolved numeric timing and threshold values shall be represented using named variables.

## 4. State Model Overview
The HC top-level state machine has five states:
1. `BOOT`
2. `LOW_LEVEL_FAULT`
3. `HIGH_LEVEL_FAULT`
4. `NORMAL`
5. `SLAVE`

These states are mutually exclusive at the top level.

## 5. State Definitions
## 5.1 State: BOOT
### Intent
`BOOT` is the initialization and self-check state entered after power-up or reset.

### Entry Conditions
The HC enters `BOOT` on:
- power-on reset
- external reset
- watchdog reset
- software reset if implemented

### Required Actions on Entry
On entry to `BOOT`, the HC shall:
- place all controllable outputs in their safe default states
- disable DUT1 and DUT2 power-enable outputs
- disable DUT1 sync outputs
- initialize clocks and core MCU services
- initialize GPIO, ADC, timers, communications, and reporting subsystems
- read hardware ID inputs
- initialize state, fault, warning, and event records
- perform required self-checks

### State Invariants
While in `BOOT`:
- DUT1 power shall remain OFF
- DUT2 power/HV-related enable shall remain OFF
- DUT1 sync outputs shall remain disabled
- no TE command shall cause operational power enable before boot completion

### Exit Conditions
`BOOT` exits only when:
- required initialization and self-checks succeed, transitioning to `NORMAL`, or
- a catastrophic/low-level boot failure is detected, transitioning to `LOW_LEVEL_FAULT`

### Notes
USB/UART initialization failures are treated as catastrophic platform failures. The practical result is transition to a non-operational condition rather than nuanced runtime handling.

## 5.2 State: LOW_LEVEL_FAULT
### Intent
`LOW_LEVEL_FAULT` indicates the HC itself is not trustworthy for normal supervisory operation.

### Entry Conditions
The HC enters `LOW_LEVEL_FAULT` when:
- a boot-time LLF is detected
- a runtime LLF is detected
- a catastrophic subsystem failure is detected

### Required Actions on Entry
On entry to `LOW_LEVEL_FAULT`, the HC shall:
- force all controllable outputs to safe OFF states
- disable DUT1 power-enable
- disable DUT2 power/HV-related enable
- disable DUT1 sync generation
- latch the triggering LLF(s)
- record fault IDs, timestamps, and supporting evidence where available
- update status/LED/reporting outputs as far as the remaining platform allows

### State Invariants
While in `LOW_LEVEL_FAULT`:
- DUT1 power shall remain OFF
- DUT2 power/HV-related enable shall remain OFF
- DUT1 sync outputs shall remain disabled
- LLFs shall remain latched
- no command shall clear an LLF
- no command shall transition directly to `NORMAL` or `SLAVE`

### Allowed Exit Conditions
`LOW_LEVEL_FAULT` may be exited only by reset followed by successful re-entry through `BOOT`.

## 5.3 State: HIGH_LEVEL_FAULT
### Intent
`HIGH_LEVEL_FAULT` indicates one or more runtime DUT/application faults have been detected while the HC platform remains operational.

### Entry Conditions
The HC enters `HIGH_LEVEL_FAULT` when:
- any HLF becomes active and latched
- any DUT-specific runtime protection rule requires top-level faulted supervisory state

### Required Actions on Entry
On entry to `HIGH_LEVEL_FAULT`, the HC shall:
- latch the triggering HLF(s)
- isolate the affected DUT by default
- preserve unaffected DUT operation by default unless the specific fault requires broader shutdown
- record fault IDs, timestamps, and supporting measurements where available
- update LED/status/event reporting

### State Invariants
While in `HIGH_LEVEL_FAULT`:
- active HLFs remain latched until explicitly cleared
- LLFs, if detected, take precedence and force transition to `LOW_LEVEL_FAULT`
- commands remain generally allowed subject to command-specific validation
- unaffected DUTs may continue operating if policy permits

### Allowed Exit Conditions
`HIGH_LEVEL_FAULT` may exit when:
- all active latched HLFs have been cleared successfully, transitioning to `NORMAL`, or
- a command explicitly places the HC into `SLAVE` and policy permits transition with active/cleared conditions as implemented, or
- a reset occurs, transitioning to `BOOT`

### Notes
Beam On does not restrict fault clearing. HLF clear still requires the defined preconditions to be satisfied.

## 5.4 State: NORMAL
### Intent
`NORMAL` is the standard supervisory operating state in which the HC performs commanded control, monitoring, fault detection, and reporting.

### Entry Conditions
The HC enters `NORMAL` when:
- `BOOT` completes successfully with no LLF present
- all active HLFs have been cleared from `HIGH_LEVEL_FAULT`
- `SLAVE` is exited back to standard supervisory operation

### Required Actions on Entry
On entry to `NORMAL`, the HC shall:
- set state bookkeeping accordingly
- maintain safe output defaults unless explicit valid commands enable DUT functions
- enable regular status/reporting behavior
- keep fault and warning histories available for TE query

### State Invariants
While in `NORMAL`:
- the HC may control DUT1 and DUT2 subject to command validation
- the HC shall monitor all required measurement and fault-detection paths
- warnings may assert and clear without forcing exit from `NORMAL`
- Beam On imposes no restrictions on commands or modes

### Allowed Exit Conditions
`NORMAL` may exit to:
- `HIGH_LEVEL_FAULT` on HLF detection
- `LOW_LEVEL_FAULT` on LLF detection
- `SLAVE` on valid command
- `BOOT` on reset

## 5.5 State: SLAVE
### Intent
`SLAVE` is an alternate operational state in which the HC remains active but follows the semantics defined for Slave operation in the broader system.

### Entry Conditions
The HC enters `SLAVE` when:
- TE issues a valid `ENTER_SLAVE` or equivalent command
- current platform and policy conditions permit the transition

### Required Actions on Entry
On entry to `SLAVE`, the HC shall:
- record the mode/state transition
- maintain required monitoring and protection functions
- continue status, fault, and warning reporting
- preserve safe behavior for any outputs not explicitly commanded otherwise

### State Invariants
While in `SLAVE`:
- LLF and HLF detection remain active
- Beam On still imposes no restrictions on commands or mode occupancy
- safety/protection actions still override supervisory requests where required
- the HC remains queryable through the TE interface if communications are available

### Allowed Exit Conditions
`SLAVE` may exit to:
- `NORMAL` on valid `EXIT_SLAVE` command
- `HIGH_LEVEL_FAULT` on HLF detection
- `LOW_LEVEL_FAULT` on LLF detection
- `BOOT` on reset

## 6. State Priority Rules
If more than one transition condition exists simultaneously, the HC shall apply this priority:

1. `LOW_LEVEL_FAULT`
2. `HIGH_LEVEL_FAULT`
3. commanded reset to `BOOT`
4. commanded transition between `NORMAL` and `SLAVE`
5. remain in current state

This ensures platform-trust faults override runtime and supervisory behavior.

## 7. Top-Level Transition Table
| From State | Trigger | To State | Notes |
|---|---|---|---|
| BOOT | Boot complete, no LLF | NORMAL | Standard successful startup |
| BOOT | LLF/catastrophic boot failure | LOW_LEVEL_FAULT | Safe OFF, latch fault |
| NORMAL | HLF asserted | HIGH_LEVEL_FAULT | Isolate affected DUT by default |
| NORMAL | LLF asserted | LOW_LEVEL_FAULT | Platform fault takes priority |
| NORMAL | Valid enter-slave command | SLAVE | Beam On does not block |
| HIGH_LEVEL_FAULT | All active HLFs cleared | NORMAL | Preconditions must be satisfied |
| HIGH_LEVEL_FAULT | LLF asserted | LOW_LEVEL_FAULT | Escalation |
| HIGH_LEVEL_FAULT | Valid enter-slave command | SLAVE | Allowed if implementation permits |
| SLAVE | Valid exit-slave command | NORMAL | Return to standard supervision |
| SLAVE | HLF asserted | HIGH_LEVEL_FAULT | Protection still active |
| SLAVE | LLF asserted | LOW_LEVEL_FAULT | Platform fault takes priority |
| ANY | Reset | BOOT | Full restart path |

## 8. DUT Output Behavior by State
| State | DUT1 Power | DUT1 Sync | DUT2 Power/HV Enable | Notes |
|---|---|---|---|---|
| BOOT | OFF | OFF | OFF | Safe defaults until boot success |
| LOW_LEVEL_FAULT | OFF | OFF | OFF | Forced safe OFF |
| HIGH_LEVEL_FAULT | Affected DUT OFF by default | OFF if DUT1 affected | Affected DUT OFF by default | Unaffected DUT may continue |
| NORMAL | Commanded | Commanded | Commanded | Subject to validation |
| SLAVE | Commanded | Commanded | Commanded | Subject to validation |

## 9. Warning Behavior
Warnings do not define a separate top-level state.

Instead:
- warnings may coexist with `NORMAL` or `SLAVE`
- warnings may coexist with `HIGH_LEVEL_FAULT`
- warnings do not override LLF/HLF state transitions
- warnings auto-clear when the underlying condition disappears unless otherwise specified

Example:
- TE USB link absent after successful initialization is a degraded warning condition and does not force exit from `NORMAL`

## 10. Command Legality by State
## 10.1 General Rule
Unless otherwise stated, commands are interpreted according to:
- platform trust
- state legality
- DUT/fault preconditions
- command-specific argument validity

## 10.2 Command Legality Matrix
| Command Class | BOOT | LOW_LEVEL_FAULT | HIGH_LEVEL_FAULT | NORMAL | SLAVE |
|---|---|---|---|---|---|
| Identity/status query | Limited/if available | Yes, if platform supports | Yes | Yes | Yes |
| Fault query | Limited/if available | Yes | Yes | Yes | Yes |
| Clear HLF | No | No | Yes | Yes, if HLF present | Yes, if HLF present |
| Clear LLF | No | No | No | No | No |
| Enter Slave | No | No | Implementation-defined/allowed by policy | Yes | N/A |
| Exit Slave | No | No | N/A | N/A | Yes |
| DUT power control | No | No | Yes, subject to fault rules | Yes | Yes |
| DUT sync control | No | No | Yes, subject to fault rules | Yes | Yes |
| Reset command | Implementation-defined | Implementation-defined | Implementation-defined | Implementation-defined | Implementation-defined |

## 10.3 Notes on Command Legality
- Beam On does not alter the matrix above.
- A command may still be rejected within an otherwise legal state if its preconditions fail.
- HLF clear is legal only for clearable HLFs whose clear preconditions are satisfied.

## 11. HLF Clear Preconditions
A clear request for an HLF shall succeed only if:
1. the targeted fault is a clearable HLF
2. the underlying condition is no longer active
3. any required recovery dwell has elapsed, if defined by variable
4. no active LLF is present
5. command syntax and target selection are valid

If any of these conditions fail, the HC shall reject the clear request and preserve the latched HLF.

## 12. Reporting Requirements by State
| State | Required Reporting Behavior |
|---|---|
| BOOT | expose boot progress/failure details where feasible |
| LOW_LEVEL_FAULT | expose latched LLF and safe-off condition where feasible |
| HIGH_LEVEL_FAULT | expose active/latched HLFs, affected DUT, and recovery status |
| NORMAL | periodic and query-driven status reporting |
| SLAVE | periodic and query-driven status reporting with mode indicated |

At minimum, status/reporting should make visible:
- top-level state
- active mode
- active LLFs/HLFs/warnings
- DUT1 and DUT2 enable state
- TE-link-active indication
- Beam status indication if available
- relevant measurement summaries and variable-governed fault context where supported

## 13. LED Indication Requirements
The exact LED patterns remain implementation-specific unless finalized elsewhere, but state ownership rules are:
- LED indication shall represent the highest-priority active condition
- `LOW_LEVEL_FAULT` indication overrides all others
- `HIGH_LEVEL_FAULT` indication overrides `NORMAL` and `SLAVE`
- warning/degraded indication shall not obscure LLF/HLF indication

## 14. Internal Ownership Model
The state machine shall be owned by the `sys_state` module.

Supporting modules provide inputs but do not directly own top-level state:
- `fault_manager` requests LLF/HLF transitions
- measurement modules provide evidence
- command parser validates requests and submits transition intents
- control modules execute output changes required by the active state
- reporting modules publish state/fault information

## 15. Implementation Guidance
Recommended implementation pattern:
- store top-level state in one authoritative state variable
- process transition requests in a single non-ISR context
- allow ISRs to capture raw events only
- centralize transition side effects in explicit entry handlers
- keep fault latching separate from state variable storage, but coordinated through `fault_manager`

## 16. Verification Guidance
The following shall be verified against this specification:
- boot success and boot-failure transitions
- LLF priority over all other transitions
- HLF entry/clear behavior
- DUT isolation scope during HLF entry
- legal and illegal commands by state
- persistence of latched HLFs until explicit clear
- impossibility of clearing LLFs by command
- Beam On non-restriction behavior
- warning coexistence without improper state escalation

## 17. Open Items
The following still need future refinement:
- whether `HIGH_LEVEL_FAULT -> SLAVE` should remain fully supported in all cases
- exact command names and payload schemas used to drive transitions
- whether any HLF subclasses should bypass top-level `HIGH_LEVEL_FAULT` and remain sub-state only
- final LED pattern mapping
- reset-command semantics if software reset is exposed over TE/debug

## 18. Revision Notes
- v1: Initial formal top-level HC state machine specification created and aligned to the HC PRD, fault matrix, variable registry, firmware architecture, TE interface spec, and protocol test plan.
