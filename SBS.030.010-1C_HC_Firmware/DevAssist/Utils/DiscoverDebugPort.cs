using System;
using System.IO;
using System.Threading;
using System.Collections.Generic;
using Microsoft.Win32;

class Discover {
    static HashSet<string> GetComPorts() {
        var ports = new HashSet<string>();
        try {
            using (RegistryKey key = Registry.LocalMachine.OpenSubKey(@"HARDWARE\DEVICEMAP\SERIALCOMM")) {
                if (key != null) {
                    foreach (string name in key.GetValueNames()) {
                        ports.Add(key.GetValue(name).ToString());
                    }
                }
            }
        } catch {}
        return ports;
    }

    static void Main() {
        Console.WriteLine("=== DevAssist: Debug Port Discovery ===");
        Console.WriteLine("1. Please UNPLUG your USB/RS232 adapter from your PC.");
        Console.Write("   Press ENTER when ready...");
        Console.ReadLine();

        var baseline = GetComPorts();
        Console.WriteLine("   Baseline ports detected: " + (baseline.Count > 0 ? string.Join(", ", baseline) : "None"));

        Console.WriteLine("\n2. Please PLUG IN your USB/RS232 adapter.");
        Console.Write("   Press ENTER when ready...");
        Console.ReadLine();
        Thread.Sleep(2000);

        var current = GetComPorts();
        Console.WriteLine("   Current ports detected: " + (current.Count > 0 ? string.Join(", ", current) : "None"));

        current.ExceptWith(baseline);

        if (current.Count == 1) {
            string port = new List<string>(current)[0];
            Console.WriteLine("\n[SUCCESS] Debug adapter identified on: " + port);
            string json = "{\"DEBUG_PORT\": \"" + port + "\"}";
            string envPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "env.json");
            File.WriteAllText(envPath, json);
            Console.WriteLine("   Saved configuration to " + envPath);
        } else if (current.Count == 0) {
            Console.WriteLine("\n[FAIL] No new COM ports detected. Was it properly plugged in?");
        } else {
            Console.WriteLine("\n[FAIL] Multiple new COM ports detected. Cannot confidently isolate.");
        }
    }
}
