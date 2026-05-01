# Preliminary TE_HC JSONL Command Structure v1

## 1. Purpose

This document defines a preliminary JSONL-based command structure for communications between the Test Executive (TE) and the Host Controller (HC) over the USB Virtual COM Port (USB VCP).

This preliminary command-layer definition establishes:
- a single primary framing model,
- top-level message types for command, response, status, event, and future execution requests,
- preliminary `GET` and `SET` packet structures,
- a common HC response structure,
- an HC information payload structure,
- deterministic transaction handling rules.

This document assumes JSONL is the primary HC↔TE protocol format over USB VCP.

---

## 2. Scope

This document covers:
- line framing,
- top-level packet type definitions,
- `GET`, `SET`, `RSP`, `STS`, `EVT`, and reserved `EXC` packet structures,
- mandatory `msg` field usage in TE-originated packets,
- timestamp field usage,
- preliminary HC information reporting,
- response conventions,
- preliminary error handling.

This document does not yet fully define:
- final timing requirements,
- final maximum frame length,
- final event catalogue,
- final fault detail payload schemas,
- final `EXC` semantics,
- final authorization/debug restrictions.

---

## 3. Terminology

- **TE**: Test Executive.
- **HC**: Host Controller.
- **USB VCP**: USB Virtual COM Port transport between TE and HC.
- **JSONL**: One complete JSON object per line.
- **GET**: A TE request asking the HC to return the present value of one or more fields.
- **SET**: A TE request asking the HC to apply one or more requested field values.
- **RSP**: An HC response packet.
- **STS**: A periodic HC status packet.
- **EVT**: An asynchronous HC event packet.
- **EXC**: A reserved TE execute packet type for future use.

---

## 4. Transport and Framing

### 4.1 Primary Transport Rule
All primary HC↔TE protocol traffic shall use JSONL over the USB VCP.

### 4.2 Frame Format
Each protocol frame shall be:
- one complete JSON object,
- encoded as UTF-8 text,
- terminated by a single newline character `\n`.

### 4.3 Framing Constraints
- A protocol frame shall not span multiple lines.
- Pretty-printed or multi-line JSON shall not be used on the wire.
- Each received line shall be parsed independently as a complete JSON object.

### 4.4 Rationale
This framing keeps parser implementation simple and deterministic and allows `GET`, `SET`, `RSP`, `STS`, and `EVT` traffic to share one uniform wire format.

---

## 5. Top-Level Packet Types

The preliminary protocol defines these top-level packet types:
- `GET` — TE to HC request for one or more field values
- `SET` — TE to HC request to apply one or more field values
- `RSP` — HC to TE response packet
- `STS` — HC to TE periodic status packet
- `EVT` — HC to TE asynchronous event packet
- `EXC` — reserved TE to HC execute request for future use

---

## 6. Common Packet Principles

### 6.1 General Principles
- All protocol packets shall be JSON objects.
- Field names are case-sensitive.
- Unknown fields should be ignored unless explicitly prohibited by a later revision.
- HC IDs are not required in TE-originated `GET`, `SET`, or future `EXC` packets.

### 6.2 No On-Wire Protocol Version Field
There shall be no `version` field in normal protocol packets.

The version of the command system shall instead be reported by the HC using the HC information payload defined in this document.

---

## 7. Timestamp Field

### 7.1 Timestamp Field Name
The protocol timestamp field shall be named `ts`.

### 7.2 Timestamp Format
The `ts` field shall use the following format:
- `YYYYMMDD HH:MM:SS.MS`

Example:
```json
{"ts":"20260501 10:14:23.42"}
```

### 7.3 Timestamp Usage
- Every `STS` packet shall include `ts`.
- Every `RSP` packet shall include `ts`.
- Every `EVT` packet should include `ts`.

---

## 8. Message Correlation Field

### 8.1 Field Name
The preliminary correlation field shall be named `msg`.

### 8.2 Type
`msg` shall be numeric only.

Example:
```json
{"type":"GET","msg":101,"args":["hc_info"]}
```

### 8.3 Usage Rule
- The TE shall include `msg` in every `GET`, `SET`, and future `EXC` packet.
- The HC may mirror `msg` in `RSP` packets when it is useful or necessary in context.
- In particular, the HC may include `msg` when reporting a response associated with a specific TE request, especially for errors or ambiguous exchanges.
- The HC is not required to include `msg` in every `RSP` if the context does not require it.

