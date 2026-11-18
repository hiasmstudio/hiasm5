#include "FormEditor.h"
#include "Elements.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "WindowManager.h"
#include "commands.h"
#include <glib/gstdio.h>
#include <iostream>
#include <memory>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)
static void drain() {
    const auto end = g_get_monotonic_time() + 150000;
    while(g_get_monotonic_time() < end) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(1000); }
}
class Editor : public WidgetEditorEx {
public:
    explicit Editor(SDK *sdk) : WidgetEditorEx(sdk) {}
    void press(int x, int y, guint state = 0) { GdkEventButton event = {}; event.button = BTN_LEFT; event.x = x; event.y = y; event.state = state; on_button_press_event(&event); }
    void move(int x, int y) { GdkEventMotion event = {}; event.x = x; event.y = y; on_motion_notify_event(&event); }
    void release(int x, int y) { GdkEventButton event = {}; event.button = BTN_LEFT; event.x = x; event.y = y; on_button_release_event(&event); }
};
int main(int argc, char **argv) {
    if(argc != 2) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.form-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs(); dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    gchar *settingsFolder = g_dir_make_tmp("hiasm-form-settings-XXXXXX", nullptr); CHECK(settingsFolder);
    homeDir = ustring(settingsFolder) + G_DIR_SEPARATOR_S; g_free(settingsFolder);
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    cmdMan = new CommandManager();
    compilerSet = new CompilerCollection(); packSet = new PackCollection();
    // Native toggle widgets must reflect the persisted command state.
    auto toolbar = std::unique_ptr<Toolbar>(cmdMan->createToolbar(MENUNAME_TOOLBAR));
    const auto bindingButton = [&](const char *name) -> CM_ToggleToolButton* {
        auto item = cmdMan->findByName(name);
        if(!item) return nullptr;
        for(auto widget : item->widgets) if(auto toggle = dynamic_cast<CM_ToggleToolButton*>(widget)) return toggle;
        return nullptr;
    };
    CHECK(bindingButton(CMD_BIND_RECT) && bindingButton(CMD_BIND_CENTER) && bindingButton(CMD_BIND_PADDING));
    MSDK sdk; sdk.pack = packSet->getPackByName("_base");
    CHECK(sdk.add("hcTplFixed",0,0));
    Element *a = sdk.add("hcTplButton",20,30), *b = sdk.add("hcTplButton",180,120);
    CHECK(a && b);
    auto set = [](Element *e, const char *name, int value) { e->props.getByName(name)->setValue(&value); };
    set(a,"Width",80); set(a,"Height",40); set(b,"Width",120); set(b,"Height",40);
    Gtk::Window window;
    window.set_default_size(640,480);
#ifdef HIASM_GTK4
    app->add_window(window);
#endif
    {
        Editor editor(&sdk);
#ifdef HIASM_GTK4
        window.set_child(editor);
#else
        window.add(editor);
#endif
        compat_show_all(window); window.present(); drain();
        editor.press(30,40); editor.move(45,60); editor.release(45,60); drain();
        CHECK(sdk.selMan->getFirst() == a);
        CHECK(a->props[0]->readInt() == 35 && a->props[1]->readInt() == 50);
        editor.press(190,130,GDK_SHIFT_MASK); editor.release(190,130); drain();
        CHECK(sdk.selMan->elements.size() == 2);
        // Resize from the union's bottom-right grip (not the first widget's).
        editor.press(300,160); editor.move(315,170); editor.release(315,170); drain();
        CHECK(a->props[2]->readInt() == 95 && a->props[3]->readInt() == 50);
        CHECK(b->props[2]->readInt() == 135 && b->props[3]->readInt() == 50);
        a->flag |= ELEMENT_FLG_IS_FREEZE;
        editor.press(45,60); editor.move(55,70); editor.release(55,70); drain();
        CHECK(a->props[0]->readInt() == 35 && b->props[0]->readInt() == 190);
        a->flag &= ~ELEMENT_FLG_IS_FREEZE;
        sdk.selMan->select(a);
        const auto reset = [&]() { set(a,"Left",20); set(a,"Top",30); set(a,"Width",80); set(a,"Height",40); set(b,"Left",180); set(b,"Top",120); set(b,"Width",120); set(b,"Height",40); drain(); };
        reset();
        SettingsManager::setFormBinding(SettingsManager::BindEdges, true);
        editor.press(30,40); editor.move(185,40); editor.release(185,40); drain();
        CHECK(a->props[0]->readInt() == 180);
        reset();
        editor.press(100,50); editor.move(175,50); editor.release(175,50); drain();
        CHECK(a->props[0]->readInt() == 20 && a->props[2]->readInt() == 160);
        SettingsManager::setFormBinding(SettingsManager::BindEdges, false);
        reset();
        SettingsManager::setFormBinding(SettingsManager::BindCenter, true);
        editor.press(30,40); editor.move(205,40); editor.release(205,40); drain();
        CHECK(a->props[0]->readInt() == 200);
        SettingsManager::setFormBinding(SettingsManager::BindCenter, false);
        reset();
        SettingsManager::setFormBinding(SettingsManager::BindPadding, true);
        editor.press(30,40); editor.move(310,40); editor.release(310,40); drain();
        CHECK(a->props[0]->readInt() == 305);
        SettingsManager::setFormBinding(SettingsManager::BindPadding, false);
        sdk.selMan->clear();
        editor.beginInsert("hcTplLabel"); editor.press(350,220); drain();
        CHECK(sdk.elements.size() == 4 && sdk.selMan->getFirst()->tpl->name == "hcTplLabel");
        CHECK(sdk.selMan->getFirst()->props[0]->readInt() == 350);
        sdk.selMan->erase(); drain(); CHECK(sdk.elements.size() == 3);
#ifdef HIASM_GTK4
        window.unset_child();
#else
        window.remove();
#endif
    }
    window.hide();
#ifdef HIASM_GTK4
    app->remove_window(window);
#endif
    CHECK(dynamic_cast<ElementTplWinControl*>(a)->ctl == nullptr);
    // Exercise the real tab controller: closing/reopening and replacing the model with undo.
    gchar *temp = g_dir_make_tmp("hiasm-form-history-XXXXXX", nullptr); CHECK(temp);
    const ustring dir(temp); g_free(temp);
    const ustring file = dir + "/form.sha";
    CHECK(g_file_set_contents(file.c_str(), "Make(_base)\nAdd(hcTplForm,1,0,0)\n{\n}\nAdd(hcTplButton,2,20,30)\n{\n Width=80\n Height=40\n}\n", -1, nullptr));
    {
        WindowManager manager;
        Gtk::Window managerWindow;
#ifdef HIASM_GTK4
        app->add_window(managerWindow);
        managerWindow.set_child(manager);
#else
        managerWindow.add(manager);
#endif
        auto tab = dynamic_cast<WM_SDK_Editor*>(manager.open(file)); CHECK(tab);
        compat_show_all(managerWindow); managerWindow.present(); drain();
        CHECK(manager.getCurrent() == tab);
        auto button = [&]() { return dynamic_cast<ElementTplWinControl*>(tab->sde->sdk->getElementById(2)); };
        auto command = [&](const char *cmd) { return tab->processCommand(cmd, ""); };
        CHECK(command(CMD_FORMEDIT)); CHECK(WidgetEditorEx::getActive(tab->sde) && button()->ctl);
        CHECK(cmdMan->isEnabled(CMD_BIND_RECT));
        CHECK(command(CMD_BIND_RECT) && SettingsManager::formBinding(SettingsManager::BindEdges));
        CHECK(bindingButton(CMD_BIND_RECT)->get_active());
        CHECK(!tab->changed);
        CHECK(command(CMD_BIND_RECT) && !SettingsManager::formBinding(SettingsManager::BindEdges));
        CHECK(!bindingButton(CMD_BIND_RECT)->get_active());
        tab->sde->beginOperation(MS_SCROLL);
        for(int i = 1; i <= 3; ++i) { set(button(), "Left", 20 + i*5); tab->sde->changeScheme(); drain(); }
        tab->sde->endOperation();
        CHECK(command(CMD_UNDO));
        CHECK(button()->props[0]->readInt() == 20 && button()->ctl && WidgetEditorEx::getActive(tab->sde));
        CHECK(command(CMD_REDO)); CHECK(button()->props[0]->readInt() == 35 && button()->ctl);
        for(int i = 0; i < 3; ++i) {
            CHECK(command(CMD_FORMEDIT)); CHECK(!WidgetEditorEx::getActive(tab->sde) && !button()->ctl);
            CHECK(command(CMD_FORMEDIT)); CHECK(WidgetEditorEx::getActive(tab->sde) && button()->ctl);
        }
        tab->sde->sdk->selMan->select(button());
        CHECK(command(CMD_DELETE)); CHECK(!button());
        CHECK(command(CMD_UNDO)); CHECK(button() && button()->ctl);
        CHECK(command(CMD_FORMEDIT)); CHECK(!button()->ctl);
        managerWindow.hide();
#ifdef HIASM_GTK4
        managerWindow.unset_child();
        app->remove_window(managerWindow);
#else
        managerWindow.remove();
#endif
    }
    // "C++ application": the MainForm project opens the built-in designer (no formedit program exists for this
    // package). Its controls are widgets of their [Edit] classes, and the form is a window with a title bar.
    const ustring cppFile = dir + "/cpp_form.sha";
    CHECK(g_file_set_contents(cppFile.c_str(),
        "Make(cpp)\nAdd(MainForm,1,0,0)\n{\n Caption=\"Test form\"\n}\nAdd(Button,2,20,30)\n{\n Caption=\"Push\"\n Left=20\n Top=30\n Width=80\n Height=30\n}\n"
        "Add(Label,3,20,90)\n{\n Caption=\"Text\"\n Left=20\n Top=90\n}\nAdd(Edit,4,20,130)\n{\n Left=20\n Top=130\n}\n", -1, nullptr));
    {
        WindowManager manager;
        Gtk::Window managerWindow;
#ifdef HIASM_GTK4
        app->add_window(managerWindow);
        managerWindow.set_child(manager);
#else
        managerWindow.add(manager);
#endif
        auto tab = dynamic_cast<WM_SDK_Editor*>(manager.open(cppFile)); CHECK(tab);
        compat_show_all(managerWindow); managerWindow.present(); drain();
        CHECK(tab->sde->sdk->pack && tab->sde->sdk->pack->name == "cpp" && tab->sde->sdk->elements.size() == 4);
        CHECK(cmdMan->isEnabled(CMD_FORMEDIT));
        CHECK(tab->processCommand(CMD_FORMEDIT, ""));
        CHECK(WidgetEditorEx::getActive(tab->sde) && !FE_Legacy::getActive(tab->sde));
        auto control = [&](int id) { return dynamic_cast<ElementTplWinControl*>(tab->sde->sdk->getElementById(id)); };
        CHECK(control(2) && control(2)->ctl && control(3)->ctl && control(4)->ctl);
        CHECK(control(1)->ctl);   // the form's own surface
#ifdef HIASM_GTK4
        // The controls are the widgets of their [Edit] classes, with their captions.
        auto button = dynamic_cast<Gtk::Button*>(control(2)->ctl);
        CHECK(button && button->get_label() == "Push");
        auto label = dynamic_cast<Gtk::Label*>(control(3)->ctl);
        CHECK(label && label->get_text() == "Text");
        CHECK(dynamic_cast<Gtk::Entry*>(control(4)->ctl));
        // The form is a window: a title bar with the Caption above the client area.
        Gtk::Widget *window = control(1)->ctl->get_parent();
        CHECK(window && window->has_css_class("hiasm-form-frame"));
        Gtk::Widget *title = window->get_first_child();
        CHECK(title && title->has_css_class("hiasm-form-title"));
        bool captioned = false;
        for(Gtk::Widget *child = title->get_first_child(); child; child = child->get_next_sibling())
            if(auto text = dynamic_cast<Gtk::Label*>(child)) captioned = text->get_text() == "Test form";
        CHECK(captioned);
#endif
        // The default colour of a C++ control is its Delphi name (clBtnFace from WinControl), not black.
        ElementProperty *color = tab->sde->sdk->getElementById(3)->props.getByName("Color");
        CHECK(color && static_cast<TypeColor*>(color->value)->toDelphi() != 0);
        // A control added while the form is open is a new element of the scheme.
        Element *added = tab->sde->sdk->add("CheckBox", 200, 60); drain();
        CHECK(added && tab->sde->sdk->elements.size() == 5);
        CHECK(dynamic_cast<ElementTplWinControl*>(added)->ctl);
        CHECK(tab->processCommand(CMD_FORMEDIT, "")); CHECK(!WidgetEditorEx::getActive(tab->sde) && !control(2)->ctl);
        // "Separate window" mode (HiAsm 4 form window) also opens a C++ scheme: the form is the element with [Edit] Class=Form.
        {
            FE_Legacy separate;
            separate.start(tab->sde);
            CHECK(separate.isActive());
            separate.stop();
        }
        managerWindow.hide();
#ifdef HIASM_GTK4
        managerWindow.unset_child();
        app->remove_window(managerWindow);
#else
        managerWindow.remove();
#endif
    }
    g_unlink(cppFile.c_str());
    g_unlink(file.c_str()); g_rmdir(dir.c_str());
    g_unlink((homeDir + "appearance.ini").c_str()); g_rmdir(homeDir.c_str());
    std::cout << "OK form editor: hit test, move, shift-select, union resize, freeze, insert/delete, cleanup" << std::endl;
}
