# Functional Outcomes

## FO-001: Select COM Ports

The user shall be able to discover available Windows hardware COM ports and
select one or more ports to expose.

## FO-002: Expose COM Ports as WebSockets

The utility shall expose each selected COM port as a WebSocket endpoint.

Each mapping shall use one local WebSocket server host/port and a unique path per
COM port.

## FO-003: Display Active Mappings

The UI shall display the COM port, baud rate, WebSocket URL, and optional
label/description for each configured mapping.

## FO-004: Optional Label

The user shall be able to assign an optional label or description to each COM
port mapping.

## FO-005: Persist Settings

The user shall be able to save and load settings from a file.

Saved settings shall include server host, server port, COM port, baud rate,
WebSocket path, and optional label/description.

## FO-006: Bidirectional Data Flow

The utility shall forward WebSocket client payloads to the mapped COM port and
forward bytes received from the COM port to connected WebSocket clients.
