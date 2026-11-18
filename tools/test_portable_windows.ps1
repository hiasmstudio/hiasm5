[CmdletBinding()]
param(
    [string]$BuildDir = '',
    [string]$InstallDir = '',
    [ValidateRange(1,10)][int]$StartupRuns = 2
)
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
if(-not $BuildDir) { $BuildDir = Join-Path $root 'build/Windows_MinGW-x64-GTK4-Debug' }
if(-not $InstallDir) {
    # Never overwrite an executable that may be open in a manual UI session.
    $InstallDir = Join-Path $root ('build/qa/portable-gtk4-' + [guid]::NewGuid().ToString('N'))
}
$InstallDir = [IO.Path]::GetFullPath($InstallDir)
Write-Output "Portable verification directory: $InstallDir"
$BuildDir = (Resolve-Path -LiteralPath $BuildDir).Path
cmake --install $BuildDir --prefix $InstallDir
if($LASTEXITCODE) { throw 'Portable install failed' }
$deployArguments = @('-NoProfile','-ExecutionPolicy','Bypass','-File',
    (Join-Path $PSScriptRoot 'deploy_gtk_runtime.ps1'),'-InstallDir',$InstallDir,
    '-RuntimeSource',$BuildDir,'-GtkVersion','4','-SkipLocale')
& powershell @deployArguments
if($LASTEXITCODE) { throw 'Portable deployment failed' }
# A second deployment must preserve the tree structure and patched GTK runtime.
$before = (Get-FileHash (Join-Path $InstallDir 'libgtk-4-1.dll')).Hash
& powershell @deployArguments
if($LASTEXITCODE) { throw 'Repeated deployment failed' }
if($before -ne (Get-FileHash (Join-Path $InstallDir 'libgtk-4-1.dll')).Hash) { throw 'GTK changed on redeploy' }
foreach($bad in @('int/int','packs/packs','share/icons/icons')) {
    if(Test-Path -LiteralPath (Join-Path $InstallDir $bad)) { throw "Nested deployment directory: $bad" }
}
for($i=0; $i -lt $StartupRuns; ++$i) {
    & powershell -NoProfile -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot 'smoke_windows.ps1') -InstallDir $InstallDir -CleanEnvironment
    if($LASTEXITCODE) { throw "Portable startup failed on run $i" }
}
$oldPath = $env:PATH
try {
    $env:PATH = "$env:WINDIR/System32;$env:WINDIR"
    $runner = Join-Path $InstallDir 'runner.exe'
    $out = Join-Path $InstallDir 'runner-validate.out'
    $err = Join-Path $InstallDir 'runner-validate.err'
    # ArgumentList needs quoting on Windows even when it receives an array.
    $args = '--validate --scheme "' + (Join-Path $InstallDir 'packs/_base/new/hcTplForm.sha') + '" --data-dir "' + $InstallDir + '"'
    $proc = Start-Process -FilePath $runner -WorkingDirectory $env:TEMP -ArgumentList $args -RedirectStandardOutput $out -RedirectStandardError $err -PassThru
    # Cache the process handle before it exits (Windows PowerShell otherwise
    # sometimes exposes a null ExitCode on a short-lived process).
    $processHandle = $proc.Handle
    if(-not $proc.WaitForExit(30000)) { $proc.Kill(); throw 'Portable runner timed out' }
    if($proc.ExitCode -ne 0) { throw "Portable runner failed: $(Get-Content $out,$err | Out-String)" }
} finally { $env:PATH = $oldPath }
Write-Output "OK portable GTK4: redeploy, $StartupRuns isolated startups, runner validation"
