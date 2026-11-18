# hiasm.dll: Ghidra Research

## Scope

Исследован открытый в Ghidra исторический файл `hiasm.dll`. По экспорту и внутренней структуре это не основной `HiAsm.exe` и не `elmen.dll`, а отдельный Win32/i386 SDK/editor plugin для визуального редактора элементов HiAsm.

Размер исследованного image по сегментам: адресное пространство от `0x10000000` до `0x10031fff`. Основной код находится в `.text` (`0x10001000-0x100261ff`), данные в `.data`, `.rdata`, `.bss`; присутствуют обычные PE-секции `.edata`, `.idata` и `.reloc`.

## Exported SDK API

Ghidra показывает следующие именованные экспорты:

```text
sdk_command    0x100012d0
sdk_init       0x100011d0
sdk_msg        0x100012a0
sdk_paint      0x10001290
```

Также присутствуют ordinal-алиасы:

```text
Ordinal_1      -> sdk_command
Ordinal_2      -> sdk_init
Ordinal_3      -> sdk_msg
Ordinal_4      -> sdk_paint
sdk_paint@4    -> sdk_paint
```

Наблюдаемые интерфейсы имеют следующий смысл:

```text
sdk_init(HWND)
sdk_msg(message pointer)
sdk_command(command string, output/context pointer)
sdk_paint(...)
```

Точные Pascal/C++ типы структур пока не восстановлены. Ghidra определяет `sdk_init`, `sdk_msg` и `sdk_command` как `__cdecl`; для `sdk_paint` виден декорированный экспорт `sdk_paint@4`, поэтому его calling convention требует отдельной проверки по вызывающему коду.

## DLL Entry And Lifetime

`entry @ 0x10001000` является стандартным DLL entry point с параметрами `hModule`, `fdwReason`, `lpReserved`.

При `DLL_PROCESS_ATTACH` функция:

1. выделяет `0x80` байт через `malloc`;
2. сохраняет глобальные указатели на таблицу callback-ов;
3. вызывает внутреннюю инициализацию рантайма;
4. затем вызывает внутреннюю регистрацию/проверку состояния DLL.

При завершении процесса DLL проходит по зарегистрированным callback-ам, вызывает их, освобождает таблицу и выполняет `fflush(NULL)`.

Обработка `DLL_THREAD_DETACH`/аналогичного пути также проходит через зарегистрированные callback-ы, если таблица существует. Точная семантика вспомогательных функций инициализации требует дальнейшей работы с CRT-конструкторами.

## sdk_init

` sdk_init(HWND hwnd)` создаёт главный внутренний объект редактора через:

```text
FUN_10022520(0x50)
FUN_10003110(object, hwnd)
```

Созданный объект сохраняется в глобальном указателе `DAT_1002a024` и используется всеми последующими экспортами.

`FUN_10003110` выполняет следующие действия:

- сохраняет переданный `HWND`;
- получает DC окна через `GetDC`;
- создаёт белую кисть и жёлтое перо;
- создаёт bitmap и memory DC для буферизованной отрисовки;
- создаёт шрифт `Time New Roman`;
- инициализирует панель команд;
- регистрирует команды `back`, `forward`, `delete`, `copy` и разделитель `-`;
- настраивает горизонтальную и вертикальную прокрутку;
- загружает и открывает базу:

```text
Elements\Delphi\Elements.db
```

Последовательность загрузки базы выполняется через внутренний объект `FUN_10009e20` и затем создаётся редакторный контрол `FUN_10008b20`.

## Database DLL Loading

В `FUN_10009e20` обнаружен динамический DB loader:

```c
DAT_1002a050 = LoadLibraryA("elman.dll");
DAT_1002a04c = GetProcAddress(DAT_1002a050, "OpenDB");
DAT_1002a048 = GetProcAddress(DAT_1002a050, "CloseDB");
DAT_1002a044 = GetProcAddress(DAT_1002a050, "EnumElements");
DAT_1002a040 = GetProcAddress(DAT_1002a050, "EnumTabs");
```

После этого функция:

1. преобразует путь к базе в используемый внутренний строковый объект;
2. вызывает `OpenDB`;
3. вызывает `EnumTabs` с callback `LAB_1000a170`;
4. вызывает `CloseDB`.

