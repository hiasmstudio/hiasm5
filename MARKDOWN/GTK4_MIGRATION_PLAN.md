# План миграции HiAsm 5 на GTK4

| Параметр | Значение |
|---|---|
| Статус | GTK4 выбран основным Windows-бэкендом. GTK4 Debug/Release и GTK4 installer pipeline собираются; GTK3 остаётся доступным явным переходным профилем. UI smoke-тесты и чистая Windows остаются в работе. |
| Дата | 2026-11-03 (последнее обновление; исходная дата 2026-10-23) |
| Цель | Перевести UI HiAsm 5 с GTK3/gtkmm3 на GTK4/gtkmm4 без потери функций |
| Целевая версия | GTK 4.24.x, gtkmm 4.24.x |
| Компилятор | GCC UCRT64, C++17 |
| Стратегия | Поэтапно, с dual-backend `HIASM_GTK_VERSION=3\|4`; GTK4 используется по умолчанию, GTK3 остаётся рабочим явным переходным профилем |
| Оценка | 9–12 недель для одного разработчика; 6–8 недель при двух параллельных потоках |
| Основной риск | Кастомный canvas (`SDK_Editor`, `FormEditor`, `PropEditor`, `Palette`) и меню/тулбары |

## 1. Краткое резюме

- Переход выполняется на GTK4/gtkmm4, а не на Qt6: существующий код уже построен вокруг GTK/gtkmm, Cairo-рисования, `GtkSourceView` и `.sha`/`.db`-модели.
- Миграция не делается одним большим изменением. Сначала вводится compile-time переключатель и совместимый слой, затем по областям переносятся lifecycle, меню, canvas, контейнеры, диалоги и упаковка.
- GTK3-сборка должна оставаться зелёной до завершения Phase 6. Это позволяет выпускать исправления и сравнивать поведение.
- В gtkmm4 многие нужные классы не удалены, а помечены deprecated (`TreeView`, `ListStore`, `ComboBox`, `Dialog`, `StyleContext`). На первом проходе их не переписываем, а используем как временный мост.
- Критический слой — не сами виджеты, а события, `Dialog::run()`, `Gtk::Stock`, `Gtk::Container` и меню. Для них нужны отдельные compatibility-компоненты.

## 2. Рамки работ

### Входит в миграцию

- Windows UCRT64 x64 — основная платформа.
- Linux-сборка — поддерживаемая вторичная платформа.
- Сохранение форматов `.sha`, `.db`, паков и структуры `elements`.
- Функциональный паритет: схема, редактор формы, палитра, свойства, текстовый редактор, настройки, менеджер проектов, запуск и сборка проекта.
- Обновление CMake, пресетов, установщика и runtime deployment.
- Обновление `MARKDOWN/BUILD_WINDOWS.md` и `MARKDOWN/CHECKLIST.md`.

### Не входит

- Редизайн интерфейса и переход на новые GTK4-паттерны (`ListView`, `ColumnView`) без необходимости.
- Изменение формата проектов и паков.
- Qt6 и другие UI-фреймворки.
- 32-битные сборки.
- Полная замена deprecated-виджетов в первом релизе GTK4.

## 3. Текущее состояние

| Область | Состояние |
|---|---|
| Исходники | 67 файлов `.cpp`/`.h` (включая `gtkcompat.h`), около 20 300 строк на 2026-10-26 (на 2026-10-23: 33 + 33 файла, около 19 279 строк) |
| UI-стек | GTK4 4.24.1, gtkmm4 4.24.0 (`HIASM_GTK_VERSION=4`) используется по умолчанию; GTK3 3.24.52/gtkmm3 3.24.10 остаётся переходным профилем |
| CMake | `HIASM_GTK_VERSION=4` по умолчанию, `3` выбирает GTK3; C++17 для обоих профилей; `gtksourceviewmm-3.0` для GTK3 и `gtksourceview-5` для GTK4 на Windows и Linux |
| Windows | MSYS2 UCRT64, Ninja, presets `windows-mingw-debug` / `windows-mingw-release` (GTK3) и `windows-mingw-gtk4-debug` / `windows-mingw-gtk4-release` (GTK4) |
| GTK4-пакеты | Установлены `gtk4 4.24.1-1`, `gtkmm-4.0 4.24.0-1`, `gtksourceview5 5.22.0-1`; `pkg-config` подтверждает версии |
| Текстовый редактор GTK4 | Wrapper `src/GsvCompat.h` над C API GtkSourceView 5 используется на Windows и Linux; подсветка, поиск, история и сохранение проверяются `source_editor` и `editor_commands` |
| Тесты | Windows GTK4 — 21 CTest, GTK3 — 18, Linux GTK4 — 19; запуск с изолированным PATH, WSL/Wine, UI smoke и silent install/uninstall проверяются отдельно; результаты — раздел 10 |
| Установщик | NSIS, runtime DLL closure через `tools/deploy_gtk_runtime.ps1`; поддерживаются GTK4 и GTK3 профили |

### Проверенные API gtkmm4

Заголовки `gtkmm-4.0 4.24.0-1` проверены из кэша MSYS2. Основные выводы:

- `Gtk::Application`, `Gtk::ApplicationWindow`, `Gtk::Popover`, `Gtk::PopoverMenu`, `Gtk::AlertDialog`, `Gtk::FileDialog`, `Gtk::ColorDialog`, `Gtk::FontDialog` доступны.
- `TreeView`, `ListStore`, `CellRenderer`, `IconView`, `ComboBox`, `ComboBoxText`, `Dialog`, `FileChooserDialog`, `MessageDialog`, `ColorChooserDialog`, `AboutDialog`, `StyleContext` сохранены, но deprecated.
- `Gtk::Stock`, `Gtk::Menu`, `Gtk::MenuBar`, `Gtk::MenuItem`, `Gtk::ImageMenuItem`, `Gtk::Toolbar`, `Gtk::ToolItem`, `Gtk::ToolButton`, `Gtk::HBox`, `Gtk::VBox`, `Gtk::EventBox`, `Gtk::HandleBox`, `Gtk::ButtonBox`, `Gtk::VButtonBox`, `Gtk::HButtonBox`, `Gtk::Container`, `Gtk::Clipboard`, `Gtk::Main`, `Gtk::FileChooserButton`, `Gtk::ColorSelectionDialog` отсутствуют.
- `Gtk::Table` в коде фактически не используется; ранее найденное число 15 было ложным срабатыванием.
- `Gtk::Widget::show_all()`, `set_events()`, `get_window()`, `on_size_allocate()` и старые event-signal'ы отсутствуют.
- `Gtk::Widget::get_allocation()` ещё есть (deprecated с 4.12), `get_width()`/`get_height()` доступны.
- `Gtk::Dialog` не имеет `run()`; `get_content_area()` заменяет `get_vbox()`.
- `Gdk::Clipboard` работает асинхронно; синхронного `wait_for_text()` нет.
- `Gdk::Cursor` создаётся из `Gdk::Texture` и hotspot либо из имени; `Gdk::Pixbuf` нужно конвертировать через `Gdk::Texture::create_for_pixbuf()`.
- `Gtk::StyleContext::paint_*` заменены на `render_*`; `get_background_color()`/`get_border_color()` отсутствуют.
- `Gtk::Snapshot` поддерживает `append_cairo()`, что позволяет сохранить существующее Cairo-рисование.

