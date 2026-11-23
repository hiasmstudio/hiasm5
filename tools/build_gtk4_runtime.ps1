[CmdletBinding()]
param([string]$Msys2Root = 'C:/msys64', [int]$Jobs = 6,
      [string]$SourceDir = '', [string]$BuildDir = '')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$work = Join-Path $root 'build/deps/gtk4-win32'
$version = '4.24.1'
$hash = 'E98ABE720E16129C8C0F50761DEE0A2E9AE2478055E31018F6CF977FCF6513E9'
$prefix = Join-Path $Msys2Root 'ucrt64'
$python = Join-Path $prefix 'bin/python.exe'
$ninja = Join-Path $prefix 'bin/ninja.exe'
$tar = Join-Path $Msys2Root 'usr/bin/tar.exe'
$patch = Join-Path $Msys2Root 'usr/bin/patch.exe'
foreach($file in @($python,$ninja,$tar,$patch)) {
    if(-not (Test-Path -LiteralPath $file)) { throw "Required build tool missing: $file" }
}
New-Item -ItemType Directory -Path $work -Force | Out-Null
$archive = Join-Path $work "gtk-$version.tar.xz"
if(-not (Test-Path -LiteralPath $archive)) {
    Invoke-WebRequest "https://download.gnome.org/sources/gtk/4.24/gtk-$version.tar.xz" -OutFile $archive
}
if((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash -ne $hash) { throw 'GTK source checksum mismatch' }
$source = if($SourceDir) { [IO.Path]::GetFullPath($SourceDir) } else { Join-Path $work "gtk-$version" }
if(-not (Test-Path -LiteralPath $source)) {
    if($SourceDir) { throw 'SourceDir must already contain GTK sources' }
    # Python handles xz without requiring an MSYS xz.exe in the caller PATH.
    # No inner double quotes: Windows PowerShell 5.1 strips them from native arguments.
    & $python -c "import sys,tarfile; tarfile.open(sys.argv[1]).extractall(sys.argv[2],filter='data')" $archive $work
    if($LASTEXITCODE -ne 0) { throw 'GTK extraction failed' }
}
$file = Join-Path $source 'gdk/win32/gdkdrag-win32.c'
$text = [IO.File]::ReadAllText($file)
$old = 'if (!dnd_queue_is_empty (display))'
$new = 'if (display != NULL && !dnd_queue_is_empty (display))'
if($text.Contains($old)) {
    [IO.File]::WriteAllText($file,$text.Replace($old,$new),[Text.UTF8Encoding]::new($false))
} elseif(-not $text.Contains($new)) { throw 'GTK source did not match the expected patch target' }
$oldPath = $env:PATH
$oldPython = $env:PYTHONPATH
try {
    $env:PATH = "$prefix/bin;$Msys2Root/usr/bin;$oldPath"
    $imePatch = Join-Path $PSScriptRoot 'patches/gtk4-win32-ime-lifecycle.patch'
    & $patch --directory=$source -p1 --batch --forward --dry-run --input=$imePatch *> $null
    if($LASTEXITCODE -eq 0) {
        & $patch --directory=$source -p1 --batch --forward --input=$imePatch
        if($LASTEXITCODE) { throw 'GTK IME patch failed' }
    } else {
        & $patch --directory=$source -p1 --batch --reverse --dry-run --input=$imePatch *> $null
        if($LASTEXITCODE) { throw 'GTK IME source does not match the patch or its applied form' }
    }
    foreach($tool in @('cargo','rustc','rustup','git')) {
        if(-not (Get-Command $tool -ErrorAction SilentlyContinue)) { throw "Required build tool missing: $tool" }
    }
    $targets = & rustup target list --installed
    if($LASTEXITCODE -ne 0 -or $targets -notcontains 'x86_64-pc-windows-gnu') {
        throw 'Install the Rust target first: rustup target add x86_64-pc-windows-gnu'
    }
    # Meson is pure Python; MSYS' package can also be used by native UCRT Python.
    $meson = Get-ChildItem (Join-Path $Msys2Root 'usr/lib') -Directory -Filter 'python*' |
        Where-Object { Test-Path (Join-Path $_.FullName 'site-packages/mesonbuild/mesonmain.py') } |
        Select-Object -First 1
    if(-not $meson) { throw 'Install the MSYS2 meson package' }
    $env:PYTHONPATH = Join-Path $meson.FullName 'site-packages'
    $build = if($BuildDir) { [IO.Path]::GetFullPath($BuildDir) } else { Join-Path $work 'build' }
    $options = @('setup',$build,$source,"--prefix=$prefix",'--buildtype=release',
        '-Dmedia-gstreamer=disabled','-Dvulkan=disabled','-Dintrospection=disabled','-Daccesskit=enabled',
        '-Ddocumentation=false','-Dbuild-tests=false','-Dbuild-examples=false',
        '-Dbuild-demos=false','-Dbuild-testsuite=false')
    if(Test-Path (Join-Path $build 'build.ninja')) { $options += '--reconfigure' }
    & $python -m mesonbuild.mesonmain @options
    if($LASTEXITCODE -ne 0) { throw 'GTK configuration failed' }
    & $ninja -C $build gtk/libgtk-4-1.dll -j $Jobs
    if($LASTEXITCODE -ne 0) { throw 'GTK build failed' }
    Copy-Item -LiteralPath (Join-Path $build 'gtk/libgtk-4-1.dll') -Destination (Join-Path $work 'libgtk-4-1.dll') -Force
    Copy-Item -LiteralPath (Join-Path $source 'COPYING') -Destination (Join-Path $work 'COPYING') -Force
    $accesskitDll = Get-ChildItem -LiteralPath $build -Recurse -File -Filter 'libaccesskit-c-0.18.dll' | Select-Object -First 1
    if(-not $accesskitDll) { throw 'AccessKit DLL was not built; the runtime would be incomplete' }
    Copy-Item -LiteralPath $accesskitDll.FullName -Destination (Join-Path $work $accesskitDll.Name) -Force
    $accesskitSource = Join-Path $source 'subprojects/accesskit-c'
    $accesskitNotices = Join-Path $work 'accesskit'
    New-Item -ItemType Directory -Path $accesskitNotices -Force | Out-Null
    foreach($notice in @('LICENSE-APACHE','LICENSE-MIT','LICENSE.chromium','COPYING.LIB','Cargo.lock')) {
        Copy-Item -LiteralPath (Join-Path $accesskitSource $notice) -Destination $accesskitNotices -Force
    }
    & git -C $accesskitSource archive --format=tar.gz "--output=$(Join-Path $accesskitNotices 'accesskit-c-0.18.0.tar.gz')" HEAD
    if($LASTEXITCODE -ne 0) { throw 'AccessKit source archive failed' }
    @('GTK 4.24.1: https://download.gnome.org/sources/gtk/4.24/gtk-4.24.1.tar.xz',
      "GTK source SHA256: $hash",'AccessKit C 0.18.0: https://github.com/AccessKit/accesskit-c/tree/0.18.0',
      'AccessKit and its locked Rust dependencies are built by the GTK Meson subproject.',
      'Local patches: tools/patches/gtk4-win32-dnd-startup.patch, tools/patches/gtk4-win32-ime-lifecycle.patch') |
        Set-Content -LiteralPath (Join-Path $work 'SOURCES.txt') -Encoding UTF8
    Write-Output "GTK runtime ready: $work/libgtk-4-1.dll"
} finally { $env:PATH=$oldPath; $env:PYTHONPATH=$oldPython }