Важное наблюдение: строка в исследованной DLL записана именно как:

```text
elman.dll
```

Это отличается от исторически известного имени `elmen.dll`. В текущем бинарном анализе это не исправлено и не интерпретируется как опечатка автоматически. Возможные объяснения: отдельная DLL с именем `elman.dll`, ошибка в конкретной исторической сборке или артефакт несовпадающих версий файлов. Для совместимости имя нужно подтвердить по фактическому каталогу установки и поведению загрузчика.

`DAT_1002a044` (`EnumElements`) используется в отдельной функции `FUN_10009880`: она лениво открывает базу через `OpenDB`, если поле `param_1[4]` ещё не установлено, вызывает `EnumElements(*param_1, &LAB_10009ac0, param_1)`, а затем закрывает базу. Первый аргумент берётся из поля внутреннего объекта; его точная семантика пока не доказана как числовой ID.

Callback `LAB_10009ac0` увеличивает счётчик, расширяет массив указателей и создаёт внутреннюю запись размером `0x18` байт. Из callback-параметров копируются строковые значения, после чего по одному из полей загружается иконка. Это подтверждает, что `EnumElements` используется для построения списка элементов редакторного контрола, а не только для проверки наличия записи. `EnumTabs` используется раньше, в `FUN_10009e20`, через callback `LAB_1000a170`.

## sdk_msg

`sdk_msg(int *message)` передаёт сообщение внутреннему редакторному объекту:

```text
FUN_10006270(DAT_1002a024, message)
```

Поддержанные Win32-сообщения, видимые в декомпиляции `FUN_10006270`:

```text
WM_MOUSEMOVE       0x0200
WM_LBUTTONUP       0x0202
WM_LBUTTONDBLCLK   0x0203
WM_RBUTTONDOWN     0x0204
WM_RBUTTONUP       0x0205
WM_PAINT           0x000f
WM_SIZE            0x0005
WM_HSCROLL         0x0114
WM_VSCROLL         0x0115
```

Сообщение передаётся в формате, совместимом с массивом полей, где первое поле содержит `message`, второе используется как `wParam`, а третье как `lParam`. Для мыши координаты извлекаются из младшего и старшего 16-битных слов третьего поля.

Обработка прокрутки ограничивает позиции значением `999`, обновляет внутренние координаты, scrollbar-и и вызывает перерисовку через `ScrollWindowEx`.

В просмотренной функции обработчик сообщения `0x403` не обнаружен. Это означает, что `0x403` из анализа single-instance `HiAsm.exe` не следует автоматически связывать с этим SDK-плагином.

## sdk_command

`sdk_command(char *command, undefined4 *context)` создаёт временный строковый объект из команды и передаёт её в:

```text
FUN_10007210(DAT_1002a024, command_object, context)
```

После вызова временный объект освобождается. Внутренний диспетчер `FUN_10007210` сравнивает имя команды и подтверждает следующие операции. Поле `param_2[1]` проверяется перед сравнением и служит признаком валидной/непустой строки команды:

```text
back
forward
delete
copy
paste
openfile
savefile
open
save
exit
```

Для `back`, `forward`, `copy`, `paste` выполняются операции над текущим элементом/историей. `delete` удаляет выделенный объект. `openfile` и `savefile` принимают путь из третьего аргумента. `open` и `save` используют стандартные диалоги файла и затем передают выбранный путь во внутренний загрузчик/сохранение. `exit` вызывает `ExitProcess(0)`.

Проверка `param_2[1] != 0` показывает, что второй параметр строкового объекта используется как признак непустой команды. Третий параметр `sdk_command` передаётся как внешний путь или контекст для команд, которым нужны данные.

Команды `back`, `forward`, `delete`, `copy` и разделитель `-` также регистрируются в панели команд внутри `FUN_10003110`; `paste`, `openfile`, `savefile`, `open`, `save`, `exit` доступны через программный command ABI.

## sdk_paint

Декомпиляция `sdk_paint` в текущем анализе не показывает полезного тела и сразу возвращает управление. Реальная отрисовка выполняется через обработку `WM_PAINT` внутри `FUN_10006270`, которая вызывает `FUN_10004aa0` с `PAINTSTRUCT` и HDC.

