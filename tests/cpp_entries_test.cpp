// Windows-only and plugin project types of the C++ package. Each entry is built by
// the real compiler and its generated source must export what its host expects.
#include "SDK.h"
#include "CodeGen.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "ProjectBuilder.h"
#include <glib/gstdio.h>
#include <iostream>
#include <string>

struct DebugLog : CallBack {
    std::string log;
    void callback(void *, CallbackType, const void *data) override {
        if(data) { log += (const char*)data; log += '\n'; }
    }
};

struct EntryCase {
    const char *entry;
    std::initializer_list<const char*> exports;
};

int main(int argc, char **argv) {
    if(argc != 2) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.cpp-entries-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs();
    dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    compilerSet = new CompilerCollection(); packSet = new PackCollection();
    Pack *pack = packSet->getPackByName("cpp");
    if(!pack) return 12;
    gchar *temp = g_dir_make_tmp("hiasm-entries-test-XXXXXX", nullptr);
    if(!temp) return 14;
    const ustring dir(temp); g_free(temp);

    const EntryCase cases[] = {
        {"WinDll", {"hiasm_run", "DllMain"}},
        {"Plugin", {"hiasm_plugin_info", "hiasm_plugin_run"}},
        {"ComExt", {"DllGetClassObject", "DllCanUnloadNow", "DllRegisterServer", "InprocServer32"}},
        {"WinService", {"StartServiceCtrlDispatcherA", "RegisterServiceCtrlHandlerA", "SetServiceStatus"}},
        {"CplApp", {"CPlApplet", "CPL_NEWINQUIRE"}},
    };
    ProjectBuilder builder;
    for(const auto &entryCase : cases) {
        PackProject *project = pack->getProjectByEntry(entryCase.entry);
        if(!project) return 13;
        MSDK scheme;
        scheme.pack = pack;
        scheme.packProject = project;
        scheme.fileName = dir + "/" + entryCase.entry + ".sha";
        Element *entry = scheme.add(entryCase.entry, 0, 0);
        if(!entry) return 110;
        entry->flag |= ELEMENT_FLG_IS_PARENT;

        DebugLog debug;
        cgt_on_debug += &debug;
        const bool built = builder.build(&scheme);
        cgt_on_debug -= &debug;
        if(!built) { std::cerr << entryCase.entry << " build failed:\n" << debug.log << std::endl; return 111; }
        const ustring artifact = builder.getOutputProjectName(&scheme);
        if(!g_file_test(artifact.c_str(), G_FILE_TEST_IS_REGULAR)) return 114;
        gchar *folder = g_path_get_dirname(artifact.c_str());
        const ustring source = ustring(folder) + "/" + entryCase.entry + ".cpp";
        gchar *text = nullptr; gsize size = 0;
        const bool readSource = g_file_get_contents(source.c_str(), &text, &size, nullptr);
        const std::string generated = readSource ? std::string(text, size) : std::string();
        g_free(text);
        const char *missing = nullptr;
        for(const char *fragment : entryCase.exports)
            if(!missing && generated.find(fragment) == std::string::npos) missing = fragment;
        g_unlink(artifact.c_str()); g_unlink(source.c_str()); g_rmdir(folder);
        g_free(folder);
        if(!readSource) { std::cerr << "Generated source is missing for " << entryCase.entry << std::endl; return 112; }
        if(missing) { std::cerr << entryCase.entry << " lacks: " << missing << std::endl; return 113; }
    }
    g_rmdir((dir + "/build").c_str()); g_rmdir(dir.c_str());
    std::cout << "OK cpp entries: WinDll, Plugin, ComExt, WinService and CplApp build and export their entry points" << std::endl;
    return 0;
}
