# HiAsm.exe

## Объект исследования

- Файл: `C:\Program Files (x86)\HiAsm\HiAsm.exe`
- Формат: 32-битный PE для x86.
- Исследование выполняется через GhidraMCP.
- Текущий статус: первичная карта бинарника и анализ точки входа.

## Карта бинарника

Сегменты, обнаруженные Ghidra:

| Сегмент | Диапазон |
|---|---:|
| `Headers` | `0x00400000-0x004003FF` |
| `.text` | `0x00401000-0x005E43FF` |
| `.itext` | `0x005E5000-0x005E67FF` |
| `.data` | `0x005E7000-0x005EE3FF` |
| `.bss` | `0x005EF000-0x005F7BEF` |
| `.idata` | `0x005F8000-0x005FC5FF` |
| `.tls` | `0x005FD000-0x005FD047` |
| `.rdata` | `0x005FE000-0x005FE1FF` |
| `.reloc` | `0x005FF000-0x00620BFF` |
| `.rsrc` | `0x00621000-0x0063DFFF` |

Экспортирована одна функция:

```text
entry @ 0x005E63D4
```

Ghidra классифицирует ее как `undefined __register entry(void)`. В бинарнике присутствуют типичные для Delphi/KOL-приложения конструкции, включая таблицы переходов, VMT-подобные вызовы и RTL-обвязку.

## Отладочные строки и происхождение

В строках сохранились пути исходников:

```text
C:\work\add\kol_hiasm\kol.pas
C:\work\hiasm\SDK.pas
C:\work\hiasm\Element.pas
```

Это указывает на Pascal-исходники и KOL-компонентную основу старого приложения.

Обнаружены также:

```text
HiAsm
HiAsm.exe
-=HiAsm=-
hiasm
HiAsmUnit.pas
```

## Точка входа `entry`

Адрес: `0x005E63D4`.

Наблюдаемая последовательность:

1. Инициализация RTL и обработчика исключений.
2. Получение/открытие настроек через `Software\HiAsm`.
3. Проверка параметров локали и окна.
4. Загрузка `Settings.cfg`.
5. Проверка уже запущенного экземпляра через объект или окно с именем `HiAsm`.
6. При наличии другого экземпляра выполняется его активация и отправляется сообщение `0x403`.
7. Инициализация приложения.
8. Создание главного окна.
9. Сохранение дескриптора главного окна в настройках/реестре.
10. Освобождение объектов и вызов `CloseHiAsmDB`.

Фрагменты, важные для дальнейшего анализа:

```text
Software\\HiAsm
Settings.cfg
HiAsm
Handle
WM/message: 0x403
OpenHiAsmDB
CloseHiAsmDB
```

### Предварительная интерпретация

`HiAsm.exe` поддерживает single-instance workflow. Второй запуск не создает независимое окно, а находит существующий экземпляр, выводит его на передний план и отправляет ему `0x403`.

В `entry @ 0x005E63D4` call site сообщения восстановлен точно:

```text
lParam = 0
wParam = 0
uMsg   = 0x403
hWnd   = registry value "Handle"
SendMessageA(hWnd, 0x403, 0, 0)
```

Непосредственно перед отправкой `local_1c` передается в функцию `FUN_0042dae4` вместе с ключом данных `0x005E6718`, но в `SendMessageA` используется именно `wParam=0` и `lParam=0`. Xref к ключу `0x005E6718` в текущем анализе ведёт только из `entry`, где он используется как параметр записи состояния перед отправкой. Поэтому payload сообщения не содержит указатель на имя файла; возможное имя/команда передается через состояние объекта или registry-backed структуру до вызова сообщения.

Поиск receiver `0x403` в просмотренных host-функциях пока не дал подтверждённой оконной процедуры. В частности, `0x403` отсутствует в обработанном пути `sdk_msg` из `hiasm.dll`; receiver следует искать среди процедур главного окна `HiAsm.exe` и возможных `CallWindowProcA`/VMT-диспетчеров.

## База данных HiAsm

Строка базы данных:

```text
.\\int\\hiasm.db
```

Ghidra распознала функции:

