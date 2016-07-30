/*
 * ElementPoint.h
 *
 *  Created on: 17.05.2010
 *      Author: dilma
 */

#ifndef ELEMENTPOINT_H_
#define ELEMENTPOINT_H_

#include "Element.h"

// point flags
#define POINT_FLG_IS_SELECT 0x01

#define POINT_OFF 3
#define POINT_SPACE 7

/*! \struct PointPos
    \brief contain point path

    Structure for storing data on the link between the two points
*/

struct PointPos {
	gdouble x;		/**< x position of the path point */
	gdouble y;		/**< y position of the path point */
	PointPos *next;	/**< pointer to next point in path */
	PointPos *prev;	/**< pointer to previous point in path */
};

/*! \class ElementPoint
    \brief element point

    Provides methods for working with the point element
*/

class Element;

class ElementPoint {
	private:
		/**
		 * Shift all point link by (dx, dy)
		 */
		void movePoints(PointPos *pb, int dx, int dy);
		/**
		 * Move link point into position (x, y)
		 */
		void movePos(PointPos *pb, int x, int y);
	public:
		PointPos *pos;			/**< Position of the element point */
		ustring name;			/**< Point name */
		ustring info;			/**< Point description */
		int type;				/**< Point type (pt_work, pt_event, pt_var, pt_data) */
		int dataType;			/**< Point data type (i.e data_int, data_str, data_real, ...) */
		Element *parent;		/**< Pointer to parent element */
		ElementPoint *point;	/**< Pointer to linked point (NULL by default) */
		int flag;				/**< Flags POINT_FLG_XXX */

		ElementPoint(Element *parent, const ustring &name, const ustring &info, int type);
		virtual ~ElementPoint();

		/**
		 * Link two point without path trace
		 * @param link connect point
		 * @return this point
		 */
		ElementPoint *connect(ElementPoint *link);
		/**
		 * Check available connection of two points
		 * @param link connect point
		 * @return TRUE if this point and link can be connected
		 */
		bool canConnect(ElementPoint *link);
		/**
		 * Remove link. Call reDraw for parent SDK
		 */
		void clear();
		/**
		 * Create path between two points (only use after connect method). Call reDraw for parent SDK
		 */
		void createPath();
		/**
		 * Remove point from link between two element points. Call reDraw for parent SDK
		 * @param lp point, who need remove
		 */
		void removeLinePoint(PointPos *lp);
		/**
		 * Move point of link to position (x,y). Call reDraw for parent SDK
		 * @param lp point, who need move
		 * @param x
		 * @param y
		 */
		void moveLinePoint(PointPos *lp, gdouble x, gdouble y);
		/**
		 * Add a point to link in the position (x,y)
		 * @param lp point after which you must add a new
		 * @param x
		 * @param y
		 * @return adding point
		 */
		static PointPos *addLinePoint(PointPos *lp, gdouble x, gdouble y);

		/**
		 * Check it primary point (pt_event or pt_data)
		 * @return TRUE if point type is pt_event or pt_data, else FALSE
		 */
		inline bool isPrimary() { return type % 2 == 0; }

		/**
		 * Check flag POINT_FLG_IS_SELECT
		 * @return true if flag is set
		 */
		inline bool isSelect() { return flag&POINT_FLG_IS_SELECT; }

		/**
		 * Move point to the specified offset
		 * @param dx offset by x
		 * @param dy offset by y
		 */
		void move(gdouble dx, gdouble dy);
		/**
		 * Calculate region rendering point
		 * @return rectangle area
		 */
		Gdk::Rectangle drawRect();
		/**
		 * Invalidate parent SDK_Editor drawRect for this point
		 */
		void invalidate();
		/**
		 * Store link data to string in format: (x1,y1),(x2,y2),...,(xN,yN)
		 * @return formatted value of the link data
		 */
		ustring serializePath();

		/**
		 * Get real point wo linked code elements (like hub, getdata and other)
		 * @return linked point or this if not found
		 */
		ElementPoint *getRealPointWithPath();

		/**
		 * Get the type of pair point to connect
		 * @return value pt_xxx
		 */
		inline int getPair() { return type + (isPrimary() ? -1 : 1); }
		/**
		 * Get the direction of point
		 * @return 1 for work-event link and 0 for var-data
		 */
		inline int getDirection() { return (type == pt_event || type == pt_work); }

		// ----------- core communication ------------------------
		void on_event(TData *data);
		void get_data(TData *data);

		void on_event() { on_event((TData *)NULL); }
		void on_event(const ustring &value) { TData dt(value); on_event(&dt); }
		void on_event(int value) { TData dt(value); on_event(&dt); }
		void on_event(double value) { TData dt(value); on_event(&dt); }
};

#endif /* ELEMENTPOINT_H_ */