### Масштаб изменений (на 2026-10-23, до миграции)

| Признак | Количество |
|---|---:|
| `Stock::` | 29 |
| `Gtk::Stock` | 26 |
| `StockID` | 11 |
| `ToolButton` | 28 |
| `TreeView` | 18 |
| `ListStore` | 22 |
| `ComboBox` | 18 |
| `ComboBoxText` | 8 |
| `HBox`/`VBox` | около 20 |
| `Gtk::Table` | 0 |

### Остаток GTK3-only API вне `gtkcompat.h` (2026-10-26)

Подсчёт по строкам `src/` (включая комментарии, поэтому приблизительно):

| Символы | Совпадений | Файлов |
|---|---:|---:|
| `Gtk::Stock` / `StockID` | 33 | 9 |
| `GdkEvent*` | 55 | 12 |
| `Toolbar` / `ToolButton` | 39 | 6 |
| `Gtk::Menu` / `MenuItem` | 31 | 7 |
| `EventBox` | 14 | 9 |
| `pack_start` / `pack_end` | 21 | 6 |
| `Glib::Thread` / `gdk_threads*` | 17 | 5 |
| `Gtk::Container`, `Gtk::Main`, `Gtk::Clipboard`, `get_vbox` | 0 | 0 |

## 4. Зафиксированные решения

| Решение | Обоснование |
|---|---|
| GTK4/gtkmm4, не Qt6 | Минимальный объём переписывания, сохранение Cairo-модели и текущих паков |
| `HIASM_GTK_VERSION=3\|4` | Позволяет вести миграцию по частям и сравнивать поведение |
| C++17 | Требование gtkmm4; старый код проверяется отдельной сборкой |
| Не определять `GTKMM_DISABLE_DEPRECATED` | `TreeView`, `ListStore`, `ComboBox`, `Dialog`, `StyleContext` используются как временный мост |
| `compat::run_dialog()` на `Glib::MainLoop` | Сохраняет синхронные вызовы `Dialog::run()` и поведение пользовательских схем |
| `StockIconMap` | Заменяет удалённые stock-иконки локальными PNG/icon names |
| `GsvCompat` над C API | GTK4 на Windows и Linux использует `src/GsvCompat.h` над GtkSourceView 5 |
| CSS вместо `gtkrc` | GTK4 не читает GTK3 RC-файлы |
| UI только в main thread | GTK4 строже к потокам; любые фоновые результаты — через `Glib::Dispatcher`/`signal_idle` |
| Сначала deprecated, потом cleanup | Снижает риск и позволяет быстрее получить работающую GTK4-сборку |
| Lifecycle через класс `Main` в `gtkcompat.h`: `Gtk::Application` с ID `com.hiasm.studio`, окно добавляется через `add_window` | Сделано в коде; `Gtk::Main` в GTK4 нет |
| Меню на собственном `HMenu`/`HMenuToolButton` поверх `Gtk::Popover` вместо `Gio::Menu`/`PopoverMenuBar` | Сделано в коде (`gtkcompat.h`); отклонение от Phase 4, нужно подтвердить |
| Stock-иконки через `Gtk::StockIconMap` внутри `gtkcompat.h`, без отдельного `StockIconMap.*` | Сделано в коде |
| Compat-функции без namespace `compat`: `widget_add_child`/`widget_remove_child`, `dialog_run`, `clipboard_wait_for_text`/`clipboard_request_text` | Сделано в коде; имена отличаются от раздела 5.3 |
| Текстовый редактор: GtkSourceView на обеих ОС | `USE_GTK_SOURCE_VIEW` и `GsvCompat`; поведение покрыто `source_editor` и `editor_commands` |

## 5. Целевая архитектура

### 5.1. Compile-time переключатель

В `CMakeLists.txt`:

```cmake
set(HIASM_GTK_VERSION "4" CACHE STRING "GTK major version: 3 or 4")
set_property(CACHE HIASM_GTK_VERSION PROPERTY STRINGS 3 4)

if(HIASM_GTK_VERSION STREQUAL "4")
    set(CMAKE_CXX_STANDARD 17)
    pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-4.0)
    set(HIASM_GTK_DEFINITIONS HIASM_GTK4=1)
else()
    pkg_check_modules(GTKMM REQUIRED IMPORTED_TARGET gtkmm-3.0)
    set(HIASM_GTK_DEFINITIONS HIASM_GTK3=1)
endif()
```

Правила:

- `#if HIASM_GTK4` допускается только в `gtkcompat.*`, `share.h` и точке входа.
- Бизнес-логика не должна знать о версии GTK.
- Все различия API прячутся за `compat::` и небольшими wrapper-классами.
- Запрещено добавлять новые `GdkEvent*`, `Gtk::Container*`, `Gtk::Stock` и `Gtk::Menu` в новый код.

### 5.2. Новые файлы

| Файл | Назначение |
|---|---|
| `src/gtkcompat.h` / `.cpp` | Контейнерные операции, `pack_*`, `run_dialog`, общие helper'ы |
| `src/gtkcompat_events.h` / `.cpp` | Адаптеры `GestureClick`/`EventControllerMotion`/`EventControllerKey` |
| `src/StockIconMap.h` / `.cpp` | Замена `Gtk::Stock` и `lookup_icon_set` |
| `src/CompatDialogs.h` / `.cpp` | Синхронные обёртки над `Dialog`, `MessageDialog`, `FileChooserDialog` |
| `src/CompatClipboard.h` / `.cpp` | Асинхронный clipboard с блокирующим helper'ом для старых call site'ов |
| `src/GsvCompat.h` | Обёртка GtkSourceView 5 C API для GTK4 на Windows и Linux |
| `resources/hiasm5.css` | Замена `resources/gtkrc` |

**Фактически (2026-10-31):** создан `src/gtkcompat.h`, `src/GsvCompat.h` и `resources/hiasm5.css`. Отдельные cpp-файлы compat-слоя не требуются текущей сборке: их роли выполняют заголовочные адаптеры и `gtkcompat.h`.

### 5.3. Базовые compatibility-API

```cpp
namespace compat {

void add_child(Gtk::Widget& parent, Gtk::Widget& child);
void remove_child(Gtk::Widget& parent, Gtk::Widget& child);

void pack(Gtk::Box& box, Gtk::Widget& child,
          bool expand, bool fill, int padding);

int run_dialog(Gtk::Dialog& dialog);
int run_message(Gtk::MessageDialog& dialog);

Glib::ustring read_clipboard_text_blocking();
void write_clipboard_text(const Glib::ustring& text);

}
```

`pack()` — не полная эмуляция GTK3. Для `expand`/`fill` использовать `set_hexpand()`, `set_vexpand()`, `set_halign()`, `set_valign()`; `padding` — через `set_margin_*()` или `Gtk::Box::set_spacing()`. Это осознанное визуальное приближение.

