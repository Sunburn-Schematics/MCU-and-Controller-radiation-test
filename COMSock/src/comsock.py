"""COMSock: expose Windows hardware COM ports as WebSocket endpoints.

This program intentionally uses only the Python standard library so it can run
on a clean Windows Python install without pyserial or third-party WebSocket
packages.
"""

from __future__ import annotations

import asyncio
import base64
import ctypes
import ctypes.wintypes as wt
import hashlib
import json
import queue
import re
import socket
import struct
import threading
import time
import tkinter as tk
from dataclasses import dataclass, field
from pathlib import Path
from tkinter import filedialog, messagebox, ttk
from typing import Callable
from winreg import HKEY_LOCAL_MACHINE, EnumValue, OpenKey


APP_NAME = "COMSock"
DEFAULT_WS_HOST = "127.0.0.1"
DEFAULT_WS_PORT = 8765
DEFAULT_BAUD = 115200
CONFIG_VERSION = 1
INVALID_HANDLE_VALUE = wt.HANDLE(-1).value


kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)


class DCB(ctypes.Structure):
    _fields_ = [
        ("DCBlength", wt.DWORD),
        ("BaudRate", wt.DWORD),
        ("fBinary", wt.DWORD, 1),
        ("fParity", wt.DWORD, 1),
        ("fOutxCtsFlow", wt.DWORD, 1),
        ("fOutxDsrFlow", wt.DWORD, 1),
        ("fDtrControl", wt.DWORD, 2),
        ("fDsrSensitivity", wt.DWORD, 1),
        ("fTXContinueOnXoff", wt.DWORD, 1),
        ("fOutX", wt.DWORD, 1),
        ("fInX", wt.DWORD, 1),
        ("fErrorChar", wt.DWORD, 1),
        ("fNull", wt.DWORD, 1),
        ("fRtsControl", wt.DWORD, 2),
        ("fAbortOnError", wt.DWORD, 1),
        ("fDummy2", wt.DWORD, 17),
        ("wReserved", wt.WORD),
        ("XonLim", wt.WORD),
        ("XoffLim", wt.WORD),
        ("ByteSize", wt.BYTE),
        ("Parity", wt.BYTE),
        ("StopBits", wt.BYTE),
        ("XonChar", ctypes.c_char),
        ("XoffChar", ctypes.c_char),
        ("ErrorChar", ctypes.c_char),
        ("EofChar", ctypes.c_char),
        ("EvtChar", ctypes.c_char),
        ("wReserved1", wt.WORD),
    ]


class COMMTIMEOUTS(ctypes.Structure):
    _fields_ = [
        ("ReadIntervalTimeout", wt.DWORD),
        ("ReadTotalTimeoutMultiplier", wt.DWORD),
        ("ReadTotalTimeoutConstant", wt.DWORD),
        ("WriteTotalTimeoutMultiplier", wt.DWORD),
        ("WriteTotalTimeoutConstant", wt.DWORD),
    ]


kernel32.CreateFileW.argtypes = [
    wt.LPCWSTR,
    wt.DWORD,
    wt.DWORD,
    wt.LPVOID,
    wt.DWORD,
    wt.DWORD,
    wt.HANDLE,
]
kernel32.CreateFileW.restype = wt.HANDLE
kernel32.CloseHandle.argtypes = [wt.HANDLE]
kernel32.CloseHandle.restype = wt.BOOL
kernel32.GetCommState.argtypes = [wt.HANDLE, ctypes.POINTER(DCB)]
kernel32.GetCommState.restype = wt.BOOL
kernel32.SetCommState.argtypes = [wt.HANDLE, ctypes.POINTER(DCB)]
kernel32.SetCommState.restype = wt.BOOL
kernel32.SetCommTimeouts.argtypes = [wt.HANDLE, ctypes.POINTER(COMMTIMEOUTS)]
kernel32.SetCommTimeouts.restype = wt.BOOL
kernel32.ReadFile.argtypes = [wt.HANDLE, wt.LPVOID, wt.DWORD, ctypes.POINTER(wt.DWORD), wt.LPVOID]
kernel32.ReadFile.restype = wt.BOOL
kernel32.WriteFile.argtypes = [wt.HANDLE, wt.LPCVOID, wt.DWORD, ctypes.POINTER(wt.DWORD), wt.LPVOID]
kernel32.WriteFile.restype = wt.BOOL


