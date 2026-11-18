/*
 * DebugPanel.h
 *
 *  Created on: 01.05.2010
 *      Author: dilma
 */

#ifndef DEBUGPANEL_H_
#define DEBUGPANEL_H_

#include "share.h"

/*! \class DebugPanel
    \brief debug panel

    Provides a window to display debugging and informational messages
*/

class DebugPanel : public TextView {
	private:
		Glib::RefPtr<TextTag> error_tag;
		Glib::RefPtr<TextTag> info_tag;
		Glib::RefPtr<TextTag> trace_tag;
	protected:
	public:
		DebugPanel();

		/**
		 * Display simple text (default color)
		 * @param text data to add
		 */
		void text(const ustring &text);
		/**
		 * Display error message (red color)
		 * @param text data to add
		 */
		void error(const ustring &text);
		/**
		 * Display information message (gray color)
		 * @param text data to add
		 */
		void info(const ustring &text);
		/**
		 * Display trace message (green color)
		 * @param text data to add
		 */
		void trace(const ustring &text);
		/**
		 * Erase content of debug panel
		 */
		void clear() { get_buffer()->set_text(""); }
};

#endif /* DEBUGPANEL_H_ */
