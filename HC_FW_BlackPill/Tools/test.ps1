<#
.SYNOPSIS
Runs hardware-in-the-loop JSON protocol tests against the programmed HC target.

.DESCRIPTION
The runner can optionally invoke the established guarded build and ST-Link
programming workflows before opening the USB CDC/VCP port. Test cases are loaded
from a JSON file so protocol coverage can grow without rewriting the runner.

The runner records all observed JSON packets, expected and actual results, and a
Markdown summary. It does not attempt to diagnose or fix firmware behavior.
#>

param(
    [string]$Preset = "Debug",

    [string]$CasePath = "Test\hw_test_cases.json",

    [string]$PortName = "",

    [int]$BaudRate = 115200,

    [int]$SerialTimeoutSeconds = 8,

    [int]$InterTestDelayMs = 200,

    [switch]$Build,

    [switch]$Program,

    [switch]$ContinueOnFail,

    [string]$ReportRoot = "Test\Reports",

    [int]$KeepRunLocalResultCount = 5,

    [switch]$SkipReportArchive
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

function Invoke-StepScript {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ScriptPath,

        [Parameter(Mandatory = $true)]
        [string[]]$Arguments,

        [Parameter(Mandatory = $true)]
        [int]$TimeoutSeconds
    )

    $psi = [System.Diagnostics.ProcessStartInfo]::new()
    $psi.FileName = "powershell"
    $psi.WorkingDirectory = $repoRoot
    $psi.UseShellExecute = $false
    $psi.CreateNoWindow = $true
    $psi.RedirectStandardOutput = $true
    $psi.RedirectStandardError = $true

    $allArguments = @("-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $ScriptPath) + $Arguments
    $psi.Arguments = ($allArguments | ForEach-Object { ConvertTo-CmdArgument $_ }) -join " "

    $process = [System.Diagnostics.Process]::new()
    $process.StartInfo = $psi
    [void]$process.Start()

    $stdoutTask = $process.StandardOutput.ReadToEndAsync()
    $stderrTask = $process.StandardError.ReadToEndAsync()

    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        try {
            $process.Kill($true)
        }
        catch {
            Stop-Process -Id $process.Id -Force -ErrorAction SilentlyContinue
        }
        throw "$ScriptPath timed out after ${TimeoutSeconds}s"
    }

    $stdout = $stdoutTask.GetAwaiter().GetResult()
    $stderr = $stderrTask.GetAwaiter().GetResult()
    Write-Output $stdout
    Write-Output $stderr

    if ($process.ExitCode -ne 0) {
        throw "$ScriptPath failed with exit code $($process.ExitCode)"
    }
}

function Open-TestSerialPort {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Name,

        [Parameter(Mandatory = $true)]
        [int]$Speed
    )

    Wait-SerialPort -Name $Name -TimeoutSeconds $SerialTimeoutSeconds

    $serialPort = [System.IO.Ports.SerialPort]::new($Name, $Speed, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
    $serialPort.ReadTimeout = 200
    $serialPort.WriteTimeout = 1000
    $serialPort.DtrEnable = $true
    $serialPort.RtsEnable = $true
    $serialPort.Open()
    Start-Sleep -Milliseconds 1200
    $null = $serialPort.ReadExisting()
    return $serialPort
}

function Close-TestSerialPort {
    param(
        $Port
    )

    if ($null -ne $Port) {
        if ($Port.IsOpen) {
            $Port.Close()
        }
        $Port.Dispose()
    }
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

    throw "No USB serial ports found for hardware test."
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

function ConvertTo-CompactJson {
    param(
        [Parameter(Mandatory = $true)]
        $Value
    )

    return ($Value | ConvertTo-Json -Depth 20 -Compress)
}

function Test-IsNumber {
    param(
        $Value
    )

    $parsed = 0.0
    return [double]::TryParse([string]$Value, [ref]$parsed)
}

function Add-JsonObjectsFromBuffer {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Buffer,

        [System.Collections.ArrayList]$Packets,

        [System.Collections.ArrayList]$MalformedLines
    )

    $lines = $Buffer -split "(`r`n|`n|`r)"
    foreach ($line in $lines) {
        $trimmed = $line.Trim()
        if ($trimmed.Length -eq 0) {
            continue
        }

        if (-not $trimmed.StartsWith("{")) {
            $null = $MalformedLines.Add($trimmed)
            continue
        }

        try {
            $packet = $trimmed | ConvertFrom-Json
            $null = $Packets.Add([pscustomobject]@{
                Raw = $trimmed
                Packet = $packet
            })
        }
        catch {
            $null = $MalformedLines.Add($trimmed)
        }
    }
}