def win_error(message: str) -> OSError:
    return ctypes.WinError(ctypes.get_last_error(), message)


def enumerate_com_ports() -> list[str]:
    ports: set[str] = set()
    try:
        with OpenKey(HKEY_LOCAL_MACHINE, r"HARDWARE\DEVICEMAP\SERIALCOMM") as key:
            index = 0
            while True:
                try:
                    _, value, _ = EnumValue(key, index)
                    ports.add(str(value))
                    index += 1
                except OSError:
                    break
    except OSError:
        pass

    # Some USB serial drivers do not use contiguous SerialN value names.
    if not ports:
        for idx in range(1, 257):
            name = f"COM{idx}"
            if _can_query_dos_device(name):
                ports.add(name)
    return sorted(ports, key=_natural_com_key)


def _can_query_dos_device(name: str) -> bool:
    buffer = ctypes.create_unicode_buffer(1024)
    return bool(kernel32.QueryDosDeviceW(name, buffer, len(buffer)))


def _natural_com_key(name: str) -> tuple[int, str]:
    match = re.fullmatch(r"COM(\d+)", name.upper())
    if match:
        return int(match.group(1)), name
    return 9999, name


class SerialPort:
    def __init__(self, name: str, baud: int) -> None:
        self.name = name.upper()
        self.baud = baud
        self._handle: int | None = None
        self._write_lock = threading.Lock()

    def open(self) -> None:
        path = rf"\\.\{self.name}"
        handle = kernel32.CreateFileW(
            path,
            0xC0000000,  # GENERIC_READ | GENERIC_WRITE
            0,
            None,
            3,  # OPEN_EXISTING
            0,
            None,
        )
        if handle == INVALID_HANDLE_VALUE:
            raise win_error(f"Could not open {self.name}")

        self._handle = int(handle)
        try:
            dcb = DCB()
            dcb.DCBlength = ctypes.sizeof(DCB)
            if not kernel32.GetCommState(self._handle, ctypes.byref(dcb)):
                raise win_error(f"Could not read {self.name} serial settings")
            dcb.BaudRate = self.baud
            dcb.ByteSize = 8
            dcb.Parity = 0
            dcb.StopBits = 0
            dcb.fBinary = 1
            dcb.fParity = 0
            dcb.fOutxCtsFlow = 0
            dcb.fOutxDsrFlow = 0
            dcb.fOutX = 0
            dcb.fInX = 0
            dcb.fDtrControl = 1  # DTR_CONTROL_ENABLE
            dcb.fRtsControl = 1  # RTS_CONTROL_ENABLE
            if not kernel32.SetCommState(self._handle, ctypes.byref(dcb)):
                raise win_error(f"Could not configure {self.name}")

            timeouts = COMMTIMEOUTS(
                ReadIntervalTimeout=50,
                ReadTotalTimeoutMultiplier=0,
                ReadTotalTimeoutConstant=50,
                WriteTotalTimeoutMultiplier=0,
                WriteTotalTimeoutConstant=1000,
            )
            if not kernel32.SetCommTimeouts(self._handle, ctypes.byref(timeouts)):
                raise win_error(f"Could not configure {self.name} timeouts")
        except Exception:
            self.close()
            raise

    def close(self) -> None:
        if self._handle is not None:
            kernel32.CloseHandle(self._handle)
            self._handle = None

    def read(self, size: int = 4096) -> bytes:
        if self._handle is None:
            return b""
        buffer = ctypes.create_string_buffer(size)
        read = wt.DWORD()
        ok = kernel32.ReadFile(self._handle, buffer, size, ctypes.byref(read), None)
        if not ok:
            raise win_error(f"Read failed on {self.name}")
        return buffer.raw[: read.value]

    def write(self, data: bytes) -> None:
        if self._handle is None or not data:
            return
        with self._write_lock:
            written = wt.DWORD()
            ok = kernel32.WriteFile(self._handle, data, len(data), ctypes.byref(written), None)
            if not ok or written.value != len(data):
                raise win_error(f"Write failed on {self.name}")


