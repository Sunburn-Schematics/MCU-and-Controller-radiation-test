# JSON Tests

## Purpose

This document provides simple test vectors for the current first-slice HC command processor implementation.

Current implemented scope:
- command processor frames one complete top-level JSON object from the USB byte stream
- `CommandHandler` supports `SET` and `GET`
- `SET` currently supports `args.date_time`, `args.sts_period_ms`, `args.dbg_period_ms`, `args.dbg_signals`, `args.adc_cal`, `args.ltc3901_cmd`, `args.lt8316_cmd`, `args.ltc3901_cfg`, and `args.lt8316_cfg`
- `SET` also supports `args.dbg_signals` as an object for setting digital signal values (power enables and LEDs)
- a `SET` request may include multiple supported fields; all valid fields are applied and the response contains one combined `args` object
- `GET` currently supports `args.date_time`, `args.sw_version`, `args.raw_adc`, `args.dbg_period_ms`, `args.dbg_signals`, `args.adc_cal`, `args.ltc3901_cfg`, and `args.lt8316_cfg`
- timestamps use seconds-only format: `YYYYMMDD HH:MM:SS`
- `RSP.hc`, `EVT.hc`, and periodic `STS.hc_id` use the same HC identifier sourced from board status
- periodic `STS` transmission is emitted by `fw_app_run()` at a configurable millisecond interval
- `args.sts_period_ms = 0` disables periodic `STS` transmission
- periodic `DBG` transmission is emitted by `fw_app_run()` at a configurable millisecond interval
- `args.dbg_period_ms = 0` disables periodic `DBG` transmission
- asynchronous `EVT` messages are emitted when application managers report notable state-machine events

---

## Valid JSON examples

### 1. Valid `SET date_time`

Request:
```json
{"type":"SET","msg":0,"args":{"date_time":"20260501 10:30:00"}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":0,"ts":"20260501 10:30:00","args":{"date_time":"20260501 10:30:00"}}
```

Notes:
- this is the primary supported happy-path example
- `ts` is expected to reflect the current HC RTC-backed time after the set succeeds

---

### 2. Another valid `SET date_time`

Request:
```json
{"type":"SET","msg":17,"args":{"date_time":"20261231 23:59:59"}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":17,"ts":"20261231 23:59:59","args":{"date_time":"20261231 23:59:59"}}
```

Notes:
- confirms that non-zero numeric `msg` values are accepted and mirrored in the response

---

### 3. Valid `GET date_time`

Request:
```json
{"type":"GET","msg":1,"args":{"date_time":true}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":1,"ts":"20260501 10:30:00","args":{"date_time":"20260501 10:30:00"}}
```

Notes:
- this is the primary `GET` happy-path example
- `args.date_time` is currently a boolean selector for `GET`
- returned `args.date_time` should match the current HC RTC-backed time

---

### 3a. Valid `GET sw_version`

Request:
```json
{"type":"GET","msg":2,"args":{"sw_version":true}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":2,"ts":"20260501 10:30:00","args":{"sw_version":"0.1.0"}}
```

Notes:
- `args.sw_version` is currently a boolean selector for `GET`
- returned `args.sw_version` is compiled into the firmware using `SW_VERSION_STRING`

---

### 4. Valid `SET sts_period_ms`

Request:
```json
{"type":"SET","msg":18,"args":{"sts_period_ms":250}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":18,"ts":"20260501 10:30:00","args":{"sts_period_ms":250}}
```

Notes:
- this sets the nominal periodic `STS` transmit interval to 250 ms
- the updated interval applies to subsequent periodic status emission

---

### 5. Valid `SET sts_period_ms` disable

Request:
```json
{"type":"SET","msg":19,"args":{"sts_period_ms":0}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":19,"ts":"20260501 10:30:00","args":{"sts_period_ms":0}}
```

Notes:
- a value of `0` disables periodic `STS` transmission

---

### 6. Valid JSON object without trailing newline

Request:
```json
{"type":"SET","msg":5,"args":{"date_time":"20260115 08:45:12"}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":5,"ts":"20260115 08:45:12","args":{"date_time":"20260115 08:45:12"}}
```

