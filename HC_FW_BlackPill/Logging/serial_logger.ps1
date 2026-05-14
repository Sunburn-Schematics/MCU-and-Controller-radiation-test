<#
.SYNOPSIS
Displays one serial port stream, writes the same data to a log file, and
optionally broadcasts it to local TCP and WebSocket listeners.
#>

param(
    [Parameter(Mandatory = $true)]
    [string]$PortName,

    [Parameter(Mandatory = $true)]
    [string]$LogPath,

    [int]$BaudRate = 115200,

    [int]$TcpPort = 0,

    [int]$WebSocketPort = 0,

    [string]$WebSocketPath = ""
)

Set-StrictMode -Version Latest
$ErrorActionPreference = "Stop"

$logDirectory = Split-Path -Parent $LogPath
if ($logDirectory -and -not (Test-Path -LiteralPath $logDirectory)) {
    New-Item -ItemType Directory -Path $logDirectory | Out-Null
}

function Get-WebSocketListenerPath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$PortName,

        [string]$RequestedPath
    )

    if ($RequestedPath.Length -gt 0) {
        $path = $RequestedPath
    }
    else {
        $path = "/$PortName"
    }

    if (-not $path.StartsWith("/")) {
        $path = "/$path"
    }

    if (-not $path.EndsWith("/")) {
        $path = "$path/"
    }

    return $path
}

function Start-LocalTcpMirrorListener {
    param(
        [Parameter(Mandatory = $true)]
        [int]$Port
    )

    if ($Port -le 0) {
        return $null
    }

    $listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Parse("127.0.0.1"), $Port)
    $listener.Start()

    [pscustomobject]@{
        Listener = $listener
        Host = "127.0.0.1"
        Port = $Port
        Clients = [System.Collections.Generic.List[System.Net.Sockets.TcpClient]]::new()
    }
}

function Update-TcpMirrorListener {
    param(
        [pscustomobject]$Server
    )

    if ($null -eq $Server) {
        return
    }

    while ($Server.Listener.Pending()) {
        $client = $null
        try {
            $client = $Server.Listener.AcceptTcpClient()
            $client.NoDelay = $true
            $Server.Clients.Add($client)
        }
        catch {
            Write-Warning "TCP mirror accept failed: $($_.Exception.Message)"
            if ($null -ne $client) {
                $client.Close()
            }
        }
    }
}

function Send-TcpMirrorData {
    param(
        [pscustomobject]$Server,

        [Parameter(Mandatory = $true)]
        [byte[]]$Data
    )

    if (($null -eq $Server) -or ($Data.Length -eq 0)) {
        return
    }

    for ($index = $Server.Clients.Count - 1; $index -ge 0; $index--) {
        $client = $Server.Clients[$index]

        if (-not $client.Connected) {
            $Server.Clients.RemoveAt($index)
            $client.Close()
            continue
        }

        try {
            $stream = $client.GetStream()
            $stream.Write($Data, 0, $Data.Length)
        }
        catch {
            $Server.Clients.RemoveAt($index)
            $client.Close()
        }
    }
}

function Stop-TcpMirrorListener {
    param(
        [pscustomobject]$Server
    )

    if ($null -eq $Server) {
        return
    }

    foreach ($client in @($Server.Clients)) {
        try {
            $client.Close()
        }
        catch {
            # Ignore shutdown errors.
        }
    }

    $Server.Listener.Stop()
}