```text
OpenHiAsmDB       @ 0x0054BE0C
CloseHiAsmDB      @ 0x0054BE14
EnumPacks         @ 0x0054BDB4
EnumCompilers     @ 0x0054BD94
AddCompiler       @ 0x0054BD9C
SetCompiler       @ 0x0054BDA4
GetCompiler       @ 0x0054BDAC
AddPackCompiler   @ 0x0054BDDC
RemovePackCompiler @ 0x0054BDE4
```

Функция `FUN_0054BE24` вызывает `OpenHiAsmDB` однократно, используя флаг инициализации `DAT_005F7174`. В текущем бинаре она вызывается из host-side DB access path; перед открытием выполняется `SetCurrentDirectoryA` для пути, подготовленного из `.` и строки базы:

```text
if DAT_005F7174 == 0:
    DAT_005F7174 = 1
    initialize local state
    SetCurrentDirectoryA(local path)
    OpenHiAsmDB()
```

Входная строка для этого пути находится по адресу `0x0054BE84`:

```text
.\int\hiasm.db
```

`FUN_0040B210` вызывает `SetCurrentDirectoryA`, а затем host thunk `OpenHiAsmDB` передаёт управление через `[0x005F96F4]`. Это подтверждает, что global DB API ожидает рабочий каталог, установленный хостом, а не обязательно полный путь в каждом вызове.

При завершении `entry` вызывает `CloseHiAsmDB`; таким образом, global DB открывается лениво один раз и закрывается после основного цикла приложения.

Дизассемблирование показывает, что это не тело базы, а переходники через глобальные указатели:

```text
0x0054BE0C: JMP dword ptr [0x005F96F4]  ; OpenHiAsmDB
0x0054BE14: JMP dword ptr [0x005F96F0]  ; CloseHiAsmDB
0x0054BDB4: JMP dword ptr [0x005F9720]  ; EnumPacks
0x0054BDF4: JMP dword ptr [0x005F96FC]  ; OpenDB
0x0054BE04: JMP dword ptr [0x005F96F8]  ; CloseDB
0x0054BD54: JMP dword ptr [0x005F9750]  ; EnumTabs
0x0054BD5C: JMP dword ptr [0x005F974C]  ; EnumElements
```

Следовательно, реализация базы находится в загруженном модуле или подключаемой библиотеке, а `HiAsm.exe` содержит только импортные фасады. Адреса в диапазоне `0x005F96F0-0x005F974C` являются указателями разрешенных функций. Отдельная функция загрузки `elmen.dll` и точные инструкции записи этих адресов пока не локализованы в текущем сеансе Ghidra; подтверждено лишь, что вызовы идут через эту таблицу.

Это объясняет неудачную декомпиляцию: Ghidra видит короткий thunk, а не реализацию SQLite/DB API. В текущем сеансе это подтверждено дизассемблированием:

```text
0x0054BE0C: JMP dword ptr [0x005F96F4]  ; OpenHiAsmDB
0x0054BE14: JMP dword ptr [0x005F96F0]  ; CloseHiAsmDB
0x0054BDFC: JMP dword ptr [0x005F96FC]  ; OpenDB
0x0054BE04: JMP dword ptr [0x005F96F8]  ; CloseDB
0x0054BDB4: JMP dword ptr [0x005F9720]  ; EnumPacks
0x0054BD5C: JMP dword ptr [0x005F974C]  ; EnumElements
0x0054BD54: JMP dword ptr [0x005F9750]  ; EnumTabs
```

Прямая функция, которая вызывает `LoadLibraryA("elmen.dll")` и заполняет все эти адреса, в текущем наборе xrefs не локализована. Строка `elmen.dll` находится в `.itext` по `0x005FC160`, а имена экспортов находятся рядом (`OpenDB` около `0x005FC1A0`), но автоматические ссылки Ghidra на эти строки отсутствуют. Поэтому последовательность host-side `GetProcAddress` пока нельзя считать восстановленной.

### Ленивое перечисление

Доступ к пакетам и компиляторам выполняется лениво:

