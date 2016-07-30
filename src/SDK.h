/*
 * SDK.h
 *
 *  Created on: 02.05.2010
 *      Author: dilma
 */

#ifndef SDK_H_
#define SDK_H_

#include "share.h"
#include "Pack.h"
#include "Element.h"
#include "SelectManager.h"

class MSDK;
class SelectManager;
class Pack;
struct ObjectType;

typedef enum {
	SDK_PERR_NONE = 0,
	SDK_PERR_SYNTAX,
	SDK_PERR_PACK_NOT_FOUND,
} SDKParseError;

//typedef std::list<Element*> ElementsList;

/*! \class SDK_Lib
    \brief container list

    Contain list of linked containers
*/

class SDK_Lib : public std::list<Element*> {
	public:
		/**
		 * Search element by ID and return it
		 * @param id identificator of searched element
		 * @return pointer to element or NULL if not search
		 */
		Element *getElementByID(int id);
		/**
		 * Add element to library with check of duplication
		 * @param element pointer to added element
		 */
		void add(Element *element);
};

/*! \class SDK
    \brief container elements of the scheme

    Provides tools for storing and managing elements scheme
*/

class SDK {
	public:
		std::list<Element*> elements;	/**< list of elements */
		Element *parent;		/**< pointer to parent element (if this SDK is child for multi element) */
		SelectManager *selMan;	/**< SelectManager required in SDK_Editor */

		Event on_redraw_rect;	/**< do when SDK require redraw rectangle */
		Event on_add_element;	/**< do when element added to SDK */
		Event on_remove_element;/**< do when element removed from SDK */

		//------------ use in sdk_editor
		gdouble pasteX;			/**< x-coordinate to insert elements from the Clipboard */
		gdouble pasteY;			/**< y-coordinate to insert elements from the Clipboard */

		gdouble scrollX;		/**< value of horizontal scrollbar */
		gdouble scrollY;		/**< value of vertical scrollbar */
		//------------------------------

		SDK();
		virtual ~SDK();

		/**
		 * Return pointer to root SDK
		 * @return root SDK
		 */
		MSDK *getMSDK();

		/**
		 * Add new element
		 * @param name internal code name from Elements.db
		 * @param x left offset
		 * @param y top offset
		 */
		Element *add(const ustring &name, gdouble x, gdouble y);
		/**
		 * Draw SDK
		 * @param dc cairo context for drawing operation
		 * @param zoom value of scaling
		 */
		void draw(DrawContext dc, double zoom);

		/**
		 * Remove all elements from SDK (do not use directly!)
		 */
		void clear();
		/**
		 * Removes one element from SDK
		 * @param element pointer to element that must be removed
		 */
		void remove(Element *element);

		/**
		 * Get full path for SDK
		 * @return full SDK path
		 */
		ustring getPath();

		/**
		 * Get element by her class name
		 * @param name - element class name
		 * @return pointer to element
		 */
		Element *getElementByName(const ustring &name);
		/**
		 * Get element by her id (search in current sdk only)
		 * @param id - element id
		 * @return pointer to element
		 */
		Element *getElementById(int id);
		/**
		 * Get element by her id (search in current sdk and in all child sdk)
		 * @param id - element id
		 * @return pointer to element
		 */
		Element *getGlobalElementById(int id);

		/**
		 * Get object by position (i.e. element, point, line, link point, ...)
		 * @param x
		 * @param y
		 * @param obj object, which is located at (x, y)
		 * @return true if the object is found, else false (equal obj.type == SDK_OBJ_NONE)
		 */
		bool getObjectAtPos(gdouble x, gdouble y, ObjectType *obj);

		/**
		 * Calculates the area occupied by all objects of the scheme
		 * @param min_x minimum x coordinate of the objects
		 * @param min_y minimum y coordinate of the objects
		 * @param max_x maximum x coordinate of the objects
		 * @param max_y maximum y coordinate of the objects
		 */
		void getSchemeGeometry(gdouble &min_x, gdouble &min_y, gdouble &max_x, gdouble &max_y);

		/**
		 * Save current SDK to text
		 * @param text saved scheme as text
		 * @param offset shift text at left edge
		 */
		void saveToText(ustring &text, ustring offset = "");
		/**
		 * Load current SDK from text
		 * @param text scheme data for loading
		 * @param line current line number (use if parse error)
		 * @param flag ELEMENT_LOAD_XXX constant
		 * @return parse error
		 */
		SDKParseError loadFromText(gchar *&text, int &line, int flag = 0);

		/**
		 * Move element to top
		 */
		void moveToTop(Element *e);
		/**
		 * Move element to down
		 */
		void moveToDown(Element *e);

		/**
		 * Prepare elements for run
		 * @param parent element fro current SDK
		 */
		void prepareForRun(Element **parent);
		/**
		 * Stop run for elements
		 */
		void stopRun();

		/**
		 * Search all managers in scheme with the name entered
		 * @param name manager name
		 * @return list of interfaces with end line delimiters
		 */
		ustring getListOfManagers(const ustring &name);
		/**
		 * Search manager of element by his name
		 * @param name
		 * @return pointer to element or NULL if it not exists
		 */
		Element *getManagerByName(ustring name);
};

/*! \class MSDK
    \brief root container for scheme

    Provides tools for working with objects, common to all containers of one scheme
*/

class MSDK : public SDK {
	private:
		int nextID;			/**< contain value of next unique ID */
		bool running;		/**< contain SDK run state */
	protected:
	public:
		Pack *pack;			/**< base Pack of current scheme */
		PackProject *packProject;	/**< base Project of current scheme */
		ustring info;		/**< user information about this scheme */
		ustring fileName;	/**< name of loaded file */
		SDK_Lib sdkLib;		/**< list of linked elements */

		Event on_run_state;	/**< emit when call run() or stop() methods */

		MSDK();
		~MSDK();

		/**
		 * Save SDK to file
		 * @param file_name name of file
		 * @return true if done, false - error
		 */
		bool saveToFile(const ustring &file_name);
		/**
		 * Load SDK from file
		 * @param file_name name of file
		 * @return true if done, false - error
		 */
		bool loadFromFile(const ustring &file_name);
		/**
		 * Get dir of current project
		 * @return path to project dir
		 */
		ustring projectDir();
		/**
		 * Generate next unique ID for all project
		 * @return unique ID
		 */
		int genNextID();

		/**
		 * Run internal core elements
		 * @param as_app run scheme as application
		 */
		void run(bool as_app = true);
		/**
		 * Stop internal core elements
		 */
		void stop();
		/**
		 * Check run state of current SDK
		 * @return true if run
		 */
		inline bool isRun() { return running; }
};

#endif /* SDK_H_ */