Notes:
- this validates that framing is based on a complete top-level JSON object, not newline termination

---

### 7. Valid `SET dbg config`

Request:
```json
{"type":"SET","msg":21,"args":{"dbg_period_ms":100,"dbg_signals":["adc.ltc3901_me.raw","adc.ltc3901_me.mv","pwm.me.freq_hz","pwm.me.duty_pct"]}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":21,"ts":"20260501 10:30:00","args":{"dbg_period_ms":100,"dbg_signals":["adc.ltc3901_me.raw","adc.ltc3901_me.mv","pwm.me.freq_hz","pwm.me.duty_pct"]}}
```

Notes:
- this enables periodic `DBG` output at 10 Hz
- selected signals are emitted in the requested order after duplicate removal

---

### 8. Valid `GET dbg config`

Request:
```json
{"type":"GET","msg":22,"args":{"dbg_period_ms":true}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":22,"ts":"20260501 10:30:00","args":{"dbg_period_ms":100,"dbg_signals":["adc.ltc3901_me.raw","adc.ltc3901_me.mv","pwm.me.freq_hz","pwm.me.duty_pct"]}}
```

Notes:
- `GET` returns both `dbg_period_ms` and `dbg_signals` even if only one selector key is present
- `args.dbg_signals:true` is also accepted as the selector

---

### 9. Valid `GET dbg signal sample`

Request:
```json
{"type":"GET","msg":23,"args":{"dbg_signals":["adc.vupstream.raw","adc.vupstream.eng"]}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":23,"ts":"20260501 10:30:00","args":{"dbg_signals":{"adc.vupstream.raw":2048,"adc.vupstream.eng":5120}}}
```

Notes:
- this is a one-shot sampled read, not a subscription change
- invalid requested signals are returned as `null`
- this does not modify the periodic `DBG` publisher configuration

---

### 10. Valid `SET adc_cal`

Request:
```json
{"type":"SET","msg":23,"args":{"adc_cal":{"channel":3,"slope_scaled":2500,"offset":-100,"valid":true}}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":23,"ts":"20260501 10:30:00","args":{"adc_cal":{"channel":3,"slope_scaled":2500,"offset":-100,"valid":true}}}
```

Notes:
- this configures the engineering-unit conversion for ADC channel `3`
- conversion is `y = ((slope_scaled * raw_counts) / 1000000) + offset`

---

### 11. Valid `SET digital signals`

Request:
```json
{"type":"SET","msg":24,"args":{"dbg_signals":{"ltc3901.pwr_en":true,"lt8316.pwr_en":false,"led.blue":true,"led.red":false,"led.green":true}}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":24,"ts":"20260501 10:30:00","args":{"dbg_signals":{"ltc3901.pwr_en":true,"lt8316.pwr_en":false,"led.blue":true,"led.red":false,"led.green":true}}}
```

Notes:
- this requests LTC3901 manager power-up, requests LT8316 manager reset, and controls the LED states
- settable digital signals include `ltc3901.pwr_en`, `lt8316.pwr_en`, `led.blue`, `led.red`, and `led.green`
- `sync.enable` is read-only because the LTC3901 manager owns the SYNC output state
- attempting to set other signals will result in an error

---

### 12. Valid `SET LTC3901 command`

Request:
```json
{"type":"SET","msg":27,"args":{"ltc3901_cmd":"RUN"}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":27,"ts":"20260501 10:30:00","args":{"ltc3901_cmd":"RUN"}}
```

Notes:
- valid command values are `RUN`, `HALT`, and `RESET`
- `RUN` requests transition to `POWER_UP` and normal manager flow
- `HALT` requests transition to `HALT`, which disables LTC3901 power and sync without clearing manager fault counters
- `RESET` requests transition to `RESET`, which disables LTC3901 power and sync and clears manager fault counters

---

### 13. Valid `SET LT8316 command`

Request:
```json
{"type":"SET","msg":28,"args":{"lt8316_cmd":"RUN"}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":28,"ts":"20260501 10:30:00","args":{"lt8316_cmd":"RUN"}}
```

