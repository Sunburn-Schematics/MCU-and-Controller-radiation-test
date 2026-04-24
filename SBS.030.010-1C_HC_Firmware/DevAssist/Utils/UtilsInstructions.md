# DevAssist Utilities Compilation Guide

This folder contains the native C# source code for the DevAssist serial infrastructure tools (`DiscoverDebugPort` and `TCPSerialBridge`). 

To maintain a strict zero-dependency environment, these tools do NOT require Visual Studio or Python to be installed. They are compiled using the native `.NET Framework` C# compiler (`csc.exe`) that is built into every modern Windows OS by default.

## Compilation Instructions

If you make changes to the `.cs` files and need to regenerate the `.exe` binaries, follow these steps:

1. Open a **PowerShell** or **Command Prompt** window.
2. Navigate to the root of the project repository:
   ```powershell
   cd C:\Users\marty\Code\STM32\SBS.030.010-1C_HC_Firmware
   ```
3. Run the following built-in Windows compiler commands to compile the `.exe` files directly into the `DevAssist` folder:

   **Compile DiscoverDebugPort:**
   ```powershell
   C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe /nologo /out:DevAssist\DiscoverDebugPort.exe DevAssist\Utils\DiscoverDebugPort.cs
   ```

   **Compile TCPSerialBridge:**
   ```powershell
   C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe /nologo /out:DevAssist\TCPSerialBridge.exe DevAssist\Utils\TCPSerialBridge.cs
   ```

That's it! The updated, zero-dependency EXEs are now ready to be used by the team.