function Get-JsonObjectStrings {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Text
    )

    $objects = [System.Collections.Generic.List[string]]::new()
    $start = -1
    $depth = 0
    $inString = $false
    $escapeActive = $false

    for ($i = 0; $i -lt $Text.Length; $i++) {
        $ch = $Text[$i]

        if ($start -lt 0) {
            if ($ch -eq '{') {
                $start = $i
                $depth = 1
                $inString = $false
                $escapeActive = $false
            }
            continue
        }

        if ($inString) {
            if ($escapeActive) {
                $escapeActive = $false
                continue
            }

            if ($ch -eq '\') {
                $escapeActive = $true
                continue
            }

            if ($ch -eq '"') {
                $inString = $false
            }
            continue
        }

        if ($ch -eq '"') {
            $inString = $true
            continue
        }

        if ($ch -eq '{') {
            $depth++
            continue
        }

        if ($ch -eq '}') {
            if ($depth -gt 0) {
                $depth--
            }

            if ($depth -eq 0) {
                $objects.Add($Text.Substring($start, $i - $start + 1))
                $start = -1
            }
        }
    }

    return @($objects)
}

function ConvertFrom-ObservedJsonObjects {
    param(
        [Parameter(Mandatory = $true)]
        [string]$RawText
    )

    $packets = [System.Collections.ArrayList]::new()
    $malformedLines = [System.Collections.ArrayList]::new()
    foreach ($objectText in (Get-JsonObjectStrings -Text $RawText)) {
        try {
            $packet = $objectText | ConvertFrom-Json
            $null = $packets.Add([pscustomobject]@{
                Raw = $objectText
                Packet = $packet
            })
        }
        catch {
            $null = $malformedLines.Add($objectText)
        }
    }

    [pscustomobject]@{
        Packets = $packets
        MalformedLines = $malformedLines
    }
}

