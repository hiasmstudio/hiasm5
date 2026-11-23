#!/bin/sh
# Debian/Ubuntu cross toolchain and isolated Wine prefix.
set -eu
install=0
prefix="${HIASM_WINE_PREFIX:-$HOME/.local/share/hiasm/wine-cpp64}"
toolchain="${HIASM_CPP_WINDOWS_TOOLCHAIN:-}"
environment=
verify_gui=0
script_dir=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd -P)
usage() { echo 'Usage: setup_cpp_linux.sh [--install-dependencies] [--prefix ABSOLUTE_PATH] [--windows-toolchain ABSOLUTE_PATH] [--environment ABSOLUTE_FILE] [--verify-gui]' >&2; exit 2; }
while [ "$#" -gt 0 ]; do
    case "$1" in
        --install-dependencies) install=1 ;;
        --prefix) [ "$#" -gt 1 ] || usage; shift; prefix="$1" ;;
        --windows-toolchain) [ "$#" -gt 1 ] || usage; shift; toolchain="$1" ;;
        --environment) [ "$#" -gt 1 ] || usage; shift; environment="$1" ;;
        --verify-gui) verify_gui=1 ;;
        *) usage ;;
    esac
    shift
done
case "$prefix" in /*) ;; *) echo 'Wine prefix must be an absolute path' >&2; exit 2 ;; esac
if [ "$install" = 1 ]; then
    if [ "$(id -u)" != 0 ]; then echo 'Run dependency installation as root' >&2; exit 1; fi
    apt-get update
    apt-get install -y --no-install-recommends g++-mingw-w64-x86-64-posix wine wine64
fi
command -v wine >/dev/null
mkdir -p "$prefix"
LC_ALL=C.UTF-8 WINEARCH=win64 WINEPREFIX="$prefix" wineboot -u
if [ -z "$toolchain" ]; then
    command -v x86_64-w64-mingw32-g++ >/dev/null
    x86_64-w64-mingw32-g++ --version
    [ "$verify_gui" = 0 ] || { echo '--verify-gui needs --windows-toolchain' >&2; exit 2; }
else
    case "$toolchain" in /*) ;; *) echo 'Toolchain path must be absolute' >&2; exit 2 ;; esac
    toolchain=$(CDPATH= cd -- "$toolchain" && pwd -P)
    export HIASM_CPP_WINDOWS_TOOLCHAIN="$toolchain" HIASM_WINE_PREFIX="$prefix"
    sh "$script_dir/cpp_wine_gxx.sh" --version
    sh "$script_dir/cpp_wine_pkg_config.sh" --modversion gtk4
    if [ "$verify_gui" = 1 ]; then
        probe=$(mktemp -d /tmp/hiasm-cross-gui-XXXXXX)
        trap 'rm -f "$probe/probe.cpp" "$probe/probe.exe" "$probe/gtk.flags"; rmdir "$probe"' EXIT HUP INT TERM
        cat >"$probe/probe.cpp" <<'CPP'
#include <gtk/gtk.h>
int main() {
  gtk_init();
  auto window=gtk_window_new();
  gtk_window_present(GTK_WINDOW(window));
  auto deadline=g_get_monotonic_time()+500000;
  while(g_get_monotonic_time()<deadline) {
    while(g_main_context_iteration(nullptr,false)) {}
    g_usleep(1000);
  }
  gtk_window_destroy(GTK_WINDOW(window));
  return 0;
}
CPP
        (cd "$probe"
         sh "$script_dir/cpp_wine_pkg_config.sh" --cflags --libs gtk4 >gtk.flags
         sh "$script_dir/cpp_wine_gxx.sh" -std=c++17 probe.cpp -o probe.exe @gtk.flags
         sh "$script_dir/cpp_wine_tool.sh" run probe.exe)
        echo 'OK: bundled Windows GTK4 compile and window lifecycle under Wine'
    fi
fi
if [ -n "$environment" ]; then
    case "$environment" in /*) ;; *) echo 'Environment file must be absolute' >&2; exit 2 ;; esac
    # Emit shell literals, including spaces, Unicode, and embedded apostrophes.
    quote() { printf "'%s'" "$(printf %s "$1" | sed "s/'/'\\\\''/g")"; }
    (printf 'export HIASM_WINE_PREFIX='; quote "$prefix"; printf '\n'
     if [ -n "$toolchain" ]; then
         printf 'export HIASM_CPP_WINDOWS_TOOLCHAIN='; quote "$toolchain"; printf '\n'
     fi) >"$environment"
    printf 'Load before launching HiAsm: . %s\n' "$environment"
fi
printf 'C++ cross toolchain ready. Launch HiAsm with HIASM_WINE_PREFIX=%s\n' "$prefix"
printf '%s\n' 'Select TargetPlatform=Windows. Use --windows-toolchain with the bundled UCRT64 closure for GTK4 GUI.'