### 8.4 Uniqueness Rule
There is no requirement that `msg` values be unique.

The primary purpose of `msg` is to provide a TE-supplied message identifier that the HC can reference, particularly when reporting request-specific errors.

### 8.5 Preliminary Recommendation
Even though it is optional in `RSP`, mirroring `msg` in `RSP` should be preferred for deterministic parsing and easier TE-side transaction tracking.

---

## 9. HC Information Payload

### 9.1 Purpose
The HC shall provide an HC information payload for reporting summary identity and version information.

### 9.2 Intended Content
The HC information payload should include:
- HC ID,
- command system version,
- firmware version,
- any additional HC summary information added later.

### 9.3 Preliminary Example
```json
{
  "hc_info": {
    "hc": 17,
    "cmd_sys_ver": "1.0-prelim",
    "fw_ver": "0.1.0",
    "summary": {
      "usb_vcp_active": true
    }
  }
}
```

### 9.4 Expansion Rule
Additional summary fields may be added later within `hc_info` without changing the top-level packet model.

---

## 10. TE to HC GET Packet

### 10.1 Purpose
A `GET` packet requests the current value of one or more HC fields.

### 10.2 Packet Shape
```json
{"type":"GET","msg":101,"args":["field1","field2"]}
```

### 10.3 Example
```json
{"type":"GET","msg":101,"args":["hc_info","beam_on"]}
```

### 10.4 Fields
| Field | Type | Required | Meaning |
|---|---|---:|---|
| `type` | string | yes | Shall be `"GET"` |
| `msg` | number | yes | TE message identifier |
| `args` | array | yes | Array of requested field names |

### 10.5 Rules
- `msg` shall be present.
- `args` shall be present.
- `args` shall be an array.
- Each entry in `args` should be a string naming a requested field or payload group.
- A `GET` packet shall request present HC values, not desired values.

---

## 11. TE to HC SET Packet

### 11.1 Purpose
A `SET` packet requests that the HC apply one or more target values.

### 11.2 Packet Shape
```json
{"type":"SET","msg":102,"args":{"field1":123,"field2":true}}
```

### 11.3 Example
```json
{"type":"SET","msg":102,"args":{"beam_on":true}}
```

### 11.4 Fields
| Field | Type | Required | Meaning |
|---|---|---:|---|
| `type` | string | yes | Shall be `"SET"` |
| `msg` | number | yes | TE message identifier |
| `args` | object | yes | Requested field/value assignments |

### 11.5 Rules
- `msg` shall be present.
- `args` shall be present.
- `args` shall be an object.
- Each field in `args` names a requested target field.
- Each value in `args` is the requested target value for that field.

---

## 12. Reserved TE to HC EXC Packet

### 12.1 Purpose
`EXC` is reserved for future use.

It is intended for TE requests that ask the HC to perform a predefined sequence of operations rather than a simple field read or field assignment.

### 12.2 Current Status
The specifics of `EXC` packet structure, semantics, and response handling are intentionally deferred.

### 12.3 Reservation Rule
The packet type name `EXC` is reserved and should not be repurposed for another meaning.

### 12.4 Preliminary `msg` Rule
Future `EXC` packets shall also include `msg`.

---

## 13. HC to TE RSP Packet

### 13.1 Purpose
`RSP` is the HC response packet used to return requested values, actual applied values, or structured error information.

### 13.2 Mandatory Fields
Every `RSP` packet shall include:
- `type`,
- `hc`,
- `ts`.

### 13.3 Standard Shape
```json
{"type":"RSP","hc":17,"ts":"20260501 10:14:23.42","args":{"beam_on":true}}
```

### 13.4 With `msg`
```json
{"type":"RSP","hc":17,"ts":"20260501 10:14:23.42","msg":102,"args":{"beam_on":true}}
```

### 13.5 Error Shape
```json
{"type":"RSP","hc":17,"ts":"20260501 10:14:23.42","msg":102,"error":{"code":"BAD_ARGS","message":"Unknown field: beam_enable"}}
```

