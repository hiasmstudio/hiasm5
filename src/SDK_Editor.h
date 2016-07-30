/*
 * SDK_Editor.h
 *
 *  Created on: 01.05.2010
 *      Author: dilma
 */

#ifndef SDK_EDITOR_H_
#define SDK_EDITOR_H_

//#define GL_SDK

#include <cairomm/cairomm.h>
#ifdef GL_SDK
	#include <gtkglextmm-1.2/gtkglmm.h>
	#include <GL/gl.h>
#endif

#include "share.h"
#include "SDK.h"
#include "LayerObjects.h"

//#define OPENGL_SDE

typedef enum {
	MS_NO_OPERATION,
	MS_INSERT_ELEMENT,
	MS_MOVE_ELEMENT,
	MS_FOCUS_RECT,
	MS_POINT_LINK,
	MS_MOVE_LINE_POINT,
	MS_LINE,
	MS_MOUSE_ELEMENT,
	MS_SELECT_BY_EDGE,
	MS_SCROLL,
	MS_MOVE_LINK_HINT,
	MS_ADD_LINK_HINT,
	MS_REMOVE_LINK_HINT,
	MS_COUNT
} MouseOperation;

#define HINT_NONE    0x00
#define HINT_POINT   0x01
#define HINT_ELEMENT 0x02
#define HINT_LH      0x04
#define HINT_LINK    0x08
#define HINT_ALL     HINT_POINT|HINT_ELEMENT|HINT_LH|HINT_LINK

typedef struct {
	MouseOperation cur_operation;	/**< code of current mouse operation */
	gdouble startx;					/**< starting position the mouse cursor on the x axis before the operation */
	gdouble starty;					/**< starting position the mouse cursor on the y axis before the operation */
	gdouble curx;					/**< current position the mouse cursor on the x axis */
	gdouble cury;					/**< current position the mouse cursor on the y axis */
	const void *data;				/**< user data stored in beginOperation() */
	ObjectType obj;					/**< the SDK object under the mouse pointer when you click */
	ObjectType select_obj;			/**< object under the pointer after moving the mouse */
	SDKObjectType begin_type;		/**< object under the pointer at the start of operation (required for popup menu) */
	MouseOperation begin_operation;	/**< op type at the start of operation (required for popup menu) */
	guint32 time;					/**< mouse event time */
} SDK_MouseEvent;

class SDK_Editor;

typedef void (SDK_Editor::*BeginProc)(const void *data);
typedef void (SDK_Editor::*DownProc)(gdouble x, gdouble y, int button, int state);
typedef void (SDK_Editor::*MoveProc)(gdouble x, gdouble y);
typedef void (SDK_Editor::*UpProc)(gdouble x, gdouble y, int button, int state);
typedef void (SDK_Editor::*EndProc)();
typedef Glib::RefPtr<Gdk::Cursor> (SDK_Editor::*CursorProc)();
typedef void (SDK_Editor::*DrawProc)(DrawContext cr);

/*! \struct ActionMap
    \brief map of mouse action for SDK_Editor

    Map of mouse events for schema editor SDK_Editor.
    All events take place only for the current operation, the code is located in mouse.cur_operation
*/

typedef struct {
	BeginProc beginProc;	/**< do after call beginOperation() */
	DownProc downProc;		/**< do then user mouse down in SDK_Editor */
	MoveProc moveProc;		/**< do then user mouse move in SDK_Editor */
	UpProc upProc;			/**< do then user mouse up in SDK_Editor */
	EndProc endProc;		/**< do after call endOperation() */
	CursorProc cursorProc;	/**< do after downProc, moveProc and upProc */
	DrawProc drawProc;		/**< do then invalidate SDK_Editor */
	unsigned int hintMask;	/**< mask to display hints to the objects of scheme during the operation */
} ActionMap;

typedef struct {
	const char *name;
	int x;
	int y;
} CursorInfo;

