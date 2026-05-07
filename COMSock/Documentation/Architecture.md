# COMSock Architecture

## Runtime Overview

COMSock is implemented as a standalone Windows Python/Tkinter application in
`src/comsock.py`.

The application has three main parts:

- Tkinter UI for COM-port selection, mapping display, and settings load/save.
- Win32 serial adapter using Python `ctypes` and `kernel32` APIs.
- Minimal asyncio WebSocket server implementing RFC 6455 handshakes and data
  frames without third-party dependencies.

## Mapping Model

The utility runs one WebSocket server on a configured host and TCP port. Each COM
port mapping receives a unique WebSocket path.

Example:

```text
COM3, 115200 baud, path /COM3 -> ws://127.0.0.1:8765/COM3
```

The UI table displays the COM port, baud rate, full WebSocket URL, and optional
label/description.

## Serial Interface

Serial ports are opened with Win32 `CreateFileW` using the `\\.\COMx` naming
form. This supports higher COM port numbers as well as `COM1` through `COM9`.

Current serial configuration:

- Baud rate: user configurable per mapping.
- Data bits: 8.
- Parity: none.
- Stop bits: 1.
- Flow control: software flow control disabled.
- DTR/RTS: enabled.

Each mapped serial port has a dedicated reader thread. Reads use bounded
timeouts to avoid permanent blocking during shutdown.

## WebSocket Interface

The WebSocket server accepts clients on mapped paths only. A client connecting to
an unmapped path receives HTTP 404.

Data behavior:

- WebSocket text and binary payloads are written to the mapped COM port.
- Bytes received from the COM port are broadcast to all connected clients on that
  mapping as binary WebSocket frames.
- Close frames are handled with a close response.

The implementation intentionally does not provide higher-level framing,
line-ending conversion, authentication, TLS, or message parsing.

## Settings File

Settings are stored as versioned JSON.

Example:

```json
{
  "version": 1,
  "host": "127.0.0.1",
  "port": 8765,
  "mappings": [
    {
      "com_port": "COM3",
      "baud": 115200,
      "path": "/COM3",
      "label": "Radiation tester control port"
    }
  ]
}
```

## Validation Notes

Validation performed without target hardware can confirm Python syntax, COM port
enumeration, settings serialization paths, and UI startup. Full validation
requires a real or loopback serial device and a WebSocket client.

Suggested hardware validation:

1. Connect a known USB serial device or serial loopback adapter.
2. Start COMSock and map the detected COM port.
3. Connect a WebSocket client to the displayed URL.
4. Send bytes from the WebSocket client and verify they appear on the serial
   device.
5. Send bytes from the serial device and verify they are received as WebSocket
   binary frames.