### 5.4. Замена иконок

```cpp
namespace stock {

Glib::RefPtr<Gdk::Texture> texture(const Glib::ustring& id);
Glib::ustring icon_name(const Glib::ustring& id);

}
```

Используемые ID: `ADD`, `CANCEL`, `CLEAR`, `EDIT`, `EXECUTE`, `FIND`, `FULLSCREEN`, `OK`, `OPEN`, `SAVE`, `SELECT_ALL`, `ZOOM_IN`, `ZOOM_OUT`.

Источники:

1. `packs/_base/icon/*.png` — существующие иконки.
2. Стандартные имена GTK4 (`list-add`, `dialog-cancel`, `edit-clear`, `document-edit`, `media-playback-start`, `edit-find`, `view-fullscreen`, `dialog-ok`, `document-open`, `document-save`, `edit-select-all`, `zoom-in`, `zoom-out`).
3. Fallback `image-missing`.

## 6. Матрица замен GTK3 → GTK4

| GTK3 | GTK4 | Стратегия | Риск |
|---|---|---|---|
| `Gtk::Main` | `Gtk::Application` | Переписать `hico.cpp`, добавить application ID | Средний |
| `Gtk::Window` (главное) | `Gtk::ApplicationWindow` | `MainGUI` наследует `ApplicationWindow` | Средний |
| `show_all()` | `set_visible(true)`, `present()` | Массовая замена, затем точечная проверка | Низкий |
| `set_position(WIN_POS_CENTER)` | нет прямого API | Убрать; позиционирование — оконным менеджером | Низкий |
| `set_icon(pixbuf)` | `set_icon_name()` / иконка приложения | Иконка через тему/ресурс | Средний |
| `HBox` / `VBox` | `Gtk::Box(Orientation, spacing)` | Wrapper или прямая замена | Низкий |
| `pack_start` / `pack_end` | `append` / `prepend` + layout-свойства | `compat::pack()` | Средний |
| `Gtk::Container` | `Gtk::Widget*` | `compat::add_child/remove_child` | Высокий |
| `EventBox` | `Gtk::Box` / `Gtk::Overlay` + controllers | Заменить по месту | Высокий |
| `HandleBox` | `Frame`/`Box` compat | `HandleBoxCompat`, без detach | Средний |
| `ButtonBox` / `VButtonBox` / `HButtonBox` | `Gtk::Box` / `ButtonBoxCompat` | `Secondary` — выравнивание в конец | Средний |
| `Fixed` | `Gtk::Fixed` | Сохраняется; `put()`/`move()` | Низкий |
| `Paned::pack1/pack2` | `set_start_child`/`set_end_child` | Прямая замена | Низкий |
| `Paned::get_child1` | `get_start_child` | Прямая замена | Низкий |
| `Expander::add` | `set_child` | Прямая замена | Низкий |
| `Gtk::Dialog::run()` | `compat::run_dialog()` | Nested `Glib::MainLoop` + `signal_response` | Высокий |
| `MessageDialog::run()` | `AlertDialog` / compat | На первом проходе — compat | Высокий |
| `FileChooserDialog::run()` | `FileDialog` async | Compat или async refactor | Высокий |
| `FileChooserButton` | `Button` + `FileDialog` | `FileChooserButtonCompat` | Средний |
| `ColorSelectionDialog` | `ColorDialog` / `ColorChooserDialog` | Async wrapper | Средний |
| `FontSelectionDialog` | `FontDialog` | Async wrapper | Низкий |
| `AboutDialog` | `AboutDialog` + `present()` | Убрать `run()` | Низкий |
| `Gtk::Menu/MenuBar/MenuItem` | `Gio::Menu` + `PopoverMenuBar` | Полный rewrite `CommandManager` | Высокий |
| `Toolbar/ToolButton` | `Box` + `Button`/`MenuButton`/`ToggleButton` | Полный rewrite | Высокий |
| `Gtk::Stock` | `StockIconMap` | Локальные PNG/icon names | Средний |
| `Gtk::Clipboard` | `Gdk::Clipboard` | Async + blocking helper | Высокий |
| `GdkEventButton/Key/Expose` | `GestureClick`/`EventControllerKey`/`Snapshot` | Event adapter | Высокий |
| `Gdk::Window::invalidate*` | `Gtk::Widget::queue_draw()` | Заменить вызовы | Средний |
| `StyleContext::paint_*` | `render_*` или свой Cairo-рисунок | `Snapshot::append_cairo` | Высокий |
| `get_background_color` / `get_border_color` | CSS `lookup_color` | `ThemeColors` | Средний |
| `Gtk::Image(pixbuf)` | `Texture`/`Paintable` | Внутри модели хранить pixbuf, наружу — texture | Средний |
| `Gdk::Cursor(pixbuf)` | `Gdk::Cursor::create(Texture, hotspot)` | Конвертация pixbuf → texture | Низкий |
| `Gsv::View`/`Gsv::Buffer` | `GsvCompat` над C API | GtkSourceView 5 на Windows и Linux | Средний |
| `Gtk::Table` | — | Не используется | Нет |
| `FILE_TEST_EXISTS` | `G_FILE_TEST_EXISTS` / `Glib::file_test` | Малый GLib compat | Низкий |
| `Glib::Thread` | `std::thread` + `Glib::Dispatcher` | Убрать `gdk_threads_enter/leave` | Высокий |
| `Glib::ListHandle` | `std::vector<Widget*>` | `get_children()` в GTK4 возвращает vector | Низкий |
| `HScale` / `VScrollbar` / `HScrollbar` | `Scale` / `Scrollbar` + `Orientation` | Прямая замена | Низкий |
| `Gtk::StringList` | локальный `StringList` переименовать/квалифицировать | Конфликт с `using namespace Gtk` | Низкий |
| `EntryIconPosition` | `Gtk::Entry::IconPosition` | Уточнить enum | Низкий |
| `BuiltinStockID` / `BuiltinIconSize` / `ICON_SIZE_*` | `StockIconMap` + pixel size | Убрать stock API | Средний |
| `Gtk::Button::set_image` | `set_child(Image)` | Обновить свойства кнопок | Средний |

## 7. Фазы миграции

### Phase 0. Dependency spike и C++17

**Срок:** 2–3 дня.

Задачи:

1. Установить пакеты:

   ```bash
   pacman -S --needed \
     mingw-w64-ucrt-x86_64-gtk4 \
     mingw-w64-ucrt-x86_64-gtkmm-4.0 \
     mingw-w64-ucrt-x86_64-gtksourceview5
   ```

2. Проверить:

   ```bash
   pkg-config --modversion gtk4 gtkmm-4.0 gtksourceview-5
   ```

3. Собрать временный `gtkmm4` hello-world:
   - `Gtk::Application`;
   - `Gtk::ApplicationWindow`;
   - `Gtk::DrawingArea::set_draw_func`;
   - `CssProvider::load_from_string`;
   - `Gdk::Clipboard::read_text_async`.
