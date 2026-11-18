#!/bin/sh
# Run under xvfb-run. This checks Linux X11 UI; it is not Windows acceptance.
set -eu
binary=$(readlink -f "${1:-build/linux-x64-gtk4-debug/hiasm}")
output=$(readlink -f "${2:-build/qa}")
mkdir -p "$output"
qa_home=$(mktemp -d /tmp/hiasm-ui-home-XXXXXX)
export HOME="$qa_home" LC_ALL=C.UTF-8
export GSETTINGS_BACKEND=memory
openbox >"$output/linux-ui-wm.log" 2>&1 &
wm=$!
"$binary" >"$output/linux-ui.log" 2>&1 &
app=$!
trap 'kill "$app" "$wm" 2>/dev/null || true' EXIT HUP INT TERM
# Startup can briefly expose a splash window; never retain that XID as the
# main window. Wait for its final title after startup has completed.
xdotool search --sync --onlyvisible --pid "$app" --name 'HiAsm Studio' >/dev/null
sleep 1
window=$(xdotool search --onlyvisible --pid "$app" --name '^HiAsm Studio \[' | head -n 1)
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
# Exercise the real MainGUI native-window close handler with an edited text
# document. Keep this fixture outside user projects and the user's home.
printf '%s\n' 'original text' >"$output/linux-ui-close.txt"
xdotool windowactivate --sync "$window"
xdotool key --clearmodifiers ctrl+o
sleep 0.4
xdotool key --clearmodifiers ctrl+l
xdotool type --clearmodifiers "$output/linux-ui-close.txt"
xdotool key Return
sleep 0.6
xdotool key --clearmodifiers ctrl+End
xdotool type --clearmodifiers ' unsaved change'
xdotool key --clearmodifiers alt+F4
sleep 0.4
warnings=$(xdotool search --onlyvisible --pid "$app" --name '^Warning$')
test -n "$warnings" || { echo 'Native close did not prompt to save' >&2; exit 1; }
import -window root "$output/linux-ui-close-warning.png"
xdotool key Escape
sleep 0.2
kill -0 "$app" || { echo 'Cancel closed the editor' >&2; exit 1; }
xdotool windowactivate --sync "$window"
xdotool key --clearmodifiers alt+F4
sleep 0.2
xdotool key --clearmodifiers alt+n
for attempt in 1 2 3 4 5; do
    if ! kill -0 "$app" 2>/dev/null; then break; fi
    sleep 0.2
done
if kill -0 "$app" 2>/dev/null; then echo 'Discard did not close the editor' >&2; exit 1; fi
test "$(cat "$output/linux-ui-close.txt")" = 'original text' || { echo 'Discard overwrote the original' >&2; exit 1; }
echo 'OK Linux X11: New/Open dialogs; native close Cancel retains changes, Discard exits without overwriting.'
