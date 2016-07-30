/*
 * share.h
 *
 *  Created on: 01.05.2010
 *      Author: dilma
 */

#ifndef SHARE_H_
#define SHARE_H_

#include <iostream>
#include <gtkmm.h>
#include <glib.h>

using namespace Gtk;
using namespace Glib;

#define HIASM_VERSION_MAJOR 5
#define HIASM_VERSION_MINOR 0
#define HIASM_VERSION_BUILD 12
gchar *HIASM_VERSION();

#ifdef G_OS_WIN32
	#define LINE_END "\n"
	#define PATH_SLASH "\\"
#else
	#define LINE_END "\n"
	#define PATH_SLASH "/"
#endif

// point types ----------------------------------------------------------------------------------------

#define pt_work  1
#define pt_event 2
#define pt_var   3
#define pt_data  4

// data types -----------------------------------------------------------------------------------------
enum DataType {
	data_null = 0,
	data_int,
	data_str,
	data_data,
	data_combo,
	data_list,
	data_icon,
	data_real,
	data_color,
	data_script,
	data_stream,
	data_bitmap,
	data_wave,
	data_array,
	data_comboEx,
	data_font,
	data_matr,
	data_jpeg,
	data_menu,
	data_code,
	data_element,
	data_flags,
	data_stock,
	data_pixbuf,

	data_count
};

extern const char *dataNames[];

#ifndef max
  #define max(x,y) (((x) > (y)) ? (x) : (y))
  #define min(x,y) (((x) < (y)) ? (x) : (y))
#endif

// element types ----------------------------------------------------------------------------------
enum ElementType {
	CI_DPElement = 1,
	CI_MultiElement,
	CI_EditMulti,
	CI_EditMultiEx,
	CI_InlineCode,
	CI_DrawElement,
	CI_AS_Special,
	CI_DPLElement,
	CI_UseHiDLL,
	CI_WinElement,
	CI_PointHint,
};

#define str_to_int(s) atoi(s.c_str())
extern ustring int_to_str(int value);
extern ustring double_to_str(double value);

extern char *getTok(char **buf, char tok);

// path ----------------------------------------------------------------------------------------------
#define INT_PATH     "int"PATH_SLASH
#define ICONS_PATH     INT_PATH"icons"PATH_SLASH
#define CURSORS_PATH   INT_PATH"cur"PATH_SLASH
#define DATABASE_FILE  "hiasm.db"
#define LANG_PATH      INT_PATH"lang"PATH_SLASH
#define ELEMENTS_PATH  "elements"PATH_SLASH
#define ELEMENTS_ICON_PATH  "icon"PATH_SLASH
#define ELEMENTS_CODE_PATH  "code"PATH_SLASH
#define ELEMENTS_CONF_PATH  "conf"PATH_SLASH
#define ELEMENTS_NEW_PATH   "new"PATH_SLASH
#define ELEMENTS_MAKE_PATH  "make"PATH_SLASH
#define PACK_ICON_FILE  "icon.png"
#define SPLASH_LOGO_FILE  "splash.png"

#define TABS_STATE_PATH	"tabs"PATH_SLASH
#define APP_SETTINGS "settings.ini"

#define ELEMENTS_NIL_FILE ELEMENTS_PATH"_base"PATH_SLASH""ELEMENTS_ICON_PATH"nil.png"

extern ustring databaseFile;
extern ustring dataDir;
extern ustring homeDir;

// internal types --------------------------------------------------------------------------------------
typedef Cairo::RefPtr<Cairo::Context> DrawContext;
typedef Glib::RefPtr<Gdk::Pixbuf> TypePixbuf;
typedef Gdk::RGBA TypeColor;

// macro -----------------------------------------------------------------------------------------------
#define TRACE_PROC //std::cout << "[" << __FILE__ << "]:" << __FUNCTION__ << std::endl;
#define DEBUG_MSG(v) {}// std::cout << v << std::endl;
#define ERROR_MSG(v) std::cout << v << std::endl;

// buttons ---------------------------------------------------------------------------------------------
#define BTN_LEFT   1
#define BTN_MIDDLE 2
#define BTN_RIGHT  3

#define KEY_SHIFT    1
#define KEY_CONTROL  4
#define KEY_ALT      8

// cross classes callback ------------------------------------------------------------------------------
/*! \enum CallbackType
    \brief callback event index

    Provides methods for the management teams of environment and interface elements that cause them
*/

