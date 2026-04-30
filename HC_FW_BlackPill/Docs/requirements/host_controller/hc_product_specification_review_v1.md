# Radiation Test Host Controller (HC) Product Specification — Review Edition v1

## 1. Purpose
This document is the cleaned, unified offline review specification for the Radiation Test Host Controller (HC).

It consolidates the current HC specification set into a single non-duplicative product definition for review. It is intended to capture agreed product behavior, architecture constraints, TE-visible interface behavior, fault handling policy, recovery behavior, verification intent, and implementation scope.

This document is a review edition derived from the current source set and should be treated as the primary human-readable consolidated specification. Detailed supporting source documents remain authoritative for expanded rationale, traceability, and planning detail.

## 2. Source Basis
This review edition consolidates the following source artifacts:
- `/a0/usr/projects/bmad_test/docs/hc_prd_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_fault_response_matrix_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_variable_registry_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_architecture_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_te_interface_spec_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_protocol_test_plan_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_state_machine_spec_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_verification_traceability_matrix_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_story_backlog_v1.md`

## 3. Product Overview
The HC is the supervisory controller used during radiation testing to monitor and control two DUT-related channels and present supervisory status to the Test Executive (TE).

The HC shall:
- manage deterministic top-level operating state
- monitor DUT1 and DUT2 related conditions
- generate required control outputs such as DUT1 sync stimulus where applicable
- manage safe DUT power-enable behavior
- detect, latch, report, and manage faults and warnings
- support supervisory TE interaction over USB VCP
- provide sufficient status visibility for operation, recovery, and verification

The HC is safety- and test-integrity-oriented. It must localize DUT runtime faults by default, preserve unaffected DUT operation where allowed, and provide TE-visible recovery state without requiring high-rate polling.

## 4. Product Scope
### 4.1 In Scope
The current HC product scope includes:
- supervision of DUT1 (`LTC3901`) and DUT2 (`LT8316`)
- top-level HC state management
- DUT-local fault detection and response
- DUT-local recovery/restart handling
- DUT-local isolation handling
- DUT1 sync-related behavior and monitoring
- DUT1 and DUT2 power-enable control
- periodic supervisory reporting to TE
- TE command/response support over USB VCP
- debug visibility as separately defined in supporting documents
- 6-bit HC hardware ID exposure
- verification-oriented observability and traceability support

### 4.2 Out of Scope
The following are not part of the current HC product scope unless later added:
- beam control itself
- long-term data archival duties
- external workstation or calibration-station behavior
- TE-side UI or application behavior beyond interface expectations
- unresolved future command families not yet specified

## 5. Governing Product Policies
The following policy decisions are already agreed and normative for this review edition.

### 5.1 Fault Latching and Clearing
- Low-Level Faults (LLFs) are latched and reset-only.
- High-Level Faults (HLFs) are latched until explicitly cleared.
- HLF clearing is permitted during Beam On.
- Debug access may clear faults where allowed by the supporting specification.

### 5.2 Beam On Policy
- Beam On imposes no restrictions on allowable commands or operational modes.
- Fault-clear preconditions still apply where relevant.

### 5.3 Runtime DUT Fault Locality
- Runtime DUT faults shall isolate only the affected DUT by default.
- The unaffected DUT shall continue operating by default unless a specific fault policy requires broader shutdown.

### 5.4 USB / TE Link Policy
- USB VCP is the primary TE communications path.
- USB stack initialization failure is treated as a catastrophic boot/platform failure.
- USB absence or non-enumeration after successful initialization is degraded behavior only and is not itself an LLF.

### 5.5 Variable Policy
- Unresolved numeric thresholds, timings, debounce values, persistence values, envelopes, and qualification intervals shall not remain as free-form TBD numerics.
- Such values shall be represented using named variables.
- Variable naming and ownership shall align with the HC variable registry.

