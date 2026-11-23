[CmdletBinding()]
param([string]$BuildDir, [string]$InstallDir, [ValidateRange(1,10)][int]$StartupRuns = 2)
$ErrorActionPreference = 'Stop'
if (!$InstallDir) { $InstallDir = "$PSScriptRoot/../build/qa/portable-qt-$([guid]::NewGuid())" }
& "$PSScriptRoot/build_qt_windows.ps1" -Configuration Release -BuildDir $BuildDir -SkipBuild -SkipTests -InstallDir $InstallDir
for ($i = 0; $i -lt $StartupRuns; ++$i) {
    & "$PSScriptRoot/smoke_windows.ps1" -InstallDir $InstallDir
}