### 13.6 Fields
| Field | Type | Required | Meaning |
|---|---|---:|---|
| `type` | string | yes | Shall be `"RSP"` |
| `hc` | number | yes | Host Controller ID |
| `ts` | string | yes | HC date/time stamp |
| `msg` | number | no | Optional TE message identifier echoed by HC |
| `args` | object | success/path-dependent | Returned actual HC values |
| `error` | object | error/path-dependent | Structured error information |

### 13.7 Response Rules
- Every response to a `GET` shall be an `RSP` packet.
- Every `GET` shall receive an `RSP`.
- A response to a `SET` is optional.
- If the HC emits an `RSP` to a `SET`, the `args` object should contain the actual values present on the HC rather than simply mirroring the TE-requested values.
- A failure response should contain an `error` object.
- If required for context, the HC may include `msg` in the `RSP`.

---

## 14. HC to TE STS Packet

### 14.1 Purpose
`STS` is the periodic HC status packet.

### 14.2 Required Fields
Every `STS` packet shall include:
- `type`,
- `hc`,
- `ts`.

### 14.3 Preliminary Shape
```json
{"type":"STS","hc":17,"ts":"20260501 10:14:23.42","state":"NORMAL","beam_on":false,"duts":{}}
```

### 14.4 Notes
- `tsb` is no longer used in `STS` packets.
- `ts` replaces `tsb` as the status timestamp field.
- The detailed `STS` payload content remains governed by the HC status specification.

---

## 15. HC to TE EVT Packet

### 15.1 Purpose
`EVT` is an asynchronous HC-to-TE notification packet.

It is intended for immediate notification of significant events that should not wait for the next periodic `STS` packet.

Examples include:
- a newly detected fault,
- an isolation action,
- a recovery completion,
- a state transition requiring prompt TE awareness.

### 15.2 Required Fields
Every `EVT` packet should include:
- `type`,
- `hc`,
- `ts`,
- an event descriptor payload.

### 15.3 Preliminary Shape
```json
{"type":"EVT","hc":17,"ts":"20260501 10:14:23.42","args":{"event":"FAULT","scope":"LTC3901","id":"HLF-003"}}
```

### 15.4 Preliminary Rules
- `EVT` packets are asynchronous and are not responses to `GET` or `SET` by default.
- `EVT` packets should be used for information requiring prompt TE attention.
- The detailed event catalogue and payload schemas remain to be defined.

---

## 16. Response Behavior for GET and SET

### 16.1 GET
A `GET` command shall always produce an `RSP` packet.

The `RSP.args` object shall contain the requested HC values that are available and relevant to the request.

Example:
```json
{"type":"GET","msg":201,"args":["hc_info","beam_on"]}
```

Example response:
```json
{"type":"RSP","hc":17,"ts":"20260501 10:14:23.42","msg":201,"args":{"hc_info":{"hc":17,"cmd_sys_ver":"1.0-prelim","fw_ver":"0.1.0"},"beam_on":false}}
```

### 16.2 SET
A `SET` command may produce an `RSP` packet.

If the HC emits `RSP` to a `SET`, the response shall report the actual values present on the HC after command handling rather than merely echoing the requested values.

Example request:
```json
{"type":"SET","msg":202,"args":{"beam_on":true}}
```

Example response:
```json
{"type":"RSP","hc":17,"ts":"20260501 10:14:23.42","msg":202,"args":{"beam_on":true}}
```

If the HC cannot process the request, it may emit:
```json
{"type":"RSP","hc":17,"ts":"20260501 10:14:23.42","msg":202,"error":{"code":"BAD_STATE","message":"SET not permitted in current HC state"}}
```

---

## 17. Preliminary Error Model

### 17.1 Error Object Shape
A failure response should use this error object structure:

```json
{"code":"BAD_ARGS","message":"Unknown field: beam_enable"}
```

### 17.2 Preliminary Error Codes
| Code | Meaning |
|---|---|
| `BAD_JSON` | Invalid JSON syntax |
| `BAD_FRAME` | Invalid frame structure or framing violation |
| `BAD_TYPE` | Unknown or invalid `type` |
| `BAD_ARGS` | Missing, malformed, or invalid arguments |
| `BAD_FIELD` | Unknown or unsupported field name |
| `BAD_VALUE` | Invalid value for a recognized field |
| `BAD_STATE` | Request not allowed in the current HC state |
| `BUSY` | HC temporarily unable to service the request |
| `INTERNAL` | Internal HC processing error |
| `NOT_SUPPORTED` | Request recognized but not implemented in this build |

