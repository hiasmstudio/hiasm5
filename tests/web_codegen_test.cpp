#include "SDK.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "ProjectBuilder.h"
#include "CodeGen.h"
#include <glib/gstdio.h>
#include <iostream>

struct BuildLog : CallBack {
    std::string text;
    void callback(void *, CallbackType, const void *data) override {
        if(data) { text += (const char*)data; text += char(10); }
    }
};
struct BuildRecord { void *cgt; MSDK *sdk; void *result; };
int main(int argc, char **argv) {
    if(argc != 2) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.web-codegen-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs(); dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    compilerSet = new CompilerCollection(); packSet = new PackCollection();
#ifdef G_OS_WIN32
    const ustring modulePath = dataDir + "packs/web/compiler/CodeGen.dll";
#else
    const ustring modulePath = dataDir + "packs/web/compiler/libCodeGen.so";
#endif
    GModule *module = g_module_open(modulePath.c_str(), G_MODULE_BIND_LAZY);
    int (*generate)(BuildRecord *) = nullptr;
    void (*release)(void *) = nullptr;
    if(!module || !g_module_symbol(module, "buildProcessProc", (gpointer*)&generate) ||
       !g_module_symbol(module, "freeBuildResult", (gpointer*)&release)) return 10;
    MSDK empty; empty.pack = packSet->getPackByName("web");
    BuildRecord invalid = {GetCGTools, &empty, nullptr};
    if(generate(&invalid) == 0 || invalid.result) return 11;
    release(nullptr);
    gchar *temp = g_dir_make_tmp("hiasm-web-XXXXXX", nullptr);
    if(!temp) return 3;
    const ustring dir(temp); g_free(temp);
    const char *entries[] = {"HtmlEntryPoint", "CSSEntryPoint", "JavaEntryPoint", "EntryPoint"};
    const char *extensions[] = {"html", "css", "js", "php"};
    BuildLog log; cgt_on_debug += &log;
    for(unsigned i = 0; i < 4; ++i) {
        MSDK sdk;
        if(!sdk.loadFromFile(dataDir + "packs/web/new/" + entries[i] + ".sha") || !sdk.packProject) return 4;
        Element *entry = sdk.elements.front();
        Element *body = sdk.add(i == 1 ? "CSS" : "Echo", 100, 100);
        if(!body) return 9;
        ustring message = u8"Привет 世界";
        if(i == 1) {
            ustring tag = "body", color = "red";
            body->props.getByName("Tag")->setValue(&tag);
            body->props.getByName("Color")->setValue(&color);
        } else body->props.getByName("Message")->setValue(&message);
        entry->points.getByName("onStart")->connect(body->points.getByName(i == 1 ? "doPrint" : "doEcho"));
        sdk.fileName = dir + u8"/Пример с пробелами.sha";
        for(unsigned repeat = 0; repeat < 10; ++repeat) {
            BuildRecord rec = {GetCGTools, &sdk, nullptr};
            if(generate(&rec) != 0 || !rec.result) return 12;
            const std::string generated(static_cast<char*>(rec.result));
            release(rec.result);
            if(generated.empty() || (i != 1 && generated.find(message.raw()) == std::string::npos)) return 13;
            for(Element *element : sdk.elements) if(element->userData) return 14;
        }
        ProjectBuilder builder;
        gchar *before = g_get_current_dir();
        if(!builder.build(&sdk)) { std::cerr << log.text; return 5; }
        gchar *after = g_get_current_dir();
        if(strcmp(before, after)) return 6;
        g_free(before); g_free(after);
        const ustring output = dir + u8"/build/Пример с пробелами." + extensions[i];
        gchar *contents = nullptr; gsize size = 0;
        if(!g_file_get_contents(output.c_str(), &contents, &size, nullptr) || !size ||
           !g_utf8_validate(contents, size, nullptr)) { std::cerr << log.text; return 7; }
        g_free(contents); g_unlink(output.c_str());
        for(Element *element : sdk.elements) if(element->userData) return 8;
    }
    cgt_on_debug -= &log;
    g_module_close(module);
    g_rmdir((dir + "/build").c_str()); g_rmdir(dir.c_str());
    std::cout << "OK RTCG: HTML, CSS, JavaScript and PHP generated through ProjectBuilder" << std::endl;
}
