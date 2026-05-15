# Host Controller Runtime Variable Registry v1

## Purpose

This registry records variables and configuration fields that are implemented in
the current firmware. Draft-only placeholders and obsolete `VAR_*` names have
been removed from this current registry.

Configuration is RAM-resident unless otherwise stated. Values are initialized
from compile-time defaults on reset and can be changed over USB VCP through JSON
`GET args` array fields and `SET args` object fields.

## Command-Visible Configuration

### LTC3901 Manager Configuration

Access:

- `GET args:["ltc3901"]`
- direct `SET args.ltc3901.<field>`

| JSON field | Firmware field | Default source | Default value | Units / meaning |
|---|---|---|---:|---|
| `isupply_ma_max` | `ltc3901_manager_config_t.isupply_ma_max` | `LTC3901_ISUPPLY_MAX_MA` | 50 | mA maximum supply current |
| `vupstream_mv_min` | `ltc3901_manager_config_t.vupstream_mv_min` | `LTC3901_VUPSTREAM_MIN_MV` | 10000 | mV minimum upstream supply |
| `ltc3901_vcc_mv_min` | `ltc3901_manager_config_t.ltc3901_vcc_mv_min` | `LTC3901_VCC_MIN_MV` | 10000 | mV minimum LTC3901 VCC sense |
| `power_up_timeout_ms` | `ltc3901_manager_config_t.power_up_timeout_ms` | `LTC3901_POWER_UP_TIMEOUT_MS` | 2000 | ms allowed for power qualification |
| `power_retry_delay_ms` | `ltc3901_manager_config_t.power_retry_delay_ms` | `LTC3901_POWER_RETRY_DELAY_MS` | 1000 | ms delay before retry from `POWER_FAULT` |
| `power_fault_max` | `ltc3901_manager_config_t.power_fault_max` | `LTC3901_POWER_FAULT_MAX` | 0 | retry-count limit; `0` means no limit |
| `sync_on_delay_ms` | `ltc3901_manager_config_t.sync_on_delay_ms` | `LTC3901_SYNC_ON_DELAY_MS` | 3000 | ms delay from `POWERED` to sync-on cycle |
| `sync_hold_on_time_ms` | `ltc3901_manager_config_t.sync_hold_on_time_ms` | `LTC3901_SYNC_HOLD_ON_TIME_MS` | 55000 | ms sync outputs remain enabled |
| `sync_hold_off_time_ms` | `ltc3901_manager_config_t.sync_hold_off_time_ms` | `LTC3901_SYNC_HOLD_OFF_TIME_MS` | 5000 | ms sync outputs remain disabled between cycles |
| `sync_stabilization_time_ms` | `ltc3901_manager_config_t.sync_stabilization_time_ms` | `LTC3901_SYNC_STABILIZATION_TIME_MS` | 100 | ms before ME/MF missing-activity checks |
| `sync_fault_delay_ms` | `ltc3901_manager_config_t.sync_fault_delay_ms` | `LTC3901_SYNC_FAULT_DELAY_MS` | 1000 | ms delay before retrying sync after sync fault |

Direct `SET args.ltc3901.<field>` values accept partial updates. The response
echoes the fields that were applied.

### LT8316 Manager Configuration

Access:

- `GET args:["lt8316"]`
- direct `SET args.lt8316.<field>`

| JSON field | Firmware field | Default source | Default value | Units / meaning |
|---|---|---|---:|---|
| `power_retry_delay_ms` | `lt8316_manager_config_t.power_retry_delay_ms` | `LT8316_POWER_RETRY_DELAY_MS` | 1000 | ms delay before retry from `FAULT` |
| `power_fault_max` | `lt8316_manager_config_t.power_fault_max` | `LT8316_POWER_FAULT_MAX` | 0 | retry-count limit; `0` means no limit |
| `power_on_stabilization_time_ms` | `lt8316_manager_config_t.power_on_stabilization_time_ms` | `LT8316_POWER_ON_STABILIZATION_TIME_MS` | 1000 | ms before missing-gate check |

Direct `SET args.lt8316.<field>` values accept partial updates. The response
echoes the fields that were applied.

## Reporting Intervals

| JSON field | Default | Units / meaning |
|---|---:|---|
| `sts_period_ms` | 1000 | Periodic `STS` interval; `0` disables periodic `STS`. |
| `dbg_period_ms` | 0 | Periodic `DBG` interval; `0` disables periodic `DBG`; nonzero values must be in the implemented valid range. |

## Debug Signal Configuration

`dbg_signals` is used to configure periodic debug telemetry. Settable digital
outputs use direct signal-name fields in `SET args`.

| Form | Purpose |
|---|---|
| `SET args.dbg_signals` array | Selects periodic debug telemetry signals for `DBG` output. |
| `SET args.<digital-signal-name>` boolean | Sets supported digital outputs. |
| `GET args:["dbg_signals"]` | Returns debug telemetry configuration or a requested signal sample. |

Supported digital output keys are:

- `ltc3901.pwr_en`
- `lt8316.pwr_en`
- `led.blue`
- `led.red`
- `led.green`

## ADC Calibration

ADC calibration is RAM-resident and command-visible through:

- direct `GET` and `SET` fields such as `adc.vupstream.slope_scaled`, `adc.vupstream.offset`, and `adc.vupstream.valid`

Raw ADC readback is available through:

- direct debug signal names such as `GET args:["adc.vupstream.raw"]`

Supported channel names include the names accepted by the command parser, such
as `ltc3901_vcc`, `lt8316_vout`, `ltc3901_me`, `ltc3901_mf`, and
`lt8316_gate`.

## Persistence

The current implementation does not persist runtime configuration or calibration
changes across reset or reprogramming.
