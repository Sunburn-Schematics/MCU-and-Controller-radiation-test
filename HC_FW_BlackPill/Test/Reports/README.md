# Hardware Test Reports

This folder is the durable archive location for hardware-in-the-loop test results
created by `Tools/test.ps1`.

Generated contents:
- `Index.md` is recreated or appended by the test runner and lists archived runs.
- `HardwareTestReport_<timestamp>.md` is the human-readable report for one run.
- `HardwareTestResults_<timestamp>.jsonl` is the machine-readable transcript for
  the same run.

The generated report artifacts are intended for local review and triage. They can
be deleted when no longer needed; the next test run will recreate `Index.md` and
new timestamped report/transcript files unless `-SkipReportArchive` is used.

Repository policy:
- Keep this `README.md` under version control.
- Ignore generated `Index.md`, `HardwareTestReport_*.md`, and
  `HardwareTestResults_*.jsonl` files unless a specific report needs to be shared.
