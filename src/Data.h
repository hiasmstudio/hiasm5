/*
 * Data.h
 *
 *  Created on: 08.05.2010
 *      Author: dilma
 */

#ifndef DATA_H_
#define DATA_H_

#include "share.h"

/*! \struct TData
    \brief internal type of data transfer

    Provides a generic container for data transfer between elements
*/

class DataArrayInterface;
typedef struct _TData {
	DataType type;		/**< data type (constant data_xxx) */
	union {
		int idata;		/**< contain integer data */
		double rdata;	/**< contain double data */
		void *data;		/**< contain object data */
	};
	ustring sdata;		/**< contain string data */

	_TData *next;		/**< pointer to next ring in MT data */

	/**
	 * Create null data
	 */
	_TData() { type = data_null; next = NULL; }
	/**
	 * Create data as copy
	 * @param source
	 */
	_TData(_TData *source) {
		next = NULL;
		if(source)
			assign(*source);
		else
			type = data_null;
	}
	/**
	 * Create data as copy
	 * @param source
	 */
	_TData(const _TData &source) {
		next = NULL;
		assign(source);
	}
	/**
	 * Create string data
	 * @param value string
	 */
	_TData(const ustring &value) {
		type = data_str;
		sdata = value;
		next = NULL;
	}
	/**
	 * Create integer data
	 * @param value integer
	 */
	_TData(int value) {
		type = data_int;
		idata = value;
		next = NULL;
	}
	/**
	 * Create double data
	 * @param value double
	 */
	_TData(double value) {
		type = data_real;
		rdata = value;
		next = NULL;
	}
	/**
	 * Create pixbuf data
	 * @param value pixbuf
	 */
	_TData(TypePixbuf *value) {
		type = data_pixbuf;
		data = value;
		next = NULL;
	}
	/**
	 * Create array data
	 * @param value of array
	 */
	_TData(DataArrayInterface *value) {
		type = data_array;
		data = value;
		next = NULL;
	}
	/**
	 * Destroy TData and MT
	 */
	~_TData();

	/**
	 * Set data type to string
	 * @param text new string value
	 * @return self
	 */
	_TData &operator = (const ustring &text);
	/**
	 * Set data type to integer
	 * @param value new integer value
	 * @return self
	 */
	_TData &operator = (int value);
	/**
	 * Set data type to unsigned integer
	 * @param value new unsigned integer value
	 * @return self
	 */
	_TData &operator = (unsigned int value) { *this = (int)value; return *this; }
	/**
	 * Set data type to double
	 * @param value new double value
	 * @return self
	 */
	_TData &operator = (double value);
	/**
	 * Set data type to pixbuf
	 * @param value new pixbuf value
	 * @return self
	 */
	_TData &operator = (TypePixbuf *value);
	/**
	 * Set data type to array
	 * @param value new array value
	 * @return self
	 */
	_TData &operator = (DataArrayInterface *value);
	/**
	 * Copy data from source
	 * @param value new data value
	 * @return self
	 */
	inline _TData &operator = (_TData *value) {
		if(value)
			assign(*value);
		else
			clear();
		return *this;
	}
	/**
	 * Copy data from source
	 * @param value new data value
	 * @return self
	 */
	inline _TData &operator = (const _TData &value) {
		return assign(value);
	}

	/**
	 * Compare two TData structure
	 * @param data
	 * @return true if data equal
	 */
	bool operator == (const _TData &data);
	/**
	 * Compare two TData structure with type conversion
	 * @param data
	 * @return true if data equal
	 */
	bool compare(_TData &data);

	/**
	 * Check data assignment
	 * @return true if value of data not empty
	 */
	bool empty();

	/**
	 * Clear data (set type to data_null)
	 */
	void clear();
	/**
	 * Check data at null type
	 * @return true if type = data_null
	 */
	inline bool isNull() { return type == data_null; }

	/**
	 * Convert data to string
	 * @return string value or "" for unsupported types
	 */
	ustring toStr();
	/**
	 * Convert data to integer
	 * @return string value or 0 for unsupported types
	 */
	int toInt();
	/**
	 * Convert data to double
	 * @return string value or 0.0 for unsupported types
	 */
	double toReal();
	/**
	 * Convert data to object
	 * @return object
	 */
	void *toObj();
	/**
	 * Convert data to pixbuf
	 * @return object
	 */
	TypePixbuf *toPixbuf();
	/**
	 * Convert data to DataArrayInterface
	 * @return object
	 */
	DataArrayInterface *toArray();

	operator int() { return toInt(); }
	operator unsigned int() { return toInt(); }
	operator double() { return toReal(); }
	operator ustring() { return toStr(); }
	operator TypePixbuf() { return *toPixbuf(); }

	/**
	 * Copy data from external TData structure
	 * @return self pointer
	 */
	_TData &assign(const _TData &data) {
		clear();
		type = data.type;
		rdata = data.rdata;
		sdata = data.sdata;
		return *this;
	}
	/**
	 * Copy data from external TData structure with all MT rings
	 * @return self pointer
	 */
	_TData &assign_mt(const _TData &data);

	/**
	 * Add MT ring to data
	 * @param value data to add
	 * @return self pointer
	 */
	template<class T>
	_TData &push(const T &value) {
		if(isNull())
			*this = value;
		else {
			_TData **n = &next;
			while(*n) n = &(*n)->next;
			*n = new _TData(value);
		}
		return *this;
	}
	/**
	 * Add MT ring to data
	 * @param value data to add
	 * @return self pointer
	 */
	template<class T>
	_TData &operator << (const T &value) {
		return push(value);
	}
	/**
	 * Give next MT ring from data and remove it from train
	 * @return self pointer
	 */
	_TData &shift();
	/**
	 * Give MT ring from data and return it
	 * @return mt ring or data_empty if not exists
	 */
	_TData pop() {
		_TData d(this);
		shift();
		return d;
	}

	void dump();
} TData;

/*! \class DataArrayInterface
    \brief interface for access array

    Provides an interface to control array data
*/

class DataArrayInterface {
	public:
		/**
		 * Return count of array items
		 * @return items count
		 */
		virtual int _arr_count() = 0;
		/**
		 * Change existing item in array
		 * @param index of item
		 * @param data new item value
		 */
		virtual void _arr_set(TData &index, TData &data) {}
		/**
		 * Retrieve array item by index
		 * @param index of item
		 * @return item value
		 */
		virtual TData _arr_get(TData &index) { return TData(); }
		/**
		 * Add new item in array
		 * @param data new item value
		 */
		virtual void _arr_add(TData &data) {}
};

#endif /* DATA_H_ */
