/*
 * SplashScreen.h
 *
 *  Created on: Jul 3, 2010
 *      Author: dilma
 */

#ifndef SPLASHSCREEN_H_
#define SPLASHSCREEN_H_

#include "share.h"

class SplashScreen : public Window {
	private:
		Image logo;
	public:
		SplashScreen();
		~SplashScreen();

		void view();
};

#endif /* SPLASHSCREEN_H_ */
