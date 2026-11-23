# core.dll: Ghidra Research

## Scope

Исследование исторического файла:

```text
C:\Program Files (x86)\HiAsm\core.dll
```

`core.dll` является небольшим Win32/i386 runtime-модулем HiAsm. Он не предоставляет DB management API уровня `elmen.dll` и не выполняет установку пакетов уровня `hitools.dll`. Его основная роль по экспортам и декомпиляции — регистрация runtime-контекста и фабрика встроенных HC-элементов.

## Exported API

Ghidra показывает два именованных экспорта с ordinal-алиасами:

```text
Ordinal_1 -> newElement @ 0x004083A4
Ordinal_2 -> init       @ 0x00408724
```

Также присутствует обычный DLL entry point:

```text
entry @ 0x00408800
```

## Imports And Dependencies

`core.dll` импортирует Win32 API для runtime, памяти, clipboard и TLS:

```text
VirtualAlloc
VirtualFree
LocalAlloc
LocalFree
TlsAlloc
TlsFree
TlsGetValue
TlsSetValue
Sleep
```

Для clipboard runtime используются:

```text
GlobalAlloc
GlobalLock
GlobalUnlock
OpenClipboard
IsClipboardFormatAvailable
GetClipboardData
EmptyClipboard
SetClipboardData
CloseClipboard
```

Также DLL напрямую импортирует SQLite API из `sqlite3.dll`:

```text
sqlite3_open
sqlite3_close
sqlite3_exec
sqlite3_prepare
sqlite3_step
sqlite3_finalize
sqlite3_errmsg
sqlite3_last_insert_rowid
sqlite3_column_count
sqlite3_column_name
sqlite3_column_type
sqlite3_column_text
sqlite3_column_int
sqlite3_column_double
sqlite3_data_count
```

В отличие от `elmen.dll`, который динамически разрешает SQLite через `GetProcAddress`, `core.dll` имеет SQLite symbols в обычной import table. Это означает, что загрузчик Windows должен разрешить `sqlite3.dll` уже при загрузке `core.dll`.

## Runtime Elements

Строки бинарника показывают встроенные классы/имена элементов и их event/data points:

```text
hcClipboard
hcStrCat
hcReplace
hcCopy
hcStrPart
hcStrPos
hcConvertor
hcArrayRead
hcDS_SQLite
hcDSC_Exec
hcDSC_QueryScalar
hcDSC_Query
```

Также присутствуют имена интерфейсных точек и параметров:

```text
onPutText
 doPutText
CurrentText
onRead
Array
Index
doRead
onStrCat
Result
doStrCat
onReplace
Source
doReplace
onCopy
Position
Count
doCopy
onPart
onSplit
onNotFound
doSplit
Right
onSearch
Target
StartPos
doSearch
doReset
onResult
doConvert
onOpen
onError
FileName
doOpen
doClose
onExec
RowID
doExec
onQuery
onColumns
DSManager
doQuery
```

Эти строки указывают, что `core.dll` содержит как строковые/массивные runtime-элементы, так и SQLite data-source элементы.

## init

Экспорт:

```text
init @ 0x00408724
```

Декомпиляция:

```text
init(param_1):
    FUN_00402558(param_1 + 4)
```

`FUN_00402558` записывает три значения из переданного контекста в глобальные function pointers:

```text
PTR_FUN_0040902C = param_1[1]
PTR_FUN_00409030 = param_1[2]
PTR_FUN_00409034 = param_1[3]
```

Следовательно, `init` принимает указатель на runtime/API-контекст, где первые три значения, вероятно, являются служебными полями или размером/версией, а следующие три — callback-ами или host services. Точные типы и назначение этих трех указателей требуют анализа caller-а в `HiAsm.exe` или package runtime DLL.

Важно: `init` не загружает SQLite, не создаёт UI и не открывает базы. Его видимая функция — передача host-provided function pointers во внутреннюю таблицу `core.dll`.

## newElement

Экспорт:

```text
newElement @ 0x004083A4
```

Декомпилятор восстанавливает минимум два аргумента:

```text
newElement(char *param_1, undefined4 *param_2)
```

`param_2` копируется в локальный runtime-контекст размером шесть 32-битных слов. Затем `newElement` выполняет последовательное сравнение/распознавание имени элемента через внутренние helpers.

Распознанные имена и вызываемые конструкторы:

| Element name | Internal constructor |
|---|---:|
| `hcClipboard` | `FUN_0040614C` |
| `hcStrCat` | `FUN_00406588` |
| `hcReplace` | `FUN_00406778` |
| `hcCopy` | `FUN_00406994` |
| `hcStrPart` | `FUN_00406B98` |
| `hcStrPos` | `FUN_00406E18` |
| `hcConvertor` | `FUN_004070D0` |
| `hcArrayRead` | `FUN_0040634C` |
| `hcDS_SQLite` | `FUN_00407600` |
| `hcDSC_Exec` | `FUN_00407C2C` |
| `hcDSC_QueryScalar` | `FUN_00407E6C` |
| `hcDSC_Query` | `FUN_004080CC` |

