# Host Controller Requirements Index

This folder now contains current, implementation-backed Host Controller
requirements and design documents. Obsolete draft bundles and retired state-table
documents have been removed.

Read these first:

- `hc_prd.md` - implemented product requirements.
- `hc_implementation_alignment_20260515.md` - implementation baseline and known
  limitations.
- `hc_te_interface_spec.md` - canonical JSON `GET` / `SET` protocol and
  `STS` / `DBG` / `EVT` reporting.
- `hc_state_machine_spec.md` - implemented `LTC3901_Manager` and
  `LT8316_Manager` state machines.

Supporting current documents:

- `hc_architecture.md`
- `hc_fault_response_matrix.md`
- `hc_variable_registry.md`
- `hc_protocol_test_plan.md`
- `hc_reset_reason_and_rtc_startup_policy.md`
- `hc_verification_traceability_matrix.md`

The current automated hardware test regime is defined by
`Test/hw_test_cases.json` and executed by `Tools/test.ps1`. Durable test reports
are archived under `Test/Reports/`.
