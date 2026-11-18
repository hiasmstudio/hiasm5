[CmdletBinding()]
param([string]$Root = '')

$ErrorActionPreference = 'Stop'
if ([string]::IsNullOrWhiteSpace($Root)) { $Root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path }
function Assert-Contains([string]$Path, [string]$Pattern) {
    $text = Get-Content -Raw -LiteralPath (Join-Path $Root $Path)
    if ($text -notmatch $Pattern) { throw "$Path does not contain expected GTK4 contract: $Pattern" }
}
Assert-Contains 'src/gtkcompat.h' 'GestureClick'
Assert-Contains 'src/gtkcompat.h' 'widget_add_child'
Assert-Contains 'src/gtkcompat.h' 'clipboard_wait_for_text'
Assert-Contains 'src/main_gui.cpp' 'HIASM_GTK4'
Assert-Contains 'src/main_gui.cpp' 'hiasm.css'
Assert-Contains 'src/WindowManager.cpp' 'CMD_MOVETO'
Assert-Contains 'src/WindowManager.cpp' 'CMD_ALIGN'
Assert-Contains 'src/FTCG/CGTShare.h' 'static_assert'
Assert-Contains 'src/RTCG/CGTShare.h' 'static_assert'
Assert-Contains 'packs/cpp/elements.sql' 'CREATE TABLE elements'
Write-Output 'OK GTK4/source contract checks'
