# Аудит команд HiAsm 5

Актуализация: 2026-11-02. Число «88 команд без обработчика» — исторический снимок.

| Команды | Состояние и проверка |
|---|---|
| undo, redo | Журнал схемы и редактора формы; текстовый GtkSourceView 5. scheme_history, form_editor, editor_commands |
| group, ungroup | Сериализация @Group, восстановление, remap при вставке. scheme_history |
| align, moveto | Шесть направлений, точные координаты, frozen selection. editor_commands |
| show_lh | Видимость и hit-test подсказок без изменения файла. editor_commands |
| searchnext | Циклический UTF-8 поиск в тексте. editor_commands |
| formedit | Встроенный для _base, внешний только при наличии helper. form_editor, editor_commands |
| ecreator, add_group | Создание конфигурации/исходника с откатом, prepared SQLite. pack_database |
| replaceto | Безопасная clone+swap замена переносит совместимые свойства, точки, связи и маршруты |
| bind_rect, bind_center, bind_padding | Настройки, сохранение, snapping и native toggle buttons реализованы |
| svn_update, svn_log, svn_show | Асинхронный GSubprocess-клиент; update требует перезапуска IDE |
| close, closeall, exit | Закрытие вкладок и preflight всех изменённых документов; Cancel/DELETE_EVENT/ошибка сохранения отменяют закрытие. Exit вызывает native close request, который также проверяет документы |
| activated_key | Отсутствует в текущей БД и исходниках |

Также работают выбор, буфер обмена, z-order, масштаб, файлы, сборка/запуск/остановка. CTest не заменяет визуальную приёмку всех меню и диалогов.