### 5.6 Reporting Policy
- The HC periodic `STS` message is the primary routine supervisory reporting path to the TE.
- In `NORMAL`, `STS` is emitted at a nominal 1 Hz rate on a best-effort basis.
- No further detailed cadence definition is required for v1.
- Immediate event emission is not required for DUT-local fault, restart, recovery, or isolation transitions.
- Periodic `STS` reporting is sufficient for TE visibility of these state changes.

## 6. Identity and Variable Model
### 6.1 HC Identity
The HC shall expose a 6-bit hardware ID derived from hardware inputs. This ID shall be available in supervisory reporting and debugging contexts.

### 6.2 Variable-Driven Product Definition
The HC uses a variable-driven specification model for unresolved numeric behavior. This applies to areas including, but not limited to:
- fault thresholds
- analog limits
- frequency limits
- debounce or persistence intervals
- restart delays
- restart attempt counts
- recovery qualification intervals
- publication filtering or averaging behavior

The single-source variable registry remains the control point for these unresolved numeric definitions.

## 7. Top-Level HC State Model
The HC top-level operating state model is:
- `BOOT`
- `LOW_LEVEL_FAULT`
- `HIGH_LEVEL_FAULT`
- `NORMAL`
- `SLAVE`

### 7.1 State Intent
- `BOOT`: startup, initialization, and validation before normal service
- `LOW_LEVEL_FAULT`: unrecoverable or reset-only low-level platform fault state
- `HIGH_LEVEL_FAULT`: high-level fault state as defined by the state-machine specification
- `NORMAL`: standard supervisory operating mode
- `SLAVE`: alternate supervisory mode as defined by the state-machine specification

### 7.2 Architectural Rule
Top-level state ownership is centralized. The firmware architecture is deterministic and avoids distributing top-level control ownership into interrupts.

## 8. DUT-Local Supervision Model
The HC supervises two DUT-local contexts:
- `LTC3901` (DUT1)
- `LT8316` (DUT2)

Each DUT has independent TE-visible status, DUT-local fault context, and DUT-local recovery behavior.

### 8.1 DUT-Local State Values
Each DUT-local `state` shall use one of:
- `NORMAL`
- `FAULT`
- `RECOVERED`
- `ISOLATED`

### 8.2 DUT-Local State Meanings
- `NORMAL`: the DUT is not isolated, no DUT-local fault is active, and the HC considers it to be operating within expected conditions.
- `FAULT`: one or more DUT-local faults have been detected and the HC is attempting to restart the device.
- `RECOVERED`: after restart due to `FAULT`, the DUT appears to be operating within `NORMAL` parameters again.
- `ISOLATED`: recovery failed and the DUT has been switched off or otherwise isolated to protect circuitry or test validity.

### 8.3 DUT-Local Transition Rules
The intended DUT-local transitions are:
- `NORMAL -> FAULT`
- `FAULT -> RECOVERED`
- `FAULT -> ISOLATED`
- `RECOVERED -> NORMAL`
- `RECOVERED -> FAULT`
- `NORMAL -> ISOLATED`
- `ISOLATED -> NORMAL`
- `ISOLATED -> FAULT`

These transitions are governed by DUT-local fault detection, restart behavior, qualification results, intentional isolation behavior, and supervisory re-enable policy.

## 9. Fault Model
### 9.1 Fault Classes
The HC fault model includes:
- Low-Level Faults (LLFs)
- High-Level Faults (HLFs)
- warnings

### 9.2 Fault Behavior Summary
- LLFs are latched and reset-only.
- HLFs are latched until explicitly cleared.
- Warnings are non-catastrophic degraded indications as defined by supporting documentation.
- Fault handling remains visible to TE through periodic status reporting and query mechanisms.

### 9.3 Fault Reporting Scope
The HC shall maintain sufficient fault context for TE visibility and diagnosis, including active-fault identity and DUT-local aggregation where applicable.

## 10. Recovery and Restart Policy
### 10.1 DUT-Local Recovery Scope
When a DUT-local fault drives a DUT into `FAULT`, the HC shall begin a DUT-local recovery attempt for the affected DUT only.

