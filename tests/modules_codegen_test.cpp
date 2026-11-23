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
    auto app = Gtk::Application::create("com.hiasm.modules-codegen-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs(); dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    compilerSet = new CompilerCollection(); packSet = new PackCollection();
#ifdef G_OS_WIN32
    const ustring modulePath = dataDir + "packs/modules/compiler/CodeGen.dll";
#else
    const ustring modulePath = dataDir + "packs/modules/compiler/libCodeGen.so";
#endif
    GModule *module = g_module_open(modulePath.c_str(), G_MODULE_BIND_LAZY);
    int (*generate)(BuildRecord *) = nullptr;
    void (*release)(void *) = nullptr;
    if(!module || !g_module_symbol(module, "buildProcessProc", (gpointer*)&generate) ||
       !g_module_symbol(module, "freeBuildResult", (gpointer*)&release)) return 10;
    MSDK empty; empty.pack = packSet->getPackByName("modules");
    BuildRecord invalid = {GetCGTools, &empty, nullptr};
    if(generate(&invalid) == 0 || invalid.result) return 11;
    release(nullptr);

    const char *entries[] = {"HISEntryPoint", "MakeEntryPoint"};
    const char *markers[] = {"sys.message(", "MessageBoxA("};
    BuildLog log; cgt_on_debug += &log;
    for(unsigned i = 0; i < 2; ++i) {
        MSDK sdk;
        if(!sdk.loadFromFile(dataDir + "packs/modules/new/" + entries[i] + ".sha") || !sdk.packProject) return 4;
        Element *entry = sdk.elements.front();
        Element *message = sdk.add("Message", 100, 100);
        if(!message) return 9;
        ustring text = "RTCG", caption = "RTCG Caption";
        message->props.getByName("Text")->setValue(&text);
        message->props.getByName("Caption")->setValue(&caption);
        if(i == 1) {
            Element *loop = sdk.add("For", 200, 100);
            Element *dir = sdk.add("DirOperation", 300, 100);
            if(!loop || !dir) return 9;
            int startValue = 0, endValue = 3, stepValue = 1;
            loop->props.getByName("Start")->setValue(&startValue);
            loop->props.getByName("End")->setValue(&endValue);
            loop->props.getByName("Step")->setValue(&stepValue);
            ustring source = "RTCG";
            dir->props.getByName("Source")->setValue(&source);
            entry->points.getByName("onStart")->connect(loop->points.getByName("doFor"));
            loop->points.getByName("onEvent")->connect(dir->points.getByName("doOperation"));
            dir->points.getByName("onOperation")->connect(message->points.getByName("doMessage"));
        }
        else entry->points.getByName("onStart")->connect(message->points.getByName("doMessage"));
        for(unsigned repeat = 0; repeat < 10; ++repeat) {
            BuildRecord rec = {GetCGTools, &sdk, nullptr};
            if(generate(&rec) != 0 || !rec.result) { std::cerr << log.text; return 12; }
            const std::string generated(static_cast<char*>(rec.result));
            release(rec.result);
            if(generated.empty() || generated.find(markers[i]) == std::string::npos ||
               generated.find("RTCG") == std::string::npos ||
               (i == 1 && (generated.find("for(") == std::string::npos ||
                           generated.find("SetCurrentDirectory(") == std::string::npos))) {
                std::cerr << "BAD OUTPUT entry=" << entries[i] << "\n" << generated << "\nLOG=" << log.text << std::endl;
                return 13;
            }
            for(Element *element : sdk.elements) if(element->userData) return 14;
        }
    }
    cgt_on_debug -= &log;
    g_module_close(module);
    std::cout << "OK RTCG: Modules HIS and make entries generated" << std::endl;
}
