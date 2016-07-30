/*
 * Element.h
 *
 *  Created on: 02.05.2010
 *      Author: dilma
 */

#ifndef ELEMENT_H_
#define ELEMENT_H_

#include "share.h"
#include "SDK.h"
#include "Pack.h"
#include "Data.h"
#include "ElementPoint.h"
#include "ElementProperty.h"

#define ICON_SIZE 24

#define ECL_LIGHT  0.2
#define ECL_DARK   (-0.2)

#define GRID_X(x) ((int)((x) / POINT_SPACE) * POINT_SPACE)
#define GRID_Y(y) ((int)((y) / POINT_SPACE) * POINT_SPACE)
#define GRID_FRM_X(x) ((int)((x) / 5) * 5)
#define GRID_FRM_Y(y) ((int)((y) / 5) * 5)

// element flags
#define ELEMENT_FLG_IS_SELECT	0x01
#define ELEMENT_FLG_IS_NOMOUSE	0x02
#define ELEMENT_FLG_IS_PARENT	0x04
#define ELEMENT_FLG_IS_CORE		0x08
#define ELEMENT_FLG_IS_FREEZE	0x10
#define ELEMENT_FLG_IS_NODELETE	0x20
#define ELEMENT_FLG_ONE_WIDGET	0x80
#define ELEMENT_FLG_IS_SYSTEM	0x400

#define ELEMENT_SAVE_CHANGED   0x01
#define ELEMENT_SAVE_SELECTED  0x02

#define ELEMENT_LOAD_FILE   0x01
#define ELEMENT_LOAD_PASTE  0x02

extern Gdk::RGBA lightColor;
extern Gdk::RGBA darkColor;
extern Gdk::RGBA hintColor;

extern TypePixbuf (*stockRender)(const ustring &name);

class SDK;
class Element;
class ElementPoint;
class LinkHint;

struct PackElement;
struct PointPos;

/*! \struct ObjectType
    \brief SDK object type

    Describes the object of the scheme, selected in the editor
*/

typedef enum {
	SDK_OBJ_NONE,
	SDK_OBJ_ELEMENT,
	SDK_OBJ_POINT,
	SDK_OBJ_LINKPOINT,
	SDK_OBJ_LINE,
	SDK_OBJ_LHINT,
	SDK_OBJ_COUNT
} SDKObjectType;

struct ObjectType {
	SDKObjectType type;
	union {
		void *obj1;
		Element *element;
		ElementPoint *point;
		LinkHint *hint;
	};
	union {
		void *obj2;
		PointPos *pos;
	};
	ObjectType() {
		type = SDK_OBJ_NONE;
	}
	ObjectType(Element *obj) {
		element = obj;
		type = SDK_OBJ_ELEMENT;
	}
	ObjectType(ElementPoint *obj) {
		point = obj;
		type = SDK_OBJ_POINT;
	}
	ObjectType(PointPos *obj, ElementPoint *point) {
		pos = obj;
		this->point = point;
		type = SDK_OBJ_LINKPOINT;
	}
	ObjectType(LinkHint *obj) {
		hint = obj;
		type = SDK_OBJ_LHINT;
	}

	bool operator != (const ObjectType &obj) {
		return type != obj.type || obj1 != obj.obj1;
	}
};

class ElementProperty;

class ElementPoints : public std::vector<ElementPoint*> {
	public:
		ElementPoint *getByName(const ustring &name);
		int indexOf(ElementPoint *point);
		int indexOfType(ElementPoint *point);
};


/*! \class ElementProperties
    \brief Base class for element properties

    Provides methods to control properties of element
*/

class ElementProperties : public std::vector<ElementProperty*> {
	public:
		/**
		 * Search property by name
		 * @param name codename of property
		 * @return pointer to ElementProperty or NULL if not found
		 */
		ElementProperty *getByName(const ustring &name);
		/**
		 * Returns pointer to property with typed point name
		 * @param name point name
		 * @return property if found, else NULL
		 */
		ElementProperty *getByPointName(const ustring &name);
		/**
		 * Call "change" event for all properties
		 */
		void change();
		/**
		 * Destroy all items and clear list
		 */
		void free();
		/**
		 * Change parent for all propertyes
		 * @param parent pointer to new parent element
		 */
		void setParent(Element *parent);

};

/*! \class LinkHint
    \brief linked hints

    Provides tools for creating and managment linked hints
*/

