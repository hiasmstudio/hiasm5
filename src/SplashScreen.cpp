/*
 * SplashScreen.cpp
 *
 *  Created on: Jul 3, 2010
 *      Author: dilma
 */

#include "SplashScreen.h"

SplashScreen::SplashScreen():Window() {
	set_decorated(false);
#ifndef HIASM_GTK4
	set_position(WIN_POS_CENTER);
#endif

#ifdef HIASM_GTK4
	set_child(logo);
#else
	add(logo);
#endif
}

SplashScreen::~SplashScreen() {
	// TODO Auto-generated destructor stub
}

void SplashScreen::view() {
	logo.set(dataDir + SPLASH_LOGO_FILE);
	int logo_width = logo.get_width();
	int logo_height = logo.get_height();
	if(logo_width > 0 && logo_height > 0)
		set_default_size(logo_width, logo_height);

	show();
	while(Main::events_pending()) Main::iteration();
}