Notes:
- valid command values are `RUN` and `RESET`
- `RUN` requests transition to `POWERED`
- `RESET` requests transition to `RESET`, which disables LT8316 HV power and clears manager fault counters

---

### 14. Valid combined manager command

Request:
```json
{"type":"SET","msg":124,"args":{"ltc3901_cmd":"RUN","lt8316_cmd":"RUN"}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":124,"ts":"20260501 10:30:00","args":{"ltc3901_cmd":"RUN","lt8316_cmd":"RUN"}}
```

Notes:
- both manager commands are applied from the same SET request
- the command receives one combined `RSP`, not two separate `RSP` records
- asynchronous `EVT` records may also be emitted later as each manager advances its state machine

---

### 15. Valid `GET LTC3901 manager config`

Request:
```json
{"type":"GET","msg":126,"args":{"ltc3901_cfg":true}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":126,"ts":"20260501 10:30:00","args":{"ltc3901_cfg":{"isupply_ma_max":50,"vupstream_mv_min":10000,"ltc3901_vcc_mv_min":10000,"power_up_timeout_ms":2000,"power_retry_delay_ms":1000,"power_fault_max":3,"sync_on_delay_ms":1000,"sync_hold_on_time_ms":10000,"sync_hold_off_time_ms":2000,"sync_stabilization_time_ms":100,"sync_fault_delay_ms":1000}}}
```

Notes:
- `args.ltc3901_cfg` is a boolean selector for `GET`
- all LTC3901 manager runtime config fields are returned

---

### 16. Valid `SET LTC3901 manager config`

Request:
```json
{"type":"SET","msg":127,"args":{"ltc3901_cfg":{"isupply_ma_max":75,"power_up_timeout_ms":2500}}}
```

Expected response shape:
```json
{"type":"RSP","hc":1,"msg":127,"ts":"20260501 10:30:00","args":{"ltc3901_cfg":{"isupply_ma_max":75,"vupstream_mv_min":10000,"ltc3901_vcc_mv_min":10000,"power_up_timeout_ms":2500,"power_retry_delay_ms":1000,"power_fault_max":3,"sync_on_delay_ms":1000,"sync_hold_on_time_ms":10000,"sync_hold_off_time_ms":2000,"sync_stabilization_time_ms":100,"sync_fault_delay_ms":1000}}}
```

Notes:
- `SET args.ltc3901_cfg` accepts partial updates
- the response echoes the resulting full runtime config

---

### 17. Valid `GET LT8316 manager config`

Request:
```json
{"type":"GET","msg":128,"args":{"lt8316_cfg":true}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":128,"ts":"20260501 10:30:00","args":{"lt8316_cfg":{"power_retry_delay_ms":1000,"power_fault_max":3,"power_on_stabilization_time_ms":1000}}}
```

Notes:
- `args.lt8316_cfg` is a boolean selector for `GET`
- all LT8316 manager runtime config fields are returned

---

### 18. Valid `SET LT8316 manager config`

Request:
```json
{"type":"SET","msg":129,"args":{"lt8316_cfg":{"power_retry_delay_ms":1500,"power_fault_max":4}}}
```

Expected response shape:
```json
{"type":"RSP","hc":1,"msg":129,"ts":"20260501 10:30:00","args":{"lt8316_cfg":{"power_retry_delay_ms":1500,"power_fault_max":4,"power_on_stabilization_time_ms":1000}}}
```

Notes:
- `SET args.lt8316_cfg` accepts partial updates
- the response echoes the resulting full runtime config

---

### 19. Valid combined `SET` command

Request:
```json
{"type":"SET","msg":125,"args":{"sts_period_ms":0,"dbg_period_ms":1000,"ltc3901_cmd":"RUN","lt8316_cmd":"RUN"}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":125,"ts":"20260501 10:30:00","args":{"sts_period_ms":0,"dbg_period_ms":1000,"ltc3901_cmd":"RUN","lt8316_cmd":"RUN"}}
```

