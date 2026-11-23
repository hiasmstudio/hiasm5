# x86 CodeGen bridge для пакетов HiAsm 4

Пункт CHECKLIST: «Реализовать отдельный x86 CodeGen bridge и подключение внешнего Delphi formedit; Wine не заменяет этот протокол».

## Проверенные факты (2026-11-05)

- Delphi-пакет HiAsm 4 (`Elements/delphi/CodeGen.dll`, `make/make_*.dll`) собран как PE32 i386. x64 IDE не может загрузить эти DLL.
- CGT содержит 85 функций. Их порядок в `Elements/CGTShare.pas` HiAsm 4 совпадает с таблицей `CGT[]` в [src/CodeGen.cpp](../src/CodeGen.cpp). Функции CGT используют `stdcall`, точки входа CodeGen и make используют `cdecl`.
- `buildMakePrj` и `buildCompliteProc` получают указатель на запись. Это видно в дизассемблированном `make_exe.dll`: `mov eax,[ebp+8]`.
- `buildProcessProc` возвращает в `result` Delphi-запись `MainForm: string; Units, IBody: PStrList` (списки KOL). Её читает только `make_*.dll`, поэтому CodeGen и make работают в одном x86-процессе.
- CodeGen записывает `def.inc` и `hi<Name>.pas` в каталог `GetParam(PARAM_CODE_PATH)`. Внешний пакет в IDE доступен только для чтения, поэтому мост возвращает там рабочий каталог.
- 9 модулей пакета подключают `def.inc` через `{$include}`, а Delphi ищет его рядом с исходником. Поэтому сборка идёт в копии всего каталога `code` (11 МБ, 894 файла).
- `IS_EDIT` стоит у элементов с секцией `[Edit]` в INI. `CodeGen.dpr` передаёт им родителя (`THIMainForm.Create(nil)`) и вызывает `Init`; без флага вызов не компилируется. `EditMulti` и `EditMultiEx` секции `[Edit]` не имеют, их `Create` без параметров.
- Ресурсы (по коду `HiAsm.exe`, методы ресурсов около `0x5697c0`, `resAddIcon` около `0x554768`). Пустая иконка даёт имя `ASMA`. Строка `ASMA ICON "..\int\main.ico"` попадает в `allres.rc`, только если у главного элемента нет иконки; иначе форма получает `LoadIcon(hInstance,'')` и падает с Access violation. Остальные строки: `<pref>ICON<n> ICON`, `<pref>BITMAP<n> BITMAP`, `<pref>WAVE<n> 100 LOADONCALL`, `<pref>STREAM<n> 100`, `<pref>Str<n> 100` (счётчик строк начинается с 1). `Share.pas` читает тип 100 как сырые байты.
- Данные ресурсов в `.sha` записаны как `[ZIP<размер LE, 4 байта><zlib>]` или `[<hex>]`. Внутри лежит готовый файл (`.ico`, `.bmp` с заголовком `BM`).
- Сопоставление проектов с make-DLL и команды компиляторов хранятся в `Int/hiasm.db` HiAsm 4 (таблицы `projects`, `compilers`, `pack_compilers`). `MainForm` → `make_exe.dll`, расширение результата `exe`; активный компилятор — `compiler\delphi\dcc32.exe "%fname%" "-U%opath%." -Q`. Расширение в `Elements.db` пакета не хранится, его читают из `Int/hiasm.db`.
- dcc32 из HiAsm 4 не загружает `RLINK32.DLL`, если его запускать через glib spawn. При запуске через `CreateProcess` с рабочим каталогом компилятора он работает.
- FTCG-элементы: CodeGen загружает `GetStartDir + 'Elements\Delphi\FTCG_CodeGen.dll'`, где `GetStartDir` — каталог процесса-хоста.
- x86-инструменты: MinGW i686 не установлен, встроенный в HiAsm 4 FPC 1.9.6 урезан под KOL. Мост написан на FPC 3.2.2 i386-win32, который `tools/setup_fpc_windows.ps1` ставит в `build/deps`; MSYS2 не меняется. Таблица CGT описана заново в `src/hiasm4_cgbridge/Cgt.pas`: в `src/FTCG/CGTShare.pas` порядок полей тот же, но соглашение `cdecl`.
- В `Elements/delphi/make` нет `formedit.exe`, и в установке HiAsm 4 его тоже нет. Формы редактирует встроенный редактор HiAsm 4, в этом проекте это `FE_Legacy` (`src/LegacyFormEditor.cpp`).

