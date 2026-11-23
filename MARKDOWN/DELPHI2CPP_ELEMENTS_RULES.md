# Правила переноса элементов HiAsm 4 (Delphi) в пакет C++

Дата: 2026-11-15. Рабочий документ. Источники: установка HiAsm 4 (`C:\Program Files (x86)\HiAsm\Elements\delphi`, только чтение), код `src/` и тесты `tests/`. Таблицы статуса генерируются скриптами: `MARKDOWN/DELPHI_MIGRATION.md` (`tools/audit_delphi_migration.py`) и `MARKDOWN/CPP_ELEMENTS_PLAN.md` (`tools/cpp_elements_plan.py`). Их не редактировать руками.

Каждое правило ниже подтверждено кодом или тестом, на который дана ссылка. Если правило расходится с кодом, верить коду и поправить этот файл.

## 1. Цель и границы

- Цель: элемент работает в пакете `cpp` так же по свойствам, точкам, данным и сохранению схемы, либо явно помечен как частичный аналог с перечнем недостающего.
- Пакет-получатель: `packs/cpp` (визуальные элементы и «Приложение на C++»); логика схемы `hc*` живёт в Core `packs/_base`.
- Pascal не переносится. Файлы `code/hi<Имя>.pas` служат справкой по поведению. Текст переписывается на RTCG, не переводится построчно.
- Совпадение имени метода или точки не доказывает совместимость. Так сформулировано и в аудите (`tools/audit_delphi_migration.py`).

## 2. Что переносим

| Категория в Delphi | Решение | Пример |
|---|---|---|
| Визуальный элемент (`[Type] Class=WinElement`, есть `[Edit] Class=`) | переносим в `packs/cpp`, код `code/<Имя>.hws` | Button, Label, Edit |
| Логика схемы (`hc*`, Core) | конфиг в `packs/_base/conf`, код в `packs/cpp/code/hc<Имя>.hws` (существующие файлы называются `hihc<Имя>.hws`) | hcMath, hcFor |
| Только среда разработки: Commander, AddElement, MoveElement, ConnectElement, SDKEvents, CommanHandler | не переносим; остаются в `planned_elements` с пометкой «только среда» | hcAddElement |
| WinCE | не переносим; группа удалена из базы | Ce* |
| Нет аналога в C++ и нет решения | остаётся в `planned_elements` | BASS_* |
| Частичный аналог | переносим, в аудите записываем PARTIAL и перечень недостающего | ListBox |

## 3. Соответствие файлов

| Delphi (`Elements/delphi`) | C++ (`packs/cpp`) | Примечание |
|---|---|---|
| `conf/<Имя>.ini` (CP1251) | `conf/<Имя>.ini` (UTF-8, BOM) | копия с английскими описаниями, раздел 5 |
| `code/hi<Имя>.pas` | `code/<Имя>.hws` | переписывается, раздел 8 |
| `icon/<Имя>.ico` | `icon/<Имя>.ico` | то же имя |
| описания на русском в самом конфиге | `lang/ru/conf/<Имя>.lang` | строки `английский=русский`, раздел 5.6 |
| `Example/**/*.sha` с `Make(delphi)` | `example/**/*.sha` с `Make(cpp)` | раздел 13 |
| строка в `Elements.sql` | `packs/cpp/elements.sql`: `elements` или `planned_elements` | раздел 9 |

Загрузчик выбирает кодировку по пакету: legacy-пакеты читаются как CP1251, пакет `cpp` — как UTF-8 (`src/ElementConfig.cpp`, `decodeText`).

## 4. Имена и класс

4.1. Имя элемента совпадает с именем Delphi (`Button`, `Label`). Core-логика — `hc<Имя>` как в Delphi (`hcMath`). Других префиксов не вводить.

4.2. `[Type] Class=` сохраняется как в Delphi. Классы в `packs/cpp` на 2026-11-15 (по `[Type]`): `Element` 651, `WinElement` 64, `DPElement` 22, `DPLElement` 19, `MultiElementEx` 7, `MultiElement` 6, `PoliMultiElement` 5, `VTElement` 4, `Cable` 4, `Hub` 2, `EditMultiEx` 2, `UseHiDLL` 2.