Notes:
- all supplied SET fields are applied from the same request
- the command receives one combined `RSP`, not one `RSP` per field
- if any supplied field is invalid, the command returns an error instead of silently skipping that field

---

### 20. Valid `GET adc_cal`

Request:
```json
{"type":"GET","msg":24,"args":{"adc_cal":3}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":24,"ts":"20260501 10:30:00","args":{"adc_cal":{"channel":3,"slope_scaled":2500,"offset":-100,"valid":true}}}
```

---

### 21. Valid `SET adc_cal` with channel name

Request:
```json
{"type":"SET","msg":25,"args":{"adc_cal":{"channel":"vupstream","slope_scaled":2500,"offset":-100,"valid":true}}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":25,"ts":"20260501 10:30:00","args":{"adc_cal":{"channel":0,"slope_scaled":2500,"offset":-100,"valid":true}}}
```

Notes:
- channel names are accepted as an alternative to numeric ADC channel indices
- the response continues to report the resolved numeric channel index

---

### 22. Valid `GET adc_cal` with channel name

Request:
```json
{"type":"GET","msg":26,"args":{"adc_cal":"vupstream"}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":26,"ts":"20260501 10:30:00","args":{"adc_cal":{"channel":0,"slope_scaled":2500,"offset":-100,"valid":true}}}
```

---

### 23. Valid JSON preceded or followed by non-JSON noise bytes

Example input stream:
```text
junk before{"type":"SET","msg":9,"args":{"date_time":"20260520 14:22:33"}}junk after
```

Expected behavior:
- bytes before the first `{` are ignored
- the first complete top-level JSON object is framed and processed

Expected response:
```json
{"type":"RSP","hc":1,"msg":9,"ts":"20260520 14:22:33","args":{"date_time":"20260520 14:22:33"}}
```

Notes:
- this validates the command processor object-framing behavior
- trailing non-JSON bytes should be ignored until the next `{`

---

## Valid HC periodic `STS` example

Example emitted line:
```json
{"type":"STS","hc_id":1,"ts":"20260501 10:30:00","state":"NORMAL","beam_on":false,"duts":{"LTC3901":{"state":"RESET","pwr_en":false,"sync":false,"vsupply":null,"vshunt":null,"isupply":null,"me_freq":null,"me_ratio":null,"me_anlg":null,"mf_freq":null,"mf_ratio":null,"mf_anlg":null},"LT8316":{"state":"RESET","pwr_en":false,"gate_freq":null,"gate_anlg":null,"vout":null}}}
```

Notes:
- `STS` is HC-originated and is not a TE request/response transaction
- `ts` uses the HC RTC-backed timestamp format `YYYYMMDD HH:MM:SS`
- DUT `state` values use the active DUT manager state names.
- fault details are emitted through asynchronous `EVT` messages rather than the periodic `STS` payload.
- unavailable measurements use `null`

---

## Valid HC asynchronous `EVT` example

Example emitted line:
```json
{"type":"EVT","hc":1,"ts":"20260501 10:30:00","args":{"msg":"LTC3901: Entering POWER_UP"}}
```

Notes:
- `EVT` is HC-originated and is not a TE request/response transaction.
- `EVT` does not include a request `msg` field because it is asynchronous.
- The current first-slice event payload is `args.msg`, and manager-generated messages start with the related device name.
- Manager state transitions may emit EVT messages such as `LTC3901: Entering POWER_UP`, `LTC3901: Powered`, or `LTC3901: Cycle Sync ON`.
- Fault EVT messages include the measured value and the comparison value where available.
- Retry EVT messages include retry count and maximum retry count.

Fault/retry examples:

```json
{"type":"EVT","hc":1,"ts":"20260501 10:30:00","args":{"msg":"LTC3901: Isupply Current too high: measured 123 mA >= limit 100 mA"}}
```

```json
{"type":"EVT","hc":1,"ts":"20260501 10:30:00","args":{"msg":"LTC3901: VUpstream Too Low: measured 2750 mV < minimum 3000 mV"}}
```

