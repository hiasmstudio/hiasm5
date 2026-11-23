/* 
 * File:   ElementData.cpp
 * Author: dilma
 * 
 * Created on May 13, 2011, 3:31 PM
 */

#include "share.h"
#include "ElementData.h"

static int _id_ = 0;

ElementData::ElementData(TUnitNode *codeTree) {
	id = ++_id_;
	this->codeTree = codeTree;
}

ElementData::~ElementData() {
	for(FiledsList::iterator f = fields.begin(); f != fields.end(); f++)
		delete *f;
	fields.clear();
}

TValue *ElementData::getField(const char *name) {
	for(FiledsList::iterator f = fields.begin(); f != fields.end(); f++)
		if(strcasecmp(name, (*f)->name.c_str()) == 0)
			return (*f)->value;

	return NULL;
}

TValue *ElementData::addField(const char *name) {
	TVarItem *var = new TVarItem(name);
	fields.push_back(var);
	return var->value = new TValue();
}

int ElementData::getFieldIndex(const char *name) {
	int i = 0;
	for(FiledsList::iterator f = fields.begin(); f != fields.end(); f++) {
		if(strcasecmp(name, (*f)->name.c_str()) == 0)
			return i;
		i++;
	}

	return -1;
}

void ElementData::init() {
	_id_ = 0;
}