#include "LayerObjects.h"
#include "SDK.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include <iostream>
#define CHECK(expr) do { if(!(expr)) { std::cerr << __LINE__ << ": " << #expr << std::endl; return 1; } } while(0)
static void drainFor(int milliseconds) {
    const auto end = g_get_monotonic_time() + milliseconds * 1000;
    while(g_get_monotonic_time() < end) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(1000); }
}
int main(int argc, char **argv) {
    if(argc != 2) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.hint-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs(); dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    databaseFile = dataDir + INT_PATH + DATABASE_FILE; mdb.open(); SettingsManager::init();
    compilerSet = new CompilerCollection(); packSet = new PackCollection();
    int criticals = 0;
    const auto handler = g_log_set_handler("GLib", G_LOG_LEVEL_CRITICAL,
        [](const gchar*, GLogLevelFlags, const gchar*, gpointer p) { ++*static_cast<int*>(p); }, &criticals);
    {
        LayerObject object(nullptr);
        CHECK(!object.visible && !object.dead && object.width == 0 && object.height == 0);
        object.show(0, 0, true); drainFor(300); object.show(0, 0, true);
        drainFor(300); CHECK(!object.visible); drainFor(250); CHECK(object.visible);
    }
    { LayerObject cancelled(nullptr); cancelled.show(0,0,true); }
    drainFor(550); CHECK(criticals == 0);
    g_log_remove_handler("GLib", handler);
    MSDK sdk; sdk.pack = packSet->getPackByName("_base");
    Element *e = sdk.add("hcDoData", 7, 14); CHECK(e);
    e->points[0]->name = u8"Действие 世界";
    e->points[0]->info = u8"Описание подсказки: длинная строка Unicode, которая должна переноситься на следующую строку без обрезания. 世界";
#ifdef HIASM_GTK4
    auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, 640, 480);
#else
    auto surface = Cairo::ImageSurface::create(Cairo::FORMAT_ARGB32, 640, 480);
#endif
    auto dc = Cairo::Context::create(surface);
    LO_PointHint point(e->points[0], 1, nullptr); point.draw(dc);
    CHECK(point.width > 100 && point.width <= 480 && point.height > ICON_SIZE + 10);
    const int w = point.width, h = point.height; dc->set_font_size(80); point.draw(dc);
    CHECK(point.width == w && point.height == h);
    LO_ElementHint element(e, 1, nullptr); element.show(20, 180); element.draw(dc);
    CHECK(element.width > 30 && element.height > 30);
    surface->write_to_png((dataDir + "layer-hints.png").raw());
    std::cout << "OK hint timers/cancellation/Unicode/wrap/stable layout" << std::endl;
}
