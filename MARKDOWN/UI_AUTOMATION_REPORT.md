# Проверка управления интерфейсом

Дата: 2026-11-02.

Windows-MCP 0.8.8 установлен в `C:/Users/Admin/.codex/mcp/windows-mcp-venv`, зарегистрирован в конфигурации Codex. Доступный драйвер Computer Use использует `@oai/sky`.

Подтверждено обнаружение HiAsm, выбор окна и чтение дерева доступности: системные кнопки Win32, меню, toolbar, вкладки, доступность команд.

Ограничения сессии:

- Скриншоты: `window capture timed out: timed out waiting on channel`.
- Щелчок: `coordinate input geometry is unavailable`.
- Ctrl+N остановлен: `user input was detected in this window; call get_window_state before continuing`. Команда не считается проверенной.
- Пользователь работает в окне, состояние меняется независимо от агента.

Скриншотная сверка, мышь, drag/drop, положение подсказок и диалоги остаются открытой приёмкой. CTest проверяет программные сценарии, но не визуальное соответствие HiAsm 4.

## Дополнение 2026-11-02

- Portable runtime `build/qa/final-portable/hiasm5.exe` повторно обнаружен как единственное окно; accessibility tree содержит native Win32 minimize/maximize/close, меню, toolbar, tabs, editor canvas и native frame controls.
- `get_window_state(include_screenshot=true)` снова завершился `window capture timed out: timed out waiting on channel`; после этого использовалась только свежая accessibility observation. Coordinate click вернул `coordinate input geometry is unavailable`, а попытка действия после пользовательского ввода получила `user input was detected in this window`. Поэтому мышь, drag/drop и визуальное соответствие Windows не отмечены выполненными.
- Для Linux GTK4 добавлен `tools/test_linux_ui.sh`; Xvfb/Openbox smoke получил `build/qa/linux-ui-startup.png`, `linux-ui-new.png`, `linux-ui-open.png`. Первоначальный прогон выявил отсутствие общих shortcut вне canvas. После исправления оконного capture-controller Ctrl+N открыл Project manager, Ctrl+O — Open file; это подтверждено списком окон и журналом command: new/open. Скрипт теперь завершается с ошибкой, если диалоги не появились.

Повторный Windows capture после получения свежего Window из `list_windows` снова вернул `window capture timed out: timed out waiting on channel`. Обход через неподтверждённые координаты не выполнялся. Linux smoke дополнен сценарием редактирования текста и native close: ожидание Warning, отмена и Discard; результат этого сценария фиксируется в `build/qa/close-linux-ui.log`.

## Проверка закрытия и остановка Computer Use (2026-11-03)

Linux X11 smoke выполнен успешно: New/Open, редактирование временного текстового файла, Alt+F4, Warning, Escape/Cancel, повторное закрытие и Discard. Процесс остаётся после Cancel и завершается после Discard; исходный файл не перезаписан. Журнал: `build/qa/close-linux-ui-trace.log`; снимок диалога: `build/qa/linux-ui-close-warning.png`. Окно ищется по итоговому заголовку, чтобы не сохранить XID стартового splash.

Повторный вызов Windows Computer Use вернул: `Computer Use was stopped by the user with the physical Escape key`. После этого управление окнами прекращено. Windows-скриншоты, мышь и визуальная сверка остаются непроверенными. Linux PNG получены, но изображение не было передано модели просмотрщиком, поэтому они также не отмечены как визуально сверенные.

## Автоматические проверки 2026-11-03

Windows-MCP 0.8.8 установлен и зарегистрирован в Codex конфигурации. После
физического Escape от пользователя Computer Use остаётся остановленным.

Без мыши проверены программные Win32 IME lifecycle (500 контекстов за пять
повторов), оконные GTK smoke и cross-GUI сценарии. Это не ручная проверка
палитры, меню, диалогов и drag/drop; визуальная приёмка остаётся открытой.
