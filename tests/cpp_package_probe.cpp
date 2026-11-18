// Check dynamically loaded support without the compiler's PATH or cwd.
#include <gtk/gtk.h>
#include <iostream>
#ifdef G_OS_WIN32
#include <glib/gwin32.h>
#endif

int main(int argc, char** argv) {
    if(argc != 2) return 2;
    gtk_init();
    GError* error = nullptr;
#ifdef G_OS_WIN32
    auto utf8args = g_win32_get_command_line();
    const char* filename = utf8args[1];
#else
    const char* filename = argv[1];
#endif
    auto image = gdk_pixbuf_new_from_file(filename, &error);
#ifdef G_OS_WIN32
    g_strfreev(utf8args);
#endif
    if(!image) {
        std::cerr << "SVG load failed: " << (error ? error->message : "unknown") << std::endl;
        g_clear_error(&error);
        return 3;
    }
    if(gdk_pixbuf_get_width(image) != 16 || gdk_pixbuf_get_height(image) != 16) return 4;
    g_object_unref(image);
    auto theme = gtk_icon_theme_get_for_display(gdk_display_get_default());
    if(!gtk_icon_theme_has_icon(theme, "document-open-symbolic")) return 5;
    auto icon = gtk_icon_theme_lookup_icon(theme, "document-open-symbolic", nullptr, 32, 1,
                                           GTK_TEXT_DIR_LTR, GTK_ICON_LOOKUP_FORCE_SYMBOLIC);
    if(!icon || !gtk_icon_paintable_get_file(icon)) return 6;
    g_object_unref(icon);
    auto window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "HiAsm standalone GTK4 probe");
    gtk_window_set_child(GTK_WINDOW(window), gtk_label_new(u8"Автономное приложение 世界"));
    gtk_window_present(GTK_WINDOW(window));
    std::cout << "OK standalone SVG and icon theme" << std::endl;
    const auto deadline = g_get_monotonic_time() + 3 * G_TIME_SPAN_SECOND;
    while(g_get_monotonic_time() < deadline) {
        while(g_main_context_iteration(nullptr, false)) {}
        g_usleep(1000);
    }
    gtk_window_destroy(GTK_WINDOW(window));
    return 0;
}
