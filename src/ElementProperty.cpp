/*
 * ElementProperty.cpp
 *
 *  Created on: 17.05.2010
 *      Author: dilma
 */

#include "ElementProperty.h"
#include "Data.h"
#include "StringList.h"

const char *empty = "(empty)";

DataValue::DataValue(DataType type) {
	this->type = type;
	value = createValue(type);
}

DataValue::~DataValue() {
	destroyValue(type, value);
}

void *DataValue::createValue(DataType type) {
	switch (type) {
		case data_int:
		case data_flags:
		case data_combo:
		case data_comboEx:	return new int();
		case data_str:
		case data_list:
		case data_element:
		case data_stock:	return new ustring();
		case data_real:		return new double();
		case data_data:		return new TData();
		case data_pixbuf:	return new TypePixbuf();
		case data_color:	return new TypeColor();
		case data_array:	return new ArrayValue();
		case data_font:		return new FontRecord();
		default:
			std::cout << "Unknown property type: " << type << std::endl;
	}
	return NULL;
}

void DataValue::destroyValue(DataType type, void *value) {
	switch(type) {
		case data_int:
		case data_flags:
		case data_combo:
		case data_comboEx:
			delete (int*)value;
			break;
		case data_str:
		case data_list:
		case data_element:
		case data_stock:
			delete (ustring*)value;
			break;
		case data_real:
			delete (double*)value;
			break;
		case data_data:
			delete (TData*)value;
			break;
		case data_pixbuf:
			delete (TypePixbuf*)value;
			break;
		case data_color:
			delete (TypeColor*)value;
			break;
		case data_array:
			delete (ArrayValue *)value;
			break;
		case data_font:
			delete (FontRecord *)value;
			break;
		default:
			;
	}
}

void DataValue::copyValue(DataType type, void *dest, void *value) {
	switch(type) {
		case data_int:
		case data_flags:
		case data_combo:
		case data_comboEx:
			*(int*)dest = *(int*)value;
			break;
		case data_stock:
		case data_str:
		case data_element:
		case data_list:
			*(ustring*)dest = *(ustring*)value;
			break;
		case data_real:
			*(double*)dest = *(double*)value;
			break;
		case data_data:
			*(TData*)dest = *(TData*)value;
			break;
		case data_color:
			*(TypeColor*)dest = *(TypeColor*)value;
			break;
		case data_pixbuf:
			*(TypePixbuf*)dest = *(TypePixbuf*)value;
			break;
		case data_array:
			*(ArrayValue*)dest =*(ArrayValue*)value;
			break;
		case data_font:
			*(FontRecord*)dest =*(FontRecord*)value;
			break;
		default:
			;
	}
}

ustring DataValue::serialize(DataType type, void *value) {
	switch(type) {
		case data_int:
		case data_flags:
		case data_combo:
		case data_comboEx:	return int_to_str(*(int*)value);
		case data_stock:
		case data_element:	return ustring("\"") + *(ustring*)value + "\"";
		case data_real: 	return double_to_str(*(double *)value);
		case data_data:
			if(((TData*)value)->type == data_null)
				return ustring("");
			return ustring(dataNames[((TData*)value)->type]) + "(" + ((TData*)value)->toStr() + ")";
		case data_str:
		case data_list:		return serializeStrList(value);
		case data_color:	return ((TypeColor*)value)->to_string();
		case data_pixbuf:	return serializePixbuf(value);
		case data_array:	return serializeArray(value);
		case data_font:		return serializeFont(value);
		default:			return ustring();
	}
}

ustring DataValue::serializeStrList(void *value) {
	ustring result("#");
	StringList list;
	list = *(ustring*)value;
	for(StringList::iterator line = list.begin(); line != list.end(); line++)
		result += int_to_str(line->size()) + ":" + *line + "|";
	return result;
}

ustring DataValue::serializePixbuf(void *value) {
	gchar *buf = NULL;
	gsize size = 0;

	(*(TypePixbuf*)value)->save_to_buffer(buf, size);
	gchar *res = g_base64_encode((const guchar*)buf, size);
	ustring result(res);
	g_free(res);
	g_free(buf);

	return result;
}

