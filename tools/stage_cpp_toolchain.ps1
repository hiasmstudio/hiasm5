[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Destination,
    [string]$QtDir = 'C:/Qt/6.9.3/llvm-mingw_64',
    [string]$LlvmDir = 'C:/Qt/Tools/llvm-mingw1706_64',
    [string]$CMakeDir = 'C:/Qt/Tools/CMake_64',
    [string]$Ninja = 'C:/Qt/Tools/Ninja/ninja.exe',
    [switch]$SkipVerify,
    [switch]$OffscreenOnly,
    [switch]$VerifyOnly
)
$ErrorActionPreference = 'Stop'
$destinationPath = [IO.Path]::GetFullPath($Destination)
$llvm = Join-Path $destinationPath 'llvm-mingw'
$qt = Join-Path $destinationPath 'qt6'
$tools = Join-Path $destinationPath 'tools'

function Copy-Tree($Source, $Target, $ExtraArgs) {
    & robocopy.exe $Source $Target /E /MT:8 /XJ /NFL /NDL /NJH /NJS /NP /R:1 /W:1 @ExtraArgs
    if ($LASTEXITCODE -ge 8) { throw "Copy failed: $Source ($LASTEXITCODE)" }
}

if (!$VerifyOnly) {
    foreach ($required in @("$QtDir/lib/cmake/Qt6/Qt6Config.cmake", "$QtDir/bin/Qt6Widgets.dll",
            "$LlvmDir/bin/clang++.exe", "$LlvmDir/LICENSE.TXT", "$CMakeDir/bin/cmake.exe", $Ninja)) {
        if (!(Test-Path -LiteralPath $required -PathType Leaf)) { throw "Missing toolchain file: $required" }
    }
    foreach ($target in @($llvm, $qt, $tools)) {
        if (Test-Path -LiteralPath $target) { throw "Toolchain destination already exists: $target" }
    }
    Copy-Tree $LlvmDir $llvm @('/XD', 'aarch64-w64-mingw32', 'armv7-w64-mingw32', 'i686-w64-mingw32', 'python')
    Copy-Tree $QtDir $qt @('/XD', 'doc', 'qml', 'phrasebooks', 'qtinsight',
        '/XF', '*.debug', '*.pdb', 'config_*.summary', 'config_*.opt')
    Copy-Tree $CMakeDir $tools @('/XD', 'doc', 'man')
    Copy-Item -LiteralPath $Ninja -Destination (Join-Path $tools 'bin/ninja.exe')
}

if ($SkipVerify) { return }
$oldPath = $env:PATH
$oldPlatform = $env:QT_QPA_PLATFORM
$oldPlugins = $env:QT_PLUGIN_PATH
$verify = Join-Path ([IO.Path]::GetTempPath()) ('hiasm-toolchain-' + [guid]::NewGuid())
New-Item -ItemType Directory -Path $verify | Out-Null
try {
    $env:PATH = "$qt/bin;$llvm/bin;$tools/bin;$env:SystemRoot/System32;$env:SystemRoot"
    $env:QT_PLUGIN_PATH = "$qt/plugins"
    $source = Join-Path $PSScriptRoot '../tests/cpp_qt_toolchain_probe.cpp'
    $console = Join-Path $verify 'console.exe'
    & "$llvm/bin/clang++.exe" -std=c++17 $source -o $console
    if ($LASTEXITCODE) { throw 'LLVM-MinGW console compilation failed' }
    & $console
    if ($LASTEXITCODE) { throw 'LLVM-MinGW console launch failed' }

    & "$tools/bin/cmake.exe" -S "$PSScriptRoot/../packs/cpp/qt" -B $verify -G Ninja `
        "-DCMAKE_PREFIX_PATH=$qt" "-DCMAKE_CXX_COMPILER=$llvm/bin/clang++.exe" `
        "-DCMAKE_MAKE_PROGRAM=$tools/bin/ninja.exe" "-DHIASM_SOURCE=$source" '-DHIASM_TOOLCHAIN_PROBE=ON'
    if ($LASTEXITCODE) { throw 'Bundled Qt configure failed' }
    & "$tools/bin/cmake.exe" --build $verify
    if ($LASTEXITCODE) { throw 'Bundled Qt compilation failed' }
    $platforms = if ($OffscreenOnly) { @('offscreen') } else { @('offscreen', 'windows') }
    foreach ($platform in $platforms) {
        $env:QT_QPA_PLATFORM = $platform
        $program = Start-Process -FilePath (Join-Path $verify 'program.exe') -PassThru -WindowStyle Hidden
        if (!$program.WaitForExit(15000)) { $program.Kill(); throw "Bundled Qt launch timed out ($platform)" }
        if ($program.ExitCode) { throw "Bundled Qt launch failed ($platform, exit $($program.ExitCode))" }
    }
    Write-Output 'OK: bundled LLVM-MinGW and Qt Widgets compile and launch with isolated PATH'
} finally {
    $env:PATH = $oldPath
    $env:QT_QPA_PLATFORM = $oldPlatform
    $env:QT_PLUGIN_PATH = $oldPlugins
    Remove-Item -LiteralPath $verify -Recurse -Force
}
