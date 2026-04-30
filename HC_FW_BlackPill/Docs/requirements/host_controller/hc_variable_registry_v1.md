# Radiation Test Host Controller (HC) Variable Registry v1

## 1. Document Purpose
This document defines the named variables used by the Radiation Test Host Controller (HC) requirements and fault logic when numeric values are not yet finalized.

These variables represent thresholds, limits, tolerances, timing values, debounce windows, update rates, and other experimentally derived constants whose numeric values will be assigned later.

This registry serves as the single source of truth for deferred numeric parameters referenced by:
- `/a0/usr/projects/bmad_test/docs/hc_prd_v1.md`
- `/a0/usr/projects/bmad_test/docs/hc_fault_response_matrix_v1.md`

## 2. Variable Policy
- Each unresolved numeric requirement shall be represented by a uniquely named variable.
- Variable names shall remain stable even if final numeric values change.
- Numeric values shall be assigned later from experiment, measurement, analysis, or datasheet review.
- Units shall be recorded explicitly.
- If a variable is derived from other variables, that relationship should be noted.
- Variables may later be mapped into firmware configuration headers, calibration tables, or nonvolatile parameter storage.

## 3. Variable Status Definitions
| Status | Meaning |
|---|---|
| Proposed | Variable name exists but may still be renamed or split |
| Defined | Variable name and purpose are stable, numeric value still TBD |
| Experiment Pending | Intended for lab characterization |
| Derived | To be calculated from other measured/defined values |
| Finalized | Numeric value approved for implementation |

