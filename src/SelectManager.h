/*
 * SelectManager.h
 *
 *  Created on: 05.05.2010
 *      Author: dilma
 */

#ifndef SELECTMANAGER_H_
#define SELECTMANAGER_H_

#include "share.h"
#include "SDK.h"
#include "Element.h"

class SDK;
class Element;

typedef std::list<Element*> ElementsList;

/*! \class SelectManager
    \brief control selected elements

    Tools for manage selected elements of the scheme in SDK
*/

class SelectManager {
	private:
		/**
		 * Request to redraw the SDK area
		 */
		void reDraw(const Gdk::Rectangle &r);
		/**
		 * Request to redraw full SDK
		 */
		void reDrawAll();

		/**
		 * Receive data from clipboard
		 */
		virtual void getClipboardText(const ustring &text);
	public:
		/**
		 * SelectByEdge types for commands SlideTo
		 */
		enum SBE_Types{SBE_EDGE_VERTICAL, SBE_EDGE_HORIZONTAL, SBE_EDGE_BOTH};

		SDK *sdk;					/**< pointer to parent SDK */
		ElementsList elements;		/**< list of selected elements */
		bool drawEnable;			/**< enable or disable drawing event */
		Event on_selection_change;	/**< call when elements has been changed */

		SelectManager(SDK *sdk);

		/**
		 * Add element in selection list
		 * @param element want to add
		 */
		void add(Element *element);
		/**
		 * Clear selection list and call add(element)
		 * @param element want to select
		 */
		void select(Element *element);
		/**
		 * Select all the elements in rectangle
		 * @param x1 left edge of area
		 * @param y1 top edge of area
		 * @param x2 right edge of area
		 * @param y2 bottom edge of area
		 */
		void selectRect(gdouble x1, gdouble y1, gdouble x2, gdouble y2);
		/**
		 * Select all elements by edge
		 */
		void selectByEdge(SBE_Types mode, gdouble x, gdouble y);
		/**
		 * Clear selection list
		 */
		void clear();
		/**
		 * Shift the selected elements on the (dx, dy)
		 * @param dx offset by x axis
		 * @param dy offset by y axis
		 */
		void move(gdouble dx, gdouble dy);
		/**
		 * Delete selected elements from SDK
		 */
		void erase();
		/**
		 * Get rectangle area of selected elements
		 * @return rectangle areas
		 */
		Gdk::Rectangle getSelectionRect();
		/**
		 * Determines the availability of removal selected items
		 * @return true if can, false if IS_PARENT in list
		 */
		bool canDelete();
		/**
		 * Check elements count
		 * @return true if elements count less then 0, else false
		 */
		inline bool empty() { return elements.empty(); }
		/**
		 * Get first element from selection list
		 * @return Element if count == 1, else NULL
		 */
		Element *getFirst();
		/**
		 * Checks the flag for all elements
		 * @param flag value from define ELEMENT_FLG_XXX
		 * @return true if all elements comtain this flag? else false
		 */
		bool checkFlag(int flag);
		/**
		 * Set flag for all elements
		 * @param flag value from define ELEMENT_FLG_XXX
		 */
		void setFlag(int flag);
		/**
		 * Unset flag for all elements
		 * @param flag value from define ELEMENT_FLG_XXX
		 */
		void unsetFlag(int flag);
		/**
		 * Remove links from all points in selection list
		 */
		void clearLinks();
		/**
		 * Select all elements from parent SDK
		 */
		void selectAll();
		/**
		 * Copy selected elements to clipboard
		 */
		void copy();
		/**
		 * Paste elements from clipboard
		 */
		void paste();
		/**
		 * Copy selected element as link to clipboard
		 */
		void copyAsLink();
};

#endif /* SELECTMANAGER_H_ */
