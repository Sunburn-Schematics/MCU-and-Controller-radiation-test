# Documentation Index

This file is the top-level reference for locating and editing the HC documentation set.

## Maintenance Rule
- Update this index whenever files are added to this folder.
- Update this index whenever a file is significantly reworked, renamed, split, or consolidated.
- Use this file as the first navigation point before editing documentation.

## Folder
- `./Docs`

## Files

- [hc_architecture_v1.md](./hc_architecture_v1.md) — Firmware architecture, module decomposition, and control ownership.
- [hc_fault_response_matrix_v1.md](./hc_fault_response_matrix_v1.md) — Fault classes, responses, latching, clearing, and recovery policy matrix.
- [hc_prd_v1.md](./hc_prd_v1.md) — Primary Product Requirements Document for the Host Controller.
- [hc_product_specification_amalgamated_v1.md](./hc_product_specification_amalgamated_v1.md) — Full concatenated source bundle of the current HC specification set.
- [hc_product_specification_review_v1.md](./hc_product_specification_review_v1.md) — Cleaned unified review edition of the HC product specification.
- [hc_product_specification_review_v1.pdf](./hc_product_specification_review_v1.pdf) — PDF rendering of the cleaned unified review edition for human review.
- [hc_protocol_test_plan_v1.md](./hc_protocol_test_plan_v1.md) — Protocol-oriented verification and test planning for HC/TE behavior.
- [hc_state_machine_spec_v1.md](./hc_state_machine_spec_v1.md) — Formal top-level HC state machine definition and state rules.
- [hc_story_backlog_v1.md](./hc_story_backlog_v1.md) — Implementation backlog and story decomposition.
- [hc_te_interface_spec_v1.md](./hc_te_interface_spec_v1.md) — HC-to-TE interface specification, including periodic STS reporting model.
- [hc_variable_registry_v1.md](./hc_variable_registry_v1.md) — Registry of named variable placeholders for deferred numeric values.
- [hc_verification_traceability_matrix_v1.md](./hc_verification_traceability_matrix_v1.md) — Traceability from requirements to verification intent and coverage.
- [tc_hc_jsonl_command_structure_preliminary_v1.md](./tc_hc_jsonl_command_structure_preliminary_v1.md) — Preliminary JSONL command structure for TC↔HC communications over USB VCP.
- [Index.md](./Index.md) — Top-level navigation file for the documentation set.

## Recommended Editing Order
1. Start here to locate the authoritative artifact.
2. Update the most specific source document first.
3. Update derived review or amalgamated documents only when needed.
4. Keep this index synchronized with the folder contents.

## Recommended Source-of-Truth Usage
- Use `hc_product_specification_review_v1.md` for offline human review of the current consolidated product definition.
- Use `hc_product_specification_amalgamated_v1.md` only when cross-checking source content in bulk.
- Use the specific source files for detailed edits in their own domains.