4. Проверить C API `gtksourceview-5`: `GtkSourceView`, `GtkSourceBuffer`, `GtkSourceLanguageManager`.
5. Собрать текущий GTK3-код с `-std=c++17` и зафиксировать языковые ошибки.
6. Проверить runtime deployment: `objdump -p hiasm5.exe`, наличие `libgtk-4-1.dll`, схем, иконок, `gtksourceview-5`.

**Выход:** подтверждены версии (`gtk4 4.24.1`, `gtkmm 4.24.0`, `gtksourceview 5.22.0`), C++17, минимальный gtkmm4-пример, CSS и clipboard; известен список языковых проблем.

**Фактически выполнено 2026-10-23:**

- Пакеты установлены, `pkg-config --modversion gtk4 gtkmm-4.0 gtksourceview-5` возвращает `4.24.1`, `4.24.0`, `5.22.0`.
- Минимальный gtkmm4-пример с `ApplicationWindow`, `DrawingArea`, CSS и `GtkSourceView` собран и запущен.
- GTK3-код проходит `-std=c++17 -fsyntax-only` (с предупреждениями).
- Первая ошибка GTK4-сборки — не GTK, а GLib-макрос `FILE_TEST_EXISTS` в `src/Pack.h:182`; заменить на `G_FILE_TEST_EXISTS`/`Glib::file_test`.

### Phase 1. Build system и compatibility skeleton

**Срок:** 4–6 дней.

Задачи:

1. Добавить `HIASM_GTK_VERSION` в `CMakeLists.txt`.
2. Для GTK4:
   - `pkg_check_modules(GTKMM ... gtkmm-4.0)`;
   - `gtksourceview-5` только для не-Windows;
   - `gthread-2.0`/`gmodule-2.0` оставить;
   - `target_compile_definitions(hiasm5 PRIVATE HIASM_GTK4=1)`.
3. Добавить пресеты `windows-mingw-gtk4-debug` и `windows-mingw-gtk4-release`.
4. Создать `src/gtkcompat.h/.cpp`, `StockIconMap`, `CompatDialogs`, `CompatClipboard`, `GsvCompat`, `resources/hiasm5.css`.
5. Обновить `cmake/StageRuntime.cmake`: для GTK4 копировать `hiasm5.css`, для GTK3 — `gtkrc`.
6. Не ломать существующие GTK3-пресеты.

**Выход:** GTK3 собирается и запускается; GTK4 конфигурируется и падает только на ожидаемых UI-ошибках, зафиксированных в списке.

**Фактически выполнено 2026-10-23:**

- Добавлены `HIASM_GTK_VERSION=3|4`, `HIASM_GTK4=1`/`HIASM_GTK3=1`, C++17 для GTK4 и отдельная ветка `gtksourceview-5` для не-Windows.
- Добавлены пресеты `windows-mingw-gtk4-debug`/`windows-mingw-gtk4-release`.
- `StageRuntime.cmake` и `install()` выбирают `hiasm5.css` для GTK4 и `gtkrc` для GTK3.
- GTK3-сборка `windows-mingw-debug` проходит.
- GTK4 configure проходит; GTK4 build ожидаемо падает. Первые ошибки:
  - `FILE_TEST_EXISTS` — `src/Pack.h:182`;
  - `Glib::Thread` отсутствует — `src/WindowManager.h:125`;
  - `Glib::ListHandle` отсутствует — `src/FormEditor.cpp:933`;
  - `Gtk::StringList` конфликтует с локальным `StringList` — `src/Palette.h:67`;
  - `HScale`/`VScrollbar`/`HScrollbar` отсутствуют;
  - `EntryIconPosition`, `BuiltinStockID`, `BuiltinIconSize`, `ICON_SIZE_MENU` отсутствуют;
  - `GdkEvent*`, `Gtk::Container`, `Menu/Toolbar`, `EventBox`, `FileChooserButton` — ожидаемые ошибки по плану.

**Обновление 2026-10-26:** список ожидаемых ошибок выше закрыт. GTK4 Debug и Release собираются и линкуются. При проверке найдены и исправлены две ошибки, которые правки 25.10 оставили в общем коде: `kx`/`ky` не были объявлены в `FormEditor.cpp` (ломали обе конфигурации; исправлено объявлением массивов, `FormEditor.cpp:357-361`) и `set_focusable` (GTK4-only) в `SDK_Editor.cpp:42-47` (ломал GTK3; обёрнут в `#ifdef HIASM_GTK4`).

### Phase 2. Lifecycle, окно, тема, splash

**Срок:** 5–7 дней.

Задачи:

1. `src/hico.cpp`:
   - убрать `g_thread_init`, `gdk_threads_init`, `Gtk::Main`;
   - создать `Gtk::Application` с ID (например, `com.hiasm.Studio5`);
   - использовать `signal_activate`/`make_window_and_run`;
   - сохранить `GL_SDK` за отдельным флагом.
2. `MainGUI`:
   - перевести на `Gtk::ApplicationWindow`;
   - заменить `set_position(WIN_POS_CENTER)` (удалить/no-op);
   - заменить `set_icon(pixbuf)` на `set_icon_name`/ресурс;
   - `box1.show_all()` → `set_visible(true)`;
   - `get_window()->get_display()` → `get_display()`.
3. Тема:
   - `gtk_rc_parse` → `CssProvider::load_from_path(dataDir + "hiasm5.css")`;
   - `StyleContext::add_provider_for_display`;
   - `get_background_color`/`get_border_color` → `ThemeColors` на `lookup_color`.
4. `SplashScreen`:
   - убрать `WINDOW_POPUP`/`set_position`;
   - `show_all` → `present`/`set_visible`;
   - `Main::events_pending`/`iteration` → `Glib::MainContext`.
5. About/окно:
   - `AboutDialog::run()` → `present()`;
   - проверить `set_logo`, `set_authors`, `set_version`.

**Выход:** GTK4-приложение запускается, показывает главное окно, тему, splash и About без canvas.

**Фактически выполнено 2026-10-31:**

- Lifecycle: класс `Main` в `gtkcompat.h` (`class Main`) создаёт `Gtk::Application` с ID `com.hiasm.studio`, добавляет окно через `add_window` и вызывает `app->run()`. `hico.cpp:40-42` вызывает `gdk_threads_init` только вне GTK4; в GTK4 вызов был бы заглушкой (`gtkcompat.h`, `gdk_threads_init`).
- Пункт 2 не выполнен: `MainGUI` наследует `Window` (`main_gui.h:23`), а не `Gtk::ApplicationWindow`.
- Тема: в GTK4 загружается `hiasm5.css` через `Gtk::CssProvider::load_from_path` (`main_gui.cpp:145-155`, блок `#ifdef HIASM_GTK4`). Для GTK3 остаётся `gtk_rc_parse`. Набор правил стартовый, полного переноса `gtkrc` нет.
- Smoke-тест GTK4 Debug (2026-10-26): запуск, открытие `packs/_base/new/hcTplForm.sha`, процесс жив 12 с, stderr пуст. Скриншот: `build/Windows_MinGW-x64-GTK4-Debug/gtk4_smoke.png`.
- Не проверялись отдельно: `SplashScreen` (`WINDOW_POPUP`, `show_all`) и окно About.

