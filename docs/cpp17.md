# Приложение на C++

Выберите пакет «Приложение на C++» и шаблон: консоль, GTK4 GUI, библиотека
или фоновый процесс. Пакет использует RTCG и GCC с C++17.

Учебные адаптации HiAsm 4 находятся в `packs/_base/example/tutorial`
и `packs/_base/example/plugs`; они также используют `Make(cpp)`.
Локализованная приветственная схема находится в
`packs/_base/lang/ru/example/tutorial/intro.sha`, а базовая — в
`packs/_base/example/tutorial/intro.sha`. Это документы Core, их не нужно
собирать или запускать.
Перед изменением установленного примера сохраните копию в свою папку.

`Code` выполняет C++17-код свойства `Code` в локальной области
действия при событии `doExecute`. `input` содержит входной `HiasmValue`;
после кода вычисляется `Expression` (по умолчанию `input`),
а результат передаётся в `onResult`. Для указателей виджетов этот вход
не предназначен. Компонент позволяет показать учебные алгоритмы,
для которых ещё нет отдельного визуального компонента.

Примеры в `packs/cpp/example`:

- `hello.sha` — вывод строки;
- `control_flow.sha` — арифметика, условие, цикл, строки и UTF-8;
- `core_state.sha` — память, счётчик, динамические операнды и остановка цикла;
- `math_functions.sha` — тригонометрия в градусах, логарифм, ошибка и сброс результата.

У родительского элемента доступны `BuildMode` (Debug/Release) и
`TargetPlatform` (Native/Windows/Linux). Результат находится рядом со схемой
в `build/windows-mingw_x64-debug|release` либо `build/linux-x64-debug|release`.
Библиотека экспортирует `hiasm_run`; для её запуска нужен вызывающий процесс.
Фоновый шаблон в Linux отделяется от терминала; в Windows создаётся GUI-subsystem
процесс. Регистрация службы ОС в шаблон не входит.

Установщик Windows поставляет GCC, заголовки и библиотеки GTK4 в
`toolchains/mingw64`. При сборке HiAsm использует этот каталог автоматически.
Другой набор можно выбрать переменной `HIASM_CPP_TOOLCHAIN` до запуска IDE.
Для сборки из исходников без комплекта нужен `g++` и `pkg-config` в PATH.

Для Windows → Linux подготовьте Debian/Ubuntu в WSL:

```powershell
powershell -File tools/setup_wsl.ps1 -Distribution Debian -InstallDependencies -VerifyGui
$env:HIASM_WSL_DISTRO = 'Debian'
```

Скрипт без `-InstallDependencies` только проверяет инструменты. Для GTK4
нужна версия дистрибутива с `libgtk-4-dev` и дисплей (например, WSLg).
`-VerifyGui` дополнительно компилирует GTK4-пример, создаёт и закрывает окно.
Выберите `TargetPlatform=Linux`: сборка и запуск выполняются в WSL.
«Стоп» посылает SIGTERM отдельной группе процессов конкретного запуска.

Для Linux → Windows установите MinGW и Wine. На Debian/Ubuntu:

```sh
sudo sh tools/setup_cpp_linux.sh --install-dependencies --prefix /absolute/path/wine-cpp64
export HIASM_WINE_PREFIX=/absolute/path/wine-cpp64
```

Для уже установленных зависимостей запустите скрипт без флага установки от
пользователя IDE; prefix должен быть доступен ему для записи. Выберите
`TargetPlatform=Windows`. Компилятор по умолчанию —
`x86_64-w64-mingw32-g++`; переменные `HIASM_MINGW_CXX`,
`HIASM_MINGW_PKG_CONFIG`, `HIASM_WINE` выбирают другие инструменты.
GTK4 GUI требует отдельных библиотек GTK4 для Windows и соответствующего
pkg-config; нативный Linux GTK4 не заменяет их.

Для GUI-кросс-сборки можно использовать тот же Windows UCRT64-комплект,
который поставляется с установщиком. Скопируйте весь `toolchains/mingw64`
в каталог Linux, например `$HOME/.local/share/hiasm/mingw64`, сохранив
`bin`, `include`, `lib`, `share` и `manifest.json`. Не смешивайте его библиотеки
с системным MinGW: у них могут различаться CRT и версия GCC.

```sh
sh tools/setup_cpp_linux.sh \
  --windows-toolchain "$HOME/.local/share/hiasm/mingw64" \
  --prefix "$HOME/.local/share/hiasm/wine-cpp64" \
  --environment "$HOME/.local/share/hiasm/cpp-environment.sh" \
  --verify-gui
. "$HOME/.local/share/hiasm/cpp-environment.sh"
```

