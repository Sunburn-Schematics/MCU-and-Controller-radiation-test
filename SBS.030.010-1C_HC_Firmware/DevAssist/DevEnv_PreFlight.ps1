$ErrorActionPreference = 'Stop'

$FixedContainerName = 'STM32_HC_DevContainer'
$Workspace = '/workspaces/STM32/SBS.030.010-1C_HC_Firmware'
$ElfPath = 'build/SBS.030.010-1C_HC_Firmware.elf'
$BinPath = 'build/SBS.030.010-1C_HC_Firmware.bin'
$FlashHelper = 'DevAssist/Utils/flash_gdb.sh'
$ReadinessHelper = 'DevAssist/Utils/openocd_readiness_probe.sh'
$RequiredMarkers = @(
    'VID:PID 0483:3748',
    'Target voltage:',
    'Listening on port 3333'
)
$FailMarkers = @(
    'Error: open failed',
    'No device found',
    'init mode failed',
    'unable to open ftdi device'
)

function Invoke-Checked {
    param(
        [string]$FilePath,
        [string[]]$Arguments,
        [switch]$AllowFailure,
        [switch]$Quiet
    )

    $output = & $FilePath @Arguments 2>&1
    $exitCode = $LASTEXITCODE

    if (-not $Quiet -and $output) {
        $output | ForEach-Object { Write-Host $_ }
    }

    $result = [PSCustomObject]@{
        Output   = @($output)
        ExitCode = $exitCode
    }

    if (-not $AllowFailure -and $exitCode -ne 0) {
        throw "Command failed with exit code ${exitCode}: $FilePath $($Arguments -join ' ')"
    }

    return $result
}

function Write-Pass {
    param([string]$Message)
    Write-Host "[PASS] $Message"
}

function Write-Fail {
    param([string]$Message)
    Write-Host "[FAIL] $Message"
}
function Write-Warn {
    param([string]$Message)
    Write-Host "[WARN] $Message"
}

function Test-UsbipdAttachment {
    $res = Invoke-Checked -FilePath 'usbipd' -Arguments @('list') -AllowFailure -Quiet
    $joined = ($res.Output -join "`n")
    if ($res.ExitCode -ne 0) {
        Write-Fail 'Unable to run `usbipd list` on the host. Run DevAssist\setup_usbipd.ps1 as Administrator, then rerun preflight.'
        return $false
    }

    if ($joined -match '0483:3748' -and $joined -match 'Attached') {
        Write-Pass 'Host USB prerequisite satisfied: ST-Link appears attached via usbipd (setup_usbipd.ps1 result present)'
        return $true
    }

    Write-Fail 'Host USB prerequisite missing: ST-Link is not shown as attached via usbipd. Run DevAssist\setup_usbipd.ps1 as Administrator, then rerun preflight.'
    if ($joined.Trim()) { Write-Host $joined }
    return $false
}

function Get-ContainerName {
    $res = Invoke-Checked -FilePath 'docker' -Arguments @('ps', '--format', '{{.Names}}') -Quiet
    $names = @($res.Output | ForEach-Object { "$_".Trim() } | Where-Object { $_ })
    if (-not $names -or $names.Count -eq 0) {
        throw 'No devcontainer is running. Open VS Code and rebuild/reopen in container.'
    }
    if ($names -contains $FixedContainerName) {
        Write-Pass "Fixed devcontainer running: $FixedContainerName"
        return $FixedContainerName
    }
    Write-Warn "Fixed container name not found. Falling back to first running container: $($names[0])"
    return $names[0]
}

function Test-Workspace {
    param([string]$ContainerName)
    $res = Invoke-Checked -FilePath 'docker' -Arguments @('exec', $ContainerName, 'bash', '-lc', "cd $Workspace && pwd") -AllowFailure -Quiet
    if ($res.ExitCode -eq 0 -and (($res.Output -join "`n") -match [regex]::Escape($Workspace))) {
        Write-Pass "Workspace available in container: $Workspace"
        return $true
    }
    Write-Fail "Workspace missing or inaccessible in container: $Workspace"
    return $false
}

function Test-RequiredFiles {
    param([string]$ContainerName)
    $cmd = "cd $Workspace && test -f Makefile && test -f $FlashHelper && test -f $ReadinessHelper"
    $res = Invoke-Checked -FilePath 'docker' -Arguments @('exec', $ContainerName, 'bash', '-lc', $cmd) -AllowFailure -Quiet
    if ($res.ExitCode -eq 0) {
        Write-Pass 'Required project files are present (Makefile, flash_gdb.sh, openocd_readiness_probe.sh)'
        return $true
    }
    Write-Fail 'Required project files are missing'
    return $false
}

