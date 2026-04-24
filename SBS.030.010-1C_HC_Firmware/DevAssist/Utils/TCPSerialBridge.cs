using System;
using System.IO;
using System.IO.Ports;
using System.Net;
using System.Net.Sockets;
using System.Threading;
using System.Collections.Generic;

class Bridge {
    static List<TcpClient> clients = new List<TcpClient>();
    static object clientLock = new object();
    static SerialPort serial;

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

        try {
            serial = new SerialPort(comPort, baud, Parity.None, 8, StopBits.One);
            serial.Open();
            Console.WriteLine("Opening " + comPort + " at " + baud + " baud...");
        } catch (Exception ex) {
            Console.WriteLine("Failed to open COM port: " + ex.Message);
            return;
        }

        Thread serialThread = new Thread(SerialToTcp);
        serialThread.IsBackground = true;
        serialThread.Start();

        TcpListener server = new TcpListener(IPAddress.Any, tcpPort);
        server.Start();
        Console.WriteLine("=== TCP Serial Bridge Active ===");
        Console.WriteLine("Listening on TCP localhost:" + tcpPort + " -> Mapped to " + comPort);
        Console.WriteLine("You can connect dynamically using:");
        Console.WriteLine("   PuTTY: Connection Type=Raw, Host=localhost, Port=5555");
        Console.WriteLine("   Netcat: ncat localhost 5555");

        try {
            while (true) {
                TcpClient client = server.AcceptTcpClient();
                Console.WriteLine("[+] Client connected from " + client.Client.RemoteEndPoint);
                lock (clientLock) {
                    clients.Add(client);
                }
                Thread t = new Thread(() => TcpToSerial(client));
                t.IsBackground = true;
                t.Start();
            }
        } catch (Exception) {
            Console.WriteLine("\nShutting down...");
        }
    }

    static void SerialToTcp() {
        byte[] buffer = new byte[4096];
        while (serial.IsOpen) {
            try {
                int bytesRead = serial.Read(buffer, 0, buffer.Length);
                if (bytesRead > 0) {
                    lock (clientLock) {
                        foreach (var client in clients) {
                            try {
                                client.GetStream().Write(buffer, 0, bytesRead);
                            } catch { }
                        }
                    }
                }
            } catch { break; }
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
            Console.WriteLine("[-] Client disconnected from " + client.Client.RemoteEndPoint);
            lock (clientLock) {
                clients.Remove(client);
            }
            client.Close();
        }
    }
}
