#include "share.h"
#include "SDK.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "RunnerPayload.h"
#include <vector>
#include <memory>
#ifdef G_OS_WIN32
#include <glib/gwin32.h>
#endif

int main(int argc, char **argv) {
#ifdef G_OS_WIN32
    // The CRT's narrow argv uses the Windows code page, not UTF-8.
    std::unique_ptr<gchar*, decltype(&g_strfreev)> utf8Args(g_win32_get_command_line(), g_strfreev);
    if(!utf8Args) return 2;
    argv = utf8Args.get();
    argc = g_strv_length(argv);
#endif
    std::string scheme, runtime, schemeFile;
    bool validate = false;
    for(int i = 1; i < argc; ++i) {
        if(!strcmp(argv[i], "--validate")) validate = true;
        else if(!strcmp(argv[i], "--scheme") && i + 1 < argc) schemeFile = argv[++i];
        else if(!strcmp(argv[i], "--data-dir") && i + 1 < argc) runtime = argv[++i];
        else { std::cerr << "Unknown or incomplete runner option: " << argv[i] << std::endl; return 2; }
    }
    if(schemeFile.empty()) {
        gchar *path = g_find_program_in_path(argv[0]);
        gchar *data = NULL;
        gsize size = 0;
        bool loaded = path && g_file_get_contents(path, &data, &size, NULL);
        g_free(path);
        std::string embeddedRuntime;
        bool valid = loaded && RunnerPayload::extract(std::string(data, size), scheme, embeddedRuntime);
        g_free(data);
        if(!valid) { std::cerr << "No valid embedded scheme; use --scheme FILE" << std::endl; return 2; }
        if(runtime.empty()) runtime = embeddedRuntime;
    }
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.runner", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs();
    const char *overrideDir = g_getenv("HIASM_DATA_DIR");
    if(overrideDir && *overrideDir) runtime = overrideDir;
    if(!runtime.empty()) dataDir = runtime + G_DIR_SEPARATOR_S;
    if(!file_test(dataDir + INT_PATH + DATABASE_FILE, FILE_TEST_EXISTS)) {
        std::cerr << "HiAsm runtime data not found: " << dataDir << std::endl; return 3;
    }
    mdb.open();
    SettingsManager::init();
    compilerSet = new CompilerCollection();
    packSet = new PackCollection();
    MSDK sdk;
    bool loaded;
    if(!schemeFile.empty()) loaded = sdk.loadFromFile(schemeFile);
    else {
        std::vector<char> buffer(scheme.begin(), scheme.end());
        buffer.push_back(0);
        gchar *cursor = buffer.data();
        int line = 1;
        loaded = sdk.loadFromText(cursor, line, ELEMENT_LOAD_FILE) == SDK_PERR_NONE && sdk.pack;
    }
    if(!loaded || !sdk.pack || sdk.pack->name != "_base") {
        std::cerr << "Unable to load an internal (_base) scheme" << std::endl; return 4;
    }
    if(validate) { std::cout << "OK runner scheme: " << sdk.elements.size() << " elements" << std::endl; return 0; }
    auto loop = Glib::MainLoop::create();
    sdk.run();
    auto stopped = Glib::signal_timeout().connect([&sdk, loop]() {
        if(!sdk.isRun()) { loop->quit(); return false; }
        return true;
    }, 50);
    loop->run();
    stopped.disconnect();
    if(sdk.isRun()) sdk.stop();
    return 0;
}
