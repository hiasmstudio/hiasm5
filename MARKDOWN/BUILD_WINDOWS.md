# Сборка и установка HiAsm на Windows

## Комплект разработки

Скрипты запускаются из PowerShell или cmd.exe. Нужны Python 3 с модулем sqlite3 и Qt SDK. В Windows проверяется следующая конфигурация.

| Компонент | Каталог или файл по умолчанию | Переменная окружения |
|---|---|---|
| Qt Widgets, Sql и LinguistTools | `C:/Qt/6.9.3/llvm-mingw_64` | `HIASM_QT_DIR` |
| LLVM-MinGW x64 | `C:/Qt/Tools/llvm-mingw1706_64/bin` | `HIASM_QT_COMPILER_DIR` |
| Ninja | `C:/Qt/Tools/Ninja/ninja.exe` | `HIASM_NINJA` |
| CMake и CTest | `C:/Qt/Tools/CMake_64/bin/cmake.exe` | `HIASM_CMAKE` |
| NSIS 3, только для упаковки | `makensis.exe` из PATH или Program Files (x86) | `HIASM_MAKENSIS` |

## Сборка и проверки

```bat
tools\build_windows.bat debug
tools\build_windows.bat release build-only
tools\build_windows_run-debug.bat check
tools\build_rtcg.bat
powershell -NoProfile -ExecutionPolicy Bypass -File tools/run_all_checks.ps1
```

После сборки и проверок BAT-скрипт запускает `hiasm.exe`. Аргумент `build-only` пропускает запуск. Общая проверка запускает тесты баз, CTest и старт IDE для Debug и Release, затем проверяет установленный Release с системным PATH. Ключ `-Python ПУТЬ` выбирает Python для проверки баз. Ключ `-SkipBuild` использует уже собранные деревья, `-SkipPortable` пропускает проверку установки. Старый аргумент `qt` в BAT-командах и `-QtOnly` в общей проверке принимаются для совместимости.

Скрипты и пресеты CMake `qt69-native-debug` и `qt69-native-release` создают `hiasm.exe` в каталогах `build/Windows_LLVM-MinGW-x64-Debug/` и `build/Windows_LLVM-MinGW-x64-Release/` соответственно. Профили VS Code и Zed используют эти же каталоги. Для нестандартного Qt SDK используйте переменные окружения в скрипте сборки. Абсолютные пути к комплекту в пресетах и профилях отладчика при этом меняются отдельно.

В Qt Creator можно открыть корневой `CMakeLists.txt` или `HiAsm.pro`. Для `HiAsm.pro` выберите комплект Desktop Qt LLVM-MinGW x64 и конфигурацию Debug или Release. В настройках сборки укажите `build/Windows_LLVM-MinGW-x64-Debug/qmake` и `build/Windows_LLVM-MinGW-x64-Release/qmake` соответственно. В этих подпапках qmake размещает `hiasm.exe`, библиотеку Core, RTCG и базы пакетов. Подпапки позволяют использовать обе системы сборки независимо. qmake вызывает `python` из `PATH` для генерации баз, выбрать другой интерпретатор можно через `HIASM_PYTHON`. Установка и тесты определены в CMake.

## Схемы с WebBrowser и Flash

