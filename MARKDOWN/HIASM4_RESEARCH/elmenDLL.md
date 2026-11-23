# elmen.dll: Ghidra Research

## Scope

Исследование исторического файла:

```text
C:\Program Files (x86)\HiAsm\elmen.dll
```

`elmen.dll` является Win32/i386 DLL исторического HiAsm и предоставляет SQLite-backed database API для глобальной базы HiAsm и package-level базы элементов.

## Exported Database API

Ghidra показывает именованные экспорты с ordinal-алиасами. Основные функции:

```text
OpenDB / CloseDB
OpenHiAsmDB / CloseHiAsmDB
CreateHiAsmDB
EnumCompilers / AddCompiler / SetCompiler / GetCompiler
EnumPacks / AddPack / RemovePack
AddPackCompiler / RemovePackCompiler
EnumTabs / EnumElements
```

Также экспортируются операции с группами, элементами, файлами, командами, меню, расширениями, локализацией, проектами и параметрами.

Примеры экспортов и адресов внутри DLL:

```text
OpenDB          0x00404f3c
CloseDB         0x00404f60
OpenHiAsmDB     0x00404f74
CloseHiAsmDB    0x00404f98
CreateHiAsmDB   0x00405244
EnumTabs        0x00406834
EnumElements    0x004068c0
EnumCompilers   0x00409aa4
EnumPacks       0x0040a444
```

Большинство экспортов имеют Delphi ordinal-алиасы. В частности, `OpenDB` экспортируется также как `Ordinal_53`, `CloseDB` как `Ordinal_52`, а функции перечисления и изменения базы сохраняют историческую ordinal-нумерацию.

## SQLite Initialization

DLL не линкуется с SQLite статически. При первой необходимости ее инициализация вызывает:

```text
LoadLibraryA("sqlite3.dll")
GetProcAddress(...)
```

Через `GetProcAddress` заполняется внутренняя таблица адресов SQLite API. В ней присутствуют, в частности:

```text
sqlite3_libversion
sqlite3_open
sqlite3_close
sqlite3_exec
sqlite3_prepare
sqlite3_step
sqlite3_finalize
sqlite3_reset
sqlite3_errcode
sqlite3_errmsg
sqlite3_bind_double
sqlite3_bind_int
sqlite3_bind_int64
sqlite3_bind_null
sqlite3_bind_text
sqlite3_bind_blob
sqlite3_column_count
sqlite3_column_name
sqlite3_column_decltype
sqlite3_column_blob
sqlite3_column_bytes
sqlite3_column_double
sqlite3_column_int
sqlite3_column_int64
sqlite3_column_text
sqlite3_column_type
sqlite3_create_function
sqlite3_create_collation
```

Экспортированные DB-функции вызывают SQLite через эту таблицу указателей. Общий initializer `FUN_0040432c` выполняется лениво при первом вызове `OpenDB` или `OpenHiAsmDB`: он загружает `sqlite3.dll` и разрешает полный набор SQLite-функций, включая `sqlite3_exec`, prepared statements, bind/column API, transaction-independent metadata API и SQLite user-defined function/collation API.

`OpenDB` и `OpenHiAsmDB` являются тонкими экспортными обёртками. Обе сначала вызывают `FUN_00404de0`, затем переходят через один и тот же глобальный указатель `PTR_DAT_0040c0d4`, то есть используют общий внутренний механизм открытия базы. Аналогично `CloseDB` и `CloseHiAsmDB` переходят через один и тот же `PTR_DAT_0040c0dc`.

На уровне декомпиляции Ghidra `OpenDB` не имеет фиксированной сигнатуры, потому что Delphi-код вызывает function pointer без восстановленного прототипа. Точные параметры нужно восстанавливать по call sites, а не по экспортной заглушке.

## Package Database Schema

