# Сборка и установщик Windows

Краткая пользовательская инструкция — в `README.md`. Здесь технические детали
пайплайна `tools/`.

## Требования

- MSYS2 UCRT64: `gcc`, `pkgconf`, `gtkmm3`, `gtksourceviewmm3`, `sqlite3`,
  `ninja`, `make`, `cmake`.
- CMake 3.21+ (пресеты `windows-mingw-debug` / `windows-mingw-release` для GTK3,
  `windows-mingw-gtk4-debug` / `windows-mingw-gtk4-release` для GTK4).
- Для GTK4 дополнительно: `mingw-w64-ucrt-x86_64-gtk4`, `mingw-w64-ucrt-x86_64-gtkmm-4.0`.
  `gtksourceview5` используется через C API и нужен на Windows и Linux.
- NSIS 3.x (`makensis.exe`) — только для установщика. Поддерживаются GTK3 и GTK4.

## Обычная сборка

```bat
tools\build_windows.bat [gtk3|gtk4] [debug|release] [clean] [run]
```

По умолчанию собирается GTK4 (`gtk4`, `debug`). GTK3 оставлен как совместимый
переходный профиль. `gtk4` использует пресеты
`windows-mingw-gtk4-*` и каталоги `build\Windows_MinGW-x64-GTK4-*`.

Скрипт:

1. добавляет `<MSYS2_ROOT>\ucrt64\bin` и `<MSYS2_ROOT>\usr\bin` в `PATH` (`MSYS2_ROOT` по умолчанию — `C:\msys64`);
2. выставляет `XDG_DATA_DIRS`;
3. вызывает `cmake --preset <preset>` (`windows-mingw[-gtk4]-<type>`) и
   `cmake --build --preset ... --parallel`;
4. печатает путь к `hiasm5.exe` (и запускает его с `run`).

Каталоги сборки:

| Бэкенд | Тип | Каталог |
|--------|-----|---------|
| GTK3 | Debug | `build/Windows_MinGW-x64-Debug` |
| GTK3 | Release | `build/Windows_MinGW-x64-Release` |
| GTK4 | Debug | `build/Windows_MinGW-x64-GTK4-Debug` |
| GTK4 | Release | `build/Windows_MinGW-x64-GTK4-Release` |

`cmake/StageRuntime.cmake` копирует `resources/` (`int/`, `gtkrc`, `splash.png`)
и `packs/` в runtime-каталог `packs/`, затем пересобирает
`packs/_base/elements.db` через `sqlite3`. Каждый пакет может содержать свой
`compiler/`; web-пакет использует `example/` с регистром, совместимым с Unix.

## Установщик

Установщик поддерживает оба backend. GTK4 является профилем по умолчанию; GTK3
выбирается явно параметром `gtk3`.

```bat
tools\build_windows_installer.bat gtk4
```

Этапы:

1. Release-сборка через выбранный пресет (`gtk4` или `gtk3`).
2. `cmake --install` выбранного каталога Release
   (`build\Windows_MinGW-x64-GTK4-Release` или
   `build\Windows_MinGW-x64-Release`) с префиксом
   `build\windows-installer\gtk4-install`/`gtk3-install`. Правила `install()` в
   `CMakeLists.txt` кладут exe, `int/`, `packs/`, CSS/`gtkrc`, `splash.png`,
   `LICENSE`).
3. `tools\deploy_gtk_runtime.ps1`:
   - рекурсивно обходит таблицы импорта `hiasm5.exe` и всех
     `lib\gdk-pixbuf-2.0\2.10.0\loaders\*.dll`;
   - копирует из `C:\msys64\ucrt64\bin` все DLL, которых нет в
     `C:\Windows\System32` (GTK/GLib/Cairo/libstdc++/libwinpthread и т.д.);
   - системные `api-ms-win-*` / `ext-ms-*` пропускаются как виртуальные;
   - копирует данные GTK: `share\glib-2.0\schemas`, `share\icons`,
     `share\gtk-4.0` и `share\gtksourceview-5` для GTK4 либо
     `share\gtksourceview-3.0` для GTK3, а также `share\themes`, `share\fontconfig`,
     `etc\fonts`, `share\locale`, а также `loaders.cache`.
4. `tools/nsis_installer.nsi`:
   - проверяет наличие exe, `int\hiasm.db`, `packs\_base\elements.db`,
     соответствующей GTK DLL (`libgtk-3-0.dll` или `libgtk-4-1.dll`),
     `gschemas.compiled` и `loaders.cache`;
   - `tools/nsis_payload.ps1` генерирует макросы `HIASM_INSTALL_PAYLOAD` /
     `HIASM_UNINSTALL_PAYLOAD` со списком всех файлов;
   - установщик кладёт файлы в `$PROGRAMFILES64\HiAsm Studio 5`, пишет ключи
     `HKLM\Software\HiAsm5` и `...\Uninstall\HiAsm Studio 5`, создаёт ярлыки
     в меню «Пуск».
