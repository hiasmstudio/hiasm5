/*
 * GsvCompat.h
 *
 * GTK4 compatibility surface for the text editor. GtkSourceView 5 does not
 * provide a gtkmm wrapper, so the view and buffer are created through its C
 * API and then exposed through gtkmm's Gtk::Widget/Gtk::TextBuffer wrappers.
 */
#ifndef HIASM_GSV_COMPAT_H_
#define HIASM_GSV_COMPAT_H_

#ifdef HIASM_GTK4
#include <gtkmm.h>
#include <gtksourceview/gtksource.h>
#include <glibmm/miscutils.h>
#include "ThemeColors.h"
namespace Gsv {
	using View = Gtk::TextView;
    inline void update_style(View &view) {
        const auto bg = themeBase(view.get_style_context());
        const bool dark = themePrefersDark() ||
            bg.get_red()*0.2126 + bg.get_green()*0.7152 + bg.get_blue()*0.0722 < 0.5;
        auto schemes = gtk_source_style_scheme_manager_get_default();
        auto scheme = gtk_source_style_scheme_manager_get_scheme(schemes, dark ? "Adwaita-dark" : "Adwaita");
        if(!scheme) scheme = gtk_source_style_scheme_manager_get_scheme(schemes, dark ? "classic-dark" : "classic");
        gtk_source_buffer_set_style_scheme(GTK_SOURCE_BUFFER(view.get_buffer()->gobj()), scheme);
    }
	inline View *create_view() {
		GtkSourceView *source_view = GTK_SOURCE_VIEW(gtk_source_view_new());
		return Glib::wrap(GTK_TEXT_VIEW(source_view));
	}
	inline void configure_view(View &view, const Glib::ustring &file_name) {
		GtkSourceView *source_view = GTK_SOURCE_VIEW(view.gobj());
		gtk_source_view_set_show_line_numbers(source_view, TRUE);
		Glib::ustring base = Glib::path_get_basename(file_name.raw());
		base = base.lowercase();
		const auto dot = base.rfind('.');
		Glib::ustring ext = dot == Glib::ustring::npos ? Glib::ustring() : base.substr(dot+1);
		if(ext == "pas" || ext == "dpr" || ext == "pp")
			ext = "pascal";
		GtkSourceLanguageManager *manager = gtk_source_language_manager_get_default();
		GtkSourceLanguage *language = (ext == "hws" || ext == "sha" || ext == "pascal") ?
			gtk_source_language_manager_get_language(manager, ext.c_str()) :
			gtk_source_language_manager_guess_language(manager, base.c_str(), nullptr);
		// Unknown extensions still need a source buffer for line numbers and undo.
		GtkTextBuffer *buffer = GTK_TEXT_BUFFER(gtk_source_buffer_new(nullptr));
		gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(buffer), language);
		if(language)
			gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(buffer), TRUE);
		Glib::RefPtr<Gtk::TextBuffer> wrapped_buffer = Glib::wrap(buffer);
		view.set_buffer(wrapped_buffer);
		view.set_monospace(true);
		gtk_source_view_set_tab_width(source_view, 4);
		gtk_source_view_set_auto_indent(source_view, TRUE);
        update_style(view);
        view.signal_map().connect([&view]() { update_style(view); });
        // GObject disconnects this handler when the view is destroyed.
        const char *property = g_object_class_find_property(G_OBJECT_GET_CLASS(gtk_settings_get_default()), "gtk-interface-color-scheme") ?
            "notify::gtk-interface-color-scheme" : "notify::gtk-application-prefer-dark-theme";
        g_signal_connect_object(gtk_settings_get_default(), property,
            G_CALLBACK(+[](GtkSettings*, GParamSpec*, gpointer object) {
                update_style(*Glib::wrap(GTK_TEXT_VIEW(object)));
            }), view.gobj(), G_CONNECT_DEFAULT);
	}
	inline void init(const Glib::ustring &data_dir) {
		gtk_source_init();
		gtk_source_language_manager_prepend_search_path(gtk_source_language_manager_get_default(),
			(data_dir + "int/language-specs").c_str());
	}
}
#endif

#endif /* HIASM_GSV_COMPAT_H_ */
