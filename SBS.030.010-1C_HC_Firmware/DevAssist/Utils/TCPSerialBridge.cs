using System;
using System.IO;
using System.IO.Ports;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Collections.Generic;
using System.Text;
using System.Security.Cryptography;

class Bridge {
    static List<TcpClient> tcpClients = new List<TcpClient>();
    static List<WebSocketClient> wsClients = new List<WebSocketClient>();
    static object clientLock = new object();
    static SerialPort serial;

    class WebSocketClient {
        public TcpClient Client;
        public NetworkStream Stream;
        public string Remote;
    }

    static void Main() {
        string envPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "env.json");
        string comPort = null;
        if (File.Exists(envPath)) {
            string env = File.ReadAllText(envPath);
            int idx = env.IndexOf("\"DEBUG_PORT\"");
            if (idx > 0) {
                int colon = env.IndexOf(':', idx);
                int startQuote = env.IndexOf('\"', colon);
                int endQuote = env.IndexOf('\"', startQuote + 1);
                if (startQuote > 0 && endQuote > startQuote) {
                    comPort = env.Substring(startQuote + 1, endQuote - startQuote - 1);
                }
            }
        }

        if (string.IsNullOrEmpty(comPort)) {
            Console.WriteLine("Error: Debug port not found in env.json. Run DiscoverDebugPort.exe first.");
            return;
        }

        int baud = 115200;
        int tcpPort = 5555;
        int wsPort = 5556;

        try {
            serial = new SerialPort(comPort, baud, Parity.None, 8, StopBits.One);
            serial.Open();
            Console.WriteLine("Opening " + comPort + " at " + baud + " baud...");
        } catch (Exception ex) {
            Console.WriteLine("Failed to open COM port: " + ex.Message);
            return;
        }

        Thread serialThread = new Thread(SerialToClients);
        serialThread.IsBackground = true;
        serialThread.Start();

        Thread tcpThread = new Thread(delegate() { RunTcpServer(tcpPort, comPort); });
        tcpThread.IsBackground = true;
        tcpThread.Start();

        Thread wsThread = new Thread(delegate() { RunWebSocketServer(wsPort, comPort); });
        wsThread.IsBackground = true;
        wsThread.Start();

        Console.WriteLine("=== TCP + WebSocket Serial Bridge Active ===");
        Console.WriteLine("TCP       : localhost:" + tcpPort + " -> " + comPort);
        Console.WriteLine("WebSocket : ws://localhost:" + wsPort + "/ -> " + comPort);
        Console.WriteLine("TCP clients: PuTTY Raw / ncat localhost 5555");
        Console.WriteLine("WS clients : Browser widget via ws://localhost:5556/");
        Console.WriteLine("Press Ctrl+C to stop.");

