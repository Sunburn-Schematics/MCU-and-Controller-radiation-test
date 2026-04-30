# Radiation Test Host Controller (HC) Firmware Architecture v1

## 1. Document Purpose
This document defines the proposed firmware architecture for the Radiation Test Host Controller (HC), based on the current Product Requirements Document, Fault Response Matrix, and Variable Registry.

It describes how the HC firmware should be structured so that:
- safety-relevant outputs default to known safe states
- DUT monitoring is deterministic and traceable
- faults are detected, latched, and reported consistently
- command/control behavior is separated cleanly from measurement and protection logic
- deferred numeric parameters are referenced through named variables

This is an initial BMAD architecture draft and should be refined as implementation details and experiments progress.

## 2. Architectural Goals
The firmware architecture shall:
- enforce safe startup and safe fault response behavior
- isolate hardware drivers from application logic
- make fault handling centralized and deterministic
- support both TE control and debug interaction without duplicating core logic
- support DUT1 and DUT2 monitoring independently, with affected-DUT-only shutdown by default
- make timing-sensitive measurement paths explicit
- allow later substitution of experimentally derived numeric values through named variables
- preserve traceability from requirements to implementation modules

## 3. Architectural Principles
- **Safe by default:** all controllable outputs are forced to safe OFF states before normal operation.
- **Single source of truth for system state:** the HC operational state machine is authoritative.
- **Single source of truth for fault state:** all LLF/HLF/WRN conditions are owned by the fault manager.
- **Separation of concerns:** peripheral access, measurement processing, state logic, and communications are separated into distinct modules.
- **Deterministic scheduling:** periodic processing shall run on a defined scheduler/tick model.
- **Interrupts for capture, not policy:** interrupts acquire time-sensitive events; application-level decisions occur outside interrupt context.
- **Variable-driven thresholds:** all unresolved numeric thresholds/timings are represented by named variables from the variable registry.

## 4. Top-Level Firmware Structure
The firmware is organized into the following major layers:

| Layer | Purpose |
|---|---|
| Platform Layer | MCU startup, clocks, GPIO, timers, ADC, UART, USB, watchdog, low-level drivers |
| Service Layer | timebase, scheduler, logging/event buffering, variable access, command parser support |
| Measurement Layer | DUT1 and DUT2 signal acquisition, scaling, validation, derived measurements |
| Control Layer | power-enable control, sync generation control, LED control |
| Fault Management Layer | fault detection, latching, prioritization, clear logic, fault records |
| Application Layer | state machine, TE behavior, debug behavior, reporting, mode transitions |

## 5. Proposed Module Breakdown

| Module | Responsibility |
|---|---|
| `sys_boot` | startup sequence, safe defaults, boot self-check orchestration |
| `sys_state` | authoritative operational state machine |
| `sys_time` | Time Since Boot (TSB), scheduler tick, timestamps |
| `drv_gpio` | GPIO reads/writes, ID switch reads, power-enable outputs, Beam On input |
| `drv_timer_capture` | input-capture setup and raw edge timing capture |
| `drv_timer_sync` | DUT1 SyncA/SyncB generation |
| `drv_adc` | ADC acquisition and channel management |
| `drv_uart_dbg` | debug UART transport |
| `drv_usb_vcp` | TE USB Virtual COM Port transport |
| `svc_variables` | named variable access and mapping to implementation constants/config |
| `svc_events` | event and fault record buffering |
| `svc_scheduler` | periodic task dispatch |
| `meas_dut1` | DUT1 current, ME/MF timing, filtered analog monitor processing |
| `meas_dut2` | DUT2 output voltage and gate timing processing |
| `ctrl_power` | DUT1/DUT2 power enable control with safe-state enforcement |
| `ctrl_led` | LED status and fault code display |
| `fault_manager` | fault classification, latching, clear preconditions, shutdown actions |
| `comms_te` | TE protocol parsing, command dispatch, periodic status reports |
| `comms_debug` | debug command parsing and responses |
| `reporting` | status packet composition and event/fault summaries |

## 6. Execution Model
### 6.1 Recommended Execution Style
A **bare-metal cooperative scheduler with interrupt-driven capture/acquisition support** is recommended.

Rationale:
- the current requirement set does not yet justify RTOS complexity
- timing-critical work is limited and can be isolated in ISR-driven capture paths
- state-machine and fault-policy logic are easier to keep deterministic in a cooperative model

### 6.2 High-Level Execution Pattern
- hardware interrupts capture raw timing or data-ready events
- ISR code stores minimal raw data into bounded buffers or snapshots
- the scheduler runs periodic service functions at defined rates
- application logic consumes processed measurements and updates states/faults
- communications and reporting run in non-interrupt context

