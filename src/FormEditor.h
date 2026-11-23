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
#include <cstdint>
#include <vector>

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
		virtual ~FormEditor() = default;

		/**
		 * Abstract method for initialize and show form editor
		 * @param sde pointer to parent SDK_Editor widget
		 */
		virtual void start(SDK_Editor *sde) { this->sde = sde; }
		/**
		 * Abstract method for close and destroy form editor
		 */
		virtual void stop() = 0;
		virtual bool isActive() const { return true; }
};

class FE_ExternalTcp : public FormEditor, public CallBack {
	private:
		GSocket *sock = nullptr;
		GSocket *client = nullptr;
		GSubprocess *helper = nullptr;
		guint timer = 0;
		gint64 deadline = 0;
		bool subscribed = false;
        guint16 requestedPort = 4588;
        guint16 boundPort = 0;
		std::vector<char> incoming, outgoing;
		bool poll();
		bool sendBytes(const void *data, size_t size);
		bool applyPacket(int command, const char *data);
		void pk_add_element(Element *e);
		void pk_send(int num, void *data, int size);
		void pk_send_prop(ElementProperty *prop);
		void pk_send_remove(Element *e);
		void pk_send_select(SelectManager *sm);

		void callback(void *owner, CallbackType type, const void *data);
	public:
		explicit FE_ExternalTcp(guint16 port = 4588):requestedPort(port) {}
        guint16 port() const { return boundPort; }
		~FE_ExternalTcp() override { stop(); }
		bool isActive() const override { return timer != 0; }

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

class ElementTplWinControl;

typedef std::map<Widget*, Element*> WEItems;

/*! \class WidgetEditorEx
    \brief edit position and geometry of widgets

    Provides tools for editing positions and sizes of visual elements in the form
    (replaces the legacy WidgetEditor/WidgetInfo snapshot-based editor)
*/

class WidgetEditorEx : public EventBox, public CallBack {
	private:
		static WidgetEditorEx *activeInstance;
		bool asParent;
		Element *parentForm;
		/** Client area of the form (parentForm->ctl); the controls are placed on it. */
		Gtk::Fixed *formSurface;
		/** Window of the form: a title bar (icon, Caption and window buttons) above the client area. */
		Gtk::Box *formFrame;
		Gtk::Box *formTitle;
		/** Field of a panel (the scheme of ChildPanel): the controls are placed on it; its size is the panel's Width x Height. */
		Gtk::Fixed *panelField;
		Glib::RefPtr<Gtk::CssProvider> panelCss;
		void applyPanelSize(Element *panel);
		/** Prepares the widget of an element for the editor; parent is the element that holds it. */
		void prepareControl(Element *e, ElementTplWinControl *ew, Element **parent);
		/** Puts the widget into the field of its panel or into the client area of its form, at its place. */
		void placeControl(Element *e, ElementTplWinControl *ew);
		Gtk::Label *formTitleText;
		Gtk::Button *formMinimize, *formMaximize;
		void buildFormFrame();
		Glib::RefPtr<Gtk::CssProvider> formCss;
		void applyFormStyle(Element *form);
		SDK_Editor *sde;
		ustring pendingInsertName;
		WEItems ctrls;
		Gdk::Rectangle widgetRectangle(Widget *widget);
		bool drawSelection(const Cairo::RefPtr<Cairo::Context> &cr);

		Element *getElementAtPos(gdouble x, gdouble y);
		void unBuffer(Widget *ctl);
		void addElement(Element *e);

		int getSizer(int x, int y);
		void moveSelection(int dx, int dy);
		void snapMovement(int &dx, int &dy, bool resizing = false);
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
		void invalidate() { widget_invalidate(*this); }
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
#ifdef HIASM_GTK4
		void on_draw_overlay(const Cairo::RefPtr<Cairo::Context> &cr) override { drawSelection(cr); }
		bool has_draw_overlay() const override { return true; }
#endif

		virtual void callback(void *owner, CallbackType type, const void *data);
	public:
		WidgetEditorEx(SDK *sdk, SDK_Editor *sde = NULL);
		~WidgetEditorEx();

