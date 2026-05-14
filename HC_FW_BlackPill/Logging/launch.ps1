<#
.SYNOPSIS
Polls connected SBS RadTest CDC USB serial devices and sets their RTC date/time.

.DESCRIPTION
Finds Windows COM ports whose USB product/friendly name contains "SBS RadTest CDC"
by default, then sends the HC JSON commands:
{"type":"SET","msg":<n>,"args":{"date_time":"YYYYMMDD HH:MM:SS"}}
{"type":"GET","msg":<n>,"args":{"sw_version":true}}

After a successful time set, starts one Windows Terminal instance per COM port.
Each terminal runs serial_logger.ps1, displays the stream, and logs the same
data to YYYYMMDD_HHMM_<hc_id>.log in the script directory. Duplicate HC IDs
are logged as <hc_id>A, <hc_id>B, etc. to keep filenames unique.
Each logger also exposes localhost raw TCP and WebSocket endpoints for live
serial data; launch.ps1 allocates free TCP ports starting at TcpBasePort and
WebSocketBasePort.

Runs one discovery/set/logging pass, then exits while the logger terminals continue.
If a matching COM port is already open, it is skipped so rerunning this script
only starts loggers for ports that are present and not currently being logged.
#>

param(
    [string]$ProductString = "SBS RadTest CDC",
    [int]$BaudRate = 115200,
    [int]$OpenFlushDelayMs = 2000,
    [int]$ReadTimeoutMs = 3000,
    [string]$TerminalPath = "wt.exe",
    [string]$LogDirectory = $PSScriptRoot,
    [int]$TcpBasePort = 9765,
    [int]$WebSocketBasePort = 8765,
    [switch]$SetTimeOnly
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

function Get-RadTestSerialDevice {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ProductString
    )

    Get-CimInstance -ClassName Win32_SerialPort |
        Where-Object PNPDeviceID -like "USB*" |
        ForEach-Object {
            $usbProductString = try {
                (Get-PnpDeviceProperty -InstanceId $_.PNPDeviceID -KeyName "DEVPKEY_Device_BusReportedDeviceDesc").Data
            }
            catch {
                $null
            }

            if (-not $usbProductString) {
                $usbProductString = $_.Description
            }

            [pscustomobject]@{
                PortName = $_.DeviceID
                ProductString = $usbProductString
                PnpDeviceId = $_.PNPDeviceID
            }
        } |
        Where-Object {
            $_.PortName -match "^COM[0-9]+$" -and
            $_.ProductString -like "*$ProductString*"
        } |
        Sort-Object -Property PortName
}

function New-DateTimeSetCommand {
    param(
        [Parameter(Mandatory = $true)]
        [int]$MessageId
    )

    $dateTime = Get-Date -Format "yyyyMMdd HH:mm:ss"
    $command = [ordered]@{
        type = "SET"
        msg = $MessageId
        args = [ordered]@{
            date_time = $dateTime
        }
    }

    [pscustomobject]@{
        DateTime = $dateTime
        Json = ($command | ConvertTo-Json -Compress -Depth 4)
    }
}

function New-SwVersionGetCommand {
    param(
        [Parameter(Mandatory = $true)]
        [int]$MessageId
    )

    $command = [ordered]@{
        type = "GET"
        msg = $MessageId
        args = [ordered]@{
            sw_version = $true
        }
    }

    [pscustomobject]@{
        Json = ($command | ConvertTo-Json -Compress -Depth 4)
    }
}

function Get-CompleteJsonObject {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    $objects = @()
    $remainder = ""
    $objectStart = -1
    $braceDepth = 0
    $inString = $false
    $escapeNext = $false
    $consumedThrough = -1

    for ($index = 0; $index -lt $Text.Length; $index++) {
        $char = $Text[$index]

        if ($braceDepth -eq 0) {
            if ($char -eq "{") {
                $objectStart = $index
                $braceDepth = 1
                $inString = $false
                $escapeNext = $false
            }

            continue
        }

        if ($escapeNext) {
            $escapeNext = $false
            continue
        }

        if ($inString) {
            if ($char -eq "\") {
                $escapeNext = $true
            }
            elseif ($char -eq '"') {
                $inString = $false
            }
            continue
        }

        if ($char -eq '"') {
            $inString = $true
            continue
        }

        if ($char -eq "{") {
            $braceDepth++
            continue
        }

        if ($char -eq "}" -and $braceDepth -gt 0) {
            $braceDepth--
            if ($braceDepth -eq 0 -and $objectStart -ge 0) {
                $objects += $Text.Substring($objectStart, $index - $objectStart + 1)
                $consumedThrough = $index
                $objectStart = -1
            }
        }
    }

    if ($braceDepth -gt 0 -and $objectStart -ge 0) {
        $remainder = $Text.Substring($objectStart)
    }
    elseif ($consumedThrough -lt ($Text.Length - 1)) {
        $remainder = $Text.Substring($consumedThrough + 1)
    }

    [pscustomobject]@{
        Objects = $objects
        Remainder = $remainder
    }
}

