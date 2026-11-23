# Build a single-image .ico from a PNG. The PNG is embedded as-is, which is
# supported by Windows Vista and newer (including NSIS and the shell).
param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(Mandatory = $true)][string]$Output
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing

$sourcePath = (Resolve-Path -LiteralPath $Source).Path
$image = [System.Drawing.Image]::FromFile($sourcePath)
try {
    $width = $image.Width
    $height = $image.Height
} finally {
    $image.Dispose()
}
if ($width -gt 256 -or $height -gt 256) {
    throw "ICO source must be 256x256 or smaller: ${width}x${height}"
}

$png = [System.IO.File]::ReadAllBytes($sourcePath)
$outputDirectory = Split-Path -Path $Output -Parent
if ($outputDirectory -and -not (Test-Path -LiteralPath $outputDirectory)) {
    New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
}

$stream = New-Object System.IO.MemoryStream
$writer = New-Object System.IO.BinaryWriter($stream)
try {
    $writer.Write([UInt16]0)                         # reserved
    $writer.Write([UInt16]1)                         # icon
    $writer.Write([UInt16]1)                         # image count
    $writer.Write([Byte]($width -band 0xFF))         # 256 is encoded as 0
    $writer.Write([Byte]($height -band 0xFF))
    $writer.Write([Byte]0)                           # palette colors
    $writer.Write([Byte]0)                           # reserved
    $writer.Write([UInt16]1)                         # color planes
    $writer.Write([UInt16]32)                        # bits per pixel
    $writer.Write([UInt32]$png.Length)
    $writer.Write([UInt32]22)                        # ICONDIR + one entry
    $writer.Write($png)
    $writer.Flush()
    [System.IO.File]::WriteAllBytes($Output, $stream.ToArray())
} finally {
    $writer.Dispose()
    $stream.Dispose()
}
