// The Project manager ("Создание проекта") lists the packs and their project types with a horizontal splitter
// between the two lists. The test opens the dialog, checks the splitter and writes a screenshot to the build folder.
#include "ProjectManager.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "Pack.h"
#include "gtkcompat.h"
#include <glib/gstdio.h>
#include <iostream>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)

static ProjectManager *dialog = nullptr;
static int splitterFound = -1;
static std::string shots;

#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
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
static void snapshot(Gtk::Window &window, const char *name) {
    auto surface = window.get_surface();
    if(!surface || shots.empty()) return;
    HWND hwnd = static_cast<HWND>(gdk_win32_surface_get_handle(surface->gobj()));
    std::cout << "SHOT " << name << "=" << capture(hwnd, (shots + "/" + name).c_str()) << std::endl;
}
#else
static void snapshot(Gtk::Window &, const char *) {}
#endif

// Runs inside the dialog's modal loop: finds the splitter between the lists, takes a picture and closes the dialog.
static int listRows(Gtk::Widget *widget) {
    int rows = 0;
    if(auto view = dynamic_cast<Gtk::IconView*>(widget)) {
        auto model = view->get_model();
        if(model) rows += (int)model->children().size();
    }
    for(Gtk::Widget *child = widget->get_first_child(); child; child = child->get_next_sibling())
        rows += listRows(child);
    return rows;
}

static Gtk::Widget *panedChild2(Gtk::Paned &paned) {
#ifdef HIASM_GTK4
    return paned.get_end_child();
#else
    return paned.get_child2();
#endif
}

static Gtk::Paned *gSplit = nullptr;
static int smallSplit = 0, largeSplit = 0;

// Second stage: the dialog was made taller, so the splitter and both lists must have grown with it.
static gboolean measure(gpointer) {
    if(gSplit) largeSplit = gSplit->get_height();
    snapshot(*dialog, "project_manager.png");
    dialog->response(0);
    return FALSE;
}

static gboolean finish(gpointer) {
    Gtk::Widget *box = dialog->get_content_area()->get_first_child();
    Gtk::Paned *split = nullptr;
    for(Gtk::Widget *child = box ? box->get_first_child() : nullptr; child; child = child->get_next_sibling())
        if((split = dynamic_cast<Gtk::Paned*>(child))) break;
    splitterFound = split != nullptr;
    // The body must expand vertically, like it does horizontally: with a fixed height the lists keep their
    // requested size and gain nothing when the window grows.
    if(!box || !box->get_vexpand()) splitterFound = 0;
    gSplit = split;
    if(split) {
        smallSplit = split->get_height();
        // Both panes must show their lists: the dialog preselects the C++ pack, so the projects are filled in.
        if(listRows(paned_get_child1(*split)) <= 0) splitterFound = 0;
        if(listRows(panedChild2(*split)) <= 0) splitterFound = 0;
    }
    if(dialog->get_width() > 600) splitterFound = 0;
    // Grow the window and let the layout settle before taking the picture.
    dialog->set_size_request(400, 520);
    g_timeout_add(400, measure, nullptr);
    return FALSE;
}

int main(int argc, char **argv) {
    if(argc < 2) return 2;
    if(argc > 2) shots = argv[2];
    auto app = Gtk::Application::create("com.hiasm.project-manager-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
    initDirs(); dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    gchar *settingsFolder = g_dir_make_tmp("hiasm-project-manager-settings-XXXXXX", nullptr); CHECK(settingsFolder);
    homeDir = ustring(settingsFolder) + G_DIR_SEPARATOR_S; g_free(settingsFolder);
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    compilerSet = new CompilerCollection(); packSet = new PackCollection();

    ProjectManager manager;
    dialog = &manager;
    g_timeout_add(900, finish, nullptr);
    manager.open();
    dialog = nullptr;
    CHECK(splitterFound == 1);
    if(largeSplit < smallSplit + 60) {
        std::cerr << "the splitter did not grow: " << smallSplit << " -> " << largeSplit << std::endl;
        return 1;
    }
    std::cout << "OK project manager: horizontal splitter between the packs and the projects, heights "
              << smallSplit << " -> " << largeSplit << std::endl;
    return 0;
}
