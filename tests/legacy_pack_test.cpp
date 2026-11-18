#include "Pack.h"
#include "SDK.h"
#include "MainDataBase.h"
#include "LegacyPackCatalog.h"
#include "WindowManager.h"
#include "SettingsManager.h"
#include "FormEditor.h"
#include "commands.h"
#include <glib/gstdio.h>
#include <iostream>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)
int propToInteger(ElementProperty *prop);	// CGT entry point (CodeGen.cpp)
static void drain() {
    const auto end = g_get_monotonic_time() + 150000;
    while(g_get_monotonic_time() < end) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(1000); }
}
static std::string cp1251(const std::string &text) {
    gsize size = 0;
    gchar *bytes = g_convert(text.data(), text.size(), "WINDOWS-1251", "UTF-8", nullptr, &size, nullptr);
    std::string result = bytes ? std::string(bytes, size) : std::string();
    g_free(bytes); return result;
}
int main(int argc, char **argv) {
    if(argc < 2) return 2;
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
    g_setenv("GTK_CSD", "0", TRUE);	// native Win32 frames, as in hico.cpp
#endif
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.legacy-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs(); dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    databaseFile = dataDir + "int/hiasm.db"; mdb.open();
    compilerSet = new CompilerCollection();
    if(argc == 3) {
        g_setenv("HIASM4_ROOTS", argv[2], TRUE);
        PackCollection collection; packSet = &collection;
        Pack *delphi = collection.getPackByName("delphi"); CHECK(delphi && delphi->isLegacy());
        delphi->ensureLoaded(); CHECK(delphi->elements.size() > 700);
        std::vector<std::string> names;
        GDir *configs = g_dir_open((delphi->path()+"conf").c_str(), 0, nullptr); CHECK(configs);
        while(const gchar *name = g_dir_read_name(configs))
            if(g_str_has_suffix(name, ".ini")) names.emplace_back(name, strlen(name)-4);
        g_dir_close(configs);
        size_t valid = 0;
        for(const auto &name : names) if(delphi->getElementByName(name)) ++valid;
        std::cout << "Legacy Delphi configuration audit: " << valid << "/" << names.size() << std::endl;
        MSDK form; CHECK(form.loadFromFile(delphi->path()+"new/MainForm.sha"));
        CHECK(form.packProject && form.elements.size() == 1);
        // WinControl.ini: @Color=...|8|clBtnFace, @Font=...|15| (empty).
        auto &formProps = form.elements.front()->props;
        CHECK(propToInteger(formProps.getByName("Color")) == -16777201);
        const FontRecord &formFont = *(FontRecord*)formProps.getByName("Font")->value;
        CHECK(formFont.name == "MS Sans Serif" && formFont.size == 8 && formFont.charset == 1);
        return valid == names.size() ? 0 : 1;
    }
    gchar *tmp = g_dir_make_tmp("hiasm-legacy-pack-XXXXXX", nullptr); CHECK(tmp);
    const std::string root(tmp); g_free(tmp);
    const std::string directory = root + "/Elements/fixture";
    for(const auto &sub : {"conf", "new", "code"}) CHECK(g_mkdir_with_parents((directory+"/"+sub).c_str(), 0700) == 0);
    sqlite3 *db = nullptr; CHECK(sqlite3_open((directory+"/Elements.db").c_str(), &db) == SQLITE_OK);
    CHECK(sqlite3_exec(db, "CREATE TABLE groups(id INTEGER,name TEXT,pos INTEGER); CREATE TABLE elements(name TEXT,info TEXT,tab INTEGER,pos INTEGER);", nullptr, nullptr, nullptr) == SQLITE_OK);
    sqlite3_stmt *insert = nullptr;
    CHECK(sqlite3_prepare_v2(db, "INSERT INTO groups VALUES(1,?1,0)", -1, &insert, nullptr) == SQLITE_OK);
    const auto group = cp1251(u8"Компоненты");
    sqlite3_bind_text(insert, 1, group.data(), group.size(), SQLITE_TRANSIENT);
    CHECK(sqlite3_step(insert) == SQLITE_DONE); sqlite3_finalize(insert);
    CHECK(sqlite3_prepare_v2(db, "INSERT INTO elements VALUES('Entry',?1,1,0)", -1, &insert, nullptr) == SQLITE_OK);
    const auto info = cp1251(u8"Тестовая форма");
    sqlite3_bind_text(insert, 1, info.data(), info.size(), SQLITE_TRANSIENT);
    CHECK(sqlite3_step(insert) == SQLITE_DONE); sqlite3_finalize(insert); sqlite3_close(db);
    const auto write = [&](const std::string &name, const std::string &body) {
        const auto bytes = cp1251(body);
        return g_file_set_contents((directory+"/"+name).c_str(), bytes.data(), bytes.size(), nullptr);
    };
    CHECK(write("conf/Base.ini", u8"[Type]\nClass=Element\n[Property]\nText=caption|2|base\n@Color=Цвет|8|clBtnFace\nBack=Фон|8|clWindow\nHex=hex|8|$00FF00\nPlain=plain|8|16711680\nEmpty=empty|8|\n@Font=Шрифт|15|\nHint=hint|15| Tahoma,8,0,0,1\n[Methods]\nonStart=Text||Index|2|0\n"));
    CHECK(write("conf/Entry.ini", u8"[Type]\nClass=Element\nInherit=Base\nInfo=Тест\n[Property]\n+Text=||Привет\n[Methods]\nValue=Value|4|2|\n"));
    CHECK(write("new/Entry.sha", u8"Make(fixture)\nAdd(Entry,1,0,0)\n{\nText=Привет\nBack=-16777196\nHint=[Arial,10,1,255]\n}\n"));
    CHECK(write("code/hiEntry.pas", "unit hiEntry;"));
    // A form and a button, as MainForm.ini/Button.ini of the Delphi package.
    CHECK(write("conf/WinBase.ini", "[Type]\nClass=WinElement\n[Property]\n@Left=left|1|0\n@Top=top|1|0\n@Width=width|1|400\n@Height=height|1|300\n"
        "@Align=align|14|0|caNone,caLeft,caTop,caRight,caBottom,caClient\n@Color=color|8|clBtnFace\n@Font=font|15|\n"
        "ParentFont=parent font|14|1|True,False\n@Enabled=enabled|14|0|True,False\n"));
    CHECK(write("conf/Form.ini", "[Type]\nClass=WinElement\nInherit=WinBase\n[Edit]\nClass=Form\nCaption=Caption\nColor=Color\nBorderStyle=BorderStyle\nClientSize=ClientSize\n"
        "[Property]\nCaption=caption|2|Form\n@BorderStyle=border|4|2|bsNone,bsSingle,bsSizeable,bsDialog,bsToolWindow,bsSizeToolWin,bsBox,bsSizeBox,bsBoxEdge,bsSizeBoxEdge\n"
        "ClientSize=client size|14|1|True,False\n"));
    CHECK(write("conf/Button.ini", "[Type]\nClass=WinElement\nInherit=WinBase\n[Edit]\nClass=Button\nCaption=Caption\nFont=Font\n[Property]\nCaption=caption|2|\n"));
    const gchar *previous = g_getenv("HIASM4_ROOTS"); const bool had = previous != nullptr;
    const std::string saved = previous ? previous : "";
    g_setenv("HIASM4_ROOTS", root.c_str(), TRUE);
    PackCollection collection; packSet = &collection;
    Pack *pack = collection.getPackByName("fixture"); CHECK(pack && pack->isLegacy());
    pack->ensureLoaded();
    CHECK(pack->groups.size() == 1 && pack->groups.front()->name.raw() == std::string(u8"Компоненты"));
    CHECK(pack->elements.size() == 1 && pack->elements.front()->info.raw() == std::string(u8"Тестовая форма"));
    auto element = pack->getElementByName("Entry"); CHECK(element && element->conf->valid);
    CHECK(element->conf->findPropByName("Text")->value.raw() == std::string(u8"Привет"));
    CHECK(element->conf->mtd.front()->info == "Text|Index");
    const auto sourcePath = pack->pathElementCode("Entry");
    if(!g_str_has_suffix(sourcePath.c_str(), "hiEntry.pas")) std::cerr << "Source path: " << sourcePath << std::endl;
    CHECK(g_str_has_suffix(sourcePath.c_str(), "hiEntry.pas"));
    CHECK(pack->projects.size() == 1 && pack->projects.front()->entry == "Entry");
    MSDK scheme; CHECK(scheme.loadFromFile(pack->projects.front()->getTemplateFile()));
    CHECK(scheme.pack == pack && scheme.packProject && scheme.elements.size() == 1);
    CHECK(scheme.elements.front()->props.getByName("Text")->readStr().raw() == std::string(u8"Привет"));
    {
        // CGT receives the Delphi TColor HiAsm 4 passed (clBtnFace = $FF00000F, not 0).
        auto &props = scheme.elements.front()->props;
        CHECK(propToInteger(props.getByName("Color")) == -16777201 && props.getByName("Color")->isDefault());
        CHECK(propToInteger(props.getByName("Back")) == -16777196 && !props.getByName("Back")->isDefault());
        CHECK(propToInteger(props.getByName("Hex")) == 0x00FF00);
        CHECK(propToInteger(props.getByName("Plain")) == 0xFF0000);
        CHECK(propToInteger(props.getByName("Empty")) == 0);
        CHECK(props.getByName("Back")->serialize() == "-16777196");
        const FontRecord &font = *(FontRecord*)props.getByName("Font")->value;
        CHECK(font.name == "MS Sans Serif" && font.size == 8 && font.style == 0 && font.color == 0 && font.charset == 1);
        CHECK(props.getByName("Font")->isDefault());
        const FontRecord &hint = *(FontRecord*)props.getByName("Hint")->value;
        CHECK(hint.name == "Arial" && hint.size == 10 && hint.style == 1 && hint.color == 255 && hint.charset == 0);
        CHECK(!props.getByName("Hint")->isDefault());
    }
    SettingsManager::init();
    cmdMan = new CommandManager();
    {
        WindowManager manager;
        auto viewer = dynamic_cast<WM_Text_Editor*>(manager.open(directory+"/conf/Entry.ini"));
        CHECK(viewer && !viewer->memo->get_editable());
        CHECK(viewer->memo->get_buffer()->get_text().raw().find(u8"Привет") != std::string::npos);
        CHECK(!viewer->saveToFile(directory+"/conf/Entry.ini"));
        CHECK(viewer->saveToFile(root+"/copy.ini"));
        gchar *copy = nullptr; gsize length = 0;
        CHECK(g_file_get_contents((root+"/copy.ini").c_str(), &copy, &length, nullptr));
        CHECK(g_utf8_validate(copy, length, nullptr)); g_free(copy);
        g_unlink((root+"/copy.ini").c_str());
    }
    {
        // HiAsm 4 has no make/formedit.exe: the form designer of the IDE edits Class=Form.
        const std::string file = root + "/form.sha";
        CHECK(g_file_set_contents(file.c_str(), "Make(fixture)\nAdd(Form,1,21,105)\n{\nLeft=50\nTop=60\nWidth=320\nHeight=240\n"
            "Caption=Designer\nClientSize=0\n}\nAdd(Button,2,100,105)\n{\nLeft=15\nTop=10\nWidth=80\nHeight=25\nCaption=OK\n}\n", -1, nullptr));
        WindowManager manager;
        Gtk::Window host;
#ifdef HIASM_GTK4
        app->add_window(host);
        host.set_child(manager);
#else
        host.add(manager);
#endif
        auto tab = dynamic_cast<WM_SDK_Editor*>(manager.open(file)); CHECK(tab);
        compat_show_all(host); host.present(); drain();
        manager.update();
        CHECK(cmdMan->isEnabled(CMD_FORMEDIT));
        CHECK(tab->processCommand(CMD_FORMEDIT, "")); drain();
        FE_Legacy *designer = FE_Legacy::getActive(tab->sde);
        CHECK(designer && designer->isActive() && designer->formElement());
        CHECK(designer->formElement()->tpl->name == "Form");
        CHECK(designer->windowTitle() == "Designer");
        Element *form = designer->formElement(), *button = tab->sde->sdk->getElementById(2);
        CHECK(button && button->tpl->name == "Button");
        auto setForm = [&](const char *name, int value) { form->props.getByName(name)->setValue(&value); drain(); };
        auto intProp = [](Element *element, const char *name) { return element->props.getByName(name)->readInt(); };
        (void)setForm;
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
        {
            // The frame follows BorderStyle exactly as THIMainForm.SetBorderStyle sets it.
            HWND hwnd = static_cast<HWND>(designer->nativeWindow());
            CHECK(hwnd && IsWindowVisible(hwnd));
            // Owned by the IDE window, like the HiAsm 4 designer.
            CHECK(GetWindow(hwnd, GW_OWNER) == static_cast<HWND>(gdk_win32_surface_get_handle(host.get_surface()->gobj())));
            const UINT dpi = GetDpiForWindow(hwnd);
            auto style = [&]() { return static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_STYLE)); };
            auto exStyle = [&]() { return static_cast<DWORD>(GetWindowLongPtrW(hwnd, GWL_EXSTYLE)); };
            auto clientRect = [&]() { RECT r; GetClientRect(hwnd, &r); return r; };
            auto windowRect = [&]() { RECT r; GetWindowRect(hwnd, &r); return r; };
            RECT client = clientRect();
            if(client.right != MulDiv(320, dpi, 96) || client.bottom != MulDiv(240, dpi, 96))
                std::cerr << "client " << client.right << "x" << client.bottom << " dpi " << dpi << std::endl;
            CHECK(client.right == MulDiv(320, dpi, 96) && client.bottom == MulDiv(240, dpi, 96));	// ClientSize=True
            const DWORD sizeable = WS_CAPTION | WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX;
            CHECK((style() & sizeable) == sizeable);	// bsSizeable (default)
            CHECK(windowRect().left == MulDiv(50, dpi, 96) && windowRect().top == MulDiv(60, dpi, 96));
            // Every BorderStyle: Style/ExStyle of the NmSet/ExSet tables in hiMainForm.pas.
            const struct { DWORD style, exStyle; } kol[10] = {
                {0, 0}, {WS_CAPTION | WS_MINIMIZEBOX, 0}, {sizeable, 0}, {WS_CAPTION, 0}, {WS_CAPTION, WS_EX_TOOLWINDOW},
                {WS_CAPTION | WS_THICKFRAME, WS_EX_TOOLWINDOW}, {0, WS_EX_DLGMODALFRAME}, {WS_THICKFRAME, WS_EX_TOOLWINDOW},
                {0, WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME}, {WS_THICKFRAME, WS_EX_CLIENTEDGE},
            };
            const DWORD exBits = WS_EX_TOOLWINDOW | WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE;
            for(int border = 0; border < 10; ++border) {
                setForm("BorderStyle", border);
                if((style() & sizeable) != kol[border].style || (exStyle() & exBits) != kol[border].exStyle)
                    std::cerr << std::hex << "BorderStyle " << border << ": style " << style() << " ex " << exStyle() << std::dec << std::endl;
                CHECK((style() & sizeable) == kol[border].style && (exStyle() & exBits) == kol[border].exStyle);
                CHECK(IsWindowVisible(hwnd));
            }
            setForm("BorderStyle", 2);
            CHECK((style() & sizeable) == sizeable);
            client = clientRect();
            CHECK(client.right == MulDiv(320, dpi, 96) && client.bottom == MulDiv(240, dpi, 96));
            setForm("ClientSize", 1);	// Width/Height become the outer size
            if(dpi == 96) {
                const RECT outer = windowRect();
                CHECK(outer.right - outer.left == 320 && outer.bottom - outer.top == 240);
            }
            // Moving or resizing the window by the user changes the form.
            const RECT before = windowRect();
            SetWindowPos(hwnd, nullptr, MulDiv(70, dpi, 96), MulDiv(80, dpi, 96),
                before.right - before.left + MulDiv(40, dpi, 96), before.bottom - before.top, SWP_NOZORDER | SWP_NOACTIVATE);
            SendMessageW(hwnd, WM_ENTERSIZEMOVE, 0, 0);
            SendMessageW(hwnd, WM_EXITSIZEMOVE, 0, 0);
            drain();
            if(intProp(form, "Left") != 70 || intProp(form, "Width") != 360)
                std::cerr << "synced " << intProp(form, "Left") << "," << intProp(form, "Top") << " " << intProp(form, "Width") << "x" << intProp(form, "Height") << std::endl;
            CHECK(intProp(form, "Left") == 70 && intProp(form, "Top") == 80);
            if(dpi == 96) CHECK(intProp(form, "Width") == 360 && intProp(form, "Height") == 240);
            CHECK(tab->changed);
        }
