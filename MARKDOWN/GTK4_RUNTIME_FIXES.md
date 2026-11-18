# Исправления GTK4 Win32 runtime

Проверено 2026-11-03. IDE и Windows-приложения C++17 используют локальную
GTK 4.24.1 с двумя исправлениями. GTK API и ABI не меняются.

## DnD startup

`tools/patches/gtk4-win32-dnd-startup.patch` проверяет наличие default display
перед обходом очереди drag/drop. Это устраняет обращение к отсутствующему
display при завершении Win32 DnD-потока.

## Жизненный цикл IME

Повторная генерация и выполнение GUI-примеров выявили ошибку
`gtk_im_context_ime_message_filter: GTK_IS_IM_CONTEXT_IME(data) failed`
и падение при уничтожении окон с GtkEntry. GDB показал display message filter,
ссылающийся на уже уничтоженный GtkIMContextIME.

`tools/patches/gtk4-win32-ime-lifecycle.patch` сохраняет display регистрации,
удаляет фильтр при detach, unrealize, dispose и focus_out и исключает повторную
регистрацию. Проверка `priv` обязательна: finalize базового GtkIMContext повторно
вызывает виртуальный set_client_widget после освобождения private-данных
производного IME. Ссылка на display освобождается вместе с регистрацией.

`tests/win32_ime_test.cpp` создаёт 100 IME-контекстов с повторным focus_in,
отсоединением виджета, уничтожением виджета до контекста и dispose без focus_out.
После освобождения отправляется WM_NULL в прежнее окно. CTest `win32_ime`
работает с `G_DEBUG=fatal-criticals`; пять повторов прошли. Это программная
проверка Win32/GTK lifecycle, без управления пользовательской мышью/клавиатурой.
Ранее падавший RTCG также прошёл три последовательных повтора.
Контроль с исходной DLL из MSYS2 в отдельном каталоге воспроизвёл ту же
Gtk-CRITICAL и выход -1073740791; исправленная DLL проходит этот тест.

Журналы: `build/qa/reactive-gdb.log`, `ime-unit-gdb.log`,
`ime-rtcg-repeat-tests.log`, `complete-ime-tests.log` и
`ime-original-regression.log`.

## Воспроизводимая сборка и поставка

`tools/build_gtk4_runtime.ps1` проверяет SHA256 исходного GTK-архива,
применяет patches и собирает GTK вместе с AccessKit C 0.18. Повторное применение
IME patch проверяется обратным dry-run. Проверены повторная конфигурация
и сборка с `-SourceDir build/qa/gtk-4.24.1 -BuildDir build/qa/gtk-patched-native`.

Канонические DLL: `build/deps/gtk4-win32/libgtk-4-1.dll` и
`libaccesskit-c-0.18.dll`. CMake staging/install и deploy сохраняют эти DLL,
исходный архив, оба patch, build script и лицензии. Для staged UCRT64 toolchain
`tools/update_cpp_runtime.ps1 -Toolchain ПУТЬ` обновляет те же DLL и
`licenses/local-gtk4`; `manifest.json.localRuntime` содержит SHA256.
`stage_cpp_toolchain.ps1` выполняет это при создании нового комплекта.

Автономный упаковщик `tools/package_cpp_windows.py` учитывает системную
`UIAutomationCore.dll`, импортируемую AccessKit, и отказывает при неизвестной
отсутствующей сторонней DLL. Проверки Windows и Wine запускают результат
вне toolchain с изолированными путями и проверяют SVG/иконки.

## Границы проверки

Тесты не доказывают работу всех раскладок IME, кандидатных окон или визуальное
соответствие HiAsm 4. Windows Computer Use остановлен пользователем; ручная
визуальная приёмка и чистая Windows остаются открытыми в CHECKLIST.md.