const CursorInfo curInfo[] = {
		{"moveLinePoint", 15, 15},
		{"addLinePoint", 10, 21},
		{"pointLink_none", 14, 2},
		{"pointLink_point", 14, 2},
		{"pointLink_line", 14, 2},
		{"insertElement", 3, 3},
		{"insertElement_line", 3, 3},
		{"insertElement_point", 3, 3},
		{"13", 3, 21},
		{"14", 3, 21},
		{"15", 3, 21},
		{"12", 16, 13},
		{"10", 13, 16},
		{"11", 13, 13},
};

typedef enum {
	// system
	CURSOR_DEFAULT,
	CURSOR_HAND,
	CURSOR_H_DOUBLE,
	CURSOR_V_DOUBLE,
	CURSOR_TL_CORNER,
	CURSOR_TR_CORNER,
	CURSOR_BL_CORNER,
	CURSOR_BR_CORNER,
	CURSOR_MOVE,
	// custom
	CURSOR_MOVE_LINE_POINT,
	CURSOR_ADD_LINE_POINT,
	CURSOR_POINT_LINK_NONE,
	CURSOR_POINT_LINK_POINT,
	CURSOR_POINT_LINK_LINE,
	CURSOR_INSERT_ELEMENT,
	CURSOR_INSERT_ELEMENT_LINE,
	CURSOR_INSERT_ELEMENT_POINT,
	CURSOR_LH_ADD_FAIL,
	CURSOR_LH_ADD_OK,
	CURSOR_LH_REMOVE,
	CURSOR_EDGE_HORZ,
	CURSOR_EDGE_VERT,
	CURSOR_EDGE_BOTH,
	CURSOR_COUNT
} CursorListType;

/*! \class CursorList
    \brief list of all application cursors

    Provides access to the application cursors. Custom cursors are loaded from the folder int/cur.
*/

class CursorList {
	private:
		Glib::RefPtr<Gdk::Cursor> *cursors;
		Glib::RefPtr<Gdk::Display> display;
	public:
		CursorList(const Glib::RefPtr<Gdk::Display>& display) {
			this->display = display;

			cursors = new Glib::RefPtr<Gdk::Cursor>[CURSOR_COUNT];
			cursors[CURSOR_DEFAULT] = Gdk::Cursor::create(Gdk::ARROW);
			cursors[CURSOR_HAND] = Gdk::Cursor::create(Gdk::HAND2);
			cursors[CURSOR_V_DOUBLE] = Gdk::Cursor::create(Gdk::SB_V_DOUBLE_ARROW);
			cursors[CURSOR_H_DOUBLE] = Gdk::Cursor::create(Gdk::SB_H_DOUBLE_ARROW);
			cursors[CURSOR_TL_CORNER] = Gdk::Cursor::create(Gdk::TOP_LEFT_CORNER);
			cursors[CURSOR_TR_CORNER] = Gdk::Cursor::create(Gdk::TOP_RIGHT_CORNER);
			cursors[CURSOR_BL_CORNER] = Gdk::Cursor::create(Gdk::BOTTOM_LEFT_CORNER);
			cursors[CURSOR_BR_CORNER] = Gdk::Cursor::create(Gdk::BOTTOM_RIGHT_CORNER);
			cursors[CURSOR_MOVE] = Gdk::Cursor::create(Gdk::FLEUR);

			std::cout << "list..." << std::endl;
			char buf[128];
			for(int i = CURSOR_MOVE_LINE_POINT; i < CURSOR_COUNT; i++) {
				const CursorInfo *inf = &curInfo[i-CURSOR_MOVE_LINE_POINT];
				sprintf(buf, "%s"CURSORS_PATH"%s.png", dataDir.c_str(), inf->name);
				cursors[i] = Gdk::Cursor::create(display, Gdk::Pixbuf::create_from_file(buf), inf->x, inf->y);
			}
		}
		inline Glib::RefPtr<Gdk::Cursor> cursor(CursorListType type) { return cursors[type]; }
		void setCustom(CursorListType type, Glib::RefPtr<Gdk::Pixbuf> pix) {
//			delete cursors[type];
			int i = type-CURSOR_MOVE_LINE_POINT;
			cursors[type] = Gdk::Cursor::create(display, pix, curInfo[i].x, curInfo[i].y);
		}
};

extern CursorList *curList;

/*! \class SDK_Editor
    \brief scheme editor

    Provides tools for displaying and editing scheme
*/

