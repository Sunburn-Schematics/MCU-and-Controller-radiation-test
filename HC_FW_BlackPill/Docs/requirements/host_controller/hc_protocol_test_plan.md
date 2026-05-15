# Host Controller Hardware Test Regime v1

## Purpose

This document describes the hardware-in-the-loop test regime that is implemented
for the current Host Controller firmware. It replaces the earlier speculative
protocol test plan. Nonexistent commands and unimplemented fault-clear workflows
are not included here.

## Implemented Test Assets

| Asset | Purpose |
|---|---|
| `Test/hw_test_cases.json` | Data-driven JSON protocol test cases. |
| `Tools/test.ps1` | Hardware test runner. |
| `Tools/build.ps1` | Guarded build workflow used when `Tools/test.ps1 -Build` is selected. |
| `Tools/program.ps1` | ST-Link/SWD programming workflow used when `Tools/test.ps1 -Program` is selected. |
| `Test/Reports/` | Durable Markdown and JSONL test reports. |
| `build/<Preset>/hw_tests/<timestamp>/` | Disposable run-local test output. |

## Primary Command

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\test.ps1 -Build -Program -ContinueOnFail
```

Useful focused variants:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\test.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\test.ps1 -CasePath Test\hw_test_cases.json -ContinueOnFail
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\test.ps1 -ReportRoot Test\Reports
```

## Test Flow

1. Optionally build the selected CMake preset using the established guarded build
   workflow.
2. Optionally program and verify the target over ST-Link/SWD.
3. Discover the USB CDC/VCP serial port. The test process must not assume a
   fixed COM port.
4. Load test cases from `Test/hw_test_cases.json`.
5. Open the serial port and clear stale input.
6. Send each test request as defined by the case file.
7. Collect JSON packets until the expected response, timeout, or no-response
   condition is reached.
8. Compare actual response fields against expected exact values or matchers.
9. Reprogram the target after a case when `program_after:true` is specified.
10. Write run-local `summary.md` and `results.jsonl`.
11. Archive durable Markdown and JSONL reports under `Test/Reports/` unless
    archive output is disabled.
12. Return a nonzero process exit code if any executed test fails.

## Current Coverage

The current test regime covers:

- valid JSON `GET` and `SET` requests from `Docs/JSON_Tests.md`
- date/time set and readback
- software version readback
- raw ADC readback
- ADC calibration set and readback
- `STS` period configuration and observable periodic `STS`
- `DBG` period and signal configuration
- supported digital output setting through `dbg_signals`
- LTC3901 manager commands and configuration
- LT8316 manager commands and configuration
- observable `EVT` behavior
- invalid command type and unsupported-field handling
- malformed, partial, oversized, and back-to-back JSON framing cases
- cleanup/reset cases that return managers and report periods to known states

The test runner is intentionally data-driven so new cases can be added to
`Test/hw_test_cases.json` without changing the runner.

## Report Outputs

| Output | Purpose |
|---|---|
| `build/<Preset>/hw_tests/<timestamp>/summary.md` | Run-local human summary. |
| `build/<Preset>/hw_tests/<timestamp>/results.jsonl` | Run-local machine-readable transcript. |
| `Test/Reports/HardwareTestReport_<timestamp>.md` | Durable human review report. |
| `Test/Reports/HardwareTestResults_<timestamp>.jsonl` | Durable raw result transcript. |
| `Test/Reports/Index.md` | Generated index of durable test reports. |

## Latest Known Comprehensive Result

At the time this document was updated:

- suite size: 58 cases
- result: 56 passed, 2 failed

Known failing cases:

| Finding | Expected | Actual |
|---|---|---|
| Unquoted JSON object key | rejected as bad JSON | accepted by firmware parser |
| Two back-to-back valid JSON objects in one input stream | matching responses emitted | no response observed |

These are logged as firmware/protocol findings for human triage. This document
does not prescribe fixes.

## Case Maintenance Rules

- Add or amend cases in `Test/hw_test_cases.json`.
- Keep `source_section` aligned to the requirement, protocol note, or
  `Docs/JSON_Tests.md` example being exercised.
- Use unique `msg` values for deterministic request/response correlation.
- Prefer low-risk read-only or configuration-neutral tests unless the target
  hardware conditions are explicitly documented.
- Use `program_after:true` for tests that intentionally leave the parser or
  target in a state that cannot be reliably cleaned up over the serial protocol.
- Preserve failing report artifacts for triage; do not fix firmware as part of
  test execution unless a separate bug-fix task is opened.

## Out of Scope

The current automated regime does not test:

- nonexistent verb-style commands
- centralized top-level fault query/clear behavior
- centralized top-level application state behavior
- final radiation-test electrical acceptance criteria
- external beamline or external HV supply control protocols
