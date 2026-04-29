# STM32 Dev Container Setup

This project uses a VS Code Dev Container to guarantee a reproducible build and debug environment (ARM GCC, OpenOCD, GDB) for all team members.

## Prerequisites (Windows)
- Docker Desktop (WSL2 backend)
- Visual Studio Code with the **Dev Containers** extension installed.
- An ST-Link programmer physically plugged into your PC.

## Step-by-Step Guide
1. **Open the Project in VS Code.** A prompt will say "Folder contains a Dev Container configuration". Click **Reopen in Container**. (Or press `Ctrl+Shift+P` -> `Dev Containers: Reopen in Container`).
2. Wait for it to build the image and install extensions.

### 🔌 Connecting the ST-Link Configuration (WSL2 Passthrough)
Because Docker uses WSL2, the container cannot see physical Windows USB ports by default.
1. Open a **brand new** standard Windows PowerShell as **Administrator** (Do not use the integrated VS Code terminal).
2. Navigate to this `.devcontainer` folder: `cd C:\Users\marty\Code\STM32\SBS.030.010-1C_HC_Firmware\.devcontainer`
3. Run: `.\setup_usbipd.ps1`
   - *(If usbipd-win is missing, it will install it. Restart your terminal and run it again).* 
   - *(If found, it will automatically locate the ST-Link's BUSID and attach it to WSL for you).* 

### 🚀 Building and Debugging
Once the ST-Link is attached, go back to your VS Code (running in the container):
- **Build:** Press `Ctrl+Shift+B` or rely on the `make -j4` task invoked during debug.
- **Debug:** Press **`F5`**. Cortex-Debug will automatically flash the MCU and attach breakpoints via OpenOCD.
