param(
  [string]$Distro = "Ubuntu",
  [string]$Emulator = "emery",
  [string]$ProjectPath = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
)

$ErrorActionPreference = "Stop"

function Quote-Bash([string]$Value) {
  return "'" + ($Value -replace "'", "'\''") + "'"
}

function Invoke-WslText([string]$Command) {
  $output = & wsl.exe -d $Distro -- bash -lc $Command
  if ($LASTEXITCODE -ne 0) {
    throw "WSL command failed: $Command"
  }
  return ($output -join "`n")
}

$projectWslPath = (Invoke-WslText ("wslpath -a " + (Quote-Bash $ProjectPath))).Trim()
$wslIp = (Invoke-WslText "hostname -I | awk '{print `$1}'").Trim()

$shimPath = "\\wsl.localhost\$Distro\tmp\pebble-capture-browser.sh"
@'
#!/bin/sh
printf '%s\n' "$1" > /tmp/pebble-opened-url
exit 0
'@ | Set-Content -NoNewline -Encoding ASCII $shimPath

Invoke-WslText "chmod +x /tmp/pebble-capture-browser.sh"
Invoke-WslText "pkill -f '[p]ebble emu-app-config' || true; rm -f /tmp/pebble-opened-url /tmp/pebble-config.out /tmp/pebble-config.err"

$bashProject = Quote-Bash $projectWslPath
$bashEmulator = Quote-Bash $Emulator
$configCommand = "cd $bashProject && BROWSER=/tmp/pebble-capture-browser.sh ~/.local/bin/pebble emu-app-config --emulator $bashEmulator >/tmp/pebble-config.out 2>/tmp/pebble-config.err"

$process = Start-Process -FilePath "wsl.exe" -ArgumentList @("-d", $Distro, "--", "bash", "-lc", $configCommand) -PassThru -WindowStyle Hidden

$openedUrl = $null
for ($i = 0; $i -lt 40; $i++) {
  Start-Sleep -Milliseconds 250
  $openedUrl = ((& wsl.exe -d $Distro -- bash -lc "cat /tmp/pebble-opened-url 2>/dev/null") -join "`n").Trim()
  if ($openedUrl) {
    break
  }
}

if (-not $openedUrl) {
  $stderr = (& wsl.exe -d $Distro -- bash -lc "cat /tmp/pebble-config.err 2>/dev/null") -join "`n"
  throw "Pebble did not produce a config URL. $stderr"
}

if ($openedUrl -notlike "file://*") {
  Start-Process $openedUrl
  Write-Host "Opened Pebble config page."
  Write-Host "Config process PID: $($process.Id)"
  return
}

$redirectPath = $openedUrl.Substring("file://".Length)
$redirectHtml = (& wsl.exe -d $Distro -- bash -lc ("cat " + (Quote-Bash $redirectPath))) -join "`n"
$redirectHtml = $redirectHtml -replace "return_to=http%3A%2F%2Flocalhost%3A([0-9]+)%2Fclose%3F", ("return_to=http%3A%2F%2F" + $wslIp + '%3A$1%2Fclose%3F')

$target = Join-Path $env:TEMP "pebble-klipper-config-live.html"
$redirectHtml | Set-Content -NoNewline -Encoding UTF8 $target

Start-Process $target

Write-Host "Opened Pebble config page: $target"
Write-Host "Leave the emulator running. After you click Save Settings, the Pebble config process should close by itself."