ustring DataValue::serializeArray(void *value) {
	ustring result("[");
	ArrayValue *arr = (ArrayValue*)value;
	for(ArrayValue::iterator i = arr->begin(); i != arr->end(); i++) {
		if(i != arr->begin())
			result += ",";
		result += "'" + i->name + "'=" + serialize(arr->type, i->data);
	}
	result += "]";

	return result;
}

ustring DataValue::serializeFont(void *value) {
	ustring result("[");
	FontRecord *font = (FontRecord*)value;
	result += font->name + "," + int_to_str(font->size) + ",";
	result += int_to_str(font->style) + ",";
	result += int_to_str(font->color) + ",";
	result += int_to_str(font->charset) + "]";

	return result;
}

void DataValue::parseStrList(const ustring &text, void *value) {
	int l = text.bytes();
	char *buf = new char[l+1], *sb = buf;
	strcpy(buf, text.c_str());
	if(*buf == '#') {
		buf++;
		if(*buf) {
			StringList list;
			do {
				int n = atoi(getTok(&buf, ':'));
				ustring s = ustring(buf, n);
				list << s;
				buf += s.bytes();	// data
				buf ++;		// | delimiter
			} while(*buf);
			*(ustring *)value = list;
		}
		else
			*(ustring *)value = "";
	}
	else {
		*(ustring *)value = buf;
	}

	delete sb;
}

void DataValue::parsePixbuf(const ustring &text, void *value) {
	gsize size;
	guchar *buf = g_base64_decode(text.c_str(), &size);
	Glib::RefPtr<Gdk::PixbufLoader> loader = Gdk::PixbufLoader::create();
	loader->write(buf, size);
	loader->close();
	(*(TypePixbuf*)value) = loader->get_pixbuf();
	g_free(buf);
}

void DataValue::parseArray(const ustring &text, void *value) {
	ArrayValue *arr = (ArrayValue*)value;
	int i = 1;	// skip [
	while (text[i] != ']') {
		i++;	// skip '
		int p = text.find('\'', i);
		void *v = arr->add(ustring(text, i, p - i));
		i = p + 2;	// skip =
		p = text.find(',', i);
		if(p == -1)
			p = text.length() - 1;
		parse(ustring(text, i, p - i), arr->type, v);
		i = p;
		if(text[i] == ',') i++;
	}
}

void DataValue::parseFont(const ustring &text, void *value) {
	int l = text.bytes();
	char *buf = new char[l+1], *sb = buf;
	strcpy(buf, text.c_str());
	buf++;
	((FontRecord*)value)->name = getTok(&buf, ',');
	((FontRecord*)value)->size = atoi(getTok(&buf, ','));
	((FontRecord*)value)->style = atoi(getTok(&buf, ','));
	((FontRecord*)value)->color = atoi(getTok(&buf, ','));
	((FontRecord*)value)->charset = atoi(getTok(&buf, ']'));
	delete sb;
}

void DataValue::parse(const ustring &text, DataType type, void *value) {
	switch(type) {
		case data_int:
		case data_combo:
		case data_flags:
		case data_comboEx:
			*(int *)value = atoi(text.c_str());
			break;
		case data_str:
			if(text[0] == '"')
				((ustring*)value)->assign(text, 1, text.length() - 2);
			else if(text[0] == '#')
				parseStrList(text, value);
			else
				*(ustring*)value = text;
			break;
		case data_stock:
		case data_element:
			if(text[0] == '"')
				((ustring*)value)->assign(text, 1, text.length() - 2);
			else
				*(ustring*)value = text;
			break;
		case data_real:
			*(double *)value = atof(text.c_str());
			break;
		case data_data: {
			int i;
			static const int dtypes[] = { data_int, data_str, data_real };

			for(int t = 0; t < 3; t++)
				if(strncmp(text.c_str(), dataNames[dtypes[t]], i = strlen(dataNames[dtypes[t]])) == 0) {
					ustring s = ustring(text, i+1, text.length() - i - 2);
					switch(t) {
						case 0: *(TData*)value = atoi(s.c_str()); break;
						case 1: *(TData*)value = s; break;
						case 2: *(TData*)value = atof(s.c_str()); break;
					}
				}
			break;
		}
		case data_list:
			parseStrList(text, value);
			break;
		case data_pixbuf:
			parsePixbuf(text, value);
			break;
		case data_array:
			parseArray(text, value);
			break;
		case data_color:
			((TypeColor*)value)->set(text);
			break;
		case data_font:
			parseFont(text, value);
			break;
		default:
			;
	}
}
//--------------------------------------------------------------------------------------

