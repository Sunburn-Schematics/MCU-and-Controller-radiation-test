# Host Controller Verification Traceability v1

## Purpose

This matrix links implemented Host Controller behavior to the documents and test
assets that verify it. It intentionally excludes unimplemented command names,
centralized top-level fault states, and proposed variables.

## Verification Methods

| Code | Meaning |
|---|---|
| INS | Inspection / document or code review |
| TST | Automated or manual test |
| DEM | Demonstration on target hardware |

## Traceability Matrix

| Behavior | Implementation source | Requirement / design doc | Verification asset | Method | Status |
|---|---|---|---|---|---|
| Safe output defaults on startup | `Bsp/bsp_board.*`, `fw_app_init()` | `hc_architecture.md` | code inspection; hardware programming loop | INS, DEM | Implemented |
| Hardware ID readout | BSP hardware ID read; `RSP.hc`; `STS.hc_id` | `hc_te_interface_spec.md` | `Test/hw_test_cases.json` status/response checks | TST | Implemented |
| USB CDC/VCP JSON transport | USB device stack, `usb_vcp_drv`, command processor | `hc_te_interface_spec.md` | hardware JSON protocol tests | TST | Implemented |
| `GET` / `SET` request model | `Services/CommandHandler/*` | `hc_te_interface_spec.md` | `Test/hw_test_cases.json` | TST | Implemented |
| `RSP` response correlation | response formatter and command dispatcher | `hc_te_interface_spec.md` | JSON response matching by `msg` | TST | Implemented |
| Periodic `STS` output | `fw_app` status reporting | `hc_te_interface_spec.md` | periodic status tests | TST | Implemented |
| Periodic `DBG` output | `hc_debug_telemetry.*`, command handler | `hc_te_interface_spec.md`, `hc_variable_registry.md` | debug telemetry tests | TST | Implemented |
| Asynchronous `EVT` output | manager event handling and response formatter | `hc_state_machine_spec.md`, `hc_fault_response_matrix.md` | event observation tests | TST | Implemented |
| Runtime date/time set/readback | RTC driver and `hc_datetime` | `hc_reset_reason_and_rtc_startup_policy.md` | date/time JSON tests | TST | Implemented |
| ADC raw readback and calibration | ADC sense driver and command fields | `hc_variable_registry.md` | ADC JSON tests | TST | Implemented |
| LTC3901 manager command/config | `ltc3901_manager.*`, command fields | `hc_state_machine_spec.md`, `hc_variable_registry.md` | manager command/config tests | TST | Implemented |
| LT8316 manager command/config | `lt8316_manager.*`, command fields | `hc_state_machine_spec.md`, `hc_variable_registry.md` | manager command/config tests | TST | Implemented |
| Hardware test reporting | `Tools/test.ps1` | `hc_protocol_test_plan.md` | `Test/Reports/*.md`, `*.jsonl` | TST | Implemented |

## Known Test Findings

The latest comprehensive hardware test run recorded two firmware/protocol
findings:

| Finding | Expected | Actual |
|---|---|---|
| Unquoted JSON object key | rejected as bad JSON | accepted |
| Two back-to-back JSON objects in one input stream | responses emitted | no response observed |

These are retained as triage findings and are not fixed by the test process.

## Out of Scope for Current Verification

The current verification matrix does not claim coverage for:

- centralized fault latching or clearing
- top-level `LOW_LEVEL_FAULT`, `HIGH_LEVEL_FAULT`, or `SLAVE` behavior
- retired verb-style commands
- persisted configuration across reset
- external beamline or external HV supply protocols