#endif
        {
            // Select, move and resize the button with the mouse (5 pixel steps).
            const double z = designer->scale();
            auto press = [&](int x, int y) { GdkEventButton e = {}; e.button = BTN_LEFT; e.x = x*z; e.y = y*z; designer->on_button_press_event(&e); };
            auto move = [&](int x, int y) { GdkEventMotion e = {}; e.x = x*z; e.y = y*z; designer->on_motion_notify_event(&e); };
            auto release = [&](int x, int y) { GdkEventButton e = {}; e.button = BTN_LEFT; e.x = x*z; e.y = y*z; designer->on_button_release_event(&e); };
            press(20, 15); move(43, 37); release(43, 37); drain();
            CHECK(tab->sde->sdk->selMan->getFirst() == button);
            CHECK(intProp(button, "Left") == 35 && intProp(button, "Top") == 30);
            press(115, 55); move(125, 65); release(125, 65); drain();	// bottom-right grip
            CHECK(intProp(button, "Width") == 90 && intProp(button, "Height") == 35);
            CHECK(intProp(button, "Left") == 35 && intProp(button, "Top") == 30);
            press(200, 150); release(200, 150); drain();
            CHECK(tab->sde->sdk->selMan->elements.empty());
            press(5, 5); move(60, 50); release(60, 50); drain();	// rubber band
            CHECK(tab->sde->sdk->selMan->getFirst() == button);
            tab->sde->sdk->selMan->clear(); drain();
            if(z == 1) {
                // Raised button bevel (clBtnHighlight) on the clBtnFace form.
#ifdef HIASM_GTK4
                auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, 320, 240);
#else
                auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 320, 240);