### Phase 3. Ввод, canvas, кастомное рисование, курсоры

**Срок:** 8–12 дней.

Задачи:

1. Ввести event-адаптер:

   ```cpp
   struct PointerEvent {
       double x;
       double y;
       guint button;
       Gdk::ModifierType state;
       guint32 time;
   };
   ```

2. Заменить старые сигналы:
   - `signal_button_press_event` → `GestureClick::signal_pressed`;
   - `signal_button_release_event` → `GestureClick::signal_released`;
   - `signal_motion_notify_event` → `EventControllerMotion`;
   - `signal_key_press_event` → `EventControllerKey`.
3. `SDK_Editor`:
   - `DrawingArea::set_draw_func`;
   - `on_expose_event`/`on_draw` → draw callback;
   - убрать `set_events`, `GdkEvent*`;
   - сохранить координаты, кнопки, state, keyval.
4. `FormEditor`:
   - активный `WidgetEditorEx` перевести на `Overlay`/`Fixed` + controllers;
   - `Container`, `get_children`, `set_above_child`, `get_window()->invalidate*` заменить;
   - dead-code `WidgetEditor`/`WidgetInfo` удалить или исключить из сборки.
5. `Palette`:
   - `get_root_window()->get_pointer` → координаты из controller;
   - popup-hint → `Gtk::Popover`;
   - `set_allocation`/`paint_*` убрать.
6. `PropEditor`:
   - `EventBox` → контейнер + controller;
   - `get_window()->invalidate` → `queue_draw()`;
   - `paint_check` → `render_check` или собственный Cairo-рисунок.
7. `LayerObjects`:
   - `get_window()->invalidate_rect` → `queue_draw()`;
   - `paint_shadow_gap` → `render_frame`/Cairo.
8. `CursorList`:
   - `Gdk::Cursor::create(name)`;
   - pixbuf → `Gdk::Texture::create_for_pixbuf`;
   - `set_cursor` на widget.

**Выход:** canvas работает: выбор, перемещение, зум, панорамирование, контекстные подсказки, палитра, свойства, редактор формы.

**Фактически выполнено 2026-10-31:**

- `SDK_Editor` подключает события через compat-адаптеры `connect_button_press`, `connect_motion_notify`, `connect_key_press` (`SDK_Editor.cpp:36-41`). Фокус: `set_focusable` для GTK4 и `set_can_focus` для GTK3 (`SDK_Editor.cpp:42-47`).
- Compat-адаптеры мыши в `gtkcompat.h` передают GTK4 `get_current_event_state()` и `get_current_event_time()` для press/release/motion в `SDK_Editor`, `Palette` и GTK4 `EventBox`; legacy bool-результат обработчика теперь определяет, захватывает ли `GestureClick` последовательность. Кэш контроллера переживает `unrealize`/`realize` и очищается при уничтожении виджета, не создавая повторный жестовый контроллер. `MEvent` сбрасывает состояние средней кнопки при соответствующем отпускании, отменяет отложенный таймер при уничтожении и проверяет наличие вложенного `ScrolledWindow` перед панорамированием. GTK4 Debug и GTK3 Debug собраны после изменений; проверка модификаторов, времени и жестов в работающем UI остаётся открытой.
- `FormEditor`: восстановлены массивы `kx`/`ky` для 8 ручек изменения размера (`FormEditor.cpp:357-361`). Порядок совпадает с `ElementDynGeometry::rePosGrids()` (`Elements.cpp:803-810`).
- Мёртвый код пока не удалён: `WidgetEditor`/`WidgetInfo` объявлены в `FormEditor.h` без определений, а в `FormEditor.cpp:789-791` вызов `new WidgetEditor` идёт после безусловного `return`. `paint_*` в `LayerObjects.cpp`, `Palette.cpp`, `PropEditor.cpp` закомментированы.
- Проверено 2026-10-26: добавление и перемещение элемента на схеме. Редактор формы открывается, но добавление элемента в него не сработало (открытый пункт чеклиста). Не проверено: зум, панорамирование, курсоры.

### Phase 4. Команды, меню, тулбары

**Срок:** 8–12 дней.

Задачи:

1. `CommandManager`:
   - сохранить загрузку `commands`, `menus`, `menu_commands` из БД;
   - на каждую команду создать `Gio::SimpleAction`;
   - состояние `enabled`/`checked` — через action state;
   - `on_command` оставить как callback.
2. Меню:
   - `Gio::Menu` + `Gtk::PopoverMenuBar`;
   - вложенные меню и radio-группы;
   - `attachMenu()` → submenu/action;
   - контекстные меню → `Gtk::PopoverMenu` + `popup()`.
3. Тулбары:
   - `Gtk::Box` + `Gtk::Button`/`Gtk::ToggleButton`/`Gtk::MenuButton`;
   - `CM_ToolButton` → `CommandButton`;
   - `CM_ToggleToolButton` → `CommandToggleButton`;
   - `CM_MenuToolButton` → `MenuButton`;
   - `CM_ImageMenuItem` → action/menu item.
4. Горячие клавиши:
   - `Gtk::Application::set_accels_for_action` или `Gtk::ShortcutController`.
5. Заменить все `Stock::` в меню/тулбарах на `StockIconMap`.

**Выход:** все команды доступны из меню и тулбара, работают shortcuts, enabled/checked, контекстные меню и меню компиляторов.

**Фактически выполнено 2026-10-26:**

- Команды и меню загружаются из БД (`CommandManager`), но отрисовываются собственным `HMenu` (`gtkcompat.h:694`) на `Gtk::Popover`. `Gio::Menu` и `PopoverMenuBar` не используются.
- `HMenuToolButton` (`gtkcompat.h:866-895`): основная часть выполняет команду, стрелка открывает меню через `popup_at`. Двойного срабатывания не воспроизводится (проверено 2026-10-26).
- Меню Build пустое: у `_base` нет записей `pack_compilers`, список заполняется только для паков с компиляторами.
- Исправлено 2026-10-26: позиционирование меню строки меню (`gtkcompat.h`, `set_submenu_mode`) и двойной запуск команд кнопок тулбара (`HToolButton`). Не проверено: горячие клавиши. `Gio::SimpleAction` не используется.

### Phase 5. Контейнеры, шаблоны элементов, диалоги

**Срок:** 8–12 дней.

Задачи:

1. `compat::add_child/remove_child` для `Box`, `Paned`, `Fixed`, `ScrolledWindow`, `Expander`, `Frame`, `Window`, `Dialog`, `Overlay`.
2. `Elements.cpp`:
   - `ElementTplVBox` → `Box`; `pack_start` → `append`;
   - `ElementTplButtonBox` → `ButtonBoxCompat`;
   - `ElementTplHandleBox` → `HandleBoxCompat` (без detach);
   - `ElementTplPaned` → `set_start_child`/`set_end_child`;
   - `ElementTplExpander` → `set_child`;
   - `ElementTplForm` → `Window` + `set_child`, `signal_close_request`;
   - `ElementTplDialog` → `Dialog::get_content_area` + `compat::run_dialog`;
   - `ElementTplFileChooserButton` → `FileChooserButtonCompat`;
   - `ElementTplColorButton` → `get_rgba()`;
   - `ElementTplImage` → хранить pixbuf в модели, наружу отдавать texture/paintable.
