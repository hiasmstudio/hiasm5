[CmdletBinding()]
param(
    [ValidateSet('gtk3', 'gtk4')][string]$GtkVersion = 'gtk4',
    [int]$WaitSeconds = 4,
    [string]$InstallDir = '',
    [switch]$CleanEnvironment
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = if ($InstallDir) {
    (Resolve-Path -LiteralPath $InstallDir).Path
} elseif ($GtkVersion -eq 'gtk4') {
    Join-Path $root 'build/Windows_MinGW-x64-GTK4-Debug'
} else {
    Join-Path $root 'build/Windows_MinGW-x64-Debug'
}
$exe = Join-Path $build 'hiasm.exe'
if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) {
    throw "Executable not found: $exe. Build it with tools\build_windows.bat $GtkVersion debug"
}

$msys = if ($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { 'C:\msys64' }
$ucrt = Join-Path $msys 'ucrt64/bin'
$usr = Join-Path $msys 'usr/bin'
$oldPath = $env:PATH
$savedEnvironment = @{}
if($CleanEnvironment) {
    # An installed build must resolve DLLs and GTK data from its own prefix.
    $env:PATH = "$env:WINDIR/System32;$env:WINDIR"
    foreach($key in @('GTK_PATH','GTK_THEME','GTK_DATA_PREFIX','GTK_EXE_PREFIX',
        'GDK_PIXBUF_MODULE_FILE','GDK_PIXBUF_MODULEDIR','GSETTINGS_SCHEMA_DIR',
        'XDG_DATA_DIRS','GI_TYPELIB_PATH','GTK_A11Y','GSK_RENDERER')) {
        $savedEnvironment[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
        [Environment]::SetEnvironmentVariable($key, $null, 'Process')
    }
} else { $env:PATH = "$ucrt;$usr;$oldPath" }
$out = Join-Path $build 'smoke.out'
$err = Join-Path $build 'smoke.err'
$proc = $null
try {
    # Deliberately launch from outside the install prefix as Explorer can do.
    $proc = Start-Process -FilePath $exe -WorkingDirectory $env:TEMP -RedirectStandardOutput $out -RedirectStandardError $err -PassThru
    $processHandle = $proc.Handle
    Start-Sleep -Seconds $WaitSeconds
    if ($proc.HasExited) {
        $proc.WaitForExit()
        $code = $proc.ExitCode
        $details = Get-Content $out,$err -ErrorAction SilentlyContinue | Out-String
        if ([string]::IsNullOrWhiteSpace($details)) {
            $details = "No stdout/stderr was produced; another HiAsm instance or a Windows termination may have closed the process."
        }
        throw "HiAsm exited during startup with code $code.`n$details"
    }
    $log = Get-Content $out,$err -ErrorAction SilentlyContinue | Out-String
    if ($log -match 'Unable to load pack|groups\.name contains NULL|missing required table|Gtk-ERROR|Gtk-CRITICAL') {
        throw "HiAsm reported a startup data error:`n$log"
    }
    if($CleanEnvironment) {
        $external = @($proc.Modules | Where-Object { $_.FileName.StartsWith($msys, [StringComparison]::OrdinalIgnoreCase) })
        if($external.Count) { throw "Loaded DLLs from MSYS2: $($external.FileName -join ', ')" }
        Write-Output 'OK isolated PATH: no modules loaded from MSYS2'
    }
    Write-Output "OK $GtkVersion startup: process $($proc.Id) remained alive for $WaitSeconds seconds"
    if ($log.Trim()) { Write-Output $log.Trim() }
} finally {
    if ($proc -and -not $proc.HasExited) { $proc.Kill(); $proc.WaitForExit() }
    foreach($key in $savedEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($key, $savedEnvironment[$key], 'Process')
    }
    $env:PATH = $oldPath
}