Запускайте HiAsm из этого окружения. Переменная `HIASM_CPP_WINDOWS_TOOLCHAIN`
выбирает Wine-адаптер для GCC/pkg-config; IDE добавляет DLL этого комплекта
только в окружение запускаемого Windows-приложения. `--verify-gui` требует
дисплея и проверяет компиляцию, создание и закрытие GTK4-окна. В WSL
храните комплект в файловой системе Linux: в проверенном окружении Wine
не смог загрузить GCC DLL с NTFS-диска, подключённого как `/mnt/d`.
Адаптер использует короткие DOS-пути и явный GCC prefix, чтобы поддерживать
каталоги с пробелами, кириллицей и апострофом; linker plugin отключён
(LTO в этом cross-профиле не поддерживается).
Запуск через Wine подтвердил функциональность, однако выдаёт предупреждения
о системных шрифтах и DWM; он не заменяет визуальную приёмку на Windows.

`Delay` блокирует вызывающий поток. В обработчиках интерфейса используйте
его только если такое блокирование задумано. `Timer` работает в главном
GTK4-потоке: `Interval` — миллисекунды (минимум 1), `Count` — число тиков
(0 или отрицательное — до остановки). `doStart` перезапускает таймер и счётчик,
`doStop` останавливает его, `onTimer` передаёт номер тика с единицы.
Закрытие окна удаляет его таймеры; после закрытия они не запускаются повторно.
В обработчике тика не выполняйте длительную работу, чтобы интерфейс оставался
отзывчивым. Таймер предназначен для GTK4 GUI, консольные шаблоны его не используют.
`Label` и `Button` поддерживают `doCaption` и чтение `Caption`,
`Edit` — `doText` и чтение `Text`. Эти элементы, флажок и progress bar
имеют `doEnabled` и `doVisible` (0/1; без данных — включить/показать).
Текстовый setter без данных использует одноимённое свойство. `Write.Data`
читает подключённую переменную, а без связи выводит данные входящего события.
Хранение виджетов раздельное для каждого окна; удалённый виджет не удерживается
в памяти, чтение отсутствующего значения возвращает пустую строку или 0.

`CheckBox`: `Checked` (0/1), `doChecked`, `onChange`, `doCaption`.
`ProgressBar`: `Position` и `doPosition` используют проценты 0–100;
значения вне диапазона ограничиваются, NaN трактуется как 0.
`doText` меняет подпись, `doPulse` выполняет шаг неопределённого прогресса.

`Memory` хранит пустое значение, целое, вещественное число или UTF-8 строку.
`doValue` получает подключённый `Data` либо данные события; `doClear` восстанавливает
`Default`. Оба входа передают снимок значения в `onData`; `Value` читает текущее
состояние. Память библиотеки/консоли создаётся заново при каждом вызове,
а в GUI принадлежит отдельному окну.

`Counter` использует знаковые 32-битные значения: `Min`, `Max`, `Step`,
`Default` и направление `Type`. `doNext` и `doPrev` меняют значение и вызывают
`onNext`; выход за границу переключает счётчик на противоположную границу.
`doReset` устанавливает Min для Inc или Max для Dec, без события. Входы
`doValue`, `doMin`, `doMax`, `doStep` также не вызывают событие.

Math/If/StrCat принимают связанные операнды. Math и StrCat сохраняют `Result`.
Math поддерживает 44 операции HiAsm 4: арифметику, биты, степени, тригонометрию,
гиперболические функции, логарифмы и округление. `ResultType` выбирает Integer
(округление к ближайшему чётному и 32-битный результат) либо Real,
`AngleMode` — радианы или градусы. `Default` задаёт исходное значение;
`doClear` сбрасывает результат и ошибку, `doDefault` меняет исходное значение,
`doAngleMode` меняет единицы углов. Ошибка области определения, деление на ноль
или переполнение вызывает `onError`; `Result` возвращает пустое значение
до успешной операции или сброса. Все числовые ошибки обрабатываются одинаково
на Windows и Linux. Имена точек и правила преобразования данных отличаются
от Delphi, поэтому компонент пока остаётся частичным аналогом.

`For`
принимает Start/End/Step из связей, поддерживает отрицательный Step, `doStop`
и `Index`; End исключён из диапазона. Нулевой шаг не выполняет итерации.
Это отличается от включённой конечной границы Delphi For.

Каждая точка схемы имеет одну физическую связь. Для объединения нескольких
событий используйте `HubEx`, для чтения одной переменной несколькими
потребителями — `GetDataEx`. Эти штатные элементы маршрутизации доступны через
базовый пакет; примеры показывают их применение. Файл с повторной связью на
занятую точку отклоняется, чтобы не повреждать схему.