#endif
                designer->on_draw(Cairo::Context::create(surface));
                surface->flush();
                auto pixel = [&](int x, int y) { return *reinterpret_cast<const guint32*>(surface->get_data() + y*surface->get_stride() + x*4) & 0xFFFFFF; };
                if(pixel(36, 30) != 0xFFFFFF || pixel(75, 95) != 0xF0F0F0)
                    std::cerr << std::hex << "pixels " << pixel(36, 30) << " " << pixel(75, 95) << std::dec << std::endl;
                CHECK(pixel(36, 30) == 0xFFFFFF && pixel(75, 95) == 0xF0F0F0);
            }
        }
        // The close button of the form ends the editing like the toolbar button.
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
        SendMessageW(static_cast<HWND>(designer->nativeWindow()), WM_CLOSE, 0, 0);
#elif defined(HIASM_GTK4)
        designer->formWindow()->close();
#else
        gtk_window_close(designer->formWindow()->gobj());
#endif
        drain();
        CHECK(!FE_Legacy::getActive(tab->sde));
        manager.update(); CHECK(cmdMan->isEnabled(CMD_FORMEDIT));
        CHECK(tab->processCommand(CMD_FORMEDIT, "")); drain();
        CHECK(FE_Legacy::getActive(tab->sde));
        // Undo replaces the scheme; the designer is reopened on the new form.
        CHECK(tab->processCommand(CMD_UNDO, "")); drain();
        CHECK(FE_Legacy::getActive(tab->sde) && FE_Legacy::getActive(tab->sde)->formElement());
        button = tab->sde->sdk->getElementById(2); CHECK(button);
        CHECK(intProp(button, "Width") == 80 && intProp(button, "Height") == 25);	// before the grip resize
        CHECK(tab->processCommand(CMD_FORMEDIT, "")); drain();
        CHECK(!FE_Legacy::getActive(tab->sde));
        host.set_visible(false);
