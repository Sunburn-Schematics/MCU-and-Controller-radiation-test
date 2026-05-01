# HC JSONL Command Processor Implementation Plan v1

## 1. Purpose

This document maps out a practical firmware implementation structure for the HC JSONL command processor, beginning with the first supported operation:
- `SET date_time`

Initial target message sequence:

### TE → HC
```json
{"type":"SET","msg":0,"args":{"date_time":"20260501 10:30:00.00"}}
```

### HC → TE
```json
{"type":"RSP","msg":0,"ts":"20260501 10:30:00.00","args":{"date_time":"20260501 10:30:00.00"}}
```

This plan is intentionally implementation-oriented. It focuses on module boundaries, data flow, validation steps, and the minimum first increment needed to get a working command processor into the firmware.

---

## 2. Current Codebase Context

Relevant existing project locations:

- `C:\Users\marty\Code\STM32\SBS\MCU-and-Controller-radiation-test\HC_FW_BlackPill\Services\jsmn\`
  - `jsmn.c`
  - `jsmn.h`
  - `jsmn_utils.c`
  - `jsmn_utils.h`
  - `json_cmd_launcher.c`
  - `json_cmd_launcher.h`
- `C:\Users\marty\Code\STM32\SBS\MCU-and-Controller-radiation-test\HC_FW_BlackPill\Drivers_Local\usb_vcp_drv.c`
- `C:\Users\marty\Code\STM32\SBS\MCU-and-Controller-radiation-test\HC_FW_BlackPill\Drivers_Local\usb_vcp_drv.h`
- `C:\Users\marty\Code\STM32\SBS\MCU-and-Controller-radiation-test\HC_FW_BlackPill\USB_DEVICE\App\usbd_cdc_if.c`
- `C:\Users\marty\Code\STM32\SBS\MCU-and-Controller-radiation-test\HC_FW_BlackPill\App\fw_app.c`
- `C:\Users\marty\Code\STM32\SBS\MCU-and-Controller-radiation-test\HC_FW_BlackPill\App\fw_app.h`

The existing `Services\jsmn` folder is the right place to anchor the JSONL command parser and dispatcher.

---

## 3. Implementation Goal for the First Increment

The first firmware increment should support exactly this capability:
- receive one newline-terminated JSONL packet from the USB VCP,
- parse it using `jsmn`,
- validate that it is a supported `SET` packet,
- validate `msg`,
- validate `args.date_time`,
- update the HC current date/time value,
- emit a well-formed JSONL `RSP` packet,
- reject malformed or unsupported packets with structured error responses where practical.

The first increment should **not** try to implement the entire protocol at once.

---

## 4. Recommended Layered Structure

The implementation should be split into five layers.

### 4.1 Transport Input Layer
Responsibility:
- receive raw bytes from USB CDC/VCP,
- accumulate bytes into a line buffer,
- detect end-of-line,
- hand complete JSONL frames to the command processor.

Suggested ownership:
- `Drivers_Local\usb_vcp_drv.*`
- or a small HC-specific wrapper above it

### 4.2 Frame Buffering Layer
Responsibility:
- maintain one RX line buffer,
- reject oversized frames,
- normalize line termination if needed,
- present a null-terminated JSON string to the parser.

This layer should be very small and deterministic.

### 4.3 JSON Parse Layer
Responsibility:
- invoke `jsmn` on one complete line,
- produce token array output,
- provide helper functions for extracting fields.

Suggested ownership:
- `Services\jsmn\jsmn_utils.*`
- new HC-specific JSON helper module if needed

### 4.4 Command Validation and Dispatch Layer
Responsibility:
- determine packet type,
- validate required fields,
- route to the appropriate handler,
- build success or error response metadata.

Suggested ownership:
- `Services\jsmn\json_cmd_launcher.*`
- or a renamed HC-specific module if `json_cmd_launcher` already exists for that purpose

### 4.5 HC State/Service Layer
Responsibility:
- apply validated operations to internal HC state,
- own the canonical current `date_time` value,
- provide current values for response generation.

Suggested ownership:
- a new service module, separate from parsing and transport

---

## 5. Recommended New Firmware Modules

The cleanest structure is to add a small set of HC-specific modules around `jsmn`.

### 5.1 Command Processor Front Door
Suggested files:
- `Services\hc_jsonl_cmd.c`
- `Services\hc_jsonl_cmd.h`

Responsibility:
- entry point for one complete JSONL frame,
- high-level dispatch,
- orchestration of parse → validate → handle → respond.

Example API:
```c
void hc_jsonl_cmd_init(void);
void hc_jsonl_cmd_process_line(const char *line);
```

### 5.2 Packet Parsing Helpers
Suggested files:
- `Services\hc_jsonl_parse.c`
- `Services\hc_jsonl_parse.h`

Responsibility:
- wrappers around `jsmn`,
- token lookup helpers,
- string/number/type checking,
- extraction of `type`, `msg`, and `args` members.

### 5.3 SET/GET Dispatch Layer
Suggested files:
- `Services\hc_jsonl_dispatch.c`
- `Services\hc_jsonl_dispatch.h`

Responsibility:
- packet-type dispatch,
- route `SET` to SET handlers,
- route later `GET` to GET handlers,
- return structured status codes.

### 5.4 Field Handler Layer
Suggested files:
- `Services\hc_jsonl_fields.c`
- `Services\hc_jsonl_fields.h`

Responsibility:
- field-specific handler functions,
- for now only `date_time`,
- later expandable to `beam_on`, DUT power fields, `hc_info`, and fault queries.

### 5.5 Date/Time Service Layer
Suggested files:
- `Services\hc_datetime.c`
- `Services\hc_datetime.h`

Responsibility:
- own the current HC date/time value,
- validate date/time string format,
- set the active value,
- return formatted current value for `ts` and `args.date_time`.

### 5.6 Response Builder Layer
Suggested files:
- `Services\hc_jsonl_rsp.c`
- `Services\hc_jsonl_rsp.h`

Responsibility:
- build `RSP`, later `STS` and `EVT`,
- centralize JSON string formatting,
- ensure field ordering is consistent,
- ensure trailing newline is always appended.

### 5.7 USB TX Adapter Layer
Suggested files:
- `Services\hc_comms_tx.c`
- `Services\hc_comms_tx.h`

Responsibility:
- isolate the command processor from the exact USB CDC transmit function,
- provide one simple `send_line()` interface.

---

## 6. Minimum Data Model

The first increment only needs a very small internal data model.

### 6.1 Current Time Storage
Use a fixed-size ASCII buffer for the canonical current date/time string.

Suggested structure:
```c
typedef struct
{
    char current[24];
    uint8_t valid;
} hc_datetime_state_t;
```

Reason:
- the on-wire format is already a string,
- the immediate requirement is protocol handling,
- avoiding RTC/calendar conversion in the first increment reduces risk.

If hardware RTC integration is needed later, this string-level service can become a wrapper around a true RTC-backed representation.

### 6.2 Parsed Packet Model
Suggested structure:
```c
typedef enum
{
    HC_PKT_UNKNOWN = 0,
    HC_PKT_GET,
    HC_PKT_SET,
    HC_PKT_EXC
} hc_pkt_type_t;

