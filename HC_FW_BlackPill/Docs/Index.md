# Documentation Index

This file is the top-level navigation point for the project documentation set.
Update it when documentation files are added, removed, renamed, split, or
consolidated.

## Requirements

- [requirements/Index.md](./requirements/Index.md) - requirements folder index.
- [requirements/host_controller/Index.md](./requirements/host_controller/Index.md) - Host Controller requirements index.

Current Host Controller requirements:

- [hc_prd.md](./requirements/host_controller/hc_prd.md) - implemented product requirements.
- [hc_architecture.md](./requirements/host_controller/hc_architecture.md) - current firmware architecture.
- [hc_te_interface_spec.md](./requirements/host_controller/hc_te_interface_spec.md) - JSON `GET` / `SET` TE interface and reporting.
- [hc_state_machine_spec.md](./requirements/host_controller/hc_state_machine_spec.md) - implemented DUT manager state machines.
- [hc_fault_response_matrix.md](./requirements/host_controller/hc_fault_response_matrix.md) - implemented manager fault/event responses.
- [hc_variable_registry.md](./requirements/host_controller/hc_variable_registry.md) - implemented runtime variables and config fields.
- [hc_protocol_test_plan.md](./requirements/host_controller/hc_protocol_test_plan.md) - implemented hardware test regime.
- [hc_reset_reason_and_rtc_startup_policy.md](./requirements/host_controller/hc_reset_reason_and_rtc_startup_policy.md) - reset reason and RTC startup policy.
- [hc_verification_traceability_matrix.md](./requirements/host_controller/hc_verification_traceability_matrix.md) - implementation-backed verification traceability.
- [hc_implementation_alignment_20260515.md](./requirements/host_controller/hc_implementation_alignment_20260515.md) - implementation baseline captured on 2026-05-15.

## Firmware Reference

- [architecture.md](./architecture.md) - firmware layering and module notes.
- [pin_connections.md](./pin_connections.md) - target pin assignments and peripheral mappings.
- [Debug_Signals.md](./Debug_Signals.md) - debug telemetry data dictionary.
- [DUT_Manager_State_Charts.md](./DUT_Manager_State_Charts.md) - DUT manager state chart diagrams.
- [Command_Reference.md](./Command_Reference.md) - TE/HC JSON command and response reference.
- [Command_Syntax.md](./Command_Syntax.md) - formal syntax diagrams for implemented TE/HC JSON commands.
- [JSON_Tests.md](./JSON_Tests.md) - JSON protocol examples and test vectors.

## Notes

- Historical Host Controller planning bundles and old Monitor state-transition
  files have been removed from the current requirements tree.
- Hardware test cases live in `Test/hw_test_cases.json`.
- Durable hardware test reports are archived under `Test/Reports/`.
