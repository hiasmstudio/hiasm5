# hitools.dll: Ghidra Research

## Scope

Исследование исторического файла:

```text
C:\Program Files (x86)\HiAsm\hitools.dll
```

`hitools.dll` является Win32/i386 DLL вспомогательных операций установки и удаления пакетов/элементов HiAsm. В отличие от `elmen.dll`, который предоставляет SQLite-backed DB API, `hitools.dll` выполняет файловые операции и вызывает DB API через динамически импортированные функции.

## Exported API

Ghidra показывает четыре именованных экспорта с ordinal-алиасами:

```text
InstallPackage  0x004054E8  Ordinal_1
LinkFile        0x00405050  Ordinal_2
RemoveElement   0x00405228  Ordinal_3
InstallElement  0x00404D48  Ordinal_4
```

Экспортные адреса `InstallElement`, `LinkFile` и `InstallPackage` в текущем проекте Ghidra представлены как export/data-thunk entries и не распознаны как обычные тела функций. Поэтому прямой вызов декомпилятора по `0x00404D48`, `0x00405050` или `0x004054E8` завершается сообщением `No function found`. Это не означает отсутствия экспорта.

`RemoveElement` по адресу `0x00405228` распознана и декомпилируется как полноценная функция.

## Dynamic Dependency On elmen.dll

В импортной таблице присутствуют функции Windows для работы с файлами и каталогами, а также символы DB API из `elmen.dll`:

```text
elmen.dll:
  OpenDB
  CloseDB
  EnumPacks
  AddGroup
  AddElement
  GroupExists
  DeleteElement
  EnumFiles
  AddFile
  RemoveFile
```

Также импортированы функции, используемые файловым слоем:

```text
CreateFileA
CloseHandle
ReadFile
WriteFile
GetFileSize
GetFileAttributesA
CopyFileA
DeleteFileA
FindFirstFileA
FindNextFileA
FindClose
SetFilePointer
SetEndOfFile
GetModuleFileNameA
LoadLibraryExA
```

Таким образом, `hitools.dll` не реализует собственную SQLite-библиотеку. Его DB-операции делегируются `elmen.dll`, а копирование, удаление и чтение файлов выполняются через Win32 API.

## Package Paths

В бинарнике присутствуют следующие относительные пути:

```text
int\hiasm.db
Elements\\
\\Elements.db
code\\hi
icon\\
conf\\
code\\
```

Пути показывают, что DLL работает относительно каталога установки HiAsm и обслуживает стандартную структуру пакета:

```text
<package>\Elements.db
<package>\code\hi<element>...
<package>\icon\...
<package>\conf\...
<package>\code\...
```

## RemoveElement

`RemoveElement @ 0x00405228` принимает минимум два стековых аргумента. По использованию регистров Delphi ABI первый аргумент находится по `[EBP+8]`, второй по `[EBP+0xC]`.

Подтвержденная последовательность:

1. Создается/нормализуется путь к пакету через внутренние строковые helpers.
2. К пути добавляется `Elements.db`.
3. Проверяется существование/доступность пакета через `FUN_004048D8`.
4. Формируются и удаляются файловые пути, связанные с элементом:
   ```text
   code\\hi...
   icon\\...
   conf\\...
   ```
5. Вызывается внутренний helper `FUN_00404C88`, который получает сведения о пакетах из глобальной базы.
6. Открывается package DB через импортированный `OpenDB`.
7. Формируется путь `code\\` и подготавливается контекст перечисления файлов.
8. Вызываются импортированные операции:
   ```text
   EnumFiles
   RemoveFile
   DeleteElement
   CloseDB
   ```

Декомпилированная логика имеет вид:

```text
if package_is_available:
    delete generated code/icon/config files
    prepare element/package context
    OpenDB()
    EnumFiles(...)
    RemoveFile(...)
    DeleteElement(...)
    CloseDB()
```

Точная сигнатура callback-а `EnumFiles`, а также точный порядок аргументов `RemoveFile` и `DeleteElement` должны восстанавливаться по импортным thunk-ам и по аналогичным вызовам в `HiAsm.exe`/`hiasm.dll`.

