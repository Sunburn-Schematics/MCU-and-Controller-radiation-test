# COMSock

COMSock is a Windows desktop utility that exposes selected hardware COM ports as
local WebSocket endpoints.

## Run

```powershell
python .\src\comsock.py
```

No third-party Python packages are required.

## Usage

1. Select a detected hardware COM port.
2. Choose a baud rate.
3. Optionally enter a WebSocket path and label.
4. Add one or more mappings.
5. Start the server.

Each row shows the exposed WebSocket URL, for example:

```text
ws://127.0.0.1:8765/COM3
```

Binary and text WebSocket payloads received from a client are written directly to
the mapped COM port. Bytes received from the COM port are broadcast to all
connected WebSocket clients on that mapping as binary frames.

## Settings

Use **Save Settings** and **Load Settings** in the UI to persist mappings as JSON.
The file stores the server host, server port, COM port, baud rate, WebSocket path,
and optional label.

## Notes

- The utility is Windows-specific because it uses Win32 serial APIs through
  Python `ctypes`.
- Serial configuration is fixed at 8 data bits, no parity, 1 stop bit, with DTR
  and RTS enabled.
- Hardware runtime behavior must be validated on the actual target setup.
