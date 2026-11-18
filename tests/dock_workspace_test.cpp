#include "DockWorkspace.h"
#include <cmath>
#include <iostream>
#include <string>

#define CHECK(expr) do { if(!(expr)) { std::cerr << __LINE__ << ": " << #expr << std::endl; return 1; } } while(0)

static void settle() {
    auto context = Glib::MainContext::get_default();
    for(int i = 0; i < 35; ++i) {
        while(context->pending()) context->iteration(false);
        g_usleep(3000);
    }
}

static graphene_rect_t bounds(Gtk::Widget &widget, Gtk::Widget &ancestor) {
    graphene_rect_t result{};
    if(!gtk_widget_compute_bounds(GTK_WIDGET(widget.gobj()), GTK_WIDGET(ancestor.gobj()), &result))
        return graphene_rect_t{};
    return result;
}

static bool inside(Gtk::Widget &widget, Gtk::Widget &ancestor) {
    const auto b = bounds(widget, ancestor);
    return b.origin.x >= -1 && b.origin.y >= -1 && b.size.width > 0 && b.size.height > 0 &&
        b.origin.x + b.size.width <= ancestor.get_width() + 1 &&
        b.origin.y + b.size.height <= ancestor.get_height() + 1;
}

// Render the actual GTK tree for inspection, without screen-capture helpers.
static bool snapshot(Gtk::Widget &widget, const std::string &path) {
    auto *paintable = gtk_widget_paintable_new(GTK_WIDGET(widget.gobj()));
    auto *capture = gtk_snapshot_new();
    gdk_paintable_snapshot(paintable, GDK_SNAPSHOT(capture), widget.get_width(), widget.get_height());
    auto *node = gtk_snapshot_free_to_node(capture);
    g_object_unref(paintable);
    if(!node) return false;
    auto *renderer = gtk_native_get_renderer(gtk_widget_get_native(GTK_WIDGET(widget.gobj())));
    graphene_rect_t viewport = GRAPHENE_RECT_INIT(0, 0, float(widget.get_width()), float(widget.get_height()));
    auto *texture = gsk_renderer_render_texture(renderer, node, &viewport);
    gsk_render_node_unref(node);
    if(!texture) return false;
    const bool saved = gdk_texture_save_to_png(texture, path.c_str());
    g_object_unref(texture);
    return saved;
}