### 10.2 Recovery Control Variables
Recovery attempt count, restart timing, and qualification timing are variable-driven. Representative variables include:
- `VAR_HC_DUT_RESTART_MAX_ATTEMPTS`
- `VAR_HC_DUT_RESTART_DELAY_MS`
- `VAR_HC_DUT_RECOVERY_QUALIFY_TIME_MS`

### 10.3 Recovery Outcome Rules
- If restart succeeds and the DUT appears to operate within `NORMAL` parameters with no active DUT-local fault, the DUT-local `state` shall transition to `RECOVERED`.
- If the DUT remains faulted, immediately re-faults, or exceeds permitted restart attempts, the DUT shall transition to `ISOLATED`.
- `RECOVERED -> NORMAL` occurs only after the required qualification interval, if implemented.
- Retry or re-enable from `ISOLATED` occurs only by explicit supervisory action or other separately defined policy.
- Recovery actions shall not interrupt the unaffected DUT by default.

### 10.4 DUT Restart Action Sequence
The intended DUT restart sequence is:
1. detect DUT-local fault and set `state = FAULT`
2. record or update fault status and related evidence
3. de-assert affected DUT enable or power path as required
4. wait restart delay
5. re-assert affected DUT enable or power path
6. allow DUT startup or settle behavior
7. observe telemetry and fault logic during qualification
8. transition to `RECOVERED` on successful qualification
9. otherwise retry if permitted or transition to `ISOLATED`

### 10.5 DUT Restart Qualification Criteria
A restart attempt is successful only when all applicable conditions are satisfied for that DUT:
- no DUT-local fault remains active
- the DUT is not isolated
- DUT power-enable is asserted as required
- relevant DUT-local telemetry indicates expected operation using applicable variable-controlled limits and qualification logic
- no renewed DUT-local fault occurs during the qualification interval

## 11. TE Interface Model
### 11.1 Primary TE Communications Path
USB VCP is the primary TE communications link.

### 11.2 Protocol Direction
Final `GET_*` response details are intentionally deferred. This review edition focuses on the already-agreed periodic status path and related DUT-local behavior.

### 11.3 Commanding Philosophy
The HC supports supervisory control, status visibility, fault visibility, and controlled recovery workflows. Exact detailed command grammar and payloads remain deferred.

## 12. Periodic `STS` Reporting Model
### 12.1 Framing
`STS` uses JSONL framing: one JSON object per emitted line.

### 12.2 Top-Level `STS` Object Keys
The v1 `STS` object uses these top-level keys:
- `type`
- `version`
- `hc_id`
- `tsb`
- `state`
- `beam_on`
- `duts`

### 12.3 `duts` Object Shape
The `duts` object is keyed by DUT name. The required DUT keys are:
- `LTC3901`
- `LT8316`

Each DUT object shall be present in every emitted `STS` line, even if the DUT is powered off or faulted.

### 12.4 Required `LTC3901` Keys
The `duts.LTC3901` object includes:
- `state`
- `pwr_en`
- `sync`
- `vsupply`
- `vshunt`
- `isupply`
- `me_freq`
- `me_ratio`
- `me_anlg`
- `mf_freq`
- `mf_ratio`
- `mf_anlg`
- `faults`

### 12.5 Required `LT8316` Keys
The `duts.LT8316` object includes:
- `state`
- `pwr_en`
- `gate_freq`
- `gate_ratio`
- `gate_anlg`
- `vout`
- `faults`

### 12.6 DUT-Local `faults` Object Shape
Each DUT-local `faults` object uses:
- `count`
- `summary`
- `ids`

Where:
- `count` is the number of active DUT-local faults
- `summary` uses one of `NONE`, `SINGLE`, `MULTIPLE`
- `ids` is the array of active fault IDs for that DUT

## 13. Measurement Capture, Scaling, and Invalid Values
### 13.1 Engineering-Unit Reporting
Periodic `STS` measurement values shall be reported in engineering units. Raw ADC counts, timer counts, or other unscaled internal values shall not be used in periodic `STS`.

