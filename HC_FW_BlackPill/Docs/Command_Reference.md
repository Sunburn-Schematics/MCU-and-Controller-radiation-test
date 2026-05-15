# TE/HC JSON Command Reference

## Purpose

This document describes the implemented JSON command structure used between the
Test Executive (TE) and the Host Controller (HC) over USB Virtual COM Port
(USB VCP).

Formal syntax diagrams for the implemented command forms are maintained in
[Command_Syntax.md](./Command_Syntax.md).

TE-originated commands are complete JSON objects. Newline characters are a
convenient send convention, but they are not the semantic frame boundary for
commands received by the device. The firmware command parser identifies complete
top-level JSON objects.

HC-originated records are emitted as compact JSON objects. The device convention
is to emit each `RSP`, `STS`, `DBG`, or `EVT` object followed by a newline so
host tools and logs can process them one object per line.

## Terminology

- **TE**: Test Executive.
- **HC**: Host Controller.
- **USB VCP**: USB Virtual COM Port transport between TE and HC.
- **GET**: TE request asking the HC to return one or more values.
- **SET**: TE request asking the HC to apply one or more values or actions.
- **RSP**: HC response to a TE request.
- **STS**: Periodic HC status record.
- **DBG**: Periodic HC debug telemetry record.
- **EVT**: Asynchronous HC event record.

## Transport and Framing

TE-to-HC command rules:

- each command is one complete JSON object
- commands are encoded as UTF-8 text
- `type` is currently `GET` or `SET`
- `msg` is a numeric TE-supplied correlation value
- `GET args` is an array of field names
- `SET args` is an object containing fields to apply
- newline after a command is allowed and is the normal host-tool convention
- newline is not required to be the only framing mechanism

HC-to-TE output rules:

- each emitted record is one compact JSON object
- records are emitted with newline separators
- host tools should tolerate asynchronous `STS`, `DBG`, and `EVT` records while
  waiting for a matching `RSP.msg`

## Message Types

| Type | Direction | Purpose |
|---|---|---|
| `GET` | TE to HC | Read/query command. |
| `SET` | TE to HC | Write/action command. |
| `RSP` | HC to TE | Response to a `GET` or `SET`. |
| `STS` | HC to TE | Periodic status record. |
| `DBG` | HC to TE | Periodic debug telemetry record. |
| `EVT` | HC to TE | Asynchronous event record. |

## Timestamps

The protocol timestamp field is `ts`.

Format:

```json
{"ts":"20260501 10:14:23"}
```

`RSP`, `STS`, `DBG`, and `EVT` records include `ts`.

## Request Correlation

TE requests include a numeric `msg` field:

```json
{"type":"GET","msg":101,"args":["date_time"]}
```

The HC mirrors `msg` in the corresponding `RSP` when the request contains a
valid numeric `msg`. `msg` values do not need to be globally unique, but unique
values make host-side matching deterministic.

## GET Commands

Shape:

```json
{"type":"GET","msg":101,"args":["field"]}
```

Implemented `GET args` fields:

| Field | Purpose |
|---|---|
| `date_time` | Return current RTC-backed date/time. |
| `sw_version` | Return compiled firmware version string. |
| `dbg_period_ms` | Return debug telemetry period. |
| `dbg_signals` | Return debug telemetry configuration. |
| debug signal name | Return a one-shot sample for that signal, for example `adc.vupstream.raw`. |
| direct ADC calibration field | Return RAM-resident ADC calibration fields, for example `adc.vupstream.slope_scaled`, `adc.vupstream.offset`, or `adc.vupstream.valid`. |
| `ltc3901` | Return LTC3901 manager configuration. |
| `lt8316` | Return LT8316 manager configuration. |

Example:

```json
{"type":"GET","msg":101,"args":["sw_version"]}
```

ADC calibration readback example:

```json
{"type":"GET","msg":102,"args":["adc.vupstream.slope_scaled","adc.vupstream.offset","adc.vupstream.valid"]}
```

Debug signal sample example:

```json
{"type":"GET","msg":103,"args":["adc.vupstream.raw","pwm.me.freq_hz"]}
```

## SET Commands

Shape:

```json
{"type":"SET","msg":102,"args":{"field":123}}
```

Implemented `SET args` fields:

