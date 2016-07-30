//============================================================================
// Name        : test.cpp
// Author      : Dilma
// Version     :
// Copyright   : HiAsm Studio 5 (C), 2003-2010
// Description : Primary file
//============================================================================

#include <iostream>

#include <gtkmm.h>
#include <glibmm.h>
#include <cairomm/cairomm.h>

#include "main_gui.h"

using namespace std;
using namespace Gtk;

int main(int argc, char** argv) {
	g_thread_init(NULL);
#ifndef G_OS_WIN32
	gdk_threads_init();
#endif

	Main kit(argc, argv);
#ifdef GL_SDK
	Gtk::GL::init(argc, argv);
#endif

	MainGUI main;

	Main::run(main);

	return (0);
}