class LinkHint {
	private:
		ustring text;			/**< text to display on hint */
		Glib::RefPtr<Pango::Layout> textLayout;
	public:
		int x, y;				/**< relative position of the hint */
		int width, height;		/**< linear size */
		Element *parent;		/**< owner element */
		ElementProperty *prop;	/**< linked property */

		LinkHint(Element *parent) { x = 0; y = 0; prop = NULL; this->parent = parent; }

		void draw(DrawContext dc);
		Gdk::Rectangle drawRect();

		void updateText();
		void setWidth(int value);
};

class LinkHints : public std::list<LinkHint*> {
	public:
		void restore(int x, int y, int width, int height, ElementProperty *prop);
		void draw(DrawContext dc);
		void free();
		Gdk::Rectangle drawRect();
		void saveToText(ustring &text, const ustring offset);
		bool getObjectAtPos(gdouble x, gdouble y, ObjectType *obj);
		void change(ElementProperty *prop);
};


/*! \struct SysColors
    \brief system colors

    Provides information about system colors
*/

struct SysColors {
	bool load;
	Gdk::RGBA btnFace;
	Gdk::RGBA btnLight;
	Gdk::RGBA btnDark;

	SysColors() { load = false; }
	void init(Widget *w) {
		Glib::RefPtr<Gtk::StyleContext> s = w->get_style_context();
		
		btnFace.set("#e5ddcf"); // = s->get_background_color(Gtk::STATE_FLAG_NORMAL);
		btnLight.set("#ffffff"); //= s->get_color(Gtk::STATE_FLAG_NORMAL);
		btnDark.set("#b2aa9c"); //= s->get_color(Gtk::STATE_FLAG_NORMAL);
		load = true;
	}
};

extern SysColors sysColors;

/*! \class Element
    \brief Base class for element scheme

    Provides methods to control and display elements in the scheme
*/

class Element {
	private:
		/**
		 * Create point and properties from element template
		 */
		void buildFromTemplate();
		/**
		 * Change element color
		 */
		void changeColor();
	protected:
		friend class ElementProperty;
		friend class ElementProperties;

		Gdk::RGBA *color, colorLight, colorDark;

		int minWidth;	/**< minimum value of width */
		int minHeight;	/**< minimum value of height */

		ElementProperty *viewProp;	/**< pointer to property who display instead of icon*/
		ustring viewText;			/**< cached text string from viewProp property*/

		/**
		 * Create text string by viewProp
		 */
		void initView();
		/**
		 * Notify about property change
		 * @param prop pointer to ElementProperty
		 */
		virtual void on_change_property(ElementProperty *prop);
		/**
		 * Scaling the size of the element depending on the number of its points
		 */
		virtual void reSize();

		/**
		 * Insert element between two points
		 * @param pl1 first point (pl1->point is second point)
		 * @param pl2 first element point
		 * @param pl3 second element point
		 * @param p1 first PointPos structure (p1->next is second PointPos structure)
		 */
		void injectElementAtLine(ElementPoint *pl1, ElementPoint *pl2, ElementPoint *pl3, PointPos *p1);
	public:
		int id;		/**< unique id of element */
		int x;		/**< x position on SDK */
		int y;		/**< y position on SDK */
		int width;	/**< size of element */
		int height;	/**< size of element */
		PackElement *tpl;			/**< pointer to element template (parsed *.ini config file) */
		SDK *sdk;					/**< pointer to child SDK (for multi element only) */
		SDK *parent;				/**< pointer to parent SDK */
		ElementPoints points;		/**< array of points */
		ElementProperties props;	/**< array of properties */
		ElementProperties sysProps;	/**< array of system properties */
		LinkHints hints;			/**< array of linked hints */
		int flag;					/**< flags */
		int cIndex;					/**< class index */
		std::list<Element*> *elinkList;	/**< list of linked elements */

		ustring codeName;	/**< code name used in CodeGen */
		void *userData;		/**< user data used in FTCG codegen */

		Event on_property_change;	/**< call when property changed */

		Element(PackElement *pe, SDK *sdk, gdouble x, gdouble y);
		virtual ~Element();

