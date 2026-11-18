#include "WindowManager.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "Localization.h"
#include "RecentFileManager.h"
#include "commands.h"
#include <glib/gstdio.h>
#include <iostream>

#define CHECK(expr) do { if(!(expr)) { std::cerr << __LINE__ << ": " << #expr << std::endl; return 1; } } while(0)
static bool copyFile(const ustring &source, const ustring &destination) {
    gchar *bytes = nullptr; gsize size = 0;
    if(!g_file_get_contents(source.c_str(), &bytes, &size, nullptr)) return false;
    const bool result = g_file_set_contents(destination.c_str(), bytes, size, nullptr);
    g_free(bytes); return result;
}
int main(int argc, char **argv) {
    if(argc != 2) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.core-welcome-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    gchar *tmp = g_dir_make_tmp("hiasm-core-welcome-XXXXXX", nullptr);
    CHECK(tmp);
    const ustring dir(tmp); g_free(tmp);
    dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    const auto runtime = dataDir;
    homeDir = dir + "/";
    databaseFile = homeDir + DATABASE_FILE;
    CHECK(copyFile(dataDir + INT_PATH + DATABASE_FILE, databaseFile));
    initializedUserProfile = true;
    mdb.open();
    CHECK(!initializedUserProfile && ui_language() == ui_system_language());
    CHECK(mdb.setLanguage("ru")); mdb.close(); mdb.open();
    CHECK(ui_language() == "ru");
    mdb.save_settings_value(IDS_NEW_PROJECT, "1");
    SettingsManager::init();
    cmdMan = new CommandManager();
    compilerSet = new CompilerCollection();
    packSet = new PackCollection();
    rfMan.load();
    for(const char *language : {"ru", "en"}) {
        MSDK welcome;
        const auto intro = runtime + "packs/_base/" + (std::string(language) == "ru" ? "lang/ru/example/tutorial/intro.sha" : "example/tutorial/intro.sha");
        CHECK(welcome.loadFromFile(intro));
        CHECK(welcome.isCoreDocument() && welcome.isInstalledCoreScheme() && welcome.elements.size() == 5);
        CHECK(std::string(HIASM_VERSION()) == "6.0.0.0");
        const auto title = welcome.elements.front()->props.getByName("Info")->readStr();
        CHECK(title.find(std::string(language) == "ru" ? u8"Добро пожаловать" : "Welcome") != ustring::npos);
        const auto saved = dir + "/" + language + ".sha";
        CHECK(welcome.saveToFile(saved) && !welcome.isInstalledCoreScheme() && welcome.isCoreDocument());
        MSDK restored;
        CHECK(restored.loadFromFile(saved) && restored.isCoreDocument());
#ifdef HIASM_GTK4
        auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, 740, 700);
#else
        auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 740, 700);
