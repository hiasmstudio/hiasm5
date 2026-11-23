[CmdletBinding()]
param(
    [string]$MakeNsis = $env:HIASM_MAKENSIS,
    [switch]$RuntimeOnly
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path "$PSScriptRoot/..").Path
if (!$MakeNsis) {
    $MakeNsis = (Get-Command makensis.exe -ErrorAction SilentlyContinue).Source
    if (!$MakeNsis) { $MakeNsis = "${env:ProgramFiles(x86)}/NSIS/makensis.exe" }
}
$MakeNsis = (Resolve-Path -LiteralPath $MakeNsis).Path
# A fresh prefix prevents untracked files from an earlier install entering the payload.
$stageParent = [IO.Path]::GetFullPath((Join-Path $root 'build/windows-installer'))
$stageName = 'qt-' + [guid]::NewGuid().ToString('N')
$stage = Join-Path $stageParent $stageName
& "$PSScriptRoot/build_qt_windows.ps1" -Configuration Release -InstallDir $stage
& "$PSScriptRoot/smoke_windows.ps1" -InstallDir $stage
if (!$RuntimeOnly) {
    $paths = @{}
    if ($env:HIASM_QT_DIR) { $paths.QtDir = $env:HIASM_QT_DIR }
    if ($env:HIASM_QT_COMPILER_DIR) { $paths.LlvmDir = Split-Path -Parent $env:HIASM_QT_COMPILER_DIR }
    if ($env:HIASM_CMAKE) { $paths.CMakeDir = Split-Path -Parent (Split-Path -Parent $env:HIASM_CMAKE) }
    if ($env:HIASM_NINJA) { $paths.Ninja = $env:HIASM_NINJA }
    & "$PSScriptRoot/stage_cpp_toolchain.ps1" -Destination "$stage/packs/cpp/compiler" -OffscreenOnly @paths
}
$cache = Get-Content -LiteralPath "$root/build/qt69-native-release/CMakeCache.txt"
$version = ($cache | Select-String '^CMAKE_PROJECT_VERSION:STATIC=(.+)$').Matches.Groups[1].Value
$outputName = if ($RuntimeOnly) { 'HiAsm_Setup_Qt_Runtime.exe' } else { 'HiAsm_Setup_Qt.exe' }
$output = Join-Path "$root/build" $outputName
$compiledInstaller = "$stage.exe"
& $MakeNsis /V2 "/DBUILD_DIR=$stage" "/DOUTPUT_FILE=$compiledInstaller" "/DPRODUCT_VERSION=$version" "$PSScriptRoot/nsis_installer.nsi"
if ($LASTEXITCODE) { throw "NSIS failed ($LASTEXITCODE)" }
if (Test-Path -LiteralPath $output) {
    [IO.File]::Replace($compiledInstaller, $output, [NullString]::Value)
} else {
    [IO.File]::Move($compiledInstaller, $output)
}
Write-Output "Installer: $output"
$scratch = Get-Item -LiteralPath $stage
if ($scratch.Parent.FullName -eq $stageParent -and $scratch.Name -eq $stageName -and
    !($scratch.Attributes -band [IO.FileAttributes]::ReparsePoint)) {
    Remove-Item -LiteralPath $scratch.FullName -Recurse -Force
}
