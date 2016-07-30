/*
 * FormEditor.h
 *
 *  Created on: 19.05.2010
 *      Author: dilma
 */

#ifndef FORMEDITOR_H_
#define FORMEDITOR_H_

#include "share.h"
#include "SDK_Editor.h"

#include <gdk/gdk.h>

/*! \class FormEditor
    \brief base class for scheme editor

    Base class for an alternative scheme editor
*/

class FormEditor {
	protected:
		SDK_Editor *sde;
	public:
		ObjectEvent on_end_edit;

		FormEditor() { }

		/**
		 * Abstract method for initialize and show form editor
		 * @param sde pointer to parent SDK_Editor widget
		 */
		virtual void start(SDK_Editor *sde) { this->sde = sde; }
		/**
		 * Abstract method for close and destroy form editor
		 */
		virtual void stop() = 0;
};

class FE_ExternalTcp : public FormEditor, public CallBack {
	private:
		GSocket *sock;
		GSocket *client;
		GCancellable *cancel;
		Glib::Thread *serverThread;
		Glib::Thread *feThread;

		void on_server_thread();
		void on_form_edit_thread();
		void pk_add_element(Element *e);
		void pk_send(int num, void *data, int size);
		void pk_send_prop(ElementProperty *prop);
		void pk_send_remove(Element *e);
		void pk_send_select(SelectManager *sm);

		void callback(void *owner, CallbackType type, const void *data);
	public:
		FE_ExternalTcp() {}

		/**
		 * Abstract method for initialize and show form editor
		 * @param sde pointer to parent SDK_Editor widget
		 */
		void start(SDK_Editor *sde);
		/**
		 * Abstract method for close and destroy form editor
		 */
		void stop();
};

/*! \class WidgetInfo
    \brief snapshot of real widgets

    Provides tools for working with the image widget
*/

class WidgetEditor;

class WidgetInfo : public CallBack {
	private:
		Widget *ctl;
		const struct ModelColumns : public TreeModelColumnRecord {
			Gtk::TreeModelColumn<ustring> text;

			ModelColumns() { add(text); }
		} listColumns;

		bool changeProperty;
		void init();
	protected:
		virtual void callback(void *owner, CallbackType type, const void *data);
		virtual void on_size_allocate(Allocation &alloc);
		virtual void on_realize();
	public:
		int x;
		int y;
		int width;
		int height;
//		Glib::RefPtr<Gdk::Pixmap> bmp;
		bool select;
		bool change;
		Element *tpl;
		WidgetEditor *wedit;
		bool nb;
		GtkWidget *window;

		WidgetInfo(WidgetEditor *parent, Element *e);
		virtual ~WidgetInfo();
		virtual bool on_expose_event(GdkEventExpose* event);

		/**
		 * Take snapshot of current widget
		 */
		void snap();
		/**
		 * Checks to place the element in the given coordinates
		 * @param x left offset
		 * @param y top offset
		 * @return true if the test is successful
		 */
		bool checkAtPos(gdouble x, gdouble y);
		/**
		 * Shift widget at offset
		 * @param dx offset by x
		 * @param dy offset by y
		 */
		void move(int dx, int dy);
		/**
		 * Resize widget by value
		 * @param dx resize by width
		 * @param dy resize by height
		 */
		void size(int dx, int dy);
		/**
		 * Calcalute widget area
		 * @return area of widget
		 */
		Gdk::Rectangle getRect() { return Gdk::Rectangle(x,y,width,height); }
};

typedef std::list<WidgetInfo*> WidgetsList;

/*! \class WidgetEditor
    \brief edit position and geometry of widgets

    Provides tools for editing positions and sizes of visual elements in the form
*/

class WidgetEditor : public EventBox, public CallBack {
	protected:
		int start_x;
		int start_y;
		int cur_x;
		int cur_y;

		enum MouseState { WE_MS_NONE, WE_MS_MOVE, WE_MS_RECT, WE_MS_SIZER };
		MouseState state;
		int selSizer;
		/**
		 * Calculates the area selection of elements
		 * @param forDraw if true the region expands to 3 pixels
		 * @return correct selectin rectangle
		 */
		Gdk::Rectangle calcRectangle(bool forDraw = true);

		virtual bool on_draw (const ::Cairo::RefPtr< ::Cairo::Context >& cr);
		virtual bool on_expose_event(GdkEventExpose* event);
		virtual bool on_button_press_event(GdkEventButton* event);
		virtual bool on_button_release_event(GdkEventButton* event);
		virtual bool on_motion_notify_event(GdkEventMotion* event);
	public:
		SDK *sdk;
		WidgetsList wlist;

		WidgetEditor(SDK *parent);
		virtual ~WidgetEditor();

		/**
		 * Add child widget
		 * @param e pointer to template element
		 */
		void addWidget(Element *e);
		/**
		 * Return an widget located in given coordinates
		 * @param x left offset
		 * @param y top offset
		 * @return widget if find? else NULL
		 */
		WidgetInfo *getWidgetAtPos(gdouble x, gdouble y);
		/**
		 * Reset selection
		 */
		void clearSelection();

		void selectWigdetsFromRect();

		void setSelectionChange(bool value);

		void moveSelection(int dx, int dy);
		void sizeSelection(int dx, int dy);

		Gdk::Rectangle getSelectionRect();

		int getSizer(int x, int y);

		/**
		 * Redraw widget area
		 */
		void invalidate();

		virtual void callback(void *owner, CallbackType type, const void *data);
};

typedef std::map<Widget*, Element*> WEItems;

class WidgetEditorEx : public EventBox, public CallBack {
	private:
		bool asParent;
		WEItems ctrls;

		Element *getElementAtPos(gdouble x, gdouble y);
		void unBuffer(Widget *ctl);
		void addElement(Element *e);

		int getSizer(int x, int y);
		void moveSelection(int dx, int dy);
		void sizeSelection(int dx, int dy);
		Gdk::Rectangle getSelectionRect();
		/**
		 * Calculates the area selection of elements
		 * @param forDraw if true the region expands to 3 pixels
		 * @return correct selectin rectangle
		 */
		Gdk::Rectangle calcRectangle(bool forDraw = true);
		void selectWigdetsFromRect();

		void changeProperty(ElementProperty *prop);
		void invalidate() { get_window()->invalidate(true); }
	protected:
		SDK *sdk;
		int start_x;
		int start_y;
		int cur_x;
		int cur_y;

		enum MouseState { WE_MS_NONE, WE_MS_MOVE, WE_MS_RECT, WE_MS_SIZER };
		MouseState state;
		int selSizer;

		virtual bool on_button_press_event(GdkEventButton* event);
		virtual bool on_button_release_event(GdkEventButton* event);
		virtual bool on_motion_notify_event(GdkEventMotion* event);
		virtual bool on_draw (const ::Cairo::RefPtr< ::Cairo::Context >& cr);

		virtual void callback(void *owner, CallbackType type, const void *data);
	public:
		WidgetEditorEx(SDK *sdk);
		~WidgetEditorEx();

		void edit(Container *w);
};

class FE_Internal : public FormEditor {
	protected:
		WidgetEditor *weditor;
		WidgetEditorEx *we;
	public:
		void start(SDK_Editor *sde);
		void stop();
};

#endif /* FORMEDITOR_H_ */
