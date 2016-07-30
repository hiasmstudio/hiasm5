/*
 * ElementConfig.h
 *
 *  Created on: 05.05.2010
 *      Author: dilma
 */

#ifndef ELEMENTCONFIG_H_
#define ELEMENTCONFIG_H_

#include "share.h"
#include "Pack.h"

#define VF_MAKEPOINT   0x01
#define VF_DBLOPEN     0x02

/*! \struct ConfMethod
    \brief describes the method element

    Contains a description of the method element in the template (parse from *.ini files)
*/

typedef struct _ConfMethod {
	ustring name;	/**< method name */
	ustring prop;	/**< combo or comboex property */
	ustring info;	/**< method description */
	int mType;		/**< method type pt_xxx */
	int dataType;	/**< point type data_xxx */
	_ConfMethod() {}
	_ConfMethod(_ConfMethod &conf) {
		name = conf.name;
		prop = conf.prop;
		info = conf.info;
		mType = conf.mType;
		dataType = conf.dataType;
	}
} ConfMethod;

/*! \struct ConfProperty
    \brief describes the property element

    Contains a description of the property element in the template (parse from *.ini files)
*/

typedef struct _ConfProperty {
	ustring name;	/**< property name */
	ustring info;	/**< property description */
	DataType type;	/**< property type data_xxx */
	ustring value;	/**< default value */
	ustring group;	/**< property group */
	ustring list;	/**< list of values for type data_combo and other */
	int flags;		/**< property flags VK_XXX */

	/**
	 * Default constructor for the struct
	 */
	_ConfProperty() {}
	/**
	 * Construct structure as copy of conf
	 * @param conf source ConfProperty for duplicate it
	 */
	_ConfProperty(_ConfProperty &conf) {
		name = conf.name;
		info = conf.info;
		type = conf.type;
		value = conf.value;
		group = conf.group;
		list = conf.list;
		flags = conf.flags;
	}
} ConfProperty;

typedef std::list<ConfProperty*> ConfPropertys;

class ConfMethods : public std::list<ConfMethod*> {
	public:
		/**
		 * Search method of template by name
		 * @param name method name
		 * @return pointer to ConfMethod if found, else NULL
		 */
		ConfMethod *getByName(const ustring &name);
};

typedef ConfMethods ConfDefMethods;

/*! \class ElementConfig
    \brief Contain element template

    Provides an interface to access the template element (parse from *.ini files)
*/

class Pack;
class ElementConfig {
	private:
		bool load(const ustring &file_name);

		// parse ini sections
		void parseAbout(char *buf);
		void parseType(char *buf);
		void parseEdit(char *buf);
		void parseProperty(char *buf);
		void parseMethod(char *buf);
		void inheritFrom(char *buf);
	public:
		ustring name;		/**< element name (Button, Label, Hub, ...) */
		ustring cClass;		/**< element class (Element, MultiElement, DPElement, ...) */
		ustring sub;		/**< name of child element */
		ustring info;		/**< element description */
		ustring interfaces;	/**< list of interfaces element */
		ustring tab;		/**< name of parent tab */

		ustring inherit;	/**< list of inherited templates */

		ustring view;		/**< name of property who display text instead of icon */
		int viewFontHeight;
		ustring editClassName;	/**< class name use in FormEditor */
		ConfPropertys prop;		/**< list of properties element */
		ConfMethods mtd;		/**< list of methods element */
		ConfDefMethods def;		/**< list of hidden element methods */

		ustring dblOpen;		/**< property name who begin edit at double click on element */
		int icon;				/**< ??? */

		ustring iconic;			/**< name of property who display element icon */

		Pack *pack;				/**< pointer to parent pack */

		ElementConfig(Pack *pack, const ustring &file_name);

		/**
		 * Duplicate properties from another source
		 * @param a_prop ConfPropertys for duplicating
		 */
		void assignProp(ConfPropertys &a_prop);
		/**
		 * Duplicate methods from another source
		 * @param a_mtd ConfMethods for duplicating
		 */
		void assignMtd(ConfMethods &a_mtd);
		/**
		 * Duplicate hidden methods from another source
		 * @param a_def ConfDefMethods for duplicating
		 */
		void assignDef(ConfDefMethods &a_def);

		/**
		 * Search property of template by name
		 * @param name property name
		 * @return ConfProperty if found, else NULL
		 */
		ConfProperty *findPropByName(const ustring &name);

		/**
		 * Check an element  for editing in the form
		 * @return true if element is GUI
		 */
		bool isWinElement() { return cClass == "WinElement"; }
};

#endif /* ELEMENTCONFIG_H_ */