## 4. Variable Registry
| Variable Name | Meaning | Units | Source | Applies To | Initial Value | Final Value | Status | Notes |
|---|---|---|---|---|---|---|---|---|
| `VAR_DUT1_SYNCA_FREQ` | DUT1 SyncA target frequency | Hz | Experiment / design intent | PRD 7.1 | TBD | TBD | Proposed | May be identical to SyncB frequency depending on architecture |
| `VAR_DUT1_SYNCB_FREQ` | DUT1 SyncB target frequency | Hz | Experiment / design intent | PRD 7.1 | TBD | TBD | Proposed | May collapse into one sync frequency variable later |
| `VAR_DUT1_SYNC_PHASE_TARGET` | DUT1 SyncA-to-SyncB phase target | degrees or time | Experiment / design intent | PRD 7.1 | TBD | TBD | Proposed | Unit to be finalized |
| `VAR_DUT1_SYNC_PHASE_TOLERANCE` | Allowed DUT1 sync phase error | degrees or time | Experiment / analysis | PRD 7.1 | TBD | TBD | Proposed | Unit to be finalized |
| `VAR_DUT1_ME_FREQ_MIN` | Minimum valid DUT1 ME gate frequency | Hz | Experiment | PRD 7.1, HLF-005 | TBD | TBD | Defined | Used with range checking |
| `VAR_DUT1_ME_FREQ_MAX` | Maximum valid DUT1 ME gate frequency | Hz | Experiment | PRD 7.1, HLF-005 | TBD | TBD | Defined | Used with range checking |
| `VAR_DUT1_ME_DUTY_MIN` | Minimum valid DUT1 ME duty cycle | % | Experiment | PRD 7.1, HLF-005 | TBD | TBD | Defined | Percent basis to be standardized |
| `VAR_DUT1_ME_DUTY_MAX` | Maximum valid DUT1 ME duty cycle | % | Experiment | PRD 7.1, HLF-005 | TBD | TBD | Defined | Percent basis to be standardized |
| `VAR_DUT1_MF_FREQ_MIN` | Minimum valid DUT1 MF gate frequency | Hz | Experiment | PRD 7.1, HLF-006 | TBD | TBD | Defined | Used with range checking |
| `VAR_DUT1_MF_FREQ_MAX` | Maximum valid DUT1 MF gate frequency | Hz | Experiment | PRD 7.1, HLF-006 | TBD | TBD | Defined | Used with range checking |
| `VAR_DUT1_MF_DUTY_MIN` | Minimum valid DUT1 MF duty cycle | % | Experiment | PRD 7.1, HLF-006 | TBD | TBD | Defined | Percent basis to be standardized |
| `VAR_DUT1_MF_DUTY_MAX` | Maximum valid DUT1 MF duty cycle | % | Experiment | PRD 7.1, HLF-006 | TBD | TBD | Defined | Percent basis to be standardized |
| `VAR_DUT1_ADC_SAMPLE_RATE` | Raw ADC sampling rate for DUT1 analog channels | samples/s | Firmware design / experiment | PRD 7.1 | TBD | TBD | Proposed | May differ from reporting/update rate |
| `VAR_DUT1_ADC_UPDATE_RATE` | Processed update rate for DUT1 analog measurements | Hz | Firmware design / experiment | PRD 7.1 | TBD | TBD | Proposed | Could be task rate rather than ADC hardware rate |
| `VAR_DUT1_OVERCURRENT_LIMIT` | DUT1 input overcurrent trip threshold | A | Experiment | PRD 7.1, 7.4, HLF-001 | TBD | TBD | Defined | Derived from shunt measurement scaling |
| `VAR_DUT1_OVERCURRENT_DEBOUNCE` | Persistence/debounce before asserting DUT1 overcurrent fault | ms or samples | Experiment | PRD 7.1, 7.4, HLF-001 | TBD | TBD | Defined | Unit to be finalized |
| `VAR_DUT1_CURRENT_SENSE_INVALID_DEBOUNCE` | Persistence before flagging DUT1 current-sense invalid condition | ms or samples | Experiment | HLF-002 | TBD | TBD | Proposed | Plausibility-rule variables may also be needed |
| `VAR_DUT1_ME_ACTIVITY_TIMEOUT` | Timeout for missing DUT1 ME activity | us or ms | Experiment | HLF-003 | TBD | TBD | Defined | Unit depends on expected switching rate |
| `VAR_DUT1_MF_ACTIVITY_TIMEOUT` | Timeout for missing DUT1 MF activity | us or ms | Experiment | HLF-004 | TBD | TBD | Defined | Unit depends on expected switching rate |
| `VAR_DUT1_ME_RANGE_DEBOUNCE` | Persistence before asserting DUT1 ME out-of-range fault | ms or samples | Experiment | HLF-005 | TBD | TBD | Defined | Applies to ME frequency/duty range checks |
| `VAR_DUT1_MF_RANGE_DEBOUNCE` | Persistence before asserting DUT1 MF out-of-range fault | ms or samples | Experiment | HLF-006 | TBD | TBD | Defined | Applies to MF frequency/duty range checks |
| `VAR_DUT1_FILTERED_ME_MIN` | Minimum valid filtered DUT1 ME analog level | V or ADC counts | Experiment | HLF-007 | TBD | TBD | Proposed | Unit to be standardized |
| `VAR_DUT1_FILTERED_ME_MAX` | Maximum valid filtered DUT1 ME analog level | V or ADC counts | Experiment | HLF-007 | TBD | TBD | Proposed | Unit to be standardized |
| `VAR_DUT1_FILTERED_ME_RANGE_DEBOUNCE` | Persistence before asserting filtered DUT1 ME analog-range anomaly | ms or samples | Experiment | HLF-007 | TBD | TBD | Proposed | May become WRN-only depending on policy |
| `VAR_DUT1_FILTERED_MF_MIN` | Minimum valid filtered DUT1 MF analog level | V or ADC counts | Experiment | HLF-008 | TBD | TBD | Proposed | Unit to be standardized |
| `VAR_DUT1_FILTERED_MF_MAX` | Maximum valid filtered DUT1 MF analog level | V or ADC counts | Experiment | HLF-008 | TBD | TBD | Proposed | Unit to be standardized |
| `VAR_DUT1_FILTERED_MF_RANGE_DEBOUNCE` | Persistence before asserting filtered DUT1 MF analog-range anomaly | ms or samples | Experiment | HLF-008 | TBD | TBD | Proposed | May become WRN-only depending on policy |
| `VAR_DUT1_SYNC_FAILURE_DEBOUNCE` | Persistence before declaring DUT1 sync generation failure | ms or samples | Firmware design / experiment | HLF-009 | TBD | TBD | Proposed | Optional if supervision is immediate |
| `VAR_DUT1_UNEXPECTED_ACTIVITY_DEBOUNCE` | Persistence before flagging unexpected DUT1 activity while power disabled | ms or samples | Experiment | HLF-010 | TBD | TBD | Proposed | Helps suppress transient artifacts |
| `VAR_DUT2_VOUT_LOW_LIMIT` | Minimum valid DUT2 output voltage | V | Experiment | PRD 7.2, HLF-012 | TBD | TBD | Defined | Fault or warning classification still policy-dependent |
| `VAR_DUT2_VOUT_HIGH_LIMIT` | Maximum valid DUT2 output voltage | V | Experiment | PRD 7.2, HLF-011 | TBD | TBD | Defined | Safety-critical threshold |
| `VAR_DUT2_GATE_FREQ_MIN` | Minimum valid DUT2 gate frequency | Hz | Experiment | PRD 7.2, HLF-014 | TBD | TBD | Defined | Used with range checking |
| `VAR_DUT2_GATE_FREQ_MAX` | Maximum valid DUT2 gate frequency | Hz | Experiment | PRD 7.2, HLF-014 | TBD | TBD | Defined | Used with range checking |
| `VAR_DUT2_GATE_DUTY_MIN` | Minimum valid DUT2 gate duty cycle | % | Experiment | PRD 7.2, HLF-014 | TBD | TBD | Defined | Percent basis to be standardized |
| `VAR_DUT2_GATE_DUTY_MAX` | Maximum valid DUT2 gate duty cycle | % | Experiment | PRD 7.2, HLF-014 | TBD | TBD | Defined | Percent basis to be standardized |
| `VAR_DUT2_GATE_ACTIVITY_TIMEOUT` | Timeout for missing DUT2 gate activity | us or ms | Experiment | PRD 7.2, HLF-013 | TBD | TBD | Defined | Unit depends on expected switching rate |
| `VAR_DUT2_ADC_SAMPLE_RATE` | Raw ADC sampling rate for DUT2 analog channels | samples/s | Firmware design / experiment | PRD 7.2 | TBD | TBD | Proposed | May differ from processed update rate |
| `VAR_DUT2_ADC_UPDATE_RATE` | Processed update rate for DUT2 analog measurements | Hz | Firmware design / experiment | PRD 7.2 | TBD | TBD | Proposed | Could be task rate rather than ADC hardware rate |
| `VAR_DUT2_VOUT_HIGH_DEBOUNCE` | Persistence before asserting DUT2 overvoltage fault | ms or samples | Experiment | HLF-011 | TBD | TBD | Defined | Unit to be finalized |
| `VAR_DUT2_VOUT_LOW_DEBOUNCE` | Persistence before asserting DUT2 undervoltage anomaly | ms or samples | Experiment | HLF-012 | TBD | TBD | Defined | Unit to be finalized |
| `VAR_DUT2_GATE_RANGE_DEBOUNCE` | Persistence before asserting DUT2 gate out-of-range fault | ms or samples | Experiment | HLF-014 | TBD | TBD | Defined | Applies to frequency/duty range checks |
| `VAR_HV_ENABLE_FEEDBACK_MISMATCH_DEBOUNCE` | Persistence before asserting HV enable feedback mismatch | ms or samples | Experiment / design | HLF-015 | TBD | TBD | Proposed | Requires feedback mechanism definition |
| `VAR_DUT1_ENABLE_FEEDBACK_MISMATCH_DEBOUNCE` | Persistence before asserting DUT1 power-enable feedback mismatch | ms or samples | Experiment / design | HLF-016 | TBD | TBD | Proposed | Requires feedback mechanism definition |
| `VAR_MEASUREMENT_ACQUISITION_STALL_TIMEOUT` | Timeout for stalled ADC/capture acquisition | ms | Firmware design / experiment | HLF-019 | TBD | TBD | Defined | May be driven by scheduler/update-rate architecture |

## 5. Additional Variable Placeholders Still Likely Needed
The following variable groups are anticipated but not fully defined yet:
- Current-sense plausibility limits for DUT1
- Unexpected-activity detection thresholds for DUT1 when power is disabled
- Feedback validation thresholds for HV and DUT1 power-enable paths
- LED flash-code timing variables
- TE periodic status report interval variables
- Fault recovery dwell-time variables if separate from debounce variables

## 6. Recommended Next Use of This Registry
This registry should next be used to:
1. align all remaining PRD `TBD` numeric items to named variables
2. reference variables directly in firmware architecture and story definitions
3. define which variables belong in build-time constants versus runtime-configurable parameters
4. support future lab characterization and traceability from experiment to firmware

## 7. Revision Notes
- v1: Initial HC variable registry created from PRD v1 and Fault Response Matrix v1.4.
