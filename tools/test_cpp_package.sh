#!/bin/sh
# Linux -> Windows GTK4 packaging, using the bundled compiler through Wine.
set -eu
root=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd -P)
toolchain="${HIASM_CPP_WINDOWS_TOOLCHAIN:?Set HIASM_CPP_WINDOWS_TOOLCHAIN}"
report=$(mktemp -d "$root/build/qa/package-linux-XXXXXX")
# Wine on this host cannot map UCRT DLLs from the Windows/NTFS mount.
# This also keeps the test distribution physically separate from the compiler.
fixture=$(mktemp -d /tmp/hiasm-package-XXXXXX)
printf '%s\n' "$fixture" >"$report/distribution-root.txt"
cd "$fixture"
sh "$root/tools/cpp_wine_pkg_config.sh" --cflags --libs gtk4 >gtk.flags
cp "$root/tests/cpp_package_probe.cpp" probe.cpp
sh "$root/tools/cpp_wine_gxx.sh" -std=c++17 probe.cpp -o probe.exe @gtk.flags
python3 "$root/tools/package_cpp_windows.py" --executable probe.exe --runtime "$toolchain" \
    --output packaged --objdump "${HIASM_OBJDUMP:-x86_64-w64-mingw32-objdump}" \
    --wine "${HIASM_WINE:-wine}" --wine-prefix "${HIASM_WINE_PREFIX:?Set HIASM_WINE_PREFIX}"
mv packaged "moved Пример's"
distribution="$fixture/moved Пример's"
cat >"$distribution/probe.svg" <<'SVG'
<svg xmlns="http://www.w3.org/2000/svg" width="16" height="16"><rect width="16" height="16" fill="red"/></svg>
SVG
# Start outside the prefix, exactly as an installed application can be run.
cd /tmp
unset GDK_PIXBUF_MODULE_FILE GDK_PIXBUF_MODULEDIR GTK_DATA_PREFIX GTK_EXE_PREFIX GTK_PATH GIO_EXTRA_MODULES GSETTINGS_SCHEMA_DIR XDG_DATA_DIRS FONTCONFIG_PATH
export WINEPATH= WINEPREFIX="$HIASM_WINE_PREFIX" WINEDEBUG=+loaddll
svg=$(winepath -w "$distribution/probe.svg")
timeout 30 wine "$distribution/probe.exe" "$svg" >"$report/run.out" 2>"$report/run.err"
grep -q 'OK standalone SVG and icon theme' "$report/run.out"
if grep -iE 'Loaded.*(hiasm-mingw64|ucrt64|toolchains).*\.dll' "$report/run.err"; then
    echo 'Loaded compiler runtime modules outside the distribution' >&2; exit 1
fi
printf 'OK standalone cross GTK4 under Wine: SVG, icon theme, relocated Unicode path, empty WINEPATH: %s\n' "$distribution"
