
Write-Host "Checking for usbipd-win..."
if (-not (Get-Command usbipd -ErrorAction SilentlyContinue)) {
    Write-Host "usbipd-win is missing! Attempting to install via winget..." -ForegroundColor Yellow
    winget install --interactive --exact dorssel.usbipd-win
    Write-Host "INSTALLATION COMPLETE: You must close and reopen this terminal (as Administrator) to refresh your PATH, then run this script again." -ForegroundColor Cyan
    exit
}

Write-Host "usbipd found! version:" (usbipd --version)
Write-Host "Locating ST-Link..."

$listOutput = usbipd list
Write-Host $listOutput

$match = $listOutput | Select-String -Pattern "^\s*([0-9]+-[0-9]+)\s+.*(?:ST-Link|STMicroelectronics)"
if ($match) {
    $busid = $match.Matches[0].Groups[1].Value
    Write-Host "Found ST-Link on BUSID: $busid" -ForegroundColor Green
    
    Write-Host "Binding..."
    usbipd bind --busid $busid
    
    Write-Host "Attaching to WSL..."
    usbipd attach --wsl --busid $busid
    
    Write-Host "Done! Your ST-Link is now passed through to WSL/Docker. You can attach Cortex-Debug now." -ForegroundColor Green
} else {
    Write-Host "Could not automatically find the ST-Link in the usbipd list. Is it plugged in?" -ForegroundColor Red
}