- `FUN_0059CE94` вызывает `EnumPacks()`, только если флаг по `param_1 + 0x20` еще не установлен;
- `FUN_0059C2A4` создает контейнер по `FUN_004CC860()`, сохраняет его в `param_1 + 4`, затем вызывает `EnumCompilers()`;
- `FUN_0059C474` ищет компилятор через `FUN_0059C388`, добавляет его идентификатор в массив пакета и при активном флаге вызывает `AddPackCompiler()`.

Call site `FUN_005716CC` показывает загрузку элементов конкретного пакета:

```text
FUN_00571638()
EnumTabs()
for each package/tab:
    build package path
    OpenDB()
    EnumElements()
    CloseDB()
FUN_005716C4()
```

Другой call site, `FUN_0055B65C`, при выборе каталога выполняет:

```text
OpenDB()
EnumElements()
CloseDB()
```

После перечисления элементы передаются в UI-список через виртуальные методы контейнера.

### Связь с текущей веткой

Старая версия использует центральную базу `int\\hiasm.db`, а текущая GTK4-ветка содержит аналогичные подсистемы `MainDataBase`, `Pack` и компиляторные записи. Структуру таблиц старой базы нужно извлечь из кода или из установленного файла, но старые пути нельзя переносить буквально: современный runtime использует `packs/`.

## Пакеты и элементы

В бинарнике обнаружены строки:

```text
Elements\\
\\Elements.db
Elements\\_base\\conf\\
elements/
HCElements
EnumElements
```

Старый runtime layout:

```text
Elements/<package>/
Elements/<package>/Elements.db
Elements/_base/conf/
```

В текущей ветке это соответствует концептуально:

```text
packs/<package>/
packs/<package>/elements.db
packs/_base/conf/
```

Различие регистра и имени базы следует учитывать отдельно: в старом бинарнике видны варианты `Elements.db` и `elements.db`, поэтому поведение файловой системы Windows скрывает возможные проблемы переносимости.

## Генератор кода

В строках присутствуют варианты загрузки:

```text
\\CodeGen.dll
..\\codegen.dll
CodeGen.dll
```

Также найдены:

```text
ConfToCode
isReadyForAdd
CheckVersionProc
```

Функция `FUN_0059F0E4`:

1. подготавливает путь к генератору через `FUN_0056D0BD`;
2. добавляет строку `\\CodeGen.dll`;
3. вызывает `LoadLibraryA`;
4. получает адрес `isReadyForAdd` через `GetProcAddress`;
5. сохраняет модуль и адрес функции в глобальных данных.

Декомпилированный фрагмент:

```text
DAT_005F79E0 = LoadLibraryA(path)
DAT_005F79DC = GetProcAddress(DAT_005F79E0, "isReadyForAdd")
```

Функция `FUN_0055F184` строит конфигурационное представление элемента с секциями:

```text
[About]
[Type]
[Property]
[Methods]
```

Она собирает поля:

```text
Version=
Author=
Mail=
Class=
Sub=Form
Inherit=
Icon=
Tree=
Info=
Interfaces=
```

После этого формирует путь через `Elements\\`, загружает `CodeGen.dll`, получает экспорт `ConfToCode` и вызывает его. Это показывает, что `ConfToCode` получает конфигурацию элемента или путь к ней и участвует в генерации/подготовке кода.

Точный ABI `ConfToCode` еще не установлен: декомпилятор видит вызов через `FARPROC`, поэтому типы аргументов требуют анализа call site и соответствующего DLL.

## Импорты Win32

Наиболее важные группы импортов:

- файлы: `CreateFileA`, `ReadFile`, `WriteFile`, `SetFilePointer`;
- модули: `LoadLibraryA`, `LoadLibraryExA`, `GetProcAddress`, `FreeLibrary`;
- реестр: `RegOpenKeyExA`, `RegQueryValueExA`, `RegCloseKey`;
- окна: `CreateWindowExA`, `ShowWindow`, `SetForegroundWindow`, `SendMessageA`;
- память: `VirtualAlloc`, `VirtualFree`, `VirtualQuery`;
- локаль/кодировки: `MultiByteToWideChar`, `WideCharToMultiByte`, `GetACP`;
- COM/OLE: `ole32.dll`, `oleaut32.dll`.

