# Приложение на C++

Пакет C++17 использует RTCG и GCC/MinGW. Типы: console, GTK4 GUI, shared library и daemon. `BuildMode` стартового элемента выбирает Debug (`-O0 -g`) или Release (`-O2 -DNDEBUG`).

Выход: `build/windows-mingw_x64-debug|release` на Windows и `build/linux-x64-debug|release` на Linux. Библиотека экспортирует `hiasm_run`, её запуск из IDE отключён. GUI требует GTK4 и pkg-config. Windows daemon — скрытый процесс, не Windows-служба; Unix daemon использует fork/setsid.

Windows installer поставляет GCC и GTK4 development files в `toolchains/mingw64`. `TargetPlatform` выбирает Native/Windows/Linux; доступны сборка и запуск через WSL либо MinGW/Wine. Подготовка окружения и ограничения GUI cross libraries описаны в `docs/cpp17.md`.

## Связь с пакетом Core

Элементы `hc*` и общие элементы (`InfoTip`, `HubEx`, `GetDataEx`, `PointElement`, `EditMulti`, `EditMultiEx`) пакет берёт из `packs/_base`, как HiAsm 4 берёт их для пакета Delphi: конфигурация, иконка и перевод `lang/ru/conf` ищутся сначала в пакете C++, затем в Core. Поэтому в `packs/cpp` лежат только собственные файлы: код `code/hi*.hws`, палитра `elements.sql`, отличающийся `conf/TplWinControl.ini` и `lang/ru/conf/Code.lang`. Регистрация проектов и компилятора находится в `resources/hiasm.sql`. Побайтные копии файлов Core не хранятся: тест `cpp_package_test` отклоняет их.

## Приложение с формой (GTK4)

Проект `MainForm` открывает окно GTK4 с поверхностью `GtkFixed`. Контролы схемы создаются при запуске по `Left/Top/Width/Height`, обработчики событий подключаются после того, как созданы все контролы. Встроенный редактор формы открывается командой «Редактор формы» (внешний `formedit` не нужен). Контролы с именами Delphi лежат в группе «Widgets» (Button, Label, Edit, Memo, CheckBox, RadioButton, ListBox, ComboBox, ProgressBar, TrackBar, GroupBox, Panel), Timer и Message — в «Controls». Одноимённые шаблоны `hcTpl*` перенесены в скрытую группу (пустое имя): палитра её не показывает, но схемы с ними открываются и собираются.

## База элементов

`elements.sql` (UTF-8) — перенос базы Delphi HiAsm 4. Группы: «Widgets», «Controls», «Helpers» с реализованными элементами; **RTCG** — собственные элементы C++17 (разделы «Проекты», «Логика», «Строки», «Вывод», «Codegen», «GTK4»). Элементы используют короткие имена (`Entry`, `Gui`, `Data`, `Counter` и т. д.) с конфигурациями и генераторами `conf/*.ini`/`code/hi*.hws`. **Core** — элементы `hc*` (инструменты, строки, массивы, файлы, сеть, базы данных, интерфейс, среда). Группа WinCE не переносится, её иконки удалены. Остальные группы Delphi (Graphics, System, Media и т. д.) в палитре пусты: заголовок появляется, только если в нём есть элемент с кодом.

Элементы Delphi без реализации лежат в таблице `planned_elements` (имя, описание, группа, секция, имя файла иконки в `icon/`, номер в базе Delphi, заметка). План с кандидатами строит `python tools/cpp_elements_plan.py` в `MARKDOWN/CPP_ELEMENTS_PLAN.md`. Когда у элемента появится код, перенесите его строку из `planned_elements` в `elements`.

Ограничения: GroupBox и Panel — рамки, а не контейнеры; все RadioButton формы образуют одну группу. Timer, Message и Clipboard требуют окна: в консольном проекте сборка завершается ошибкой. Элементы без C++-кода (Image, FileChooserButton, ColorButton, вложенные контейнеры, диалоги открытия и сохранения, HTTPText, базы данных, элементы среды) скрыты из палитры, а схема с ними не собирается: сообщение перечисляет такие элементы. Core-элементы `hc*` (строки, массивы, математика, циклы, файлы и т. д.) компилируются RTCG; проверки находятся в `cpp_hc_test`, палитру проверяет `cpp_package_test`.

## Типы проектов

При создании схемы C++-пакет предлагает `Entry`/`Console`, `Gui`, `MainForm`
(открывается по умолчанию), `Library`, `Daemon`/`Service`, `WinDll`, `ComExt`,
`WinService` и `CplApp`. `Plugin` зарегистрирован, но скрыт в диалоге «Новый
проект»: IDE пока не умеет загружать плагины.

- `Plugin` — кроссплатформенная библиотека с контрактом ABI 1
  (`hiasm_plugin_info`, `hiasm_plugin_run`).
- `WinDll`, `ComExt`, `WinService`, `CplApp` — Windows-only: по умолчанию
  собираются для Windows, а при явной Linux-цели исходник содержит `#error`.
- `MainForm` и `Gui` создают GTK4-приложение, `Library` экспортирует
  `hiasm_run`, консольные типы используют `main`, фоновые — `WinMain` на
  Windows и `fork/setsid` на Unix.

## Состав

Палитра содержит 24 активных RTCG-элемента: четыре точки входа, вывод, данные, арифметика, условие, hub, цикл, строки, память, счётчик, действие C++17, блокирующая задержка, GTK4 box/button/label/entry/checkbox/progress, неблокирующий таймер и выход. Это не полный перенос 795 Delphi-элементов. Кодогенерация, сборка и запуск проверяются `rtcg_test`; cross-проверки включаются переменными `HIASM_TEST_CROSS`, `HIASM_TEST_WINE` и `HIASM_TEST_STOP_CROSS`. `core_state.sha` проверяет типизированную память и счётчик; GTK4-контролы покрыты пробами `chat`, `panex` и `std` в `packs/_base/example/plugs`. Для объединения связей используются базовые HubEx/GetDataEx. Bundled Windows UCRT64-комплект можно использовать из Linux через Wine; настройка и автономная упаковка результата описаны в `docs/cpp17.md`.

`Code` добавляет действие C++17: `Code` — операторы, `Expression` — выражение результата, `input` — входное значение (`std::string`, если события без данных), `onResult` — событие с результатом. Компонент зарегистрирован в палитре вместе с остальными элементами RTCG. Учебные адаптации Core находятся в `packs/_base/example/tutorial` и `packs/_base/example/plugs`; перед сборкой установленного примера сохраните копию в свою папку.