3. `SettingsManager`, `ProjectManager`, `ArrayEditor`:
   - `Dialog` без `run` → compat;
   - `get_vbox` → `get_content_area`;
   - `VButtonBox`/`HButtonBox` → `Box`;
   - `show_all` → `set_visible`.
4. `WindowManager`:
   - вкладки: `ToolButton` → `Button`, `HBox` → `Box`;
   - `MEvent` scrollbars через `Adjustment`;
   - file/color/about/message dialogs.
5. `FormEditor`:
   - завершить контейнерные операции;
   - `insertElement` через helper;
   - удалить мёртвый legacy-путь.

**Выход:** открываются проекты, работают редактор формы, настройки, менеджер проектов, массивы, вкладки и пользовательские диалоги.

**Фактически выполнено 2026-10-26:**

- Диалоги `shaDlg.run(name)` (`SHADialogs.cpp:12-34`) возвращают `NULL`, если нет файла `int/lang/en/<name>.sha`. В репозитории нет `new_palette_group.sha` и `element_creator.sha`; в upstream (`hiasmstudio/hiasm5`) по этим путям ответ 404.
- Исправлено: `CMD_ADD_GROUP` (`main_gui.cpp:322-327`) и `CMD_ECREATOR` (`WindowManager.cpp:887-889`) проверяют `NULL` и не падают. `CMD_ADD_GROUP` дополнительно проверяет `sde`. Сами функции не работают, пока схемы не созданы.
- Имена helpers отличаются от раздела 5.3: `widget_add_child`/`widget_remove_child`, `dialog_run`, `Gtk::StockIconMap`.
- Проверено 2026-10-26: окно Open (GTK file chooser) и Settings manager открываются поверх главного окна. Не проверено: менеджер проектов (New), массивы, About, выбор цвета.

### Phase 6. Clipboard, GtkSourceView, прочее

**Срок:** 5–7 дней.

Задачи:

1. Clipboard:
   - `Gdk::Display::get_default()->get_clipboard()`;
   - `set_text`/`read_text_async`;
   - `CompatClipboard::read_clipboard_text_blocking()` для `ElementCoreClipboard::read_var`;
   - `SelectManager::copy/paste` — async или blocking helper.
2. GtkSourceView:
   - `GsvCompat` с `GtkSourceView`/`GtkSourceBuffer`/`GtkSourceLanguageManager`;
   - `Gsv::init` → wrapper init;
   - GtkSourceView 5 на Windows и Linux.
3. Splash:
   - `Glib::MainContext::get_default()->iteration(false)` вместо `Main::iteration`.
4. Прочее:
   - `gtk_show_uri` → `Gtk::show_uri`/`Gio::AppInfo`;
   - `Gtk::Clipboard` в `ElementCoreClipboard`/`SelectManager`;
   - `Gtk::Window` icon, fullscreen, title;
   - `Gtk::MessageDialog`/`AlertDialog`.
5. Удалить оставшиеся `GdkEvent*` из UI-кода, кроме compat-слоя.

**Выход:** работает текстовый редактор, copy/paste, splash, пользовательские сообщения и clipboard-элементы.

**Фактически выполнено 2026-10-26:**

- Clipboard: блокирующий `clipboard_wait_for_text()` (`gtkcompat.h`) используется в `ElementCoreClipboard::read_var` (`Elements.cpp:1765`). Асинхронный `clipboard_request_text()` определён в обеих ветках и используется в `SelectManager.cpp:232`. В GTK4 callback чтения удерживает общее состояние через `shared_ptr` до завершения/тайм-аута, но вложенный main context по-прежнему может блокировать интерфейс при вызове элемента `Clipboard`.
- На 2026-10-26 Windows ещё использовал TextView. С 2026-11-02 GTK4 обеих ОС использует GtkSourceView 5 (`src/GsvCompat.h`); wrapper инициализирует buffer и язык по расширению. Подсветка и редакторские команды покрыты source_editor/editor_commands на Windows и Linux; визуальная Windows-приёмка отдельно.
- Copy/paste в редакторе схемы проверены 2026-10-26 (Ctrl+A, Ctrl+C, Ctrl+V). Не проверено: текстовый редактор, splash, элемент `Clipboard` в схеме.

### Phase 7. Упаковка, документация, приёмка

**Срок:** 5–7 дней.

Задачи:

1. CMake install:
   - для GTK4 устанавливать `hiasm5.css`, не `gtkrc`;
   - сохранить `splash.png`, `LICENSE`, `int`, `elements`.
2. `tools/deploy_gtk_runtime.ps1`:
   - DLL closure через `objdump` уже версионно-нейтрален;
   - добавить данные `share/gtk-4.0`, `share/gtksourceview-5`;
   - сохранить `share/icons`, `share/themes`, `share/glib-2.0/schemas`, `share/fontconfig`;
   - проверить `libgtk-4-1.dll` и `libgtksourceview-5-0.dll` при наличии.
3. `tools/build_windows.bat`:
   - добавить выбор `gtk3|gtk4`;
   - обновить подсказку пакетов;
   - GTK4 оставить профилем по умолчанию, GTK3 сохранить как явный переходный профиль.
4. `tools/build_windows_installer.bat`:
   - параметр `GTK_VERSION`;
   - отдельные имена установщиков (`HiAsm5_Setup_GTK4.exe`).
5. `MARKDOWN/BUILD_WINDOWS.md`:
   - обе конфигурации;
   - зависимости;
   - известные отличия.
6. `MARKDOWN/CHECKLIST.md`:
   - отметить выбор GTK4;
   - ссылка на этот план.
7. `MARKDOWN/INIT.MD`:
   - обновить «C++/GTK3, gtkmm 3» на GTK4-профиль в отдельной документационной правке;
   - синхронизировать `.cursor/commands/init.md` байт-в-байт.

**Выход:** GTK4-установщик собирается и содержит полный runtime; проверка установки и запуска на чистой Windows без MSYS2 остаётся отдельным пунктом. GTK3-сборка доступна как переходный профиль.

**Фактически выполнено 2026-10-26:**

