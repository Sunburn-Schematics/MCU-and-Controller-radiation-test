# Host Controller DUT Monitor Functions Specification v1

## 1. Document Purpose
This document defines the intended high-level application behavior for the two DUT supervisory functions:
- `Monitor_LTC3901()`
- `Monitor_LT8316()`

These functions are intended to live in the HC application layer and provide the controller-owned logic required to:
- safely turn on each DUT
- monitor each DUT during operation
- detect and contain DUT-local fault conditions
- preserve unaffected-DUT operation by default
- publish supervisory state for reporting, debugging, and later verification

This document is requirements-oriented and implementation-guiding. It is intended to bridge the existing HC PRD, fault matrix, architecture, TE interface, and state machine artifacts to the current firmware application control flow in `App/fw_app.c`.

## 2. Scope
### 2.1 In Scope
This document defines:
- the role of `Monitor_LTC3901()` and `Monitor_LT8316()` in the main application loop
- the supervisory responsibilities owned by each function
- startup, monitoring, and containment behavior for each DUT
- interaction with top-level HC state, DUT-local state, fault handling, and reporting
- assumptions and open items that must remain visible while the implementation evolves

### 2.2 Out of Scope
This document does not yet define:
- final numeric thresholds, debounce windows, or qualification intervals
- low-level ADC, timer, or USB driver implementation details
- final TE command grammar
- final debug override policy
- complete unit/integration test procedures

## 3. Source Context
This specification is derived from and shall remain aligned with:
- `hc_prd_v1.md`
- `hc_fault_response_matrix_v1.md`
- `hc_architecture_v1.md`
- `hc_state_machine_spec_v1.md`
- `hc_te_interface_spec_v1.md`
- `hc_variable_registry_v1.md`
- current application control flow in `App/fw_app.c`

## 4. Problem Statement
The current `fw_app.c` implementation initializes low-level services and periodically services measurements, comms, and reporting, but it does not yet contain a clear application-owned supervisory layer for DUT1 and DUT2.

In particular:
- `fw_app_init()` currently enables the LTC3901 sync output unconditionally during initialization
- `fw_app_run()` currently behaves primarily as a service pump
- no dedicated high-level DUT monitor functions currently own DUT startup qualification, runtime monitoring, or fault containment behavior

This document defines the requirements for introducing that missing supervisory layer.

## 5. Architectural Role of the Monitor Functions
The DUT monitor functions shall belong to the HC application layer.

They shall sit above:
- BSP/platform control functions
- ADC acquisition drivers
- PWM/input-capture drivers
- debug and command transport services

They shall sit below or alongside:
- top-level HC application control flow in `fw_app_run()`
- status/reporting composition
- future centralized fault/state ownership if later refactored into dedicated modules

### 5.1 Ownership Intent
`Monitor_LTC3901()` and `Monitor_LT8316()` shall be the primary owners of DUT-local supervisory behavior for their respective devices.

They shall not replace:
- MCU peripheral drivers
- raw measurement acquisition drivers
- transport protocol parsing

They shall consume those lower-level services and produce supervisory outcomes such as:
- DUT enable/disable decisions
- DUT-local health/fault state
- containment actions
- status fields suitable for reporting

## 6. Integration with `fw_app_run()`
The monitor functions shall be executed from the main application loop after measurement/task services have had an opportunity to refresh data and after command-processing logic has accepted any new supervisory requests.

### 6.1 Intended Main Loop Position
The intended application-level loop structure is:
1. service low-level acquisition and transport tasks
2. accept or update command-driven supervisory intent
3. execute `Monitor_LTC3901()`
4. execute `Monitor_LT8316()`
5. refresh and emit periodic HC status/reporting data

### 6.2 Rationale
This ordering ensures that:
- each monitor function evaluates recent measurements
- externally requested enable/disable intents are visible before monitoring decisions are applied
- periodic status output reflects the latest supervisory result rather than stale pre-monitoring data

## 7. Shared Supervisory Principles
Both monitor functions shall follow these shared principles.