#endif
        auto dc = Cairo::Context::create(surface);
        dc->set_source_rgb(1,1,1); dc->paint(); restored.draw(dc, 1);
        // Match InfoTip's real Pango layout and fail if onboarding text is clipped.
        for(auto element : restored.elements) {
            auto layout = Pango::Layout::create(dc);
            layout->set_text(element->props.getByName("Info")->readStr());
            layout->set_font_description(Pango::FontDescription("arial 10"));
            layout->set_wrap(Pango::WRAP_WORD);
            layout->set_indent(10);
            layout->set_width((element->width - 24) * Pango::SCALE);
            int width = 0, height = 0;
            layout->get_pixel_size(width, height);
            CHECK(width <= element->width - 24 && height <= element->height - 24);
        }
        surface->write_to_png((runtime + "core-welcome-" + language + ".png").raw());
        ProjectBuilder builder;
        CHECK(!builder.build(&restored));
        builder.run(&restored); CHECK(!builder.isRunning());
    }
    MSDK internal;
    CHECK(internal.loadFromFile(runtime + "packs/_base/new/hcTplForm.sha") && !internal.isCoreDocument());
    {
        WindowManager manager; manager.init();
        auto welcome = dynamic_cast<WM_SDK_Editor*>(manager.getCurrent());
        CHECK(welcome && welcome->sde->sdk->isCoreDocument());
        CHECK(welcome->fileName() == runtime + "packs/_base/lang/ru/example/tutorial/intro.sha");
        CHECK(g_file_test((homeDir + "welcome-core-v1.seen").c_str(), G_FILE_TEST_IS_REGULAR));
        CHECK(!cmdMan->isEnabled(CMD_BUILD) && !cmdMan->isEnabled(CMD_RUN));
        CHECK(!cmdMan->isEnabled(CMD_FORMEDIT));
        CHECK(welcome->processCommand(CMD_BUILD, "") && welcome->processCommand(CMD_RUN, ""));
        // Windows GTK4 uses a native Win32 chooser, not a GtkDialog. Exercise
        // cancellation on GTK3/Linux; native chooser acceptance needs UI QA.
#if !defined(HIASM_GTK4) || !defined(G_OS_WIN32)
        // Even a writable bundled lesson must route to Save As.
        bool saveAsShown = false;
        auto cancelSave = Glib::signal_timeout().connect([&]() {
            for(auto window : Gtk::Window::list_toplevels()) {
                auto dialog = dynamic_cast<Gtk::FileChooserDialog*>(window);
                if(dialog && dialog->get_visible()) {
                    saveAsShown = true;
                    dialog->response(0);
                    return false;
                }
            }
            return true;
        }, 10);
        ustring changedText = "An edited onboarding document";
        welcome->sde->sdk->elements.front()->props.getByName("Info")->setValue(&changedText);
        welcome->sde->changeScheme();
        CHECK(welcome->processCommand(CMD_SAVE, ""));
        cancelSave.disconnect();
        CHECK(saveAsShown && welcome->changed && welcome->sde->sdk->isInstalledCoreScheme());
        welcome->setChanged(false);
#endif
        const auto count = manager.tabs.size();
        CHECK(manager.openWelcome() && manager.tabs.size() == count);
        CHECK(manager.closeAll());
    }
    // Isolated fixture: never rename or corrupt the real installation files.
    dataDir = dir + "/welcome-fixture/";
    const auto fixtureRoot = dataDir + "packs/_base/";
    CHECK(g_mkdir_with_parents((dataDir + ICONS_PATH).c_str(), 0700) == 0);
    CHECK(copyFile(runtime + INT_PATH "close.png", dataDir + INT_PATH "close.png"));
    CHECK(copyFile(runtime + ICONS_PATH "components.png", dataDir + ICONS_PATH "components.png"));
    CHECK(g_mkdir_with_parents((fixtureRoot + "lang/ru/example/tutorial").c_str(), 0700) == 0);
    CHECK(g_mkdir_with_parents((fixtureRoot + "example/tutorial").c_str(), 0700) == 0);
    CHECK(copyFile(runtime + "packs/_base/example/tutorial/intro.sha", fixtureRoot + "example/tutorial/intro.sha"));
    CHECK(copyFile(runtime + "packs/_base/lang/ru/example/tutorial/intro.sha", fixtureRoot + "lang/ru/example/tutorial/intro.sha"));
    ui_set_language("ru");
    g_unlink((homeDir + "welcome-core-v1.seen").c_str());
    {
        WindowManager manager; manager.init();
        CHECK(manager.getCurrent() && manager.getCurrent()->fileName() == fixtureRoot + "lang/ru/example/tutorial/intro.sha");
        CHECK(g_file_test((homeDir + "welcome-core-v1.seen").c_str(), G_FILE_TEST_IS_REGULAR));
        CHECK(manager.closeAll());
    }
    g_unlink((fixtureRoot + "lang/ru/example/tutorial/intro.sha").c_str());
    g_unlink((homeDir + "welcome-core-v1.seen").c_str());
    {
        WindowManager manager; manager.init();
        CHECK(manager.getCurrent() && manager.getCurrent()->fileName() == fixtureRoot + "example/tutorial/intro.sha");
        CHECK(manager.closeAll());
    }
    rfMan.clear();
    g_unlink((fixtureRoot + "example/tutorial/intro.sha").c_str());
    g_unlink((homeDir + "welcome-core-v1.seen").c_str());
    {
        WindowManager manager; manager.init();
        CHECK(manager.getCurrent() && manager.getCurrent()->type == WMT_NEW_PROJECT);
        CHECK(!g_file_test((homeDir + "welcome-core-v1.seen").c_str(), G_FILE_TEST_EXISTS));
        CHECK(manager.closeAll());
    }
    dataDir = runtime;
    CHECK(g_file_set_contents((homeDir + "welcome-core-v1.seen").c_str(), "1\n", -1, nullptr));
    {
        WindowManager manager; manager.init();
        CHECK(manager.getCurrent() && manager.getCurrent()->type == WMT_NEW_PROJECT);
        CHECK(manager.closeAll());
    }
    CHECK(g_file_set_contents((homeDir + "desktop.lst").c_str(), (dir + "/en.sha").c_str(), -1, nullptr));
    {
        WindowManager manager; manager.init();
        CHECK(manager.tabs.size() == 1 && manager.getCurrent()->fileName() == dir + "/en.sha");
        CHECK(manager.closeAll());
    }
    g_unlink((homeDir + "desktop.lst").c_str());
    g_unlink((homeDir + "welcome-core-v1.seen").c_str());
    ui_set_language("en");
    {
        WindowManager manager; manager.init();
        CHECK(manager.getCurrent()->fileName() == runtime + "packs/_base/example/tutorial/intro.sha");
        CHECK(manager.closeAll());
    }
    mdb.close();
    g_unlink(databaseFile.c_str());
    g_unlink((homeDir + "welcome-core-v1.seen").c_str());
    g_unlink((homeDir + "ru.sha").c_str()); g_unlink((homeDir + "en.sha").c_str());
    // The recent-file singleton flushes its private temporary profile on exit.
    std::cout << "OK Core welcome: localized docs, layout bounds, no build/run, Save As, first launch, restore, fallback, internal forms" << std::endl;
    return 0;
}
