/*
 * SplashScreen.cpp
 *
 *  Created on: Jul 3, 2010
 *      Author: dilma
 */

#include "SplashScreen.h"

SplashScreen::SplashScreen():Window(WINDOW_POPUP) {
	set_decorated(false);
	set_position(WIN_POS_CENTER);

	add(logo);
}

SplashScreen::~SplashScreen() {
	// TODO Auto-generated destructor stub
}

void SplashScreen::view() {
	logo.set(dataDir + SPLASH_LOGO_FILE);
	set_default_size(logo.get_width(), logo.get_height());

	show_all();
	while(Main::events_pending()) Main::iteration();
}