## Архитектура

```
IDE (x64), src/LegacyBuild.cpp          hiasm4_cgbridge.exe (x86, FPC)
1. копия <пакет>/code в work
2. копия моста и FTCG_CodeGen.dll в work\host (раскладка HiAsm 4)
3. sdkSerialize() -> scheme.json  -->  CodeGen.dll + make_<тип>.dll: .dpr, hi*.pas,
                                       файлы ресурсов, allres.rc
4. GoRC /r allres.rc -> allres.res
5. компилятор из hiasm.db (CreateProcess)
6. --complete                     -->  buildCompliteProc: перенос результата
```

Командные строки моста: `--codegen <dll> --make <dll> --scheme <json> --work <dir> --project <файл.dpr> [--rc <allres.rc> --default-icon <main.ico>]` и `--complete --make <dll> --project <файл.dpr> --app <файл>`. Строки stdout используют префиксы `cgt_on_debug`: `!` ошибка, `#` трассировка, `@` информация.

Сериализатор ([src/SdkSerializer.cpp](../src/SdkSerializer.cpp)) заранее вычисляет ответы в семантике HiAsm 4: флаги `IS_*`, признак значения по умолчанию, имена динамических точек, реальные связи через хабы, значения `data`/`array`/`font`. Свойства-ресурсы передаются исходным текстом HiAsm 4.

`buildLegacyProject()` ([src/LegacyBuild.h](../src/LegacyBuild.h)) выполняет всю цепочку; `legacyActiveCompiler()`, `legacyProjectMaker()` и `legacyProjectExtension()` читают `Int/hiasm.db`. Подстановки в команде компилятора: `%fname%` — `.dpr` в work, `%opath%` и `%upath%` — work, `%curdir%` — корень HiAsm 4, `%oname%` — имя результата.

IDE ([src/ProjectBuilder.cpp](../src/ProjectBuilder.cpp)): `ProjectBuilder::build` для схемы legacy-пакета вызывает `buildLegacyScheme` → `buildLegacyProject()`. Корень установки отдаёт `Pack::legacyRoot()`, мост ищется рядом с программой (`dataDir\hiasm4_cgbridge.exe`). Результат `build/<Имя>.exe` лежит рядом со схемой; `ProjectBuilder::run` запускает его. Рабочий каталог удаляется после успешной сборки, после ошибки остаётся для разбора.

## Этапы

1. **Генерация — сделано (2026-11-05).** Сериализатор, мост с 85 функциями CGT, цепочка `buildPrepareProc` → `buildProcessProc` → `buildMakePrj`.
2. **Компиляция — сделано (2026-11-05).** Ресурсы, GoRC, компилятор из `hiasm.db`, `buildCompliteProc`. `cgbridge_test` собирает `new/MainForm.sha` настоящими `CodeGen.dll`, `make_exe.dll`, GoRC и dcc32 из HiAsm 4, запускает exe, проверяет окно с заголовком «Тест» и закрывает его. Второй сценарий задаёт иконку в формате `[ZIP…]` и проверяет `ICON0` в `allres.rc` и в коде, а также побайтное совпадение файла иконки.
3. **IDE — сделано (2026-11-14).** Сборка и запуск legacy-схем через `ProjectBuilder` (см. «Архитектура»). `cgbridge_test` собирает копию `Example/Info.sha` через `ProjectBuilder::build` и запускает `build/Info.exe`.
4. **FTCG и formedit — частично (2026-11-14).** Каталог-хост с раскладкой HiAsm 4 даёт `FTCG_CodeGen.dll` для `GetStartDir`: генератор загружается, и `Example/FTCG/SpeedTest.sha` собирается и запускается. Formedit отдельным `formedit.exe` не нужен: форма редактируется встроенным `FE_Legacy`. Открытые проблемы — ниже.

## Сборка и проверка

FPC 3.2.2 ставится в `build/deps` скриптом `tools/setup_fpc_windows.ps1` (скачивает с freepascal.org, проверяет SHA-256). Мост собирается при `-DHIASM_BUILD_CGBRIDGE=ON` (по умолчанию выключено):

```bash
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/setup_fpc_windows.ps1
cmake -DHIASM_BUILD_CGBRIDGE=ON -DHIASM4_TEST_ROOT="C:/Program Files (x86)/HiAsm" <каталог сборки>
```

