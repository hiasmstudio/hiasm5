[CmdletBinding()]
param(
    [string]$Root = '',
    [switch]$SkipNegative
)

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($Root)) {
    $scriptPath = $PSCommandPath
    if ([string]::IsNullOrWhiteSpace($scriptPath)) { $scriptPath = $MyInvocation.MyCommand.Path }
    $scriptDir = Split-Path -Parent $scriptPath
    $Root = (Resolve-Path (Join-Path $scriptDir '..')).Path
}

function Find-Sqlite3 {
    $candidates = @('C:/msys64/ucrt64/bin/sqlite3.exe', 'sqlite3.exe')
    if (-not [string]::IsNullOrWhiteSpace($env:MSYS2_UCRT64_PREFIX)) {
        $candidates = @((Join-Path $env:MSYS2_UCRT64_PREFIX 'bin/sqlite3.exe')) + $candidates
    }
    foreach ($candidate in $candidates) {
        if ($candidate -and (Test-Path -LiteralPath $candidate)) { return (Resolve-Path -LiteralPath $candidate).Path }
        if ($candidate -eq 'sqlite3.exe') {
            $command = Get-Command sqlite3.exe -ErrorAction SilentlyContinue
            if ($command) { return $command.Source }
        }
    }
    throw 'sqlite3.exe was not found. Install mingw-w64-ucrt-x86_64-sqlite3 or set MSYS2_UCRT64_PREFIX.'
}

function Invoke-Sqlite([string]$Sqlite, [string]$Database, [string]$Sql) {
    $output = & $Sqlite $Database $Sql 2>&1
    if ($LASTEXITCODE -ne 0) {
        throw "SQLite command failed for ${Database}: $($output -join [Environment]::NewLine)"
    }
    return @($output | ForEach-Object { $_.ToString().Trim() } | Where-Object { $_ })
}

$sqlite = Find-Sqlite3
$databases = @(
    (Join-Path $Root 'resources/hiasm.db'),
    (Join-Path $Root 'packs/web/elements.db'),
    (Join-Path $Root 'packs/cpp/elements.db')
)
$required = @{
    'resources/hiasm.db' = @('localization','commands','menus','menu_commands','packs','projects','params','compilers','pack_compilers')
    'packs/web/elements.db' = @('groups','elements')
    'packs/cpp/elements.db' = @('groups','elements')
}

foreach ($database in $databases) {
    if (-not (Test-Path -LiteralPath $database)) { throw "Database not found: $database" }
    $rootPrefix = $Root.TrimEnd('\','/')
    $relative = $database.Substring($rootPrefix.Length).TrimStart('\','/').Replace('\','/')
    $integrity = Invoke-Sqlite $sqlite $database 'PRAGMA integrity_check;'
    if (($integrity -join '') -ne 'ok') { throw "Integrity check failed for ${relative}: $($integrity -join ' ')" }
    $tables = @(Invoke-Sqlite $sqlite $database "SELECT name FROM sqlite_master WHERE type='table';")
    foreach ($table in $required[$relative]) {
        if ($tables -notcontains $table) { throw "Required table $table is missing from $relative" }
    }
    Write-Output "OK ${relative}: integrity and required tables"
}

if (-not $SkipNegative) {
    $temp = Join-Path ([IO.Path]::GetTempPath()) ('hiasm5-db-negative-' + [guid]::NewGuid().ToString('N') + '.db')
    try {
        Copy-Item -LiteralPath (Join-Path $Root 'packs/web/elements.db') -Destination $temp
        Invoke-Sqlite $sqlite $temp "DROP TABLE elements;" | Out-Null
        $tables = @(Invoke-Sqlite $sqlite $temp "SELECT name FROM sqlite_master WHERE type='table' AND name='elements';")
        if ($tables.Count -ne 0) { throw 'Negative fixture still contains elements table' }
        Write-Output 'OK negative fixture: missing elements table detected'
    }
    finally {
        Remove-Item -LiteralPath $temp -Force -ErrorAction SilentlyContinue
    }
}
