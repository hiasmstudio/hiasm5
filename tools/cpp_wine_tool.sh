#!/bin/sh
# Execute the bundled Windows UCRT toolchain, without mixing it with the
# native MinGW CRT. Each invocation receives an isolated child environment.
set -eu
mode="${1:-}"
[ "$#" -gt 0 ] && shift
root="${HIASM_CPP_WINDOWS_TOOLCHAIN:-}"
case "$root" in /*) ;; *) echo 'Set HIASM_CPP_WINDOWS_TOOLCHAIN to an absolute Linux path' >&2; exit 2 ;; esac
root=$(CDPATH= cd -- "$root" && pwd -P)
test -f "$root/bin/g++.exe" && test -f "$root/bin/pkg-config.exe" || {
    echo 'Windows toolchain must contain bin/g++.exe and bin/pkg-config.exe' >&2; exit 2;
}
wine="${HIASM_WINE:-wine}"
winepath="${HIASM_WINEPATH:-winepath}"
if [ -n "${HIASM_WINE_PREFIX:-}" ]; then
    case "$HIASM_WINE_PREFIX" in /*) ;; *) echo 'Wine prefix must be absolute' >&2; exit 2 ;; esac
    export WINEPREFIX="$HIASM_WINE_PREFIX"
fi
# pkgconf treats apostrophes in an automatically relocated prefix as shell
# syntax and can return empty flags with status 0. A DOS short path also
# avoids ACP losses in Windows tools, while the caller retains its real path.
windows_root=$("$winepath" -w "$root")
windows_root=$("$winepath" -s "$windows_root")
bin="$windows_root/bin"
export WINEPATH="$bin${WINEPATH:+;$WINEPATH}"
export PKG_CONFIG_PATH=
export PKG_CONFIG_LIBDIR="$windows_root/lib/pkgconfig;$windows_root/share/pkgconfig"
# GCC's automatic executable-prefix discovery expands the short path again;
# explicit prefixes keep its subprocess and CRT/library paths representable.
export GCC_EXEC_PREFIX="$windows_root/lib/gcc/"
export COMPILER_PATH="$bin"
case "$mode" in
    compiler) exec "$wine" "$bin/g++.exe" "--sysroot=$windows_root" -fno-use-linker-plugin "$@" ;;
    pkg-config) exec "$wine" "$bin/pkg-config.exe" "$@" ;;
    run) exec "$wine" "$@" ;;
    *) echo 'Usage: cpp_wine_tool.sh compiler|pkg-config|run ARGS...' >&2; exit 2 ;;
esac