Автономную Windows-папку для готового x86-64 GTK4 EXE можно создать без копирования
всего компилятора. Требуется Python 3.9+ и полный Windows runtime/toolchain:

```powershell
python tools/package_cpp_windows.py --executable "путь/Программа.exe" --runtime "toolchains/mingw64" --output "dist/Программа" --objdump "toolchains/mingw64/bin/objdump.exe"
```

Выходной каталог должен отсутствовать и находиться вне runtime. Скрипт собирает
транзитивные DLL, GTK data, SVG-загрузчик, темы иконок и лицензии, создаёт
перемещаемый cache и `distribution.json` с SHA256. Неизвестная отсутствующая DLL
завершает упаковку ошибкой. Для Linux → Windows используйте тот же скрипт с
`--objdump x86_64-w64-mingw32-objdump --wine wine --wine-prefix /путь/к/prefix`.
Проверки автономного запуска: `tools/test_cpp_package.ps1` на Windows и
`tools/test_cpp_package.sh` на Linux. Пользовательские файлы приложения,
базы данных и сторонние plugins добавляются отдельно.

## Приложение с формой (GTK4)

Шаблон `MainForm` создаёт окно GTK4 с поверхностью `GtkFixed`. Контролы схемы
создаются при запуске в порядке схемы и ставятся по `Left/Top/Width/Height`;
затем подключаются обработчики и срабатывает `onCreate`. Команда «Редактор формы»
открывает встроенный дизайнер (внешний `formedit` не нужен), форма принимает любое
число контролов.

Палитра: группа «Widgets» (имена Delphi: Button, Label, Edit, Memo, CheckBox,
RadioButton, ListBox, ComboBox, ProgressBar, TrackBar, GroupBox, Panel), «Controls»
(Timer, Message). `MainForm` — шаблон проекта и в палитре не показывается.
Одноимённые шаблоны `hcTpl*` (Button, Label, Edit, Memo, CheckBox, ListBox,
ComboBox, ProgressBar) лежат в скрытой группе с пустым именем: схемы с ними
открываются и собираются, но новые элементы берите из «Widgets».

Группы базы `elements.sql`: **RTCG** — собственные элементы `Cpp*` (проекты,
логика, строки, вывод, Codegen, GTK4), **Core** — элементы `hc*`. Группа WinCE не
переносится. Элементы Delphi без реализации записаны в таблицу
`planned_elements` вместе с именами файлов иконок; план — в
`MARKDOWN/CPP_ELEMENTS_PLAN.md` (его строит `tools/cpp_elements_plan.py`).
Общие точки контролов: `doLeft`, `doTop`, `doWidth`, `doHeight`, `doVisible`,
`doEnabled`, `doHint`.

Ограничения:

- GroupBox и Panel — рамки: контролы лежат на форме рядом с ними, а не внутри;
- все RadioButton формы образуют одну группу;
- ComboBox построен на устаревшем `GtkComboBoxText` (единственный стандартный
  виджет с полем ввода и списком);
- Timer, Message и Clipboard требуют окна: в консольном проекте сборка
  завершается ошибкой компиляции с названием элемента;
- Trace пишет в stderr, `doClear` ничего не делает.

## Core-элементы `hc*`

RTCG компилирует Hub, DoData, Console, If_else, For, Math, Convertor, Random,
Memory, TimeCounter, EventFromData, IndexToChanel, ChanelToIndex, StrCat, StrPos,
FormatStr, Copy, StrPart, Replace, Insert, Delete, BlockFind, StrList, ArrayRead,
ArrayEnum, FileSearch, Execute, Trace, Timer, Message и Clipboard. Позиции в
Copy/Delete/Insert считаются в символах UTF-8 с единицы, как в Delphi; в StrPos —
с нуля, как в Core. `IncludeEnd`, `IncludeBlock` и `Delete` по умолчанию `True`.
Данные неподключённых точек берутся из потока по порядку чтения, поэтому
каждую точку скрипт читает один раз.

Элементы без C++-кода (Image, FileChooserButton, ColorButton, вложенные
контейнеры `hcTplChild*`, `hcMultiElement`, OpenDialog/SaveDialog, HTTPText,
DS_SQLite/DSC_*, Commander, HiAsmTransmitter) не показываются в палитре, а схема
с ними не собирается: сообщение перечисляет такие элементы. Они работают только
в проектах Core.

Пакет развивается: контролы «Widgets»/«Controls» — часть из 795 элементов
Delphi из HiAsm 4 (остальное в `planned_elements`), и совместимость с
остальными компонентами не заявляется.
