# Radiation Test Host Controller (HC) Product Specification — Amalgamated v1

## Purpose
This document consolidates the current HC product specification set into a single offline review artifact.

## Included source documents
1. PRD v1
2. Fault Response Matrix v1
3. Variable Registry v1
4. Firmware Architecture v1
5. TE Interface Spec v1
6. Protocol Test Plan v1
7. State Machine Spec v1
8. Verification & Traceability Matrix v1
9. Story Backlog v1

---

## Table of Contents
- [1. Product Requirements Document (PRD) v1](#1-product-requirements-document-prd-v1)
- [2. Fault Response Matrix v1](#2-fault-response-matrix-v1)
- [3. Variable Registry v1](#3-variable-registry-v1)
- [4. Firmware Architecture v1](#4-firmware-architecture-v1)
- [5. TE Interface Specification v1](#5-te-interface-specification-v1)
- [6. Protocol Test Plan v1](#6-protocol-test-plan-v1)
- [7. State Machine Specification v1](#7-state-machine-specification-v1)
- [8. Verification & Traceability Matrix v1](#8-verification--traceability-matrix-v1)
- [9. Story Backlog v1](#9-story-backlog-v1)

---


## 1. Product Requirements Document (PRD) v1

# Radiation Test Host Controller (HC) Product Requirements Document v1

## 1. Document Purpose
This document defines the functional requirements for the Radiation Test Host Controller (HC). The HC monitors devices under test (DUTs) while they are subjected to Heavy Ion / Proton beam bombardment, detects faults, controls selected power rails and stimulus signals, and periodically reports status to the Test Executive (TE).

This version is a BMAD-style PRD v1 draft derived from an incomplete source description. Open items are explicitly marked as **TBD**.

## 2. Product Goal
The HC shall provide deterministic monitoring and control of the attached DUTs during radiation test activity and shall communicate sufficient status and fault information to the TE so that the broader test system can manage execution and data collection.

## 3. Scope
### 3.1 In Scope
- Monitoring DUT1 and DUT2 electrical behavior
- Generating DUT1 quadrature stimulus outputs
- Controlling DUT1 and DUT2 power-enable signals
- Detecting and classifying low-level and high-level faults
- Communicating status and command responses with the TE over USB serial
- Providing a local debug UART during development
- Providing visual status and fault indications using red and green LEDs
- Reading a 6-bit hardware ID from GPIO inputs

### 3.2 Out of Scope
- Control of the radiation beam itself
- Control of external power supply output setpoints unless explicitly added later
- Long-term archival data storage on the HC
- Calibration workstation functionality
- TE-side software behavior except where required by interface definition

## 4. Stakeholders and External Actors
- **Test Executive (TE):** Primary supervisory controller for the test system
- **Test Operator / Developer:** Uses debug UART and LED indications for setup and diagnostics
- **Heavy Ion / Proton Beam System:** Provides a Beam On logic signal to the HC
- **External Power Supplies:** Provide DUT operating voltages under HC enable control

## 5. System Context
The HC is one component of a larger radiation test system comprised of:
1. Test Executive (TE)
2. Heavy Ion / Proton Radiation Beam
3. High Voltage Power Supply
4. Low Voltage Power Supply
5. LTC3901 DC/DC Power Controller IC (DUT1)
6. LT8316 DC/DC Power Controller IC (DUT2)
7. Optional serial debug terminal
8. Two status LEDs

## 6. Assumptions and Constraints
- The HC firmware executes on an MCU platform with GPIO, timers, ADC, UART, and USB peripherals.
- The HC shall default all controllable outputs to safe OFF states at power-up and reset.
- The TE-HC connection is over USB and carries serial-style command/response and status traffic.
- A debug UART is available on PA9 (TX) and PA10 (RX) at 115200 baud.
- DUT1 requires a 12V input path controlled by `nLTC3901_Pwr_Enable` (active LOW).
- DUT2 requires a high-voltage input path controlled by `HV_Pwr_Enable` (active HIGH).
- Unresolved numeric thresholds, timing tolerances, and debounce/persistence values shall be represented by individually named variables whose numeric values will be assigned later from experimental results.

## 7. External Interfaces

### 7.1 HC <-> DUT1 (LTC3901)
The HC shall provide a quadrature-encoded SyncA and SyncB stimulus signal to DUT1.

The HC shall directly measure the DUT1 ME and MF gate driver outputs using timer input capture or equivalent timing measurement hardware.

The HC shall monitor low-pass filtered versions of the DUT1 ME and MF gate driver outputs on separate ADC channels.

The HC shall monitor the input current feeding DUT1 by measuring the voltage on both sides of a shunt resistor located in the DUT1 input power path.

#### DUT1 Open Items
- SyncA/SyncB frequency range: variable(s) `VAR_DUT1_SYNCA_FREQ`, `VAR_DUT1_SYNCB_FREQ` or equivalent naming TBD
- Required phase relationship and tolerance: variable(s) `VAR_DUT1_SYNC_PHASE_TARGET`, `VAR_DUT1_SYNC_PHASE_TOLERANCE`
- Valid ME/MF frequency and duty cycle ranges: variables `VAR_DUT1_ME_FREQ_MIN`, `VAR_DUT1_ME_FREQ_MAX`, `VAR_DUT1_ME_DUTY_MIN`, `VAR_DUT1_ME_DUTY_MAX`, `VAR_DUT1_MF_FREQ_MIN`, `VAR_DUT1_MF_FREQ_MAX`, `VAR_DUT1_MF_DUTY_MIN`, `VAR_DUT1_MF_DUTY_MAX`
- ADC sample/update rate: variable(s) `VAR_DUT1_ADC_SAMPLE_RATE`, `VAR_DUT1_ADC_UPDATE_RATE`
- Current threshold(s) and fault persistence time: variables `VAR_DUT1_OVERCURRENT_LIMIT`, `VAR_DUT1_OVERCURRENT_DEBOUNCE`
### 7.2 HC <-> DUT2 (LT8316)
When powered, DUT2 requires no further run stimulus from the HC.

The HC shall monitor the DUT2 output voltage through an ADC input.

The HC shall monitor the DUT2 gate driver output via direct timing measurement using timer input capture or equivalent.

#### DUT2 Open Items
- Valid output voltage range: variables `VAR_DUT2_VOUT_LOW_LIMIT`, `VAR_DUT2_VOUT_HIGH_LIMIT`
- Valid gate activity range / timeout: variables `VAR_DUT2_GATE_FREQ_MIN`, `VAR_DUT2_GATE_FREQ_MAX`, `VAR_DUT2_GATE_DUTY_MIN`, `VAR_DUT2_GATE_DUTY_MAX`, `VAR_DUT2_GATE_ACTIVITY_TIMEOUT`
- ADC sample/update rate: variable(s) `VAR_DUT2_ADC_SAMPLE_RATE`, `VAR_DUT2_ADC_UPDATE_RATE`
- Fault thresholds and persistence times: variable set to be defined per fault condition

### 7.3 HC <-> Heavy Ion / Proton Beam
The radiation beam system, under TE control, shall assert a `Beam On` logic signal to the HC.

The HC shall monitor the Beam On signal as a digital input.

#### Beam On Open Items
- Active polarity: **TBD**
- Beam On imposes no restrictions on allowable commands or operational modes.

### 7.4 HC <-> Low Voltage Power Supply
The HC shall control an active LOW signal `nLTC3901_Pwr_Enable` to enable or disable 12V power to DUT1.

The HC shall measure DUT1 input current using differential shunt sense measurements derived from two ADC inputs.

#### Low Voltage Power Open Items
- Default startup state confirmed OFF: assumed YES
- Maximum allowed DUT1 current: variable `VAR_DUT1_OVERCURRENT_LIMIT`
- Overcurrent response behavior: variable-driven by `VAR_DUT1_OVERCURRENT_DEBOUNCE` plus policy logic

### 7.5 HC <-> High Voltage Power Supply
The HC shall control an active HIGH signal `HV_Pwr_Enable` to enable or disable high voltage power to DUT2.

#### High Voltage Power Open Items
- Whether HV may be enabled automatically by firmware or only by command: **TBD**
- Required shutdown timing on fault: **TBD**

### 7.6 TE <-> HC
The TE and HC shall communicate over a USB connection using a Virtual COM Port (VCP) interface and a serial communications protocol.

The USB VCP interface shall be the primary mode of communication between the HC and the TE.

The HC shall periodically report status to the TE.

The HC shall accept ad hoc commands from the TE and shall respond when a response is required by the command protocol.

#### TE Interface Open Items
- Packet format: **TBD**
- Command set: **TBD**
- Status report period: **TBD**
- Error detection mechanism (checksum/CRC): **TBD**
- Timeout and retry behavior: **TBD**

### 7.7 Debug <-> HC
The HC shall expose a serial debug terminal on PA9/PA10.

The HC shall configure the debug UART for 115200 baud.

The debug terminal shall provide development insight and optional control functions.

A debug command / response shall indicate only whether the USB VCP connection to the TE is active or not.

#### Debug Open Items
- Command set and authority limits: **TBD**
- Whether debug can override TE commands: **TBD**

### 7.8 Status LEDs <-> HC
The HC shall drive one red LED and one green LED as visual indicators of system state.

The HC shall communicate low-level fault conditions using LED flash codes.

#### LED Open Items
- Normal-state LED patterns: **TBD**
- Fault flash code table: **TBD**
- Flash cadence and repeat period: **TBD**

### 7.9 ID <-> HC
The HC shall read its hardware ID from 6 binary switches connected to GPIO inputs.

The HC shall make the ID available to internal software and to TE status reporting.

#### ID Open Items
- Bit ordering and active polarity: **TBD**
- Whether invalid combinations exist: **TBD**

## 8. Internal Platform Context
To fulfill its functional requirements, the HC shall utilize the following internal hardware peripherals:
1. System management
2. Timers
3. ADC
4. UART
5. USB
6. GPIO

## 9. Operational States
The HC shall implement a state machine with the following states:
1. Boot
2. Low-Level Fault
3. High-Level Fault
4. Normal
5. Slave

### 9.1 State: Boot
The HC shall enter the Boot state on power-up and after reset.

#### Boot Entry Actions
1. Initialize all GPIO and ensure all output signals are set to OFF.
2. Set the Time Since Boot (TSB) clock to 0.
3. Initialize the debug UART at 115200 baud.
4. Transmit the phrase `BOOTING...` on the debug UART.
5. Initialize required timers, ADC resources, USB resources, and internal software modules.
6. Read the 6-bit HC ID.

#### Boot Exit Conditions
- The HC shall transition from Boot to Normal when all mandatory low-level tests and initialization steps pass.
- The HC shall transition from Boot to Low-Level Fault when a mandatory low-level test or initialization step fails, except where a failure is considered catastrophic as described below.
- Initialization failure of the debug UART or USB/VCP interface shall be treated as a catastrophic hardware and/or firmware failure for which normal LLF-versus-other fault management distinctions are operationally moot.

#### Boot Open Items
- Whether USB connection establishment is mandatory before leaving Boot: **TBD**
- Required boot-time self-tests: **TBD**

### 9.2 State: Low-Level Fault
The HC shall enter Low-Level Fault when an internal platform or initialization failure prevents trusted normal operation, except for catastrophic initialization failures where continued fault-state management is operationally moot.

Candidate examples include:
- Failure to initialize required peripherals
- Internal configuration failure
- Self-test failure
- Other internal conditions that invalidate monitoring/control behavior

In Low-Level Fault, the HC shall place controlled outputs in a safe state unless otherwise specified.

The HC shall continue attempting USB connection establishment if USB is not connected or not yet enumerated, unless a more specific requirement supersedes this behavior.

Initialization failure of the debug UART or USB/VCP interface shall be regarded as a catastrophic boot failure caused by hardware and/or firmware failure; handling such cases as LLF versus any other runtime fault category is not operationally meaningful.

#### Low-Level Fault Open Items
- Full list of low-level faults: **TBD**
- USB absence after successful stack initialization shall be treated as a **degraded condition only**.
- Recovery method: reset only, command clear, or auto-retry: **TBD**

### 9.3 State: High-Level Fault
The HC shall enter High-Level Fault when a fault is detected on one or more DUTs during runtime.

Examples may include:
- DUT1 overcurrent
- DUT1 gate activity out of range
- DUT2 output voltage out of range
- DUT2 gate activity missing or invalid

#### High-Level Fault Open Items
- Exact detection criteria: **TBD**
- Latching behavior: **TBD**
- Whether DUT power rails are automatically disabled: **TBD**
- Recovery authority and sequence: **TBD**

### 9.4 State: Normal
In Normal state, the HC shall perform normal monitoring operation and ongoing communications with the TE.

In Normal state, the HC shall:
- Monitor DUT1 and DUT2 signals
- Update internal status and fault assessments
- Send periodic status reports to the TE
- Process allowed TE commands
- Maintain LED indications for healthy operation

### 9.5 State: Slave
Slave state is similar to Normal but permits additional external command-driven overrides through the TE or debug interface.

#### Slave Open Items
- Which interface may request Slave state: **TBD**
- Which signals/behaviors may be overridden: **TBD**
- Safety restrictions while in Slave state: **TBD**
- Whether Beam On restricts Slave behavior: **TBD**

## 10. Functional Requirements

### 10.1 Safe Startup and Default Output Control
- The HC shall initialize all controllable outputs to their safe OFF states at startup and reset before enabling any DUT power path.
- The HC shall maintain safe output states when entering Low-Level Fault unless explicitly documented otherwise.

### 10.2 Time Since Boot
- The HC shall maintain a Time Since Boot (TSB) counter beginning at zero during Boot.
- The HC shall make TSB available for status reporting. Format and resolution are **TBD**.

### 10.3 DUT1 Stimulus Generation
- The HC shall generate quadrature-encoded SyncA and SyncB outputs for DUT1.
- The HC shall maintain the configured quadrature phase relationship within a tolerance of **TBD**.
- The HC shall provide a means to configure or select the Sync frequency if required by the system design. Configuration authority is **TBD**.

### 10.4 DUT1 Monitoring
- The HC shall measure DUT1 ME and MF direct gate driver outputs.
- The HC shall monitor filtered ME and MF gate signals using ADC inputs.
- The HC shall monitor DUT1 input current using shunt-based differential voltage measurements.
- The HC shall detect DUT1 faults according to thresholds, timing limits, and persistence criteria defined elsewhere in this PRD or a linked fault matrix.

### 10.5 DUT2 Monitoring
- The HC shall monitor DUT2 output voltage via ADC.
- The HC shall monitor DUT2 gate driver output via direct timing measurement.
- The HC shall detect DUT2 faults according to thresholds, timing limits, and persistence criteria defined elsewhere in this PRD or a linked fault matrix.

### 10.6 Power Control
- The HC shall control DUT1 power through `nLTC3901_Pwr_Enable`.
- The HC shall control DUT2 power through `HV_Pwr_Enable`.
- The HC shall define and enforce state-based rules governing when each power rail may be enabled. Those rules are **TBD**.

### 10.7 TE Communications
- The HC shall communicate with the TE over USB.
- The HC shall send periodic status reports at an interval of **TBD**.
- The HC shall respond to TE commands according to a command protocol that is **TBD**.
- The HC shall include HC ID, state, and fault status in periodic status reporting. Additional required fields are **TBD**.

### 10.8 Debug Interface
- The HC shall provide a debug UART on PA9/PA10 at 115200 baud.
- On each boot, the HC shall transmit `BOOTING...` on the debug UART.
- Additional debug output behavior shall be controlled to avoid disrupting timing-critical monitoring. Policy is **TBD**.

### 10.9 LED Indication
- The HC shall drive red and green LEDs to indicate system state.
- The HC shall encode low-level fault conditions as LED flash codes.
- The detailed LED indication table is **TBD**.

### 10.10 ID Reporting
- The HC shall read a 6-bit hardware ID from GPIO inputs.
- The HC shall expose the ID in status reporting and debug visibility.

## 11. Fault Management Requirements
The HC shall classify faults into at least the following categories:
- **Low-Level Fault:** Internal/platform faults that prevent trusted operation
- **High-Level Fault:** Runtime DUT-related faults detected during operation

The HC shall maintain fault reason information sufficient for TE reporting and local diagnosis.

The HC shall define for each fault:
- trigger condition
- source of detection
- debounce/persistence requirement
- severity
- state transition result
- output shutdown behavior
- reporting behavior
- recovery method

These details remain **TBD** pending creation of a fault response matrix.

## 12. Command and Reporting Requirements
A future interface control section shall define:
- command names or packet IDs
- argument formats
- response formats
- event report formats
- periodic status report fields
- protocol framing
- integrity protection
- timing and timeout rules

Candidate command capabilities include:
- Get status
- Get ID
- Query faults
- Clear faults
- Enter/Exit Slave mode
- Control DUT1 power
- Control DUT2 power
- Configure reporting behavior
- Debug/diagnostic queries

All command semantics remain **TBD**.

## 13. Non-Functional Requirements
### 13.1 Determinism
The HC firmware shall execute monitoring and control logic with timing sufficient to meet all detection and reporting deadlines. Numerical deadlines are **TBD**.

### 13.2 Safety / Fail-Safe Behavior
The HC shall favor safe output shutdown behavior when entering fault states, unless an explicit requirement states otherwise.

### 13.3 Serviceability
The HC shall provide enough local and remote observability to determine current state, major measurements, and fault causes during development and test execution.

### 13.4 Maintainability
The HC firmware design shall support traceability between requirements, architecture, implementation, and tests.

## 14. Acceptance Criteria (Draft)
The following draft acceptance criteria shall be refined into a verification matrix:

1. On reset, all output control pins are set to safe OFF states before any DUT power enable assertion.
2. The debug UART initializes at 115200 baud and transmits `BOOTING...` during boot.
3. The HC reads the 6-bit ID and includes it in internal status.
4. The HC enters Normal only after required initialization and low-level checks pass.
5. The HC enters Low-Level Fault when required initialization fails.
6. The HC generates DUT1 quadrature SyncA/SyncB outputs according to configured behavior.
7. The HC measures DUT1 ME/MF timing and DUT1 current.
8. The HC measures DUT2 output voltage and gate activity.
9. The HC sends periodic status reports to the TE at the defined interval.
10. The HC detects defined DUT runtime faults and transitions to High-Level Fault as specified.
11. The HC provides LED indications for normal and low-level fault conditions.
12. The HC supports Slave mode behavior according to defined override restrictions.

## 15. Risks and Open Questions
1. USB connectivity behavior during boot and faulted conditions is not yet fully defined.
2. TE protocol format is not yet defined.
3. Fault thresholds and persistence/debounce rules are not yet defined.
4. Slave mode authority and safety boundaries are not yet defined.
5. Exact normal LED behavior and flash-code mapping are not yet defined.
6. Timing budgets for ADC sampling, input capture evaluation, and reporting are not yet defined.
7. The exact MCU/platform and execution model are not yet named in this document.

## 16. Recommended Next BMAD Artifacts
The next artifacts to produce are:
1. **Fault Response Matrix**
2. **TE/Debug Interface Control Specification**
3. **State Machine Specification**
4. **Firmware Architecture Document**
5. **Story Backlog with Acceptance Criteria**
6. **Verification and Traceability Matrix**

## 17. Architecture Placeholder
The firmware architecture shall be defined in a separate architecture section or linked architecture document. It should cover:
- execution model
- module decomposition
- interrupt usage
- timer allocation
- ADC acquisition design
- communication stack structure
- state machine implementation approach
- fault arbitration
- data model for status and reporting

## 18. Revision Notes
- v1: Initial BMAD-style PRD draft created from incomplete source text with explicit TBD markers for unresolved requirements.


---

## 2. Fault Response Matrix v1

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


---

## 3. Variable Registry v1

# Radiation Test Host Controller (HC) Variable Registry v1

## 1. Document Purpose
This document defines the named variables used by the Radiation Test Host Controller (HC) requirements and fault logic when numeric values are not yet finalized.

These variables represent thresholds, limits, tolerances, timing values, debounce windows, update rates, and other experimentally derived constants whose numeric values will be assigned later.

This registry serves as the single source of truth for deferred numeric parameters referenced by:
- `/a0/usr/projects/bmad_test/docs/hc_prd_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_fault_response_matrix_v1.md`

## 2. Variable Policy
- Each unresolved numeric requirement shall be represented by a uniquely named variable.
- Variable names shall remain stable even if final numeric values change.
- Numeric values shall be assigned later from experiment, measurement, analysis, or datasheet review.
- Units shall be recorded explicitly.
- If a variable is derived from other variables, that relationship should be noted.
- Variables may later be mapped into firmware configuration headers, calibration tables, or nonvolatile parameter storage.

## 3. Variable Status Definitions
| Status | Meaning |
|---|---|
| Proposed | Variable name exists but may still be renamed or split |
| Defined | Variable name and purpose are stable, numeric value still TBD |
| Experiment Pending | Intended for lab characterization |
| Derived | To be calculated from other measured/defined values |
| Finalized | Numeric value approved for implementation |

## 4. Variable Registry
| Variable Name | Meaning | Units | Source | Applies To | Initial Value | Final Value | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| `VAR_DUT1_SYNCA_FREQ` | DUT1 SyncA target frequency | Hz | Experiment / design intent | PRD 7.1 | TBD | TBD | Proposed | May be identical to SyncB frequency depending on architecture |
| `VAR_DUT1_SYNCB_FREQ` | DUT1 SyncB target frequency | Hz | Experiment / design intent | PRD 7.1 | TBD | TBD | Proposed | May collapse into one sync frequency variable later |
| `VAR_DUT1_SYNC_PHASE_TARGET` | DUT1 SyncA-to-SyncB phase target | degrees or time | Experiment / design intent | PRD 7.1 | TBD | TBD | Proposed | Unit to be finalized |
| `VAR_DUT1_SYNC_PHASE_TOLERANCE` | Allowed DUT1 sync phase error | degrees or time | Experiment / analysis | PRD 7.1 | TBD | TBD | Proposed | Unit to be finalized |
| `VAR_DUT1_ME_FREQ_MIN` | Minimum valid DUT1 ME gate frequency | Hz | Experiment | PRD 7.1, HLF-005 | TBD | TBD | Defined | Used with range checking |
| `VAR_DUT1_ME_FREQ_MAX` | Maximum valid DUT1 ME gate frequency | Hz | Experiment | PRD 7.1, HLF-005 | TBD | TBD | Defined | Used with range checking |
| `VAR_DUT1_ME_DUTY_MIN` | Minimum valid DUT1 ME duty cycle | % | Experiment | PRD 7.1, HLF-005 | TBD | TBD | Defined | Percent basis to be standardized |
| `VAR_DUT1_ME_DUTY_MAX` | Maximum valid DUT1 ME duty cycle | % | Experiment | PRD 7.1, HLF-005 | TBD | TBD | Defined | Percent basis to be standardized |
| `VAR_DUT1_MF_FREQ_MIN` | Minimum valid DUT1 MF gate frequency | Hz | Experiment | PRD 7.1, HLF-006 | TBD | TBD | Defined | Used with range checking |
| `VAR_DUT1_MF_FREQ_MAX` | Maximum valid DUT1 MF gate frequency | Hz | Experiment | PRD 7.1, HLF-006 | TBD | TBD | Defined | Used with range checking |
| `VAR_DUT1_MF_DUTY_MIN` | Minimum valid DUT1 MF duty cycle | % | Experiment | PRD 7.1, HLF-006 | TBD | TBD | Defined | Percent basis to be standardized |
| `VAR_DUT1_MF_DUTY_MAX` | Maximum valid DUT1 MF duty cycle | % | Experiment | PRD 7.1, HLF-006 | TBD | TBD | Defined | Percent basis to be standardized |
| `VAR_DUT1_ADC_SAMPLE_RATE` | Raw ADC sampling rate for DUT1 analog channels | samples/s | Firmware design / experiment | PRD 7.1 | TBD | TBD | Proposed | May differ from reporting/update rate |
| `VAR_DUT1_ADC_UPDATE_RATE` | Processed update rate for DUT1 analog measurements | Hz | Firmware design / experiment | PRD 7.1 | TBD | TBD | Proposed | Could be task rate rather than ADC hardware rate |
| `VAR_DUT1_OVERCURRENT_LIMIT` | DUT1 input overcurrent trip threshold | A | Experiment | PRD 7.1, 7.4, HLF-001 | TBD | TBD | Defined | Derived from shunt measurement scaling |
| `VAR_DUT1_OVERCURRENT_DEBOUNCE` | Persistence/debounce before asserting DUT1 overcurrent fault | ms or samples | Experiment | PRD 7.1, 7.4, HLF-001 | TBD | TBD | Defined | Unit to be finalized |
| `VAR_DUT1_CURRENT_SENSE_INVALID_DEBOUNCE` | Persistence before flagging DUT1 current-sense invalid condition | ms or samples | Experiment | HLF-002 | TBD | TBD | Proposed | Plausibility-rule variables may also be needed |
| `VAR_DUT1_ME_ACTIVITY_TIMEOUT` | Timeout for missing DUT1 ME activity | us or ms | Experiment | HLF-003 | TBD | TBD | Defined | Unit depends on expected switching rate |
| `VAR_DUT1_MF_ACTIVITY_TIMEOUT` | Timeout for missing DUT1 MF activity | us or ms | Experiment | HLF-004 | TBD | TBD | Defined | Unit depends on expected switching rate |
| `VAR_DUT1_ME_RANGE_DEBOUNCE` | Persistence before asserting DUT1 ME out-of-range fault | ms or samples | Experiment | HLF-005 | TBD | TBD | Defined | Applies to ME frequency/duty range checks |
| `VAR_DUT1_MF_RANGE_DEBOUNCE` | Persistence before asserting DUT1 MF out-of-range fault | ms or samples | Experiment | HLF-006 | TBD | TBD | Defined | Applies to MF frequency/duty range checks |
| `VAR_DUT1_FILTERED_ME_MIN` | Minimum valid filtered DUT1 ME analog level | V or ADC counts | Experiment | HLF-007 | TBD | TBD | Proposed | Unit to be standardized |
| `VAR_DUT1_FILTERED_ME_MAX` | Maximum valid filtered DUT1 ME analog level | V or ADC counts | Experiment | HLF-007 | TBD | TBD | Proposed | Unit to be standardized |
| `VAR_DUT1_FILTERED_ME_RANGE_DEBOUNCE` | Persistence before asserting filtered DUT1 ME analog-range anomaly | ms or samples | Experiment | HLF-007 | TBD | TBD | Proposed | May become WRN-only depending on policy |
| `VAR_DUT1_FILTERED_MF_MIN` | Minimum valid filtered DUT1 MF analog level | V or ADC counts | Experiment | HLF-008 | TBD | TBD | Proposed | Unit to be standardized |
| `VAR_DUT1_FILTERED_MF_MAX` | Maximum valid filtered DUT1 MF analog level | V or ADC counts | Experiment | HLF-008 | TBD | TBD | Proposed | Unit to be standardized |
| `VAR_DUT1_FILTERED_MF_RANGE_DEBOUNCE` | Persistence before asserting filtered DUT1 MF analog-range anomaly | ms or samples | Experiment | HLF-008 | TBD | TBD | Proposed | May become WRN-only depending on policy |
| `VAR_DUT1_SYNC_FAILURE_DEBOUNCE` | Persistence before declaring DUT1 sync generation failure | ms or samples | Firmware design / experiment | HLF-009 | TBD | TBD | Proposed | Optional if supervision is immediate |
| `VAR_DUT1_UNEXPECTED_ACTIVITY_DEBOUNCE` | Persistence before flagging unexpected DUT1 activity while power disabled | ms or samples | Experiment | HLF-010 | TBD | TBD | Proposed | Helps suppress transient artifacts |
| `VAR_DUT2_VOUT_LOW_LIMIT` | Minimum valid DUT2 output voltage | V | Experiment | PRD 7.2, HLF-012 | TBD | TBD | Defined | Fault or warning classification still policy-dependent |
| `VAR_DUT2_VOUT_HIGH_LIMIT` | Maximum valid DUT2 output voltage | V | Experiment | PRD 7.2, HLF-011 | TBD | TBD | Defined | Safety-critical threshold |
| `VAR_DUT2_GATE_FREQ_MIN` | Minimum valid DUT2 gate frequency | Hz | Experiment | PRD 7.2, HLF-014 | TBD | TBD | Defined | Used with range checking |
| `VAR_DUT2_GATE_FREQ_MAX` | Maximum valid DUT2 gate frequency | Hz | Experiment | PRD 7.2, HLF-014 | TBD | TBD | Defined | Used with range checking |
| `VAR_DUT2_GATE_DUTY_MIN` | Minimum valid DUT2 gate duty cycle | % | Experiment | PRD 7.2, HLF-014 | TBD | TBD | Defined | Percent basis to be standardized |
| `VAR_DUT2_GATE_DUTY_MAX` | Maximum valid DUT2 gate duty cycle | % | Experiment | PRD 7.2, HLF-014 | TBD | TBD | Defined | Percent basis to be standardized |
| `VAR_DUT2_GATE_ACTIVITY_TIMEOUT` | Timeout for missing DUT2 gate activity | us or ms | Experiment | PRD 7.2, HLF-013 | TBD | TBD | Defined | Unit depends on expected switching rate |
| `VAR_DUT2_ADC_SAMPLE_RATE` | Raw ADC sampling rate for DUT2 analog channels | samples/s | Firmware design / experiment | PRD 7.2 | TBD | TBD | Proposed | May differ from processed update rate |
| `VAR_DUT2_ADC_UPDATE_RATE` | Processed update rate for DUT2 analog measurements | Hz | Firmware design / experiment | PRD 7.2 | TBD | TBD | Proposed | Could be task rate rather than ADC hardware rate |
| `VAR_DUT2_VOUT_HIGH_DEBOUNCE` | Persistence before asserting DUT2 overvoltage fault | ms or samples | Experiment | HLF-011 | TBD | TBD | Defined | Unit to be finalized |
| `VAR_DUT2_VOUT_LOW_DEBOUNCE` | Persistence before asserting DUT2 undervoltage anomaly | ms or samples | Experiment | HLF-012 | TBD | TBD | Defined | Unit to be finalized |
| `VAR_DUT2_GATE_RANGE_DEBOUNCE` | Persistence before asserting DUT2 gate out-of-range fault | ms or samples | Experiment | HLF-014 | TBD | TBD | Defined | Applies to frequency/duty range checks |
| `VAR_HV_ENABLE_FEEDBACK_MISMATCH_DEBOUNCE` | Persistence before asserting HV enable feedback mismatch | ms or samples | Experiment / design | HLF-015 | TBD | TBD | Proposed | Requires feedback mechanism definition |
| `VAR_DUT1_ENABLE_FEEDBACK_MISMATCH_DEBOUNCE` | Persistence before asserting DUT1 power-enable feedback mismatch | ms or samples | Experiment / design | HLF-016 | TBD | TBD | Proposed | Requires feedback mechanism definition |
| `VAR_MEASUREMENT_ACQUISITION_STALL_TIMEOUT` | Timeout for stalled ADC/capture acquisition | ms | Firmware design / experiment | HLF-019 | TBD | TBD | Defined | May be driven by scheduler/update-rate architecture |

## 5. Additional Variable Placeholders Still Likely Needed
The following variable groups are anticipated but not fully defined yet:
- Current-sense plausibility limits for DUT1
- Unexpected-activity detection thresholds for DUT1 when power is disabled
- Feedback validation thresholds for HV and DUT1 power-enable paths
- LED flash-code timing variables
- TE periodic status report interval variables
- Fault recovery dwell-time variables if separate from debounce variables

## 6. Recommended Next Use of This Registry
This registry should next be used to:
1. align all remaining PRD `TBD` numeric items to named variables
2. reference variables directly in firmware architecture and story definitions
3. define which variables belong in build-time constants versus runtime-configurable parameters
4. support future lab characterization and traceability from experiment to firmware

## 7. Revision Notes
- v1: Initial HC variable registry created from PRD v1 and Fault Response Matrix v1.4.


---

## 4. Firmware Architecture v1

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


---

## 5. TE Interface Specification v1

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


---

## 6. Protocol Test Plan v1

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


---

## 7. State Machine Specification v1

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


---

## 8. Verification & Traceability Matrix v1

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

---

## 9. Story Backlog v1

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


---
