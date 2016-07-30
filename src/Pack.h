/*
 * Pack.h
 *
 *  Created on: 03.05.2010
 *      Author: dilma
 */

#ifndef PACK_H_
#define PACK_H_

#include "share.h"
#include "sqlite3.h"
#include "ElementConfig.h"
#include "Compiler.h"

class ElementConfig;

struct PackElement {
	ustring name;
	ustring info;
	Glib::RefPtr<Gdk::Pixbuf>  image;
	ElementConfig *conf;
	//----- use palette ----
//	Image *img;
};

typedef struct {
	ustring name;
	std::list<PackElement*> elements;
} PackGroup;

/*! \class PackProjects
    \brief access to hiasm project

    Provides tools for working with the installed pack projects
*/

class Pack;

class PackProject {
	public:
		ustring name;	/**< project code name */
		ustring info;	/**< project description */
		ustring entry;	/**< main element(parent) for this project */
		ustring ext;	/**< result extension of application */
		ustring lib;	/**< module who build this project */
		Pack *pack;		/**< contain pointer to owner pack */

		PackProject(Pack *parent) { pack = parent; }

		/**
		 * Provide information about the project template
		 * @return full template filename
		 */
		ustring getTemplateFile();

		/**
		 * Make project by data from codegen
		 * @param project output file name of project
		 * @param result project body
		 */
		void make(const ustring &project, void *result);
		/**
		 * Run created project if it exists
		 * @param project file name for run
		 */
		void run(const ustring &project);
};

typedef std::list<PackProject*> PackProjects;

/*! \class Pack
    \brief access to hiasm packs

    Provides tools for working with the installed hiasm packages
*/

class Pack {
	private:
		typedef struct _CompilerItem {
			Compiler *compiler;
			bool active;
			_CompilerItem(Compiler *compiler, bool active) { this->compiler = compiler; this->active = active; }
		} CompilerItem;

		TypePixbuf icon;
		TypePixbuf icon_small;

		void loadIcons();
		PackElement *addElement(const ustring &name, const ustring &info);
		bool load();
	public:
		typedef std::list<CompilerItem> CompilerItems;

		int id;								/**< pack id in db*/
		ustring name;						/**< internal pack code name */
		ustring info;						/**< short pack description */
		ustring title;						/**< pack title */
		ustring ext;						/**< extension of pack elements */
		std::list<PackGroup*> groups;		/**< palette groups */
		std::list<PackElement*> elements;	/**< pack elements */
		CompilerItems compilers;			/**< list of available compilers */

		PackProjects projects;

		Pack(const ustring &name, const ustring &info);
		/**
		 * Loads the configuration element from the file and returns it
		 * @param name element code name
		 * @return information about element (PackElement)
		 */
		PackElement *getElementByName(const ustring &name);

		/**
		 * @return path for package
		 */
		ustring path();
		/**
		 * @return path for Elements.db
		 */
		ustring pathElementsDb();
		/**
		 * Path for element config file
		 * @param name element code name
		 * @return path for elements/<pack>/conf/<name>.ini
		 */
		ustring pathElementConf(const ustring &name);
		/**
		 * Path for element source code file
		 * @param name element code name
		 * @return path for elements/<pack>/code/hi<name>.<ext>
		 */
		ustring pathElementCode(const ustring &name);
		/**
		 * path for element icon
		 * @return path for elements/<pack>/icon/<name>.png
		 */
		ustring pathElementIcon(const ustring &name);
		/**
		 * Path source files
		 * @return path for elements/<pack>/code/
		 */
		ustring pathCode();
		/**
		 * Path make modules
		 * @return path for elements/<pack>/make/
		 */
		ustring pathMake();
		/**
		 * Provide pack icon
		 * @return pointer to Pixbuf icon
		 */
		TypePixbuf getIcon();
		/**
		 * Provide pack small icon (16x16)
		 * @return pointer to Pixbuf icon
		 */
		TypePixbuf getSmallIcon();

		/**
		 * Search project by base element name and return it
		 * @param name base element name
		 * @return project if found, else NULL
		 */
		PackProject *getProjectByEntry(const ustring &name);

		/**
		 * Get current active compiler
		 * @return pointer to compiler or NULL if not set
		 */
		Compiler *getActiveCompiler();

		/**
		 * Check exists pack on hard disk
		 */
		inline bool isExists() { return file_test(pathElementsDb(), FILE_TEST_EXISTS); }

		/**
		 * Add element to pack
		 * @param tab name of the tab who contains new element
		 * @param name element codename
		 * @param info element description
		 */
		void addElement(const ustring &tab, const ustring &name, const ustring &info);
		/**
		 * Add group to tab
		 * @param tab name of tab
		 * @param group name of new group
		 */
		void addGroup(const ustring &tab, const ustring &group);
};

/*! \class PackCollection
    \brief contain information about all packages

    Contains a list of packages hiasm
*/

class PackCollection : public std::list<Pack*> {
	private:
		void load();
	public:
		PackCollection();

		/**
		 * Search pack by code name and return it
		 * @param name internal pack name
		 * @return pack if found, else NULL
		 */
		Pack *getPackByName(const ustring &name);
};

extern PackCollection *packSet;

#endif /* PACK_H_ */