function Test-Tools {
    param([string]$ContainerName)
    $allPassed = $true

    $gcc = Invoke-Checked -FilePath 'docker' -Arguments @('exec', $ContainerName, 'bash', '-lc', 'arm-none-eabi-gcc --version | head -n 1') -AllowFailure -Quiet
    if ($gcc.ExitCode -eq 0 -and ($gcc.Output -join '').Trim()) {
        Write-Pass ("ARM GCC available: " + ($gcc.Output -join '').Trim())
    } else {
        Write-Fail 'ARM GCC missing inside container'
        $allPassed = $false
    }

    $make = Invoke-Checked -FilePath 'docker' -Arguments @('exec', $ContainerName, 'bash', '-lc', 'make --version | head -n 1') -AllowFailure -Quiet
    if ($make.ExitCode -eq 0 -and ($make.Output -join '').Trim()) {
        Write-Pass ("GNU Make available: " + ($make.Output -join '').Trim())
    } else {
        Write-Fail 'GNU Make missing inside container'
        $allPassed = $false
    }

    $openocd = Invoke-Checked -FilePath 'docker' -Arguments @('exec', $ContainerName, 'bash', '-lc', 'openocd --version 2>&1 | head -n 1') -AllowFailure -Quiet
    if ($openocd.ExitCode -eq 0 -and ($openocd.Output -join '').Trim()) {
        Write-Pass ("OpenOCD available: " + ($openocd.Output -join '').Trim())
    } else {
        Write-Fail 'OpenOCD missing inside container'
        $allPassed = $false
    }

    return $allPassed
}

function Test-Artifacts {
    param([string]$ContainerName)
    $cmd = "cd $Workspace && test -f $ElfPath && test -f $BinPath"
    $res = Invoke-Checked -FilePath 'docker' -Arguments @('exec', $ContainerName, 'bash', '-lc', $cmd) -AllowFailure -Quiet
    if ($res.ExitCode -eq 0) {
        Write-Pass 'Firmware artifacts are present (.elf, .bin)'
        return $true
    }
    Write-Fail 'Firmware artifacts are missing (.elf and/or .bin)'
    return $false
}

function Test-OpenOcdReadiness {
    param([string]$ContainerName)

    $prep = "cd $Workspace && tr -d '\r' < $ReadinessHelper > /tmp/devassist_preflight_readiness.sh && chmod +x /tmp/devassist_preflight_readiness.sh"
    $prepRes = Invoke-Checked -FilePath 'docker' -Arguments @('exec', $ContainerName, 'bash', '-lc', $prep) -AllowFailure -Quiet
    if ($prepRes.ExitCode -ne 0) {
        Write-Fail 'Failed to prepare normalized readiness helper in container.'
        return $false
    }

    $run = "bash /tmp/devassist_preflight_readiness.sh /tmp/devassist_preflight_openocd.log"
    $res = Invoke-Checked -FilePath 'docker' -Arguments @('exec', $ContainerName, 'bash', '-lc', $run) -AllowFailure -Quiet
    $joined = ($res.Output -join "`n")

    foreach ($marker in $FailMarkers) {
        if ($joined -match [regex]::Escape($marker)) {
            Write-Fail 'OpenOCD readiness probe failed. ST-Link is not openable from this container path.'
            if ($joined.Trim()) { Write-Host $joined }
            return $false
        }
    }

    $allMarkersPresent = $true
    foreach ($marker in $RequiredMarkers) {
        if ($joined -notmatch [regex]::Escape($marker)) {
            $allMarkersPresent = $false
        }
    }
    if ($allMarkersPresent) {
        Write-Pass 'OpenOCD readiness gate passed (VID:PID, target voltage, GDB server startup)'
        return $true
    }
    Write-Fail 'OpenOCD readiness proof markers were incomplete.'
    if ($joined.Trim()) { Write-Host $joined }
    return $false
}


try {
    Write-Host '=== DevAssist Startup Environment Pre-Flight Check ==='
    $allPassed = $true

    $repoDir = Get-Location
    if (Test-Path $repoDir) {
        Write-Pass "Host project workspace located: $repoDir"
    } else {
        Write-Fail "Host project workspace missing at $repoDir"
        $allPassed = $false
    }

    if (-not (Test-UsbipdAttachment)) { $allPassed = $false }
    $container = Get-ContainerName

    if (-not (Test-Workspace -ContainerName $container)) { $allPassed = $false }
    if (-not (Test-RequiredFiles -ContainerName $container)) { $allPassed = $false }
    if (-not (Test-Tools -ContainerName $container)) { $allPassed = $false }
    if (-not (Test-Artifacts -ContainerName $container)) { $allPassed = $false }
    if (-not (Test-OpenOcdReadiness -ContainerName $container)) { $allPassed = $false }

    Write-Host '========================================================'
    if ($allPassed) {
        Write-Host '>>> ALL CHECKS PASSED. Environment and hardware path are ready. <<<'
        exit 0
    } else {
        Write-Host '>>> PRE-FLIGHT CHECKS FAILED. Resolve issues before continuing. <<<'
        exit 1
    }
}
catch {
    Write-Host "[FAIL] $($_.Exception.Message)"
    Write-Host '========================================================'
    Write-Host '>>> PRE-FLIGHT CHECKS FAILED. Resolve issues before continuing. <<<'
    exit 1
}
