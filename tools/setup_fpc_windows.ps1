[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$cache = Join-Path $root 'build/deps/fpc-3.2.2'
New-Item -ItemType Directory -Path $cache -Force | Out-Null
function Get-VerifiedDownload([string]$Url, [string]$Path, [string]$Sha256) {
    if (-not (Test-Path -LiteralPath $Path)) {
        & curl.exe -fL --retry 2 --connect-timeout 30 $Url -o ($Path + '.part')
        if ($LASTEXITCODE) { throw "Download failed: $Url" }
        if ((Get-FileHash -LiteralPath ($Path + '.part') -Algorithm SHA256).Hash -ne $Sha256) { throw "Checksum mismatch: $Path.part" }
        Move-Item -LiteralPath ($Path + '.part') -Destination $Path
    }
    if ((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash -ne $Sha256) { throw "Checksum mismatch: $Path" }
}
Get-VerifiedDownload 'https://downloads.freepascal.org/fpc/dist/3.2.2/i386-win32/fpc-3.2.2.win32.and.win64.exe' (Join-Path $cache 'setup.exe') '8C255390544B051388B577EB61C6191A04883264AFE0E3369B3600A56DAF7BDE'
Get-VerifiedDownload 'https://constexpr.org/innoextract/files/innoextract-1.9-windows.zip' (Join-Path $cache 'innoextract.zip') '6989342C9B026A00A72A38F23B62A8E6A22CC5DE69805CF47D68AC2FEC993065'
$extractor = Join-Path $cache 'innoextract/innoextract.exe'
if (-not (Test-Path -LiteralPath $extractor)) { Expand-Archive -LiteralPath (Join-Path $cache 'innoextract.zip') -DestinationPath (Join-Path $cache 'innoextract') }
$compiler = Join-Path $cache 'extracted/app/bin/i386-win32/ppcrossx64.exe'
if (-not (Test-Path -LiteralPath $compiler)) {
    & $extractor --silent --output-dir (Join-Path $cache 'extracted') (Join-Path $cache 'setup.exe')
    if ($LASTEXITCODE) { throw 'Free Pascal extraction failed' }
}
if (-not (Test-Path -LiteralPath $compiler)) { throw 'Win64 compiler is missing after extraction' }
Write-Output "OK portable FPC 3.2.2 Win64: $compiler"
