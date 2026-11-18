// Nested schemes in the scheme editor. A double click on a container (Core hcMultiElement, "Nested scheme")
// enters its scheme; a wire dropped on the container's frame (its editor element) exposes a point to the parent
// scheme; the back command returns to the parent. Screenshots go to the build folder on Windows.
#include "SDK_Editor.h"
#include "Elements.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "WindowManager.h"
#include "commands.h"
#include <glib/gstdio.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <vector>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)

static void drain() {
    const auto end = g_get_monotonic_time() + 150000;
    while(g_get_monotonic_time() < end) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(1000); }
}

static int childCount(Gtk::Widget *widget) {
    int count = 0;
    for(Gtk::Widget *child = widget->get_first_child(); child; child = child->get_next_sibling()) count++;
    return count;
}

#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
// Client area of a native window as a PNG (PrintWindow draws it into a bitmap).
static bool capture(HWND hwnd, const char *path) {
    RECT rc = {};
    GetClientRect(hwnd, &rc);
    const int w = rc.right, h = rc.bottom;
    if(w <= 0 || h <= 0) return false;
    HDC screen = GetDC(nullptr);
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, w, h);
    HGDIOBJ previous = SelectObject(memory, bitmap);
    PrintWindow(hwnd, memory, PW_CLIENTONLY | PW_RENDERFULLCONTENT);
    BITMAPINFO info = {};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = w;
    info.bmiHeader.biHeight = -h;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    std::vector<guchar> pixels(static_cast<size_t>(w) * h * 4);
    GetDIBits(memory, bitmap, 0, h, pixels.data(), &info, DIB_RGB_COLORS);
    for(size_t i = 0; i < pixels.size(); i += 4) { std::swap(pixels[i], pixels[i + 2]); pixels[i + 3] = 255; }
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_data(pixels.data(), GDK_COLORSPACE_RGB, TRUE, 8, w, h, w * 4, nullptr, nullptr);
    const gboolean saved = gdk_pixbuf_save(pixbuf, path, "png", nullptr, nullptr);
    g_object_unref(pixbuf);
    SelectObject(memory, previous);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);
    return saved;
}
static void *native_handle(Gtk::Window &window) {
    auto surface = window.get_surface();
    return surface ? gdk_win32_surface_get_handle(surface->gobj()) : nullptr;
}
#define SHOT(name) do { if(!shots.empty()) std::cout << "SHOT " << name << "=" << capture(static_cast<HWND>(native_handle(window)), (shots + "/" + name).c_str()) << std::endl; } while(0)
#else
#define SHOT(name) do {} while(0)
#endif