function Get-FieldValue {
    param(
        [Parameter(Mandatory = $true)]
        $Object,

        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $cursor = $Object
    foreach ($part in ($Path -split "\.")) {
        if ($null -eq $cursor) {
            return $null
        }

        $property = $cursor.PSObject.Properties[$part]
        if ($null -eq $property) {
            return $null
        }
        $cursor = $property.Value
    }

    return $cursor
}

function Test-ExpectedValue {
    param(
        [Parameter(Mandatory = $true)]
        [string]$Path,

        $Expected,

        $Actual,

        [System.Collections.ArrayList]$Failures
    )

    $expectedProperties = if ($null -ne $Expected) { $Expected.PSObject.Properties } else { @() }
    $isMatcher = ($expectedProperties.Name -contains "matches")

    if ($isMatcher) {
        $pattern = [string]$Expected.matches
        if (($null -eq $Actual) -or ([string]$Actual -notmatch $pattern)) {
            $null = $Failures.Add("${Path}: expected to match /${pattern}/, actual '$Actual'")
        }
        return
    }

    if ($expectedProperties.Name -contains "any") {
        if (($Expected.any -eq $true) -and ($null -eq $Actual)) {
            $null = $Failures.Add("${Path}: expected any non-null value, actual null")
        }
        return
    }

    if ($expectedProperties.Name -contains "present") {
        if (($Expected.present -eq $true) -and ($null -eq $Actual)) {
            $null = $Failures.Add("${Path}: expected field to be present")
        }
        return
    }

    if ($expectedProperties.Name -contains "number") {
        if ($Expected.number -eq $true) {
            if ($null -eq $Actual) {
                $null = $Failures.Add("${Path}: expected number, actual null")
            }
            elseif (-not (Test-IsNumber -Value $Actual)) {
                $null = $Failures.Add("${Path}: expected number, actual '$Actual'")
            }
        }
        return
    }

    if ($expectedProperties.Name -contains "nullable_number") {
        if (($null -ne $Actual) -and (-not (Test-IsNumber -Value $Actual))) {
            $null = $Failures.Add("${Path}: expected number or null, actual '$Actual'")
        }
        return
    }

    if ($expectedProperties.Name -contains "one_of") {
        $allowed = @($Expected.one_of)
        if ($allowed -notcontains $Actual) {
            $null = $Failures.Add("${Path}: expected one of '$($allowed -join ', ')', actual '$Actual'")
        }
        return
    }

    if ($Expected -is [System.Management.Automation.PSCustomObject]) {
        foreach ($property in $Expected.PSObject.Properties) {
            $childActual = $null
            if ($null -ne $Actual) {
                $actualProperty = $Actual.PSObject.Properties[$property.Name]
                if ($null -ne $actualProperty) {
                    $childActual = $actualProperty.Value
                }
            }
            Test-ExpectedValue -Path "$Path.$($property.Name)" -Expected $property.Value -Actual $childActual -Failures $Failures
        }
        return
    }

    if ($Expected -is [System.Array]) {
        $expectedJson = ConvertTo-CompactJson -Value $Expected
        $actualJson = ConvertTo-CompactJson -Value $Actual
        if ($expectedJson -ne $actualJson) {
            $null = $Failures.Add("${Path}: expected $expectedJson, actual $actualJson")
        }
        return
    }

    if ($Expected -ne $Actual) {
        $null = $Failures.Add("${Path}: expected '$Expected', actual '$Actual'")
    }
}

function Find-ResponsePacket {
    param(
        [System.Collections.ArrayList]$Packets,

        [Parameter(Mandatory = $true)]
        [int]$MessageId
    )

    foreach ($entry in $Packets) {
        $packet = $entry.Packet
        if (($packet.type -eq "RSP") -and ($packet.msg -eq $MessageId)) {
            return $entry
        }
    }

    return $null
}

function Find-ExpectedPacket {
    param(
        [System.Collections.ArrayList]$Packets,

        [Parameter(Mandatory = $true)]
        $Expected,

        [int[]]$UsedIndexes = @()
    )

    $expectedType = Get-FieldValue -Object $Expected -Path "type"
    $expectedMsg = Get-FieldValue -Object $Expected -Path "msg"
    $hasExpectedMsg = ($Expected.PSObject.Properties.Name -contains "msg")

    for ($i = 0; $i -lt $Packets.Count; $i++) {
        if ($UsedIndexes -contains $i) {
            continue
        }

        $packet = $Packets[$i].Packet
        if (($null -ne $expectedType) -and ($packet.type -ne $expectedType)) {
            continue
        }

        if ($hasExpectedMsg -and ($packet.msg -ne $expectedMsg)) {
            continue
        }

        if (($null -eq $expectedType) -and (-not $hasExpectedMsg) -and ($packet.type -ne "RSP")) {
            continue
        }

        $candidateFailures = [System.Collections.ArrayList]::new()
        Test-ExpectedValue -Path "candidate" -Expected $Expected -Actual $packet -Failures $candidateFailures
        if ($candidateFailures.Count -gt 0) {
            continue
        }

        return [pscustomobject]@{
            Index = $i
            Entry = $Packets[$i]
        }
    }

    return $null
}

function Get-CaseInputText {
    param(
        [Parameter(Mandatory = $true)]
        $Case
    )

    if ($Case.PSObject.Properties.Name -contains "request_raw") {
        return [string]$Case.request_raw
    }

    return ConvertTo-CompactJson -Value $Case.request
}

function Get-ExpectedPackets {
    param(
        [Parameter(Mandatory = $true)]
        $Case
    )

    if ($Case.PSObject.Properties.Name -contains "expected_responses") {
        return @($Case.expected_responses)
    }

    if ($Case.PSObject.Properties.Name -contains "expected") {
        return @($Case.expected)
    }

    return @()
}

function Invoke-TestCase {
    param(
        [Parameter(Mandatory = $true)]
        [System.IO.Ports.SerialPort]$Port,

        [Parameter(Mandatory = $true)]
        $Case,

        [Parameter(Mandatory = $true)]
        [string]$TranscriptPath
    )

    $requestText = Get-CaseInputText -Case $Case
    $sendNewline = if ($Case.PSObject.Properties.Name -contains "send_newline") { [bool]$Case.send_newline } else { $true }
    $timeoutSeconds = if ($Case.PSObject.Properties.Name -contains "timeout_seconds") { [int]$Case.timeout_seconds } else { $SerialTimeoutSeconds }
    $expectNoResponse = ($Case.PSObject.Properties.Name -contains "expect_no_response") -and [bool]$Case.expect_no_response
    $port.DiscardInBuffer()
    if ($sendNewline) {
        $port.Write($requestText + "`n")
    }
    else {
        $port.Write($requestText)
    }

    $packets = [System.Collections.ArrayList]::new()
    $malformedLines = [System.Collections.ArrayList]::new()
    $rawBuffer = ""
    $deadline = [DateTime]::UtcNow.AddSeconds($timeoutSeconds)
    # PowerShell unwraps single pipeline results. Force expected packets back
    # into an array so single-response tests execute the same matcher path as
    # multi-response tests and record the matched packet in the report.
    $expectedPackets = @(Get-ExpectedPackets -Case $Case)
    $matchedPackets = @()

    while ([DateTime]::UtcNow -lt $deadline) {
        Start-Sleep -Milliseconds 100
        $chunk = $port.ReadExisting()
        if ($chunk.Length -gt 0) {
            $rawBuffer += $chunk
            $parsed = ConvertFrom-ObservedJsonObjects -RawText $rawBuffer
            $packets = $parsed.Packets
            $malformedLines = $parsed.MalformedLines
        }

        if ($expectNoResponse) {
            continue
        }

        $usedIndexes = @()
        $matches = [System.Collections.Generic.List[object]]::new()
        foreach ($expectedPacket in $expectedPackets) {
            $match = Find-ExpectedPacket -Packets $packets -Expected $expectedPacket -UsedIndexes $usedIndexes
            if ($null -ne $match) {
                $usedIndexes += $match.Index
                $matches.Add($match)
            }
        }

        if ($matches.Count -eq $expectedPackets.Count) {
            $matchedPackets = @($matches)
            break
        }
    }

    $failures = [System.Collections.ArrayList]::new()
    if ($expectNoResponse) {
        if ($packets.Count -gt 0) {
            $null = $failures.Add("Expected no JSON response within ${timeoutSeconds}s, observed $($packets.Count) packet(s)")
        }
    }
    else {
        $usedIndexes = @()
        $matchedList = [System.Collections.Generic.List[object]]::new()
        for ($i = 0; $i -lt $expectedPackets.Count; $i++) {
            $expectedPacket = $expectedPackets[$i]
            $match = Find-ExpectedPacket -Packets $packets -Expected $expectedPacket -UsedIndexes $usedIndexes
            if ($null -eq $match) {
                $expectedText = ConvertTo-CompactJson -Value $expectedPacket
                $null = $failures.Add("No matching packet observed for expected[$i] $expectedText within ${timeoutSeconds}s")
                continue
            }

            $usedIndexes += $match.Index
            $matchedList.Add($match.Entry.Packet)
            Test-ExpectedValue -Path "expected[$i]" -Expected $expectedPacket -Actual $match.Entry.Packet -Failures $failures
        }
        $matchedPackets = @($matchedList)
    }

    if ($Case.PSObject.Properties.Name -contains "cleanup_raw") {
        $port.Write([string]$Case.cleanup_raw)
        Start-Sleep -Milliseconds 250
        $null = $port.ReadExisting()
    }

    $result = [pscustomobject]@{
        id = $Case.id
        title = $Case.title
        kind = if ($Case.kind) { $Case.kind } else { "test" }
        source_section = $Case.source_section
        request = if ($Case.PSObject.Properties.Name -contains "request") { $Case.request } else { $null }
        request_raw = if ($Case.PSObject.Properties.Name -contains "request_raw") { $Case.request_raw } else { $null }
        expected = if ($expectNoResponse) { "no response" } else { $expectedPackets }
        actual = @($matchedPackets | ForEach-Object { if ($_.PSObject.Properties.Name -contains "Packet") { $_.Packet } else { $_ } })
        observed_packets = @($packets | ForEach-Object { $_.Packet })
        malformed_lines = @($malformedLines)
        raw_response = $rawBuffer.Trim()
        pass = ($failures.Count -eq 0)
        failures = @($failures)
    }

    Add-Content -Path $TranscriptPath -Value (ConvertTo-CompactJson -Value $result)
    return $result
}

function Write-TestReport {
    param(
        [Parameter(Mandatory = $true)]
        [array]$Results,

        [Parameter(Mandatory = $true)]
        [string]$ReportPath,

        [Parameter(Mandatory = $true)]
        [string]$TranscriptPath,

        [Parameter(Mandatory = $true)]
        [string]$PortDescription,

        [Parameter(Mandatory = $true)]
        [string]$CaseFile,

        [Parameter(Mandatory = $true)]
        [string]$PresetName,

        [Parameter(Mandatory = $true)]
        [bool]$BuildRequested,

        [Parameter(Mandatory = $true)]
        [bool]$ProgramRequested
    )

    $passCount = @($Results | Where-Object { $_.pass }).Count
    $failCount = @($Results | Where-Object { -not $_.pass }).Count

    $lines = [System.Collections.Generic.List[string]]::new()
    $lines.Add("# Hardware Test Report")
    $lines.Add("")
    $lines.Add("- Time: $((Get-Date).ToString("yyyy-MM-dd HH:mm:ss zzz"))")
    $lines.Add("- Preset: $PresetName")
    $lines.Add("- Build step requested: $BuildRequested")
    $lines.Add("- Program step requested: $ProgramRequested")
    $lines.Add("- Case file: $CaseFile")
    $lines.Add("- Serial target: $PortDescription")
    $lines.Add("- Transcript: $TranscriptPath")
    $lines.Add("- Result: $passCount passed, $failCount failed")
    $lines.Add("")
    $lines.Add("## Results")
    $lines.Add("")

    foreach ($result in $Results) {
        $status = if ($result.pass) { "PASS" } else { "FAIL" }
        $lines.Add("### $status - $($result.id)")
        $lines.Add("")
        $lines.Add("- Title: $($result.title)")
        if ($result.source_section) {
            $lines.Add("- Source: $($result.source_section)")
        }
        if ($null -ne $result.request) {
            $lines.Add("- Request: ``$(ConvertTo-CompactJson -Value $result.request)``")
        }
        else {
            $lines.Add("- Request raw: ``$($result.request_raw)``")
        }
        $lines.Add("- Expected: ``$(ConvertTo-CompactJson -Value $result.expected)``")
        $actualText = if ($null -ne $result.actual) { ConvertTo-CompactJson -Value $result.actual } else { "<none>" }
        $lines.Add("- Actual: ``$actualText``")
        if (-not $result.pass) {
            $lines.Add("- Failure notes: $($result.failures -join "; ")")
        }
        $lines.Add("")
    }

    Set-Content -Path $ReportPath -Value $lines -Encoding UTF8
}

function ConvertTo-RelativePath {
    param(
        [Parameter(Mandatory = $true)]
        [string]$BasePath,

        [Parameter(Mandatory = $true)]
        [string]$TargetPath
    )

    $baseFullPath = [System.IO.Path]::GetFullPath($BasePath)
    $targetFullPath = [System.IO.Path]::GetFullPath($TargetPath)

    if (-not $baseFullPath.EndsWith([System.IO.Path]::DirectorySeparatorChar)) {
        $baseFullPath += [System.IO.Path]::DirectorySeparatorChar
    }

    $baseUri = [System.Uri]::new($baseFullPath)
    $targetUri = [System.Uri]::new($targetFullPath)
    $relativeUri = $baseUri.MakeRelativeUri($targetUri)
    return [System.Uri]::UnescapeDataString($relativeUri.ToString()).Replace("/", "\")
}

function Write-ArchivedReport {
    param(
        [Parameter(Mandatory = $true)]
        [string]$SourceReportPath,

        [Parameter(Mandatory = $true)]
        [string]$SourceTranscriptPath,

        [Parameter(Mandatory = $true)]
        [string]$ArchiveRoot,

        [Parameter(Mandatory = $true)]
        [string]$RunTimestamp,

        [Parameter(Mandatory = $true)]
        [array]$Results,

        [Parameter(Mandatory = $true)]
        [string]$PresetName,

        [Parameter(Mandatory = $true)]
        [string]$PortDescription,

        [Parameter(Mandatory = $true)]
        [bool]$BuildRequested,

        [Parameter(Mandatory = $true)]
        [bool]$ProgramRequested
    )

    New-Item -ItemType Directory -Path $ArchiveRoot -Force | Out-Null

    $archiveReportPath = Join-Path $ArchiveRoot "HardwareTestReport_$RunTimestamp.md"
    $archiveTranscriptPath = Join-Path $ArchiveRoot "HardwareTestResults_$RunTimestamp.jsonl"
    Copy-Item -Path $SourceReportPath -Destination $archiveReportPath -Force
    Copy-Item -Path $SourceTranscriptPath -Destination $archiveTranscriptPath -Force

    Add-Content -Path $archiveReportPath -Value ""
    Add-Content -Path $archiveReportPath -Value "## Archived Artifacts"
    Add-Content -Path $archiveReportPath -Value ""
    Add-Content -Path $archiveReportPath -Value "- Archived transcript: ``$(Split-Path -Leaf $archiveTranscriptPath)``"
    Add-Content -Path $archiveReportPath -Value "- Build-output report: ``$SourceReportPath``"
    Add-Content -Path $archiveReportPath -Value "- Build-output transcript: ``$SourceTranscriptPath``"

    $passCount = @($Results | Where-Object { $_.pass }).Count
    $failCount = @($Results | Where-Object { -not $_.pass }).Count
    $resultText = if ($failCount -eq 0) { "PASS" } else { "FAIL" }
    $indexPath = Join-Path $ArchiveRoot "Index.md"
    $reportLink = ConvertTo-RelativePath -BasePath $ArchiveRoot -TargetPath $archiveReportPath
    $transcriptLink = ConvertTo-RelativePath -BasePath $ArchiveRoot -TargetPath $archiveTranscriptPath

    if (-not (Test-Path $indexPath)) {
        $indexLines = @(
            "# Hardware Test Reports",
            "",
            "Durable review records for hardware-in-the-loop test runs.",
            "",
            "| Time | Result | Preset | Serial Target | Build | Program | Passed | Failed | Report | Transcript |",
            "|---|---|---|---|---|---|---:|---:|---|---|"
        )
        Set-Content -Path $indexPath -Value $indexLines -Encoding UTF8
    }

    $indexRow = "| $((Get-Date).ToString("yyyy-MM-dd HH:mm:ss zzz")) | $resultText | $PresetName | $PortDescription | $BuildRequested | $ProgramRequested | $passCount | $failCount | [$([System.IO.Path]::GetFileName($archiveReportPath))]($reportLink) | [$([System.IO.Path]::GetFileName($archiveTranscriptPath))]($transcriptLink) |"
    Add-Content -Path $indexPath -Value $indexRow

    [pscustomobject]@{
        ReportPath = $archiveReportPath
        TranscriptPath = $archiveTranscriptPath
        IndexPath = $indexPath
    }
}

function Remove-OldRunLocalResults {
    param(
        [Parameter(Mandatory = $true)]
        [string]$ResultRoot,

        [Parameter(Mandatory = $true)]
        [int]$KeepCount,

        [Parameter(Mandatory = $true)]
        [string]$CurrentRunDirectory
    )

    if (-not (Test-Path -LiteralPath $ResultRoot)) {
        return
    }

    $rootFullPath = [System.IO.Path]::GetFullPath($ResultRoot)
    $currentFullPath = [System.IO.Path]::GetFullPath($CurrentRunDirectory)
    $removeDirectories = Get-ChildItem -LiteralPath $ResultRoot -Force -Directory -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -Skip $KeepCount

    foreach ($directory in $removeDirectories) {
        $fullName = [System.IO.Path]::GetFullPath($directory.FullName)
        if ($fullName -eq $currentFullPath) {
            continue
        }
        if (-not $fullName.StartsWith($rootFullPath, [System.StringComparison]::OrdinalIgnoreCase)) {
            throw "Refusing to remove hardware-test result outside result root: $fullName"
        }
        Remove-Item -LiteralPath $fullName -Recurse -Force -ErrorAction SilentlyContinue
    }
}

$repoRoot = Resolve-Path (Join-Path $PSScriptRoot "..")
$caseFullPath = Resolve-Path (Join-Path $repoRoot $CasePath)
$buildDir = Join-Path $repoRoot "build\$Preset"
$timestamp = Get-Date -Format "yyyyMMdd_HHmmss"
$resultDir = Join-Path $buildDir "hw_tests\$timestamp"
New-Item -ItemType Directory -Path $resultDir -Force | Out-Null

$transcriptPath = Join-Path $resultDir "results.jsonl"
$reportPath = Join-Path $resultDir "summary.md"

if ($Build) {
    Invoke-StepScript `
        -ScriptPath (Join-Path $repoRoot "Tools\build.ps1") `
        -Arguments @("-Preset", $Preset, "-Backend", "Commands", "-PerCommandTimeoutSeconds", "30") `
        -TimeoutSeconds 300
}

if ($Program) {
    Invoke-StepScript `
        -ScriptPath (Join-Path $repoRoot "Tools\program.ps1") `
        -Arguments @("-ElfPath", "build\$Preset\HC_FW_BlackPill.elf") `
        -TimeoutSeconds 90
}

$cases = Get-Content -Raw -Path $caseFullPath | ConvertFrom-Json
$serialDevice = Get-UsbCdcPort -RequestedPortName $PortName
$PortName = $serialDevice.PortName
$portDescription = "$($serialDevice.PortName) [$($serialDevice.ProductString)]"

Write-Output "case file: $caseFullPath"
Write-Output "results: $resultDir"
Write-Output "serial port: $portDescription"

$results = [System.Collections.Generic.List[object]]::new()
$port = $null

try {
    $port = Open-TestSerialPort -Name $PortName -Speed $BaudRate

    foreach ($case in $cases.tests) {
        $result = Invoke-TestCase -Port $port -Case $case -TranscriptPath $transcriptPath
        $results.Add($result)
        $status = if ($result.pass) { "PASS" } else { "FAIL" }
        Write-Output "$status $($result.id)"

        if (($case.PSObject.Properties.Name -contains "program_after") -and [bool]$case.program_after) {
            Write-Output "RECOVER program_after $($case.id)"
            Close-TestSerialPort -Port $port
            $port = $null
            Invoke-StepScript `
                -ScriptPath (Join-Path $repoRoot "Tools\program.ps1") `
                -Arguments @("-ElfPath", "build\$Preset\HC_FW_BlackPill.elf") `
                -TimeoutSeconds 90
            $serialDevice = Get-UsbCdcPort -RequestedPortName $PortName
            $PortName = $serialDevice.PortName
            $port = Open-TestSerialPort -Name $PortName -Speed $BaudRate
        }

        if (($case.PSObject.Properties.Name -contains "reset_after") -and [bool]$case.reset_after) {
            Write-Output "RECOVER reset_after $($case.id)"
            Close-TestSerialPort -Port $port
            $port = $null
            Start-Sleep -Milliseconds 1500
            $serialDevice = Get-UsbCdcPort -RequestedPortName $PortName
            $PortName = $serialDevice.PortName
            $port = Open-TestSerialPort -Name $PortName -Speed $BaudRate
        }

        if ((-not $result.pass) -and (-not $ContinueOnFail)) {
            break
        }

        Start-Sleep -Milliseconds $InterTestDelayMs
    }
}
finally {
    Close-TestSerialPort -Port $port
}

Write-TestReport `
    -Results @($results) `
    -ReportPath $reportPath `
    -TranscriptPath $transcriptPath `
    -PortDescription $portDescription `
    -CaseFile $caseFullPath `
    -PresetName $Preset `
    -BuildRequested ([bool]$Build) `
    -ProgramRequested ([bool]$Program)

$archiveResult = $null
if (-not $SkipReportArchive) {
    $archiveRootFullPath = Join-Path $repoRoot $ReportRoot
    $archiveResult = Write-ArchivedReport `
        -SourceReportPath $reportPath `
        -SourceTranscriptPath $transcriptPath `
        -ArchiveRoot $archiveRootFullPath `
        -RunTimestamp $timestamp `
        -Results @($results) `
        -PresetName $Preset `
        -PortDescription $portDescription `
        -BuildRequested ([bool]$Build) `
        -ProgramRequested ([bool]$Program)
}

$failed = @($results | Where-Object { -not $_.pass })
Write-Output "summary: $reportPath"
Write-Output "transcript: $transcriptPath"
if ($archiveResult) {
    Write-Output "archived report: $($archiveResult.ReportPath)"
    Write-Output "archived transcript: $($archiveResult.TranscriptPath)"
    Write-Output "report index: $($archiveResult.IndexPath)"
}

Remove-OldRunLocalResults `
    -ResultRoot (Join-Path $buildDir "hw_tests") `
    -KeepCount $KeepRunLocalResultCount `
    -CurrentRunDirectory $resultDir

Write-Output "passed: $(@($results | Where-Object { $_.pass }).Count)"
Write-Output "failed: $($failed.Count)"

if ($failed.Count -gt 0) {
    exit 1
}