		/**
		 * Draw element. Call drawBody(), drawIcon(), drawPoints()
		 * @param dc cairo context to draw
		 * @param zoom value of scaling
		 */
		virtual void draw(DrawContext dc, double zoom);
		/**
		 * Draw element area rectangle
		 * @param dc cairo context to draw
		 * @param zoom value of scaling
		 */
		virtual void drawBody(DrawContext dc, double zoom);
		/**
		 * Draw element icon
		 * @param dc cairo context to draw
		 */
		virtual void drawIcon(DrawContext dc);
		/**
		 * Draw element points without links
		 * @param dc cairo context to draw
		 */
		virtual void drawPoints(DrawContext dc);
		/**
		 * Draw links of element points
		 * @param dc cairo context to draw
		 */
		virtual void drawLinks(DrawContext dc);

		/**
		 * Add point to element
		 * @param name internal point name
		 * @param info short description of the point
		 * @param type constant pt_xxx
		 * @return created point
		 */
		ElementPoint *addPoint(const ustring &name, const ustring &info, int type);
		/**
		 * Remove point from element
		 * @param point pointer to ElementPoint
		 */
		void removePoint(ElementPoint *point);
		/**
		 * Placement of the element points at its edges, depending on their types
		 */
		virtual void rePosPoints();
		/**
		 * Placement of the element points at its edges, depending on their types without size calculate
		 */
		virtual void rePosPointsWOsize();

		/**
		 * Remove links from all points
		 */
		void clearLinks();

		/**
		 * Shift element by (dx, dy)
		 * @param dx offset at left edge
		 * @param dy offset at top edge
		 */
		virtual void move(gdouble dx, gdouble dy);

		/**
		 * Call before remove element from SDK
		 */
		virtual void remove();
		/**
		 * Restore link from first linked work/var element point to first linked event/data point
		 * @param we restore work-event link
		 * @param vd restore var-data link
		 */
		void removeFromLine(bool we, bool vd);

		/**
		 * Get object by position (i.e. element, point, line, link point, ...)
		 * @param x position
		 * @param y position
		 * @param obj return object at position (x, y)
		 * @return true if object found
		 */
		virtual bool getObjectAtPos(gdouble x, gdouble y, ObjectType *obj);
		/**
		 * Get pointer of point who contain line at position (x,y)
		 * @param x position
		 * @param y position
		 * @param obj return object at position (x, y)
		 */
		bool findLine(gdouble x, gdouble y, ObjectType *obj);
		/**
		 * Check element hit at pos (x,y)
		 * @param x value by x axis
		 * @param y value by y axis
		 * @return true if element exists in (x,y)
		 */
		virtual bool checkCollise(gdouble x, gdouble y);
		/**
		 * Check element hit at rect (x1, y2, x2, y2)
		 */
		virtual bool checkColliseRect(gdouble x1, gdouble y1, gdouble x2, gdouble y2);

		virtual bool mouseDown(gdouble x, gdouble y, int button) { return false; }
		virtual void mouseMove(gdouble x, gdouble y) {}
		virtual void mouseUp(gdouble x, gdouble y, int button) {}
		virtual int mouseCursor(gdouble x, gdouble y) { return 0; }

		/**
		 * Insert element at line between two points
		 * @param point for which is inserted
		 * @param pos nearest point of bending on the line
		 * @param ix shift to the left for the newly inserted element
		 * @param iy shift to the top for the newly inserted element
		 */
		virtual void insertInLine(ElementPoint *point, PointPos *pos);
		/**
		 * Connect point to element
		 * @param point who connect
		 */
		virtual void connectToPoint(ElementPoint *point);

		/**
		 * Place element on SDK from editor
		 */
		virtual void place(int x, int y) {}

		/**
		 * Get element rectangle for reDraw operation include all points and linked hint
		 * @return element rectangle
		 */
		virtual Gdk::Rectangle drawRect();

		/**
		 * Invalidate parent SDK_Editor drawRect for this element
		 */
		void invalidate();