ElementProperty::ElementProperty(Element *parent, ConfProperty *prop):DataValue(prop->type) {
	this->parent = parent;
	name = prop->name;
	info = prop->info;
	list = prop->list;
	type = prop->type;
	flag = prop->flags;
	group = prop->group;

	lock = false;

	defValue = createValue(type);

	if(!prop->value.empty())
		initFromText(prop->value, true);
}

ElementProperty::ElementProperty(Element *parent, const ustring &name, const ustring &info, DataType type, const ustring &list):
		DataValue(type)
{
	this->parent = parent;
	this->name = name;
	this->info = info;
	this->list = list;
	this->type = type;
	flag = 0;

	lock = false;
	
	defValue = createValue(type);
}

ElementProperty::~ElementProperty() {
	destroyValue(type, defValue);
}

void ElementProperty::initFromText(const ustring &text, bool isDefault) {
	//DEBUG_MSG("Init prop: " << name.c_str() << " = " << text.c_str() << " l: " << text.length())
	if(type == data_array && isDefault)	// load from INI file
		((ArrayValue*)value)->type = (DataType)atoi(text.c_str());
	else {
		parse(text, type, value);
		if(isDefault)
			copyValue(type, defValue, value);
		else
			change();
	}
}

bool ElementProperty::isDefault() {
	switch(type) {
		case data_int:
		case data_flags:
		case data_combo:
		case data_comboEx:	return *(int *)value == *(int*)defValue;
		case data_stock:
		case data_str:
		case data_element:
		case data_list:		return *(ustring*)value == *(ustring*)defValue;
		case data_real: 	return *(double *)value == *(double *)defValue;
		case data_data:		return *(TData*)value == *(TData*)defValue;
		case data_pixbuf:	return *(TypePixbuf*)value == *(TypePixbuf*)defValue;
		case data_color:	return *(TypeColor*)value == *(TypeColor*)defValue;
		case data_array:	return ((ArrayValue*)value)->empty();
		case data_font:		return *(FontRecord*)value == *(FontRecord*)defValue;
		default:			return false;
	}
}

void ElementProperty::setAsDefault() {
	switch(type) {
		case data_int:
		case data_flags:
		case data_combo:
		case data_comboEx:	*(int *)defValue = *(int*)value; break;
		case data_stock:
		case data_str:
		case data_element:
		case data_list:		*(ustring*)defValue = *(ustring*)value; break;
		case data_real: 	*(double *)defValue = *(double *)value; break;
		case data_data:		*(TData*)defValue = *(TData*)value; break;
		case data_pixbuf:	*(TypePixbuf*)defValue = *(TypePixbuf*)value; break;
		case data_color:	*(TypeColor*)defValue = *(TypeColor*)value; break;
		case data_font:		*(FontRecord*)defValue = *(FontRecord*)value; break;
		default:
			;
	}
}

void ElementProperty::setValue(void *value) {
	copyValue(type, this->value, value);
	change();
}

void ElementProperty::change() {
	if(!lock) {
		lock = true;
		parent->on_change_property(this);
		lock = false;
	}
}

void ElementProperty::makePoint() {
	parent->addPoint(ustring("do") + name, info, pt_work);
	parent->rePosPoints();
	parent->invalidate();
}

bool ElementProperty::checkPoint() {
	return parent->getPointByName(ustring("do") + name) != NULL;
}

void ElementProperty::removePoint() {
	ElementPoint *p = parent->getPointByName(ustring("do") + name);
	parent->removePoint(p);
	parent->rePosPoints();
	parent->invalidate();
}