function Read-MatchingResponse {
    param(
        [Parameter(Mandatory = $true)]
        [System.IO.Ports.SerialPort]$Port,

        [Parameter(Mandatory = $true)]
        [int]$MessageId,

        [Parameter(Mandatory = $true)]
        [int]$TimeoutMs
    )

    $deadline = [DateTime]::UtcNow.AddMilliseconds($TimeoutMs)
    $buffer = ""

    while ([DateTime]::UtcNow -lt $deadline) {
        $chunk = $Port.ReadExisting()
        if ($chunk.Length -gt 0) {
            $buffer += $chunk
            $extracted = Get-CompleteJsonObject -Text $buffer

            foreach ($jsonText in $extracted.Objects) {
                try {
                    $json = $jsonText | ConvertFrom-Json
                    if ($json.type -eq "RSP" -and $json.msg -eq $MessageId) {
                        return $jsonText
                    }
                }
                catch {
                    # Ignore malformed fragments; the serial stream may start mid-record.
                }
            }

            $buffer = $extracted.Remainder
        }

        Start-Sleep -Milliseconds 50
    }

    return $buffer.Trim()
}

function Initialize-RadTestDevice {
    param(
        [Parameter(Mandatory = $true)]
        [pscustomobject]$Device,

        [Parameter(Mandatory = $true)]
        [int]$MessageId,

        [Parameter(Mandatory = $true)]
        [int]$BaudRate,

        [Parameter(Mandatory = $true)]
        [int]$OpenFlushDelayMs,

        [Parameter(Mandatory = $true)]
        [int]$ReadTimeoutMs
    )

    $setRequest = New-DateTimeSetCommand -MessageId $MessageId
    $versionMessageId = $MessageId + 1
    $versionRequest = New-SwVersionGetCommand -MessageId $versionMessageId
    $port = [System.IO.Ports.SerialPort]::new($Device.PortName, $BaudRate, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
    $port.Encoding = [System.Text.Encoding]::ASCII
    $port.ReadTimeout = $ReadTimeoutMs
    $port.WriteTimeout = $ReadTimeoutMs
    $port.DtrEnable = $true
    $port.RtsEnable = $true
    $port.NewLine = "`n"

    try {
        $port.Open()
        $port.DiscardInBuffer()
        $port.DiscardOutBuffer()
        Start-Sleep -Milliseconds $OpenFlushDelayMs
        $null = $port.ReadExisting()
        $port.DiscardInBuffer()
        $port.Write($setRequest.Json)
        $setResponse = Read-MatchingResponse -Port $port -MessageId $MessageId -TimeoutMs $ReadTimeoutMs
        $hcId = Get-HcIdFromResponse -Response $setResponse
        $port.Write($versionRequest.Json)
        $versionResponse = Read-MatchingResponse -Port $port -MessageId $versionMessageId -TimeoutMs $ReadTimeoutMs
        $fwVersion = Get-SwVersionFromResponse -Response $versionResponse

        [pscustomobject]@{
            PortName = $Device.PortName
            DateTime = $setRequest.DateTime
            SetRequest = $setRequest.Json
            SetResponse = $setResponse.Trim()
            VersionRequest = $versionRequest.Json
            VersionResponse = $versionResponse.Trim()
            HcId = $hcId
            LogId = $hcId
            FwVersion = $fwVersion
            Success = $true
            Error = $null
        }
    }
    catch {
        [pscustomobject]@{
            PortName = $Device.PortName
            DateTime = $setRequest.DateTime
            SetRequest = $setRequest.Json
            SetResponse = ""
            VersionRequest = $versionRequest.Json
            VersionResponse = ""
            HcId = $null
            LogId = $null
            FwVersion = $null
            Success = $false
            Error = $_.Exception.Message
        }
    }
    finally {
        if ($port.IsOpen) {
            $port.Close()
        }
        $port.Dispose()
    }
}

function Get-HcIdFromResponse {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Response
    )

    if ($Response.Length -eq 0) {
        return $null
    }

    $extracted = Get-CompleteJsonObject -Text $Response

    foreach ($jsonText in $extracted.Objects) {
        try {
            $json = $jsonText | ConvertFrom-Json
            if ($json.type -eq "RSP" -and $null -ne $json.hc) {
                return [string]$json.hc
            }
        }
        catch {
            continue
        }
    }

    try {
        $json = $Response | ConvertFrom-Json
        if ($json.type -eq "RSP" -and $null -ne $json.hc) {
            return [string]$json.hc
        }
    }
    catch {
        return $null
    }

    return $null
}