### 13.2 Units and Scaling
- `*_freq` fields are reported in `Hz`
- `*_anlg` fields are reported in `mV`
- `vsupply`, `vshunt`, and `vout` are reported in `mV`
- `isupply` is reported in `mA`
- `*_ratio` fields are reported over the range `0` to `100`

### 13.3 Field-Specific Interpretation
- `me_freq`, `mf_freq`, `gate_freq`: frequency-like measurements in `Hz`
- `me_anlg`, `mf_anlg`, `gate_anlg`, `vsupply`, `vshunt`, `vout`: analog-derived measurements in `mV`
- `me_ratio`, `mf_ratio`, `gate_ratio`: derived ratio measurements over `0` to `100`

### 13.4 Publication Behavior
- capture/scaling behavior shall be deterministic for a given firmware build
- any averaging, filtering, debounce, persistence, or qualification used before publication shall be controlled by named variables

### 13.5 Invalid / Unavailable Measurements
If a measurement is not meaningful because the DUT is powered off, isolated, restarting, or otherwise not in a valid measurement condition:
- the field shall still be emitted
- `null` is the preferred invalid or unavailable representation
- `-1` shall be used only if `null` cannot be accommodated by the protocol implementation

## 14. Deterministic Firmware Architecture
The HC firmware architecture is deterministic and centered on explicit ownership of state, fault handling, reporting, and hardware interaction.

The architectural model includes:
- centralized top-level state ownership
- explicit fault manager behavior
- periodic supervisory reporting support
- modular DUT1 and DUT2 supervision functions
- deterministic separation of ISR capture from cooperative/stateful control logic
- variable-driven measurement qualification and thresholds
- reporting support aligned with TE supervisory needs

## 15. Safety and Output Behavior
The HC favors safe behavior during fault handling.

The intended safety/output model is:
- platform-level faults drive safe behavior according to the top-level state machine and fault policy
- runtime DUT faults localize to the affected DUT by default
- unaffected DUT operation is preserved by default where allowed
- failed recovery causes DUT isolation
- DUT isolation is intended to protect circuitry and preserve test integrity

## 16. Verification Intent
The HC specification is intended to support structured verification of:
- top-level state behavior
- fault latching and clearing policy
- DUT-local fault detection and localization
- DUT-local restart and recovery behavior
- periodic `STS` visibility
- TE-visible DUT-local state changes through 1 Hz `STS`
- engineering-unit measurement reporting
- invalid-measurement handling
- implementation of variable-driven thresholds and qualification timing

Verification detail remains mapped more fully in the supporting traceability and protocol test documents.

## 17. Implementation Scope
The current implementation scope is organized around:
- platform and safe boot foundation
- top-level state and fault core
- TE communications and reporting
- DUT1 control and monitoring
- DUT2 control and monitoring
- diagnostics, logging, and verification support

This implementation scope is captured in more granular form in the story backlog, but is summarized here to keep product behavior and implementation decomposition aligned.

## 18. Open / Deferred Items
The following items are intentionally deferred or remain open at this review stage:
- exact `GET_*` payload schemas
- exact fault-detail response schema beyond current direction
- final command grammar and error-code table
- final numeric assignment of thresholds, delays, debounce values, and qualification intervals
- detailed build-time versus runtime configurability boundaries
- any future expansion of TE interface details beyond the agreed `STS` model

These open items do not invalidate the already-agreed product behavior captured in this review edition.

## 19. Review Summary
This review edition captures the current HC product definition as a single cleaned document.

The most important settled product behaviors are:
- deterministic HC supervisory architecture
- explicit top-level HC state model
- localized DUT runtime fault handling by default
- distinct LLF and HLF latching behavior
- DUT-local recovery flow through `FAULT`, `RECOVERED`, and `ISOLATED`
- periodic JSONL `STS` as the primary TE supervisory visibility path
- nominal 1 Hz best-effort `STS` cadence in `NORMAL`
- no immediate DUT-local event requirement
- engineering-unit periodic measurement reporting
- explicit invalid/unavailable measurement handling using `null`, or `-1` only if needed by implementation

This document is intended to be the primary offline review copy of the current HC product specification.