#ifdef GL_SDK
class SDK_Editor : public GL::DrawingArea, public CallBack {
#else
class SDK_Editor : public DrawingArea, public CallBack {
#endif
	private:
		SDK_MouseEvent mouse;	/**< structure contain parameters for mouse operations */
		double zoom;			/**< zoom factor */
		Glib::RefPtr<Gdk::Cursor> cursor;	/**< current cursor image */

		LayerObjects lobj;		/**< objects from top level drawing layer (hints, menus and other) */
		LayerObject *oldObj;	/**< last added layer object */

#ifdef OPENGL_SDE
		cairo_surface_t *opengl_surface;
		DrawContext cairo_contex;
		Cairo::RefPtr<Cairo::Surface> cairo_surface;
#endif

		Glib::RefPtr<Gdk::PixbufLoader> cursorLoader;
		Menu hintMenu;

		ElementPoints selPoints;	/**< list of selection points */

		void selectPoint(ElementPoint *point);
		void unselectPoints();

		void invalidateRect(gdouble x1, gdouble y1, gdouble x2, gdouble y2);
		void invalidateRealRect(gdouble x1, gdouble y1, gdouble x2, gdouble y2);

		/**
		 * Change current SDK_Editor cursor
		 */
		void changeCursor();
		/**
		 * Show layer hint for select object
		 * @param x mouse coordinate
		 * @param y mouse coordinate
		 */
		void showObjectHint(gdouble x, gdouble y);
		/**
		 * Set width and height for SDK_Editor
		 */
		void setGeometry();

		virtual Cairo::ErrorStatus savePNG(const unsigned char* data, unsigned int length);
		void createInsertCursor(TypePixbuf img, const char *maskName, CursorListType cursor);

		// mouse operation --------------------------------------------------------------------
		void noOperation_DownProc(gdouble x, gdouble y, int button, int state);
		void noOperation_MoveProc(gdouble x, gdouble y);
		void noOperation_UpProc(gdouble x, gdouble y, int button, int state);
		Glib::RefPtr<Gdk::Cursor> noOperation_cursorProc();

		void insertElement_BeginProc(const void *data);
		void insertElement_DownProc(gdouble x, gdouble y, int button, int state);
		void insertElement_MoveProc(gdouble x, gdouble y);
		void insertElement_UpProc(gdouble x, gdouble y, int button, int state);
		void insertElement_DrawProc(DrawContext cr);
		Glib::RefPtr<Gdk::Cursor> insertElement_cursorProc();

		void focusRect_DownProc(gdouble x, gdouble y, int button, int state);
		void focusRect_MoveProc(gdouble x, gdouble y);
		void focusRect_UpProc(gdouble x, gdouble y, int button, int state);
		void focusRect_DrawProc(DrawContext cr);

		void moveElement_DownProc(gdouble x, gdouble y, int button, int state);
		void moveElement_MoveProc(gdouble x, gdouble y);
		void moveElement_UpProc(gdouble x, gdouble y, int button, int state);

		void pointLink_DownProc(gdouble x, gdouble y, int button, int state);
		void pointLink_MoveProc(gdouble x, gdouble y);
		void pointLink_UpProc(gdouble x, gdouble y, int button, int state);
		void pointLink_DrawProc(DrawContext cr);
		Glib::RefPtr<Gdk::Cursor> pointLink_cursorProc();

		void moveLinePoint_DownProc(gdouble x, gdouble y, int button, int state);
		void moveLinePoint_MoveProc(gdouble x, gdouble y);
		void moveLinePoint_UpProc(gdouble x, gdouble y, int button, int state);
		Glib::RefPtr<Gdk::Cursor> moveLinePoint_cursorProc();

		void line_DownProc(gdouble x, gdouble y, int button, int state);

		void mouseElement_MoveProc(gdouble x, gdouble y);
		void mouseElement_UpProc(gdouble x, gdouble y, int button, int state);
		Glib::RefPtr<Gdk::Cursor> mouseElement_cursorProc();

