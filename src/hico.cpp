//============================================================================
// Name        : test.cpp
// Author      : Dilma
// Version     :
// Copyright   : HiAsm Studio (C), 2003-2010
// Description : Primary file
//============================================================================

#include <iostream>

#include <gtkmm.h>
#include <glibmm.h>
#include <cairomm/cairomm.h>

#include "main_gui.h"

using namespace std;
using namespace Gtk;

#ifdef G_OS_WIN32
// libstdc++ on Windows cannot construct named locales, so glibmm/gtkmm
// always fail to sync the global C++ locale with the Windows locale name
// and print benign warnings on startup. Drop only those messages and
// pass everything else to the default handler.
static void drop_locale_warnings(const gchar *domain, GLogLevelFlags level,
		const gchar *message, gpointer data) {
	if(message && strstr(message, "locale"))
		return;
	g_log_default_handler(domain, level, message, data);
}
#endif

int main(int argc, char** argv) {
#if defined(G_OS_WIN32) && defined(HIASM_GTK4)
	// GTK4 enables its own titlebar on Win32 unless explicitly disabled before
	// GTK initialization. Changing WS_CAPTION afterwards leaves a second bar.
	g_setenv("GTK_CSD", "0", TRUE);
#endif
#ifdef G_OS_WIN32
	g_log_set_handler(NULL, (GLogLevelFlags)(G_LOG_LEVEL_WARNING |
		G_LOG_FLAG_FATAL | G_LOG_FLAG_RECURSION), drop_locale_warnings, NULL);
#endif

#if !defined(G_OS_WIN32) && !defined(HIASM_GTK4)
	gdk_threads_init();	// removed in GTK4
#endif

	Main kit(argc, argv);
#ifdef GL_SDK
	Gtk::GL::init(argc, argv);
#endif

	MainGUI main;

	Main::run(main);

	return (0);
}