typedef struct
{
    hc_pkt_type_t type;
    uint32_t msg;
    const char *args_obj_start;
    int args_tok_index;
} hc_cmd_request_t;
```

This should remain lightweight. Avoid deep copying JSON fields unless required.

---

## 7. Recommended Processing Flow

### 7.1 RX Path
1. USB CDC receives bytes.
2. Bytes are appended into an HC command RX line buffer.
3. On newline, the frame is terminated with `\0`.
4. The completed line is handed to `hc_jsonl_cmd_process_line()`.

### 7.2 Parse and Validate Path
1. Parse JSON with `jsmn`.
2. Confirm root token is an object.
3. Extract `type`.
4. Confirm `type == "SET"` for this first increment.
5. Extract `msg` and confirm it is numeric.
6. Extract `args` and confirm it is an object.
7. Confirm `args.date_time` exists and is a string.
8. Validate the `date_time` string format.

### 7.3 Execute Path
1. Pass validated date/time string to `hc_datetime_set()`.
2. If accepted, update the active HC date/time state.
3. Build `RSP`.
4. Transmit one JSONL response line.

### 7.4 Error Path
On any failure:
1. determine best available `msg` context,
2. build `RSP` with `error`,
3. include `msg` if it was successfully parsed,
4. include `ts` if available,
5. transmit error line.

---

## 8. Date/Time Validation Strategy

For the first increment, validate only the wire-format string plus basic range sanity.

Required format:
- `YYYYMMDD HH:MM:SS.MS`

Expected fixed positions:
- positions `0..7` = date digits
- position `8` = space
- positions `9..10` = hour digits
- position `11` = `:`
- positions `12..13` = minute digits
- position `14` = `:`
- positions `15..16` = second digits
- position `17` = `.`
- positions `18..19` = centisecond/millisecond digits as currently specified

### 8.1 Minimum v1 Validation
- exact length check,
- separator check,
- all digit positions numeric,
- month range `01..12`,
- day range `01..31`,
- hour range `00..23`,
- minute range `00..59`,
- second range `00..59`.

### 8.2 Deferred Validation
These may be deferred to a later increment:
- month/day calendar correctness,
- leap year handling,
- RTC synchronization,
- timezone handling,
- daylight savings handling.

---

## 9. Recommended Public APIs

### 9.1 Date/Time Service
```c
void hc_datetime_init(void);
bool hc_datetime_is_valid_string(const char *value);
bool hc_datetime_set(const char *value);
const char *hc_datetime_get(void);
```

### 9.2 Command Processor
```c
void hc_jsonl_cmd_init(void);
void hc_jsonl_cmd_process_line(const char *line);
```

### 9.3 Response Builder
```c
bool hc_jsonl_rsp_build_set_datetime_ok(char *out, size_t out_size, uint32_t msg, const char *date_time);
bool hc_jsonl_rsp_build_error(char *out, size_t out_size, bool include_msg, uint32_t msg, const char *ts, const char *code, const char *message);
```

### 9.4 Transport Adapter
```c
bool hc_comms_tx_send_line(const char *line);
```

---

## 10. Suggested File Responsibilities

### 10.1 `hc_jsonl_cmd.c`
- one top-level entry function,
- allocate parser token buffer,
- call parser helpers,
- call dispatch,
- trigger response transmit.

### 10.2 `hc_jsonl_parse.c`
- locate root keys,
- compare token strings,
- parse unsigned integer `msg`,
- fetch nested members under `args`.

### 10.3 `hc_jsonl_dispatch.c`
- map packet type to handler,
- for now only `SET` supported,
- reject `GET` and `EXC` with `NOT_SUPPORTED` until implemented.

### 10.4 `hc_jsonl_fields.c`
- implement `SET date_time` handler,
- later add other field handlers.

### 10.5 `hc_datetime.c`
- own current date/time storage,
- validate and set string,
- provide current value for outgoing `ts`.

### 10.6 `hc_jsonl_rsp.c`
- format output JSONL using `snprintf`,
- guarantee newline termination,
- escape strings if needed,
- keep packet formatting centralized.

### 10.7 `hc_comms_tx.c`
- call the actual USB VCP transmit function,
- isolate TX retry/busy behavior from command logic.

---

## 11. Response Formatting Rules for the First Increment

For the first `SET date_time` implementation, generate:

```json
{"type":"RSP","msg":0,"ts":"20260501 10:30:00.00","args":{"date_time":"20260501 10:30:00.00"}}
```

### 11.1 Important Note
Your example response does **not** include `hc`.

That differs from the current preliminary protocol document, which says every `RSP` includes `hc` and `ts`.

This must be resolved before the implementation is treated as protocol-authoritative.

Two options:
1. implement exactly your current desired message for the first firmware increment, then update the spec later,
2. align the firmware now with the current preliminary spec and include `hc` in the response.

Example if aligned to current preliminary spec:
```json
{"type":"RSP","hc":17,"msg":0,"ts":"20260501 10:30:00.00","args":{"date_time":"20260501 10:30:00.00"}}
```

I recommend resolving this before coding the formatter.

---

## 12. Error Response Structure for First Increment

Recommended error shape:

```json
{"type":"RSP","msg":0,"ts":"20260501 10:30:00.00","error":{"code":"BAD_VALUE","message":"Invalid date_time format"}}
```

Possible initial error codes:
- `BAD_JSON`
- `BAD_TYPE`
- `BAD_ARGS`
- `BAD_FIELD`
- `BAD_VALUE`
- `NOT_SUPPORTED`
- `INTERNAL`

### 12.1 Recommended First-Cut Error Cases
- malformed JSON,
- missing `type`,
- unsupported `type`,
- missing `msg`,
- non-numeric `msg`,
- missing `args`,
- `args` not an object,
- missing `args.date_time`,
- invalid `date_time` format.

---

## 13. Buffering Recommendations

### 13.1 RX Buffer
Use a fixed RX line buffer sized for modest protocol expansion.

Suggested starting size:
- `256` bytes minimum
- `384` or `512` bytes preferred

### 13.2 TX Buffer
Use a dedicated fixed TX formatting buffer.

Suggested starting size:
- same size as RX buffer

### 13.3 Token Count
For `jsmn`, use a fixed token array sized for current and modest near-term packets.

Suggested starting size:
- `32` tokens minimum
- `48` preferred

---

## 14. Determinism and Safety Rules

For the first increment:
- process one complete line at a time,
- do not accept partial JSON as executable state,
- do not mutate HC date/time until the full packet is validated,
- do not parse in interrupt context if avoidable,
- keep handler execution bounded and non-blocking,
- centralize all outgoing packet formatting.

Recommended execution model:
- ISR or CDC receive callback only appends bytes into a buffer or queue,
- main loop performs parse/dispatch/respond.

---

## 15. Recommended Incremental Development Plan

### Step 1. Establish transport handoff
- confirm where USB CDC bytes enter firmware,
- add a line accumulator,
- route complete lines to a command processor stub.

### Step 2. Implement parser skeleton
- parse root object,
- extract `type`, `msg`, `args`,
- return explicit parser status codes.

### Step 3. Implement `SET` dispatch only
- reject other packet types with `NOT_SUPPORTED`.

### Step 4. Implement `SET date_time`
- add date/time validation,
- store canonical current value,
- generate success response.

### Step 5. Implement error responses
- cover malformed packets and unsupported fields.

### Step 6. Add unit-like host-side test vectors
At minimum test these input lines:
- valid `SET date_time`
- missing `msg`
- non-numeric `msg`
- missing `args`
- `args` not object
- missing `date_time`
- bad date format
- unsupported `type`
- oversized frame

### Step 7. Add `GET` for `date_time`
After `SET date_time` works, `GET` for `date_time` should be the next smallest useful feature.

---

## 16. Suggested Status Codes for Internal Use

Suggested internal enum:

```c
typedef enum
{
    HC_CMD_OK = 0,
    HC_CMD_ERR_BAD_JSON,
    HC_CMD_ERR_BAD_TYPE,
    HC_CMD_ERR_BAD_ARGS,
    HC_CMD_ERR_BAD_FIELD,
    HC_CMD_ERR_BAD_VALUE,
    HC_CMD_ERR_NOT_SUPPORTED,
    HC_CMD_ERR_INTERNAL
} hc_cmd_status_t;
```

This keeps parser/dispatcher/handler boundaries clean.

---

## 17. Recommended First Implementation Boundary

The first accepted TE packet should be exactly this pattern:

```json
{"type":"SET","msg":<number>,"args":{"date_time":"YYYYMMDD HH:MM:SS.MS"}}
```

All other packets should initially fail cleanly.

This is the right first boundary because it proves:
- USB line capture,
- JSON tokenization,
- field lookup,
- argument validation,
- state mutation,
- JSON response generation,
- TE-visible deterministic behavior.

---

## 18. Recommended Next Documentation Updates

Once implementation starts, the following documents should also be updated:
- `C:\Users\marty\Code\STM32\SBS\MCU-and-Controller-radiation-test\HC_FW_BlackPill\Docs\requirements\host_controller\hc_te_interface_spec_v1.md`
  - add concrete `SET date_time` and `GET date_time` field definitions
- `C:\Users\marty\Code\STM32\SBS\MCU-and-Controller-radiation-test\HC_FW_BlackPill\Docs\requirements\host_controller\hc_story_backlog_v1.md`
  - add implementation stories/tasks for JSONL command processor bring-up
- `C:\Users\marty\Code\STM32\SBS\MCU-and-Controller-radiation-test\HC_FW_BlackPill\Docs\requirements\host_controller\hc_protocol_test_plan_v1.md`
  - add parser and response test cases for `SET date_time`

---

## 19. Recommended Immediate Next Action

Implement the first vertical slice in this order:
1. USB VCP line accumulation
2. `hc_jsonl_cmd_process_line()`
3. `jsmn` parse helpers for `type`, `msg`, and `args.date_time`
4. `hc_datetime_set()`
5. `RSP` success formatter
6. `RSP` error formatter

Do **not** start with a generic full protocol engine.
Start with one working vertical slice and grow outward.