		void selectByEdge_DownProc(gdouble x, gdouble y, int button, int state);
		void selectByEdge_MoveProc(gdouble x, gdouble y);
		void selectByEdge_UpProc(gdouble x, gdouble y, int button, int state);
		void selectByEdge_DrawProc(DrawContext cr);
		Glib::RefPtr<Gdk::Cursor> selectByEdge_cursorProc();

		void scroll_DownProc(gdouble x, gdouble y, int button, int state);
		void scroll_MoveProc(gdouble x, gdouble y);
		void scroll_UpProc(gdouble x, gdouble y, int button, int state);

		void linkHint_DownProc(gdouble x, gdouble y, int button, int state);
		void linkHint_MoveProc(gdouble x, gdouble y);
		void linkHint_UpProc(gdouble x, gdouble y, int button, int state);
		Glib::RefPtr<Gdk::Cursor> linkHint_cursorProc();

		void lh_add_DownProc(gdouble x, gdouble y, int button, int state);
		Glib::RefPtr<Gdk::Cursor> lh_add_cursorProc();

		void lh_remove_DownProc(gdouble x, gdouble y, int button, int state);
		Glib::RefPtr<Gdk::Cursor> lh_remove_cursorProc();
	protected:
		virtual void callback(void *owner, CallbackType type, const void *data);
		virtual bool on_key_press_event(GdkEventKey *key);
	public:
		MSDK *sdk;	/**< root sdk */
		SDK *csdk;	/**< current sdk, displayed in editor */

		ActionMap actionMap[MS_COUNT];	/**< map of mouse events in the editor */

		Event on_begin_operation;		/**< run after call beginOperation() method */
		Event on_end_operation;			/**< run after call endOperation() method */

		Event on_popup_menu;			/**< start when necessary to show the context menu */
		Event on_key_down;				/**< run after the user presses any key */

		Event on_change_sdk;			/**< run change current SDK */

		Event on_change_scheme;			/**< emit after scheme is change */

		SDK_Editor();
		~SDK_Editor();

		virtual bool on_draw (const ::Cairo::RefPtr< ::Cairo::Context >& cr);
		virtual bool on_expose_event(GdkEventExpose* event);
		virtual void on_realize();
#ifdef GL_SDK
		virtual bool on_configure_event(GdkEventConfigure* event);
		virtual void on_idle();
#endif
		virtual bool on_button_press_event(GdkEventButton* event);
		virtual bool on_button_release_event(GdkEventButton* event);
		virtual bool on_motion_notify_event(GdkEventMotion* event);
		bool load(const ustring& file_name);

		/**
		 * Begins one of the operations using the mouse
		 * @param op operation code from enum ::MouseOperation
		 * @param data some user data
		 */
		void beginOperation(MouseOperation op, const void *data = NULL);
		/**
		 * Finish mouse operation
		 */
		void endOperation();

		/**
		 * Request to full redraw the editor
		 */
		void reDraw();
		/**
		 * Request to redraw the rectangle region editor
		 */
		void reDrawRect(const Gdk::Rectangle &r);

		/**
		 * Obtaining a current zoom factor
		 * @return zoom factor
		 */
		inline double getZoom() { return zoom; }
		/**
		 * Set the current scale factor
		 * @param value zoom factor
		 */
		void setZoom(double value) { zoom = value; setGeometry(); reDraw(); }

		/**
		 * Save MSDK to file
		 * @param file_name name of file
		 * @return true if done, false - error
		 */
		bool saveToFile(const ustring &file_name);

		/**
		 * Enter in child SDK for MultiElement
		 * @param element pointer to MultiElement
		 */
		void enterSDK(Element *element);
		/**
		 * Return to parent SDK from MultiElement
		 */
		void leaveSDK();

		/**
		 * Call after scheme is change
		 */
		inline void changeScheme() { on_change_scheme.run(this); }

		/**
		 *
		 */
		bool pasteElementOnLine(const ustring &name);
};

typedef struct _ChangeSDKInfo {
	bool change;	/**< false - before change, true - after */
	SDK *sdk;		/**< pointer to changed SDK */
	_ChangeSDKInfo(SDK *sdk) { change = false; this->sdk = sdk; }
	void setSDK(SDK *sdk) { change = true; this->sdk = sdk; }
} ChangeSDKInfo;

#endif /* SDK_EDITOR_H_ */
