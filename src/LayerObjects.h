/*
 * LayerObjects.h
 *
 *  Created on: 18.05.2010
 *      Author: dilma
 */

#ifndef LAYEROBJECTS_H_
#define LAYEROBJECTS_H_

#include "share.h"
#include "Element.h"
#include "ElementPoint.h"

/*! \class LayerObject
    \brief SDK_Editor draw objects

    Base class for objects are displayed in the editor over the scheme
*/

class LayerObject {
	private:
		guint timerID;	/**< timer ID for show object by delay */
	protected:
	public:
		gdouble x;	/**< x coordinate of the object */
		gdouble y;	/**< y coordinate of the object */
		int width;	/**< width of the object */
		int height;	/**< height of the object */
		Widget *parent;	/**< pointer to parent widget */

		bool dead;		/**< true if this object need to destroy */
		bool visible;	/**< true if this object visible in SDK_Editor */

		LayerObject(Widget *parent);
		virtual ~LayerObject();

		/**
		 * Call when object need draw in SDK_Editor
		 * @param dc context to draw
		 */
		virtual void draw(DrawContext dc) {}
		/**
		 * Call when user click in object area
		 * @param x coordinate of the mouse pointer
		 * @param y coordinate of the mouse pointer
		 * @param button of the mouse
		 * @return true if you want to continue processing the event, false - break process
		 */
		virtual bool downProc(gdouble x, gdouble y, int button) { return true; }
		/**
		 * Call when user moves the mouse over area of object
		 * @param x coordinate of the mouse pointer
		 * @param y coordinate of the mouse pointer
		 * @return true if you want to continue processing the event, false - break process
		 */
		virtual bool moveProc(gdouble x, gdouble y) { return true; }
		/**
		 * Call when user mouse up
		 * @param x coordinate of the mouse pointer
		 * @param y coordinate of the mouse pointer
		 * @param button of the mouse
		 * @return true if you want to continue processing the event, false - break process
		 */
		virtual bool upProc(gdouble x, gdouble y, int button) { return true; }
		/**
		 * Call when mouse leaves the area of object
		 */
		virtual void enterProc() {}
		/**
		 * Call when mouse enter in the area of object
		 */
		virtual void leaveProc() {}

		/**
		 * Checks the mouse pointer location in the object area
		 * @return true if it is
		 */
		bool inRect(gdouble mx, gdouble my);
		/**
		 * Update hint rectangle
		 */
		void invalidate();

		/**
		 * Show object in SDK_Editor
		 * @param delay if set object show by timer
		 */
		virtual void show(gdouble x, gdouble y, bool delay = false);

		/**
		 * Get rect of hint
		 * @return rect
		 */
		Gdk::Rectangle getRect() { return Gdk::Rectangle(x-2, y-2, width+4, height+4); }
};

typedef std::list<LayerObject*> LayerObjects;

/*! \class LO_PointHint
    \brief hint for element point

    Displays information about the point - the type, name, description, arguments
*/

class LO_PointHint : public LayerObject {
	private:
		Glib::RefPtr<Gdk::Pixbuf> typeIcon;
		std::list<ustring> info;
		int type;

		void drawHintRect(DrawContext dc);
	public:
		LO_PointHint(ElementPoint *point, double zoom, Widget *parent);
		~LO_PointHint();

		void draw(DrawContext dc);
};

/*! \class LO_ElementHint
    \brief hint for element

    Displays information about the element
*/

class LO_ElementHint : public LayerObject {
	private:
		std::list<ustring> info;

		void drawHintRect(DrawContext dc);
	public:
		LO_ElementHint(Element *e, double zoom, Widget *parent);

		void draw(DrawContext dc);
		void show(gdouble x, gdouble y, bool delay = false);
};

#endif /* LAYEROBJECTS_H_ */
