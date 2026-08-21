# Download the existing STM32F4 HEX image with SEGGER J-Link.
# This script intentionally does not build the firmware.

$ErrorActionPreference = 'Stop'

$projectRoot = Split-Path -Parent $PSScriptRoot
$jlinkExe = 'D:\App\Code\JLink\JLink_V968a\JLink.exe'
$firmwareHex = Join-Path $projectRoot 'build\stm32f4\rtthread-stm32f4.hex'

if (-not (Test-Path -LiteralPath $jlinkExe -PathType Leaf)) {
    throw "J-Link executable not found: $jlinkExe"
}

if (-not (Test-Path -LiteralPath $firmwareHex -PathType Leaf)) {
    throw "Firmware HEX not found: $firmwareHex"
}

$commanderScript = Join-Path ([IO.Path]::GetTempPath()) 'stm32f4-download.jlink'
$commands = @(
    'RSetType 0'
    'r'
    'h'
    ('loadfile "{0}"' -f $firmwareHex)
    'r'
    'sleep 100'
    'g'
    'exit'
)

[IO.File]::WriteAllLines($commanderScript, $commands, [Text.Encoding]::ASCII)

try {
    & $jlinkExe -Device STM32F407ZG -If SWD -Speed 4000 -AutoConnect 1 -CommanderScript $commanderScript
    if ($LASTEXITCODE -ne 0) {
        throw "J-Link download failed with exit code $LASTEXITCODE."
    }
}
finally {
    if (Test-Path -LiteralPath $commanderScript -PathType Leaf) {
        Remove-Item -LiteralPath $commanderScript -Force
    }
}