```json
{"type":"EVT","hc":1,"ts":"20260501 10:30:00","args":{"msg":"LTC3901: Power Up Timeout (2000ms): VUpstream (mV) 11116 < 12000; VCC (mV) 11045 < 12000"}}
```

```json
{"type":"EVT","hc":1,"ts":"20260501 10:30:00","args":{"msg":"LT8316: GATE Stopped: measured null Hz, required valid frequency after 1000 ms; elapsed 1010 ms"}}
```

```json
{"type":"EVT","hc":1,"ts":"20260501 10:30:00","args":{"msg":"LTC3901: Retrying 1/3 Power Up"}}
```

---

## Valid JSON but invalid-command examples

These are valid JSON syntactically, but should produce an error response because they are unsupported or invalid for the current implementation.

### 24. Unsupported packet type `EXC`

Request:
```json
{"type":"EXC","msg":1,"args":{}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":1,"ts":"<current_hc_time>","error":{"code":"NOT_SUPPORTED","message":"Packet type not implemented yet"}}
```

Notes:
- exact error message text may vary
- `ts` should be the current HC time at response generation time

---

### 25. Missing supported field in `SET`

Request:
```json
{"type":"SET","msg":2,"args":{}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":2,"ts":"<current_hc_time>","error":{"code":"BAD_FIELD","message":"SET currently supports args.date_time, args.sts_period_ms, dbg_period_ms, dbg_signals, adc_cal, ltc3901_cmd, lt8316_cmd, ltc3901_cfg, or lt8316_cfg"}}
```

---

### 26. `args.date_time` wrong format in `SET`

Request:
```json
{"type":"SET","msg":3,"args":{"date_time":"2026-05-01T10:30:00"}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":3,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"args.date_time must match YYYYMMDD HH:MM:SS"}}
```

Notes:
- current expected format is `YYYYMMDD HH:MM:SS`
- ISO-8601 style strings should currently be rejected

---

### 27. Invalid calendar/time value in `SET`

Request:
```json
{"type":"SET","msg":4,"args":{"date_time":"20260230 10:30:00"}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":4,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"args.date_time contains an invalid calendar/time value"}}
```

---

### 28. Invalid `args.sts_period_ms` type in `SET`

Request:
```json
{"type":"SET","msg":20,"args":{"sts_period_ms":true}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":20,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"args.sts_period_ms must be a non-negative integer"}}
```

---

### 29. `GET` missing `args`

Request:
```json
{"type":"GET","msg":5}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":5,"ts":"<current_hc_time>","error":{"code":"BAD_ARGS","message":"GET requires an args object"}}
```

---

### 30. `GET` missing supported field

Request:
```json
{"type":"GET","msg":6,"args":{}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":6,"ts":"<current_hc_time>","error":{"code":"BAD_FIELD","message":"GET currently supports args.date_time, args.sw_version, args.raw_adc, dbg_period_ms, dbg_signals, adc_cal, ltc3901_cfg, or lt8316_cfg"}}
```

---

### 31. Invalid `SET dbg_period_ms`

Request:
```json
{"type":"SET","msg":23,"args":{"dbg_period_ms":50}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":23,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"args.dbg_period_ms must be 0 or 100..60000, and args.dbg_signals must be a valid signal array"}}
```

---

### 32. Invalid `SET dbg_signals`

Request:
```json
{"type":"SET","msg":24,"args":{"dbg_signals":["pwm.me.freq_hz","not.a.real.signal"]}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":24,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"args.dbg_period_ms must be 0 or 100..60000, and args.dbg_signals must be a valid signal array"}}
```

---

### 33. Invalid `SET adc_cal`

Request:
```json
{"type":"SET","msg":25,"args":{"adc_cal":{"channel":99,"slope_scaled":2500,"offset":0,"valid":true}}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":25,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"args.adc_cal.channel is out of range"}}
```

---

### 34. Invalid `GET dbg_signals`

Request:
```json
{"type":"GET","msg":26,"args":{"dbg_signals":["adc.vupstream.raw","not.a.real.signal"]}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":26,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"GET args.dbg_period_ms must be true, and args.dbg_signals must be true or a valid signal array"}}
```

