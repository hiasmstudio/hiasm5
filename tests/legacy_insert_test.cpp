// A palette element of a HiAsm 4 package (Delphi, PDA, CNet, ...) is inserted into the scheme by a click of the
// scheme editor, as for the C++ package: the palette starts the insert operation, the press places the element.
// Usage: legacy_insert_test <build dir> [pack codename]. Every element of the pack's palette is inserted once.
#include "SDK_Editor.h"
#include "Elements.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "WindowManager.h"
#include "commands.h"
#include "Pack.h"
#include <glib/gstdio.h>
#include <iostream>
#include <list>
#include <memory>
#include <string>
#include <vector>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)

static void drain() {
    const auto end = g_get_monotonic_time() + 60000;
    while(g_get_monotonic_time() < end) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(1000); }
}

int main(int argc, char **argv) {
    if(argc < 2) return 2;
    const std::string packName = argc > 2 ? argv[2] : "delphi";
    auto app = Gtk::Application::create("com.hiasm.legacy-insert-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
    initDirs(); dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    gchar *settingsFolder = g_dir_make_tmp("hiasm-legacy-insert-settings-XXXXXX", nullptr); CHECK(settingsFolder);
    homeDir = ustring(settingsFolder) + G_DIR_SEPARATOR_S; g_free(settingsFolder);
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    cmdMan = new CommandManager();
    compilerSet = new CompilerCollection(); packSet = new PackCollection();
    if(!curList) curList = new CursorList(Gdk::Display::get_default());

    Pack *pack = packSet->getPackByName(packName.c_str());
    if(!pack) { std::cout << "SKIP: HiAsm 4 package " << packName << " is not installed" << std::endl; return 77; }
    pack->ensureLoaded();

    // The palette of the package: what Palette::loadPack offers.
    std::vector<std::string> palette;
    for(PackGroup *group : pack->groups) {
        if(group->name.empty()) continue;
        for(PackElement *element : group->elements) {
            if(element->name.c_str()[0] == '*') continue;
            if(!pack->compiles(element->name)) continue;
            palette.push_back(element->name.raw());
        }
    }
    std::cout << "PALETTE " << packName << " " << palette.size() << " elements" << std::endl;

    gchar *dirTemp = g_dir_make_tmp("hiasm-legacy-insert-XXXXXX", nullptr); CHECK(dirTemp);
    const ustring dir(dirTemp); g_free(dirTemp);
    const ustring file = dir + "/Insert.sha";
    const std::string text = "Make(" + packName + ")\n";
    CHECK(g_file_set_contents(file.c_str(), text.c_str(), text.size(), nullptr));

    WindowManager manager;
    Gtk::Window window;
    app->add_window(window);
    window.set_child(manager);
    auto tab = dynamic_cast<WM_SDK_Editor*>(manager.open(file)); CHECK(tab);
    window.set_default_size(900, 600);
    compat_show_all(window); window.present(); drain();
    SDK_Editor *editor = tab->sde;
    CHECK(editor->sdk->pack == pack);

    std::vector<std::string> failed, occupied;
    int index = 0;
    for(const std::string &name : palette) {
        const size_t before = editor->csdk->elements.size();
        const double x = 40, y = 40;
        index++;
        ObjectType under;
        editor->csdk->getObjectAtPos(x, y, &under);
        if(under.type != SDK_OBJ_NONE) occupied.push_back(name + " under click: type " + std::to_string((int)under.type));
        editor->beginOperation(MS_INSERT_ELEMENT, name.c_str());
        GdkEventButton press = {};
        press.type = GDK_BUTTON_PRESS;
        press.button = BTN_LEFT;
        press.x = x; press.y = y;
        editor->on_button_press_event(&press); drain();
        GdkEventButton release = press;
        release.type = GDK_BUTTON_RELEASE;
        editor->on_button_release_event(&release); drain();
        const bool added = editor->csdk->elements.size() == before + 1;
        if(!added) {
            failed.push_back(name);
            PackElement *pe = pack->getElementByName(name);
            Element *direct = editor->csdk->add(name, 500, 500);
            std::cout << "DIAG " << name << " pe=" << (void*)pe << " class=" << (pe && pe->conf ? pe->conf->cClass.raw() : std::string("-"))
                      << " direct=" << (void*)direct << " sel=" << editor->csdk->selMan->elements.size() << std::endl;
            if(direct) { editor->csdk->selMan->clear(); editor->csdk->remove(direct); }
        }
        else {
            Element *e = editor->csdk->elements.back();
            const bool drawn = e->drawRect().get_width() > 0 && e->drawRect().get_height() > 0;
            if(!drawn) failed.push_back(name + " (not drawn)");
            editor->csdk->selMan->clear();
            editor->csdk->remove(e);
        }
        drain();
    }
    std::cout << "INSERTED " << (palette.size() - failed.size()) << " of " << palette.size() << std::endl;
    for(const auto &name : failed) std::cout << "FAILED " << name << std::endl;
    for(const auto &name : occupied) std::cout << "OCCUPIED " << name << std::endl;

    window.hide();
    window.unset_child();
    app->remove_window(window);
    g_unlink(file.c_str()); g_rmdir(dir.c_str());
    CHECK(failed.empty());
    std::cout << "OK legacy insert: every palette element of " << packName << " is placed by a click" << std::endl;
    return 0;
}
