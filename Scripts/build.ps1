# RT-Thread BSP CMake build script.
#
# CMake owns ELF/HEX/BIN generation; this script only validates local tools,
# configures one explicitly selected BSP and invokes the build.
#
# BSP 切换说明：已接入的 BSP 可通过 -TargetBsp <名称> 选择，无需修改脚本。
# 新增 BSP 时，须将与顶层 CMake TARGET_BSP 一致的名称加入下方 ValidateSet；
# 只有希望改变默认构建目标时，才修改 $TargetBsp 的默认值。
[CmdletBinding()]
param(
    # BSP 切换参数：名称必须与顶层 CMakeLists.txt 和 CMakePresets.json 中的
    # TARGET_BSP 一致。新增 BSP 时在 ValidateSet 中添加其名称。
    # 默认值决定未传入 -TargetBsp 时构建的 BSP。
    [ValidateSet('cc2538', 'stm32f4')]
    [string]$TargetBsp = 'stm32f4',

    # ARM GNU Toolchain root; bin\arm-none-eabi-gcc.exe must exist below it.
    # 常规 BSP 切换无需修改；仅在新 BSP 使用不同工具链安装位置时调整。
    [string]$ArmToolchainRoot = 'C:/msys64/ucrt64',

    # Ninja is intentionally configured outside CMake source files.
    # 常规 BSP 切换无需修改；仅在 Ninja 安装位置变化时调整。
    [string]$NinjaExe = 'C:/msys64/ucrt64/bin/ninja.exe',

    # Empty selects <project>/build/<TargetBsp>.  It can be overridden for CI
    # or a temporary validation build without changing project files. BSP 切换时
    # 保持为空即可自动使用与目标 BSP 隔离的构建目录。
    [string]$BuildDirectory = '',

    # Reconfigure from a clean CMake cache when the compiler or generator changed.
    [switch]$Fresh
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot

function Require-File {
    param(
        [Parameter(Mandatory)] [string]$Path,
        [Parameter(Mandatory)] [string]$Description
    )

    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) {
        throw "$Description not found: $Path"
    }
}

function Resolve-GlobalCommand {
    param(
        [Parameter(Mandatory)] [string]$Name,
        [Parameter(Mandatory)] [string]$Description
    )

    $command = Get-Command -Name $Name -CommandType Application -ErrorAction SilentlyContinue |
        Select-Object -First 1
    if ($null -eq $command) {
        throw "$Description is not available in the global Path: $Name"
    }

    return $command.Source
}

$armBin = Join-Path $ArmToolchainRoot 'bin'
$gcc = Join-Path $armBin 'arm-none-eabi-gcc.exe'

# Make the selected toolchain's compiler support tools and bundled CMake
# available to this process before resolving commands.
$env:Path = "$armBin;$env:Path"
$cmake = Resolve-GlobalCommand -Name 'cmake.exe' -Description 'CMake'

Require-File -Path $gcc -Description 'ARM GCC'
Require-File -Path $NinjaExe -Description 'Ninja'

if ([string]::IsNullOrWhiteSpace($BuildDirectory)) {
    $buildDirectory = Join-Path $projectRoot (Join-Path 'build' $TargetBsp)
}
elseif (-not [IO.Path]::IsPathRooted($BuildDirectory)) {
    $buildDirectory = Join-Path $projectRoot $BuildDirectory
}

$firmwareBaseName = "rtthread-$TargetBsp"
$elf = Join-Path $buildDirectory "$firmwareBaseName.elf"
$hex = Join-Path $buildDirectory "$firmwareBaseName.hex"
$bin = Join-Path $buildDirectory "$firmwareBaseName.bin"
$map = Join-Path $buildDirectory "$firmwareBaseName.map"

Write-Host "Project:       $projectRoot"
Write-Host "BSP:           $TargetBsp"
Write-Host "Build dir:     $buildDirectory"
Write-Host "CMake:         $cmake"
Write-Host "Ninja:         $NinjaExe"
Write-Host "ARM GCC:       $gcc"

$configureArguments = @(
    '-S', $projectRoot,
    '-B', $buildDirectory,
    '-G', 'Ninja',
    '-DCMAKE_SYSTEM_NAME=Generic',
    '-DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY',
    "-DCMAKE_C_COMPILER=$gcc",
    "-DCMAKE_ASM_COMPILER=$gcc",
    "-DCMAKE_MAKE_PROGRAM=$NinjaExe",
    "-DTARGET_BSP=$TargetBsp"
)

if ($Fresh) {
    $configureArguments = @('--fresh') + $configureArguments
}

& $cmake @configureArguments
if ($LASTEXITCODE -ne 0) {
    throw "CMake configuration failed with exit code $LASTEXITCODE."
}

& $cmake --build $buildDirectory --parallel
if ($LASTEXITCODE -ne 0) {
    throw "Build failed with exit code $LASTEXITCODE."
}

Require-File -Path $elf -Description 'Firmware ELF'
Require-File -Path $hex -Description 'Firmware HEX'
Require-File -Path $bin -Description 'Firmware BIN'
Require-File -Path $map -Description 'Firmware MAP'

Write-Host "`nBuild completed:"
Write-Host "ELF: $elf"
Write-Host "HEX: $hex"
Write-Host "BIN: $bin"
Write-Host "MAP: $map"
