[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$Toolchain)
$ErrorActionPreference='Stop'
$destination=(Resolve-Path -LiteralPath $Toolchain).Path
$local=Join-Path $PSScriptRoot '../build/deps/gtk4-win32'
$manifestPath=Join-Path $destination 'manifest.json'
if(-not (Test-Path -LiteralPath $manifestPath) -or
   -not (Test-Path -LiteralPath (Join-Path $destination 'bin/g++.exe'))) {
    throw 'Toolchain must be a staged HiAsm GCC/GTK4 prefix with manifest.json'
}
$manifest=Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
if($manifest.environment -ne 'ucrt64') { throw 'Local GTK4 runtime requires the UCRT64 toolchain' }
$notice=Join-Path $destination 'licenses/local-gtk4'
$null=New-Item -ItemType Directory -Force -Path $notice
$binaries=@()
foreach($name in @('libgtk-4-1.dll','libaccesskit-c-0.18.dll')) {
    $source=Join-Path $local $name
    if(-not (Test-Path -LiteralPath $source)) { throw "Build the local GTK4 runtime first: $source" }
    Copy-Item -LiteralPath $source -Destination (Join-Path $destination "bin/$name") -Force
    $binaries+= [ordered]@{file="bin/$name";sha256=(Get-FileHash -LiteralPath $source).Hash.ToLowerInvariant()}
}
foreach($name in @('COPYING','SOURCES.txt','gtk-4.24.1.tar.xz')) {
    Copy-Item -LiteralPath (Join-Path $local $name) -Destination (Join-Path $notice $name) -Force
}
$null=New-Item -ItemType Directory -Force -Path (Join-Path $notice 'accesskit')
Copy-Item -Path (Join-Path $local 'accesskit/*') -Destination (Join-Path $notice 'accesskit') -Force
foreach($name in @('gtk4-win32-dnd-startup.patch','gtk4-win32-ime-lifecycle.patch')) {
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot "patches/$name") -Destination (Join-Path $notice $name) -Force
}
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'build_gtk4_runtime.ps1') -Destination $notice -Force
$manifest | Add-Member -NotePropertyName localRuntime -NotePropertyValue ([ordered]@{
    version='GTK 4.24.1 + HiAsm Win32 lifecycle fixes';binaries=$binaries;notices='licenses/local-gtk4'
}) -Force
[IO.File]::WriteAllText($manifestPath,($manifest | ConvertTo-Json -Depth 12),[Text.UTF8Encoding]::new($false))
Write-Output "Updated C++ toolchain GTK4 runtime and source notices: $destination"
