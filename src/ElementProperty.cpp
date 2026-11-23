/*
 * ElementProperty.cpp
 *
 *  Created on: 17.05.2010
 *      Author: dilma
 */

#include "ElementProperty.h"
#include "Data.h"
#include "StringList.h"
#include <cerrno>
#include <climits>
#include <cmath>

const char *empty = "(empty)";

namespace {
// Preserve serialized v4 resources until a dedicated native editor/CGT adapter
// exists. Keeping their original type avoids silently changing the scheme ABI.
bool opaqueResource(DataType type) {
	return type == data_icon || type == data_script || type == data_stream ||
		type == data_bitmap || type == data_wave || type == data_matr ||
		type == data_jpeg || type == data_menu || type == data_code;
}
}

DataValue::DataValue(DataType type) {
	this->type = type;
	value = createValue(type);
}

DataValue::~DataValue() {
	destroyValue(type, value);
}

void *DataValue::createValue(DataType type) {
	if(opaqueResource(type)) return new ustring();
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
	if(opaqueResource(type)) { delete static_cast<ustring*>(value); return; }
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
	if(opaqueResource(type)) { *static_cast<ustring*>(dest) = *static_cast<ustring*>(value); return; }
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
	if(opaqueResource(type)) return *static_cast<ustring*>(value);
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
		case data_color:	return ((TypeColor*)value)->isDelphi() ? int_to_str(((TypeColor*)value)->toDelphi()) : ((TypeColor*)value)->to_string();
		case data_pixbuf:	return serializePixbuf(value);
		case data_array:	return serializeArray(value);
		case data_font:		return serializeFont(value);
		default:			return ustring();
	}
}

ustring DataValue::serializeStrList(void *value) {
	ustring result("#");
	HStringList list;
	list = *(ustring*)value;
	for(HStringList::iterator line = list.begin(); line != list.end(); line++)
		result += int_to_str(line->size()) + ":" + *line + "|";
	return result;
}