---

### 35. `GET args.date_time` not `true`

Request:
```json
{"type":"GET","msg":7,"args":{"date_time":false}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":7,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"GET args.date_time must be true"}}
```

---

### 36. Non-numeric `msg`

Request:
```json
{"type":"SET","msg":"abc","args":{"date_time":"20260501 10:30:00"}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"ts":"<current_hc_time>","error":{"code":"BAD_ARGS","message":"Invalid msg"}}
```

Notes:
- because `msg` was not successfully parsed, the response may omit `msg`

---

## Invalid JSON examples for command_processor and parser validation

These inputs should be used to validate JSON framing, malformed-object handling, and parser robustness.

### 37. Missing closing brace

Request:
```json
{"type":"SET","msg":8,"args":{"date_time":"20260501 10:30:00"}
```

Expected behavior:
- command processor should continue waiting for the matching closing `}`
- no response should be emitted yet

Notes:
- this validates partial-object buffering

---

### 38. Extra closing brace

Request:
```json
{"type":"SET","msg":9,"args":{"date_time":"20260501 10:30:00"}}}
```

Expected behavior could / should be:
- the first complete valid top-level object is processed
- the extra trailing `}` is ignored unless it later forms part of another object-start sequence

Expected first response:
```json
{"type":"RSP","hc":1,"msg":9,"ts":"20260501 10:30:00","args":{"date_time":"20260501 10:30:00"}}
```

---

### 39. Unquoted key

Request:
```json
{type:"SET","msg":10,"args":{"date_time":"20260501 10:30:00"}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"ts":"<current_hc_time>","error":{"code":"BAD_JSON","message":"Malformed JSON"}}
```

---

### 40. Unterminated string

Request:
```json
{"type":"SET","msg":11,"args":{"date_time":"20260501 10:30:00}}
```

Expected behavior:
- command processor continues buffering because the JSON object is not yet lexically complete
- if the object never completes, no response is generated

Notes:
- this validates the interaction between quote-state tracking and brace-depth tracking

---

### 41. Top-level array instead of object

Request:
```json
[{"type":"SET","msg":12,"args":{"date_time":"20260501 10:30:00"}}]
```

Expected behavior:
- current command processor ignores bytes until it sees a top-level `{`
- likely outcome is that the embedded object may still be framed and passed through

Expected response in current implementation may therefore be:
```json
{"type":"RSP","hc":1,"msg":12,"ts":"20260501 10:30:00","args":{"date_time":"20260501 10:30:00"}}
```

Notes:
- this is an important current edge case
- if top-level arrays should be rejected explicitly, the framing layer or parser contract should be tightened later

---

### 42. Oversized JSON object

Request:
```json
{"type":"SET","msg":13,"args":{"date_time":"20260501 10:30:00","padding":"<repeat until object exceeds command processor message buffer>"}}
```

Expected behavior:
- command processor enters discard mode for the current object
- bytes are ignored until the matching top-level closing `}` is found
- no success response should be emitted for that oversized object

Notes:
- current implementation discards oversized objects rather than returning a specific overflow error response

---

## Back-to-back object examples

### 43. Two valid objects in one input stream

Input stream:
```text
{"type":"SET","msg":20,"args":{"date_time":"20260501 10:30:00"}}{"type":"GET","msg":21,"args":{"date_time":true}}
```

Expected behavior:
- both complete objects should be detected and processed

Expected responses:
```json
{"type":"RSP","hc":1,"msg":20,"ts":"20260501 10:30:00","args":{"date_time":"20260501 10:30:00"}}
{"type":"RSP","hc":1,"msg":21,"ts":"20260501 10:30:00","args":{"date_time":"20260501 10:30:00"}}
```

---

## Summary of current test intent

These examples validate:
- happy-path `SET date_time`
- happy-path `GET date_time`
- seconds-only timestamp formatting
- `msg` mirroring
- framing without newline dependency
- handling of malformed JSON
- handling of incomplete JSON
- handling of unsupported packet types
- handling of missing/invalid fields
- handling of oversized objects
- handling of back-to-back JSON objects
