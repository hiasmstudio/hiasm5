#ifndef HIASM_THEME_COLORS_H
#define HIASM_THEME_COLORS_H

inline bool themePrefersDark() {
    auto settings = gtk_settings_get_default();
    if(!settings) return false;
    if(g_object_class_find_property(G_OBJECT_GET_CLASS(settings), "gtk-interface-color-scheme")) {
        int scheme = 0;
        g_object_get(settings, "gtk-interface-color-scheme", &scheme, nullptr);
        return scheme == 2; // GTK_INTERFACE_COLOR_SCHEME_DARK (GTK 4.20+)
    }
    gboolean dark = FALSE;
    g_object_get(settings, "gtk-application-prefer-dark-theme", &dark, nullptr);
    return dark;
}
inline void themeSetDark(bool dark) {
    auto settings = gtk_settings_get_default();
    if(!settings) return;
    if(g_object_class_find_property(G_OBJECT_GET_CLASS(settings), "gtk-interface-color-scheme"))
        g_object_set(settings, "gtk-interface-color-scheme", dark ? 2 : 3, nullptr);
    else
        g_object_set(settings, "gtk-application-prefer-dark-theme", dark, nullptr);
}

// Shared colors for Cairo-drawn widgets. Never use the foreground as a fill.
inline Gdk::RGBA themeNamedColor(const Glib::RefPtr<Gtk::StyleContext> &style,
                                const char *name, const char *fallback) {
    Gdk::RGBA color;
    if(!style || !style->lookup_color(name, color)) color.set(fallback);
    return color;
}
inline Gdk::RGBA themeBackground(const Glib::RefPtr<Gtk::StyleContext> &style) {
    return themeNamedColor(style, "theme_bg_color", "#f6f5f4");
}
inline Gdk::RGBA themeBase(const Glib::RefPtr<Gtk::StyleContext> &style) {
    return themeNamedColor(style, "theme_base_color", "#ffffff");
}
inline Gdk::RGBA themeText(const Glib::RefPtr<Gtk::StyleContext> &style) {
    return style->get_color();
}
inline Gdk::RGBA themeSelected(const Glib::RefPtr<Gtk::StyleContext> &style) {
    return themeNamedColor(style, "theme_selected_bg_color", "#3584e4");
}
inline Gdk::RGBA themeSelectedText(const Glib::RefPtr<Gtk::StyleContext> &style) {
    return themeNamedColor(style, "theme_selected_fg_color", "#ffffff");
}
inline Gdk::RGBA themeMix(const Gdk::RGBA &a, const Gdk::RGBA &b, double weight) {
    Gdk::RGBA color;
    color.set_rgba(a.get_red()*(1-weight)+b.get_red()*weight,
                   a.get_green()*(1-weight)+b.get_green()*weight,
                   a.get_blue()*(1-weight)+b.get_blue()*weight, 1);
    return color;
}
#endif