		/**
		 * Returns the first free point of this type
		 * @param type point type (constants pt_xxx)
		 * @return point if found, else NULL
		 */
		ElementPoint *getFirstFreePoint(int type);
		/**
		 * Returns the last free point of this type
		 * @param type point type (constants pt_xxx)
		 * @return point if found, else NULL
		 */
		ElementPoint *getLastFreePoint(int type);
		/**
		 * Returns the last point of this type
		 * @param type point type (constants pt_xxx)
		 * @return point if found, else NULL
		 */
		ElementPoint *getLastPoint(int type);
		/**
		 * Returns pointer to point with typed name
		 * @param name point code name
		 * @return point if found, else NULL
		 */
		inline ElementPoint *getPointByName(const ustring &name) { return points.getByName(name); }
		/**
		 * Calculates the number of points of this type
		 * @param type point type (constants pt_xxx)
		 * @return points count of this type
		 */
		int getPointsCount(int type);
		/**
		 * Returns a connected point for this element, transparent for the code generation
		 * @param point pointer to this element point
		 * @return connected point
		 */
		virtual ElementPoint *getRealPoint(ElementPoint *point) { return point; }

		/**
		 * Call after load element from text or file
		 */
		virtual void init() { changeColor(); }

		/**
		 * Returns user name of element
		 * @return name of element
		 */
		virtual ustring getName() { ustring s = getHint(); return s.empty() ? tpl->name : s; }
		/**
		 * Returns element description
		 * @return description
		 */
		ustring getHint();

		/**
		 * Save element settings to text
		 * @param text save storage
		 * @param offset from the beginning of the line
		 * @param flags save parameters from ELEMENT_SAVE_XXX
		 */
		virtual void saveToText(ustring &text, const ustring offset, int flags);
		/**
		 * Save link of element as text
		 * @param text save storage
		 */
		virtual void saveAsLink(ustring &text);
		/**
		 * Save element properties to text
		 * @param text save storage
		 * @param offset from the beginning of the line
		 * @param flags save parameters (ELEMENT_SAVE_CHANGED)
		 */
		virtual void saveParamsToText(ustring &text, const ustring offset, int flags);
		/**
		 * Save system element properties to text
		 * @param text save storage
		 * @param offset from the beginning of the line
		 * @param flags save parameters (ELEMENT_SAVE_CHANGED)
		 */
		virtual void saveSysParamsToText(ustring &text, const ustring offset, int flags);
		/**
		 * Save element points to text
		 * @param text save storage
		 * @param offset from the beginning of the line
		 * @param flags save parameters (ELEMENT_SAVE_XXX)
		 */
		virtual void savePointsToText(ustring &text, const ustring offset, int flags);

		/**
		 * Get element with index 1 from child SDK if exists
		 * @return parent element if SDK is created? else NULL
		 */
		Element *getParentElement();

		/**
		 * Check providing an element of this interface
		 * @param name interface name
		 * @return true if element provide interface
		 */
		bool checkInterfaceProvide(const ustring &name);

		/**
		 * Create owner for list of linked elements
		 */
		void createMainELink();
		/**
		 * Add this element to linked list
		 * @param owner of linked elements list
		 */
		virtual void createELink(Element *owner);
		/**
		 * Check linked state for this element
		 * @return true if element contain in linked list
		 */
		inline bool isELink() { return elinkList != NULL; }
		/**
		 * Check owner of linked list for this element
		 * @return true if this element is owner of linked list
		 */
		bool isMainELink() { return isELink() && *elinkList->begin() == this; }
		/**
		 * Return owner of linked list
		 * @return first item from linked list
		 */
		inline Element *getMainELink() { return *elinkList->begin(); }

		/**
		 * Update pointers to element propertyes
		 */
		virtual void storePropPointers();

		/**
		 * Check flag ELEMENT_FLG_IS_SELECT
		 * @return true if set
		 */
		inline bool isSelect() { return flag & ELEMENT_FLG_IS_SELECT; }
		/**
		 * Check flag ELEMENT_FLG_IS_PARENT
		 * @return true if set
		 */
		inline bool isParent() { return flag & ELEMENT_FLG_IS_PARENT; }
		/**
		 * Check flag ELEMENT_FLG_IS_CORE
		 * @return true if set
		 */
		inline bool isCore() { return flag & ELEMENT_FLG_IS_CORE; }
		/**
		 * Check flag ELEMENT_FLG_IS_FREEZE
		 * @return true if set
		 */
		inline bool isFreeze() { return flag & ELEMENT_FLG_IS_FREEZE; }
		/**
		 * Check flag ELEMENT_FLG_IS_NODELETE and ELEMENT_FLG_IS_PARENT
		 * @return true if set
		 */
		inline bool isNoDelete() { return flag & (ELEMENT_FLG_IS_NODELETE | ELEMENT_FLG_IS_PARENT); }
};

#endif /* ELEMENT_H_ */