function Get-SwVersionFromResponse {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Response
    )

    if ($Response.Length -eq 0) {
        return $null
    }

    $extracted = Get-CompleteJsonObject -Text $Response

    foreach ($jsonText in $extracted.Objects) {
        try {
            $json = $jsonText | ConvertFrom-Json
            if ($json.type -eq "RSP" -and $null -ne $json.args -and $null -ne $json.args.sw_version) {
                return [string]$json.args.sw_version
            }
        }
        catch {
            continue
        }
    }

    try {
        $json = $Response | ConvertFrom-Json
        if ($json.type -eq "RSP" -and $null -ne $json.args -and $null -ne $json.args.sw_version) {
            return [string]$json.args.sw_version
        }
    }
    catch {
        return $null
    }

    return $null
}

function Join-CommandLineArgument {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Argument
    )

    ($Argument | ForEach-Object {
        if ($_ -match '[\s"]') {
            '"' + ($_ -replace '"', '\"') + '"'
        }
        else {
            $_
        }
    }) -join " "
}

function Test-TcpPortAvailable {
    param(
        [Parameter(Mandatory = $true)]
        [int]$Port
    )

    $listener = $null

    try {
        $listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Parse("127.0.0.1"), $Port)
        $listener.Start()
        return $true
    }
    catch {
        return $false
    }
    finally {
        if ($null -ne $listener) {
            $listener.Stop()
        }
    }
}

function Get-AvailableTcpPort {
    param(
        [Parameter(Mandatory = $true)]
        [int]$StartPort
    )

    if ($StartPort -le 0) {
        return 0
    }

    for ($port = $StartPort; $port -le 65535; $port++) {
        if (Test-TcpPortAvailable -Port $port) {
            return $port
        }
    }

    throw "No available TCP port found at or above $StartPort."
}

function Start-SerialLoggerTerminal {
    param(
        [Parameter(Mandatory = $true)]
        [pscustomobject]$Device,

        [Parameter(Mandatory = $true)]
        [string]$HcId,

        [Parameter(Mandatory = $true)]
        [int]$BaudRate,

        [Parameter(Mandatory = $true)]
        [string]$TerminalPath,

        [Parameter(Mandatory = $true)]
        [string]$LogDirectory,

        [int]$TcpPort = 0,

        [int]$WebSocketPort = 0
    )

    if (-not (Test-Path -LiteralPath $LogDirectory)) {
        New-Item -ItemType Directory -Path $LogDirectory | Out-Null
    }

    $timestamp = Get-Date -Format "yyyyMMdd_HHmm"
    $safeHcId = $HcId -replace "[^A-Za-z0-9_-]", "_"
    $logPath = Join-Path -Path $LogDirectory -ChildPath "$($timestamp)_$($safeHcId).log"
    $logFileName = Split-Path -Leaf $logPath
    $loggerPath = Join-Path -Path $PSScriptRoot -ChildPath "serial_logger.ps1"
    $webSocketPath = "/$safeHcId"
    $arguments = @(
        "new-tab",
        "--title", $logFileName,
        "--",
        "powershell",
        "-NoProfile",
        "-ExecutionPolicy", "Bypass",
        "-File", $loggerPath,
        "-PortName", $Device.PortName,
        "-BaudRate", "$BaudRate",
        "-LogPath", $logPath
    )

    if ($TcpPort -gt 0) {
        $arguments += @(
            "-TcpPort", "$TcpPort"
        )
    }

    if ($WebSocketPort -gt 0) {
        $arguments += @(
            "-WebSocketPort", "$WebSocketPort",
            "-WebSocketPath", $webSocketPath
        )
    }

    $argumentLine = Join-CommandLineArgument -Argument $arguments

    $process = Start-Process -FilePath $TerminalPath -ArgumentList $argumentLine -PassThru
    $tcpHost = if ($TcpPort -gt 0) {
        "127.0.0.1"
    }
    else {
        $null
    }
    $webSocketUrl = if ($WebSocketPort -gt 0) {
        "ws://127.0.0.1:$WebSocketPort$webSocketPath/"
    }
    else {
        $null
    }

    [pscustomobject]@{
        PortName = $Device.PortName
        HcId = $HcId
        LogPath = $logPath
        TcpHost = $tcpHost
        TcpPort = $TcpPort
        WebSocketUrl = $webSocketUrl
        ProcessId = $process.Id
    }
}