### 6.3 Suggested Periodic Tasks
All periods below are symbolic and should be implemented using named variables or architecture constants where appropriate.

| Task | Purpose | Suggested Trigger |
|---|---|---|
| fast measurement service | process capture deltas, update edge-derived stats | periodic tick |
| ADC service | convert raw ADC data to engineering values | periodic tick / DMA completion |
| fault evaluation | evaluate all detection rules and update latches | periodic tick |
| state machine update | evaluate transitions and mode actions | periodic tick |
| TE communications service | command parsing and packet transmission | periodic tick / USB events |
| debug service | parse debug RX and generate responses | periodic tick / UART RX events |
| LED service | maintain visual status/fault indications | periodic tick |
| status reporting service | generate periodic TE report | variable-driven interval |

## 7. Operational State Machine Ownership
The `sys_state` module owns the HC operational state.

### States
- Boot
- Low-Level Fault
- High-Level Fault
- Normal
- Slave

### State Responsibilities
| State | Primary Behavior |
|---|---|
| Boot | initialize platform, enforce safe outputs, run mandatory checks |
| Low-Level Fault | platform not trusted for normal operation; safe outputs enforced |
| High-Level Fault | DUT/runtime fault present; affected DUT isolated by default |
| Normal | standard monitoring and TE-controlled operation |
| Slave | Normal-like operation with explicit override semantics |

### State Transition Policy
- only `sys_state` may change the operational state
- `fault_manager` may request transitions based on LLF/HLF conditions
- `comms_te` and `comms_debug` may request mode changes, but only through `sys_state`
- direct driver modules must never change the HC state themselves

## 8. Fault Management Architecture
The `fault_manager` module is the authoritative owner of all fault state.

### Responsibilities
- maintain records for LLF, HLF, and WRN conditions
- apply latching rules
- enforce clear preconditions
- map faults to shutdown actions
- request state transitions
- provide fault summaries to TE/debug/reporting modules

### Fault Processing Pipeline
1. measurement/control modules publish observations
2. fault rules evaluate observations against variable-defined criteria
3. `fault_manager` records assertion/clear candidates
4. latching and clear policy are applied
5. output shutdown actions are executed through `ctrl_power`
6. state transition requests are issued to `sys_state`
7. event/report records are emitted through `svc_events` and `reporting`

### Fault Record Content
Each fault record should contain:
- fault ID
- class (LLF/HLF/WRN)
- active flag
- latched flag
- first occurrence TSB
- latest occurrence TSB
- occurrence count
- related measurements / evidence if available
- source module

## 9. Measurement Architecture
### 9.1 DUT1 Measurement Path
The `meas_dut1` module should provide:
- shunt-based input current calculation
- ME gate activity and timing measurements
- MF gate activity and timing measurements
- filtered ME analog monitor evaluation
- filtered MF analog monitor evaluation
- expected-active determination inputs for fault logic

### 9.2 DUT2 Measurement Path
The `meas_dut2` module should provide:
- output-voltage measurement and scaling
- gate activity and timing measurements
- expected-active determination inputs for fault logic

### 9.3 Measurement Ownership Rules
- driver modules provide raw captures/samples only
- `meas_dut1` and `meas_dut2` convert raw data into engineering quantities
- threshold comparisons should happen in or through the fault layer, not hidden inside drivers
- measurement modules may publish validity/quality flags in addition to values

## 10. Interrupt vs Non-Interrupt Responsibilities
### Interrupt Context
Allowed ISR responsibilities:
- capture timestamps or edge periods
- store raw values into ring buffers or latest-sample structures
- service ADC DMA completion or sample-ready flags
- service UART/USB low-level transport events only as needed

ISR code shall not:
- make state-transition decisions
- latch/clear faults
- enable/disable DUT power directly unless required for immediate hardware safety
- compose TE reports
- parse high-level commands

### Non-Interrupt Context
Non-ISR code shall:
- process raw captures into engineering metrics
- evaluate thresholds and debounce logic
- apply fault policy
- manage state transitions
- compose communications payloads

## 11. Control Architecture
### 11.1 Power Control
The `ctrl_power` module should expose a small set of controlled operations:
- set DUT1 power enabled/disabled
- set DUT2 HV enabled/disabled
- force all controllable outputs safe OFF
- report commanded power state

Policy decisions such as when power should be disabled belong to `fault_manager` or `sys_state`, not `ctrl_power`.

### 11.2 DUT1 Sync Generation
The `drv_timer_sync` module should:
- configure and start SyncA/SyncB generation
- stop sync generation safely
- report operational status to supervising logic

Supervision of whether sync generation is valid or failed should be visible to `meas_dut1` and `fault_manager`.

### 11.3 LED Control
The `ctrl_led` module should abstract LED patterns from system policy.

