# CommandHandler

This folder contains the Host Controller JSONL command processor implementation scaffolding.

## Current scope

Initial vertical slice only:
- parse TE `SET` packet
- validate numeric `msg`
- validate `args.date_time`
- store current HC date/time value
- build `RSP` success or error packet

## Planned files

- `hc_cmd_types.h` — shared enums, structs, and constants
- `hc_jsonl_cmd.h/.c` — top-level command processor entry point
- `hc_jsonl_parse.h/.c` — JSON token lookup and extraction helpers using `jsmn`
- `hc_jsonl_dispatch.h/.c` — packet dispatch logic
- `hc_jsonl_fields.h/.c` — field-specific handlers, starting with `date_time`
- `hc_jsonl_rsp.h/.c` — response formatting helpers
- `hc_datetime.h/.c` — current HC date/time storage and validation
- `hc_comms_tx.h/.c` — transport abstraction for line-based TX

## Current integration status

These files are scaffolding only.
They are not yet wired into the rest of the firmware build or runtime path.
Integration into USB RX/TX and the application loop will be done later in controlled steps.

## Test Commands
```jsonl
{"type":"SET","msg":0,"args":{"date_time":"20260501 14:57:09"}}
{"type":"SET","msg":0,"args":{"date_time":"20260501 14:57:09","Something":"Does this work"}}
```