Если имя не совпадает ни с одним из перечисленных элементов, фабрика возвращает null.

Упрощенная логика:

```text
context := copy six dwords from param_2
name := param_1

if name == "hcClipboard":      create clipboard element
else if name == "hcStrCat":    create string-concat element
else if name == "hcReplace":   create replace element
else if name == "hcCopy":      create substring/copy element
else if name == "hcStrPart":   create split/part element
else if name == "hcStrPos":    create string-search element
else if name == "hcConvertor": create conversion element
else if name == "hcArrayRead": create array-read element
else if name == "hcDS_SQLite": create SQLite data-source element
else if name == "hcDSC_Exec": create SQL exec component
else if name == "hcDSC_QueryScalar": create scalar query component
else if name == "hcDSC_Query": create row/column query component
else:
    return null
```

После вызова конструктора `newElement` проверяет результат. При успешном создании из первого слова объекта извлекается function pointer/virtual method value, который сохраняется во внутреннем глобальном результате. При отсутствии совпадения результат обнуляется.

Точная C-структура возвращаемого объекта не восстановлена. Поведение указывает на Delphi/KOL-подобный объектный layout: первый 32-битный элемент созданного объекта используется как method/VMT pointer.

## SQLite Runtime Components

SQLite-компоненты представлены отдельными фабричными именами:

```text
hcDS_SQLite
hcDSC_Exec
hcDSC_QueryScalar
hcDSC_Query
```

По импортам и строкам можно установить их общую область ответственности:

```text
hcDS_SQLite       управление DSManager / открытие SQLite datasource
hcDSC_Exec        выполнение SQL и возврат RowID/error
hcDSC_QueryScalar scalar SQL query
hcDSC_Query       query с колонками, строками и callback/error events
```

Подтвержденные связанные поля/события:

```text
DSManager
FileName
onOpen
onError
doOpen
doClose
onExec
RowID
doExec
onQuery
onColumns
doQuery
Result
```

`core.dll` напрямую импортирует SQLite функции, но формат host-level `DSManager` и точные структуры event callbacks требуют анализа внутренних конструкторов `FUN_00407600`, `FUN_00407C2C`, `FUN_00407E6C` и `FUN_004080CC`.

## Clipboard And String Runtime Components

`hcClipboard` использует Windows clipboard imports и, вероятно, обслуживает обмен текстом через `onPutText`, `doPutText`, `CurrentText`.

Строковые компоненты имеют следующие группы:

```text
hcStrCat:
  onStrCat, doStrCat, Result

hcReplace:
  onReplace, Source, Result, doReplace

hcCopy:
  onCopy, Position, Count, Result, doCopy

hcStrPart:
  onPart, onSplit, onNotFound, Right, doSplit

hcStrPos:
  onSearch, Target, StartPos, doSearch, doReset, Position

hcConvertor:
  doConvert, Result
```

Эти имена позволяют связать фабрику `newElement` с набором базовых элементов, встроенных непосредственно в `core.dll`, а не загружаемых как отдельные package DLL.

## Relation To Other DLLs

```text
HiAsm.exe / runtime host
    |
    | init(host context)
    v
core.dll
    |
    | newElement(name, context)
    +--> built-in string/clipboard elements
    +--> SQLite runtime elements

elmen.dll
    +--> package/global DB API

hitools.dll
    +--> install/remove/link package files and DB records
```

`core.dll` не импортирует DB API `elmen.dll`, `hitools.dll` или `hiasm.dll`. Его SQLite access относится к runtime data-source components, а не к управлению `Elements.db` или `int\\hiasm.db`.

## ABI Notes

Оба экспорта имеют Delphi-compatible Win32/i386 ABI и ordinal aliases. Надежные публичные C-сигнатуры пока не восстановлены полностью:

```text
init(host_context)
newElement(element_name, runtime_context)
```

Для `newElement` подтверждено, что второй аргумент содержит минимум шесть 32-битных значений, копируемых в локальный context. Для `init` подтверждено чтение значений по `param_1 + 4`, то есть установка трех host-provided pointers из offsets `+4`, `+8`, `+C`.

Нужно учитывать, что Ghidra может показывать Delphi/KOL object methods и callbacks как `undefined4`/`int *`, пока не восстановлены структуры и calling conventions.

## Conclusions

1. `core.dll` — runtime-фабрика встроенных HiAsm-элементов, а не package/database manager.
2. Публичные экспорты: `newElement` и `init`.
3. `init` передаёт три host-provided function pointers во внутреннюю таблицу runtime.
4. `newElement` распознаёт 12 встроенных элементов: clipboard, строковые, массивный и SQLite-компоненты.
5. SQLite runtime используется напрямую через импортированную `sqlite3.dll`.
6. `hcDS_SQLite`, `hcDSC_Exec`, `hcDSC_QueryScalar` и `hcDSC_Query` образуют отдельный runtime data-source слой.
7. `core.dll` не зависит от `elmen.dll` и `hitools.dll` по импортной таблице.
8. Точные структуры `runtime_context`, объектов элементов и callbacks требуют анализа caller-ов и внутренних конструкторов.
