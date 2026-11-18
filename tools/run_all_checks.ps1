[CmdletBinding()]
param([switch]$SkipBuild, [switch]$SkipPortable, [switch]$Cross,
      [string]$WslDistribution = 'Debian', [switch]$Package,
      [string]$CppRuntime = '', [string]$Python = 'python')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
Push-Location $root
try {
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/validate_databases.ps1
    if ($LASTEXITCODE -ne 0) { throw 'Database validation failed' }
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/check_gtk4_contract.ps1
    if ($LASTEXITCODE -ne 0) { throw 'Source lint failed (this is not a UI behavior test)' }
    if (-not $SkipBuild) {
        cmd /d /c tools\build_windows.bat gtk4 debug
        if ($LASTEXITCODE -ne 0) { throw 'GTK4 build failed' }
        cmd /d /c tools\build_windows.bat gtk3 debug
        if ($LASTEXITCODE -ne 0) { throw 'GTK3 build failed' }
    }
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/smoke_windows.ps1 -GtkVersion gtk4 -WaitSeconds 4
    if ($LASTEXITCODE -ne 0) { throw 'GTK4 startup smoke failed' }
    powershell -NoProfile -ExecutionPolicy Bypass -File tools/smoke_windows.ps1 -GtkVersion gtk3 -WaitSeconds 4
    if ($LASTEXITCODE -ne 0) { throw 'GTK3 startup smoke failed' }
    $oldPath = $env:PATH
    $oldCross=$env:HIASM_TEST_CROSS; $oldStop=$env:HIASM_TEST_STOP_CROSS; $oldDistro=$env:HIASM_WSL_DISTRO
    try {
        $msys = if($env:MSYS2_ROOT) { $env:MSYS2_ROOT } else { 'C:/msys64' }
        $env:PATH = "$msys/ucrt64/bin;$msys/usr/bin;$oldPath"
        if($Cross) { $env:HIASM_TEST_CROSS='1'; $env:HIASM_TEST_STOP_CROSS='1'; $env:HIASM_WSL_DISTRO=$WslDistribution }
        foreach($build in @('Windows_MinGW-x64-GTK4-Debug','Windows_MinGW-x64-Debug')) {
            ctest --test-dir (Join-Path $root "build/$build") --output-on-failure
            if($LASTEXITCODE -ne 0) { throw "Regression tests failed: $build" }
        }
    } finally { $env:PATH = $oldPath; $env:HIASM_TEST_CROSS=$oldCross; $env:HIASM_TEST_STOP_CROSS=$oldStop; $env:HIASM_WSL_DISTRO=$oldDistro }
    if(-not $SkipPortable) {
        $oldPath = $env:PATH
        try {
            $env:PATH = "$msys/ucrt64/bin;$msys/usr/bin;$oldPath"
            & powershell -NoProfile -ExecutionPolicy Bypass -File tools/test_portable_windows.ps1
            if($LASTEXITCODE -ne 0) { throw 'GTK4 portable verification failed' }
        } finally { $env:PATH = $oldPath }
    }
    if($Package) {
        & powershell -NoProfile -ExecutionPolicy Bypass -File tools/test_cpp_package.ps1 -Runtime $CppRuntime -Python $Python
        if($LASTEXITCODE -ne 0) { throw 'Standalone C++ GTK4 packaging verification failed' }
    }
    foreach ($item in @('elements.sql','elements.db','conf','icon','new','example','compiler')) {
        if (-not (Test-Path -LiteralPath (Join-Path $root "packs/cpp/$item"))) { throw "packs/cpp/$item is missing" }
    }
    Write-Output 'OK all HiAsm checks'
} finally { Pop-Location }