Наличие `LoadLibraryA/GetProcAddress` вместе со строками `CodeGen.dll`, `core.dll`, `hitools.dll` подтверждает модульную архитектуру старого приложения.

## Host-side загрузка core.dll

В текущем сеансе подтверждена отдельная host-side цепочка для runtime `core.dll` в функции `FUN_005CE834`:

```text
build path: "core.dll"
DAT_005F7B08 = LoadLibraryA(path)
DAT_005F7B10 = GetProcAddress(module, "newElement")
GetProcAddress(module, ordinal/name at lpProcName_005CE9C8)
call resolved init entry with a local runtime context
```

После этого хост вызывает фабрику:

```text
(*DAT_005F7B10)(element_name, runtime_context)
```

Результат сохраняется в объекте элемента по смещению `+0x98`, а дополнительное значение контекста — по `+0x9C`. Это связывает `HiAsm.exe` с `core.dll`: хост передает runtime context, затем создает встроенный элемент по имени через экспорт `newElement`.

В `core.dll` второй экспорт `init` находится по `0x00408724`. Вызов из `FUN_005CE834` разрешается через второй `GetProcAddress`, но в текущей декомпиляции имя этого запроса не восстановлено. По поведению `core.dll` этот экспорт устанавливает три host-provided function pointers.

Найденные в текущем сеансе вызовы `LoadLibraryA` относятся к следующим модулям:

```text
hitools.dll  -> InstallElement, LinkFile, RemoveElement
core.dll     -> init/newElement
CodeGen.dll  -> build*/isElementMaker/synReadFuncList
plug\\*.dll  -> plugin APIs
```

Прямой call site `LoadLibraryA("elmen.dll")` пока не подтвержден: строка `elmen.dll` присутствует в `.itext` по `0x005FC160`, но автоматические xrefs к ней и к DB export names отсутствуют. Поэтому найденная host-side runtime цепочка не заменяет отдельную, пока не локализованную DB loader-функцию.

## Связанные DLL и плагины

Найдены строки:

```text
core.dll
hitools.dll
elmen.dll
plug\\Hook.dll
plug\\vicx.dll
plug\\frame.dll
plug\\zlib.dll
plug\\jpeg.dll
lnk_trace.dll
vcltest3.dll
```

Также бинарник импортирует системные компоненты:

```text
USER32.DLL
COMCTL32.DLL
OLE32.DLL
WS2_32.DLL
WININET.DLL
OPENGL32.DLL
GDI32.DLL
MSIMG32.DLL
VERSION.DLL
```

## Подтвержденные факты по DB API

| Факт | Адрес/источник |
|---|---|
| `OpenHiAsmDB` является thunk через глобальный указатель | `0x0054BE0C -> [0x005F96F4]` |
| `CloseHiAsmDB` является thunk через глобальный указатель | `0x0054BE14 -> [0x005F96F0]` |
| `EnumPacks` является thunk | `0x0054BDB4 -> [0x005F9720]` |
| `EnumElements` является thunk | `0x0054BD5C -> [0x005F974C]` |
| `OpenDB` является thunk | `0x0054BDFC -> [0x005F96FC]` |
| `CloseDB` является thunk | `0x0054BE04 -> [0x005F96F8]` |
| `EnumTabs` является thunk | `0x0054BD54 -> [0x005F9750]` |
| перечисление пакетов выполняется лениво | `FUN_0059CE94` |
| перечисление компиляторов выполняется лениво | `FUN_0059C2A4` |
| элементы открываются через цикл `OpenDB/EnumElements/CloseDB` | `FUN_005716CC` |
| связь пакет-компилятор может сохраняться в базе | `FUN_0059C474`, `AddPackCompiler` |

Это усиливает гипотезу, что DB API экспортируется отдельной библиотекой, а `HiAsm.exe` выступает клиентом этого API. Набор фасадов разделен на две области:

- глобальная база HiAsm: `OpenHiAsmDB`, `CloseHiAsmDB`, `EnumPacks`, `EnumCompilers`;
- выбранная package DB: `OpenDB`, `CloseDB`, `EnumTabs`, `EnumElements`.

