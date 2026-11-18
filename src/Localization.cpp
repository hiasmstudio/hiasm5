#include "Localization.h"
#include <glib.h>
#include <map>
namespace {
std::string language = "ru";
const std::map<std::string, const char *> russian = {
    {"Welcome", "Добро пожаловать"}, {"New project", "Новый проект"},
    {"Add", "Добавить"}, {"Cancel", "Отмена"}, {"Clear", "Очистить"},
    {"Edit", "Правка"}, {"Execute", "Выполнить"}, {"Find", "Найти"},
    {"Fullscreen", "Полный экран"}, {"OK", "ОК"}, {"Open", "Открыть"},
    {"Save", "Сохранить"}, {"Select All", "Выделить всё"},
    {"Zoom In", "Увеличить"}, {"Zoom Out", "Уменьшить"},
    {"Yes", "Да"}, {"No", "Нет"}, {"Close", "Закрыть"},
    {"_Cancel", "_Отмена"}, {"_Open", "_Открыть"},
    {"C++ package", "Пакет C++"}, {"Editor components", "Компоненты редактора"},
    {"Elements", "Элементы"}, {"Element property", "Свойства элемента"},
    {"Build settings", "Настройка сборки"}, {"Compiler", "Компилятор"},
    {"Build", "Собрать"}, {"Build current scheme", "Собрать текущую схему"},
    {"Package", "Пакет"}, {"No package selected", "Пакет не выбран"},
    {"Element palette", "Палитра элементов"}, {"Information panel", "Панель информации"},
    {"Debug panel", "Панель отладки"}, {"Short help", "Краткая справка"},
    {"Errors", "Ошибки"}, {"Terminal", "Терминал"},
    {"Enter a command", "Введите команду"}, {"Run", "Выполнить"},
    {"Stop", "Остановить"}, {"Command failed", "Ошибка выполнения команды"},
    {"Unknown error", "Неизвестная ошибка"}, {"Exit code", "Код завершения"},
    {"Process stopped", "Процесс остановлен"},
    {"Shell", "Оболочка"}, {"New tab", "Новая вкладка"}, {"Close tab", "Закрыть вкладку"},
    {"A command is already running; stop it before starting another.", "Дождитесь завершения команды или остановите процесс."},
    {"Project information", "Информация о проекте"}, {"Error panel", "Панель ошибок"},
    {"Show all panels", "Показать все панели"}, {"Hide all panels", "Скрыть все панели"},
    {"Hide panel", "Скрыть панель"}, {"Attach panel", "Закрепить панель"},
    {"Detach panel", "Открепить панель"},
    {"Project repository", "Репозиторий проекта"},
    {"Examples", "Примеры"},
    {"Examples catalog", "Каталог примеров"},
    {"Project catalog", "Каталог проектов"},
    {"Tutorials", "Уроки"},
    {"Refresh examples catalog", "Обновить каталог примеров"},
    {"Double-click a scheme to open it.", "Дважды щёлкните по схеме, чтобы открыть её."},
    {"Current project", "Текущий проект"}, {"Selected folder", "Выбранная папка"},
    {"My projects", "Мои проекты"}, {"Projects", "Проекты"},
    {"Choose project directory", "Выбрать папку проекта"},
    {"Refresh repository", "Обновить список проектов"},
    {"Create Git repository", "Создать репозиторий Git"},
    {"Clone Git repository", "Клонировать репозиторий Git"},
    {"Update repository", "Получить изменения репозитория"},
    {"Repository history", "История изменений"}, {"Repository changes", "Изменения файлов"},
    {"Repository status", "Состояние репозитория"}, {"Save to repository", "Сохранить в репозиторий"},
    {"Repository URL or local path", "Адрес репозитория или локальный путь"}, {"Clone", "Клонировать"},
    {"No project selected", "Проект не выбран"},
    {"Choose a project or repository folder.", "Выберите проект или папку репозитория."},
    {"Repository operation in progress…", "Выполняется операция с репозиторием…"},
    {"Cannot read repository directory.", "Не удалось прочитать папку репозитория."},
    {"Repository listing limited to 2000 entries.", "Список ограничен 2000 записями."},
    {"SVN client not found. Install svn or set HIASM_SVN.", "Клиент SVN не найден. Установите svn или задайте HIASM_SVN."},
    {"Git client not found. Install Git or set HIASM_GIT.", "Клиент Git не найден. Установите Git или задайте HIASM_GIT."},
    {"HiAsm 4 catalog (SVN). Open schemes by double-clicking.", "Каталог HiAsm 4 (SVN). Двойной щелчок открывает схему."},
    {"Git repository. Open schemes by double-clicking.", "Репозиторий Git. Двойной щелчок открывает схему."},
    {"HiAsm 4 catalog: no SVN working copy found. Files can be browsed read-only.", "Каталог HiAsm 4: рабочая копия SVN не найдена. Доступен просмотр файлов."},
    {"Local folder. Create or clone a Git repository to enable version control.", "Локальная папка. Создайте или клонируйте репозиторий Git для управления версиями."},
    {"Cannot start repository operation. Check the client and working copy.", "Не удалось начать операцию. Проверьте клиент и рабочую копию."},
    {"Choose an empty destination folder before cloning.", "Перед клонированием выберите пустую папку назначения."},
    {"Operation completed.", "Операция завершена."},
    {"Repository operation failed.", "Операция с репозиторием завершилась ошибкой."},
    {"Reload open schemes and restart HiAsm after updating package definitions.", "После обновления перезагрузите открытые схемы; для изменённых определений пакетов перезапустите HiAsm."},
    {"Use a .sha filename for the repository copy.", "Используйте имя с расширением .sha для копии схемы."},
    {"A file with this name already exists. Choose another name.", "Файл с таким именем уже существует. Выберите другое имя."},
    {"Scheme copied to repository.", "Копия схемы сохранена в репозитории."},
    {"Open a scheme before saving to the repository.", "Перед сохранением в репозиторий откройте схему."},
    {"Project manager", "Создание проекта"}, {"Settings manager", "Настройки"},
    {"Interface theme", "Тема интерфейса"}, {"System", "Системная"},
    {"Light", "Светлая"}, {"Dark", "Тёмная"}, {"Language", "Язык интерфейса"},
    {"Restart HiAsm to apply the language.", "Перезапустите HiAsm для смены языка."},
    {"Warning", "Предупреждение"}, {"(none)", "(нет)"},
    {"Selected: ", "Выбрано: "}, {"Save file", "Сохранение файла"},
    {"Open file", "Открытие файла"}, {"Save text as", "Сохранить текст как"},
    {"Find text", "Поиск текста"}, {"Replace element", "Замена элемента"},
    {"Replacement template name (properties and connected points must be compatible):", "Имя нового элемента (свойства и соединённые точки должны быть совместимы):"},
    {"Unable to save scheme: ", "Не удалось сохранить схему: "},
    {"Unable to open UTF-8 text: ", "Не удалось открыть текст UTF-8: "},
    {"Unable to save text: ", "Не удалось сохранить текст: "},
    {"Unable to save text", "Не удалось сохранить текст"},
    {"File %1 has not been saved! Save project before close?", "Файл %1 не сохранён. Сохранить проект перед закрытием?"},
    {"Create element", "Создание элемента"}, {"Custom event relay", "Передача пользовательского события"},
    {"Element name", "Имя элемента"}, {"Description", "Описание"},
    {"Palette tab", "Вкладка палитры"}, {"Add palette group", "Добавление группы в палитру"},
    {"Creates doWork → onEvent. Configuration and generator source open for editing.", "Создаёт doWork → onEvent. Конфигурация и исходный код генератора откроются для редактирования."},
    {"Could not add palette group: the name is empty, already exists, or the package database is not writable.", "Не удалось добавить группу: пустое или повторяющееся имя либо база пакета недоступна для записи."},
    {"Path", "Путь"}, {"Size", "Размер"}, {"Date", "Дата"},
    {"Start page", "Начальная страница"}, {" array", " — массив"},
    {"Items: ", "Элементов: "}, {", select: ", ", выбрано: "},
    {"Text editor", "Редактор текста"}, {"Image editor", "Редактор изображения"},
    {"New param", "Новый параметр"}, {"Code name", "Идентификатор"},
    {"Core", "Основные компоненты"}, {"Strings", "Строки"}, {"Math", "Математика"},
    {"Arrays", "Массивы"}, {"Files", "Файлы"}, {"Forms", "Формы"},
    {"Debug", "Отладка"}, {"Type", "Тип"}, {"Name", "Имя"},
    {"Select file", "Выбор файла"}, {"All files", "Все файлы"},
    {"Can not insert more then one interface widget!", "Нельзя добавить более одного корневого виджета интерфейса."}
};
}
const char *ui_tr(const char *english) {
    if(language == "ru") {
        const auto found = russian.find(english);
        if(found != russian.end()) return found->second;
    }
    return english;
}
const std::string &ui_language() { return language; }
void ui_set_language(const std::string &value) { language = value == "en" ? "en" : "ru"; }
std::string ui_system_language() {
    const gchar *const *names = g_get_language_names();
    for(auto name = names; name && *name; ++name) {
        const std::string locale(*name);
        if(locale == "ru" || locale.compare(0,3,"ru_")==0 || locale.compare(0,3,"ru-")==0) return "ru";
        if(locale == "en" || locale.compare(0,3,"en_")==0 || locale.compare(0,3,"en-")==0) return "en";
    }
    return "en";
}