Higher-level modules should request symbolic indications such as:
- boot pattern
- normal pattern
- degraded pattern
- HLF pattern
- LLF flash code pattern

## 12. Communications Architecture
### 12.1 TE Communications
The `comms_te` module shall use USB VCP as the primary communication interface.

Responsibilities:
- receive TE commands
- validate and parse command frames
- dispatch command intents to state/control/fault services
- return command responses
- coordinate with `reporting` for periodic status messages

### 12.2 Debug Communications
The `comms_debug` module shall provide development/debug access over UART.

Current policy impact:
- debug may clear faults
- Beam On imposes no restrictions on commands or modes
- a debug command/response only indicates whether the USB VCP connection is active or not, where required by the current PRD

### 12.3 Authority Model
Recommended command authority handling:
- TE and debug both route through the same core action handlers
- policy checks occur centrally, not separately in each interface module
- interface-specific parsing differs, but operational semantics remain shared

## 13. Reporting Architecture
The `reporting` module should build status/event payloads from authoritative sources:
- `sys_state`
- `fault_manager`
- `meas_dut1`
- `meas_dut2`
- `drv_gpio` for ID and Beam On status
- transport status from `drv_usb_vcp`

### Recommended Status Report Content
- HC ID
- current operational state
- Beam On status
- DUT1 enabled state
- DUT2 enabled state
- key DUT1 measurements
- key DUT2 measurements
- active faults/warnings summary
- TE communication link status
- timestamp / TSB

## 14. Variable Integration Strategy
The `svc_variables` module should provide a stable interface between requirement-level variable names and implementation-level storage.

### Recommended Approaches
Initial implementation may store variable values as:
- compile-time constants
- generated configuration headers
- structured constant tables

Later evolution may allow:
- calibration tables
- nonvolatile parameter storage
- debug-readable parameter inspection

### Architectural Rule
Requirement documents shall reference named variables from:
- `/a0/usr/projects/bmad_test/docs/hc_variable_registry_v1.md`

Implementation should preserve traceability to those names where practical.

## 15. Boot Architecture
The `sys_boot` module should execute the boot sequence in a strict order.

### Recommended Boot Sequence
1. initialize clocks and essential platform support
2. force all controllable outputs to safe OFF
3. initialize `sys_time`
4. initialize debug UART
5. emit `BOOTING...` if debug UART initialization succeeds
6. initialize timers/input capture/sync resources
7. initialize ADC resources
8. initialize USB VCP resources
9. initialize service and application modules
10. read and validate HC ID
11. perform mandatory self-checks
12. request transition to Normal if all mandatory checks pass

### Boot Failure Policy
- catastrophic initialization failures, including debug UART and USB/VCP initialization failure under current policy interpretation, render nuanced runtime management moot
- safe outputs shall remain enforced
- if any reporting path survives, minimal failure indication may be attempted

## 16. Data Flow Overview
### DUT1 Example
1. edge captured by `drv_timer_capture`
2. raw timing stored
3. `meas_dut1` computes period/duty/activity status
4. `fault_manager` evaluates range/timeouts using variable-defined criteria
5. `ctrl_power` disables DUT1 if required
6. `reporting` includes fault/measurement in TE status

### DUT2 Example
1. ADC sample acquired by `drv_adc`
2. `meas_dut2` computes scaled output voltage
3. `fault_manager` evaluates against `VAR_DUT2_VOUT_LOW_LIMIT` / `VAR_DUT2_VOUT_HIGH_LIMIT`
4. `ctrl_power` disables HV if required
5. `reporting` records and transmits the event/status

## 17. Verification Hooks and Testability
The architecture should support verification through:
- injectable measurement samples for unit tests
- deterministic state-transition testing
- fault-rule evaluation independent of hardware drivers
- transport-independent command handling tests
- explicit traceability from requirement to module and test

### Recommended Test Seams
- mockable variable source
- mockable measurement source
- mockable power-control backend
- testable fault-manager rule evaluation API
- testable state-machine transition API
- testable reporting packet builder API

## 18. Key Open Architectural Decisions
The following are still open and should be refined in future revisions:
- exact TE command/response protocol framing
- exact periodic report format and interval variable mapping
- whether any runtime peripheral-acquisition failure should escalate from HLF to LLF
- exact LED flash-code table
- exact build-time versus runtime configurability policy for variables
- whether SyncA and SyncB frequencies collapse into a single frequency variable

## 19. Recommended Next BMAD Artifacts
This architecture enables the next level of decomposition:
1. TE Interface / Command Specification
2. Formal State Machine Specification
3. Story Backlog for implementation
4. Verification and traceability matrix

## 20. Revision Notes
- v1: Initial HC firmware architecture draft based on PRD v1, Fault Response Matrix v1.4, and Variable Registry v1.