/** test */
typedef enum {
	CBT_SELECT_ELEMENT_PALETTE,	/**< do when user select element in Palette */
	CBT_BEGIN_OPERATION,		/**< do when call beginOperation in SDK_Editor */
	CBT_END_OPERATION,			/**< do when call endOperation in SDK_Editor */
	CBT_POPUP_MENU,				/**< do when necessary to show the context menu in SDK_Editor */
	CBT_SDK_REDRAW_RECT,		/**< do when SDK require redraw rect */
	CBT_SDK_ADD_ELEMENT,		/**< do when element added to SDK */
	CBT_SDK_REMOVE_ELEMENT,		/**< do when element removed from SDK */
	CBT_SDK_CHANGE_SELECT,		/**< do when selectionset changes in SelectManager */
	CBT_SDK_RUN_CHANGE,			/**< do when call run() or stop() methods in SDK */
	CBT_RUN_COMMAND,			/**< do when user call internal command */
	CBT_CMD_ENABLED,			/**< do when commands need to refresh her status */
	CBT_CHANGE_PROPERTY,		/**< do when user changed property value in PropEditor */
	CBT_SELECT_PROPERTY,		/**< do when user selected property in PropEditor */
	CBT_CHECK_PROPERTY,			/**< do when user clicked on checkbox of property in PropEditor */
	CBT_EDIT_PROPERTY,			/**< do when user clicked on edit button in PropEditor */
	CBT_TAB_OPEN,				/**< do when successfully opened a new file in WindowManager (data is WM_Tab) */
	CBT_TAB_CHANGE,				/**< do when selected a new current tab in WindowManager (data is WM_Tab) */
	CBT_TAB_CLOSE,				/**< do when tab has been closed in WindowManager (data is WM_Tab) */
	CBT_KEY_DOWN,				/**< do when user press any key */
	CBT_ELEMENT_PROP_CHANGE,	/**< do when element property changed */
	CBT_CHANGE_SDK,				/**< do when changed current SDK in SDK_Editor */
	CBT_CHANGE_SCHEME,			/**< do when scheme is changed */
	CBT_CHANGE_RECENT_LIST,		/**< do when recent file list is change */
	CBT_CHANGE_SETTINGS,		/**< do when user change application settings */
	CBT_DEBUG_INFO,				/**< do when module generates some information for user */
} CallbackType;

/*! \class CallBack
    \brief interface for callback events

    Provides an interface for organizing callbacks between classes.
    To subscribe to the events use a field of type Event. Example: myObject->on_some_event += this
*/

class CallBack {
	public:
		virtual ~CallBack() { }
		/**
		 * Emit for all subscribers then user call on_some_event.run(..)
		 * @param owner class who emit this event
		 * @param type index of event from CallbackType enum
		 * @param data pointer to user data
		 */
		virtual void callback(void *owner, CallbackType type, const void *data) = 0;
};

/*! \class Event
    \brief list of subscribers

    Provides an interface to create a list of subscribers to callback between classes
*/

class Event {
	private:
		std::list<CallBack*> objs;
		CallbackType type;
		void *owner;
	public:
		bool enabled;	/**< Event enabled. If false, the execution method run have no effect */

		/**
		 * Create event object
		 * @param owner class who contain this event (usually this)
		 * @param type index of event
		 */
		Event(void *owner, CallbackType type) { this->type = type; this->owner = owner; enabled = true; }
		~Event() { objs.clear(); }
		/**
		 * Subscribe to event
		 * @param obj class who receive callback (must inherit from class CallBack)
		 */
		Event& operator +=(CallBack *obj) {
			objs.push_back(obj);
			return *this;
		}
		/**
		 * Unsubscribe from the event
		 * @param obj class that was previously added to the list
		 */
		Event& operator -=(CallBack *obj) {
			objs.remove(obj);
			return *this;
		}
		/**
		 * Run event for all subscribers
		 * @param data user data
		 */
		void run(const void *data) {
			if(enabled)
				for(std::list<CallBack*>::iterator event = objs.begin(); event != objs.end(); event++)
					(*event)->callback(owner, type, data);
		}
};

class ObjectEvent {
	public:
		typedef void (*event)(void *obj);
	private:
		void *obj;
		event proc;
	public:
		ObjectEvent() { clear(); }
		ObjectEvent(void *obj, event proc) { this->obj = obj; this->proc = proc; }
		void operator ()() {
			if(proc)
				proc(obj);
		}
		void clear() { obj = NULL; proc = NULL; }
		inline bool empty() { return obj == NULL; }
};

#define EVENT(x) ObjectEvent(this, (ObjectEvent::event)x)

Glib::RefPtr<Gdk::Pixbuf> getPointIcon(int type);

extern void initDirs();

extern void parseHintName(const ustring &text, ustring &name, ustring &hint);

#endif

/* SHARE_H_ */
