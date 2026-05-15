# Hardware Test Automation Notes

Goal: provide a repeatable hardware-in-the-loop test process that can be run after the established build and programming workflows.

Current scope:
- transport: USB CDC / VCP
- protocol: JSONL request/response
- case source: `Test/hw_test_cases.json`
- report output: `build/<Preset>/hw_tests/<timestamp>/summary.md`
- transcript output: `build/<Preset>/hw_tests/<timestamp>/results.jsonl`
- durable review report output: `Test/Reports/HardwareTestReport_<timestamp>.md`
- durable review transcript output: `Test/Reports/HardwareTestResults_<timestamp>.jsonl`
- durable report index: `Test/Reports/Index.md`

Primary test command:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\test.ps1 -Build -Program -ContinueOnFail
```

Useful variants:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\test.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\test.ps1 -PortName COM7
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\test.ps1 -CasePath Test\hw_test_cases.json -ContinueOnFail
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\test.ps1 -ReportRoot Test\Reports
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\test.ps1 -SkipReportArchive
```

The `-Build` option invokes the guarded command-list build:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\build.ps1 -Preset Debug -Backend Commands -PerCommandTimeoutSeconds 30
```

The `-Program` option invokes the established ST-Link/OpenOCD programming flow:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Tools\program.ps1 -ElfPath build\Debug\HC_FW_BlackPill.elf
```

Test flow:
1. Optionally build the selected preset.
2. Optionally program and verify the target over ST-Link/SWD.
3. Discover the USB CDC/VCP serial port. Do not assume it will always be `COM7`.
4. Load test definitions from `Test/hw_test_cases.json`.
5. Open the serial port and clear stale input.
6. Send each test request as compact JSON plus newline.
7. Collect JSON packets until the matching `RSP.msg` is observed or the serial timeout expires.
8. Compare the actual response against the expected fields in the case file.
9. If a case is marked `program_after:true`, close the serial port, re-run the established ST-Link programming workflow, rediscover the USB CDC/VCP port, and continue testing against the reset target.
10. Write one JSONL result per case and a human-readable Markdown summary.
11. Archive the Markdown report, raw JSONL transcript, and index entry under `Test/Reports/` unless `-SkipReportArchive` is supplied.
12. Return exit code `0` only when all executed cases pass.

Report artifacts:
- `build/<Preset>/hw_tests/<timestamp>/summary.md` is the immediate run-local summary.
- `build/<Preset>/hw_tests/<timestamp>/results.jsonl` is the immediate run-local machine-readable transcript.
- `Test/Reports/HardwareTestReport_<timestamp>.md` is the durable human review record.
- `Test/Reports/HardwareTestResults_<timestamp>.jsonl` is the durable raw transcript copy.
- `Test/Reports/Index.md` is automatically appended with the run time, pass/fail status, preset, serial target, build/program flags, pass/fail counts, and links to the durable artifacts.
- Run-local `build/<Preset>/hw_tests/<timestamp>` folders are disposable once archived. The runner keeps only the latest local runs, controlled by `-KeepRunLocalResultCount` which defaults to `5`.

Current coverage:
- setup and cleanup cases for periodic `STS`, periodic `DBG`, and manager reset state
- all numbered examples from `Docs/JSON_Tests.md`, currently examples 1 through 43
- observable `STS` and `EVT` examples from `Docs/JSON_Tests.md`
- valid command examples, invalid-command examples, malformed/partial JSON examples, oversized-object behavior, and back-to-back object framing

Case-file maintenance:
- Add new cases to `Test/hw_test_cases.json`.
- Keep `source_section` pointing back to the relevant requirement, protocol note, or `Docs/JSON_Tests.md` example.
- Use unique `msg` values so request/response correlation stays deterministic.
- Prefer low-risk read-only or configuration-neutral tests first.
- Avoid manager `RUN`, power-control, or output-driving tests unless the hardware test conditions are explicitly documented.
- Expected values can be exact values or regex matchers:
  - exact: `"date_time": "20260501 10:30:00"`
  - matcher: `"date_time": { "matches": "^20260501 10:30:[0-5][0-9]$" }`
- The runner also supports structural matchers for dynamic hardware values:
  - any non-null value: `{ "any": true }`
  - field must be present: `{ "present": true }`
  - numeric value: `{ "number": true }`
  - numeric or `null`: `{ "nullable_number": true }`
  - constrained values: `{ "one_of": ["RESET", "NORMAL"] }`
- Use `request_raw` and `send_newline:false` for framing tests that are not normal JSONL requests.
- Use `expected_responses` when one input stream should produce more than one expected packet.
- Use `expect_no_response:true` with a short `timeout_seconds` for partial-object or oversized-object tests.
- Use `cleanup_raw` only to recover the target command processor after a deliberate partial-object test has already been scored.
- Use `program_after:true` for destructive framing tests that may leave the firmware command processor unable to accept cleanup commands. This is harness recovery only; the original failure remains logged.

Bug handling policy:
- The hardware test runner must not fix firmware bugs.
- When a test fails, preserve the generated `summary.md` and `results.jsonl`.
- Review the matching `Test/Reports/HardwareTestReport_<timestamp>.md` entry first; it is intended as the long-term triage record.
- Log the test conditions, request, expected response, actual response, observed packets, and malformed lines.
- Review failures with a human before changing target firmware.

Known runner constraints:
- The runner extracts JSON objects from the observed serial stream using brace-depth and string-state tracking, so it can handle back-to-back objects and non-JSON noise around objects.
- Periodic `STS` is disabled as a setup step to reduce asynchronous traffic during request/response tests.
- The runner validates fields explicitly listed under `expected`; extra fields such as `hc` are allowed unless a case constrains them.
- Some comprehensive cases intentionally exercise power-manager commands and settable digital signals from `Docs/JSON_Tests.md`; cleanup cases reset managers and disable periodic traffic afterward.
