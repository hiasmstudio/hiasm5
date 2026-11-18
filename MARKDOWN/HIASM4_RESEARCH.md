# HiAsm 4: структура, пакеты, Ghidra и RTCG

Исторический снимок: 2026-10-26. Текущие команды и проверки см. `COMMAND_AUDIT.md` и `CHECKLIST.md` (актуализация 2026-11-02). Связано с пунктами 15–19 `CHECKLIST.md`.

Источники:
- установка `C:\Program Files (x86)\HiAsm` (версия `HiAsm_4.05b186`);
- Ghidra, `HiAsm.exe` (Delphi-приложение);
- базы `Int\hiasm.db` (v4) и `resources/hiasm.sql` (v5, SQL-дамп);
- журнал изменений `fixes.txt` (кодировка CP1251);
- GitHub `hiasmstudio/RTCG`, ветка `master`, GPL-3.0.

## 1. Структура установки (пункт 16)

Всего 11 897 файлов, 211.5 МБ.

| Папка | Файлов | Содержимое |
|---|---:|---|
| `Elements\` | 11 222 | паки элементов (таблица ниже) |
| `compiler\` | 272 | компиляторы и инструменты: `Delphi`, `fpc`, `FlatAsm`, `arm`, `GoRC.exe`, `Py2Exe.exe`, `PyCode.exe` |
| `Int\` | 294 | `hiasm.db` (74 КБ), `hiasm.sql` (108 КБ), иконки, курсоры, языки `en`/`ru`, `autorun.sha` |
| `Help\` | 26 | `HiAsm.chm` (3.5 МБ) и HTML-справка по core-элементам |
| `Tutorial\` | 14 | учебные схемы `.sha`: Intro, Element, Hub, MultiElementEx, Stack, Types и др. |
| `Plug\` | 23 | плагины и утилиты: `vicx`, `Hook.dll`, `frame.dll`, `png2ico.exe`, `sqlite3.exe` |
| `SVN\` | 19 | клиент `hisvn.exe` и библиотеки SlikSvn |

Корень: `HiAsm.exe` (2.2 МБ), `hiasm.dll`, `core.dll`, `elmen.dll`, `sqlite3.dll`, `libmySQL.dll`, `HiUpdate.exe`, `WikiCenter.exe`, `ValueChanger.exe`, `findcomponent.exe`, `fixes.txt`, `version`.

Паки в `Elements\` (число `.ini` ≈ число элементов):

| Пакет | `.ini` | `.ico` | `.sha` | `.pas` | Файлов всего |
|---|---:|---:|---:|---:|---:|
| delphi | 798 | 1032 | 355 | 843 | 3229 |
| CNET | 880 | 1153 | 545 | 0 | 3755 |
| vbs | 286 | 297 | 39 | 1 | 963 |
| web | 239 | 243 | 23 | 1 | 779 |
| QT | 188 | 178 | 36 | 1 | 637 |
| Python | 176 | 208 | 14 | 0 | 589 |
| PocketPC | 98 | 98 | 10 | 0 | 322 |
| _base | 91 | 93 | 0 | 0 | 184 |
| Fasm | 69 | 67 | 1 | 0 | 219 |
| Modules | 59 | 58 | 8 | 1 | 184 |
| wxMSW | 52 | 7 | 19 | 1 | 158 |
| PDA | 38 | 47 | 3 | 3 | 143 |
| RTCG | 0 | 0 | 0 | 0 | 22 (исходники C++) |
| RTCG_Win_fork | 0 | 0 | 0 | 0 | 26 (исходники C++) |
| FTCG | 0 | 0 | 0 | 2 | 4 (Delphi CodeGen) |

## 2. Сравнение баз v4 и v5 (пункт 18)

- `commands`: 144 (v4) и 143 (v5). Только в v4: `activated_key` (`fixes.txt`: «add: activate key dialog», 20.01.2011).
- `params`: 129 и 129, имена совпадают.
- `menus`: 17 и 17, имена совпадают.
- `packs`: v4 — 13 паков (delphi, cnet, web, Fasm, PocketPC, Modules, GCC, QT, darkbasic, avr, vbs, PDA, Python). v5 — 5 записей (delphi, web, PocketPC, QT, _base). Только в v4: cnet, Fasm, Modules, GCC, darkbasic, avr, vbs, PDA, Python. В v5 записи delphi, web, PocketPC и QT есть в БД, но каталогов в `packs/` нет: `packs/` содержит только `_base`.

Команды v5 без обработчика в коде: имя команды не встречается ни в одном `src/*.cpp`, кроме `commands.cpp`. Таких 88 из 140 `CMD_*`:

```
print reopen projects elements navigate debug cmdedit stinfo capture components help dmaster
sha_source web_back web_forward web_refresh rooms upload close closeall hichat pn_debug pn_help
pn_info undo group ungroup align compilers pack tutor moveto addtemp usetemp media addppoint
makeext hub_ex break dodata panels connectpoint redo elmoveup elmovedown delelement add_tab
del_tab makelement translate public_sha update_sha list_sha clone make_wmdock wm_split pageup
pagedown replaceto paste_dodata paste_strcat paste_hub clear_link rename_point insert_point
move_point show_lh qpan_add qpan_addbtn qpan_show qpan_showall qpan_hideall bind_rect bind_center
bind_padding del_point pn_errors pal_filter svn svn_update svn_log svn_show debug_a pn_rep
savetorep searchnext icons decom_pal
```

Часть из них видна в меню и неактивна: `undo`, `redo`, `align`, `searchnext` (Editor), `group`, `ungroup`, `moveto`, `replaceto` (Edit), `projects`, `elements`, `pn_*` (View). Это не «нереализованные пункты», а пункты меню без обработчика.

Core-элементы. В справке v4 (`Help\*.html`) есть темы, для которых в v5 нет класса `ElementCore*` с таким именем: CallDLL, ClipboardHook, Context, DropFile, GlobalVar, Hashsum, hiPlugs, MP3_info, ODBC, Position, Service, SMTP, Thread, UDP, VBScript, Zipper (16 тем). Всего в v5 40 классов `ElementCore*` (`src/Elements.h`). Соответствие по имени, не по функции.

Журнал v4 (`fixes.txt`): 125 записей `add:`. Примеры с латинскими названиями: `activate key dialog` (2011), `paste from clipboard` в редакторе растровых изображений (2010), `hide inherited propertys` (2010), `propSaveToFile` в codegen (2010), `elGetGroup`, `elGetInterface` (codegen), `support TableShape`, `support PrintShape` (2009), `document template technology` (2009), `english language` (2009), `interface for external form edit` (2008), `polimorf elements and codegen interface` (2008), `HCWinManager` (2008).

SVN. В v4 была интеграция с SVN (папка `SVN\`, запись «upg: change svn address», 2010). В v5 команды `svn`, `svn_update`, `svn_log`, `svn_show` есть в БД, но в коде не обрабатываются. Меню `pal_el_menu`, где они объявлены, в коде не используется.

## 3. Delphi-пакет v4: инвентаризация (пункт 19)

Каталог `Elements\delphi\`:
- `conf\` 795 элементов (`.ini`), `icon\` 1004 (`.ico`), `code\` 968 файлов, `Example\` 377 файлов (репозиторий примеров), `make\` 20 (модули сборки), `new\` 9 (шаблоны `.sha`), `tabs\` 23, `draw\` 18, `ulib\` 1; `tpls\` и `prof\` пустые.
- `Elements.sql` (82 КБ) и `Elements.db` (54 КБ): каталог элементов, тот же механизм, что `packs/_base/elements.sql` в v5.
- `CodeGen.dpr` (51 КБ), `CodeGen.dll` (57 КБ), `FTCG_CodeGen.dll` (89 КБ): генератор кода на Delphi.

Вывод для нового пакета: каталог элементов и примеры переносятся как данные (после проверки лицензии). Генерацию кода нужно писать заново на C++17 (см. раздел 4).

## 4. RTCG (пункт 15)

- Назначение (README): «Run Time Code Generator. Code generator for new projects of HiAsm IDE and Hion». Лицензия GPL-3.0. Ветка `master`, 33 коммита.
- Состав: `CGTShare.h` (интерфейс таблицы CGT), `Parser.*` (разбор текста), `CodeTree.*` (дерево кода), `Objects.*`, `IntFunc.*`, `FuncArgs.*` (встроенные функции и аргументы), `Runner.*` (выполнение блоков элемента), `direct.*` (логирование и описание языка `TLangRec`), `ElementData.*`, `dllmain.cpp` (экспорт плагина), `Makefile`, `Makefile.win`.
- Точка входа плагина (`dllmain.cpp`): `buildPrepareProc`, `buildProcessProc(TBuildProcessRec*)`, `CheckVersionProc`. Процесс: `ElementData::init()`, поиск родительского элемента (`ELEMENT_FLG_IS_PARENT`), вызов `run(e, "dostart")`, затем `finally_sdk`. Результат — блок `result` дерева кода (`asCode()`).
- Совместимость с HiAsm 5: `src/ProjectBuilder.cpp:57-85` загружает `libCodeGen.so` и вызывает те же имена экспортов с `TBuildProcessRec`. Имена совпадают. Поля структур и таблицу CGT (85 записей по разведке, `CodeGen.cpp`) надо сверить с `CGTShare.h` побайтно.
- Копии в установке v4: `Elements\RTCG` (22 файла, без `ElementData`) и `Elements\RTCG_Win_fork` (26 файлов, Windows-вариант с `codegen.rc`, `CodeGen.h`, `ElementData.*`).

## 5. Ghidra: HiAsm.exe (пункт 17)

- Секции: `.text` 0x401000–0x5E43FF, `.itext`, `.data`, `.bss`, `.idata`, `.tls`, `.rdata`, `.reloc`, `.rsrc` 0x621000–0x63DFFF.
- Функции работы с элементами в бинарнике: `AddElement` (0x54BD3C), `DeleteElement` (0x54BD44), `ElementExists` (0x54BD4C), `EnumElements` (0x54BD5C), `GetElementInfo` (0x54BD74).
- Загрузчик схем `.sha`: `FUN_0056B468`. На него указывает строка `"Make("` по адресу 0x56C3E4. Разбор построчный, распознаёт: `Make(`, `Add(`, блоки `{ }` со свойствами, `Point(`, `link(`, `elink(`, `BEGIN_SDK`/`END_SDK`, `@Group`, `@Freez`, `@IsLib`, `@Color`, `MakeExt(`, `MakeTrans(`, `AddHint(`, `PColor(`. Если пакет не указан, берётся `delphi` (строка «Name of package not found! Set into Delphi.»).
- Итог: ключевые слова `.sha` в HiAsm 4 и HiAsm 5 (`SDK.cpp`, `loadFromText`) совпадают, это подтверждает совместимость сохранённых схем.

## 6. Открытые вопросы

1. Лицензия переноса ассетов v4 (`conf`, `icon`, `Example`) в репозиторий. `LICENSE` — GPL-3.0, нужно подтверждение автора.
2. Пакет на базе RTCG или на новом генераторе. RTCG интерпретирует блоки элементов во время сборки, это влияет на архитектуру пакета.
3. Какие из 88 команд без обработчика реализовать первыми. В первую очередь `undo`/`redo` (меню Editor).

## Дополнение 2026-11-02

Реализованы history схемы, grouping с @Group, align/moveto, show_lh и UTF-8 searchnext. RTCG C++17 предлагает console, GTK4 GUI, shared library и daemon с Debug/Release профилями. FTCG собирается Win64 portable FPC, генерирует HTML/CSS/JavaScript/PHP с повторной проверкой освобождения результата.

Число 88 необработанных команд выше относится к старому снимку, а не текущей сборке. Полная совместимость v4 не достигнута: x86 bridge, 795 Delphi-элементов и GUI cross libraries остаются задачами. Автономный Windows toolchain и SVN уже реализованы и проверены.

## Обновление 2026-11-02

Каталог `packs/hiasm4/paths.ini` теперь используется как read-only catalog: пути нормализуются, переменные и CP1251 обрабатываются, дубликаты подавляются, native pack имеет приоритет. `legacy_pack_test` повторно прочитал 795/795 конфигураций установленного Delphi-пака и проверил открытие MainForm.sha и CP1251 text viewer. Это аудит загрузки ассетов, а не перенос Delphi-элементов в C++17.

C++17 пакет содержит 21 native элемент, включая control widgets GTK4, checkbox/progress bar, `Delay` и неблокирующий `Timer`. Text/state setters и чтение значений проверены с двумя окнами и удалением виджета; cross profiles и managed WSL/Wine запуск покрыты RTCG test flags. Автономный toolchain содержит манифест, upstream/license references и не полагается на исходный MSYS2 PATH.

Поэлементный реестр: `DELPHI_MIGRATION.md` и `DELPHI_MIGRATION.json`. Он включает 795 конфигураций и эффективные свойства/методы с наследованием. Только 13 компонентов имеют явно сопоставленные частичные C++-аналоги; 782 не имеют проверенного аналога. Имена не доказывают совместимость, поэтому полностью закрытых строк пока нет.

Linux → Windows GTK4 проверен bundled UCRT64 GCC под Wine: сгенерированный GUI выполняет callback таймера и завершается, IDE launch также проверен. GTK headers/libs из нативного Linux или Debian MinGW нельзя смешивать с этим UCRT closure. Адаптер использует DOS short path и GCC_EXEC_PREFIX/COMPILER_PATH; проверены пробелы, кириллица и апостроф. У Wine остаются предупреждения DWM/шрифтов; standalone cross-GUI runtime packaging и Windows → Linux GUI приёмка остаются открытыми.

`Timer` — новый GTK4-элемент с явным Start, Stop и Count, не полная эмуляция Delphi Timer: автоматический Enable, динамические Interval/AutoStop, OverCall и onStop Delphi Timer требуют отдельного переноса. Загрузка block.load и скриптов RTCG проверена в каталоге с кириллицей; недостающий runtime возвращает диагностику, не segfault.

## Уточнение статуса 2026-11-03

Windows → Linux GTK4 GUI теперь проверен в отдельном Debian 13 WSL/WSLg;
обратный путь использует bundled UCRT64 через Wine. Запуск, callback таймера,
естественное завершение и Stop покрыты RTCG. Автономная упаковка Windows GUI
проверена отдельно на Windows/Wine после перемещения в Unicode-каталог
без доступа к DLL компилятора; `tools/package_cpp_windows.py` сохраняет
DLL/data/SVG/иконки/лицензии и manifest SHA256.

В C++ пакете 23 элемента: добавлены Memory и Counter; Math расширен до 44
операций с default/clear, угловыми единицами, динамическими входами и ошибками.
Реестр: 15 частичных аналогов, 780 без проверенного аналога, 0 полностью
совместимых. Форматы variant, наследуемые свойства и имена точек Delphi пока
не эмулируются полностью. Пользователь разрешил перенос ассетов v4;
открытый вопрос о таком разрешении в старом разделе 6 больше не актуален.

Чтение 795 конфигураций и Ghidra-анализ не заменяют x86 CodeGen bridge.
Этот bridge и внешний Delphi formedit остаются открытыми; прямой вызов
32-битной DLL из 64-битной IDE намеренно запрещён с диагностикой.
