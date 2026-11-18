# Make a HiAsm installation self-contained: copy the MSYS2 UCRT64 DLL closure
# of hiasm.exe plus the GTK/GLib/GdkPixbuf/GtkSourceView data directories that
# the runtime resolves relative to the installation prefix.
param(
    [Parameter(Mandatory = $true)][string]$InstallDir,
    [string]$Msys2Root = 'C:\msys64',
    [string]$Msys2Environment = 'ucrt64',
    [ValidateSet('3', '4')][string]$GtkVersion = '3',
    [string]$RuntimeSource = '',
    [switch]$SkipLocale
)
$ErrorActionPreference = 'Stop'

$install = (Resolve-Path -LiteralPath $InstallDir).Path.TrimEnd('\')
$prefix = Join-Path $Msys2Root $Msys2Environment
$bin = Join-Path $prefix 'bin'
$exe = Join-Path $install 'hiasm.exe'
$objdump = Join-Path $bin 'objdump.exe'
$system32 = Join-Path $env:WINDIR 'System32'

foreach ($required in @($exe, $objdump)) {
    if (-not (Test-Path -LiteralPath $required -PathType Leaf)) {
        throw "Required file not found: $required"
    }
}

if (-not (Test-Path -LiteralPath $system32 -PathType Container)) {
    throw "Windows system directory not found: $system32"
}

function Get-DllImports([string]$Path) {
    $output = & $objdump -p $Path
    if ($LASTEXITCODE -ne 0) {
        throw "objdump failed for $Path"
    }
    foreach ($line in $output) {
        if ($line -match 'DLL Name:\s*(.+?)\s*$') {
            $matches[1]
        }
    }
}

function Copy-FileWithParents([string]$Source, [string]$Destination) {
    if([IO.Path]::GetFullPath($Source) -eq [IO.Path]::GetFullPath($Destination)) { return }
    $parent = Split-Path -Path $Destination -Parent
    if ($parent -and -not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    Copy-Item -LiteralPath $Source -Destination $Destination -Force
}

function Copy-DirectoryContents([string]$Source, [string]$Destination) {
    if([IO.Path]::GetFullPath($Source) -eq [IO.Path]::GetFullPath($Destination)) { return }
    # The CMake install step already creates int/ and packs/. Copy the staged
    # tree contents into those directories instead of copying the directory
    # itself, which would create an accidental int/int or packs/packs level.
    if (-not (Test-Path -LiteralPath $Destination -PathType Container)) {
        New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    }
    Get-ChildItem -LiteralPath $Source -Force | ForEach-Object {
        $target = Join-Path $Destination $_.Name
        if($_.PSIsContainer) { Copy-DirectoryContents $_.FullName $target }
        else { Copy-FileWithParents $_.FullName $target }
    }
}

# The executable is installed separately from the staged HiAsm data tree.
# Copy that tree into the same prefix so a portable install does not depend on
# the source checkout or on MSYS2's working directory.
if ([string]::IsNullOrWhiteSpace($RuntimeSource)) {
    $candidate = Join-Path (Split-Path -Parent $install) 'Windows_MinGW-x64-GTK4-Release'
    if ($GtkVersion -eq '3') {
        $candidate = Join-Path (Split-Path -Parent $install) 'Windows_MinGW-x64-Release'
    }
    if (Test-Path -LiteralPath (Join-Path $candidate 'int') -PathType Container) {
        $RuntimeSource = $candidate
    }
}
if (-not [string]::IsNullOrWhiteSpace($RuntimeSource)) {
    $runtime = (Resolve-Path -LiteralPath $RuntimeSource).Path.TrimEnd('\')
    foreach ($relative in @('int', 'packs', 'runner.exe', 'splash.png', 'hiasm.css', 'gtkrc', 'LICENSE')) {
        $source = Join-Path $runtime $relative
        if (-not (Test-Path -LiteralPath $source)) { continue }
        $destination = Join-Path $install $relative
        if ((Get-Item -LiteralPath $source).PSIsContainer) {
            Copy-DirectoryContents $source $destination
        } else {
            Copy-FileWithParents $source $destination
        }
    }
}

# The SVG loader is required by the Adwaita icon theme (GTK aborts if it cannot
# load image-missing.svg), and it has its own dependency tree. Seed the closure
# with the GdkPixbuf loaders as well as the executable.
$queue = New-Object 'System.Collections.Generic.Queue[string]'
$queue.Enqueue($exe)
# GLib uses these executables for synchronous/async spawn on Windows. They
# are not visible in DLL import tables, yet build/SVN/form helpers need them.
foreach($helper in @('gspawn-win64-helper.exe','gspawn-win64-helper-console.exe')) {
    $source = Join-Path $bin $helper
    if(-not(Test-Path -LiteralPath $source -PathType Leaf)) { throw "GLib process helper missing: $source" }
    $destination = Join-Path $install $helper
    Copy-FileWithParents $source $destination
    $queue.Enqueue($destination)
}
$runner = Join-Path $install 'runner.exe'
if(Test-Path -LiteralPath $runner) { $queue.Enqueue($runner) }
foreach($module in (Get-ChildItem (Join-Path $install 'packs') -Filter '*.dll' -File -Recurse -ErrorAction SilentlyContinue)) {
    $queue.Enqueue($module.FullName)
}
$gdkSource = Join-Path $prefix 'lib\gdk-pixbuf-2.0'
$loaderSource = Join-Path $gdkSource '2.10.0\loaders'
if (Test-Path -LiteralPath $loaderSource) {
    foreach ($loader in (Get-ChildItem -LiteralPath $loaderSource -File -Filter '*.dll')) {
        $loaderDestination = Join-Path (Join-Path $install 'lib\gdk-pixbuf-2.0\2.10.0\loaders') $loader.Name
        Copy-FileWithParents $loader.FullName $loaderDestination
        $queue.Enqueue($loaderDestination)
    }
}

# Walk the import table recursively; any DLL that Windows does not provide comes
# from the UCRT64 prefix (GTK, GLib, Cairo, libstdc++, libwinpthread, ...).
$seen = New-Object 'System.Collections.Generic.HashSet[string]' ([StringComparer]::OrdinalIgnoreCase)
$deployed = New-Object 'System.Collections.Generic.List[string]'
while ($queue.Count -gt 0) {
    $module = $queue.Dequeue()
    foreach ($name in (Get-DllImports $module)) {
        if (-not $seen.Add($name)) { continue }
        if ($name -like 'api-ms-win-*' -or $name -like 'ext-ms-*') {
            continue
        }
        if (Test-Path -LiteralPath (Join-Path $system32 $name) -PathType Leaf) {
            continue
        }
        $source = Join-Path $bin $name
        if($GtkVersion -eq '4' -and $name -in @('libgtk-4-1.dll','libaccesskit-c-0.18.dll')) {
            $patched = Join-Path $PSScriptRoot "../build/deps/gtk4-win32/$name"
            if(Test-Path -LiteralPath $patched) { $source = (Resolve-Path -LiteralPath $patched).Path }
        }
        if (-not (Test-Path -LiteralPath $source -PathType Leaf)) {
            throw "Dependency not found in ${bin}: $name (imported by $(Split-Path $module -Leaf))"
        }
        $destination = Join-Path $install $name
        if([IO.Path]::GetFullPath($source) -ne [IO.Path]::GetFullPath($destination)) {
            Copy-FileWithParents $source $destination
        }
        $deployed.Add($name)
        $queue.Enqueue($destination)
    }
}

function Copy-Tree([string]$RelativePath) {
    $source = Join-Path $prefix $RelativePath
    if (-not (Test-Path -LiteralPath $source)) { return }
    $destination = Join-Path $install $RelativePath
    $parent = Split-Path -Path $destination -Parent
    if ($parent -and -not (Test-Path -LiteralPath $parent)) {
        New-Item -ItemType Directory -Path $parent -Force | Out-Null
    }
    Copy-DirectoryContents $source $destination
}

# GTK resolves these through the installation prefix (the directory containing
# the DLLs), so keeping the MSYS2 layout below makes the copy relocatable.
Copy-Tree 'share\glib-2.0\schemas'
Copy-Tree 'share\icons'
if ($GtkVersion -eq '4') {
    Copy-Tree 'share\gtk-4.0'
    Copy-Tree 'share\gtksourceview-5'
} else {
    Copy-Tree 'share\gtksourceview-3.0'
}
Copy-Tree 'share\themes'
Copy-Tree 'share\fontconfig'
Copy-Tree 'etc\fonts'
if (-not $SkipLocale) {
    Copy-Tree 'share\locale'
}

# GdkPixbuf's loaders.cache already stores paths relative to the prefix
# ("lib\gdk-pixbuf-2.0\..."); the loader DLLs were copied before the closure.
if (Test-Path -LiteralPath $gdkSource) {
    Get-ChildItem -LiteralPath $gdkSource -Recurse -File -Filter '*.cache' |
        ForEach-Object {
            $relative = $_.FullName.Substring($gdkSource.Length).TrimStart('\')
            Copy-FileWithParents $_.FullName (Join-Path (Join-Path $install 'lib\gdk-pixbuf-2.0') $relative)
        }
}

Write-Host "Deployed $($deployed.Count) runtime DLLs and GTK data to $install"
if($GtkVersion -eq '4') {
    $localGtk = Join-Path $PSScriptRoot '../build/deps/gtk4-win32'
    if(Test-Path -LiteralPath (Join-Path $localGtk 'libgtk-4-1.dll')) {
        foreach($file in @('COPYING','gtk-4.24.1.tar.xz','SOURCES.txt')) {
            Copy-FileWithParents (Join-Path $localGtk $file) (Join-Path $install "licenses/gtk4/$file")
        }
        Copy-FileWithParents (Join-Path $PSScriptRoot 'patches/gtk4-win32-dnd-startup.patch') (Join-Path $install 'licenses/gtk4/gtk4-win32-dnd-startup.patch')
        Copy-FileWithParents (Join-Path $PSScriptRoot 'patches/gtk4-win32-ime-lifecycle.patch') (Join-Path $install 'licenses/gtk4/gtk4-win32-ime-lifecycle.patch')
        Copy-FileWithParents (Join-Path $PSScriptRoot 'build_gtk4_runtime.ps1') (Join-Path $install 'licenses/gtk4/build_gtk4_runtime.ps1')
        Copy-DirectoryContents (Join-Path $localGtk 'accesskit') (Join-Path $install 'licenses/accesskit')
    }
}
