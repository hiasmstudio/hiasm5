[CmdletBinding()]
param([string]$Runtime = '', [string]$Python = 'python', [string]$Output = '')
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if(-not $Runtime) { throw 'Specify -Runtime pointing to the staged packs/cpp/compiler directory' }
$Runtime = (Resolve-Path -LiteralPath $Runtime).Path
if(-not $Output) { $Output = Join-Path $root ('build/qa/standalone-' + [guid]::NewGuid().ToString('N')) }
$Output = [IO.Path]::GetFullPath($Output)
if(Test-Path -LiteralPath $Output) { throw 'Output must not exist' }
$fixture = Join-Path $root ('build/qa/package-source-' + [guid]::NewGuid().ToString('N'))
$null = New-Item -ItemType Directory -Path $fixture
if (Test-Path -LiteralPath (Join-Path $Runtime 'qt6/bin/windeployqt.exe')) {
    $saved = @{}
    foreach ($key in 'PATH','QT_QPA_PLATFORM','QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH') {
        $saved[$key] = [Environment]::GetEnvironmentVariable($key)
    }
    try {
        $pythonTool = (Get-Command $Python -ErrorAction Stop).Source
        $qt = Join-Path $Runtime 'qt6'
        $llvm = Join-Path $Runtime 'llvm-mingw/bin'
        $bin = Join-Path $Runtime 'tools/bin'
        $env:PATH = "$qt/bin;$llvm;$bin;$env:SystemRoot/System32;$env:SystemRoot"
        $options = @("-DCMAKE_PREFIX_PATH=$qt", "-DCMAKE_CXX_COMPILER=$llvm/clang++.exe",
            "-DCMAKE_MAKE_PROGRAM=$bin/ninja.exe", "-DHIASM_SOURCE=$root/tests/cpp_qt_toolchain_probe.cpp",
            '-DHIASM_TOOLCHAIN_PROBE=ON')
        & "$bin/cmake.exe" -S "$root/packs/cpp/qt" -B $fixture -G Ninja @options
        if ($LASTEXITCODE) { throw 'Qt program configure failed' }
        & "$bin/cmake.exe" --build $fixture
        if ($LASTEXITCODE) { throw 'Qt program compilation failed' }
        & $pythonTool "$PSScriptRoot/package_cpp_windows.py" --executable "$fixture/program.exe" --runtime $Runtime --output $Output
        if ($LASTEXITCODE) { throw 'Packaging failed' }
        $manifest = Get-Content "$Output/distribution.json" -Raw -Encoding UTF8 | ConvertFrom-Json
        foreach ($file in $manifest.files) {
            if ((Get-FileHash -LiteralPath (Join-Path $Output $file.path)).Hash -ne $file.sha256) {
                throw "Hash mismatch: $($file.path)"
            }
        }
        $env:PATH = "$env:SystemRoot/System32;$env:SystemRoot"
        $env:QT_QPA_PLATFORM = 'offscreen'
        $env:QT_PLUGIN_PATH = $null
        $env:QT_QPA_PLATFORM_PLUGIN_PATH = $null
        & "$Output/program.exe" | Out-Host
        if ($LASTEXITCODE -ne 0) { throw "Packaged program failed ($LASTEXITCODE)" }
        Write-Output "OK: standalone Qt Widgets program with a system-only PATH: $Output"
    } finally {
        foreach ($entry in $saved.GetEnumerator()) {
            [Environment]::SetEnvironmentVariable($entry.Key, $entry.Value, 'Process')
        }
    }
    return
}
$oldPath = $env:PATH
$savedEnvironment = @{}
$proc = $null
try {
    $bin = Join-Path $Runtime 'bin'
    $env:PATH = "$bin;$oldPath"
    $flags = & (Join-Path $bin 'pkg-config.exe') --cflags --libs gtk4
    if($LASTEXITCODE) { throw 'GTK4 pkg-config failed' }
    # A response file preserves quoted include/library paths from pkgconf.
    $response = Join-Path $fixture 'gtk.flags'
    [IO.File]::WriteAllText($response, ($flags -join [Environment]::NewLine), (New-Object Text.UTF8Encoding($false)))
    $exe = Join-Path $fixture 'package-probe.exe'
    & (Join-Path $bin 'g++.exe') "--sysroot=$Runtime" -std=c++17 (Join-Path $root 'tests/cpp_package_probe.cpp') -o $exe "@$response"
    if($LASTEXITCODE) { throw 'Package probe compilation failed' }
    & $Python (Join-Path $PSScriptRoot 'package_cpp_windows.py') --executable $exe --runtime $Runtime --output $Output --objdump (Join-Path $bin 'objdump.exe')
    if($LASTEXITCODE) { throw 'Packaging failed' }
    $unicodeName = -join ([char[]](0x041F,0x0440,0x0438,0x043C,0x0435,0x0440))
    $relocated = $Output + " moved $unicodeName's"
    if(Test-Path -LiteralPath $relocated) { throw 'Relocation destination already exists' }
    Move-Item -LiteralPath $Output -Destination $relocated
    $Output = $relocated
    $manifest = Get-Content -LiteralPath (Join-Path $Output 'distribution.json') -Raw | ConvertFrom-Json
    foreach($file in $manifest.files) {
        $path = Join-Path $Output $file.path
        if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $file.sha256) { throw "Hash mismatch: $path" }
    }
    foreach($compilerPart in @('bin/g++.exe','include','lib/libgtk-4.dll.a')) {
        if(Test-Path -LiteralPath (Join-Path $Output $compilerPart)) { throw "Compiler files shipped: $compilerPart" }
    }
    $svg = Join-Path $Output 'probe.svg'
    [IO.File]::WriteAllText($svg, '<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16"><rect width="16" height="16" fill="red"/></svg>')
    foreach($key in @('GTK_PATH','GTK_THEME','GTK_DATA_PREFIX','GTK_EXE_PREFIX','GDK_PIXBUF_MODULE_FILE','GDK_PIXBUF_MODULEDIR','GSETTINGS_SCHEMA_DIR','XDG_DATA_DIRS','GI_TYPELIB_PATH','FONTCONFIG_PATH','WINEPATH')) {
        $savedEnvironment[$key] = [Environment]::GetEnvironmentVariable($key, 'Process')
        [Environment]::SetEnvironmentVariable($key, $null, 'Process')
    }
    $env:PATH = "$env:WINDIR/System32;$env:WINDIR"
    $out = Join-Path $fixture 'run.out'; $err = Join-Path $fixture 'run.err'
    $proc = Start-Process -FilePath (Join-Path $Output $manifest.entry) -ArgumentList ('"' + $svg + '"') -WorkingDirectory $env:TEMP -RedirectStandardOutput $out -RedirectStandardError $err -PassThru
    $processHandle = $proc.Handle
    Start-Sleep -Milliseconds 1200
    if($proc.HasExited) { throw "Probe exited before module check: $(Get-Content $out,$err | Out-String)" }
    $external = @($proc.Modules | Where-Object {
        $_.FileName.StartsWith($Runtime, [StringComparison]::OrdinalIgnoreCase) -or $_.FileName -match '[\\/]msys64[\\/]'
    })
    if($external.Count) { throw "External modules loaded: $($external.FileName -join ', ')" }
    if(-not $proc.WaitForExit(30000)) { throw 'Standalone probe timed out' }
    if($proc.ExitCode -ne 0 -or (Get-Content $out -Raw) -notmatch 'OK standalone SVG and icon theme') {
        throw "Standalone failed ($($proc.ExitCode)): $(Get-Content $out,$err | Out-String)"
    }
    Write-Output "OK standalone GTK4: relocated Unicode path, isolated PATH, SVG, icon theme, SHA256 manifest, no compiler modules: $Output"
} finally {
    if($proc -and -not $proc.HasExited) { $proc.Kill(); $proc.WaitForExit() }
    foreach($key in $savedEnvironment.Keys) { [Environment]::SetEnvironmentVariable($key, $savedEnvironment[$key], 'Process') }
    $env:PATH = $oldPath
}