### 17.3 Error Context Rule
If error context depends on a particular TE request, the HC should include the associated `msg` value in the `RSP`.

---

## 18. Preliminary Parsing and Validation Order

When the HC receives one line from the USB VCP, the HC should validate in this order:
1. a complete line has been received,
2. the line parses as a JSON object,
3. `type` exists and is one of the supported TE-originated types,
4. `msg` exists and is numeric,
5. `args` exists and has the correct JSON type for the packet type,
6. requested fields are recognized,
7. requested values are valid,
8. HC state and policy preconditions are satisfied,
9. the request is executed,
10. any required `RSP` packet is emitted.

For preliminary v1, supported TE-originated types are:
- `GET`
- `SET`

`EXC` remains reserved.

---

## 19. Preliminary v1 Constraints

To keep initial firmware implementation simple, the following constraints are recommended for v1:
- top-level protocol packets shall be JSON objects only,
- one line shall contain exactly one packet,
- binary payload transport is out of scope,
- maximum frame length shall be defined later as an explicit implementation limit,
- malformed requests should receive a structured error response when practical,
- TE shall include `msg` in every TE-originated packet,
- `msg` values do not need to be unique,
- TE should avoid multiple outstanding transactions until correlation behavior is finalized,
- HC should prefer echoing `msg` in `RSP` for deterministic debugging and logging even though it is not strictly mandatory in all contexts.

---

## 20. Example Packet Set

### 20.1 GET request for HC information
```json
{"type":"GET","msg":1001,"args":["hc_info"]}
```

### 20.2 GET response with HC information
```json
{"type":"RSP","hc":17,"ts":"20260501 10:14:23.42","msg":1001,"args":{"hc_info":{"hc":17,"cmd_sys_ver":"1.0-prelim","fw_ver":"0.1.0","summary":{"usb_vcp_active":true}}}}
```

### 20.3 SET request
```json
{"type":"SET","msg":1002,"args":{"beam_on":true}}
```

### 20.4 Optional SET response using actual HC values
```json
{"type":"RSP","hc":17,"ts":"20260501 10:14:23.42","msg":1002,"args":{"beam_on":true}}
```

### 20.5 Periodic STS packet
```json
{"type":"STS","hc":17,"ts":"20260501 10:14:24.00","state":"NORMAL","beam_on":true,"duts":{"LTC3901":{"state":"NORMAL"},"LT8316":{"state":"NORMAL"}}}
```

### 20.6 Asynchronous EVT packet
```json
{"type":"EVT","hc":17,"ts":"20260501 10:14:24.11","args":{"event":"FAULT","scope":"LT8316","id":"HLF-007","severity":"HIGH"}}
```

### 20.7 Error response
```json
{"type":"RSP","hc":17,"ts":"20260501 10:14:24.20","msg":1002,"error":{"code":"BAD_STATE","message":"SET not permitted while HC is in LOW_LEVEL_FAULT"}}
```

---

## 21. Open Items

The following items remain open for later definition:
- final field catalogue for `GET` and `SET`,
- exact `hc_info.summary` content,
- exact `STS` payload field list,
- exact `EVT` payload catalogue and severity model,
- whether `SET` acknowledgments should remain optional or become mandatory for some field classes,
- timeout expectations,
- maximum allowed line length,
- the detailed `EXC` packet format and response behavior.

---

## 22. Preliminary Recommendation

The recommended preliminary v1 direction is:
- all HC↔TE protocol traffic uses JSONL over USB VCP,
- TE issues read requests with `type: "GET"`,
- TE issues write requests with `type: "SET"`,
- HC responds using `type: "RSP"`,
- HC provides periodic status using `type: "STS"`,
- HC provides immediate asynchronous notifications using `type: "EVT"`,
- `EXC` is reserved for future command sequencing,
- `ts` replaces `tsb`,
- `msg` is numeric and mandatory in all TE-originated packets,
- `msg` does not need to be unique,
- `hc` is mandatory in `RSP`, `STS`, and normally in `EVT`,
- command-system version is reported through `hc_info` rather than an on-wire `version` field.

This provides a uniform and firmware-friendly packet structure for early implementation and later refinement.
