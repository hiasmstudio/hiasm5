#!/bin/sh
# Run under xvfb-run. This checks Linux X11 UI; it is not Windows acceptance.
set -eu
binary=$(readlink -f "${1:-build/qt/hiasm}")
output=$(readlink -f "${2:-build/qa}")
mkdir -p "$output"
qa_home=$(mktemp -d /tmp/hiasm-ui-home-XXXXXX)
export HOME="$qa_home" LC_ALL=C.UTF-8
export QT_QPA_PLATFORM=xcb
openbox >"$output/linux-ui-wm.log" 2>&1 &
wm=$!
"$binary" >"$output/linux-ui.log" 2>&1 &
app=$!
trap 'kill "$app" "$wm" 2>/dev/null || true' EXIT HUP INT TERM
timeout 15 xdotool search --sync --onlyvisible --pid "$app" --name ' - HiAsm$' >/dev/null
window=$(xdotool search --onlyvisible --pid "$app" --name ' - HiAsm$' | head -n 1)
test -n "$window"
xdotool windowactivate --sync "$window"
import -window root "$output/linux-ui-startup.png"
xdotool key --clearmodifiers ctrl+n
sleep 1
new_windows=$(xdotool search --onlyvisible --pid "$app" --name '.')
if [ "$(printf '%s\n' "$new_windows" | wc -l)" -lt 2 ]; then echo 'Ctrl+N did not open a dialog' >&2; exit 1; fi
for id in $new_windows; do xdotool getwindowname "$id"; done >"$output/linux-ui-new-windows.log"
import -window root "$output/linux-ui-new.png"
xdotool key Escape
sleep 0.2
xdotool windowactivate --sync "$window"
xdotool key --clearmodifiers ctrl+o
sleep 1
open_windows=$(xdotool search --onlyvisible --pid "$app" --name '.')
if [ "$(printf '%s\n' "$open_windows" | wc -l)" -lt 2 ]; then echo 'Ctrl+O did not open a dialog' >&2; exit 1; fi
for id in $open_windows; do xdotool getwindowname "$id"; done >"$output/linux-ui-open-windows.log"
import -window root "$output/linux-ui-open.png"
xdotool key Escape
sleep 0.2
# The same X display is used for editing, undo and close-confirmation tests.
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
"$(dirname "$binary")/qt_scheme_editor_test" "$root/packs/_base/example/tutorial/Index.sha"
echo 'OK Linux X11: startup, dialogs and scheme editor'