`CreateHiAsmDB` напрямую выполняет четыре SQL-команды через внутреннюю SQLite-обёртку `FUN_00404fb4` и создаёт следующие таблицы:

```sql
CREATE TABLE elements(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name varchar(64),
  info varchar(64),
  tab int,
  pos int,
  hash varchar(32)
);
CREATE TABLE groups(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name varchar(64),
  info varchar(128),
  pos int
);
CREATE TABLE files(
  id INTEGER PRIMARY KEY AUTOINCREMENT,
  name varchar(128),
  count int
);
CREATE TABLE files_link(file_id int, el_id int);
```

Проверенная установленная база:

```text
C:\Program Files (x86)\HiAsm\Elements\delphi\Elements.db
```

содержит именно эти четыре таблицы, включая служебную `sqlite_sequence`.

Назначение таблиц:

```text
groups       вкладки/группы элементов
elements     элементы, их вкладка, позиция и hash
files        файлы, связанные с элементами
files_link   связь file_id -> el_id
```

## Global HiAsm Database

Глобальная база:

```text
C:\Program Files (x86)\HiAsm\int\hiasm.db
```

Ее схема содержит:

```text
info_id_ru
info_id_en
menus
menu_commands
ext_assign
localization
compilers
projects
packs
commands
pack_compilers
settings
params
```

Таким образом, `int\hiasm.db` хранит общие сведения о пакетах, компиляторах, проектах, командах, меню, локализации, расширениях, параметрах и настройках. `Elements.db` конкретного пакета хранит группы, элементы и связи с файлами реализации.

## Relation To HiAsm.exe

В `HiAsm.exe` DB-функции вызываются через короткие thunk-функции и глобальную таблицу function pointers. Из ранее исследованных адресов:

| HiAsm API | Таблица/указатель |
|---|---:|
| `CloseHiAsmDB` | `0x005F96F0` |
| `OpenHiAsmDB` | `0x005F96F4` |
| `CloseDB` | `0x005F96F8` |
| `OpenDB` | `0x005F96FC` |
| `EnumPacks` | `0x005F9720` |
| `EnumElements` | `0x005F974C` |
| `EnumTabs` | `0x005F9750` |

`elmen.dll` предоставляет реализацию экспортированных операций. Сама запись адресов в таблицу `HiAsm.exe` выполняется хостом при загрузке DLL, а не внутри `elmen.dll`. В текущем сеансе Ghidra точная функция хоста, содержащая `LoadLibraryA("elmen.dll")` и последовательные вызовы `GetProcAddress`, еще не локализована; это остается отдельной задачей по `HiAsm.exe`.

При сравнении с исследованным `hiasm.dll` зафиксировано расхождение имени: `hiasm.dll` содержит строку `LoadLibraryA("elman.dll")`, тогда как текущий исследуемый файл и известная установка используют имя `elmen.dll`. В самом `elmen.dll` строка `elman.dll` не является собственной загрузочной зависимостью: его единственная аналогичная динамическая загрузка относится к `sqlite3.dll`. Вызов `FUN_0040432c` выполняет `LoadLibraryA("sqlite3.dll")`, после чего последовательно вызывает внутренний `FUN_004042d4` для разрешения SQLite-символов. Lazy-обёртка `FUN_00404de0` вызывает этот initializer только если `DAT_0040c0c8 == 0`.

Дополнительные DLL имеют отдельные зоны ответственности:

```text
core.dll    newElement, init
elmen.dll   database/compiler/package API
hitools.dll InstallPackage, InstallElement, RemoveElement, LinkFile
```

## Export Wrapper And Callback Notes