4.3. Класс обязан быть в `createElement` (`src/Elements.cpp`, около 80 имён). Класс, которого там нет, даёт голый `Element` без поведения. Такой элемент не компилируется в схеме (`Pack::compiles` в `src/Pack.cpp`). Новый класс — отдельное решение: правка `createElement` и тест.

4.4. `[Edit] Class=` — класс виджета для редактора форм. Это не `[Type] Class`. Раздел 10.

4.5. `Inherit=`, `Interfaces=`, `Info=`, `Icon=`, `Sub=`, `Tab=` сохраняются как в Delphi. `Info` пишется английским текстом: по нему строится ключ перевода (раздел 5.6).

4.6. Раздел `[About]` не менять без причины.

## 5. Конфиг `conf/<Имя>.ini`

5.1. Разделы: `[Type]`, `[Edit]`, `[Property]`, `[Methods]`. Строки `;` и `//` — комментарии. Внутри `[Property]` строка `##Имя` задаёт группу свойств в редакторе.

5.2. Строка свойства: `Имя=подпись|тип|значение|список` (`src/ElementConfig.cpp`, `parseProperty`).
- подпись — английская; символ `|` в ней не использовать;
- тип — номер из таблицы 5.4;
- значение пустое — значение не задано; `#13#10` в значении заменяется на перевод строки;
- список нужен только для типов 4, 14, 20, 21: пункты через `|`.

5.3. Префиксы строки свойства и точки:

| Префикс | Смысл | Где видно |
|---|---|---|
| `+` | основное свойство (`VF_DBLOPEN` → `PROP_FLG_MAIN`) | жирным в редакторе свойств |
| `@` | свойство может стать точкой (`VF_MAKEPOINT`) | флажок в редакторе; точка `do<Имя>`; в схеме строка `Point(do<Имя>)` |
| `*` перед именем точки в `[Methods]` | необязательная точка | создаётся только строкой `Point(имя)` в схеме (`src/SDK.cpp`, `Element::savePointsToText`) |

5.4. Типы данных (`src/share.h`, enum `DataType`):

| Номер | Имя | Пример |
|---|---|---|
| 1 | int | `Width` |
| 2 | str | `Caption` |
| 3 | data | `Data` |
| 4 | combo | `Alignment` (список) |
| 6 | icon | `Icon` в Delphi |
| 8 | color | `Color` |
| 11 | bitmap | `Picture` в Delphi |
| 14 | comboEx | `True,False` |
| 15 | font | `Font` |
| 20 | element | `TransparentManager` |
| 21 | flags | флаги |
| 23 | pixbuf | `Icon`, `Picture` в `packs/cpp/conf/MainForm.ini` |

5.5. Наследование. `Inherit=WinControl` подтягивает свойства из `packs/cpp/conf/WinControl.ini` (`Left`, `Top`, `Width`, `Height`, `Align`, `Color`, `Font`, `Visible`, `Enabled`, `Cursor`, `Hint` и др.). Переопределить наследуемое свойство можно строкой `Имя= | |значение`: тип пустой. Переопределять можно только существующее свойство, иначе загрузка падает с ошибкой `override of unknown property`.

5.6. Перевод. Конфиг в `packs/cpp` содержит английские подписи. Русский текст лежит в `lang/ru/conf/<Имя>.lang` в виде `английский текст=русский текст`. Пример `Button.lang` начинается строкой `Button=Кнопка`, где ключ совпадает со значением `Info`. Строки без перевода показываются по-английски.

5.7. Значения цвета. Пустой `Color` даёт чёрный цвет. У `Button` в `packs/cpp/conf/Button.ini` стоит `Color= |0|`, поэтому в редакторе фон был чёрным; это обработано на стороне редактора (`colorOf` в `src/LegacyFormEditor.cpp`). Для новых элементов задавать цвет по умолчанию именем Delphi (`clBtnFace`, `clWindow`): такие имена разбираются при загрузке конфига (`ElementProperty::initFromText`). Пользовательский ввод имена Delphi не принимает — это проверяет `tests/property_value_test.cpp`.

5.8. Валидность. Конфиг, который не разбирается, получает `valid = false` (`src/ElementConfig.cpp`). Тип свойства должен быть в диапазоне `0..data_count-1`, тип точки — `1..4`. Тест `cpp_package` сообщает такой элемент как `invalid-config`.

## 6. Точки (`[Methods]`)

6.1. Строка точки: `имя=подпись|тип точки|тип данных`.