Это может означать, что экспорт оставлен как ABI-заглушка для хоста, а отрисовка обычно инициируется через `sdk_msg(WM_PAINT)`.

## GUI And Element Model

В `.rdata` присутствуют имена C++-типов, подтверждающие объектную модель редактора:

```text
TElement
TMultiElement
TEditMulti
TGetDataEx
TITElement
TLTElement
THubEx
TCommandCtrl
TIconManager
TSizeElement
TToolBarCtrl
TCaptionElement
TDynPointElement
TWorkDynPointElement
THList
THStrList
TMenuCtrl
```

Также найдены строки, связанные с описанием элементов и SDK-данными:

```text
DPElement
GetDataEx
MultiElement
EditMulti
LTElement
HubEx
ITElement
BEGIN_SDK
END_SDK
onEventData
doWork
doWork1
doWork2
doWork3
onEvent
```

Это подтверждает, что DLL содержит не только простой DB browser, а редакторный runtime для работы с элементами, событиями, точками соединений, меню и SDK-описаниями.

## Imported Dependencies

Основные группы импортов:

- Win32 GUI/GDI: `BeginPaint`, `EndPaint`, `GetDC`, `CreateCompatibleBitmap`, `CreateCompatibleDC`, `BitBlt`, `DrawTextA`, `Rectangle`, `SetScrollInfo`, `ScrollWindowEx`;
- окна и ввод: `SetFocus`, `InvalidateRect`, `SetTimer`, `KillTimer`, clipboard API, file dialogs;
- загрузка и синхронизация: `LoadLibraryA`, `GetProcAddress`, `CreateSemaphoreA`, `WaitForSingleObject`, TLS API;
- CRT: `malloc`, `free`, `realloc`, `memcpy`, `memset`, `strcmp`, `strcpy`, `strlen`, `fprintf` и другие.

Наличие `LoadLibraryA`/`GetProcAddress` используется именно для runtime DB DLL, а не для SQLite: в этом бинаре строки SQLite API не обнаружены.

## Relation To Other Historical DLLs

По наблюдаемым функциям ответственность можно разделить так:

```text
HiAsm.exe   основной host, окно, проект и межмодульная координация
hiasm.dll   SDK/editor plugin, GUI-контрол и работа с Elements.db
elmen.dll   исторически известный DB API для Elements.db/hiasm.db
core.dll    runtime element functions
hitools.dll package/element installation tools
```

Главное расхождение, требующее проверки совместимости: `hiasm.dll` в текущем бинаре пытается загрузить `elman.dll`, тогда как ранее исследованный DB-модуль называется `elmen.dll`.

## Conclusions

1. `hiasm.dll` экспортирует SDK ABI из четырёх функций: `sdk_init`, `sdk_msg`, `sdk_command`, `sdk_paint`.
2. `sdk_init` создаёт редакторный объект, GUI-ресурсы и открывает `Elements\\Delphi\\Elements.db`.
3. DLL динамически разрешает `OpenDB`, `CloseDB`, `EnumElements`, `EnumTabs`; перечисление выполняется через callback-функции `LAB_1000a170` и `LAB_10009ac0`.
4. В бинаре зафиксирован строковый loader path `elman.dll`, что не совпадает с ранее исследованным `elmen.dll` и должно быть проверено по конкретной установке.
5. `sdk_msg` обрабатывает стандартные GUI-сообщения окна, мышь, paint, resize и scrollbar-и; `0x403` в просмотренном пути не найден.
6. `sdk_command` предоставляет команды редактирования, навигации, открытия и сохранения файлов, а также завершения процесса.
7. Внутренняя модель является C++-объектной и содержит элементы, multi-elements, hubs, точки, меню, toolbar и SDK callbacks.
8. `EnumElements` лениво строит массив внутренних записей элементов; callback копирует строковые поля и загружает иконки.
9. Для полного восстановления ABI остаются задачи: определить структуры `message`, `context` и DB callback, а также установить, существует ли исторический `elman.dll` отдельно от `elmen.dll`.
