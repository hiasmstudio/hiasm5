#include "share.h"
#include "ThemeColors.h"
#include <iostream>

int main() {
    g_setenv("GTK_CSD", "0", TRUE);
    auto app = Gtk::Application::create("com.hiasm.frame-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
    Gtk::Window window;
    window.set_title("HiAsm native frame test");
    window.set_default_size(400, 250);
    compat_standard_frame(window);
    app->add_window(window);
    window.present();
    auto context = Glib::MainContext::get_default();
    for(int i=0;i<50;++i) { while(context->pending()) context->iteration(false); g_usleep(2000); }
    auto surface = window.get_surface();
    if(!surface) return 1;
    HWND handle = static_cast<HWND>(gdk_win32_surface_get_handle(surface->gobj()));
    const auto style = GetWindowLongPtrW(handle, GWL_STYLE);
    if((style & WS_OVERLAPPEDWINDOW) != WS_OVERLAPPEDWINDOW || (style & (WS_CHILD | WS_POPUP))) return 2;
    if(gtk_window_get_titlebar(window.gobj())) return 3;
    themeSetDark(true);
    if(!themePrefersDark()) return 4;
    themeSetDark(false);
    window.hide();
    app->remove_window(window);
    std::cout << "OK Win32 overlapped frame, no GTK titlebar, theme change" << std::endl;
    return 0;
}
