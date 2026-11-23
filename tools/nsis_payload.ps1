# Compile-time NSIS inventory for a staged HiAsm installation.
# Generates macros that install exactly the files in -Source and remove exactly
# those files (then empty directories) on uninstall.
param(
    [Parameter(Mandatory = $true)][string]$Source,
    [Parameter(Mandatory = $true)][string]$Output
)
$ErrorActionPreference = 'Stop'

$sourceDirectory = Get-Item -LiteralPath $Source
if (-not $sourceDirectory.PSIsContainer -or
    ($sourceDirectory.Attributes -band [System.IO.FileAttributes]::ReparsePoint)) {
    throw 'Payload source must be a directory, not a file, link or junction.'
}
$payloadRoot = $sourceDirectory.FullName.TrimEnd('\')

function NsisLiteral([string]$Value) {
    return $Value.Replace('$', '$$').Replace('"', '$\"')
}

$pendingDirectories = New-Object 'System.Collections.Generic.Queue[string]'
$payloadFiles = New-Object 'System.Collections.Generic.List[System.IO.FileInfo]'
$payloadDirectories = New-Object 'System.Collections.Generic.List[System.IO.DirectoryInfo]'
$pendingDirectories.Enqueue($payloadRoot)
while ($pendingDirectories.Count -gt 0) {
    $directory = $pendingDirectories.Dequeue()
    foreach ($entry in Get-ChildItem -LiteralPath $directory -Force) {
        if ($entry.Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
            throw "Payload must not contain links or junctions: $($entry.FullName)"
        }
        if ($entry.PSIsContainer) {
            $payloadDirectories.Add($entry)
            $pendingDirectories.Enqueue($entry.FullName)
        } else {
            $payloadFiles.Add($entry)
        }
    }
}

$lines = New-Object 'System.Collections.Generic.List[string]'
$lines.Add('!macro HIASM_INSTALL_PAYLOAD')
foreach ($file in ($payloadFiles | Sort-Object FullName)) {
    $relative = $file.FullName.Substring($payloadRoot.Length + 1)
    $parent = Split-Path -Path $relative -Parent
    $destination = '$INSTDIR'
    if ($parent) { $destination += '\' + (NsisLiteral $parent) }
    $lines.Add('  SetOutPath "' + $destination + '"')
    # File's source is a compile-time path; $ escaping applies only to its destination.
    $lines.Add('  File "/oname=' + (NsisLiteral $file.Name) + '" "' + (NsisLiteral $file.FullName) + '"')
}
$lines.Add('!macroend')

$lines.Add('!macro HIASM_UNINSTALL_PAYLOAD')
foreach ($file in ($payloadFiles | Sort-Object FullName)) {
    $relative = $file.FullName.Substring($payloadRoot.Length + 1)
    $lines.Add('  Delete "$INSTDIR\' + (NsisLiteral $relative) + '"')
}
foreach ($directory in ($payloadDirectories | Sort-Object { $_.FullName.Length } -Descending)) {
    $relative = $directory.FullName.Substring($payloadRoot.Length + 1)
    $lines.Add('  RMDir "$INSTDIR\' + (NsisLiteral $relative) + '"')
}
$lines.Add('!macroend')

[System.IO.File]::WriteAllLines($Output, $lines, (New-Object System.Text.UTF8Encoding($true)))
