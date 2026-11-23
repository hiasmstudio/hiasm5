[CmdletBinding()]
param([string]$WinePrefix = 'build/wine-hiasm4', [switch]$InstallRuntime)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if(-not (Get-Command wine -ErrorAction SilentlyContinue)) {
    throw 'Wine is not installed. Install wine and winetricks with the Linux package manager, then rerun this script.'
}
$prefix = if([System.IO.Path]::IsPathRooted($WinePrefix)) { $WinePrefix } else { Join-Path $root $WinePrefix }
$env:WINEPREFIX = [System.IO.Path]::GetFullPath($prefix)
New-Item -ItemType Directory -Force -Path $env:WINEPREFIX | Out-Null
if($InstallRuntime) {
    if(-not (Get-Command winetricks -ErrorAction SilentlyContinue)) { throw 'winetricks is required for -InstallRuntime' }
    & winetricks -q corefonts vcrun2019
    if($LASTEXITCODE -ne 0) { throw "winetricks failed with exit code $LASTEXITCODE" }
}
& wineboot -u
if($LASTEXITCODE -ne 0) { throw "wineboot failed with exit code $LASTEXITCODE" }
Write-Output "Wine prefix ready: $env:WINEPREFIX"
Write-Output 'HiAsm 4 DLLs remain optional legacy assets; the GTK4 host does not load 32-bit Delphi packages.'
