<#
.SYNOPSIS
Programs HC_FW_BlackPill through ST-Link/SWD and verifies runtime response over USB CDC.

.DESCRIPTION
Uses OpenOCD with the project target interface (`interface/stlink.cfg` and
`target/stm32f4x.cfg`) to identify the STM32F411 target, program and verify the
ELF image, then query the firmware over the USB virtual COM port with
`GET args.sw_version:true`.
#>

param(
    [string]$ElfPath = "build\Debug\HC_FW_BlackPill.elf",

    [string]$PortName = "",

    [int]$BaudRate = 115200,

    [int]$AdapterSpeedKHz = 1800,

    [int]$TimeoutSeconds = 60,

    [int]$SerialTimeoutSeconds = 8,

    [switch]$SkipSerialVerify,

    [switch]$IdentifyOnly
)

$ErrorActionPreference = "Stop"

function ConvertTo-CmdArgument {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Argument
    )

    if ($Argument -match '[\s&()^=;!+,`~\[\]{}]') {
        return '"' + ($Argument -replace '"', '\"') + '"'
    }

    return $Argument
}

function Invoke-LoggedProcess {
    param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,

        [Parameter(Mandatory = $true)]
        [string]$WorkingDirectory,

        [Parameter(Mandatory = $true)]
        [int]$TimeoutSeconds
    )

    $startInfo = [System.Diagnostics.ProcessStartInfo]::new()
    $startInfo.FileName = $FilePath
    $startInfo.WorkingDirectory = $WorkingDirectory
    $startInfo.UseShellExecute = $false
    $startInfo.CreateNoWindow = $true
    $startInfo.RedirectStandardOutput = $true
    $startInfo.RedirectStandardError = $true

    $startInfo.Arguments = ($Arguments | ForEach-Object { ConvertTo-CmdArgument $_ }) -join " "

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $startInfo

    [void]$process.Start()
    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()

    $completed = $process.WaitForExit($TimeoutSeconds * 1000)
    if (-not $completed) {
        try {
            $process.Kill($true)
        }
        catch {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        }

        throw "$FilePath timed out after ${TimeoutSeconds}s"
    }

    $process.WaitForExit()
    $process.Refresh()

    [pscustomobject]@{
        ExitCode = $process.ExitCode
        Stdout = $stdoutTask.GetAwaiter().GetResult()
        Stderr = $stderrTask.GetAwaiter().GetResult()
        CommandLine = (ConvertTo-CmdArgument $FilePath) + " " + (($Arguments | ForEach-Object { ConvertTo-CmdArgument $_ }) -join " ")
    }
}

function Invoke-OpenOcd {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Commands
    )

    $arguments = @(
        "-f", "interface/stlink.cfg",
        "-f", "target/stm32f4x.cfg"
    )

    foreach ($command in $Commands) {
        $arguments += "-c"
        $arguments += $command
    }

    $result = Invoke-LoggedProcess `
        -FilePath "openocd" `
        -Arguments $arguments `
        -WorkingDirectory $repoRoot `
        -TimeoutSeconds $TimeoutSeconds

    Write-Output $result.Stdout
    Write-Output $result.Stderr

    if ($result.ExitCode -ne 0) {
        throw "OpenOCD failed with exit code $($result.ExitCode)"
    }

    return $result
}

function Wait-SerialPort {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [Parameter(Mandatory = $true)]
        [int]$TimeoutSeconds
    )

    $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
    while ([DateTime]::UtcNow -lt $deadline) {
        $port = Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue |
            Where-Object { $_.DeviceID -eq $Name } |
            Select-Object -First 1

        if ($port) {
            return
        }

        Start-Sleep -Milliseconds 250
    }

    throw "Serial port $Name was not available within ${TimeoutSeconds}s"
}

function Get-UsbCdcPort {
    param(
        [string]$RequestedPortName
    )

    $ports = @(Get-CimInstance Win32_SerialPort -ErrorAction SilentlyContinue |
        Where-Object { $_.DeviceID -match "^COM[0-9]+$" -and $_.PNPDeviceID -like "USB*" } |
        ForEach-Object {
            $productString = try {
                (Get-PnpDeviceProperty -InstanceId $_.PNPDeviceID -KeyName "DEVPKEY_Device_BusReportedDeviceDesc" -ErrorAction Stop).Data
            }
            catch {
                $_.Description
            }

            [pscustomobject]@{
                PortName = $_.DeviceID
                Name = $_.Name
                Description = $_.Description
                ProductString = $productString
                PnpDeviceId = $_.PNPDeviceID
            }
        })

    if ($RequestedPortName) {
        $selected = @($ports | Where-Object { $_.PortName -eq $RequestedPortName })
        if ($selected.Count -ne 1) {
            $available = if ($ports.Count -gt 0) { ($ports | ForEach-Object { "$($_.PortName) [$($_.ProductString)]" }) -join ", " } else { "<none>" }
            throw "Requested serial port $RequestedPortName was not found. Available USB serial ports: $available"
        }
        return $selected[0]
    }

    $preferred = @($ports | Where-Object {
        $_.PnpDeviceId -like "USB\VID_0483&PID_5740*" -or
        $_.ProductString -like "*SBS RadTest CDC*" -or
        $_.ProductString -like "*STM32*"
    })

    if ($preferred.Count -eq 1) {
        return $preferred[0]
    }

    if ($preferred.Count -gt 1) {
        $candidates = ($preferred | ForEach-Object { "$($_.PortName) [$($_.ProductString)]" }) -join ", "
        throw "Multiple candidate STM32 USB CDC ports found: $candidates. Re-run with -PortName <COMx>."
    }

    if ($ports.Count -eq 1) {
        return $ports[0]
    }

    if ($ports.Count -gt 1) {
        $candidates = ($ports | ForEach-Object { "$($_.PortName) [$($_.ProductString)]" }) -join ", "
        throw "Multiple USB serial ports found: $candidates. Re-run with -PortName <COMx>."
    }

    throw "No USB serial ports found for runtime verification."
}