`EnumTabs` на экспортном адресе `0x00406834` является короткой Delphi-обёрткой, которая без собственной логики передаёт управление через `PTR_DAT_0040c0cc`. В текущем Ghidra-сеансе `EnumElements` экспортирована по адресу `0x004068c0`, но этот адрес не распознан как обычное тело функции: он присутствует как data-reference (`DATA`), а также имеет ссылку из `Entry Point [EXTERNAL]`. Поэтому попытка декомпилировать `0x004068c0` как функцию завершается сообщением `No function found`. Это является особенностью анализа экспортной/Delphi thunk-цепочки, а не доказательством отсутствия экспорта. Точную реализацию и сигнатуру `EnumElements` следует восстанавливать через этот экспортный указатель и call sites вызывающих модулей.

`EnumPacks` показывает характерный для Delphi callback-шаблон: локально создаётся служебный контекст, затем вызывается внутренняя функция подготовки результата, после чего управление передаётся через `PTR_DAT_0040c0cc`. Тот же указатель вызывается из `EnumTabs`, `EnumCommands`, `EnumMenuList`, `EnumExt`, `EnumLoc` и `EnumCompilers`, что подтверждает общий callback-диспетчер/внутренний DB-метод для группы enumeration API. Это подтверждает, что callback и пользовательский context являются частью общего ABI перечислений.

Вызовы из `hiasm.dll` уточняют ожидаемую форму `EnumElements`:

```text
EnumElements(selector_or_parent, callback, user_data)
```

В `hiasm.dll` callback получает записи, из которых извлекаются строковые поля элемента и данные для загрузки иконки. Значение первого аргумента пока нельзя надёжно классифицировать как `id`; оно берётся из внутреннего объекта редактора.

## Message 0x403

В `elmen.dll` не найдено оконного класса HiAsm, главного window procedure или dispatch path для сообщения `0x403`. Следовательно, обработчик сообщения и формат `wParam`/`lParam` принадлежат `HiAsm.exe` и должны исследоваться в его Ghidra-проекте.

Это согласуется с общей схемой single-instance: второй экземпляр активирует существующее окно HiAsm и отправляет ему `0x403`. В `HiAsm.exe`, `entry @ 0x005E63D4`, отправка подтверждена как `SendMessageA(hwnd, 0x403, 0, 0)`: оба параметра payload явно обнуляются. Перед этим данные о второй команде передаются в объект/registry-backed состояние через `FUN_0042dae4`, поэтому имя файла или команда не передаются через `wParam/lParam`. Receiver сообщения еще не локализован.

## CodeGen ABI Note

Исследование связанных Pascal-исходников показало:

```pascal
function CheckVersionProc(const params: THiAsmVersion): integer;
procedure ConfToCode(const Pack, UName: string);
function isReadyForAdd(
  cgt: PCodeGenTools;
  const rfd: TRFD_Rec;
  sdk: id_sdk
): boolean; cdecl;
```

`isReadyForAdd` присутствует в общем историческом `Elements\delphi\CodeGen.dpr`, но отсутствует в отдельном `FTCG_CodeGen.dpr`. Это различие между CodeGen-ветками, а не функция `elmen.dll`.

## Conclusions

1. `elmen.dll` является реализацией исторического DB API, а не обработчиком сообщений главного окна.
2. DLL динамически разрешает SQLite API и работает через внутреннюю таблицу function pointers.
3. `OpenDB`/`OpenHiAsmDB` используют общий указатель открытия, а `CloseDB`/`CloseHiAsmDB` общий указатель закрытия.
4. `CreateHiAsmDB` напрямую создаёт четыре package-таблицы: `elements`, `groups`, `files`, `files_link`.
5. `Elements.db` и `int\hiasm.db` имеют разные области ответственности.
6. Таблица DB function pointers в `HiAsm.exe` заполняется кодом хоста при загрузке DLL.
7. Вызовы перечисления используют callback и user context; `hiasm.dll` подтверждает трёхаргументную форму `EnumElements(selector_or_parent, callback, user_data)`.
8. Для восстановления receiver `0x403` и точного порядка загрузки необходимо продолжить анализ `HiAsm.exe`; call site сообщения уже установлен: `wParam=0`, `lParam=0`.
