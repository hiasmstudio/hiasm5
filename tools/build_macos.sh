#!/usr/bin/env bash
# Сборка HiAsm Studio на macOS (CMake, GTK3/gtkmm3 из Homebrew).
#
# Использование:
#   bash tools/build_macos.sh              # Debug
#   bash tools/build_macos.sh release      # Release
#   bash tools/build_macos.sh debug test   # Debug + ctest
#   bash tools/build_macos.sh release run  # Release + запуск из build/macos

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(cd "$SCRIPT_DIR/.." && pwd)"
ENV_FILE="${HOME}/.hiasm_macos_env.sh"

if [[ -f "$ENV_FILE" ]]; then
    # shellcheck disable=SC1090
    source "$ENV_FILE"
fi

BUILD_TYPE="Debug"
RUN_TEST=0
RUN_APP=0

for arg in "$@"; do
    case "$arg" in
        release|Release) BUILD_TYPE="Release" ;;
        debug|Debug) BUILD_TYPE="Debug" ;;
        test) RUN_TEST=1 ;;
        run) RUN_APP=1 ;;
        *)
            echo "Неизвестный аргумент: $arg"
            echo "Использование: $0 [debug|release] [test] [run]"
            exit 1
            ;;
    esac
done

BUILD_DIR="${PROJECT_ROOT}/build/macos"

echo "=== HiAsm Studio macOS: ${BUILD_TYPE} ==="
cmake -S "$PROJECT_ROOT" -B "$BUILD_DIR" \
    -DCMAKE_BUILD_TYPE="$BUILD_TYPE" \
    -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build "$BUILD_DIR" --parallel

APP_BIN="${BUILD_DIR}/hiasm"
if [[ ! -x "$APP_BIN" ]]; then
    echo "[ОШИБКА] Исполняемый файл не найден: $APP_BIN"
    exit 1
fi

echo "Собрано: $APP_BIN"

if [[ "$RUN_TEST" -eq 1 ]]; then
    echo "=== CTest ==="
    ctest --test-dir "$BUILD_DIR" --output-on-failure
fi

if [[ "$RUN_APP" -eq 1 ]]; then
    echo "=== Запуск HiAsm Studio ==="
    # Программа ищет int/, elements/, gtkrc рядом с текущим каталогом.
    (cd "$BUILD_DIR" && "$APP_BIN")
fi
