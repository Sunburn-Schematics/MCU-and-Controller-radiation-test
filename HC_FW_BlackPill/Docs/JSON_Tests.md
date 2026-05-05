# JSON Tests

## Purpose

This document provides simple test vectors for the current first-slice HC command processor implementation.

Current implemented scope:
- command processor frames one complete top-level JSON object from the USB byte stream
- `CommandHandler` supports `SET` and `GET`
- `SET` currently supports `args.date_time`, `args.sts_period_ms`, `args.dbg_period_ms`, and `args.dbg_signals`
- `SET` also supports `args.adc_cal`
- `GET` currently supports `args.date_time`, `args.raw_adc`, `args.dbg_period_ms`, `args.dbg_signals`, and `args.adc_cal`
- timestamps use seconds-only format: `YYYYMMDD HH:MM:SS`
- current temporary HC identifier in responses is `1`
- periodic `STS` transmission is emitted by `fw_app_run()` at a configurable millisecond interval
- `args.sts_period_ms = 0` disables periodic `STS` transmission
- periodic `DBG` transmission is emitted by `fw_app_run()` at a configurable millisecond interval
- `args.dbg_period_ms = 0` disables periodic `DBG` transmission

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

### 11. Valid `GET adc_cal`

Request:
```json
{"type":"GET","msg":24,"args":{"adc_cal":3}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":24,"ts":"20260501 10:30:00","args":{"adc_cal":{"channel":3,"slope_scaled":2500,"offset":-100,"valid":true}}}
```

---

### 12. Valid `SET adc_cal` with channel name

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

### 13. Valid `GET adc_cal` with channel name

Request:
```json
{"type":"GET","msg":26,"args":{"adc_cal":"vupstream"}}
```

Expected response:
```json
{"type":"RSP","hc":1,"msg":26,"ts":"20260501 10:30:00","args":{"adc_cal":{"channel":0,"slope_scaled":2500,"offset":-100,"valid":true}}}
```

---

### 14. Valid JSON preceded or followed by non-JSON noise bytes

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
{"type":"STS","hc_id":1,"ts":"20260501 10:30:00","state":"NORMAL","beam_on":false,"duts":{"LTC3901":{"state":"NORMAL","pwr_en":false,"sync":true,"vsupply":null,"vshunt":null,"isupply":null,"me_freq":null,"me_ratio":null,"me_anlg":null,"mf_freq":null,"mf_ratio":null,"mf_anlg":null,"faults":{"count":0,"summary":"NONE","ids":[]}},"LT8316":{"state":"NORMAL","pwr_en":false,"gate_freq":null,"gate_ratio":null,"gate_anlg":null,"vout":null,"faults":{"count":0,"summary":"NONE","ids":[]}}}}
```

Notes:
- `STS` is HC-originated and is not a TE request/response transaction
- `ts` uses the HC RTC-backed timestamp format `YYYYMMDD HH:MM:SS`
- current first-slice implementation uses application-layer placeholder values for DUT telemetry fields that are not yet wired to live measurements
- unavailable measurements use `null`

---

## Valid JSON but invalid-command examples

These are valid JSON syntactically, but should produce an error response because they are unsupported or invalid for the current implementation.

### 15. Unsupported packet type `EXC`

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

### 16. Missing supported field in `SET`

Request:
```json
{"type":"SET","msg":2,"args":{}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":2,"ts":"<current_hc_time>","error":{"code":"BAD_FIELD","message":"SET currently supports args.date_time, args.sts_period_ms, dbg_period_ms, dbg_signals, or adc_cal"}}
```

---

### 17. `args.date_time` wrong format in `SET`

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

### 18. Invalid calendar/time value in `SET`

Request:
```json
{"type":"SET","msg":4,"args":{"date_time":"20260230 10:30:00"}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":4,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"args.date_time contains an invalid calendar/time value"}}
```

---

### 19. Invalid `args.sts_period_ms` type in `SET`

Request:
```json
{"type":"SET","msg":20,"args":{"sts_period_ms":true}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":20,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"args.sts_period_ms must be a non-negative integer"}}
```

---

### 20. `GET` missing `args`

Request:
```json
{"type":"GET","msg":5}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":5,"ts":"<current_hc_time>","error":{"code":"BAD_ARGS","message":"GET requires an args object"}}
```

---

### 21. `GET` missing supported field

Request:
```json
{"type":"GET","msg":6,"args":{}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":6,"ts":"<current_hc_time>","error":{"code":"BAD_FIELD","message":"GET currently supports args.date_time, args.raw_adc, dbg_period_ms, dbg_signals, or adc_cal"}}
```

---

### 22. Invalid `SET dbg_period_ms`

Request:
```json
{"type":"SET","msg":23,"args":{"dbg_period_ms":50}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":23,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"args.dbg_period_ms must be 0 or 100..60000, and args.dbg_signals must be a valid signal array"}}
```

---

### 23. Invalid `SET dbg_signals`

Request:
```json
{"type":"SET","msg":24,"args":{"dbg_signals":["pwm.me.freq_hz","not.a.real.signal"]}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":24,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"args.dbg_period_ms must be 0 or 100..60000, and args.dbg_signals must be a valid signal array"}}
```

---

### 24. Invalid `SET adc_cal`

Request:
```json
{"type":"SET","msg":25,"args":{"adc_cal":{"channel":99,"slope_scaled":2500,"offset":0,"valid":true}}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":25,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"args.adc_cal.channel is out of range"}}
```

---

### 25. Invalid `GET dbg_signals`

Request:
```json
{"type":"GET","msg":26,"args":{"dbg_signals":["adc.vupstream.raw","not.a.real.signal"]}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":26,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"GET args.dbg_period_ms must be true, and args.dbg_signals must be true or a valid signal array"}}
```

---

### 26. `GET args.date_time` not `true`

Request:
```json
{"type":"GET","msg":7,"args":{"date_time":false}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"msg":7,"ts":"<current_hc_time>","error":{"code":"BAD_VALUE","message":"GET args.date_time must be true"}}
```

---

### 27. Non-numeric `msg`

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

### 28. Missing closing brace

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

### 29. Extra closing brace

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

### 30. Unquoted key

Request:
```json
{type:"SET","msg":10,"args":{"date_time":"20260501 10:30:00"}}
```

Expected response could / should be:
```json
{"type":"RSP","hc":1,"ts":"<current_hc_time>","error":{"code":"BAD_JSON","message":"Malformed JSON"}}
```

---

### 31. Unterminated string

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

### 32. Top-level array instead of object

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

### 33. Oversized JSON object

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

### 34. Two valid objects in one input stream

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
