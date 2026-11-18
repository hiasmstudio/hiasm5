[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Destination,
    [string]$Msys2Root='C:\msys64',
    [switch]$SkipVerify,
    [switch]$VerifyOnly
)
$ErrorActionPreference='Stop'
$msys=(Resolve-Path -LiteralPath $Msys2Root).Path
$prefix=Join-Path $msys 'ucrt64'
$destinationPath=[IO.Path]::GetFullPath($Destination)
if($destinationPath.StartsWith($msys,[StringComparison]::OrdinalIgnoreCase)) { throw 'Destination must be outside the MSYS2 installation' }
New-Item -ItemType Directory -Force -Path $destinationPath | Out-Null
if(-not $VerifyOnly) {
if(Get-ChildItem -LiteralPath $destinationPath -Force | Select-Object -First 1) { throw 'Destination must be empty; use a new directory for each toolchain version' }

# Use the installed package database, including versioned dependencies and
# virtual providers, rather than guessing GCC internal directories or DLLs.
$packages=@{}; $providers=@{}
foreach($directory in Get-ChildItem -LiteralPath (Join-Path $msys 'var/lib/pacman/local') -Directory) {
    $desc=Join-Path $directory.FullName 'desc'
    if(-not (Test-Path -LiteralPath $desc)) { continue }
    $fields=@{}; $section=''
    foreach($line in [IO.File]::ReadAllLines($desc)) {
        if($line -match '^%(.+)%$') { $section=$matches[1]; $fields[$section]=@() }
        elseif($line -and $section) { $fields[$section]+= $line }
    }
    $name=$fields['NAME'][0]
    $entry=@{ Name=$name; Base=$fields['BASE'][0]; Version=$fields['VERSION'][0]; Depends=$fields['DEPENDS']; Directory=$directory.FullName; Url=$fields['URL']; License=$fields['LICENSE'] }
    $packages[$name]=$entry; $providers[$name]=$name
    foreach($provided in $fields['PROVIDES']) { $providers[($provided -split '[<>=]')[0]]=$name }
}
$queue=New-Object 'System.Collections.Generic.Queue[string]'
# SVG is an optional GdkPixbuf dependency but GTK's icon theme requires it.
foreach($seed in @('gcc','pkgconf','gtk4','librsvg')) { $queue.Enqueue('mingw-w64-ucrt-x86_64-'+$seed) }
$selected=@{}
while($queue.Count) {
    $dependency=($queue.Dequeue() -split '[<>=]')[0]
    if(-not $providers.ContainsKey($dependency)) { throw "Installed dependency not found: $dependency" }
    $name=$providers[$dependency]
    if($selected.ContainsKey($name)) { continue }
    $entry=$packages[$name]; $selected[$name]=$entry
    foreach($dependency in $entry.Depends) { $queue.Enqueue($dependency) }
}
$inventory=New-Object 'System.Collections.Generic.List[object]'
$fileCount=0
foreach($entry in $selected.Values | Sort-Object Name) {
    $inFiles=$false
    foreach($relative in [IO.File]::ReadAllLines((Join-Path $entry.Directory 'files'))) {
        if($relative -eq '%FILES%') { $inFiles=$true; continue }
        if($relative -match '^%') { $inFiles=$false }
        if(-not $inFiles -or -not $relative.StartsWith('ucrt64/') -or $relative.EndsWith('/')) { continue }
        $source=Join-Path $msys $relative
        if(-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Package file missing: $source" }
        $target=Join-Path $destinationPath $relative.Substring(7)
        New-Item -ItemType Directory -Force -Path (Split-Path -Parent $target) | Out-Null
        Copy-Item -LiteralPath $source -Destination $target -Force
        ++$fileCount
    }
    $inventory.Add([ordered]@{name=$entry.Name; version=$entry.Version; upstream=$entry.Url; license=$entry.License; sourcePackage=('https://packages.msys2.org/base/'+$entry.Base)})
}
$manifest=[ordered]@{format=1; architecture='x86_64-w64-mingw32'; environment='ucrt64'; files=$fileCount; packages=$inventory}
$manifest | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $destinationPath 'manifest.json') -Encoding UTF8
Write-Output "Staged $($selected.Count) packages, $fileCount files: $destinationPath"
& (Join-Path $PSScriptRoot 'update_cpp_runtime.ps1') -Toolchain $destinationPath
} elseif(-not (Test-Path -LiteralPath (Join-Path $destinationPath 'manifest.json'))) { throw 'Existing toolchain manifest not found' }
if(-not $SkipVerify) {
    $oldPath=$env:PATH; $oldPkg=$env:PKG_CONFIG_PATH; $oldLib=$env:PKG_CONFIG_LIBDIR
    $verify=Join-Path $destinationPath ('verify-'+[Guid]::NewGuid().ToString('N'))
    New-Item -ItemType Directory -Path $verify | Out-Null
    Push-Location -LiteralPath $verify
    try {
        $env:PATH=(Join-Path $destinationPath 'bin')+';'+$env:SystemRoot+'\System32;'+$env:SystemRoot
        $env:PKG_CONFIG_PATH=''; $env:PKG_CONFIG_LIBDIR=(Join-Path $destinationPath 'lib/pkgconfig')+';'+(Join-Path $destinationPath 'share/pkgconfig')
        $compiler=Join-Path $destinationPath 'bin/g++.exe'
        [IO.File]::WriteAllText((Join-Path $verify 'console.cpp'),'#include <iostream>'+[Environment]::NewLine+'int main(){std::cout << "bundled compiler OK";}')
        & $compiler ('--sysroot='+$destinationPath) '-std=c++17' 'console.cpp' '-o' 'console.exe'
        if($LASTEXITCODE -ne 0) { throw 'Bundled GCC console compilation failed' }
        $result=& (Join-Path $verify 'console.exe')
        if($LASTEXITCODE -ne 0 -or $result -ne 'bundled compiler OK') { throw 'Bundled console execution failed' }
        $flags=& (Join-Path $destinationPath 'bin/pkg-config.exe') --cflags --libs gtk4
        if($LASTEXITCODE -ne 0) { throw 'Bundled GTK4 development files could not be resolved' }
        if(($flags -join ' ') -match [regex]::Escape($prefix)) { throw 'pkg-config leaked the original MSYS2 prefix' }
        # GCC response files preserve the quoting of paths with spaces from pkgconf.
        [IO.File]::WriteAllText((Join-Path $verify 'gtk.flags'),($flags -join [Environment]::NewLine))
        [IO.File]::WriteAllText((Join-Path $verify 'gui.cpp'),'#include <gtk/gtk.h>'+[Environment]::NewLine+'int main(){return gtk_get_major_version()!=4;}')
        & $compiler ('--sysroot='+$destinationPath) '-std=c++17' 'gui.cpp' '@gtk.flags' '-o' 'gui.exe'
        if($LASTEXITCODE -ne 0) { throw 'Bundled GCC GTK4 compilation failed' }
        & (Join-Path $verify 'gui.exe')
        if($LASTEXITCODE -ne 0) { throw 'Bundled GTK4 runtime failed' }
        Write-Output 'OK: bundled C++17 console and GTK4 compile/run with isolated PATH'
    } finally {
        Pop-Location
        $env:PATH=$oldPath; $env:PKG_CONFIG_PATH=$oldPkg; $env:PKG_CONFIG_LIBDIR=$oldLib
        # Only files produced in this unique verification directory are removed.
        foreach($file in @('console.cpp','console.exe','gui.cpp','gui.exe','gtk.flags')) { Remove-Item -LiteralPath (Join-Path $verify $file) -ErrorAction SilentlyContinue }
        Remove-Item -LiteralPath $verify -ErrorAction SilentlyContinue
    }
}
