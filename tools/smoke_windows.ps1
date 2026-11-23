[CmdletBinding()]
param([string]$InstallDir)
$ErrorActionPreference = 'Stop'
if (!$InstallDir) {
    & "$PSScriptRoot/build_qt_windows.ps1" -Configuration Debug -SkipBuild -SkipTests -RunIde -Check
    return
}
$prefix = (Resolve-Path -LiteralPath $InstallDir).Path
$previous = @{}
foreach ($name in 'PATH','QT_PLUGIN_PATH','QT_QPA_PLATFORM_PLUGIN_PATH','QT_QPA_PLATFORM') {
    $previous[$name] = [Environment]::GetEnvironmentVariable($name)
}
try {
    $env:PATH = "$env:SystemRoot/System32;$env:SystemRoot"
    $env:QT_PLUGIN_PATH = $null
    $env:QT_QPA_PLATFORM_PLUGIN_PATH = $null
    $env:QT_QPA_PLATFORM = 'offscreen'
    foreach ($file in 'int/hiasm.db','qt/platforms/qwindows.dll','qt/hiasm-core-qt.dll',
             'packs/cpp/elements.db','packs/cpp/compiler/CodeGen.dll','packs/web/compiler/CodeGen.dll',
             'packs/modules/compiler/CodeGen.dll') {
        if (!(Test-Path -LiteralPath (Join-Path $prefix $file) -PathType Leaf)) { throw "Missing: $file" }
    }
    & "$prefix/qt/hiasm.exe" --check "$prefix/packs/cpp/new/QtForm.sha" | Out-Host
    if ($LASTEXITCODE -ne 0) { throw "Installed IDE failed ($LASTEXITCODE)" }
} finally {
    foreach ($entry in $previous.GetEnumerator()) {
        [Environment]::SetEnvironmentVariable($entry.Key, $entry.Value, 'Process')
    }
}
Write-Output 'OK: installed Qt IDE with a system-only PATH'
