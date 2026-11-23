[CmdletBinding()]
param(
    [string]$QtDir = $env:HIASM_QT_DIR,
    [string]$CompilerDir = $env:HIASM_QT_COMPILER_DIR,
    [string]$Ninja = $env:HIASM_NINJA,
    [string]$CMake = $env:HIASM_CMAKE,
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Release',
    [string]$BuildDir,
    [string]$InstallDir,
    [switch]$SkipBuild,
    [switch]$SkipTests,
    [switch]$RtcgOnly,
    [switch]$RunIde,
    [switch]$Check
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if (!$QtDir) { $QtDir = 'C:/Qt/6.9.3/llvm-mingw_64' }
if (!$CompilerDir) { $CompilerDir = 'C:/Qt/Tools/llvm-mingw1706_64/bin' }
$sdkTools = [IO.Path]::GetFullPath((Join-Path $QtDir '../../Tools'))
if (!$Ninja) { $Ninja = Join-Path $sdkTools 'Ninja/ninja.exe' }
if (!$CMake) {
    $CMake = Join-Path $sdkTools 'CMake_64/bin/cmake.exe'
    if (!(Test-Path -LiteralPath $CMake -PathType Leaf)) {
        $CMake = (Get-Command cmake.exe -CommandType Application -ErrorAction SilentlyContinue).Source
        if (!$CMake) { throw 'CMake not found. Install CMake from the Qt SDK or set HIASM_CMAKE.' }
    }
}
if (!$BuildDir) { $BuildDir = Join-Path $root "build/qt69-native-$($Configuration.ToLowerInvariant())" }
if ($RtcgOnly -and $InstallDir) { throw 'InstallDir requires a complete Qt build' }
if ($RtcgOnly -and $RunIde) { throw 'Launching an application requires the Qt build' }
if ($Check -and !$RunIde) { throw 'Check requires RunIde' }
$BuildDir = [IO.Path]::GetFullPath($BuildDir)
if (!$RtcgOnly) { $QtDir = (Resolve-Path -LiteralPath $QtDir).Path }
$compilerCandidate = Join-Path $CompilerDir 'clang++.exe'
if (!(Test-Path -LiteralPath $compilerCandidate -PathType Leaf)) { $compilerCandidate = Join-Path $CompilerDir 'g++.exe' }
$compiler = (Resolve-Path -LiteralPath $compilerCandidate).Path
$Ninja = (Resolve-Path -LiteralPath $Ninja).Path
$CMake = (Resolve-Path -LiteralPath $CMake).Path
$CTest = Join-Path (Split-Path -Parent $CMake) 'ctest.exe'
$oldPath = $env:PATH
$oldPlatform = $env:QT_QPA_PLATFORM
function Invoke-Tool($Program, $ToolArgs) {
    & $Program @ToolArgs
    if ($LASTEXITCODE -ne 0) { throw "$Program failed (exit $LASTEXITCODE)" }
}
try {
    $env:PATH = "$(Split-Path -Parent $compiler);$oldPath"
    if (!$RtcgOnly) { $env:PATH = "$(Join-Path $QtDir 'bin');$env:PATH" }
    if (!$SkipBuild) {
        $configureArgs = @('-S', $root, '-B', $BuildDir, '-G', 'Ninja',
            "-DCMAKE_BUILD_TYPE=$Configuration", '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
            "-DCMAKE_CXX_COMPILER=$compiler", "-DCMAKE_MAKE_PROGRAM=$Ninja")
        if ($RtcgOnly) { $configureArgs += '-DHIASM_BUILD_RTCG_ONLY=ON' }
        else {
            $configureArgs += @('-DHIASM_BUILD_RTCG_ONLY=OFF',
                '-DBUILD_TESTING=ON', "-DCMAKE_PREFIX_PATH=$QtDir")
        }
        Invoke-Tool $CMake $configureArgs
        $buildArgs = @('--build', $BuildDir, '--parallel', '6')
        if ($RtcgOnly) { $buildArgs += @('--target', 'rtcg_codegen') }
        Invoke-Tool $CMake $buildArgs
    }
    if (!$SkipTests -and !$RtcgOnly) { Invoke-Tool $CTest @('--test-dir', $BuildDir, '--output-on-failure', '--no-tests=error') }
    if ($RtcgOnly) {
        $codegen = Join-Path $BuildDir 'packs/cpp/compiler/CodeGen.dll'
        if (!(Test-Path -LiteralPath $codegen -PathType Leaf)) { throw "Missing RTCG output: $codegen" }
        Write-Output "Built: $codegen"
    }
    if ($InstallDir) { Invoke-Tool $CMake @('--install', $BuildDir, '--prefix', [IO.Path]::GetFullPath($InstallDir)) }
    $runArgs = @()
    if ($Check) {
        $env:QT_QPA_PLATFORM = 'offscreen'
        $runArgs += '--check'
    }
    if ($RunIde) {
        Invoke-Tool (Join-Path $BuildDir 'hiasm.exe') $runArgs
    }
} finally {
    $env:PATH = $oldPath
    $env:QT_QPA_PLATFORM = $oldPlatform
}