Для восстановления реализации необходимо исследовать импортируемые модули и сами файлы установленной версии, особенно `core.dll`, `elmen.dll` и связанные runtime DLL.

## Дополнительные подтверждённые факты текущего сеанса

| Факт | Источник |
|---|---|
| `entry` имеет адрес `0x005E63D4` | текущий Ghidra project |
| global DB открывается лениво один раз | `FUN_0054BE24`, флаг `DAT_005F7174` |
| перед `OpenHiAsmDB` меняется текущий каталог | `FUN_0040B210` -> `SetCurrentDirectoryA` |
| путь базы содержит `.\\int\\hiasm.db` | строка `0x0054BE84` |
| `entry` закрывает global DB после основного цикла | вызов `CloseHiAsmDB` в `entry` |
| строка `elmen.dll` присутствует в host image | `0x005FC160` |
| имена DB exports присутствуют в host image | `OpenDB` около `0x005FC1A0`, `CloseHiAsmDB` около `0x005FC178` |
| host-side loader DB пока не локализован | текущие xrefs `LoadLibraryA/GetProcAddress` |
| `core.dll` загружается хостом динамически | `FUN_005CE834`: `LoadLibraryA("core.dll")` |
| `newElement` разрешается и вызывается хостом | `FUN_005CE834`: `GetProcAddress` и вызов фабрики |
| `core.dll` получает runtime context от хоста | `FUN_005CE834` передает локальный context в `newElement` |

## Текущие гипотезы

| Гипотеза | Уверенность | Основание |
|---|---|---|
| Бинарник собран на Pascal/Delphi/KOL-стеке | высокая | пути `.pas`, VMT-подобные вызовы, RTL-структуры |
| Приложение использует single-instance механизм | высокая | объект/окно `HiAsm`, `Handle`, сообщение `0x403` |
| Пакеты старого runtime находятся под `Elements\\` | высокая | многочисленные строки и построение путей |
| `CodeGen.dll` загружается из package-local каталога | высокая | `Elements\\...\\CodeGen.dll`, `LoadLibraryA` |
| `ConfToCode` преобразует конфигурацию элемента | средняя | call site в `FUN_0055F184` |
| `isReadyForAdd` проверяет готовность генератора к добавлению элемента | низкая | только имя экспорта, реализация не исследована |
| `int\\hiasm.db` содержит пакеты и компиляторы | высокая | `EnumPacks`, `EnumCompilers`, пути и entry point |
| глобальная база и package DB используют разные API-фасады | высокая | отдельные thunk-и `OpenHiAsmDB/CloseHiAsmDB` и `OpenDB/CloseDB` |
| перечисление элементов выполняется после явного открытия package DB | высокая | `FUN_005716CC`: `OpenDB -> EnumElements -> CloseDB` |
| `core.dll` является отдельным runtime factory layer | высокая | `FUN_005CE834` загружает `core.dll`, разрешает `newElement` и сохраняет результат в объекте элемента |

## Следующие точки исследования

1. Найти функцию загрузки `elmen.dll` и инструкции, заполняющие указатели `0x005F96F0-0x005F9750`; автоматические xrefs к строкам пока отсутствуют, поэтому потребуется ручной анализ `.itext` или вызовов `GetProcAddress`. В текущем сеансе подтверждено, что найденные поздние загрузчики относятся к `hitools.dll`, `core.dll`, `CodeGen.dll` и plugins.
2. Исследовать `EnumPacks`, `EnumElements`, `EnumCompilers` и операции с базой.
3. Найти receiver сообщения `0x403` в оконном dispatch; call site уже подтверждает `wParam=0`, `lParam=0`.
4. Найти все xrefs к `CodeGen.dll`, `ConfToCode`, `isReadyForAdd` и `CheckVersionProc`.
5. Сопоставить старые записи компиляторов с текущим `ProjectBuilder`.
6. При наличии доступа к установленным файлам извлечь и исследовать `int\\hiasm.db`, `Elements.db` и `CodeGen.dll` отдельно.