@dataclass
class PortConfig:
    com_port: str
    baud: int = DEFAULT_BAUD
    path: str = ""
    label: str = ""

    def normalized_path(self) -> str:
        if self.path:
            path = self.path.strip()
            return path if path.startswith("/") else f"/{path}"
        return f"/{self.com_port.upper()}"


@dataclass
class RuntimePort:
    config: PortConfig
    serial: SerialPort
    clients: set[asyncio.StreamWriter] = field(default_factory=set)
    clients_lock: threading.Lock = field(default_factory=threading.Lock)
    stop_event: threading.Event = field(default_factory=threading.Event)
    thread: threading.Thread | None = None

    async def broadcast(self, data: bytes) -> None:
        frame = websocket_frame(data, opcode=2)
        stale: list[asyncio.StreamWriter] = []
        with self.clients_lock:
            clients = list(self.clients)
        for writer in clients:
            try:
                writer.write(frame)
                await writer.drain()
            except (ConnectionError, OSError, RuntimeError):
                stale.append(writer)
        if stale:
            with self.clients_lock:
                for writer in stale:
                    self.clients.discard(writer)


class BridgeController:
    def __init__(self, status_cb: Callable[[str], None]) -> None:
        self.status_cb = status_cb
        self.loop: asyncio.AbstractEventLoop | None = None
        self.server: asyncio.AbstractServer | None = None
        self.thread: threading.Thread | None = None
        self.runtime_by_path: dict[str, RuntimePort] = {}
        self.running = threading.Event()

    def start(self, host: str, port: int, configs: list[PortConfig]) -> None:
        if self.running.is_set():
            raise RuntimeError("Bridge is already running")
        if not configs:
            raise RuntimeError("Select at least one COM port")

        paths = [cfg.normalized_path() for cfg in configs]
        if len(paths) != len(set(paths)):
            raise RuntimeError("Each WebSocket path must be unique")

        for cfg in configs:
            serial = SerialPort(cfg.com_port, cfg.baud)
            serial.open()
            runtime = RuntimePort(config=cfg, serial=serial)
            self.runtime_by_path[cfg.normalized_path()] = runtime

        self.loop = asyncio.new_event_loop()
        self.thread = threading.Thread(
            target=self._run_loop,
            args=(host, port),
            name="COMSock-WebSocket",
            daemon=True,
        )
        self.thread.start()
        started_at = time.monotonic()
        while not self.running.is_set():
            if time.monotonic() - started_at > 5:
                self.stop()
                raise RuntimeError("Timed out while starting WebSocket server")
            time.sleep(0.02)

        for runtime in self.runtime_by_path.values():
            runtime.thread = threading.Thread(
                target=self._read_serial,
                args=(runtime,),
                name=f"COMSock-{runtime.config.com_port}",
                daemon=True,
            )
            runtime.thread.start()

    def stop(self) -> None:
        for runtime in self.runtime_by_path.values():
            runtime.stop_event.set()
        if self.loop and self.loop.is_running():
            future = asyncio.run_coroutine_threadsafe(self._shutdown_async(), self.loop)
            try:
                future.result(timeout=3)
            except Exception:
                pass
            self.loop.call_soon_threadsafe(self.loop.stop)
        if self.thread:
            self.thread.join(timeout=3)
        for runtime in self.runtime_by_path.values():
            runtime.serial.close()
        self.runtime_by_path.clear()
        self.server = None
        self.loop = None
        self.thread = None
        self.running.clear()

    def _run_loop(self, host: str, port: int) -> None:
        assert self.loop is not None
        asyncio.set_event_loop(self.loop)
        try:
            self.server = self.loop.run_until_complete(
                asyncio.start_server(self._handle_client, host, port)
            )
            self.running.set()
            self.loop.run_forever()
        except Exception as exc:
            self.status_cb(f"Server error: {exc}")
        finally:
            self.loop.run_until_complete(self._shutdown_async())
            self.loop.close()

    async def _shutdown_async(self) -> None:
        if self.server:
            self.server.close()
            await self.server.wait_closed()
        for runtime in self.runtime_by_path.values():
            with runtime.clients_lock:
                clients = list(runtime.clients)
                runtime.clients.clear()
            for writer in clients:
                try:
                    writer.close()
                    await writer.wait_closed()
                except Exception:
                    pass

    def _read_serial(self, runtime: RuntimePort) -> None:
        while not runtime.stop_event.is_set():
            try:
                data = runtime.serial.read()
                if data and self.loop and self.loop.is_running():
                    asyncio.run_coroutine_threadsafe(runtime.broadcast(data), self.loop)
            except Exception as exc:
                self.status_cb(f"{runtime.config.com_port}: {exc}")
                break

    async def _handle_client(
        self, reader: asyncio.StreamReader, writer: asyncio.StreamWriter
    ) -> None:
        runtime: RuntimePort | None = None
        try:
            headers = await read_http_headers(reader)
            path = headers.get(":path", "/")
            runtime = self.runtime_by_path.get(path)
            if runtime is None:
                await reject_http(writer, 404, "No COM port mapped to this path")
                return
            key = headers.get("sec-websocket-key")
            if not key:
                await reject_http(writer, 400, "Missing Sec-WebSocket-Key")
                return

            accept = base64.b64encode(
                hashlib.sha1((key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11").encode()).digest()
            ).decode("ascii")
            response = (
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                f"Sec-WebSocket-Accept: {accept}\r\n\r\n"
            )
            writer.write(response.encode("ascii"))
            await writer.drain()
            with runtime.clients_lock:
                runtime.clients.add(writer)

            while True:
                opcode, payload = await read_ws_frame(reader)
                if opcode == 8:
                    writer.write(websocket_frame(b"", opcode=8))
                    await writer.drain()
                    return
                if opcode in (1, 2):
                    runtime.serial.write(payload)
        except (asyncio.IncompleteReadError, ConnectionError, OSError):
            pass
        except Exception as exc:
            self.status_cb(f"WebSocket client error: {exc}")
        finally:
            if runtime is not None:
                with runtime.clients_lock:
                    runtime.clients.discard(writer)
            try:
                writer.close()
                await writer.wait_closed()
            except Exception:
                pass


async def read_http_headers(reader: asyncio.StreamReader) -> dict[str, str]:
    raw = await reader.readuntil(b"\r\n\r\n")
    text = raw.decode("iso-8859-1")
    lines = text.split("\r\n")
    request = lines[0].split()
    headers: dict[str, str] = {}
    if len(request) >= 2:
        headers[":path"] = request[1]
    for line in lines[1:]:
        if ":" in line:
            key, value = line.split(":", 1)
            headers[key.strip().lower()] = value.strip()
    return headers


async def reject_http(writer: asyncio.StreamWriter, code: int, message: str) -> None:
    body = message.encode("utf-8")
    reason = {400: "Bad Request", 404: "Not Found"}.get(code, "Error")
    writer.write(
        (
            f"HTTP/1.1 {code} {reason}\r\n"
            "Connection: close\r\n"
            f"Content-Length: {len(body)}\r\n\r\n"
        ).encode("ascii")
        + body
    )
    await writer.drain()
    writer.close()
    await writer.wait_closed()


async def read_ws_frame(reader: asyncio.StreamReader) -> tuple[int, bytes]:
    first_two = await reader.readexactly(2)
    b1, b2 = first_two[0], first_two[1]
    opcode = b1 & 0x0F
    masked = bool(b2 & 0x80)
    length = b2 & 0x7F
    if length == 126:
        length = struct.unpack("!H", await reader.readexactly(2))[0]
    elif length == 127:
        length = struct.unpack("!Q", await reader.readexactly(8))[0]
    mask = await reader.readexactly(4) if masked else b""
    payload = await reader.readexactly(length) if length else b""
    if masked:
        payload = bytes(byte ^ mask[idx % 4] for idx, byte in enumerate(payload))
    return opcode, payload


def websocket_frame(payload: bytes, opcode: int = 2) -> bytes:
    first = 0x80 | opcode
    length = len(payload)
    if length < 126:
        header = struct.pack("!BB", first, length)
    elif length <= 0xFFFF:
        header = struct.pack("!BBH", first, 126, length)
    else:
        header = struct.pack("!BBQ", first, 127, length)
    return header + payload


class ComSockApp(tk.Tk):
    def __init__(self) -> None:
        super().__init__()
        self.title(APP_NAME)
        self.geometry("980x540")
        self.minsize(860, 460)
        self.status_queue: queue.Queue[str] = queue.Queue()
        self.controller = BridgeController(self.post_status)
        self.mappings: list[PortConfig] = []

        self.host_var = tk.StringVar(value=DEFAULT_WS_HOST)
        self.port_var = tk.StringVar(value=str(DEFAULT_WS_PORT))
        self.com_var = tk.StringVar()
        self.baud_var = tk.StringVar(value=str(DEFAULT_BAUD))
        self.label_var = tk.StringVar()
        self.path_var = tk.StringVar()
        self.status_var = tk.StringVar(value="Stopped")

        self._build_ui()
        self.refresh_ports()
        self.after(100, self._drain_status)
        self.protocol("WM_DELETE_WINDOW", self._on_close)

    def _build_ui(self) -> None:
        root = ttk.Frame(self, padding=12)
        root.pack(fill=tk.BOTH, expand=True)

        server = ttk.LabelFrame(root, text="WebSocket Server", padding=10)
        server.pack(fill=tk.X)
        ttk.Label(server, text="Host").grid(row=0, column=0, sticky=tk.W)
        ttk.Entry(server, textvariable=self.host_var, width=18).grid(row=0, column=1, padx=(6, 16))
        ttk.Label(server, text="Port").grid(row=0, column=2, sticky=tk.W)
        ttk.Entry(server, textvariable=self.port_var, width=8).grid(row=0, column=3, padx=(6, 16))
        ttk.Button(server, text="Start", command=self.start_bridge).grid(row=0, column=4, padx=3)
        ttk.Button(server, text="Stop", command=self.stop_bridge).grid(row=0, column=5, padx=3)
        ttk.Label(server, textvariable=self.status_var).grid(row=0, column=6, padx=(20, 0), sticky=tk.W)
        server.columnconfigure(6, weight=1)

        add = ttk.LabelFrame(root, text="Add COM Port", padding=10)
        add.pack(fill=tk.X, pady=(10, 0))
        ttk.Label(add, text="COM Port").grid(row=0, column=0, sticky=tk.W)
        self.com_combo = ttk.Combobox(add, textvariable=self.com_var, width=14, state="readonly")
        self.com_combo.grid(row=0, column=1, padx=(6, 8))
        ttk.Button(add, text="Refresh", command=self.refresh_ports).grid(row=0, column=2, padx=(0, 16))
        ttk.Label(add, text="Baud").grid(row=0, column=3, sticky=tk.W)
        ttk.Entry(add, textvariable=self.baud_var, width=10).grid(row=0, column=4, padx=(6, 16))
        ttk.Label(add, text="Path").grid(row=0, column=5, sticky=tk.W)
        ttk.Entry(add, textvariable=self.path_var, width=16).grid(row=0, column=6, padx=(6, 16))
        ttk.Label(add, text="Label").grid(row=0, column=7, sticky=tk.W)
        ttk.Entry(add, textvariable=self.label_var, width=24).grid(row=0, column=8, padx=(6, 16))
        ttk.Button(add, text="Add", command=self.add_mapping).grid(row=0, column=9)
        add.columnconfigure(8, weight=1)

        columns = ("com", "baud", "url", "label")
        self.tree = ttk.Treeview(root, columns=columns, show="headings", selectmode="extended")
        self.tree.heading("com", text="COM Port")
        self.tree.heading("baud", text="Baud")
        self.tree.heading("url", text="WebSocket URL")
        self.tree.heading("label", text="Label / Description")
        self.tree.column("com", width=100, stretch=False)
        self.tree.column("baud", width=90, stretch=False)
        self.tree.column("url", width=340)
        self.tree.column("label", width=260)
        self.tree.pack(fill=tk.BOTH, expand=True, pady=(10, 0))

        actions = ttk.Frame(root)
        actions.pack(fill=tk.X, pady=(10, 0))
        ttk.Button(actions, text="Remove Selected", command=self.remove_selected).pack(side=tk.LEFT)
        ttk.Button(actions, text="Load Settings", command=self.load_settings).pack(side=tk.RIGHT, padx=(6, 0))
        ttk.Button(actions, text="Save Settings", command=self.save_settings).pack(side=tk.RIGHT)

    def refresh_ports(self) -> None:
        ports = enumerate_com_ports()
        self.com_combo["values"] = ports
        if ports and self.com_var.get() not in ports:
            self.com_var.set(ports[0])
        elif not ports:
            self.com_var.set("")
        self.post_status(f"Found {len(ports)} COM port(s)")

    def add_mapping(self) -> None:
        try:
            com_port = self.com_var.get().strip().upper()
            if not com_port:
                raise ValueError("Select a COM port")
            baud = int(self.baud_var.get())
            if baud <= 0:
                raise ValueError("Baud must be positive")
            cfg = PortConfig(
                com_port=com_port,
                baud=baud,
                path=self.path_var.get().strip() or f"/{com_port}",
                label=self.label_var.get().strip(),
            )
            if cfg.normalized_path() in [item.normalized_path() for item in self.mappings]:
                raise ValueError("WebSocket path already exists")
            if cfg.com_port in [item.com_port for item in self.mappings]:
                raise ValueError("COM port is already mapped")
            self.mappings.append(cfg)
            self._refresh_table()
            self.path_var.set("")
            self.label_var.set("")
        except Exception as exc:
            messagebox.showerror(APP_NAME, str(exc))

    def remove_selected(self) -> None:
        selected = set(self.tree.selection())
        self.mappings = [cfg for idx, cfg in enumerate(self.mappings) if str(idx) not in selected]
        self._refresh_table()

    def start_bridge(self) -> None:
        try:
            self.controller.start(self.host_var.get().strip(), int(self.port_var.get()), list(self.mappings))
            self.status_var.set("Running")
            self._refresh_table()
        except Exception as exc:
            messagebox.showerror(APP_NAME, str(exc))
            self.controller.stop()
            self.status_var.set("Stopped")

    def stop_bridge(self) -> None:
        self.controller.stop()
        self.status_var.set("Stopped")

    def save_settings(self) -> None:
        path = filedialog.asksaveasfilename(
            title="Save COMSock Settings",
            defaultextension=".json",
            filetypes=[("JSON settings", "*.json"), ("All files", "*.*")],
        )
        if not path:
            return
        data = {
            "version": CONFIG_VERSION,
            "host": self.host_var.get().strip(),
            "port": int(self.port_var.get()),
            "mappings": [cfg.__dict__ for cfg in self.mappings],
        }
        Path(path).write_text(json.dumps(data, indent=2), encoding="utf-8")
        self.post_status(f"Saved settings to {path}")

    def load_settings(self) -> None:
        path = filedialog.askopenfilename(
            title="Load COMSock Settings",
            filetypes=[("JSON settings", "*.json"), ("All files", "*.*")],
        )
        if not path:
            return
        try:
            data = json.loads(Path(path).read_text(encoding="utf-8"))
            if data.get("version") != CONFIG_VERSION:
                raise ValueError("Unsupported settings file version")
            self.host_var.set(str(data.get("host", DEFAULT_WS_HOST)))
            self.port_var.set(str(data.get("port", DEFAULT_WS_PORT)))
            self.mappings = [
                PortConfig(
                    com_port=str(item["com_port"]).upper(),
                    baud=int(item.get("baud", DEFAULT_BAUD)),
                    path=str(item.get("path", "")),
                    label=str(item.get("label", "")),
                )
                for item in data.get("mappings", [])
            ]
            self._refresh_table()
            self.post_status(f"Loaded settings from {path}")
        except Exception as exc:
            messagebox.showerror(APP_NAME, str(exc))

    def _refresh_table(self) -> None:
        for row in self.tree.get_children():
            self.tree.delete(row)
        for idx, cfg in enumerate(self.mappings):
            url = f"ws://{self.host_var.get().strip()}:{self.port_var.get().strip()}{cfg.normalized_path()}"
            self.tree.insert("", tk.END, iid=str(idx), values=(cfg.com_port, cfg.baud, url, cfg.label))

    def post_status(self, message: str) -> None:
        self.status_queue.put(message)

    def _drain_status(self) -> None:
        try:
            while True:
                self.status_var.set(self.status_queue.get_nowait())
        except queue.Empty:
            pass
        self.after(100, self._drain_status)

    def _on_close(self) -> None:
        self.stop_bridge()
        self.destroy()


def main() -> None:
    if socket.gethostname() == "":
        raise RuntimeError("Socket subsystem unavailable")
    app = ComSockApp()
    app.mainloop()


if __name__ == "__main__":
    main()