### 7.1 Safe by Default
A DUT shall not be enabled merely because initialization has completed.

Enablement shall occur only when:
- the top-level HC state allows the DUT to operate
- no higher-priority platform fault prevents trusted supervision
- DUT-local preconditions for safe enable are satisfied

### 7.2 Supervisory Ownership
DUT-local power and activity behavior shall be controlled by supervisory logic rather than by unconditional initialization side effects.

### 7.3 Affected-DUT-Only Containment
When a DUT-local runtime fault is detected, the affected DUT shall be isolated by default while the unaffected DUT remains operational unless a broader shutdown rule is explicitly defined elsewhere.

### 7.4 Deterministic Non-ISR Execution
The monitor functions shall run in non-interrupt context.
They may consume measurement snapshots prepared by interrupt-driven acquisition, but they shall not rely on making supervisory decisions inside ISR context.

### 7.5 Traceable Fault and Status Behavior
Supervisory outcomes shall be representable in HC reporting and, where practical, traceable to existing fault IDs and requirement artifacts.

## 8. Top-Level HC State Interaction
The monitor functions shall obey the HC top-level state machine.

### 8.1 In `BOOT`
- DUT power shall remain OFF
- DUT sync outputs shall remain OFF
- no monitor function shall force DUT startup
- monitor functions may initialize or reset DUT-local supervisory state if needed

### 8.2 In `LOW_LEVEL_FAULT`
- both DUTs shall remain safely disabled
- no DUT recovery or enable attempt shall be made
- monitor functions shall preserve or expose a safely contained DUT state

### 8.3 In `HIGH_LEVEL_FAULT`
- monitor functions shall preserve containment for DUT-local faults
- only valid clear/re-enable policy shall permit return toward normal operation
- unaffected DUT operation may continue if permitted by policy

### 8.4 In `NORMAL`
- monitor functions shall perform normal supervisory enable, monitoring, and containment behavior

### 8.5 In `SLAVE`
- monitor functions shall continue monitoring and protection
- supervisory safety rules shall continue to override unsafe requests

## 9. DUT-Local Supervisory State Model
Each monitor function should maintain or drive a DUT-local supervisory state model.

### 9.1 Recommended DUT-Local States
For both DUTs, the recommended DUT-local state set is:
- `OFF`
- `STARTING`
- `RUNNING`
- `FAULT`
- `RECOVERED`
- `ISOLATED`

### 9.2 State Meanings
- `OFF`: DUT is intentionally not enabled
- `STARTING`: DUT has been enabled and is within startup/settle/qualification time
- `RUNNING`: DUT is enabled and presently considered healthy
- `FAULT`: a DUT-local fault has been detected and immediate containment/recovery logic is active
- `RECOVERED`: a prior DUT-local fault has cleared and the DUT is operating again within expected limits
- `ISOLATED`: the DUT has been disabled or held disabled to protect hardware integrity or test validity

### 9.3 Relationship to Top-Level HC State
The DUT-local state model shall not replace the top-level HC state.
It is subordinate to it.

If a top-level HC state forbids DUT operation, the DUT-local monitor shall honor that regardless of local DUT state.

## 10. `Monitor_LTC3901()` Requirements

### 10.1 Purpose
`Monitor_LTC3901()` shall supervise the LTC3901 DUT path, including:
- DUT1 power enable behavior
- DUT1 sync enable behavior
- DUT1 current monitoring
- DUT1 ME/MF activity and timing supervision
- DUT1 fault containment behavior

### 10.2 Inputs
`Monitor_LTC3901()` should be able to consume, directly or indirectly:
- top-level HC state / mode
- commanded LTC3901 enable intent
- commanded sync enable intent or supervisory policy equivalent
- current LTC3901 power-enable state
- current sync-enable state
- DUT1 supply/shunt/current measurements
- DUT1 ME frequency/activity data
- DUT1 MF frequency/activity data
- any available filtered analog ME/MF monitor values
- current DUT1 fault/containment status
- current timebase / tick information for debounce and qualification timing