- `CMakeLists.txt` ставит `hiasm5.css` для GTK4 и `gtkrc` для GTK3 (`HIASM_GTK_THEME_FILE`). `cmake/StageRuntime.cmake` копирует то же самое.
- Пункт 3 выполнен: `tools/build_windows.bat` принимает `gtk3|gtk4`; GTK4 является профилем по умолчанию, GTK3 выбирается явно. Проверены GTK4 Debug и Release, а также GTK3 Debug.
- GTK4 installer pipeline завершён: `tools/build_windows.bat gtk4 installer` собрал `build/HiAsm5_Setup_GTK4.exe` (38,801,672 байта), deployment содержит 69 runtime DLL и GTK data, включая `gtk-4.0` и `gtksourceview-5`. Установка на чистой Windows не проверена.
- `MARKDOWN/BUILD_WINDOWS.md` обновлён под новый синтаксис `build_windows.bat`; установщик поддерживает GTK4 и GTK3.
- `MARKDOWN/INIT.MD` и `.cursor/commands/init.md` совпадают байт в байт (SHA256). Фразу «C++/GTK3, gtkmm 3» нужно обновить в отдельной документационной правке.
- `MARKDOWN/CHECKLIST.md` обновлён по результатам GTK4 installer pipeline 2026-10-31.

## 8. Карта затронутых файлов

| Файл | Объём | Основные работы |
|---|---|---|
| `src/hico.cpp` | S | `Gtk::Application`, удаление `Gtk::Main` |
| `src/main_gui.cpp/.h` | L | shell, тема, окно, toolbar, splash, иконки |
| `src/CommandManager.cpp/.h` | L | actions, menus, toolbars, state |
| `src/commands.cpp/.h` | M | команды и привязки |
| `src/WindowManager.cpp/.h` | L | вкладки, диалоги, Gsv, clipboard |
| `src/SDK_Editor.cpp/.h` | L | canvas, события, Cairo |
| `src/FormEditor.cpp/.h` | L | embedding, overlay, события, dead code |
| `src/Elements.cpp/.h` | L | шаблоны, контейнеры, stock, clipboard |
| `src/Palette.cpp/.h` | M | popup hint, pointer, цвета |
| `src/PropEditor.cpp/.h` | L | EventBox, рисование, диалоги |
| `src/LayerObjects.cpp/.h` | M | invalidate, style render |
| `src/SettingsManager.cpp/.h` | M | Dialog, layout |
| `src/ProjectManager.cpp/.h` | M | Dialog, file chooser |
| `src/ArrayEditor.cpp/.h` | S | Dialog, buttons |
| `src/SplashScreen.cpp/.h` | S | popup, events |
| `src/SelectManager.cpp/.h` | S | clipboard |
| `src/share.h` | M | includes, typedefs, compat |
| `src/Element*.cpp/.h` | M | события, свойства, geometry |
| `CMakeLists.txt` | M | версии, C++17, pkg-config |
| `CMakePresets.json` | M | GTK4 presets |
| `cmake/StageRuntime.cmake` | S | CSS/gtkrc staging |
| `tools/deploy_gtk_runtime.ps1` | M | GTK4 data/DLL |
| `tools/build_windows*.bat` | M | выбор backend |
| `resources/gtkrc` → `resources/hiasm5.css` | S/M | перенос стилей |
| `MARKDOWN/*` | S | документация |

## 9. Риски и меры

| Риск | Вероятность | Влияние | Мера |
|---|---|---|---|
| Сложность `SDK_Editor` и `FormEditor` | Высокая | Высокое | Отдельный spike, event-адаптер, раннее ручное тестирование |
| `Dialog::run()` и синхронные сценарии | Высокая | Высокое | `compat::run_dialog()` на nested `MainLoop`; позже async |
| Меню/тулбары на `Gio::Action` | Высокая | Высокое | Отдельная фаза, сохранить БД-схему команд |
| `Gtk::Container` и `pack_*` | Высокая | Среднее | `compat::add_child`, `compat::pack`, тесты layout |
| `GtkSourceView` без gtkmm4 | Средняя | Среднее | C-wrapper GtkSourceView 5, включая Windows |
| Clipboard async | Средняя | Среднее | Blocking helper для старых call site'ов |
| Визуальные отличия CSS | Средняя | Среднее | Постепенный перенос `gtkrc` → CSS, скриншот-сравнение |
| Потоки и UI | Средняя | Высокое | Все UI-обновления через `Glib::Dispatcher`/`signal_idle` |
| Упаковка GTK4 runtime | Средняя | Высокое | Проверка на чистой Windows, `objdump` closure |
| Удаление deprecated-API в будущем | Низкая | Среднее | Зафиксировать GTK 4.24, не включать `GTKMM_DISABLE_DEPRECATED` |
| Потеря поведения `HandleBox`/`ButtonBox` | Средняя | Низкое | Задокументировать изменения, при необходимости — custom widget |
| Старый C++-код под C++17 | Средняя | Среднее | Отдельная GTK3 C++17-сборка в Phase 0 |

## 10. Критерии готовности (актуализация 2026-11-03)

- [x] Windows GTK4 Debug/Release собираются. Повторная сборка Release/NSIS после последних правок ведётся отдельно.
- [x] GTK4 portable запускается с очищенным PATH, DLL не загружаются из MSYS2; runner validation проходит.
- [x] GTK3 Debug сохраняет совместимость; 18/18 CTest.
- [x] Linux GTK4: сборка в Debian chroot, 19/19 CTest через Xvfb, включая MinGW/Wine GTK4 GUI и Stop.
- [x] Схемы: Unicode load/save, history, группы, выравнивание, move, copy/paste, сохранение связей.
- [x] Редактор формы: выбор, shift-select, move/resize, insert/delete, freeze, cleanup, undo/redo и переоткрытие.
- [x] Текстовый редактор использует GtkSourceView 5 в GTK4; source_editor и editor_commands проверяют подсветку, поиск, сохранение и историю.
- [x] Создание элемента и группы использует native dialogs и prepared SQLite; отсутствующие схемы больше не являются зависимостью этих команд.
- [x] Нативная Win32 рамка без GTK titlebar, переключение темы и безопасные таймеры подсказок покрыты тестами.
- [ ] Визуальная приёмка палитры, подсказок, drag/drop, меню и всех диалогов. Ограничения: UI_AUTOMATION_REPORT.md.
- [ ] Установка/удаление на чистой Windows без MSYS2. Portable изолирует PATH, но не заменяет чистую ОС.
- [ ] Полностью убрать старые имена GTK3 API вне compatibility boundary. Текущий перенос использует адаптеры и переходные native API.
- [x] SVN update/log/show выполняются асинхронно; регрессия использует локальную рабочую копию.
- [ ] Функциональное соответствие HiAsm 4: x86 CodeGen/formedit bridge и полный C++17 пакет. Legacy read-only metadata и непрозрачные ресурсы уже поддерживаются; подзадачи — в CHECKLIST.md.

## 11. Открытые вопросы