5. Результат — `build\HiAsm5_Setup.exe`, версия берётся из `src/share.h`
   (`HIASM_VERSION_*`).

### Почему SVG-загрузчик обязателен

Тема Adwaita содержит `image-missing.svg`. Если `pixbufloader_svg.dll` или его
зависимость `librsvg-2-2.dll` отсутствует, GTK падает с
`ensure_surface_for_gicon: assertion failed`. Поэтому замыкание зависимостей
строится не только от exe, но и от всех GdkPixbuf loaders.

`loaders.cache` в MSYS2 уже хранит относительные пути
(`lib\gdk-pixbuf-2.0\...`), поэтому его можно копировать без пересборки.
Приложение и установщик запускают exe с рабочим каталогом установки; это же
ожидает `src/share.cpp` для поиска `int/`, `packs/`, `gtkrc`.

## macOS

```sh
bash tools/build_macos.sh [debug|release] [test] [run]
```

Скрипт использует CMake и каталог `build/macos`. `macdeployqt`
не используется: проект на GTK3, а не Qt.

## Известные ограничения

- Скрипты `tools\build_windows.bat`, `tools\build_windows_installer.bat`,
  `tools\run-debug.bat` и `tools\make_windows.bat` принимают путь через
  переменную `MSYS2_ROOT`; по умолчанию используется `C:\msys64`. Скрипты
  передают выбранные компилятор, Ninja и `pkg-config` в CMake, поэтому
  редактировать `CMakePresets.json` для нестандартного пути не требуется.
- `share\locale` добавляет ~57 МБ до сжатия; для уменьшения установщика можно
  передать `-SkipLocale` в `deploy_gtk_runtime.ps1`.
- Ассоциация `.sha` не регистрируется: `main_gui.cpp` пока не разбирает
  аргументы командной строки.
- Иконка `resources/hiasm.ico` генерируется из `resources/hiasm.png`
  скриптом `tools/png_to_ico.ps1` (32×32, PNG-в-ICO) и встраивается в exe
  через `resources/hiasm.rc`.

## FTCG и portable Free Pascal

`powershell -NoProfile -ExecutionPolicy Bypass -File tools/setup_fpc_windows.ps1` загружает проверенные SHA256 дистрибутив FPC 3.2.2 и innoextract в `build/deps`, извлекает Win64 compiler без системной установки. `tools/fpc_windows.bat` запускает этот компилятор.

Windows build helper автоматически включает FTCG при наличии поддерживаемого FPC. `ftcg_test` проверяет HTML/CSS/JavaScript/PHP через ProjectBuilder и Unicode пути. C++17 project types и BuildMode описаны в `packs/cpp/README.md`.

`powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_all_checks.ps1` выполняет source/database checks, GTK4/GTK3 build и CTest, startup smoke и изолированную portable проверку. Визуальная приёмка и установка/удаление на чистой Windows остаются отдельными шагами.

## Проверенный статус 2026-11-03

- GTK4 Debug/Release: 21/21 CTest; GTK3 Debug: 18/18; Linux GTK4: 19/19.
- Debug проверен с `-CrossGui -WslDistribution HiAsm-GTK4-QA`: Windows → Linux
  GTK4/console, запуск и Stop. Linux suite проверяет обратный GTK4 cross-путь
  через Wine. Окружение целевой ОС необходимо подготовить заранее.
- `-Package -CppRuntime ПУТЬ -Python ПУТЬ` в `run_all_checks.ps1` добавляет
  автономную упаковку Windows C++17 GUI. Проверки переноса каталога, SVG,
  иконок и отсутствия DLL из MSYS2/toolchain выполнены на Windows и Wine.
- Повторный Release portable deploy, два запуска с очищенным окружением
  и runner validation прошли; Windows capture и чистая ОС остаются открытыми.
- Installer `build/HiAsm5_Setup_GTK4.exe` собран; 7-Zip проверил весь NSIS
  payload. Извлечённая IDE запускается из изолированного PATH, toolchain
  собирает и запускает GTK4 probe. Текущий процесс не elevated, поэтому
  именно этот installer не запускался и не устанавливался; чистая Windows
  остаётся открытой.
- Локальный GTK runtime исправляет DnD startup и Win32 IME use-after-free.
  Сборка, лицензии, patches, включение в IDE и bundled C++ toolchain описаны
  в `GTK4_RUNTIME_FIXES.md`. Не подменяйте эти DLL случайной системной версией.

Журналы: `build/qa/complete-debug-tests.log`, `complete-release-tests.log`,
`complete-gtk3-tests.log`, `final-linux-tests.log`, `complete-portable-tests.log`,
`complete-cpp-package-tests.log`, `complete-cpp-package-linux.log`,
`complete-installer-build.log`, `complete-installer-archive-test.log` и
`complete-installer-payload-tests.log`.