		static WidgetEditorEx *getActive(SDK_Editor *editor) {
			return activeInstance && activeInstance->sde == editor ? activeInstance : NULL;
		}
		void beginInsert(const ustring &name);

		void edit(Container *w);
};

class FE_Internal : public FormEditor {
	protected:
		WidgetEditorEx *we;
	public:
		void start(SDK_Editor *sde);
		void stop();
};

/*! \class FE_Legacy
    \brief form editor of external HiAsm 4 packages

    HiAsm 4 had the designer built into HiAsm.exe (no make/formedit.exe), so
    this editor shows the form ([Edit] Class=Form) as a separate window with
    the frame selected by its BorderStyle (the KOL table of hiMainForm.pas).
    On Windows it is a plain Win32 window: GTK resets the style bits of its
    own windows on every layout. The editor
    draws the controls in classic Windows style and lets the user select, move
    and resize them. Moving or resizing the window changes Left/Top/Width/Height.
*/
class FE_Legacy : public EventBox, public FormEditor, public CallBack {
	private:
		struct Item { Element *element; Gdk::Rectangle rect; };
		enum Drag { DragNone, DragMove, DragSize, DragBand };
		static FE_Legacy *activeInstance;
		Gtk::Window *window = nullptr;
		Element *form = nullptr;
		SDK *sdk = nullptr;
		Drag drag = DragNone;
		int grip = -1;
		int startX = 0, startY = 0, curX = 0, curY = 0;
		double zoom = 1;			/**< form pixels (96 DPI) -> widget pixels */
		bool syncing = false;		/**< writing window geometry to the form */
		guint closeIdle = 0, syncIdle = 0;
		void *hwnd = nullptr;		/**< HWND of the form window (Win32) */
		int hoverGrip = -1;

		std::vector<Item> computeLayout();
		Element *elementAt(double x, double y);
		Gdk::Rectangle selectionRect();
		int gripAt(double x, double y);
		void shiftSelection(const char *xName, const char *yName, int dx, int dy, int minimum);
		void drawControl(const Cairo::RefPtr<Cairo::Context> &cr, const Item &item);
		void drawSelection(const Cairo::RefPtr<Cairo::Context> &cr, const std::vector<Item> &items);
		void paint(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height);
		void invalidate();
		void applyFrame();
		void updateZoom();
		void createNative(void *owner);
		void scheduleClose();
		void subscribe(Element *element) { element->on_property_change -= this; element->on_property_change += this; }

		void callback(void *owner, CallbackType type, const void *data) override;
	public:
		FE_Legacy() {}
		~FE_Legacy() override { stop(); }

		void start(SDK_Editor *sde) override;
		void stop() override;
		bool isActive() const override { return window != nullptr || hwnd != nullptr; }

		/** Form element of a HiAsm 4 scheme ([Edit] Class=Form), or NULL */
		static Element *findForm(SDK *sdk);
		static FE_Legacy *getActive(SDK_Editor *editor) {
			return activeInstance && activeInstance->sde == editor ? activeInstance : nullptr;
		}
		/** GTK window of the form (non-Win32 builds) */
		Gtk::Window *formWindow() const { return window; }
		/** HWND of the form (Win32 builds) */
		void *nativeWindow() const { return hwnd; }
		Element *formElement() const { return form; }
		ustring windowTitle() const;
		double scale() const { return zoom; }
		/** Called on WM_EXITSIZEMOVE: copy the window geometry to the form */
		void queueSync();
		void syncFromWindow();
		/** Window procedure of the Win32 form window */
		intptr_t nativeMessage(void *window, unsigned msg, uintptr_t wParam, intptr_t lParam);

		bool on_button_press_event(GdkEventButton *event) override;
		bool on_button_release_event(GdkEventButton *event) override;
		bool on_motion_notify_event(GdkEventMotion *event) override;
		bool on_draw(const Cairo::RefPtr<Cairo::Context> &cr) override;
};

#endif /* FORMEDITOR_H_ */
