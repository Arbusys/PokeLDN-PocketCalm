<#
  PokéLDN T-Dongle S3 Pocket Calm v0.7 - Windows build/flash helper.
  Run this script from the ESP-IDF v6.1 PowerShell shortcut.

  Build:   .\build_tdongle_windows.ps1
  Flash:   .\build_tdongle_windows.ps1 -Port COM5 -Flash
#>
param(
    [string]$Port = "",
    [switch]$Flash
)

$ErrorActionPreference = "Stop"
if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    throw "ESP-IDF not found. Open the ESP-IDF v6.1 PowerShell terminal first."
}
if ($Flash -and [string]::IsNullOrWhiteSpace($Port)) {
    throw "Specify the USB port, for example -Port COM5 -Flash."
}

# On Windows, devkitPro/MSYS2 may precede Espressif in PATH. That CMake
# interprets F:\... paths as Unix-relative paths and cannot build IDF.
# Prefer the native CMake installed by idf_tools.py, without changing the
# user's permanent PATH or breaking their devkitPro installation.
if ($env:IDF_TOOLS_PATH) {
    $CmakeFolder = Join-Path $env:IDF_TOOLS_PATH "tools\cmake"
    if (Test-Path -LiteralPath $CmakeFolder) {
        $EspCmake = Get-ChildItem -LiteralPath $CmakeFolder -Recurse -File -Filter "cmake.exe" |
                    Sort-Object FullName -Descending | Select-Object -First 1
        if ($EspCmake) {
            $env:Path = "$($EspCmake.DirectoryName);$env:Path"
            Write-Host "Native ESP-IDF CMake: $($EspCmake.FullName)" -ForegroundColor Cyan
        }
    }
}
$UsedCmake = (Get-Command cmake -ErrorAction SilentlyContinue).Source
if (-not $UsedCmake -or $UsedCmake -match "\\msys2\\usr\\bin\\cmake.exe$") {
    throw "Native Windows CMake not found. Install it with: python `"$env:IDF_PATH\tools\idf_tools.py`" install cmake"
}

$env:POKELDN_DISPLAY = "T_DONGLE_S3"
$Firmware = Join-Path $PSScriptRoot "firmware\esp32"
Push-Location $Firmware
try {
    & idf.py set-target esp32s3
    if ($LASTEXITCODE -ne 0) { throw "ESP32-S3 target setup failed." }
    & idf.py build
    if ($LASTEXITCODE -ne 0) { throw "Firmware build failed." }
    Write-Host "English Pocket Calm firmware built for the LILYGO T-Dongle S3." -ForegroundColor Green

    if ($Flash) {
        Write-Host "Flashing $Port : make sure PokéLDN is closed." -ForegroundColor Yellow
        & idf.py -p $Port flash
        if ($LASTEXITCODE -ne 0) { throw "Firmware flash failed." }
        Write-Host "Flash complete. Unplug and reconnect the dongle." -ForegroundColor Green
    }
} finally {
    Pop-Location
}
