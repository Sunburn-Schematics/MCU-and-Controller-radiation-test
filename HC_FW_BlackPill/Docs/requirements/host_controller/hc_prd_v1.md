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
