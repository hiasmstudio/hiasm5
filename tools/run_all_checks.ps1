[CmdletBinding()]
param([switch]$SkipBuild, [switch]$SkipPortable, [switch]$Package,
      [string]$CppRuntime, [string]$Python = 'py', [switch]$QtOnly)
$ErrorActionPreference = 'Stop'
& "$PSScriptRoot/validate_databases.ps1" -Python $Python
foreach ($configuration in 'Debug','Release') {
    & "$PSScriptRoot/build_qt_windows.ps1" -Configuration $configuration -SkipBuild:$SkipBuild -RunIde -Check
}
if (!$SkipPortable) { & "$PSScriptRoot/test_portable_windows.ps1" }
if ($Package) {
    if (!$CppRuntime) { throw 'Package verification requires -CppRuntime pointing to a staged compiler directory' }
    & "$PSScriptRoot/stage_cpp_toolchain.ps1" -Destination $CppRuntime -VerifyOnly -OffscreenOnly
    & "$PSScriptRoot/test_cpp_package.ps1" -Runtime $CppRuntime -Python $Python
}
Write-Output 'OK: HiAsm Qt checks'