#ifdef HIASM_GTK4
        host.unset_child();
        app->remove_window(host);
#else
        host.remove();
#endif
        g_unlink(file.c_str());
    }
    CHECK(collection.isLegacyFile(directory+"/conf/Entry.ini"));
    CHECK(!collection.isLegacyFile(directory+"-other/conf/Entry.ini"));
    CHECK(!scheme.saveToFile(directory+"/new/Entry.sha"));
    ustring error; CHECK(!pack->createElement("New", u8"Компоненты", "test", error) && !error.empty());
    CHECK(!pack->addElement(u8"Компоненты", "New", "test"));
    CHECK(!pack->addGroup(u8"Компоненты", "test"));
    CHECK(!pack->projects.front()->make(root+"/ignored", nullptr));
    if(had) g_setenv("HIASM4_ROOTS", saved.c_str(), TRUE); else g_unsetenv("HIASM4_ROOTS");
    for(const auto &file : {"conf/Base.ini", "conf/Entry.ini", "conf/WinBase.ini", "conf/Form.ini", "conf/Button.ini",
            "new/Entry.sha", "code/hiEntry.pas", "Elements.db"}) g_unlink((directory+"/"+file).c_str());
    for(const auto &sub : {"conf", "new", "code"}) g_rmdir((directory+"/"+sub).c_str());
    g_rmdir(directory.c_str()); g_rmdir((root+"/Elements").c_str()); g_rmdir(root.c_str());
    std::cout << "OK external legacy pack: CP1251 DB/config/schema, inheritance, templates, read-only, ABI guard" << std::endl;
}