function Invoke-SwVersionQuery {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [Parameter(Mandatory = $true)]
        [int]$Baud,

        [Parameter(Mandatory = $true)]
        [int]$TimeoutSeconds
    )

    Wait-SerialPort -Name $Name -TimeoutSeconds $TimeoutSeconds

    $port = [System.IO.Ports.SerialPort]::new($Name, $Baud, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
    $port.ReadTimeout = 200
    $port.WriteTimeout = 1000
    $port.DtrEnable = $true
    $port.RtsEnable = $true

    try {
        $port.Open()
        Start-Sleep -Milliseconds 1200
        $null = $port.ReadExisting()

        $messageId = 101
        $command = '{"type":"GET","msg":101,"args":{"sw_version":true}}' + "`n"
        $port.Write($command)

        $deadline = [DateTime]::UtcNow.AddSeconds($TimeoutSeconds)
        $buffer = ""

        while ([DateTime]::UtcNow -lt $deadline) {
            Start-Sleep -Milliseconds 100
            $chunk = $port.ReadExisting()
            if ($chunk.Length -gt 0) {
                $buffer += $chunk
                if (($buffer -match '"type"\s*:\s*"RSP"') -and
                    ($buffer -match ('"msg"\s*:\s*' + $messageId)) -and
                    ($buffer -match '"sw_version"\s*:\s*"([^"]+)"')) {
                    break
                }
            }
        }

        if ($buffer.Length -eq 0) {
            throw "No serial response received from $Name"
        }

        if ($buffer -notmatch '"type"\s*:\s*"RSP"') {
            throw "Serial response did not contain an RSP object: $buffer"
        }

        if ($buffer -notmatch ('"msg"\s*:\s*' + $messageId)) {
            throw "Serial response did not match message id ${messageId}: $buffer"
        }

        if ($buffer -notmatch '"sw_version"\s*:\s*"([^"]+)"') {
            throw "Serial response did not include args.sw_version: $buffer"
        }

        [pscustomobject]@{
            PortName = $Name
            Command = $command.Trim()
            Response = $buffer.Trim()
            SwVersion = $Matches[1]
        }
    }
    finally {
        if ($port.IsOpen) {
            $port.Close()
        }
        $port.Dispose()
    }
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$elfFullPath = Resolve-Path (Join-Path $repoRoot $ElfPath)
$serialDevice = $null
if (-not $SkipSerialVerify) {
    $serialDevice = Get-UsbCdcPort -RequestedPortName $PortName
    $PortName = $serialDevice.PortName
}

Write-Output "elf: $elfFullPath"
if (-not $SkipSerialVerify) {
    Write-Output "serial port: $($serialDevice.PortName) [$($serialDevice.ProductString)]"
}
else {
    Write-Output "serial verify: skipped"
}
Write-Output "adapter speed: ${AdapterSpeedKHz} kHz"

$identifyResult = Invoke-OpenOcd -Commands @(
    "transport select swd",
    "adapter speed $AdapterSpeedKHz",
    "init",
    "targets",
    "reset halt",
    "mdw 0xE0042000 1",
    "shutdown"
)

$identifyText = $identifyResult.Stdout + "`n" + $identifyResult.Stderr
if ($identifyText -match "0xe0042000:\s+([0-9a-fA-F]+)") {
    Write-Output "DBGMCU_IDCODE: 0x$($Matches[1])"
}

if ($IdentifyOnly) {
    Write-Output "identify-only complete"
    exit 0
}

$elfForOpenOcd = $elfFullPath.Path.Replace("\", "/")
Invoke-OpenOcd -Commands @(
    "transport select swd",
    "adapter speed $AdapterSpeedKHz",
    "program `"$elfForOpenOcd`" verify reset exit"
) | Out-Null

Write-Output "program/verify: OK"

if (-not $SkipSerialVerify) {
    $serialResult = Invoke-SwVersionQuery -Name $PortName -Baud $BaudRate -TimeoutSeconds $SerialTimeoutSeconds
    Write-Output "serial command: $($serialResult.Command)"
    Write-Output "serial response: $($serialResult.Response)"
    Write-Output "runtime sw_version: $($serialResult.SwVersion)"
}

Write-Output "target programming workflow complete"
