#include <gtk/gtk.h>
#include <gdk/win32/gdkwin32.h>
#include <windows.h>
#include <iostream>

// Explicit IME contexts reproduce the dangling Win32 message filter without
// requiring keyboard input or changing the user's desktop focus.
int main() {
    gtk_init();
    auto window=GTK_WINDOW(gtk_window_new());
    auto box=gtk_box_new(GTK_ORIENTATION_VERTICAL,0);
    gtk_window_set_child(window,box);
    gtk_widget_realize(GTK_WIDGET(window));
    auto surface=gtk_native_get_surface(GTK_NATIVE(window));
    const auto handle=gdk_win32_surface_get_handle(surface);
    if(!handle) return 1;
    for(int i=0;i<100;++i) {
        auto entry=gtk_entry_new();
        gtk_box_append(GTK_BOX(box),entry);
        gtk_widget_realize(entry);
        auto context=GTK_IM_CONTEXT(g_object_new(g_type_from_name("GtkIMContextIME"),nullptr));
        if(!context) return 2;
        gtk_im_context_set_client_widget(context,entry);
        gtk_im_context_focus_in(context);
        gtk_im_context_focus_in(context); // Do not register duplicate filters.
        if(i%3==0) {
            gtk_im_context_set_client_widget(context,nullptr);
            g_object_unref(context);
            gtk_box_remove(GTK_BOX(box),entry);
        } else if(i%3==1) {
            // The widget/surface disappears before the input context.
            gtk_box_remove(GTK_BOX(box),entry);
            gtk_im_context_focus_out(context);
            g_object_unref(context);
        } else {
            g_object_unref(context);
            gtk_box_remove(GTK_BOX(box),entry);
        }
        // The old filter was attached to the display and used to receive this
        // message after its context had already been freed.
        SendMessageW(handle,WM_NULL,0,0);
        while(g_main_context_iteration(nullptr,false)) {}
    }
    gtk_window_destroy(window);
    std::cout<<"OK Win32 IME lifecycle: focus, detach, unrealize, dispose, 100 cycles\n";
}