        while (serial.IsOpen) {
            Thread.Sleep(1000);
        }
    }

    static void RunTcpServer(int tcpPort, string comPort) {
        TcpListener server = new TcpListener(IPAddress.Loopback, tcpPort);
        server.Start();
        try {
            while (true) {
                TcpClient client = server.AcceptTcpClient();
                Console.WriteLine("[+] TCP client connected from " + client.Client.RemoteEndPoint);
                lock (clientLock) {
                    tcpClients.Add(client);
                }
                Thread t = new Thread(delegate() { TcpToSerial(client); });
                t.IsBackground = true;
                t.Start();
            }
        } catch (Exception ex) {
            Console.WriteLine("TCP server stopped: " + ex.Message);
        }
    }

    static void RunWebSocketServer(int wsPort, string comPort) {
        TcpListener server = new TcpListener(IPAddress.Loopback, wsPort);
        server.Start();
        try {
            while (true) {
                TcpClient client = server.AcceptTcpClient();
                Thread t = new Thread(delegate() { HandleWebSocketClient(client); });
                t.IsBackground = true;
                t.Start();
            }
        } catch (Exception ex) {
            Console.WriteLine("WebSocket server stopped: " + ex.Message);
        }
    }

    static void SerialToClients() {
        byte[] buffer = new byte[4096];
        while (serial.IsOpen) {
            try {
                int bytesRead = serial.Read(buffer, 0, buffer.Length);
                if (bytesRead > 0) {
                    BroadcastTcp(buffer, bytesRead);
                    BroadcastWebSocket(buffer, bytesRead);
                }
            } catch {
                break;
            }
        }
    }

    static void BroadcastTcp(byte[] buffer, int bytesRead) {
        List<TcpClient> dead = new List<TcpClient>();
        lock (clientLock) {
            foreach (TcpClient client in tcpClients) {
                try {
                    client.GetStream().Write(buffer, 0, bytesRead);
                } catch {
                    dead.Add(client);
                }
            }
            foreach (TcpClient client in dead) {
                tcpClients.Remove(client);
                try { client.Close(); } catch { }
            }
        }
    }

    static void BroadcastWebSocket(byte[] buffer, int bytesRead) {
        List<WebSocketClient> dead = new List<WebSocketClient>();
        lock (clientLock) {
            foreach (WebSocketClient client in wsClients) {
                try {
                    SendWebSocketFrame(client.Stream, buffer, bytesRead, 2);
                } catch {
                    dead.Add(client);
                }
            }
            foreach (WebSocketClient client in dead) {
                wsClients.Remove(client);
                try { client.Client.Close(); } catch { }
            }
        }
    }

    static void TcpToSerial(TcpClient client) {
        byte[] buffer = new byte[4096];
        try {
            while (client.Connected) {
                int bytesRead = client.GetStream().Read(buffer, 0, buffer.Length);
                if (bytesRead == 0) break;
                if (serial.IsOpen) {
                    serial.Write(buffer, 0, bytesRead);
                }
            }
        } catch { }
        finally {
            Console.WriteLine("[-] TCP client disconnected from " + client.Client.RemoteEndPoint);
            lock (clientLock) {
                tcpClients.Remove(client);
            }
            client.Close();
        }
    }

    static void HandleWebSocketClient(TcpClient client) {
        NetworkStream stream = null;
        WebSocketClient wsClient = null;
        try {
            stream = client.GetStream();
            if (!PerformWebSocketHandshake(stream)) {
                client.Close();
                return;
            }

            wsClient = new WebSocketClient();
            wsClient.Client = client;
            wsClient.Stream = stream;
            wsClient.Remote = client.Client.RemoteEndPoint.ToString();

            lock (clientLock) {
                wsClients.Add(wsClient);
            }
            Console.WriteLine("[+] WebSocket client connected from " + wsClient.Remote);

            while (client.Connected) {
                WebSocketFrame frame = ReadWebSocketFrame(stream);
                if (frame == null) {
                    break;
                }
                if (frame.Opcode == 8) {
                    break;
                }
                if (frame.Opcode == 9) {
                    SendWebSocketFrame(stream, frame.Payload, frame.Payload.Length, 10);
                    continue;
                }
                if (frame.Opcode == 1 || frame.Opcode == 2) {
                    if (serial.IsOpen && frame.Payload.Length > 0) {
                        serial.Write(frame.Payload, 0, frame.Payload.Length);
                    }
                }
            }
        } catch { }
        finally {
            if (wsClient != null) {
                Console.WriteLine("[-] WebSocket client disconnected from " + wsClient.Remote);
                lock (clientLock) {
                    wsClients.Remove(wsClient);
                }
            }
            try { client.Close(); } catch { }
        }
    }

    class WebSocketFrame {
        public int Opcode;
        public byte[] Payload;
    }

    static bool PerformWebSocketHandshake(NetworkStream stream) {
        string request = ReadHttpHeader(stream);
        if (string.IsNullOrEmpty(request) || request.IndexOf("Upgrade: websocket", StringComparison.OrdinalIgnoreCase) < 0) {
            return false;
        }

        string key = GetHeaderValue(request, "Sec-WebSocket-Key");
        if (string.IsNullOrEmpty(key)) {
            return false;
        }

        string accept = ComputeWebSocketAccept(key);
        string response =
            "HTTP/1.1 101 Switching Protocols\r\n" +
            "Upgrade: websocket\r\n" +
            "Connection: Upgrade\r\n" +
            "Sec-WebSocket-Accept: " + accept + "\r\n" +
            "\r\n";
        byte[] responseBytes = Encoding.ASCII.GetBytes(response);
        stream.Write(responseBytes, 0, responseBytes.Length);
        return true;
    }

    static string ReadHttpHeader(NetworkStream stream) {
        MemoryStream ms = new MemoryStream();
        byte[] one = new byte[1];
        while (true) {
            int read = stream.Read(one, 0, 1);
            if (read <= 0) {
                break;
            }
            ms.Write(one, 0, 1);
            if (ms.Length >= 4) {
                byte[] arr = ms.ToArray();
                int len = arr.Length;
                if (arr[len - 4] == 13 && arr[len - 3] == 10 && arr[len - 2] == 13 && arr[len - 1] == 10) {
                    break;
                }
            }
            if (ms.Length > 16384) {
                break;
            }
        }
        return Encoding.ASCII.GetString(ms.ToArray());
    }

    static string GetHeaderValue(string request, string headerName) {
        string[] lines = request.Split(new string[] { "\r\n" }, StringSplitOptions.None);
        string prefix = headerName + ":";
        foreach (string line in lines) {
            if (line.StartsWith(prefix, StringComparison.OrdinalIgnoreCase)) {
                return line.Substring(prefix.Length).Trim();
            }
        }
        return null;
    }

    static string ComputeWebSocketAccept(string key) {
        string magic = key.Trim() + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";
        SHA1 sha1 = SHA1.Create();
        byte[] hash = sha1.ComputeHash(Encoding.ASCII.GetBytes(magic));
        return Convert.ToBase64String(hash);
    }

    static WebSocketFrame ReadWebSocketFrame(NetworkStream stream) {
        int b1 = stream.ReadByte();
        if (b1 < 0) return null;
        int b2 = stream.ReadByte();
        if (b2 < 0) return null;

        int opcode = b1 & 0x0F;
        bool masked = (b2 & 0x80) != 0;
        ulong payloadLen = (ulong)(b2 & 0x7F);

        if (payloadLen == 126) {
            byte[] ext = ReadExact(stream, 2);
            payloadLen = (ulong)((ext[0] << 8) | ext[1]);
        } else if (payloadLen == 127) {
            byte[] ext = ReadExact(stream, 8);
            payloadLen = 0;
            for (int i = 0; i < 8; i++) {
                payloadLen = (payloadLen << 8) | ext[i];
            }
        }

        byte[] mask = masked ? ReadExact(stream, 4) : null;
        byte[] payload = ReadExact(stream, (int)payloadLen);

        if (masked && mask != null) {
            for (int i = 0; i < payload.Length; i++) {
                payload[i] = (byte)(payload[i] ^ mask[i % 4]);
            }
        }

        WebSocketFrame frame = new WebSocketFrame();
        frame.Opcode = opcode;
        frame.Payload = payload;
        return frame;
    }

    static byte[] ReadExact(NetworkStream stream, int count) {
        byte[] buffer = new byte[count];
        int offset = 0;
        while (offset < count) {
            int read = stream.Read(buffer, offset, count - offset);
            if (read <= 0) {
                throw new IOException("Socket closed");
            }
            offset += read;
        }
        return buffer;
    }

    static void SendWebSocketFrame(NetworkStream stream, byte[] payload, int payloadLength, int opcode) {
        MemoryStream frame = new MemoryStream();
        frame.WriteByte((byte)(0x80 | (opcode & 0x0F)));
        if (payloadLength <= 125) {
            frame.WriteByte((byte)payloadLength);
        } else if (payloadLength <= 65535) {
            frame.WriteByte(126);
            frame.WriteByte((byte)((payloadLength >> 8) & 0xFF));
            frame.WriteByte((byte)(payloadLength & 0xFF));
        } else {
            frame.WriteByte(127);
            ulong len = (ulong)payloadLength;
            for (int i = 7; i >= 0; i--) {
                frame.WriteByte((byte)((len >> (8 * i)) & 0xFF));
            }
        }
        frame.Write(payload, 0, payloadLength);
        byte[] bytes = frame.ToArray();
        stream.Write(bytes, 0, bytes.Length);
    }
}