int main(int argc, char **argv) {
    if(argc != 2) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.nested-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs(); dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    const std::string shots = argv[1];
    gchar *settingsFolder = g_dir_make_tmp("hiasm-nested-settings-XXXXXX", nullptr); CHECK(settingsFolder);
    homeDir = ustring(settingsFolder) + G_DIR_SEPARATOR_S; g_free(settingsFolder);
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    cmdMan = new CommandManager();
    compilerSet = new CompilerCollection(); packSet = new PackCollection();
    // The application loads the cursor list before any editor is shown.
    if(!curList) curList = new CursorList(Gdk::Display::get_default());
    auto toolbar = std::unique_ptr<Toolbar>(cmdMan->createToolbar(MENUNAME_TOOLBAR));
    gchar *dirTemp = g_dir_make_tmp("hiasm-nested-XXXXXX", nullptr); CHECK(dirTemp);
    const ustring dir(dirTemp); g_free(dirTemp);
    const ustring file = dir + "/nested.sha";
    CHECK(g_file_set_contents(file.c_str(),
        "Make(_base)\nAdd(hcTplFixed,1,0,0)\n{\n}\nAdd(hcMultiElement,2,40,60)\n{\n}\n", -1, nullptr));

    WindowManager manager;
    Gtk::Window window;
#ifdef HIASM_GTK4
    app->add_window(window);
    window.set_child(manager);
#else
    window.add(manager);
#endif
    auto tab = dynamic_cast<WM_SDK_Editor*>(manager.open(file)); CHECK(tab);
    window.set_default_size(900, 600);
    compat_show_all(window); window.present(); drain();
    SDK_Editor *editor = tab->sde;
    CHECK(editor->sdk->elements.size() == 2);
    Element *container = editor->sdk->getElementById(2);
    CHECK(container && container->sdk && container->points.size() == 0);
    CHECK(editor->csdk->getPath().raw() == "ROOT\\");
    // A point of the scheme where nothing is: no entering.
    CHECK(!editor->requestEnterAt(400, 500));
    SHOT("root.png");

    // The GTK gesture of the editor reports the presses of a click. One press only selects the container.
    auto gesture = compat_click_controller(*editor);
    const double px = container->x + 12.0, py = container->y + 12.0;
    g_signal_emit_by_name(gesture->gobj(), "pressed", 1, px, py); drain();
    CHECK(editor->csdk == editor->sdk);
    // The second press of a double click enters the scheme; the path shows it, as in HiAsm 4 (ROOT\name\).
    // A synthetic press has no current mouse button, so the entering is requested directly here. A real double
    // click reaches the same request through the gesture: its second press has two presses and button 1.
    CHECK(editor->requestEnterAt(px, py)); drain();
    CHECK(editor->csdk == container->sdk);
    CHECK(editor->csdk->getPath().raw() == "ROOT\\hcMultiElement\\");
    Element *frame = nullptr;
    for(Element *e : editor->csdk->elements) if(e->tpl->name == "hcEditMulti") frame = e;
    CHECK(frame);
    SHOT("entered.png");

    // Elements inside: a double click on an ordinary element does not enter it.
    Element *inner = editor->csdk->add("hcTplButton", 40, 40); drain();
    CHECK(inner && !inner->sdk);
    CHECK(!editor->requestEnterAt(inner->x + 5, inner->y + 5));
    CHECK(editor->csdk == container->sdk);
    SHOT("entered_with_button.png");

    // A wire dropped on the frame exposes the point to the parent scheme (SDK_Editor::pointLink_UpProc).
    ElementPoint *source = inner->points.getByName("onClick");
    CHECK(source);
    const size_t framePoints = frame->points.size();
    frame->connectToPoint(source);
    CHECK(frame->points.size() == framePoints + 1 && source->point);
    drain();
    SHOT("connected_to_frame.png");

    // The back command returns to the parent scheme; the container shows the exposed point there.
    CHECK(tab->processCommand(CMD_BACK, "")); drain();
    CHECK(editor->csdk == editor->sdk);
    CHECK(editor->csdk->getPath().raw() == "ROOT\\");
    CHECK(container->points.size() == 1);
    SHOT("left.png");

    // The forward command enters the selected container again.
    editor->csdk->selMan->clear(); editor->csdk->selMan->add(container);
    CHECK(tab->processCommand(CMD_FORWARD, "")); drain();
    CHECK(editor->csdk == container->sdk);
    CHECK(tab->processCommand(CMD_BACK, "")); drain();
    CHECK(editor->csdk == editor->sdk);

    // The path line names the schemes from ROOT down to the shown one; leaving goes up to any of them.
    CHECK(tab->sdkPathText().raw() == "ROOT\\");
    CHECK(editor->requestEnterAt(px, py)); drain();
    Element *deeper = editor->csdk->add("hcMultiElement", 40, 40); drain();
    CHECK(deeper && deeper->sdk);
    CHECK(editor->requestEnterAt(deeper->x + 5, deeper->y + 5)); drain();
    CHECK(editor->csdk == deeper->sdk);
    CHECK(tab->sdkPathText().raw() == "ROOT\\hcMultiElement\\hcMultiElement\\");
    // The path line leads to the middle level, then to ROOT; each step redraws the scheme it shows.
    editor->leaveSDKTo(container->sdk); drain();
    CHECK(editor->csdk == container->sdk);
    CHECK(tab->sdkPathText().raw() == "ROOT\\hcMultiElement\\");
    editor->leaveSDKTo(editor->sdk); drain();
    CHECK(editor->csdk == editor->sdk);
    CHECK(tab->sdkPathText().raw() == "ROOT\\");

    // The C++ package: its palette "Panel" is the container ChildPanel, as in HiAsm 4; a double click enters it.
    const ustring cppFile = dir + "/child_panel.sha";
    CHECK(g_file_set_contents(cppFile.c_str(), "Make(cpp)\nAdd(MainForm,1,21,105)\n{\n}\nAdd(ChildPanel,2,140,105)\n{\n}\nAdd(ChildForm,3,300,105)\n{\n}\n", -1, nullptr));
    auto cppTab = dynamic_cast<WM_SDK_Editor*>(manager.open(cppFile)); CHECK(cppTab); drain();
    SDK_Editor *cppEditor = cppTab->sde;
    Element *childPanel = cppEditor->sdk->getElementById(2);
    CHECK(childPanel && childPanel->sdk && childPanel->getParentElement());
    CHECK(childPanel->getParentElement()->tpl->name == "Panel");
    // The form editor of the root scheme, where the user adds the container (MainForm is the window).
    CHECK(cppTab->processCommand(CMD_FORMEDIT, "")); drain();
    SHOT("root_containers.png");
    CHECK(cppTab->processCommand(CMD_FORMEDIT, "")); drain();
    CHECK(cppEditor->requestEnterAt(childPanel->x + 5, childPanel->y + 5)); drain();
    CHECK(cppEditor->csdk == childPanel->sdk);
    CHECK(cppTab->sdkPathText().raw() == "ROOT\\ChildPanel\\");
    // The form editor of the nested scheme (the Panel is its parent element): a field of the panel's Width x Height,
    // and an element added while the editor is open is placed on that field.
    CHECK(cppTab->processCommand(CMD_FORMEDIT, "")); drain();
    ElementTplWinControl *panel = dynamic_cast<ElementTplWinControl*>(childPanel->getParentElement());
    CHECK(panel && panel->ctl);
    Gtk::Frame *panelFrame = dynamic_cast<Gtk::Frame*>(panel->ctl);
    CHECK(panelFrame && panelFrame->get_child());
    // The frame of the panel is its Width x Height, as in the parent form; the field fills it.
    int frameWidth = 0, frameHeight = 0;
    panelFrame->get_size_request(frameWidth, frameHeight);
    CHECK(frameWidth == panel->props[2]->readInt() && frameHeight == panel->props[3]->readInt());
    CHECK(panelFrame->get_child()->get_hexpand() && panelFrame->get_child()->get_vexpand());
    SHOT("panel_empty.png");
    Element *panelButton = cppEditor->csdk->add("Button", 10, 10); drain();
    ElementTplWinControl *button = dynamic_cast<ElementTplWinControl*>(panelButton);
    CHECK(button && button->ctl && button->ctl->get_parent() == panelFrame->get_child());
    // A child window (ChildForm) inside the panel's scheme is not a control of the panel: no frame for it.
    const int panelFieldChildren = childCount(panelFrame->get_child());
    Element *panelChildForm = cppEditor->csdk->add("ChildForm", 10, 40); drain();
    CHECK(panelChildForm && panelChildForm->sdk);
    CHECK(childCount(panelFrame->get_child()) == panelFieldChildren);
    SHOT("panel_with_button.png");
    CHECK(cppTab->processCommand(CMD_FORMEDIT, "")); drain();
    cppEditor->leaveSDKTo(cppEditor->sdk); drain();
    CHECK(cppTab->sdkPathText().raw() == "ROOT\\");
    // A child form: its window is the main form of its scheme; the form editor works inside it too.
    Element *childForm = cppEditor->sdk->getElementById(3);
    CHECK(childForm && childForm->sdk && childForm->getParentElement() && childForm->getParentElement()->tpl->name == "MainForm");
    CHECK(cppEditor->requestEnterAt(childForm->x + 5, childForm->y + 5)); drain();
    CHECK(cppEditor->csdk == childForm->sdk);
    CHECK(cppTab->sdkPathText().raw() == "ROOT\\ChildForm\\");
    CHECK(cppTab->processCommand(CMD_FORMEDIT, "")); drain();
    // An element added to the child form while its editor is open is placed on the form's client area.
    ElementTplWinControl *childWindow = dynamic_cast<ElementTplWinControl*>(childForm->getParentElement());
    CHECK(childWindow && childWindow->ctl);
    const int clientChildren = childCount(childWindow->ctl);
    ElementTplWinControl *formButton = dynamic_cast<ElementTplWinControl*>(cppEditor->csdk->add("Button", 10, 10));
    drain();
    CHECK(formButton && formButton->ctl && formButton->ctl->get_parent() != nullptr);
    CHECK(childCount(childWindow->ctl) == clientChildren + 1);
    // A child window added inside the child form is not a control of it either.
    Element *nestedChildForm = cppEditor->csdk->add("ChildForm", 40, 60); drain();
    CHECK(nestedChildForm && nestedChildForm->sdk);
    CHECK(childCount(childWindow->ctl) == clientChildren + 1);
    CHECK(cppTab->processCommand(CMD_FORMEDIT, "")); drain();
    cppEditor->leaveSDKTo(cppEditor->sdk); drain();
    CHECK(cppTab->sdkPathText().raw() == "ROOT\\");

    // Containers added while the form editor of the root scheme is open (a palette insert, see addElement).
    {
        const ustring addFile = dir + "/add_containers.sha";
        CHECK(g_file_set_contents(addFile.c_str(), "Make(cpp)\nAdd(MainForm,1,21,105)\n{\n}\n", -1, nullptr));
        auto addTab = dynamic_cast<WM_SDK_Editor*>(manager.open(addFile)); CHECK(addTab); drain();
        SDK_Editor *addEditor = addTab->sde;
        CHECK(addTab->processCommand(CMD_FORMEDIT, "")); drain();
        Element *mainForm = addEditor->sdk->getElementById(1);
        ElementTplWinControl *mainControl = dynamic_cast<ElementTplWinControl*>(mainForm);
        CHECK(mainControl && mainControl->ctl);
        auto listChildren = [&](const char *title) {
            std::cout << title << std::endl;
            for(Gtk::Widget *c = mainControl->ctl->get_first_child(); c; c = c->get_next_sibling()) {
                std::cout << "  " << G_OBJECT_TYPE_NAME(c->gobj()) << " at " << c->get_allocation().get_x() << ","
                          << c->get_allocation().get_y() << " " << c->get_allocation().get_width() << "x"
                          << c->get_allocation().get_height() << std::endl;
            }
        };
        listChildren("BEFORE");
        addEditor->csdk->add("ChildPanel", 140, 105); drain();
        listChildren("AFTER PANEL");
        const int withPanel = childCount(mainControl->ctl);
        CHECK(withPanel == 1);
        addEditor->csdk->add("ChildForm", 300, 105); drain();
        listChildren("AFTER FORM");
        // A child window is not a control of the main form: the editor draws no window frame on it.
        CHECK(childCount(mainControl->ctl) == withPanel);
        SHOT("add_containers.png");
        CHECK(addTab->processCommand(CMD_FORMEDIT, "")); drain();
        g_unlink(addFile.c_str());
    }

    window.hide();
#ifdef HIASM_GTK4
    window.unset_child();
    app->remove_window(window);
#endif
    g_unlink(file.c_str()); g_unlink(cppFile.c_str()); g_rmdir(dir.c_str());
    std::cout << "OK nested scheme: double click enters, frame exposes a point, back and forward commands" << std::endl;
    return 0;
}
