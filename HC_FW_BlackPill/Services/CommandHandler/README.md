# CommandHandler

This folder contains the Host Controller JSONL command processor implementation scaffolding.

## Current scope

Initial vertical slice only:
- parse TE `SET` packet
- validate numeric `msg`
- validate `args.date_time`
- validate `args.sw_version`
- validate `args.sts_period_ms`
- validate `args.dbg_period_ms`
- validate `args.dbg_signals`
- validate `args.adc_cal`
- validate `args.ltc3901_cmd` and `args.lt8316_cmd`
- validate `args.ltc3901_cfg` and `args.lt8316_cfg`
- store current HC date/time value
- store periodic `STS` interval in milliseconds
- store periodic `DBG` interval and selected debug signal subset
- store per-channel ADC engineering-unit calibration factors
- forward LTC3901 `RUN`, `HALT`, and `RESET` commands to the application manager facade
- forward LT8316 `RUN` and `RESET` commands to the application manager facade
- update and return runtime DUT manager configuration values
- apply all supported fields present in one `SET args` object and return one combined `RSP args` object
- build `RSP` success/error packets and asynchronous `EVT` packets
- populate `RSP.hc` from the same application status HC ID used by periodic `STS.hc_id`

## Planned files

- `hc_cmd_types.h` — shared enums, structs, and constants
- `hc_jsonl_cmd.h/.c` — top-level command processor entry point
- `hc_jsonl_parse.h/.c` — JSON token lookup and extraction helpers using `jsmn`
- `hc_jsonl_dispatch.h/.c` — packet dispatch logic
- `hc_jsonl_fields.h/.c` — field-specific handlers, starting with `date_time`
- `hc_jsonl_rsp.h/.c` — response and event formatting helpers
- `hc_datetime.h/.c` — current HC date/time storage and validation
- `hc_comms_tx.h/.c` — transport abstraction for line-based TX

## Current integration status

These files are integrated into the USB RX/TX path and application loop for the currently supported `SET`/`GET` fields.

## Test Commands
```jsonl
{"type":"SET","msg":0,"args":{"date_time":"20260501 14:57:09"}}
{"type":"SET","msg":1,"args":{"sts_period_ms":250}}
{"type":"SET","msg":2,"args":{"sts_period_ms":0}}
{"type":"SET","msg":3,"args":{"dbg_period_ms":100,"dbg_signals":["adc.ltc3901_me.raw","adc.ltc3901_me.mv","pwm.me.freq_hz","pwm.me.duty_pct"]}}
{"type":"GET","msg":4,"args":{"sw_version":true}}
{"type":"GET","msg":5,"args":{"dbg_period_ms":true}}
{"type":"GET","msg":7,"args":{"dbg_signals":["adc.vupstream.raw","adc.vupstream.eng"]}}
{"type":"SET","msg":5,"args":{"adc_cal":{"channel":3,"slope_scaled":2500,"offset":0,"valid":true}}}
{"type":"SET","msg":6,"args":{"adc_cal":{"channel":"vupstream","slope_scaled":2500,"offset":0,"valid":true}}}
{"type":"GET","msg":7,"args":{"adc_cal":3}}
{"type":"GET","msg":8,"args":{"adc_cal":"vupstream"}}
{"type":"SET","msg":9,"args":{"ltc3901_cmd":"RUN"}}
{"type":"SET","msg":10,"args":{"ltc3901_cmd":"HALT"}}
{"type":"SET","msg":11,"args":{"ltc3901_cmd":"RESET"}}
{"type":"SET","msg":12,"args":{"lt8316_cmd":"RUN"}}
{"type":"SET","msg":13,"args":{"lt8316_cmd":"RESET"}}
{"type":"GET","msg":14,"args":{"ltc3901_cfg":true}}
{"type":"SET","msg":15,"args":{"ltc3901_cfg":{"isupply_ma_max":50,"power_up_timeout_ms":2000}}}
{"type":"GET","msg":16,"args":{"lt8316_cfg":true}}
{"type":"SET","msg":17,"args":{"lt8316_cfg":{"power_retry_delay_ms":1000,"power_fault_max":3}}}
{"type":"SET","msg":0,"args":{"date_time":"20260501 14:57:09","Something":"Does this work"}}
```

## ADC Calibration Channel Names

`adc_cal.channel` for `SET`, and `args.adc_cal` for `GET`, may be either a numeric ADC channel index or one of these names:

- `vupstream`
- `ltc3901_vcc`
- `lt8316_vout`
- `ltc3901_me`
- `ltc3901_mf`
- `lt8316_gate`
- `temp`
- `vrefint`
