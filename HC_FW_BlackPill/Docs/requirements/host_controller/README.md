# Host Controller Requirements

This folder contains current requirements and implementation-aligned design
documents for the STM32 Host Controller firmware.

The documentation is intentionally centered on behavior that is implemented in
the project source. Obsolete draft artifacts, old verb-style command plans, and
old state-table files have been removed to avoid conflicting sources of truth.

Use `Index.md` as the entry point.

Primary sources of truth:

- firmware source under `App/`, `Bsp/`, `Drivers_Local/`, and `Services/`
- JSON protocol examples in `Docs/JSON_Tests.md`
- hardware test cases in `Test/hw_test_cases.json`
- hardware test reports in `Test/Reports/`

Documentation maintenance rules:

- Add new requirements only when they describe intended current behavior.
- Keep command requirements in the JSON `GET` / `SET args` model.
- Use `Manager` nomenclature for LTC3901 and LT8316 state machines.
- Keep runtime variables aligned to implemented command-visible fields.
- Do not reintroduce retired command names or draft-only fault models unless
  they are first implemented or explicitly approved as new requirements.