int main(int argc, char **argv) {
    auto app = Gtk::Application::create("com.hiasm.dock-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
    if(Glib::file_test("hiasm.css", Glib::FileTest::IS_REGULAR)) {
        auto css = Gtk::CssProvider::create();
        css->load_from_path("hiasm.css");
        Gtk::StyleProvider::add_provider_for_display(Gdk::Display::get_default(), css,
            GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    }
    Gtk::TextView editor;
    editor.get_buffer()->set_text("Scheme editor / Редактор схемы");
    editor.set_size_request(200, 120);
    Gtk::Label debug("Debug output / Отладка");
    debug.set_size_request(100, 70);
    Gtk::ListBox elements;
    for(const char *name : {"Button / Кнопка", "Edit / Поле ввода", "Label / Надпись"})
        elements.append(*Gtk::make_managed<Gtk::Label>(name));
    Gtk::ScrolledWindow palette;
    palette.set_child(elements);
    palette.set_size_request(360, 100); // Content may exceed the preferred panel width.
    Gtk::Entry value;
    value.set_text("Editable property");
    Gtk::ScrolledWindow properties;
    properties.set_child(value);
    DockWorkspace workspace(editor, debug);
    auto &left = workspace.add_panel("Элементы", palette, DockSide::Left, 240);
    auto &right = workspace.add_panel("Свойства элемента", properties, DockSide::Right, 300);
    Gtk::Window window;
    window.set_title("HiAsm embedded dock test");
    window.set_default_size(1100, 720);
    window.set_child(workspace);
    app->add_window(window);
    window.present();
    settle();
    const auto toplevels = Gtk::Window::list_toplevels().size();
    auto embedded = [&]() {
        for(auto panel : {&left, &right}) {
            if(!panel->is_panel_visible()) continue;
            if(gtk_widget_get_root(GTK_WIDGET(panel->gobj())) != GTK_ROOT(window.gobj())) return false;
            if(gtk_widget_get_native(GTK_WIDGET(panel->gobj())) != GTK_NATIVE(window.gobj())) return false;
        }
        return Gtk::Window::list_toplevels().size() == toplevels;
    };
    CHECK(workspace.get_width() >= 900);
    CHECK(embedded() && inside(left, workspace) && inside(right, workspace));
    CHECK(left.is_pinned() && left.side() == DockSide::Left);
    CHECK(right.is_pinned() && right.side() == DockSide::Right);
    CHECK(bounds(left, workspace).origin.x < bounds(editor, workspace).origin.x);
    CHECK(bounds(right, workspace).origin.x > bounds(editor, workspace).origin.x);
    CHECK(left.header().get_width() > left.header().get_height());
    CHECK(right.header().get_width() > right.header().get_height());
    g_signal_emit_by_name(left.close_button().gobj(), "clicked");
    settle();
    CHECK(!left.is_panel_visible() && left.collapsed_button().get_parent() != nullptr);
    CHECK(left.collapsed_button().get_width() >= 24 && left.collapsed_button().get_height() >= 80);
    CHECK(left.collapsed_button().get_parent()->get_width() >= 24);
    g_signal_emit_by_name(left.collapsed_button().gobj(), "clicked");
    settle();
    CHECK(left.is_panel_visible() && left.collapsed_button().get_parent() == nullptr);
    if(argc == 2) CHECK(snapshot(workspace, std::string(argv[1]) + "/dock-default.png"));

    int changes = 0;
    workspace.signal_changed().connect([&]() { ++changes; });
    g_signal_emit_by_name(left.close_button().gobj(), "clicked");
    settle();
    CHECK(!left.is_panel_visible() && !left.get_parent() && changes == 1);
    workspace.set_visible(left, true); // Same operation as restoring via View.
    settle();
    CHECK(left.is_panel_visible() && embedded() && inside(left, workspace));
    g_signal_emit_by_name(left.pin_button().gobj(), "clicked");
    settle();
    CHECK(!left.is_pinned() && left.get_parent() == &workspace && embedded());
    CHECK(inside(left, workspace) && left.header().get_width() > left.header().get_height());
    g_signal_emit_by_name(left.pin_button().gobj(), "clicked");
    settle();
    CHECK(left.is_pinned() && embedded());

    const auto header = bounds(left.header(), workspace);
    const double startX = header.origin.x + header.size.width / 2;
    const double startY = header.origin.y + header.size.height / 2;
    workspace.begin_drag(left, startX, startY);
    workspace.update_drag(startX + 2, startY + 2);
    workspace.end_drag();
    CHECK(left.is_pinned() && workspace.preview_side() == -1);

    workspace.begin_drag(left, startX, startY);
    workspace.update_drag(workspace.get_width() / 2, workspace.get_height() / 2);
    settle();
    CHECK(!left.is_pinned() && workspace.preview_side() == -1 && embedded());
    workspace.update_drag(workspace.get_width() - 5, 150);
    settle();
    CHECK(workspace.preview_side() == int(DockSide::Right));
    if(argc == 2) CHECK(snapshot(workspace, std::string(argv[1]) + "/dock-preview.png"));
    workspace.end_drag();
    settle();
    CHECK(left.is_pinned() && left.side() == DockSide::Right && embedded());

    for(auto side : {DockSide::Bottom, DockSide::Left}) {
        const auto b = bounds(left.header(), workspace);
        workspace.begin_drag(left, b.origin.x + b.size.width / 2, b.origin.y + b.size.height / 2);
        const double x = side == DockSide::Bottom ? workspace.get_width() / 2 : 4;
        const double y = side == DockSide::Bottom ? workspace.get_height() - 4 : 160;
        workspace.update_drag(x, y);
        CHECK(workspace.preview_side() == int(side));
        workspace.end_drag();
        settle();
        CHECK(left.is_pinned() && left.side() == side && embedded() && inside(left, workspace));
        if(side == DockSide::Bottom) {
            CHECK(bounds(left, workspace).origin.y > bounds(editor, workspace).origin.y);
            CHECK(left.header().get_width() < left.header().get_height());
        } else {
            CHECK(left.header().get_width() > left.header().get_height());
        }
    }

    const auto leftHeader = bounds(left.header(), workspace);
    workspace.begin_drag(left, leftHeader.origin.x + leftHeader.size.width / 2,
                         leftHeader.origin.y + leftHeader.size.height / 2);
    workspace.update_drag(450, 180);
    workspace.end_drag(true);
    settle();
    CHECK(left.is_pinned() && left.side() == DockSide::Left && embedded());
    workspace.set_pinned(left, false);
    settle();
    const auto original = bounds(left, workspace);
    workspace.begin_drag(left, original.origin.x + 30, original.origin.y + 14);
    workspace.update_drag(650, 250);
    workspace.end_drag(true);
    settle();
    CHECK(!left.is_pinned() && embedded());
    CHECK(std::abs(bounds(left, workspace).origin.x - original.origin.x) < 1);
    CHECK(std::abs(bounds(left, workspace).origin.y - original.origin.y) < 1);

    workspace.begin_drag(left, original.origin.x + 30, original.origin.y + 14);
    workspace.update_drag(workspace.get_width() + 200, workspace.get_height() + 200);
    workspace.end_drag();
    settle();
    CHECK(!left.is_pinned() && workspace.preview_side() == -1 && embedded() && inside(left, workspace));
    if(argc == 2) CHECK(snapshot(workspace, std::string(argv[1]) + "/dock-floating.png"));
    window.set_default_size(760, 480);
    settle();
    CHECK(inside(left, workspace) && inside(right, workspace) && embedded());
    window.hide();
    window.unset_child();
    app->remove_window(window);
    std::cout << "OK embedded docks: one native window, hide/restore, pin/unpin, three edges, preview, cancel, bounds" << std::endl;
    return 0;
}