В Windows используется тот же комплект Qt для LLVM-MinGW, что и для обычных C++-схем. Chromium предоставляется Microsoft Edge WebView2 Runtime. При первой сборке схемы HiAsm загружает зафиксированную версию SDK WebView2 из NuGet и сохраняет её в локальном кэше. Для сборки без сети задайте `HIASM_WEBVIEW2_SDK_DIR` с путём к распакованному пакету SDK, содержащему `build/native/include/WebView2.h`. На компьютере, где запускается собранная схема, нужен установленный Evergreen Runtime. [Развёртывание WebView2](https://learn.microsoft.com/en-us/microsoft-edge/webview2/concepts/distribution)

На Linux и macOS Chromium входит в Qt WebEngine. Установите расширение Qt WebEngine для используемого комплекта Qt. Для отдельного комплекта с той же архитектурой и ABI укажите `HIASM_WEB_QT_DIR`. Исходники сборки Chromium из SDK Qt находятся в `Src/qtwebengine/src/3rdparty/chromium`. [Требования Qt WebEngine 6.9](https://doc.qt.io/archives/qt-6.9/qtwebengine-platform-notes.html)

Для `Flash` HiAsm ищет `ruffle.js` и файлы WebAssembly в `thirdparty/ruffle/web/packages/selfhosted/dist/`. Соберите эту папку по инструкции Ruffle или скачайте официальный комплект командой `python tools/fetch_ruffle_web.py build/deps/ruffle`. Для комплекта в другом каталоге задайте `HIASM_RUFFLE_DIR`. Установка HiAsm через CMake включает локальный `dist`, если он доступен. При сборке схемы HiAsm встраивает Ruffle в исполняемый файл. [Сборка Ruffle из исходников](https://github.com/ruffle-rs/ruffle/tree/master/web#building-from-source)

Подключите `doPlay` к событию схемы и задайте `FileName` или подайте значение на одноимённую точку. Относительный путь разрешается от рабочей папки программы. При запуске из IDE это папка схемы. Поддерживаются локальный SWF и HTTP(S), если сервер разрешает загрузку через CORS. Локальный проигрыватель получает доступ к выбранному SWF. Для фильма с отдельными внешними ресурсами используйте доступный по HTTP(S) адрес. `doPause` переключает паузу, `doStop` перезагружает фильм в остановленном состоянии. `CurrentFrame` возвращает `-1`, а `doGoToFrame` выдаёт сообщение об отсутствии покадровой навигации в веб-API Ruffle. [API проигрывателя Ruffle](https://ruffle.rs/js-docs/master/interfaces/Player.PlayerV1.html)

`WebBrowser` возвращает в `Page` снимок HTML после завершения загрузки. Он доступен к моменту `onNavigate`. `CurrentURL` возвращает текущий адрес. `doSavePage` сохраняет HTML без отдельных ресурсов страницы.

F7 и F9 помещают результат в `build/qt-webview2/` в Windows и в `build/qt-webengine/` в Linux и macOS. Эти папки создаются рядом со схемой. В Windows HiAsm копирует туда DLL и плагины Qt, библиотеки LLVM-MinGW и `WebView2Loader.dll`. При вызове `hiasm --build` задайте путь к EXE в отдельной папке. Для распространения используйте Release и передавайте всю папку.

## Каталоги и установка

CMake пересобирает базы четырёх пакетов из `elements.sql` и регистрирует C++ в копии главной базы. Сборка пишет только в выходной каталог. Установка включает эти базы, исходные данные пакетов, RTCG для `cpp`, `web` и `modules`, приложения и плагины Qt.

```powershell
.\tools\build_qt_windows.ps1 -Configuration Release -InstallDir C:\Temp\HiAsm
.\tools\smoke_windows.ps1 -InstallDir C:\Temp\HiAsm
```

Точка входа — `qt/hiasm.exe` внутри установки. Для сборки схем из такой копии нужен Qt SDK с компилятором. Рядом с `qt/` находятся `packs/` и `int/`. Проверка запуска открывает поставляемую схему в режиме offscreen с очищенными путями к SDK и плагинам.

## Установщик NSIS

```bat
tools\build_windows.bat installer
```

Скрипт создаёт отдельный каталог в `build/windows-installer/`, выполняет установку CMake и проверяет её запуск. В `packs/cpp/compiler/` добавляются LLVM-MinGW, Qt SDK, CMake и Ninja для сборки проектов на компьютере пользователя. NSIS создаёт `build/HiAsm_Setup_Qt.exe`. Вариант для компьютера с уже установленным SDK можно собрать через `tools/build_windows_installer.ps1 -RuntimeOnly`.

Версия Qt устанавливается в `Program Files\HiAsm Studio Qt` и использует собственный ключ реестра. Проверка подготовленного каталога и запуск установленного через NSIS продукта — разные сценарии.

## Распространение собранной программы

Для передачи программы на другой компьютер можно собрать её DLL в отдельную папку. В следующей команде `--runtime` указывает на комплект компилятора из полного установщика, а `--output` — на новую папку.

```powershell
py -3 tools/package_cpp_windows.py --executable Project.exe --runtime "C:/Program Files/HiAsm Studio Qt/packs/cpp/compiler" --output Project-portable
```

Для приложения Qt скрипт вызывает `windeployqt`. В результат входят нужные DLL, плагины и `distribution.json` с контрольными суммами. `tools/run_all_checks.ps1 -Package -CppRuntime ПУТЬ` дополнительно проверяет создание и автономный запуск такой поставки.

## Историческая реализация

Код прежней IDE и её тесты сохранены для сверки поведения при переносе. Их отдельная точка конфигурации — `cmake/gtk-reference/`. История платформенных исправлений находится в [отчёте GTK4](GTK4_RUNTIME_FIXES.md). Для повседневной разработки Qt используется корневой CMake-проект.

Сценарии `build_gtk4_runtime.ps1`, `check_gtk4_contract.ps1`, `deploy_gtk_runtime.ps1` и `update_cpp_runtime.ps1` относятся к прежнему рантайму. Сценарии WSL/Wine описывают кросс-сборку прежних проектов. Нативный компилятор схем Qt собирает для текущей платформы.

## Проверенные сценарии Windows

На Windows 21 ноября 2026 года в Debug и Release прошли все 13 тестов CTest. Сквозные тесты вызывают F9 из IDE для консольной программы, формы C++ и формы Core. Редактор форм проверяется перетаскиванием, изменением размера, отменой и переключением темы. Для этих тестов используется платформа Qt `offscreen`. Переключение тёмной и светлой палитр также проверено с плагином `windows`.

В Qt Creator комплект Desktop Qt 6.9.3 LLVM-MinGW x64 собирает `HiAsm.pro` в обеих конфигурациях. Исполняемые файлы находятся в подпапках `qmake/` соответствующих каталогов Debug и Release. Проверка `--check` открывает `QtForm.sha`, а `--build` собирает `hello.sha`.

Проверка переносимой установки также открывает `QtForm.sha`. Отдельная команда `-Package` проверяет запуск собранной программы с DLL из собственной папки.

Полный установщик распакован и проверен в каталоге с пробелами и кириллицей. IDE открыла `QtForm.sha`, а компилятор из комплекта собрал и запустил `hello.sha` при системном `PATH`. При распаковке под системным `%TEMP%` на этой машине Windows отказала в запуске `clang-17.exe`. Интерактивная установка и удаление через NSIS на чистой Windows ещё требуют проверки.