| Field | Purpose |
|---|---|
| `date_time` | Set RTC-backed date/time in `YYYYMMDD HH:MM:SS` format. |
| `sts_period_ms` | Configure periodic `STS`; `0` disables periodic `STS`. |
| `dbg_period_ms` | Configure periodic `DBG`; `0` disables periodic `DBG`. |
| `dbg_signals` array | Select periodic debug telemetry signals. |
| `hc_cmd` | Apply host-controller-level commands. Currently supports standalone `RESET` to request an MCU software reset while preserving the RTC backup domain. |
| direct digital signal fields | Set supported digital outputs, such as `led.blue` or `ltc3901.pwr_en`. |
| direct ADC calibration fields | Set RAM-resident ADC calibration with `adc.<channel>.slope_scaled`, `adc.<channel>.offset`, and `adc.<channel>.valid`. |
| `ltc3901_cmd` | Apply `RUN`, `HALT`, or `RESET` to the LTC3901 manager. |
| `lt8316_cmd` | Apply `RUN` or `RESET` to the LT8316 manager. |
| direct LTC3901 config fields | Partially update manager configuration with fields such as `ltc3901.isupply_ma_max`. |
| direct LT8316 config fields | Partially update manager configuration with fields such as `lt8316.power_fault_max`. |

Example:

```json
{"type":"SET","msg":102,"args":{"sts_period_ms":1000}}
{"type":"SET","msg":103,"args":{"hc_cmd":"RESET"}}
{"type":"SET","msg":104,"args":{"adc.vupstream.slope_scaled":2500,"adc.vupstream.offset":-100,"adc.vupstream.valid":true}}
{"type":"SET","msg":105,"args":{"ltc3901.isupply_ma_max":75,"lt8316.power_fault_max":4}}
```

## Responses

Success response shape:

```json
{"type":"RSP","hc":17,"msg":102,"ts":"20260501 10:14:23","args":{"sts_period_ms":1000}}
```

Error response shape:

```json
{"type":"RSP","hc":17,"msg":102,"ts":"20260501 10:14:23","error":{"code":"BAD_ARGS","message":"SET currently supports ..."}}
```

Response fields:

| Field | Meaning |
|---|---|
| `type` | Always `RSP`. |
| `hc` | Host Controller hardware ID. |
| `msg` | Echoed TE message identifier when available. |
| `ts` | HC timestamp. |
| `args` | Returned or applied values for successful requests. |
| `error` | Structured error details for failed requests. |

## Periodic STS Records

`STS` is the canonical periodic status record.

Canonical top-level shape:

```json
{
  "type": "STS",
  "hc_id": 17,
  "ts": "20260501 10:14:23",
  "beam_on": false,
  "duts": {
    "LTC3901": {},
    "LT8316": {}
  }
}
```

DUT-specific manager states are reported under `duts`.

## Periodic DBG Records

`DBG` records are disabled by default. Enable them with `dbg_period_ms` and
select fields with `dbg_signals`.

Example:

```json
{"type":"DBG","hc_id":17,"ts":"20260501 10:14:23","signals":{"adc.vupstream.raw":8}}
```

## Asynchronous EVT Records

`EVT` records report DUT manager transitions and fault-like events. They are not
responses to TE requests and do not include `msg`.

Example:

```json
{"type":"EVT","hc":17,"ts":"20260501 10:14:23","args":{"msg":"LTC3901: Cycle Sync ON"}}
```

## Error Codes

Common error codes include:

| Code | Meaning |
|---|---|
| `BAD_JSON` | Invalid JSON syntax. |
| `BAD_TYPE` | Unknown or invalid `type`. |
| `BAD_ARGS` | Missing, malformed, or invalid arguments. |
| `BAD_FIELD` | Unknown or unsupported field name. |
| `BAD_VALUE` | Invalid value for a recognized field. |
| `BAD_STATE` | Request not allowed in the current manager/application state. |
| `BUSY` | HC temporarily unable to service the request. |
| `INTERNAL` | Internal HC processing error. |
| `NOT_SUPPORTED` | Request recognized but not implemented in this build. |

## Notes

- Runtime manager configuration is RAM-resident and is not persisted across reset.
- Host tools should use `msg` to match `RSP` records and ignore unrelated
  asynchronous records while waiting.
- Durable hardware test transcripts use `.jsonl` files because they store one
  result object per line; that file format does not change the command protocol
  framing rule.
