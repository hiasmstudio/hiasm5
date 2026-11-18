/*
 * Data.cpp
 *
 *  Created on: 08.05.2010
 *      Author: dilma
 */

#include "Data.h"

TData::~_TData() {
	if(next)
		delete next;
}

_TData &TData::operator = (const ustring &text) {
	clear();
	type = data_str;
	sdata = text;
	return *this;
}

_TData &TData::operator = (int value) {
	clear();
	type = data_int;
	idata = value;
	return *this;
}

_TData &TData::operator = (double value) {
	clear();
	type = data_real;
	rdata = value;
	return *this;
}

_TData &TData::operator = (TypePixbuf *value) {
	clear();
	type = data_pixbuf;
	data = value;
	return *this;
}

_TData &TData::operator = (DataArrayInterface *value) {
	clear();
	type = data_array;
	data = value;
	return *this;
}

ustring TData::toStr() {
	switch(type) {
		case data_int:	return int_to_str(idata);
		case data_str:	return sdata;
		case data_real:	return double_to_str(rdata);
		default:		return ustring();
	}
}

int TData::toInt() {
	switch(type) {
		case data_int:	return idata;
		case data_str:	return atoi(sdata.c_str());
		case data_real:	return (int)rdata;
		default:		return 0;
	}
}

double TData::toReal() {
	switch(type) {
		case data_int:	return idata;
		case data_str:	return atof(sdata.c_str());
		case data_real:	return rdata;
		default:		return 0.0;
	}
}

void *TData::toObj() {
	return data;
}

TypePixbuf *TData::toPixbuf() {
	return (type == data_pixbuf) ? (TypePixbuf *)data : NULL;
}

DataArrayInterface *TData::toArray() {
	return (type == data_array) ? (DataArrayInterface *)data : NULL;
}

bool TData::operator == (const _TData &data) {
	if(type == data.type)
		switch(type) {
			case data_null:	return data.type == data_null;
			case data_int:	return data.idata == idata;
			case data_real:	return data.rdata == rdata;
			case data_str:	return data.sdata == sdata;
			default:		return false;
		}
	return false;
}

bool TData::empty() {
	switch(type) {
		case data_int:	return idata == 0;
		case data_real:	return rdata == 0.0;
		case data_str:	return sdata.empty();
		default:		return true;
	}
}

bool TData::compare(_TData &data) {
	switch(type) {
		case data_null:	return data.empty();
		case data_int:	return idata == data.toInt();
		case data_real:	return rdata == data.toReal();
		case data_str:	return sdata == data.toStr();
		default:		return false;
	}
}

void TData::clear() {
	type = data_null;
	if(next) {
		delete next;
		next = NULL;
	}
}

TData &TData::shift() {
	if(next) {
		type = next->type;
		rdata = next->rdata;
		sdata = next->sdata;
		TData *n = next->next;
		next->next = NULL;
		delete next;
		next = n;
	}
	else
		clear();
	return *this;
}

void TData::dump() {
	DEBUG_MSG("Data dump-------------------")
	TData *n = this;
	int i = 0;
	while(n) {
		DEBUG_MSG("[" << i++ << "] = " << n->toStr())
		n = n->next;
	}
}

_TData &TData::assign_mt(const _TData &data) {
	assign(data);
	_TData **n = &next;
	_TData *sn = data.next;
	while(sn) {
		*n = new _TData(sn);
		n = &(*n)->next;
		sn = sn->next;
	}
	return *this;
}

/*
template<class T>
_TData &TData::push(const T &value) {
	if(isNull())
		*this = value;
	else {
		TData **n = &next;
		while(*n) n = &next->next;
		*n = new _TData(value);
	}
	return *this;
}
*/
