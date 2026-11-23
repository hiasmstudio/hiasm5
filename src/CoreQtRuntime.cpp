#include "CoreQtRuntime.h"
#include "Elements.h"
#include "qt/core_widgets.h"
#include <gmodule.h>
#include <unordered_map>
#ifdef G_OS_WIN32
#include <windows.h>
#endif

namespace CoreQtRuntime {
namespace {
const HiasmQtWidgets *api;
#ifdef G_OS_WIN32
HMODULE module;
#else
GModule *module;
#endif
bool closing;
std::unordered_map<Element *, void *> handles;
std::unordered_map<Element *, TypePixbuf> pictures;

const char *property(void *element, const char *name) {
    static std::string result;
    result.clear();
    auto *prop = static_cast<Element *>(element)->props.getByName(name);
    if (!prop) return result.c_str();
    switch (prop->type) {
        case data_int: case data_combo: case data_comboEx:
            result = std::to_string(prop->readInt()); break;
        case data_real: {
            char number[G_ASCII_DTOSTR_BUF_SIZE];
            result = g_ascii_dtostr(number, sizeof(number), prop->readReal()); break;
        }
        case data_pixbuf:
            result = prop->serialize().raw(); break;
        case data_color:
            result = static_cast<TypeColor *>(prop->value)->to_string().raw(); break;
        case data_array: {
            const auto &array = *static_cast<ArrayValue *>(prop->value);
            if (array.type != data_str && array.type != data_stock) break;
            for (size_t i = 0; i < array.size(); ++i) {
                if (i) result += '\n';
                result += static_cast<const ustring *>(array[i].data)->raw();
            }
            break;
        }
        default: result = prop->readStr().raw(); break;
    }
    return result.c_str();
}

void event(void *element, const char *name, const char *text, double number, int type) {
    if (g_str_equal(name, "@close")) { closing = true; return; }
    auto *control = static_cast<Element *>(element);
    if (auto *point = control->getPointByName(name)) {
        TData value;
        if (type == 1) value = ustring(text);
        else if (type == 2) value = static_cast<int>(number);
        else if (type == 3) value = number;
        else if (g_str_equal(name, "onClick")) {
            auto *prop = control->props.getByName("Data");
            if (prop && prop->readData()) value = *prop->readData();
        }
        point->on_event(&value);
    }
}
}

bool start(const char *runtimeDirectory) {
    const auto directory = std::string(runtimeDirectory) + "qt" G_DIR_SEPARATOR_S;
#ifdef G_OS_WIN32
    const auto path = directory + "hiasm-core-qt.dll";
#elif defined(__APPLE__)
    const auto path = directory + "libhiasm-core-qt.dylib";
#else
    const auto path = directory + "libhiasm-core-qt." G_MODULE_SUFFIX;
#endif
    auto *absolute = g_canonicalize_filename(path.c_str(), nullptr);
    HiasmQtGetWidgets getWidgets = nullptr;
#ifdef G_OS_WIN32
    auto *wide = g_utf8_to_utf16(absolute, -1, nullptr, nullptr, nullptr);
    if (wide) module = LoadLibraryExW(reinterpret_cast<LPCWSTR>(wide), nullptr,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    const auto loadError = GetLastError();
    g_free(wide);
    if (module) getWidgets = reinterpret_cast<HiasmQtGetWidgets>(GetProcAddress(module, "hiasm_qt_widgets"));
    else if (g_file_test(absolute, G_FILE_TEST_IS_REGULAR))
        std::cerr << "Cannot load Qt Core backend: Win32 error " << loadError << '\n';
#else
    module = g_module_open(absolute, G_MODULE_BIND_LOCAL);
    if (!module && g_file_test(absolute, G_FILE_TEST_IS_REGULAR))
        std::cerr << "Cannot load Qt Core backend: " << g_module_error() << '\n';
    gpointer entry = nullptr;
    if (module && g_module_symbol(module, "hiasm_qt_widgets", &entry))
        getWidgets = reinterpret_cast<HiasmQtGetWidgets>(entry);
#endif
    auto *pluginDir = g_path_get_dirname(absolute);
    g_free(absolute);
    static const HiasmQtHost host{property, event};
    if (getWidgets) api = getWidgets(2);
    const bool started = api && api->start(&host, pluginDir);
    g_free(pluginDir);
    if (!started) { api = nullptr; stop(); return false; }
    closing = false;
    // Qt retains process-wide platform state after QApplication destruction.
#ifndef G_OS_WIN32
    g_module_make_resident(module);
#endif
    return true;
}

void stop() {
    const bool initialized = api != nullptr;
    if (initialized) api->stop();
    handles.clear();
    pictures.clear();
    api = nullptr;
#ifdef G_OS_WIN32
    if (module && !initialized) FreeLibrary(module);
#else
    if (module) g_module_close(module);
#endif
    module = nullptr;
}

bool owns(Element *element) { return handles.count(element) != 0; }
bool active() { return api != nullptr; }
bool isWidget(Element *element) { return element->tpl->conf->cClass.compare(0, 5, "HCWE_") == 0; }
bool supports(Element *) {
    // The SDK editor is implemented by the Qt QPlainTextEdit bridge.
    // Keep this predicate explicit so new Core classes can be gated here.
    return true;
}
void pump() { if (api) api->pump(); }
bool closeRequested() { return closing; }

bool prepare(Element *element, Element *parent) {
    if (!api) return false;
    if (owns(element)) return true;
    const auto &kind = element->tpl->conf->cClass;
    if (kind.compare(0, 5, "HCWE_") != 0 && kind != "HCMessage" &&
        kind != "HCOpenDialog" && kind != "HCSaveDialog" && kind != "HCPopupMenu") return false;
    const auto owner = handles.find(parent);
    if (auto *handle = api->create(element, kind.c_str(), owner == handles.end() ? nullptr : owner->second)) {
        handles.emplace(element, handle);
        return true;
    }
    return false;
}

bool show(Element *element) {
    const auto item = handles.find(element);
    if (item == handles.end()) return false;
    event(element, "onCreate", "", 0, 0);
    if (!closing) api->show(item->second);
    return true;
}

bool work(ElementPoint *point, TData *data) {
    const auto item = handles.find(point->parent);
    if (item == handles.end()) return false;
    const auto &name = point->name;
    const char *inputName = nullptr;
    if (name == "doText" || name == "doText2" || name == "doAdd" || name == "doInsert") inputName = "Str";
    else if (name == "doLoad" || name == "doSave" || name == "doLoadFromFile" || name == "doExecute") inputName = "FileName";
    else if (name == "doMessage") inputName = "Message";
    else if (name == "doAddItem") inputName = "Text";
    else if (name == "doPosition") inputName = "Position";
    else if (name == "doSelect" || name == "doSelectByIndex") inputName = "Index";
    else if (name == "doValue") inputName = "Value";
    TData value(data);
    if (inputName) {
        if (auto *input = point->parent->getPointByName(inputName); input && input->type == pt_data && input->point)
            input->get_data(&value);
        else if (auto *prop = point->parent->props.getByName(inputName); prop && !prop->readStr().empty())
            value = prop->readStr();
        else if (data) value = data->pop();
    }
    const auto text = name == "doLoadFromImage" && value.toPixbuf()
        ? DataValue::serialize(data_pixbuf, value.toPixbuf()) : value.toStr();
    api->work(item->second, name.c_str(), text.c_str(), value.toReal());
    return true;
}

bool read(ElementPoint *point, TData *data) {
    const auto item = handles.find(point->parent);
    if (item == handles.end()) return false;
    const char *text = nullptr;
    double number = 0;
    const int type = api->read(item->second, point->name.c_str(), &text, &number);
    if (type == 1) *data = ustring(text);
    else if (type == 2) *data = static_cast<int>(number);
    else if (type == 3) *data = number;
    else if (type == 4) {
        auto &picture = pictures[point->parent];
        if (DataValue::parse(text, data_pixbuf, &picture)) *data = &picture;
    }
    return true;
}
}