function Start-LocalWebSocketListener {
    param(
        [Parameter(Mandatory = $true)]
        [int]$Port,

        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    if ($Port -le 0) {
        return $null
    }

    $listener = [System.Net.Sockets.TcpListener]::new([System.Net.IPAddress]::Parse("127.0.0.1"), $Port)
    $listener.Start()

    [pscustomobject]@{
        Listener = $listener
        Url = "ws://127.0.0.1:$Port$Path"
        Path = $Path
        Clients = [System.Collections.Generic.List[System.Net.Sockets.TcpClient]]::new()
    }
}

function Get-WebSocketHandshakeHeader {
    param(
        [Parameter(Mandatory = $true)]
        [System.Net.Sockets.NetworkStream]$Stream
    )

    $buffer = [byte[]]::new(1024)
    $requestBytes = [System.Collections.Generic.List[byte]]::new()
    $deadline = [DateTime]::UtcNow.AddSeconds(2)

    while ([DateTime]::UtcNow -lt $deadline) {
        if ($Stream.DataAvailable) {
            $read = $Stream.Read($buffer, 0, $buffer.Length)
            if ($read -le 0) {
                break
            }

            for ($index = 0; $index -lt $read; $index++) {
                $requestBytes.Add($buffer[$index])
            }

            $requestText = [System.Text.Encoding]::ASCII.GetString($requestBytes.ToArray())
            if ($requestText.Contains("`r`n`r`n")) {
                return $requestText
            }
        }

        Start-Sleep -Milliseconds 10
    }

    return ""
}

function Get-WebSocketHeaderValue {
    param(
        [Parameter(Mandatory = $true)]
        [string]$HeaderText,

        [Parameter(Mandatory = $true)]
        [string]$Name
    )

    foreach ($line in ($HeaderText -split "`r`n")) {
        $separator = $line.IndexOf(":")
        if ($separator -le 0) {
            continue
        }

        $headerName = $line.Substring(0, $separator).Trim()
        if ($headerName.Equals($Name, [System.StringComparison]::OrdinalIgnoreCase)) {
            return $line.Substring($separator + 1).Trim()
        }
    }

    return ""
}

function Confirm-WebSocketHandshake {
    param(
        [Parameter(Mandatory = $true)]
        [System.Net.Sockets.TcpClient]$Client,

        [Parameter(Mandatory = $true)]
        [string]$ExpectedPath
    )

    $stream = $Client.GetStream()
    $request = Get-WebSocketHandshakeHeader -Stream $stream
    if ($request.Length -eq 0) {
        return $false
    }

    $firstLine = ($request -split "`r`n" | Select-Object -First 1)
    $parts = @($firstLine -split " ")
    $expectedPathWithoutTrailingSlash = $ExpectedPath.TrimEnd("/")
    if (($parts.Count -lt 2) -or
        ($parts[0] -ne "GET") -or
        (($parts[1] -ne $ExpectedPath) -and ($parts[1] -ne $expectedPathWithoutTrailingSlash))) {
        return $false
    }

    $key = Get-WebSocketHeaderValue -HeaderText $request -Name "Sec-WebSocket-Key"
    if ($key.Length -eq 0) {
        return $false
    }

    $sha1 = [System.Security.Cryptography.SHA1]::Create()
    try {
        $acceptSeed = $key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11"
        $acceptBytes = $sha1.ComputeHash([System.Text.Encoding]::ASCII.GetBytes($acceptSeed))
        $accept = [Convert]::ToBase64String($acceptBytes)
    }
    finally {
        $sha1.Dispose()
    }

    $response = "HTTP/1.1 101 Switching Protocols`r`n" +
                "Upgrade: websocket`r`n" +
                "Connection: Upgrade`r`n" +
                "Sec-WebSocket-Accept: $accept`r`n" +
                "`r`n"
    $responseBytes = [System.Text.Encoding]::ASCII.GetBytes($response)
    $stream.Write($responseBytes, 0, $responseBytes.Length)
    return $true
}

function Update-WebSocketListener {
    param(
        [pscustomobject]$Server
    )

    if ($null -eq $Server) {
        return
    }

    while ($Server.Listener.Pending()) {
        $client = $null
        try {
            $client = $Server.Listener.AcceptTcpClient()
            $client.NoDelay = $true
            if (Confirm-WebSocketHandshake -Client $client -ExpectedPath $Server.Path) {
                $Server.Clients.Add($client)
            }
            else {
                $client.Close()
            }
        }
        catch {
            Write-Warning "WebSocket accept failed: $($_.Exception.Message)"
            if ($null -ne $client) {
                $client.Close()
            }
        }
    }
}

function New-WebSocketBinaryFrame {
    param(
        [Parameter(Mandatory = $true)]
        [byte[]]$Data
    )

    $length = [uint64]$Data.Length
    $frame = [System.Collections.Generic.List[byte]]::new()
    $frame.Add([byte]0x82)

    if ($length -lt 126) {
        $frame.Add([byte]$length)
    }
    elseif ($length -le 65535) {
        $frame.Add(126)
        $frame.Add([byte](($length -shr 8) -band 0xFF))
        $frame.Add([byte]($length -band 0xFF))
    }
    else {
        $frame.Add(127)
        for ($shift = 56; $shift -ge 0; $shift -= 8) {
            $frame.Add([byte](($length -shr $shift) -band 0xFF))
        }
    }

    foreach ($byte in $Data) {
        $frame.Add($byte)
    }

    return ,$frame.ToArray()
}

function Send-WebSocketData {
    param(
        [pscustomobject]$Server,

        [Parameter(Mandatory = $true)]
        [byte[]]$Data
    )

    if (($null -eq $Server) -or ($Data.Length -eq 0)) {
        return
    }

    [byte[]]$frame = New-WebSocketBinaryFrame -Data $Data

    for ($index = $Server.Clients.Count - 1; $index -ge 0; $index--) {
        $client = $Server.Clients[$index]

        if (-not $client.Connected) {
            $Server.Clients.RemoveAt($index)
            $client.Close()
            continue
        }

        try {
            $stream = $client.GetStream()
            $stream.Write($frame, 0, $frame.Length)
        }
        catch {
            $Server.Clients.RemoveAt($index)
            $client.Close()
        }
    }
}

function Stop-LocalWebSocketListener {
    param(
        [pscustomobject]$Server
    )

    if ($null -eq $Server) {
        return
    }

    foreach ($client in @($Server.Clients)) {
        try {
            $client.Close()
        }
        catch {
            # Ignore shutdown errors.
        }
    }

    $Server.Listener.Stop()
}

$port = [System.IO.Ports.SerialPort]::new($PortName, $BaudRate, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
$port.Encoding = [System.Text.Encoding]::ASCII
$port.ReadTimeout = 250
$port.WriteTimeout = 1000
$port.DtrEnable = $true
$port.RtsEnable = $true
$tcpMirrorServer = $null
$webSocketServer = $null

try {
    if ($TcpPort -gt 0) {
        $tcpMirrorServer = Start-LocalTcpMirrorListener -Port $TcpPort
    }

    if ($WebSocketPort -gt 0) {
        $listenerPath = Get-WebSocketListenerPath -PortName $PortName -RequestedPath $WebSocketPath
        $webSocketServer = Start-LocalWebSocketListener -Port $WebSocketPort -Path $listenerPath
    }

    $port.Open()
    $port.DiscardInBuffer()
    $port.DiscardOutBuffer()

    Write-Host "Logging $PortName at $BaudRate baud to $LogPath"
    if ($null -ne $tcpMirrorServer) {
        Write-Host "Broadcasting serial data to TCP $($tcpMirrorServer.Host):$($tcpMirrorServer.Port)"
    }
    if ($null -ne $webSocketServer) {
        Write-Host "Broadcasting serial data to $($webSocketServer.Url)"
    }
    Write-Host "Press Ctrl+C to stop."

    try {
        while ($true) {
            Update-TcpMirrorListener -Server $tcpMirrorServer
            Update-WebSocketListener -Server $webSocketServer

            $data = $port.ReadExisting()
            if ($data.Length -gt 0) {
                $dataBytes = [System.Text.Encoding]::ASCII.GetBytes($data)
                Write-Host -NoNewline $data
                Add-Content -LiteralPath $LogPath -Value $data -NoNewline -Encoding ASCII
                Send-TcpMirrorData -Server $tcpMirrorServer -Data $dataBytes
                Send-WebSocketData -Server $webSocketServer -Data $dataBytes
            }

            Start-Sleep -Milliseconds 50
        }
    }
    catch {
        Write-Warning "Serial logger stopped for $PortName`: $($_.Exception.Message)"
    }
}
finally {
    Stop-TcpMirrorListener -Server $tcpMirrorServer
    Stop-LocalWebSocketListener -Server $webSocketServer

    if ($port.IsOpen) {
        $port.Close()
    }
    $port.Dispose()
}