## Global Package Enumeration Helper

`FUN_00404C88` является внутренним helper-ом, который возвращает/заполняет данные о пакетах:

```text
FUN_00404C88(package_or_path, output_context)
```

Подтвержденная последовательность внутри функции:

1. Нормализуется входной строковый параметр.
2. Вызывается `FUN_00404438`, после чего результат сохраняется в глобальном/внутреннем контексте `DAT_004074F4`.
3. К контексту добавляется строка:
   ```text
   int\\hiasm.db
   ```
4. Вызывается импортированный `OpenDB`.
5. Вызывается импортированный `EnumPacks`.
6. Вызывается импортированный `CloseDB`.
7. Полученный контекст копируется во второй параметр.

Это подтверждает, что `hitools.dll` использует глобальную базу `int\\hiasm.db` для разрешения/перечисления пакетов перед операциями над package-level `Elements.db`.

## InstallElement, LinkFile And InstallPackage

На текущем этапе точные тела трех экспортов не распознаны Ghidra как функции, но их назначение подтверждается экспортами, строками и DB imports:

```text
InstallElement  установка/копирование описания и файлов элемента
LinkFile        добавление файла и связывание его с элементом через elmen.dll
InstallPackage  установка структуры пакета и его Elements.db
```

Ожидаемая ответственность по импортам:

```text
InstallPackage:
  создать/скопировать каталог пакета
  создать или скопировать Elements.db
  зарегистрировать пакет через глобальную DB

InstallElement:
  скопировать code/icon/conf файлы элемента
  добавить группу/элемент в package DB

LinkFile:
  добавить файл в таблицу files
  связать file_id с el_id через files_link
```

Эта часть является рабочей реконструкцией по ABI и файловым строкам. Точные параметры, callback-сигнатуры и условия ошибок требуют ручного анализа export thunk-цепочки или вызовов из хост-приложения.

## ABI Notes

Все экспортированные функции имеют Delphi-совместимый Win32 ABI. Ghidra не восстановила для них надежные C-сигнатуры автоматически. Для `RemoveElement` аргументы читаются из стандартных стековых позиций `[EBP+8]` и `[EBP+0xC]`.

Импортированные DB-функции представлены в `hitools.dll` как внешние символы, поэтому их вызовы можно сопоставлять с экспортами `elmen.dll`:

```text
hitools.dll -> elmen.dll -> SQLite/database files
hitools.dll -> Win32 file APIs -> package files
```

## Relation To Other DLLs

```text
elmen.dll:
  SQLite-backed database API
  Elements.db and int\\hiasm.db access

hitools.dll:
  install/remove/link operations
  package file management
  calls elmen.dll DB API

core.dll:
  runtime element creation and initialization
```

`hitools.dll` не является обработчиком сообщения `0x403` и не содержит оконного UI-пути главного HiAsm. Его назначение ограничено операциями над пакетами, элементами, связанными файлами и базами данных.

## Conclusions

1. `hitools.dll` является файловым и package-management слоем исторического HiAsm.
2. Он динамически/импортно использует DB API `elmen.dll`, а не содержит собственную SQLite-реализацию.
3. `RemoveElement` подтверждает очистку generated `code`, `icon` и `conf` файлов перед удалением элемента из DB.
4. Удаление элемента использует `OpenDB`, `EnumFiles`, `RemoveFile`, `DeleteElement` и `CloseDB`.
5. `FUN_00404C88` открывает `int\\hiasm.db`, перечисляет пакеты через `EnumPacks` и возвращает подготовленный контекст.
6. `InstallElement`, `LinkFile` и `InstallPackage` экспортированы, но часть их тел в текущем Ghidra-сеансе представлена как data/thunk entries; их точные сигнатуры пока не доказаны.
7. Взаимодействие DLL можно представить так:
   ```text
   hitools.dll -> elmen.dll -> SQLite
   hitools.dll -> Win32 file APIs
   ```