6.2. Тип точки (`src/share.h`): `1` — `pt_work` (действие, `do<Имя>`), `2` — `pt_event` (событие, `on<Имя>`), `3` — `pt_var` (переменная), `4` — `pt_data` (точка данных).

6.3. Тип данных — номер из таблицы 5.4 или имя `str`, `int`, `arr`, `data`. Пусто — без данных (`pointDataType`, `src/ElementConfig.cpp`).

6.4. Соответствие Delphi:

| Delphi | Конфиг `packs/cpp` | Код `.hws` |
|---|---|---|
| `_work_doX` | `doX=подпись\|1\|` | `func doX(_data)` |
| `_event_OnX` | `onX=подпись\|2\|тип` | `linked(onX)` и `event(onX, Data)` |
| `_data_X` | `X=подпись\|4\|тип` | `d('X')` |
| `_prop_X` | строка в `[Property]` | `X` или `get('X')` |
| `_var_X` | `X=подпись\|3\|` | — |

6.5. Каждая точка Delphi с типом действия, события или данных должна быть в конфиге `packs/cpp` с тем же именем и тем же типом точки. Новые точки добавлять в конец списка.

6.6. Точка со звёздочкой (`*`) не создаётся по умолчанию. Схема получает её строкой `Point(имя)`, и только тогда её можно подключить. Если удалить такую точку из конфига, схемы с `Point(имя)` перестают загружаться.

6.7. Один провод на точку (`ElementPoint::canConnect`). Веер (одна точка данных, несколько потребителей) делается отдельными элементами, например `HubEx`/`GetDataEx`.

6.8. Связи в файле схемы: `link(точка,id:точка,[…])`, точки — `Point(имя)`. Это формат `src/SDK.cpp`, его менять нельзя.

## 7. Сохранение схем и совместимость

7.1. Схема хранит свойства по имени: `Имя=значение`. Системные свойства — `@Имя=значение` (`src/Element.cpp`). Комбобокс хранит числовой индекс пункта (`ElementProperty::serialize`).

7.2. Без миграции нельзя:
- переименовать свойство или точку;
- менять тип существующего свойства (например, `Picture` 11 → 23);
- переставлять или удалять пункты списка: индекс в сохранённой схеме начнёт означать другой пункт;
- менять значение по умолчанию, если от него зависит сгенерированный код: схемы с этим значением поменяют поведение молча.

7.3. Можно: добавить свойство или точку в конец, добавить пункт в конец списка, уточнить описание.

7.4. Любое отклонение от Delphi записывать в раздел 13.

7.5. При замене дубликата старые схемы должны загружаться. Так проверяет `tests/cpp_package_test.cpp` (`old schemes with … no longer load`).

## 8. Код `code/<Имя>.hws`

8.1. Имя файла совпадает с конфигом: `code/Button.hws`. Старое имя `code/hiButton.hws` читается (`Pack::pathElementCode`), но новые файлы пишутся без префикса `hi`. Для Core-элементов новые файлы — `code/hc<Имя>.hws`.

8.2. Структура шаблона (образцы: `code/hiButton.hws`, `code/hiLabel.hws`, `code/hiEdit.hws`):
- `func doStart` выводит C++ для создания элемента через `println(...)`;
- `func doX(_data)` для каждой точки действия;
- `linked(onX)` проверяет, подключено ли событие, `event(onX, Data)` выводит его значение в код.

8.3. Значения:
- свойство — по имени: `Alignment`, `Left`, `Top`;
- `get('Имя')` — значение свойства, когда имя передаётся строкой (`get('StockIconSize')`);
- `d('Имя')` — значение точки данных. Если точка подключена, значение берётся от связанного элемента. Иначе — свойство с таким именем. Иначе — входящие данные контекста (`map_d`, `src/RTCG/IntFunc.cpp`);
- строки в C++ всегда через `cstr(...)`, иначе не экранируются кавычки.

8.4. Размещение: `println('hiasm_place("Button', _id_, '", button, ', Left, ', ', Top, ');')` (образец: `code/hiButton.hws`). Строка C++ получает имя элемента с номером `_id_`, виджет, `Left` и `Top`.

