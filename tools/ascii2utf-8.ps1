# Convert-IniToUtf8.ps1
# Processes *.ini files in the same folder as this script.
# Default: ASCII files are rewritten as UTF-8 with BOM.
# Use -NoBom to write UTF-8 without BOM.
# Use -Backup to create .bak before changing a file.
# Use -AnsiCodePage 1251 if you also need to convert ANSI/Windows-1251 files.

[CmdletBinding()]
param(
    [switch]$NoBom,
    [switch]$Backup,
    [int]$AnsiCodePage = 0
)

$scriptPath = $MyInvocation.MyCommand.Path
if ($scriptPath) {
    $root = Split-Path -Parent $scriptPath
} elseif ($PSScriptRoot) {
    $root = $PSScriptRoot
} else {
    $root = (Get-Location).Path
}

$utf8Bom = New-Object System.Text.UTF8Encoding -ArgumentList $true
$utf8NoBom = New-Object System.Text.UTF8Encoding -ArgumentList $false

if ($NoBom) {
    $targetEnc = $utf8NoBom
} else {
    $targetEnc = $utf8Bom
}

$asciiEnc = [System.Text.Encoding]::ASCII

$ansiEnc = $null
if ($AnsiCodePage -gt 0) {
    try {
        $ansiEnc = [System.Text.Encoding]::GetEncoding($AnsiCodePage)
    } catch {
        Write-Warning "Cannot get code page $AnsiCodePage."
    }
}

$files = Get-ChildItem -LiteralPath $root -Filter *.ini |
    Where-Object { -not $_.PSIsContainer } |
    Sort-Object Name

if (-not $files) {
    Write-Host "No .ini files found in: $root"
    return
}

foreach ($file in $files) {
    try {
        $bytes = [System.IO.File]::ReadAllBytes($file.FullName)

        if ($bytes.Length -eq 0) {
            Write-Host "SKIP   : $($file.Name) (empty file)"
            continue
        }

        # UTF-8 with BOM: EF BB BF
        if ($bytes.Length -ge 3 -and $bytes[0] -eq 0xEF -and $bytes[1] -eq 0xBB -and $bytes[2] -eq 0xBF) {
            Write-Host "SKIP   : $($file.Name) (already UTF-8 with BOM)"
            continue
        }

        # Check if file is pure ASCII
        $isAscii = $true
        foreach ($b in $bytes) {
            if ($b -gt 0x7F) {
                $isAscii = $false
                break
            }
        }

        if ($isAscii) {
            if ($Backup) {
                Copy-Item -LiteralPath $file.FullName -Destination "$($file.FullName).bak" -Force
            }

            $text = $asciiEnc.GetString($bytes)
            [System.IO.File]::WriteAllText($file.FullName, $text, $targetEnc)

            if ($NoBom) {
                Write-Host "CONVERT: $($file.Name) (ASCII -> UTF-8 without BOM; bytes unchanged)"
            } else {
                Write-Host "CONVERT: $($file.Name) (ASCII -> UTF-8 with BOM)"
            }

            continue
        }

        # Not ASCII. Check if it is valid UTF-8 without BOM.
        try {
            $strictUtf8 = New-Object System.Text.UTF8Encoding -ArgumentList $false, $true
            [void]$strictUtf8.GetString($bytes)
            Write-Host "SKIP   : $($file.Name) (already UTF-8 without BOM)"
            continue
        } catch {}

        # Optional: convert ANSI/Windows-1251 etc. if code page was specified.
        if ($ansiEnc -ne $null) {
            if ($Backup) {
                Copy-Item -LiteralPath $file.FullName -Destination "$($file.FullName).bak" -Force
            }

            $text = $ansiEnc.GetString($bytes)
            [System.IO.File]::WriteAllText($file.FullName, $text, $targetEnc)

            Write-Host "CONVERT: $($file.Name) (code page $AnsiCodePage -> UTF-8)"
        } else {
            Write-Warning "SKIP   : $($file.Name) (not ASCII and not valid UTF-8; use -AnsiCodePage if it is ANSI)"
        }
    } catch {
        Write-Warning "ERROR  : $($file.Name): $($_.Exception.Message)"
    }
}

Write-Host "Done."