1. Нужна ли поддержка GTK3 после первого GTK4-релиза? Предположение: один переходный релиз, затем удаление.
2. Нужна ли Linux-сборка в релизе или достаточно Windows? Предположение: поддерживать, но приоритет — Windows.
3. Нужно ли сохранять detach-поведение `HandleBox`? Если да — потребуется отдельный custom-widget и +3–5 дней.
4. Допустимы ли изменения внешнего вида `ButtonBox`/`FileChooserButton`? Если нет — нужен более точный compat-слой.
5. Нужно ли сохранять точный внешний вид GTK3 или допустим современный Adwaita-стиль?
6. Используется ли `GL_SDK`/OpenGL в реальных сборках? Если нет — можно не портировать.
7. Какие паки содержат `HCWE_*` с удалёнными виджетами? Нужен автоматический аудит `packs/`.
8. Минимальная версия Windows для установщика?
9. Закрыто 2026-11-02: создание группы/элемента использует native dialogs. Историческая проблема: создать или восстановить схемы `int/lang/en/new_palette_group.sha` и `int/lang/en/element_creator.sha`. В репозитории их нет, в upstream по этим путям 404 (2026-10-26). Без них команды «добавить группу» и «создать элемент» не работают.
10. Фон окна в GTK4: `resources/hiasm5.css` задаёт `window { background-color: #f6f6f6 }`. В GTK3 фон окна через `gtkrc` не задавался. Оставить светлый фон или перейти на системную тему?
11. Закрыто 2026-11-02: Linux GTK4 собирается и проходит 18 CTest, включая текстовый редактор; визуальная Windows-приёмка ведётся отдельно.

## 12. Первые 10 рабочих дней

1. Установить GTK4-пакеты и подтвердить `pkg-config`.
2. Собрать gtkmm4 hello-world + CSS + clipboard.
3. Собрать GTK3-код под C++17 и записать ошибки.
4. Добавить `HIASM_GTK_VERSION` и GTK4-пресеты.
5. Создать `gtkcompat.h/.cpp` и `StockIconMap`.
6. Перевести `hico.cpp` и `MainGUI` на `Gtk::Application`.
7. Перенести тему на CSS.
8. Запустить GTK4-шелл без canvas.
9. Начать event-адаптер и `SDK_Editor`.
10. Зафиксировать первый отчёт: что собирается, что падает, какие API требуют нестандартных решений.

## 13. Ссылки

- `MARKDOWN/CHECKLIST.md` — статус миграции.
- `MARKDOWN/BUILD_WINDOWS.md` — сборка и установщик.
- `MARKDOWN/INIT.MD` — правила документации.
- `CMakeLists.txt`, `CMakePresets.json` — текущая конфигурация.
- `cmake/StageRuntime.cmake` — staging ресурсов.
- `tools/deploy_gtk_runtime.ps1` — runtime DLL/data.
- `src/hico.cpp`, `src/main_gui.cpp`, `src/CommandManager.cpp`, `src/Elements.cpp`, `src/WindowManager.cpp`, `src/SDK_Editor.cpp`, `src/FormEditor.cpp` — ключевые точки миграции.

## 14. Текущие следующие шаги (2026-11-03)

1. Обновлять Release/portable/NSIS после изменений, сохраняя проверенную матрицу GTK4/GTK3/Linux.
2. Устранить недоступный захват окна в Computer Use и выполнить визуальную приёмку. Не считать дерево доступности скриншотом.
3. Реализовать отдельный x86 CodeGen/formedit bridge. Поиск установок, CP1251 и read-only metadata уже работают.
4. Поддерживать проверенные console/GUI cross-профили: Windows → Linux через WSL, Linux → Windows через bundled UCRT64/Wine. Сборка, запуск, Stop и автономная Windows GUI-упаковка проверены; clean-OS и визуальная приёмка отдельные.
5. Продолжить перенос Delphi-элементов по фактическим функциям, конфигурациям и примерам.
6. Разбирать оставшиеся несовпадения COMMAND_AUDIT.md; SVN update/log/show уже реализованы.
7. Чистая Windows: install, launch, uninstall, сохранность пользовательских проектов.

Ранее изложенные фазы и количественные снимки относятся к датам в их заголовках; текущие результаты приведены в разделе 10 и CHECKLIST.md.

## Фактический статус 2026-11-02 (финальная проверка этого прохода)

- Windows GTK4 Debug: 19/19 CTest; GTK4 Release: 19/19; GTK3 Debug: 17/17.
- Linux GTK4 в Trixie chroot/Xvfb: 18/18 CTest. Дополнительные проверки собрали Windows PE через MinGW и запустили его через Wine; Windows → Linux собран и запущен в WSL.
- `tools/stage_cpp_toolchain.ps1` формирует relocatable UCRT64 closure (92 пакета, manifest и лицензии), проверенный с isolated PATH, включая каталог с пробелами и Unicode.
- `tools/test_linux_ui.sh` сохранил `build/qa/linux-ui-startup.png`, `linux-ui-new.png` и `linux-ui-open.png`. Это функциональный X11 smoke и материал для ручной сверки. Windows Computer Use прочитал дерево HiAsm, но захват окна истёк по timeout; клики с неизвестной геометрией не засчитываются.
- NSIS: silent install/uninstall в отдельном временном каталоге прошёл; чистая ОС без MSYS2 всё ещё требует отдельной ручной проверки.

Открытыми остаются точный визуальный parity с HiAsm 4/5, полный перенос Delphi-пака (795 конфигураций прочитаны, но все native C++ аналоги не созданы), x86 Delphi CodeGen bridge и GTK4 development closure для cross-target. Асинхронный GTK timer добавлен в C++-пакет; результаты проверки приводятся в CHECKLIST.md.

## Фактический статус 2026-11-03

Этот раздел заменяет количественные результаты предыдущего исторического снимка.

- Windows GTK4 Debug/Release: 21/21 CTest; GTK3 Debug: 18/18;
  Linux GTK4: 19/19. Debug включает Windows → Linux GTK4 в отдельном
  HiAsm-GTK4-QA/WSLg; Linux включает обратный Wine cross-путь. Проверены Stop,
  callback таймера и естественное закрытие generated GUI.
- C++17: 23 собственных элемента, включая Memory/Counter, динамические данные,
  состояние по окнам/вызовам и 44 операции Math. Реестр Delphi: 15 частичных
  аналогов, 780 без проверенного аналога, ни один не объявлен полностью совместимым.
- Исправлено повреждение графа при повторных связях и зависание при routing-cycle.
  Неисправные .sha отклоняются до подключения проводов; примеры используют
  штатные HubEx/GetDataEx. Проверены вставка в линию и её восстановление.
- Исправлен GTK Win32 IME use-after-free message filter; отдельный lifecycle test
  и повторный RTCG проходят. GTK patches/исходники/лицензии поставляются с IDE
  и C++ комплектом. Подробности: `GTK4_RUNTIME_FIXES.md`.
- `package_cpp_windows.py` создаёт автономный Windows GUI runtime с manifest,
  SVG и иконками. Перемещённая Unicode-папка запускается на Windows и Wine
  без DLL компилятора. Release portable redeploy/startup/runner прошёл.
- Обновлённый GTK4 NSIS installer прошёл archive integrity, payload SHA256,
  isolated startup после извлечения и bundled GTK4 compiler probe. В текущей
  сессии installer не запускался: процесс не elevated. Проверка на чистой ОС
  остаётся отдельным открытым пунктом.

Открыты: визуальная приёмка (Computer Use остановлен пользователем), чистая
Windows, x86 CodeGen/formedit bridge и полный перенос Delphi-пакета.
Не закрывать эти задачи результатами source lint или CTest.
