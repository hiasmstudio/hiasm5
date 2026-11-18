/*
 * ElementProperty.h
 *
 *  Created on: 17.05.2010
 *      Author: dilma
 */

#ifndef ELEMENTPROPERTY_H_
#define ELEMENTPROPERTY_H_

#include "share.h"
#include "ElementConfig.h"
#include "Element.h"

extern const char *empty;

typedef struct _FontRecord {
	ustring name;
	int color = 0;
	int size = 0;
	int style = 0;
	int charset = 0;

	bool operator == (const _FontRecord &src) {
		return name == src.name && color == src.color && size == src.size && style == src.style && charset == src.charset;
	}
} FontRecord;

/*! \class DataValue
    \brief pointer to DataType value

    Provides methods for working with the DataType value
*/

class DataValue {
	private:
		/**
		 * Convert string list to one long string with special format
		 * @param value data to serialize
		 * @return long string with format #<len1>:<text1>|<len2>:<text2>|...
		 */
		static ustring serializeStrList(void *value);
		/**
		 * Convert pixbuf image to one long string
		 * @param value data to serialize
		 * @return long string in base64 format
		 */
		static ustring serializePixbuf(void *value);
		/**
		 * Convert data array to one long string
		 * @param value data to serialize
		 * @return long string in format: ['name1'=value1,'name2'=value2,...]
		 */
		static ustring serializeArray(void *value);
		/**
		 * Convert font record to string
		 * @param font value to serialize
		 * @return string in format: [name,size,style,color,charset]
		 */
		static ustring serializeFont(void *value);
		/**
		 * Parse string line to string with \n delimiters
		 * @param value data to parse
		 * @param text string with format #<len1>:<text1>|<len2>:<text2>|...
		 */
		static bool parseStrList(const ustring &text, void *value);
		/**
		 * Parse pixbuf
		 * @param value data to parse
		 * @param text string in base64 format
		 */
		static bool parsePixbuf(const ustring &text, void *value);
		/**
		 * Parse array
		 * @param value data to parse
		 * @param text string
		 * @param legacy items use HiAsm 4 syntax
		 */
		static bool parseArray(const ustring &text, void *value, bool legacy);
		/**
		 * Parse font record
		 * @param value font to parse
		 * @param text string
		 */
		static bool parseFont(const ustring &text, void *value);
	public:
		DataType type;	/**< property type i.e. data_int, data_str, data_real, ... */
		void *value;	/**< pointer to property value (determined by field type) */

		DataValue(DataType type);
		virtual ~DataValue();
		/**
		 * Create property value by type
		 * @param type of value
		 * @return allocated memory for value type
		 */
		static void *createValue(DataType type);
		/**
		 * Destroy value by type
		 * @param type of value
		 * @param value data to destroy
		 */
		static void destroyValue(DataType type, void *value);
		/**
		 * Set value from memory buffer
		 * @param type of dest and value args
		 * @param dest destination location
		 * @param value source location
		 */
		static void copyValue(DataType type, void *dest, void *value);

		/**
		 * Store data as string (require for save in text)
		 * @param type of value
		 * @param value data to serialize
		 * @return value as formatted text
		 */
		static ustring serialize(DataType type, void *value);
		/**
		 * Parse value from text
		 * @param type of value
		 * @param value data to parse
		 * @param text data for value parse
		 * @param legacy text comes from an external HiAsm 4 package: colors are
		 *        Delphi TColor (clBtnFace, -16777201, $00FF00) and fonts use the v4 rules
		 */
		static bool parse(const ustring &text, DataType type, void *value, bool legacy = false);
};

/*! \class ElementProperty
    \brief element property

    Provides methods for working with the property element
*/

class Element;

class ElementProperty : public DataValue {
	private:
		void *defValue;	/**< pointer to default value */
		bool lock;		/**< lock property for change */
		/** True if the parent element belongs to an external HiAsm 4 package */
		bool legacy() const;
	public:
		Element *parent;/**< pointer to parent Element */
		ustring name;	/**< the name of property */
		ustring info;	/**< property description for user */
		ustring list;	/**< list values for data_combo, data_comboex types */
		int flag;		/**< flags */
		ustring group;	/**< name of parent group */

		ElementProperty(Element *parent, ConfProperty *prop);
		ElementProperty(Element *parent, const ustring &name, const ustring &info, DataType type, const ustring &list = ustring());
		virtual ~ElementProperty();

		/**
		 * Parse property value from text
		 * @param text value of property in text record
		 * @param isDefault if true the text set as default value for this property
		 */
		bool initFromText(const ustring &text, bool isDefault = false);
		/**
		 * Store property data as string (require for save in text)
		 * @return formatted value of the property
		 */
		ustring serialize(){ return DataValue::serialize(type, value); }
		/**
		 * Verifies the property is changed
		 * @return true if value == defValue
		 */
		bool isDefault();
		/**
		 * Assign default value to current property value
		 */
		void setAsDefault();
		/**
		 * Set property value
		 * @param value
		 */
		void setValue(void *value);

		/**
		 * Make element point for this property (do<name>)
		 */
		void makePoint();
		/**
		 * Checking the existence of points with this name
		 * @return true if point with name do<name> exists
		 */
		bool checkPoint();
		/**
		 * Remove element point for this property (do<name>)
		 */
		void removePoint();

		/**
		 * Read value as integer (need for core elements)
		 * @return numeric value of property
		 */
		int readInt() { return (type == data_int)||(type == data_combo)||(type == data_comboEx) ? *(int*)value : 0; }
		/**
		 * Read value as double (need for core elements)
		 * @return double value of property
		 */
		double readReal() { return type == data_real ? *(double*)value : 0; }
		/**
		 * Read value as string (need for core elements)
		 * @return string value of property
		 */
		ustring readStr() { return (type == data_str)||(type == data_list)||(type == data_stock)||(type == data_element) ? *(ustring*)value : ustring(); }
		/**
		 * Read value as TData (need for core elements)
		 * @return string value of property
		 */
		TData *readData() { return type == data_data ? (TData*)value : NULL; }

		/**
		 * Emit change event in parent element
		 */
		void change();
};

typedef struct _ArrayValueItem {
	ustring name;
	void *data;
	_ArrayValueItem(ustring name, void *data) { this->name = name; this->data = data; }
} ArrayValueItem;

class ArrayValue : public std::vector<ArrayValueItem> {
	public:
		DataType type;

		ArrayValue() : type(data_str) { }
		ArrayValue(const ArrayValue &source) : type(source.type) {
			for(const auto &item : source)
				DataValue::copyValue(type, add(item.name), item.data);
		}
		ArrayValue &operator=(const ArrayValue &source) {
			if(this != &source) {
				ArrayValue copy(source);
				std::swap(type, copy.type);
				swap(copy);
			}
			return *this;
		}
		~ArrayValue() { clear(); }
		void clear() {
			for(const auto &item : *this) DataValue::destroyValue(type, item.data);
			std::vector<ArrayValueItem>::clear();
		}

		void *add(const ustring &name = ustring()) {
			if(type <= data_null || type >= data_count) return NULL;
			void *value = DataValue::createValue(type);
			if(!value) return NULL;
			push_back(ArrayValueItem(name.empty() ? dataNames[type] : name, value));
			return value;
		}
		void remove(int i) {
			DataValue::destroyValue(type, at(i).data);
			erase(begin() + i);
		}
};

#endif /* ELEMENTPROPERTY_H_ */
