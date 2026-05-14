# Logging

Host-side logging helpers for SBS RadTest CDC USB serial devices.

## `launch.ps1`

Discovers connected USB serial ports whose USB product string matches `SBS RadTest CDC`, initializes each available device, and starts a live logger terminal for each one.

For each device, the script:

1. Opens the COM port.
2. Waits for startup backlog data to drain.
3. Sends `SET date_time` using the host's current local time.
4. Sends `GET sw_version`.
5. Displays the device HC ID and firmware version responses.
6. Warns if devices report different firmware versions.
7. Warns if multiple devices report the same HC ID.
8. Starts `serial_logger.ps1` in Windows Terminal unless `-SetTimeOnly` is used.
9. Allocates one localhost raw TCP endpoint per logger for terminal monitors.
10. Allocates one localhost WebSocket endpoint per logger for WebSocket clients.

Log files are written in this folder by default:

```text
YYYYMMDD_HHMM_<hc_id>.log
```

If duplicate HC IDs are detected, suffixes are appended to keep filenames unique:

```text
YYYYMMDD_HHMM_<hc_id>A.log
YYYYMMDD_HHMM_<hc_id>B.log
```

### Usage

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Logging\launch.ps1
```

Set time and read firmware versions without launching logger terminals:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Logging\launch.ps1 -SetTimeOnly
```

### Options

| Option | Default | Description |
| --- | --- | --- |
| `-ProductString` | `SBS RadTest CDC` | USB product string filter used to select target COM ports. |
| `-BaudRate` | `115200` | Serial baud rate for initialization and logger sessions. |
| `-OpenFlushDelayMs` | `2000` | Delay after opening a COM port before commands are sent; lets backed-up serial data drain. |
| `-ReadTimeoutMs` | `3000` | Timeout for command response reads. |
| `-TerminalPath` | `wt.exe` | Terminal executable used to launch logger sessions. |
| `-LogDirectory` | script folder | Directory where log files are created. |
| `-TcpBasePort` | `9765` | First localhost TCP port considered for raw TCP monitor endpoints. Set to `0` to disable raw TCP endpoints. |
| `-WebSocketBasePort` | `8765` | First localhost TCP port considered for logger WebSocket endpoints. Set to `0` to disable WebSocket endpoints. |
| `-SetTimeOnly` | off | Runs discovery, `SET date_time`, and `GET sw_version`, but does not start loggers. |

## `serial_logger.ps1`

Attaches to one COM port, displays incoming serial data in the terminal, writes the same data to a log file, and can broadcast the serial receive stream to multiple local raw TCP and WebSocket listeners.

This script is normally started by `launch.ps1`, but can also be run directly for testing.

### Usage

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Logging\serial_logger.ps1 -PortName COM7 -LogPath Logging\manual.log
```

Run directly with a WebSocket endpoint:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Logging\serial_logger.ps1 -PortName COM7 -LogPath Logging\manual.log -WebSocketPort 8765 -WebSocketPath /COM7
```

Run directly with a raw TCP endpoint suitable for terminal monitors:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File Logging\serial_logger.ps1 -PortName COM7 -LogPath Logging\manual.log -TcpPort 9765
```

### Options

| Option | Default | Description |
| --- | --- | --- |
| `-PortName` | required | COM port to open, for example `COM7`. |
| `-LogPath` | required | Path to the log file to write. |
| `-BaudRate` | `115200` | Serial baud rate. |
| `-TcpPort` | `0` | Localhost raw TCP port for terminal monitors. `0` disables raw TCP support. |
| `-WebSocketPort` | `0` | Localhost TCP port for the WebSocket listener. `0` disables WebSocket support. |
| `-WebSocketPath` | `/<PortName>` | WebSocket path. A leading and trailing slash are added if omitted. |

Stop a logger with `Ctrl+C` in its terminal window.

TCP and WebSocket endpoints are bound to `127.0.0.1` only. Raw TCP clients receive the ASCII bytes from the COM port directly. WebSocket clients receive the same bytes as binary frames. These endpoints are monitor-only; incoming TCP/WebSocket data is not written back to the COM port.