8.5. Общий код берётся из прелюдии `packs/cpp/code/MainForm.hws`. Там объявлены `hiasm_place`, `hiasm_set_color`, `hiasm_set_font`, `hiasm_set_default`, `hiasm_set_background`, `hiasm_set_border`, `hiasm_list_*`, `hiasm_combo_*`, `hiasm_text_*`, `hiasm_ini_*`, `hiasm_timer`, `hiasm_win_*` и другие. Повторять такой код внутри элемента нельзя: добавлять в прелюдию.

8.6. Только GTK4: `gtk_box_append`, `gtk_button_set_child` и так далее. Устаревшие API (например, `GtkComboBoxText`) допускаются, только если заменить их нечем, и с пометкой.

8.7. Код, который работает только в Windows, оборачивается в `#if defined(_WIN32)`. Ветка для Linux остаётся явным no-op с комментарием. Образец: `packs/cpp/code/MainForm.hws` (11 таких блоков).

8.8. Грабли RTCG: `include` — служебное слово; тернарный оператор с `==` внутри присваивания не разбирается, пишите `if`; `d(...)` снимает данные при каждом вызове, читайте в переменную один раз.

## 9. Иконка, база элементов, палитра

9.1. Иконка элемента — `icon/<Имя>.ico`. Заглушка `packs/_base/icon/nil.png` не допускается: её видит тест `cpp_package` как `placeholder-icon`. Иконки Core — `.png` (`packs/_base/icon/hcButton.png`). Функция `_iconPath` (`src/Pack.cpp`) ищет `.ico`, затем `.png`.

9.2. `packs/cpp/elements.sql` (источник базы):
- `groups(id, name, info, pos)`: id 1 Widgets, 3 Controls, 16 Helpers, 35 RTCG, 36 Core, 15 Tools; id 21 — скрытая группа с пустым именем. Палитра её не показывает;
- `elements(id, name, info, tab, pos, hash)`: `tab` — id группы, `pos` — порядок в группе (при добавлении элемента в базе берётся максимум `pos` группы, `src/Pack.cpp`). Приложение читает `name, info, tab, pos`. Колонку `hash` приложение не читает. Для новых строк её можно оставить пустой, как у `hcTplButton`;
- `planned_elements(name, info, group_name, icon)`: элементы, у которых есть конфиг, но нет кода. При реализации строка переносится в `elements`, из `planned_elements` удаляется.

9.3. Элемент попадает в палитру, если выполнены оба условия. Во-первых, он стоит в `elements`, а не в `planned_elements`, и его группа не скрытая (id 21). Во-вторых, `Pack::compiles(name)` возвращает true: есть `code/<Имя>.hws` (или `code/hi<Имя>.hws`), либо класс `HubEx`, `GetDataEx`, `ITElement` (`src/Pack.cpp`). Конфиг без кода — это план, а не реализация.

9.4. База пересобирается сборкой проекта: `cmake/StageRuntime.cmake` выполняет `sqlite3 elements.db ".read elements.sql"` в каталоге сборки. Вручную тот же результат: `sqlite3 elements.db ".read elements.sql"` в копии каталога. Проверено на временной копии: таблица `planned_elements` создаётся, строк 654 (план) и 203 (реализовано).

9.5. `tools/cpp_elements_plan.py` читает `packs/cpp/elements.sql` напрямую (в памяти SQLite), поэтому устаревший `elements.db` не влияет на `MARKDOWN/CPP_ELEMENTS_PLAN.md`. Тест `cpp_package` читает `planned_elements` по колонкам `name, info, group_name, icon` (`tests/cpp_package_test.cpp`); схема таблицы должна совпадать с этим запросом.

## 10. Редактор форм

10.1. Ключи `[Edit]` связывают роли редактора с именами свойств: `Caption=Caption`, `Color=Color`, `Font=Font`. Функция `mapped()` (`src/LegacyFormEditor.cpp`) ищет свойство по этой роли.

10.2. Встроенный редактор (режим по умолчанию, `WidgetEditorEx`, `src/FormEditor.cpp`). `ElementWinElement::createWidget` (`src/Elements.cpp`) создаёт виджет по классу `[Edit]`: Button, Label, Edit, CheckBox и RadioButton, ComboBox, Progress; GroupBox, Panel и ScrollBox — рамка; Memo, ListBox и TreeView — окно с прокруткой; остальные классы — рамка с именем класса.