Тест `cgbridge` требует `-DHIASM4_TEST_ROOT=<каталог HiAsm 4>`; без него он пропускается (код 77). Тест проверяет `new/MainForm.sha` (окно «Тест», иконка), `Example/Info.sha` через мост и через `ProjectBuilder` (окно «Eorm») и `Example/FTCG/SpeedTest.sha` (окно «:(»).

Если бинарника моста нет, IDE сообщает: «The 32-bit CodeGen bridge is not built; configure with -DHIASM_BUILD_CGBRIDGE=ON».

## Исправлено на этапе 4 (2026-11-14)

- `IS_EDIT` ставится только у элементов с секцией `[Edit]`. Раньше его получали `EditMulti`/`EditMultiEx`, и генератор передавал им родительский контрол: `Too many actual parameters` (`src/SdkSerializer.cpp`).
- Методы вида `doStart%Precision%` сериализуются как `doStart<индекс значения>` (`doStart0` при `Precision = Millisecond`). Генератор строит имена `_work_<имя>` по точке (`src/SdkSerializer.cpp`). Раньше имя терялось, и генератор ссылался на несуществующий `_work_doStart`.
- Разбор `%свойство%` в конфиге брал в имя свойства закрывающий `%` (`Precision%`), поэтому индекс не находился (`src/ElementConfig.cpp`).
- Legacy-элементы с `Sub=` (InlineCode и др.) создаются как `ElementDynPoints`; свойство-список (`data_list`) даёт по точке на каждое имя (`syncNamedPoints` в `src/Elements.cpp`). Раньше такие точки не создавались, и схема `TechSpeedReport` не загружалась.
- Расширение результата берётся из `Int/hiasm.db` (`projects.ext`), а не из `Elements.db`, где его нет.

## Открытые проблемы

- **`Example/FTCG/ReplaceChars.sha`.** Сгенерированный `hiFTCG_Tools_*.pas` объявляет `arr8: array[0..0] of Char = ('a`, перевод строки, `b`, …`)`: многострочный текст вместо массива. Delphi отвечает `Incompatible types: 'Char' and 'String'`. Генератор FTCG строит массивы через `lang(arr:array)` и `sub(arr, 13 & 2)` (`code/hiFT_StringToArray.hws`), вход — `DoData` с `Data=String(abcdef)`. Причина не установлена: нужно сравнить вывод с оригинальным HiAsm 4 на этой схеме.
- **`Example/FTCG/TechSpeedReport.sha`.** Схема загружается, но генерируемый `hiMainForm.pas` использует модуль `hiInlineCode`, которого нет: элемент `InlineCode` получает код через пользовательский `Code` (`unit HiAsmUnit`), и Delphi-генератор не записывает для него `hiInlineCode.pas`. Нужно выяснить, какой модуль CodeGen создаёт для элементов с пользовательским кодом.
- Оба случая не попадают в `cgbridge_test`, потому что нет эталона для сравнения.

## Ограничения

- Linux (Wine) не входит в этапы 1–4.
- Проверены: `new/MainForm.sha` (одна форма, dcc32), `Example/Info.sha` (кнопка, сообщение, подсказки), `Example/FTCG/SpeedTest.sha` (FTCG, таймер). Остальные типы проектов и FPC из `hiasm.db` не проверены.
- Меню как ресурс (`resAddMenu`) не поддерживается; Delphi CodeGen эту функцию не вызывает.
- Пути рабочего каталога и результата должны быть представимы в кодовой странице ANSI: KOL в DLL пакета открывает файлы через ANSI API. Если это не так, мост сообщает об ошибке.
- Каталог `code` копируется при каждой сборке.
- Установщик упаковывает всё содержимое `BUILD_DIR`. Мост в корне сборки попадает в установку, но проверено это не было.

## Пробелы IDE, найденные на этапе 1 — исправлены (2026-11-05)

- Legacy-цвета (`clBtnFace` и др.) и пустой шрифт по умолчанию теперь загружаются как в HiAsm 4 ([src/ElementProperty.cpp](../src/ElementProperty.cpp), флаг `legacy` у `DataValue::parse`). CodeGen выводит `_prop_Color := -16777201;` (`clBtnFace` = `$FF00000F`) и `hiCreateFont('MS Sans Serif',8,0,0,1)`; `cgbridge_test` это проверяет.
