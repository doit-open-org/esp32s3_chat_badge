$ErrorActionPreference = "Stop"

$RootDir = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$BuildDir = if ($env:BUILD_DIR) { $env:BUILD_DIR } else { Join-Path $RootDir "build" }
$Defaults = "sdkconfig.defaults;sdkconfig.defaults.esp32s3;main/boards/esp32s3-chat-badge/sdkconfig.defaults"

if (-not (Get-Command idf.py -ErrorAction SilentlyContinue)) {
    throw "idf.py was not found. Open an ESP-IDF PowerShell or run export.ps1 first."
}

Push-Location $RootDir
try {
    & idf.py `
        -B $BuildDir `
        "-DSDKCONFIG=$BuildDir/sdkconfig" `
        "-DSDKCONFIG_DEFAULTS=$Defaults" `
        -DIDF_TARGET=esp32s3 `
        build
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}
finally {
    Pop-Location
}