ustring DataValue::serializePixbuf(void *value) {
	if(!*(TypePixbuf*)value) return "";
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

namespace {
bool parseInteger(const ustring &text, int &value) {
	if(text.empty()) return false;
	errno = 0;
	char *end = NULL;
	const long parsed = strtol(text.c_str(), &end, 10);
	if(errno == ERANGE || end == text.c_str() || *end || parsed < INT_MIN || parsed > INT_MAX) return false;
	value = static_cast<int>(parsed);
	return true;
}
bool parseReal(const ustring &text, double &value) {
	if(text.empty()) return false;
	errno = 0;
	char *end = NULL;
	const double parsed = g_ascii_strtod(text.c_str(), &end);
	if(errno == ERANGE || end == text.c_str() || *end || !std::isfinite(parsed)) return false;
	value = parsed;
	return true;
}
// HiAsm 4 IDE property loader (HiAsm.exe, Graphics.Colors without the
// Delphi 7 additions): a case-sensitive name lookup, then a lenient Str2Int.
const struct { const char *name; int value; } legacyColorNames[] = {
	{"clBlack", 0x000000}, {"clMaroon", 0x000080}, {"clGreen", 0x008000}, {"clOlive", 0x008080},
	{"clNavy", 0x800000}, {"clPurple", 0x800080}, {"clTeal", 0x808000}, {"clGray", 0x808080},
	{"clSilver", 0xC0C0C0}, {"clRed", 0x0000FF}, {"clLime", 0x00FF00}, {"clYellow", 0x00FFFF},
	{"clBlue", 0xFF0000}, {"clFuchsia", 0xFF00FF}, {"clAqua", 0xFFFF00}, {"clWhite", 0xFFFFFF},
	{"clMoneyGreen", 0xC0DCC0}, {"clSkyBlue", 0xF0CAA6}, {"clCream", 0xF0FBFF}, {"clMedGray", 0xA4A0A0},
	{"clActiveBorder", -16777206}, {"clActiveCaption", -16777214}, {"clAppWorkSpace", -16777204},
	{"clBackground", -16777215}, {"clBtnFace", -16777201}, {"clBtnHighlight", -16777196},
	{"clBtnShadow", -16777200}, {"clBtnText", -16777198}, {"clCaptionText", -16777207},
	{"clDefault", 0x20000000}, {"clGrayText", -16777199}, {"clHighlight", -16777203},
	{"clHighlightText", -16777202}, {"clInactiveBorder", -16777205}, {"clInactiveCaption", -16777213},
	{"clInactiveCaptionText", -16777197}, {"clInfoBk", -16777192}, {"clInfoText", -16777193},
	{"clMenu", -16777212}, {"clMenuText", -16777209}, {"clNone", 0x1FFFFFFF},
	{"clScrollBar", -16777216}, {"cl3DDkShadow", -16777195}, {"cl3DLight", -16777194},
	{"clWindow", -16777211}, {"clWindowFrame", -16777210}, {"clWindowText", -16777208},
};
// KOL Str2Int as used by HiAsm 4: optional sign, decimal digits up to the
// first other character, 32-bit wrap-around; empty or garbage text gives 0.
int legacyInteger(const std::string &text) {
	std::string::size_type pos = 0;
	const bool negative = !text.empty() && text[0] == '-';
	if(!text.empty() && (text[0] == '-' || text[0] == '+')) ++pos;
	unsigned value = 0;
	for(; pos < text.size() && text[pos] >= '0' && text[pos] <= '9'; ++pos)
		value = value * 10 + (text[pos] - '0');
	return static_cast<int>(negative ? 0u - value : value);
}
int legacyColor(const ustring &text) {
	for(const auto &color : legacyColorNames)
		if(text.raw() == color.name) return color.value;
	// Delphi hex literal $BBGGRR / $FF00000F (HiAsm 4 configs do not use it).
	if(text.size() > 1 && text.size() <= 9 && text[0] == '$') {
		unsigned value = 0;
		for(ustring::size_type i = 1; i < text.size(); ++i) {
			const int digit = text[i] > 0x7F ? -1 : g_ascii_xdigit_value(static_cast<gchar>(text[i]));
			if(digit < 0) return 0;
			value = value << 4 | digit;
		}
		return static_cast<int>(value);
	}
	// Edited colors are saved by HiAsm 6 as RGBA text.
	TypeColor parsed;
	if((text.raw().compare(0, 3, "rgb") == 0 || (!text.empty() && text[0] == '#')) && parsed.set(text))
		return parsed.toDelphi();
	return legacyInteger(text.raw());
}
// HiAsm 4 font record: the first character ('[' or the space after '|') is
// skipped, then name,size,style,color[,charset]. An empty value is the IDE
// default font.
FontRecord legacyFont(const ustring &text) {
	FontRecord font;
	if(text.empty()) {
		font.name = "MS Sans Serif";
		font.size = 8;
		font.charset = 1;	// DEFAULT_CHARSET
		return font;
	}
	std::string rest = text.raw().substr(g_utf8_next_char(text.c_str()) - text.c_str());
	auto token = [&rest](char delimiter) -> std::string {
		const auto end = rest.find(delimiter);
		const std::string field = rest.substr(0, end);
		rest.erase(0, end == std::string::npos ? end : end + 1);
		return field;
	};
	font.name = token(',');
	font.size = legacyInteger(token(','));
	font.style = legacyInteger(token(',')) & 0xFF;
	if(rest.find(',') != std::string::npos) {
		font.color = legacyInteger(token(','));
		font.charset = legacyInteger(token(']')) & 0xFF;
	}
	else
		font.color = legacyInteger(token(']'));
	return font;
}
// Scan one encoded list without ever reading beyond the supplied string.
bool scanList(const ustring &text, ustring::size_type &pos, HStringList *list) {
	++pos; // #
	while(pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
		ustring::size_type count = 0;
		do {
			const unsigned digit = text[pos++] - '0';
			if(digit > text.size() || count > (text.size() - digit) / 10) return false;
			count = count * 10 + digit;
		} while(pos < text.size() && text[pos] >= '0' && text[pos] <= '9');
		if(pos == text.size() || text[pos++] != ':' || count > text.size() - pos) return false;
		const auto start = pos;
		pos += count;
		if(pos == text.size() || text[pos++] != '|') return false;
		if(list) list->push_back(text.substr(start, count));
	}
	return true;
}
}

bool DataValue::parseStrList(const ustring &text, void *value) {
	if(text.empty() || text[0] != '#') { *(ustring*)value = text; return true; }
	HStringList list;
	ustring::size_type pos = 0;
	if(!scanList(text, pos, &list) || pos != text.size()) return false;
	*(ustring*)value = list.text();
	return true;
}

bool DataValue::parsePixbuf(const ustring &text, void *value) {
	if(text.empty()) { ((TypePixbuf*)value)->reset(); return true; }
	gsize size = 0;
	guchar *buf = g_base64_decode(text.c_str(), &size);
	bool valid = false;
	try {
		auto loader = Gdk::PixbufLoader::create();
		loader->write(buf, size);
		loader->close();
		if(loader->get_pixbuf()) { *(TypePixbuf*)value = loader->get_pixbuf(); valid = true; }
	} catch(const Glib::Error &) { }
	g_free(buf);
	return valid;
}


// Skips a list of strings (#count:text|...) that starts at pos. The texts are not needed by an array value.
static bool skipStringList(const std::string &text, std::string::size_type &pos) {
	++pos; // #
	while(pos < text.size() && text[pos] >= '0' && text[pos] <= '9') {
		std::string::size_type count = 0;
		do {
			const unsigned digit = text[pos++] - '0';
			if(digit > text.size() || count > (text.size() - digit) / 10) return false;
			count = count * 10 + digit;
		} while(pos < text.size() && text[pos] >= '0' && text[pos] <= '9');
		if(pos == text.size() || text[pos++] != ':' || count > text.size() - pos) return false;
		for(std::string::size_type i = 0; i < count && pos < text.size(); ++i)
			do { ++pos; } while(pos < text.size() && (text[pos] & 0xC0) == 0x80); // the count is in characters
		if(pos == text.size() && count) return false;
		if(pos == text.size() || text[pos++] != '|') return false;
	}
	return true;
}

// Works on bytes: every delimiter is ASCII, and indexing a ustring costs time proportional to its length.
bool DataValue::parseArray(const ustring &source, void *value, bool legacy) {
	const std::string text = source.raw();
	ArrayValue *arr = (ArrayValue*)value;
	if(text.size() < 2 || text[0] != '[' || text[text.size()-1] != ']') return false;
	ArrayValue parsed;
	parsed.type = arr->type;
	if(parsed.type <= data_null || parsed.type >= data_count) return false;
	std::string::size_type pos = 1;
	while(pos < text.size()-1) {
		if(text[pos++] != 0x27) return false;
		const auto endName = text.find('\'', pos);
		if(endName == std::string::npos || endName + 1 >= text.size() || text[endName+1] != '=') return false;
		const ustring name(text.substr(pos, endName-pos));
		pos = endName + 2;
		const auto start = pos;
		int brackets = 0, parens = 0;
		bool quoted = false;
		while(pos < text.size()-1) {
			const char c = text[pos];
			if(!quoted && c == '#') {
				if(!skipStringList(text, pos)) return false;
				continue;
			}
			if(c == '"') quoted = !quoted;
			if(!quoted) {
				if(c == '[') ++brackets;
				if(c == ']' && --brackets < 0) return false;
				if(c == '(') ++parens;
				if(c == ')' && --parens < 0) return false;
				if(c == ',' && !brackets && !parens) break;
			}
			++pos;
		}
		if(quoted || brackets || parens) return false;
		void *item = parsed.add(name);
		if(!item || !parse(ustring(text.substr(start, pos-start)), parsed.type, item, legacy)) return false;
		if(pos < text.size()-1 && ++pos == text.size()-1) return false;
	}
	std::swap(arr->type, parsed.type);
	arr->swap(parsed);
	return true;
}

bool DataValue::parseFont(const ustring &text, void *value) {
	if(text.size() < 2 || text[0] != '[' || text[text.size()-1] != ']') return false;
	FontRecord parsed;
	ustring::size_type pos = 1;
	const auto comma = text.find(',', pos);
	if(comma == ustring::npos) return false;
	parsed.name = text.substr(pos, comma-pos);
	pos = comma+1;
	int *fields[] = { &parsed.size, &parsed.style, &parsed.color, &parsed.charset };
	for(int i=0; i<4; ++i) {
		const auto end = i == 3 ? text.size()-1 : text.find(',', pos);
		if(end == ustring::npos || !parseInteger(text.substr(pos, end-pos), *fields[i])) return false;
		pos = end+1;
	}
	*(FontRecord*)value = parsed;
	return true;
}

bool DataValue::parse(const ustring &text, DataType type, void *value, bool legacy) {
	if(!value || !g_utf8_validate(text.c_str(), text.bytes(), NULL)) return false;
	if(opaqueResource(type)) { *static_cast<ustring*>(value) = text; return true; }
	switch(type) {
		case data_int:
		case data_combo:
		case data_flags:
		case data_comboEx:
			return parseInteger(text, *(int*)value);
			break;
		case data_str:
			if(!text.empty() && text[0] == '"') {
				if(text.size() < 2 || text[text.size()-1] != '"') return false;
				((ustring*)value)->assign(text, 1, text.length() - 2);
			}
			else if(!text.empty() && text[0] == '#')
				return parseStrList(text, value);
			else
				*(ustring*)value = text;
			break;
		case data_stock:
		case data_element:
			if(!text.empty() && text[0] == '"') {
				if(text.size() < 2 || text[text.size()-1] != '"') return false;
				((ustring*)value)->assign(text, 1, text.length() - 2);
			}
			else
				*(ustring*)value = text;
			break;
		case data_real:
			return parseReal(text, *(double*)value);
		case data_data: {
			int i;
			static const int dtypes[] = { data_int, data_str, data_real };

			if(text.empty()) { ((TData*)value)->clear(); return true; }
			for(int t = 0; t < 3; t++)
				if(strncmp(text.c_str(), dataNames[dtypes[t]], i = strlen(dataNames[dtypes[t]])) == 0 &&
				   text.size() >= static_cast<unsigned>(i+2) && text[i] == '(' && text[text.size()-1] == ')') {
					ustring s = ustring(text, i+1, text.length() - i - 2);
					switch(t) {
						case 0: {
							int number;
							if(!parseInteger(s, number)) return false;
							*(TData*)value = number;
							break;
						}
						case 1: *(TData*)value = s; break;
						case 2: {
							double number;
							if(!parseReal(s, number)) return false;
							*(TData*)value = number;
							break;
						}
					}
					return true;
				}
			return false;
		}
		case data_list:
			return parseStrList(text, value);
			break;
		case data_pixbuf:
			return parsePixbuf(text, value);
			break;
		case data_array:
			return parseArray(text, value, legacy);
			break;
		case data_color:
			{
				if(legacy) {
					*(TypeColor*)value = TypeColor::fromDelphi(legacyColor(text));
					break;
				}
				TypeColor parsed;
				if(!parsed.set(text)) return false;
				*(TypeColor*)value = parsed;
				break;
			}
		case data_font:
			if(legacy) {
				*(FontRecord*)value = legacyFont(text);
				break;
			}
			return parseFont(text, value);
			break;
		default:
			return false;
	}
	return true;
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

	// HiAsm 4 also gives an empty color or font default a value (black,
	// MS Sans Serif 8), and CodeGen emits it.
	if(!prop->value.empty() || ((type == data_color || type == data_font) && legacy()))
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

bool ElementProperty::legacy() const {
	return parent && parent->tpl && parent->tpl->conf && parent->tpl->conf->pack && parent->tpl->conf->pack->isLegacy();
}

bool ElementProperty::initFromText(const ustring &text, bool isDefault) {
	//DEBUG_MSG("Init prop: " << name.c_str() << " = " << text.c_str() << " l: " << text.length())
	if(type == data_array && isDefault)	// load from INI file
	{
		int itemType = 0;
		if(!parseInteger(text, itemType) || itemType <= data_null || itemType >= data_count) return false;
		void *check = createValue((DataType)itemType);
		if(!check) return false;
		destroyValue((DataType)itemType, check);
		((ArrayValue*)value)->clear();
		((ArrayValue*)defValue)->clear();
		((ArrayValue*)value)->type = (DataType)itemType;
		((ArrayValue*)defValue)->type = (DataType)itemType;
	}
	else {
		// Colour defaults of the C++ package are Delphi names (WinControl: clBtnFace); they are read as HiAsm 4 reads them.
		const bool delphiColor = type == data_color && !legacy() && text.raw().compare(0, 2, "cl") == 0;
		if(!parse(text, type, value, legacy() || delphiColor)) return false;
		if(isDefault)
			copyValue(type, defValue, value);
		else
			change();
	}
	return true;
}

bool ElementProperty::isDefault() {
	if(opaqueResource(type)) return static_cast<ustring*>(value)->raw() == static_cast<ustring*>(defValue)->raw();
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
	if(opaqueResource(type)) { *static_cast<ustring*>(defValue) = *static_cast<ustring*>(value); return; }
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
