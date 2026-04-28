This file describes the functional requirements for the Radiation Test Host Controller - HC.


The HC monitors several devices under test (DUTs) while they are subjected to Heavy Ion / Proton Beam bombardment. The HC provides monitoring and fault detection logic, and it periodically reports to the Test Executive. 

# External System Context
This Host Controller is an element of a bigger Test System comprised of:
1. A Test Executive (TE)
2. A Heavy Ion / Proton Radiation Beam
3. A High Voltage Power Supply
4. A Low Voltage Power Supply
5. An LTC3901 DC/DC Power Controller IC (DUT1)
6. An LT8316 DC/DC Power Controller ID (DUT2)
7. An (optional) serial Debug terminal
8. Two Status LEDs
## External Connectivity
### HC <-> LTC3901 (DUT1)
The HC provides a Quadrature Encoded SyncA and SyncB stimulus signal to the LTC3901.
The HC measures the LTC3901 ME & MF Gate Driver outputs via direct (Input Capture) measurements.
A Low Pass Filtered version of the LTC3901's ME & MF Gate Driver outputs are also monitored on separate ADC inputs to the HC.
The HC monitors the current feeding into the LTC3901

### HC <-> LT8316 (DUT2)
When powered, the LT8316 requires no further stimulus from the HC to run.
The HC monitors the LT8316 output voltage through an ADC input
The HC monitors the LT8316 Gate Driver Output via direct (Input Capture) measurement.

### HC <-> Heavy Ion / Proton Beam
The Heavy Ion / Proton Beam is under the control of the TE and asserts a "Beam On" logic signal to the HC.

### HC <-> Low Voltage Power Supply
DUT1 requires 12V to operate.
The HC monitors the current to DUT1 by measuring the voltage on both sides of a shunt resistor sitting in the input power path.
The HC controls an active LOW "nLTC3901_Pwr_Enable" signal to enable 12V power to DUT1

### HC <-> High Voltage Power Supply
DUT2 requires a High Voltage (>150V) input to operate.
The HC controls an active HIGH "HV_Pwr_Enable" signal to enable HV power to DUT2

### TE <-> HC
The TE and HC communicate with one another via serial communications over a USB connection
The HC periodically reports the status of the DUTs to the TE
The TE sends various commands on an adhoc basis to the HC. The HC will typically respond to these commands.

### Debug <-> HC
A serial Debug terminal will be connected to the HC to provide further insight and control over the HC during development.
The HC will utilize PA9 and PA10 as its UART TX and RX pins respectively.
The baudrate will be 115200.
### Status LED <-> HC
Two separate RED and GREEN LEDs will be connected to the HC as a means for providing a visual indicator of the HC's state.
Low-level fault conditions will be communicated as Flash Codes via these LEDs.

### ID <-> HC
The HC's "ID" will be set via 6 binary switches connected to its GPIO inputs

## HC Internal System Context
To fulfill its functional requirements, the HC will utilize the following internal HW peripherals:
1. System Management
2. Timers
3. Analog-to-Digital Converters (ADC)
4. UART
5. USB
6. GPIO

### HC Operational States
The HC will migrate through a State Machine comprised of the following States:
1. Boot
2. Low-Level Fault
3. High-Level Fault
4. Normal
5. Slave

#### State: Boot
The HC will power up (or return from Reset) in the Boot State
After satisfying all low level tests and initialization sequences, the HC will proceed to the Normal State.
If a fault is detected during low-level tests and initialization, the HC will move to the Low-Level Fault State.

##### Boot Sequence
1. Initialize all GPIO and ensure all output signals are set to OFF
2. Set the TimeSinceBoot (TSB) clock to 0.
3. Initialize the DEBUG UART at 115200 baud and transmit the phrase "BOOTING..."

#### State: Low-Level Fault
Note: USB connectivity fails to be established - continue to poll for a connection
Other?

#### State: High-Level Fault
A fault has been detected on one or more of the DUTs
#### State: Normal
Normal monitoring operation with ongoing communications to the TE

#### State: Slave
Similar to Normal but with overrides imposed through external commands (from the Debug or TE interface)

# STM32 Pin Connections

| Signal Name | Pin | Peripheral | Description                                            |
| ----------- | --- | ---------- | ------------------------------------------------------ |
| SDRA        | PB4 | T3_Ch1     | Combine with SDRB to generate a 1kHz Quadrature Signal |
| SDRB        | PB5 | T3_Ch2     |                                                        |
