$ErrorActionPreference = 'Stop'

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

function Get-DevContainerName {
    $res = Invoke-Checked -FilePath 'docker' -Arguments @('ps', '--format', '{{.Names}}') -Quiet
    $names = @($res.Output | ForEach-Object { "$_".Trim() } | Where-Object { $_ })
    if (-not $names -or $names.Count -eq 0) {
        throw 'No running devcontainer found.'
    }
    return $names[0]
}

function Get-WorkspacePath {
    param([string]$ContainerName)

    $res = Invoke-Checked -FilePath 'docker' -Arguments @('exec', $ContainerName, 'bash', '-lc', 'find /workspaces -name Makefile -type f 2>/dev/null') -Quiet
    $makefiles = @($res.Output | ForEach-Object { "$_".Trim() } | Where-Object { $_ })
    foreach ($mf in $makefiles) {
        if ($mf -match 'SBS\.030\.010-1C_HC_Firmware') {
            return [System.IO.Path]::GetDirectoryName($mf).Replace('\','/')
        }
    }
    if ($makefiles.Count -gt 0) {
        return [System.IO.Path]::GetDirectoryName($makefiles[0]).Replace('\','/')
    }
    throw 'Could not locate project workspace in devcontainer.'
}

function Clear-OpenOcd {
    param([string]$ContainerName)
    Write-Host 'Cleaning up stale OpenOCD sessions...'
    [void](Invoke-Checked -FilePath 'docker' -Arguments @('exec', $ContainerName, 'bash', '-lc', 'pkill -f openocd || true; sleep 1') -AllowFailure -Quiet)
}

function Invoke-Build {
    param([string]$ContainerName, [string]$Workspace)
    Write-Host "Building in $Workspace ..."
    [void](Invoke-Checked -FilePath 'docker' -Arguments @('exec', '-w', $Workspace, $ContainerName, 'make', '-j4'))
}

function Test-Artifacts {
    param([string]$ContainerName, [string]$Workspace)
    Write-Host 'Validating build artifacts...'
    [void](Invoke-Checked -FilePath 'docker' -Arguments @('exec', '-w', $Workspace, $ContainerName, 'bash', '-lc', 'test -f build/SBS.030.010-1C_HC_Firmware.elf && test -f build/SBS.030.010-1C_HC_Firmware.bin && ls -l build/SBS.030.010-1C_HC_Firmware.elf build/SBS.030.010-1C_HC_Firmware.bin'))
}

function Invoke-FlashGdb {
    param([string]$ContainerName, [string]$Workspace)
    Write-Host 'Flashing target via readiness-gated OpenOCD + GDB helper...'
    $cmd = "set -e; cd '$Workspace'; sed 's/\r$//' DevAssist/Utils/flash_gdb.sh > /tmp/flash_gdb.sh; chmod +x /tmp/flash_gdb.sh; /tmp/flash_gdb.sh build/SBS.030.010-1C_HC_Firmware.elf"
    $res = Invoke-Checked -FilePath 'docker' -Arguments @('exec', '-w', $Workspace, $ContainerName, 'bash', '-lc', $cmd) -AllowFailure
    $joined = ($res.Output -join "`n").ToLowerInvariant()
    $joined = ($res.Output -join "`n").ToLowerInvariant()

    if ($res.ExitCode -ne 0) {
        throw "Readiness-gated flash returned a non-zero exit code: $($res.ExitCode)"
    }
    if ($joined -notmatch 'flash_ok') {
        throw 'Flash helper completed without explicit FLASH_OK marker. Treating flash as failed.'
    }
    if (($joined -notmatch 'loading section') -and ($joined -notmatch 'transfer rate') -and ($joined -notmatch 'start address')) {
        throw 'Flash helper completed without explicit GDB load markers. Treating flash as failed.'
    }
    if (($joined -notmatch 'vid:pid 0483:3748') -or ($joined -notmatch 'target voltage:') -or ($joined -notmatch 'starting gdb server')) {
        throw 'OpenOCD readiness markers were missing. Treating flash as failed.'
    }
    if ($joined -match 'error:' -or $joined -match 'timed out' -or $joined -match 'no such file') {
        throw 'GDB/OpenOCD reported an error during flash.'
    }

    Write-Host 'Readiness-gated GDB flash completed successfully.'
}

try {
    $container = Get-DevContainerName
    Write-Host "Using devcontainer: $container"
    $workspace = Get-WorkspacePath -ContainerName $container
    Write-Host "Using workspace: $workspace"
    Clear-OpenOcd -ContainerName $container
    Invoke-Build -ContainerName $container -Workspace $workspace
    Test-Artifacts -ContainerName $container -Workspace $workspace
    Clear-OpenOcd -ContainerName $container
    Invoke-FlashGdb -ContainerName $container -Workspace $workspace
    Write-Host 'Build and flash sequence complete.'
    exit 0
}
catch {
    Write-Host "ERROR: $($_.Exception.Message)"
    exit 1
}