$messageId = 0
$results = @()
$devices = @(Get-RadTestSerialDevice -ProductString $ProductString)

if ($devices.Count -eq 0) {
    Write-Host "No '$ProductString' USB serial devices found."
}

foreach ($device in $devices) {
    $result = Initialize-RadTestDevice `
        -Device $device `
        -MessageId $messageId `
        -BaudRate $BaudRate `
        -OpenFlushDelayMs $OpenFlushDelayMs `
        -ReadTimeoutMs $ReadTimeoutMs

    $messageId = ($messageId + 2) % 1000000

    if ($result.Success) {
        $results += $result
        Write-Host "[$($result.PortName)] SET date_time $($result.DateTime)"
        if ($result.SetResponse.Length -gt 0) {
            Write-Host "[$($result.PortName)] $($result.SetResponse)"
        }
        if ($result.VersionResponse.Length -gt 0) {
            Write-Host "[$($result.PortName)] $($result.VersionResponse)"
        }
        if ($null -ne $result.FwVersion) {
            Write-Host "[$($result.PortName)] FW version $($result.FwVersion)"
        }
        else {
            Write-Warning "[$($result.PortName)] Could not read FW version from GET sw_version RSP."
        }
    }
    else {
        if ($result.Error -like "*Access to the port*$($result.PortName)*is denied*") {
            Write-Host "[$($result.PortName)] Port is already open; assuming logger is active and skipping."
        }
        else {
            Write-Warning "[$($result.PortName)] Failed to SET date_time: $($result.Error)"
        }
    }
}

$successfulResults = @($results | Where-Object { $_.Success })
$versions = @($successfulResults | Where-Object { $null -ne $_.FwVersion } | Select-Object -ExpandProperty FwVersion -Unique)
if ($versions.Count -gt 1) {
    Write-Warning "Connected devices report different FW versions: $($versions -join ', ')"
}

$duplicateIdGroups = @($successfulResults | Where-Object { $null -ne $_.HcId } | Group-Object -Property HcId | Where-Object { $_.Count -gt 1 })
foreach ($group in $duplicateIdGroups) {
    Write-Warning "Duplicate HC ID $($group.Name) detected on ports: $((@($group.Group) | Select-Object -ExpandProperty PortName) -join ', ')"
    $suffixCode = [int][char]'A'
    foreach ($result in @($group.Group | Sort-Object -Property PortName)) {
        $result.LogId = "$($result.HcId)$([char]$suffixCode)"
        $suffixCode++
    }
}

if (-not $SetTimeOnly) {
    $nextTcpPort = $TcpBasePort
    $nextWebSocketPort = $WebSocketBasePort
    foreach ($result in $successfulResults) {
        if ($null -eq $result.HcId) {
            Write-Warning "[$($result.PortName)] Could not read hc from SET date_time RSP; serial logger not started."
            continue
        }

        try {
            $device = $devices | Where-Object { $_.PortName -eq $result.PortName } | Select-Object -First 1
            $tcpPort = if ($TcpBasePort -gt 0) {
                Get-AvailableTcpPort -StartPort $nextTcpPort
            }
            else {
                0
            }
            if ($tcpPort -gt 0) {
                $nextTcpPort = $tcpPort + 1
            }

            if (($tcpPort -gt 0) -and ($nextWebSocketPort -eq $tcpPort)) {
                $nextWebSocketPort = $tcpPort + 1
            }

            $webSocketPort = if ($WebSocketBasePort -gt 0) {
                Get-AvailableTcpPort -StartPort $nextWebSocketPort
            }
            else {
                0
            }
            if ($webSocketPort -gt 0) {
                $nextWebSocketPort = $webSocketPort + 1
            }
            $logger = Start-SerialLoggerTerminal `
                -Device $device `
                -HcId $result.LogId `
                -BaudRate $BaudRate `
                -TerminalPath $TerminalPath `
                -LogDirectory $LogDirectory `
                -TcpPort $tcpPort `
                -WebSocketPort $webSocketPort

            Write-Host "[$($result.PortName)] Started logger terminal PID $($logger.ProcessId), logging to $($logger.LogPath)"
            if ($null -ne $logger.TcpHost) {
                Write-Host "[$($result.PortName)] TCP monitor host $($logger.TcpHost), port $($logger.TcpPort)"
            }
            if ($null -ne $logger.WebSocketUrl) {
                Write-Host "[$($result.PortName)] WebSocket $($logger.WebSocketUrl)"
            }
        }
        catch {
            Write-Warning "[$($result.PortName)] Failed to start serial logger: $($_.Exception.Message)"
        }
    }
}