10.3. Окно «Отдельное окно» (`FE_Legacy`, `src/LegacyFormEditor.cpp`, `drawControl`) рисует Button, Label, Edit, Memo, ListBox, ComboBox, CheckBox, RadioButton, GroupBox, Panel, Progress, ScrollBar, TrackBar, Tab, TreeView, ScrollBox. Остальные классы — пунктирная рамка с именем шаблона.

10.4. Визуальный элемент, которого нет в списках 10.2 и 10.3, даёт рабочую рамку-заглушку. Это временная мера: записать её в раздел 13.

## 11. Перенос одного элемента: порядок шагов

1. Найти элемент в `MARKDOWN/DELPHI_MIGRATION.md` и посмотреть статус и PARTIAL-запись. Если записи нет, читать `Elements/delphi/conf/<Имя>.ini`.
2. Скопировать конфиг в `packs/cpp/conf/<Имя>.ini`. Сохранить имена, типы, порядок свойств и точек, `[Edit]`, `Inherit`, `Interfaces`. Описания перевести на английский.
3. Добавить `packs/cpp/lang/ru/conf/<Имя>.lang` с парами `английский=русский` для всех описаний и для `Info`.
4. Написать `packs/cpp/code/<Имя>.hws` по разделу 8. Файл `.pas` не копировать.
5. Положить иконку `packs/cpp/icon/<Имя>.ico`. Если её нет, взять `Elements/delphi/icon/<Имя>.ico`.
6. Для визуального элемента проверить раздел 10: класс `[Edit]` должен попадать в один из списков или осознанно давать заглушку.
7. Добавить строку в `elements` в `packs/cpp/elements.sql` и удалить строку из `planned_elements`.
8. Если в Delphi есть пример схемы, положить его в `packs/cpp/example/` с `Make(cpp)`.
9. Добавить тест: кейс в `tests/cpp_hc_test.cpp` (для `hc*`) или проверку в `tests/cpp_widgets_test.cpp` (для визуальных). Затем запустить `ctest -R "cpp_package|cpp_hc|cpp_widgets|cpp_entries|rtcg|form_editor|new_project"`.
10. Если элемент частичный, добавить запись PARTIAL в `tools/audit_delphi_migration.py` и перегенерировать `DELPHI_MIGRATION.md`.
11. В отчёте написать, что проверено и что нет (раздел 12).

## 12. Что считается готовым

12.1. Статусы: «не перенесён» → «частичный аналог» → «полностью совместим».

12.2. «Полностью совместим» ставится только после проверки свойств, событий, данных, сохранения схемы и рабочего примера на Windows и на Linux. Пока это не сделано, в `DELPHI_MIGRATION.md` стоит «полностью совместимых: 0». Скрипт `tools/audit_delphi_migration.py` пишет это число как константу (строка 135), поэтому при появлении проверенного элемента скрипт нужно поправить, а не редактировать его отчёт руками.

12.3. Частичный аналог: PARTIAL в `tools/audit_delphi_migration.py` перечисляет, что работает и что осталось. Образцы: `Button`, `Label`, `Edit`.

12.4. Не писать «работает», если тест не запускался или окно не смотрели. Писать «не проверено глазами».

12.5. Тесты, которые опираются на эти правила:

| Тест | Что проверяет |
|---|---|
| `cpp_package` | палитра: конфиг валиден, иконка не заглушка, элемент создаётся и имеет точку; план не дублирует реализованное; старые схемы грузятся при замене дубликата |
| `cpp_hc` | схемы из `hc*` собираются RTCG, компилируются g++, вывод совпадает с поведением Core |
| `cpp_widgets` | свойства WinControl из Delphi попадают в сгенерированный GTK-код и компилируются |
| `cpp_entries` | пять типов проектов собираются, точки входа экспорта на месте |
| `rtcg` | генератор и `new/MainForm.sha` собираются и запускаются (код возврата 77 — пропуск) |
| `new_project` | шаблоны `new/*.sha` загружаются, форма открывается, сборка проходит |
| `form_editor` | виджеты редактора формы C++ создаются с подписями из Caption, у формы заголовок |
| `property_value` | разбор значений: имена цветов Delphi из конфига принимаются, из пользовательского ввода — нет |

## 13. Известные отклонения и проблемы (на 2026-11-15)

