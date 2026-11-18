// Creating a new project from the project manager. Each new-project template is
// loaded the way the editor loads a new project, then the form project (MainForm)
// is opened the same way the New project command opens it.
#include "Elements.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "WindowManager.h"
#include "commands.h"
#include "ProjectBuilder.h"
#include <glib/gstdio.h>
#include <iostream>
#include <string>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)

static void drain() {
    const auto end = g_get_monotonic_time() + 300000;
    while(g_get_monotonic_time() < end) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(1000); }
}

// Finds the generated MainForm.cpp below the build folder (the platform folder name varies).
static std::string findGenerated(const std::string &dir) {
    GDir *handle = g_dir_open(dir.c_str(), 0, nullptr);
    if(!handle) return std::string();
    std::string found;
    while(const gchar *name = g_dir_read_name(handle)) {
        const std::string path = dir + "/" + name;
        if(std::string(name) == "MainForm.cpp" && g_file_test(path.c_str(), G_FILE_TEST_IS_REGULAR))
            found = path;
        else if(g_file_test(path.c_str(), G_FILE_TEST_IS_DIR))
            found = findGenerated(path);
        if(!found.empty()) break;
    }
    g_dir_close(handle);
    return found;
}

int main(int argc, char **argv) {
    if(argc != 2) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.new-project-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs(); dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    gchar *settingsFolder = g_dir_make_tmp("hiasm-newproject-settings-XXXXXX", nullptr); CHECK(settingsFolder);
    homeDir = ustring(settingsFolder) + G_DIR_SEPARATOR_S; g_free(settingsFolder);
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    cmdMan = new CommandManager();
    compilerSet = new CompilerCollection(); packSet = new PackCollection();

    // The C++ package is the pack whose projects include the form project.
    Pack *owner = nullptr;
    for(Pack *candidate : *packSet) if(candidate->getProjectByEntry("MainForm")) { owner = candidate; break; }
    CHECK(owner);
    PackProject *form = owner->getProjectByEntry("MainForm");
    CHECK(form);
    const ustring templateFile = form->getTemplateFile();
    CHECK(g_file_test(templateFile.c_str(), G_FILE_TEST_IS_REGULAR));

    // Which new-project templates load at all, in the editor's own loader.
    for(const char *name : {"Entry", "Gui", "Daemon", "Library", "MainForm"}) {
        MSDK probe;
        const bool loaded = probe.loadFromFile(owner->path() + "new/" + name + ".sha");
        std::cout << "template " << name << " loads: " << (loaded ? "yes" : "NO") << std::endl;
    }

    WindowManager manager;
    Gtk::Window managerWindow;
#ifdef HIASM_GTK4
    app->add_window(managerWindow);
    managerWindow.set_child(manager);
#else
    managerWindow.add(manager);
#endif
    compat_show_all(managerWindow); managerWindow.present(); drain();
    // The call the New project command makes after the dialog returns the form project.
    auto tab = dynamic_cast<WM_SDK_Editor*>(manager.open(templateFile, true));
    CHECK(tab);
    drain();
    CHECK(tab->sde && tab->sde->sdk && tab->sde->sdk->pack == owner);
    CHECK(tab->sde->sdk->elements.size() == 1);
    // Building the new project must produce a program.
    gchar *buildDir = g_dir_make_tmp("hiasm-newproject-build-XXXXXX", nullptr); CHECK(buildDir);
    tab->sde->sdk->fileName = ustring(buildDir) + "/MainForm.sha";
    const std::string buildPath = buildDir;
    g_free(buildDir);
    ProjectBuilder builder;
    const bool built = builder.build(tab->sde->sdk);
    std::cout << "build of the new form project: " << (built ? "yes" : "NO") << std::endl;
    CHECK(built);
    // The window takes the frame of BorderStyle (default bsSizeable) and the Windows CSD switch.
    const std::string source = findGenerated(buildPath);
    CHECK(!source.empty());
    gchar *text = nullptr; gsize length = 0;
    CHECK(g_file_get_contents(source.c_str(), &text, &length, nullptr));
    const std::string code(text, length);
    g_free(text);
    // BorderStyle 2 (bsSizeable) with the default NativeFrame = True: the standard Windows frame.
    CHECK(code.find("hiasm_apply_frame(GTK_WINDOW(window), 2, 1);") != std::string::npos);
    CHECK(code.find("gtk_window_set_decorated(window, border >= 1 && border <= 5);") != std::string::npos);
    CHECK(code.find("g_setenv(\"GTK_CSD\", \"0\", TRUE);") != std::string::npos);
    CHECK(code.find("hiasm_native_style(GTK_WINDOW(window), 0);") != std::string::npos);
    CHECK(code.find("g_signal_connect(window, \"map\"") != std::string::npos);
    // MainForm properties: defaults from packs/cpp/conf/MainForm.ini (Width 400, Height 300, Color clBtnFace).
    CHECK(code.find("hiasm_restore_size(GTK_WINDOW(window), 400, 300);") != std::string::npos);
    CHECK(code.find("hiasm_set_alpha(GTK_WINDOW(window), 255);") != std::string::npos);
    CHECK(code.find("hiasm_set_window_background(window, \"#") != std::string::npos);
    CHECK(code.find("hiasm_win_taskbar(GTK_WINDOW(window), 0);") != std::string::npos);
    CHECK(code.find("hiasm_apply_state(GTK_WINDOW(window), 0, 0, 1, 400, 300);") != std::string::npos);
    // DragForm defaults to False, so the drag gesture is not generated.
    CHECK(code.find("gtk_gesture_drag_new") == std::string::npos);

    managerWindow.hide();
#ifdef HIASM_GTK4
    managerWindow.unset_child();
    app->remove_window(managerWindow);
#else
    managerWindow.remove();
#endif
    g_rmdir(homeDir.c_str());
    std::cout << "OK new project: the form template opens in the designer" << std::endl;
    return 0;
}