### 10.3 Outputs / Effects
`Monitor_LTC3901()` shall be able to:
- enable or disable LTC3901 power
- enable or disable LTC3901 sync generation
- update DUT1-local supervisory state
- assert or request DUT1 fault status
- contain the DUT1 path without unnecessarily disturbing DUT2
- update status fields used by HC reporting

### 10.4 Startup / Enable Policy
`Monitor_LTC3901()` shall:
- ensure LTC3901 power is not enabled unless top-level HC state permits operation
- ensure sync is not left enabled merely because initialization occurred earlier
- coordinate sync enablement with DUT1 operating intent and supervisory safety policy
- support a DUT-local startup qualification phase before considering DUT1 healthy

### 10.5 Runtime Monitoring Policy
While DUT1 is expected to operate, `Monitor_LTC3901()` shall supervise:
- input current behavior
- ME activity presence and timeout behavior
- MF activity presence and timeout behavior
- ME timing range behavior if available
- MF timing range behavior if available
- filtered analog monitor plausibility if available

### 10.6 Fault Detection Alignment
The function shall be designed to align with existing DUT1 fault definitions including, where implemented:
- `HLF-001` DUT1 input overcurrent
- `HLF-003` DUT1 ME gate activity missing
- `HLF-004` DUT1 MF gate activity missing
- `HLF-005` DUT1 ME gate out of range
- `HLF-006` DUT1 MF gate out of range
- `HLF-007` DUT1 filtered ME analog out of range
- `HLF-008` DUT1 filtered MF analog out of range
- `HLF-009` DUT1 sync generation failure
- `HLF-010` DUT1 unexpected activity while power disabled

### 10.7 Fault Containment Policy
On a fault condition requiring containment, `Monitor_LTC3901()` shall:
- disable LTC3901 power when required
- disable sync when required
- place DUT1 into `FAULT` or `ISOLATED` according to policy and recovery outcome
- preserve DUT2 operation by default unless another policy explicitly requires broader action

### 10.8 Recovery Intent
The function should be structured so that future DUT1 recovery logic can be implemented through:
- controlled power disable
- restart delay
- re-enable attempt
- qualification window
- transition to `RECOVERED` or `ISOLATED`

This document does not require automatic retry to be fully implemented immediately, but the control-flow structure shall not prevent it.

## 11. `Monitor_LT8316()` Requirements

### 11.1 Purpose
`Monitor_LT8316()` shall supervise the LT8316 DUT path, including:
- DUT2 power/HV enable behavior
- DUT2 output-voltage monitoring
- DUT2 gate-activity supervision where available
- DUT2 fault containment behavior

### 11.2 Inputs
`Monitor_LT8316()` should be able to consume, directly or indirectly:
- top-level HC state / mode
- commanded LT8316 enable intent
- current LT8316 power-enable state
- DUT2 output-voltage measurement(s)
- DUT2 gate activity / timing data if available
- current DUT2 fault/containment status
- current timebase / tick information for debounce and qualification timing

### 11.3 Outputs / Effects
`Monitor_LT8316()` shall be able to:
- enable or disable LT8316 power/HV path
- update DUT2-local supervisory state
- assert or request DUT2 fault status
- contain the DUT2 path without unnecessarily disturbing DUT1
- update status fields used by HC reporting

### 11.4 Startup / Enable Policy
`Monitor_LT8316()` shall:
- ensure LT8316 power is not enabled unless top-level HC state permits operation
- support a DUT-local startup qualification phase before considering DUT2 healthy
- avoid repeated uncontrolled enable toggling when the DUT is faulted or isolated

### 11.5 Runtime Monitoring Policy
While DUT2 is expected to operate, `Monitor_LT8316()` shall supervise:
- output-voltage behavior
- gate activity presence if measured
- gate timing range if measured
- any additional plausibility checks exposed by the measurement path

### 11.6 Fault Detection Alignment
The function shall be designed to align with existing DUT2 fault definitions including, where implemented:
- `HLF-011` DUT2 output voltage out of range high
- `HLF-012` DUT2 output voltage out of range low
- `HLF-013` DUT2 gate activity missing
- `HLF-014` DUT2 gate out of range
- `HLF-015` HV enable feedback mismatch if such feedback exists

