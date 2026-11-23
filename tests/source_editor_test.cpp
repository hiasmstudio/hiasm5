#include "share.h"
#include "GsvCompat.h"
#include <iostream>

int main(int argc, char **argv) {
    if(argc != 2) return 2;
    auto app = Gtk::Application::create("com.hiasm.source-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
    Gsv::init(Glib::ustring(argv[1]) + G_DIR_SEPARATOR_S);
    const char *files[] = {"test.cpp", "test.hpp", "test.pas", "TEST.DPR", "test.hws", "test.sha", "unknown.zzz"};
    const char *ids[] = {"cpp", "cpphdr", "pascal", "pascal", "hws", "sha", nullptr};
    for(unsigned i=0; i<7; ++i) {
        auto view = Gsv::create_view();
        Gsv::configure_view(*view, files[i]);
        auto buffer = view->get_buffer();
        if(!GTK_SOURCE_IS_BUFFER(buffer->gobj())) return 3;
        auto source = GTK_SOURCE_BUFFER(buffer->gobj());
        auto language = gtk_source_buffer_get_language(source);
        if(ids[i] && (!language || strcmp(gtk_source_language_get_id(language), ids[i]))) {
            std::cerr << files[i] << ": " << (language ? gtk_source_language_get_id(language) : "missing language") << std::endl;
            return 4;
        }
        if(!gtk_source_view_get_show_line_numbers(GTK_SOURCE_VIEW(view->gobj())) || !view->get_monospace()) return 5;
        buffer->set_text(u8"Привет 世界\nint main() {}");
        if(buffer->get_text() != u8"Привет 世界\nint main() {}") return 6;
        auto start = buffer->begin(), end = buffer->end();
        gtk_source_buffer_ensure_highlight(source, start.gobj(), end.gobj());
        themeSetDark(true);
        auto scheme = gtk_source_buffer_get_style_scheme(source);
        if(!scheme || !strstr(gtk_source_style_scheme_get_id(scheme), "dark")) return 7;
        themeSetDark(false);
        delete view;
    }
    std::cout << "OK source editor languages, Unicode, line numbers and source buffers" << std::endl;
    return 0;
}
