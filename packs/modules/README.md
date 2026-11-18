# Modules (HiAsm 4)

Пакет перенесён из `C:\Program Files (x86)\HiAsm\Elements\Modules`.

- Кодовое имя пакета: `modules`.
- Проекты: `HISEntryPoint`, `MakeEntryPoint`, `MainForm`.
- Все текстовые файлы (`conf`, `code/*.hws`, `new`, `example`, `prof`, `elements.sql`,
  `Modules.his`, `tabs.ini`) перекодированы из CP1251 в UTF-8.
- `elements.db` пересобран из UTF-8-версии `elements.sql`.
- `icon.png` получен из `Modules.ico`.
- Кодогенератор пакета — общий C++17 `RTCG` (`src/RTCG`). Секции `java`
  (`HISEntryPoint`) и `cpp` (`MakeEntryPoint`) описаны в `code/codegen.ini`.
- Дельфийские `CodeGen.dpr`, `direct.inc`, `errors.pas` и 32-битные
  `CodeGen.dll`/`make/make_mod.dll` удалены. `PackProject::make()` нативно
  записывает сгенерированный текст проекта в файл, поэтому пакет работает в
  64-битной сборке и на Linux.