### 11.7 Fault Containment Policy
On a fault condition requiring containment, `Monitor_LT8316()` shall:
- disable LT8316 power/HV path when required
- place DUT2 into `FAULT` or `ISOLATED` according to policy and recovery outcome
- preserve DUT1 operation by default unless another policy explicitly requires broader action

### 11.8 Recovery Intent
The function should be structured so that future DUT2 recovery logic can be implemented through:
- controlled power disable
- restart delay
- re-enable attempt
- qualification window
- transition to `RECOVERED` or `ISOLATED`

## 12. Interaction with Reporting
Both monitor functions shall make their supervisory outputs available to HC reporting.

At minimum, reporting should be able to reflect:
- DUT-local state
- DUT power-enable state
- sync-enable state for LTC3901
- key summary measurements
- active DUT-local fault count / IDs / summary where supported

The periodic `STS` payload shall eventually reflect monitor-owned supervisory outcomes rather than only raw BSP state.

## 13. Interaction with Commands and Debug
### 13.1 Command Relationship
External commands should be interpreted as supervisory intent, not as unconditional authority to bypass safety logic.

### 13.2 Autostart Relationship
The application may support a compile-time autostart policy. When enabled, autostart shall issue supervisory `RUN` intent to the DUT managers after a configured startup delay rather than directly asserting hardware outputs.

Autostart shall be deterministic and one-shot per boot. It shall not override an already pending external manager command, and it shall only target a manager that remains in its `RESET` state when the autostart delay expires.

### 13.3 Debug Relationship
The current debug path can directly assert digital outputs such as:
- `ltc3901.pwr_en`
- `lt8316.pwr_en`
- `sync.enable`

This creates a policy question that remains open:
- whether these remain raw overrides, or
- whether they become requests mediated by DUT supervisory logic

Until finalized, any implementation shall document how debug actions interact with the monitor functions.

## 14. Timing and Variable Dependency
The monitor functions shall not hard-code unresolved timing and threshold values in their requirements definition.

They should be parameterized conceptually by named HC variables such as:
- DUT current thresholds
- DUT timing range limits
- activity timeouts
- debounce windows
- restart delay values
- recovery qualification intervals

Where new variables are needed, they should be added to the HC variable registry in a future revision.

## 15. Open Items
The following remain open and should be resolved in follow-on revisions:
- exact DUT-local state representation and naming in code
- exact startup qualification criteria for each DUT
- exact automatic recovery policy versus explicit supervisory/manual recovery
- exact interaction between debug overrides and monitor ownership
- exact mapping from DUT-local faults to top-level `HIGH_LEVEL_FAULT`
- whether current `fw_app_init()` sync auto-enable is removed immediately or temporarily retained behind supervision
- whether monitor functions directly latch/report faults or route through a later centralized fault manager abstraction

## 16. Recommended Near-Term Implementation Guidance
The first implementation step should be to introduce the two monitor functions into `fw_app.c` as explicit high-level application calls and move DUT-specific enable/containment policy out of unconditional initialization behavior.

A recommended first implementation approach is:
1. define monitor-owned DUT-local state containers
2. move sync enable control under `Monitor_LTC3901()` ownership
3. keep measurement acquisition in existing drivers/tasks
4. consume measurement summaries inside each monitor function
5. drive DUT enable/disable and local status updates from the monitor functions
6. expose resulting supervisory state through existing HC status/reporting paths

## 17. Traceability Intent
This specification is primarily intended to support future traceability for:
- DUT startup supervision
- DUT runtime monitoring
- DUT fault containment
- unaffected-DUT preservation
- reporting of DUT-local supervisory state

These traceability links should be added to the verification and backlog artifacts in a later documentation pass.

## 18. Revision Notes
- v1: Initial DUT monitor-function specification created to define the high-level application requirements for `Monitor_LTC3901()` and `Monitor_LT8316()` based on current HC requirements and the present `fw_app.c` application flow.