| Что | Где | Что делать |
|---|---|---|
| `MainForm`: `Icon` и `Picture` имеют тип 23 (pixbuf), в Delphi 6 и 11; `TransparentColor` по умолчанию `#FFFFFF` вместо `clWhite` | `packs/cpp/conf/MainForm.ini` | сознательное отличие; загрузку старых схем не проверяли |
| Другие конфиги с изображениями используют тип 11 | `packs/cpp/conf/*.ini` (10 файлов) | выбрать один тип для одного понятия; менять только с миграцией (раздел 7) |
| `Edit`: и `@MaxLenField` (как в Delphi), и `@MaxLength` (добавлен в cpp). В скриптах используется только `MaxLength` | `packs/cpp/conf/Edit.ini`, `packs/cpp/code/hiEdit.hws` | решить, какое свойство остаётся; второе — с миграцией |
| `Button`: `Color= \|0\|` — пустой цвет по умолчанию | `packs/cpp/conf/Button.ini` | не менять без проверки `cpp_hc` и редактора (раздел 5.7) |
| В `packs/cpp/example` 7 схем всё ещё с `Make(delphi)`: `Files/DragAndDrop.sha`, `Files/Finder.sha`, `Files/FindFile.sha`, `Files/RepairData.sha`, `Files/StreamArray.sha`, `Files/TempFile.sha`, `Internet/TCP_Chat/lh-server.sha` | `packs/cpp/example` | заменить на `Make(cpp)` и проверить, что схема собирается |
| В `packs/cpp/example` 40 схем без строки `Make` | `packs/cpp/example` | проверить каждую: нужна ли строка, и какой пакет ожидается |
| Палитра: в Delphi «Панель» (`ChildPanel`) и «Дочерняя форма» (`ChildForm`) — контейнеры вложенной схемы (`Class=MultiElement`, `Sub=Panel` / `Sub=MainForm`), в `packs/cpp` они были рамкой и отдельным окном без вложенной схемы | `packs/cpp/elements.sql`, `src/Elements.cpp` (`ElementMulti`), `src/RTCG/IntFunc.cpp`, `packs/cpp/code/MainForm.hws`, `packs/cpp/code/hiPanel.hws` | палитра: `ChildPanel` и `ChildForm` в группе Widgets, двойной клик входит в схему; `Panel` стал скрытым внутренним элементом (группа без имени). Класс `MultiElement` без `[Edit]` получает `EditMulti`. Связи проходят через рамку (`rtcgRealLink`). Проверено кодогенерацией (`cpp_hc`: `form_child_panel`, `form_child_form`), `cpp_package`, `nested_scheme` (двойной вход, путь, «Редактор формы» в корне, внутри панели и внутри дочерней формы; раньше падал на корне; поле панели по её Width x Height, элементы, добавленные в панель при открытом редакторе, видны на поле). Снимок редактора с панелью и дочерней формой смотрели; двойной клик мышью и вид в работающем окне не проверены |

## 14. Источники в коде

- Парсер конфига и типы точек: `src/ElementConfig.cpp` (`parseProperty`, `parseMethod`, `pointDataType`, проверка диапазонов), `src/ElementConfig.h` (`VF_MAKEPOINT`, `VF_DBLOPEN`).
- Типы данных: `src/share.h` (`DataType`, `pt_work`…`pt_data`).
- Классы элементов: `src/Elements.cpp` (`createElement`, `ElementWinElement`).
- Пакет и палитра: `src/Pack.cpp` (`compiles`, `pathElementCode`, `_iconPath`, чтение групп и планов).
- Схемы и связи: `src/SDK.cpp`, `src/Element.cpp` (сохранение `name=`, `@name=`, `Point(...)`, `link(...)`).
- RTCG-функции: `src/RTCG/IntFunc.cpp` (`map_d`).
- Редакторы форм: `src/FormEditor.cpp`, `src/LegacyFormEditor.cpp`.
- Тесты: `tests/cpp_package_test.cpp`, `tests/cpp_hc_test.cpp`, `tests/cpp_widgets_test.cpp`, `tests/form_editor_test.cpp`, `tests/property_value_test.cpp`.
- Инструменты: `tools/audit_delphi_migration.py` (таблица PARTIAL; поиск конфигов сначала в пакете, затем в `packs/_base`; перегенерация 2026-11-15 дала файлы, идентичные закоммиченным `DELPHI_MIGRATION.md` и `.json`), `tools/cpp_elements_plan.py` (читает `packs/cpp/elements.sql` напрямую).
