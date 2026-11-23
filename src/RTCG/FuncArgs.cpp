#include <list>

#include "FuncArgs.h"
#include "Objects.h"
#include "IntFunc.h"

#include "share.h"
#include "Runner.h"

const char *procNames[PROC_COUNT] = {
	"{script proc}",
	"{object proc}",
	"{internal proc}"
};

#include <cstdio>

const char *stringLexem = "'";
bool cppStringEscapes = false;
static std::string escapeCppString(const char* text) {
    if(!cppStringEscapes) return text;
    std::string result;
    for(const unsigned char* p = reinterpret_cast<const unsigned char*>(text); *p; ++p) {
        switch(*p) {
        case '\\': result += "\\\\"; break;
        case '"': result += "\\\""; break;
        case '\n': result += "\\n"; break;
        case '\r': result += "\\r"; break;
        case '\t': result += "\\t"; break;
        default:
            // Three-digit octal escapes cannot absorb a following hex digit.
            if(*p < 32 || *p == 127) {
                char octal[5]; std::snprintf(octal, sizeof(octal), "\\%03o", *p);
                result += octal;
            } else result += static_cast<char>(*p);
        }
    }
    return result;
}
const char *concatLexem = " + ";

TVarsList *gvars = NULL;
int globalRef = 0;

TValue::TValue() {
	init();
	type = DATA_NONE;
}

TValue::TValue(TValue *val) {
	init();
	type = DATA_NONE;
	copy(val);
}

TValue::TValue(const char *val, bool code) {
	init();
	type = DATA_STR;
	value = new char[strlen(val)+1];
	strcpy((char*)value, val);
	if(code)
		flags |= FLG_CODE;
}

TValue::TValue(int val) {
	init();
	type = DATA_INT;
		value = reinterpret_cast<void*>(static_cast<intptr_t>(val));
}

TValue::TValue(void *val) {
	init();
	type = DATA_INT;
	value = val;
}

TValue::TValue(bool val) {
	init();
	type = DATA_INT;
	value = val ? (void*) 1 : NULL;
}

TValue::TValue(double val) {
	init();
	type = DATA_REAL;
	double *fl = new double;
	*fl = val;
	value = fl;
}

TValue::TValue(TScriptObject *val) {
	init();
	type = DATA_OBJECT;
	value = copyObject(val);
}

TValue::TValue(TScriptProc *val) {
	init();
	type = DATA_PROC;
	value = val;
}

TValue* TValue::fromProperty(id_element e, id_prop prop) {
	switch(cgt->propGetType(prop)) {
		case data_int:
		case data_flags:
		case data_combo:
			return new TValue(cgt->propToInteger(prop));
		case data_color:
#ifdef COLOR_INT
			return new TValue(cgt->propToInteger(prop));
#else
		{
			int color = cgt->propToInteger(prop);
			if((color & 0xFF000000) == 0xFF000000) return new TValue("");
			char buf[16];
			snprintf(buf, sizeof(buf), "#%02X%02X%02X", color & 0xFF, (color >> 8) & 0xFF, (color >> 16) & 0xFF);
			return new TValue(buf);
		}
#endif
		case data_list:
		case data_code:
			return new TValue(cgt->propToString(prop));
		case data_stock:
			return new TValue(cgt->propToString(prop));
		case data_font:
			return new TValue(cgt->propToString(prop));
		case data_pixbuf:
			return new TValue(cgt->propToString(prop));
		case data_str: {
			std::string str(cgt->propToString(prop));
			size_t start_pos = 0;
            while(!cppStringEscapes && (start_pos = str.find("\n", start_pos)) != std::string::npos) {
				str.replace(start_pos, 1, "\\n");
				start_pos += 3;
			}
			return new TValue(str.c_str());
		}
		case data_real:
			return new TValue(cgt->propToReal(prop));
		case data_comboEx:
			return new TValue(cgt->propToString(prop), true);
		case data_data: {
			id_data data = (id_data)cgt->propGetValue(prop);
			switch(cgt->dtType(data)) {
				case data_int:
					return new TValue(cgt->dtInt(data));
				case data_str:
					return new TValue(cgt->dtStr(data));
				case data_real:
					return new TValue(cgt->dtReal(data));
			}
			return new TValue(); // NULL data
		}
		case data_array: {
			id_array arr = (id_array)cgt->propGetValue(prop);
			TArrayObject *aobj = new TArrayObject();
			for(int i = 0; i < cgt->arrCount(arr); i++) {
				switch(cgt->arrType(arr)) {
					case data_int: aobj->add(new TValue(cgt->propToInteger(cgt->arrGetItem(arr, i)))); break;
					case data_str: aobj->add(new TValue(cgt->propToString(cgt->arrGetItem(arr, i)))); break;
				}
			}
			return new TValue(aobj);
		}
		case data_element:
			id_element le = cgt->propGetLinkedElement(e, cgt->propGetName(prop));
			erun->init(le);
			return new TValue(new TElementObject(le));
	}
	return new TValue();
}

TValue::~TValue() {
	clear();
	globalRef--;
}

void TValue::init() {
	val_c = NULL;
	value = NULL;
	flags = 0;
	codeType = 0;
	ref = 1;
	globalRef++;
}

void TValue::clear() {
	if (val_c)
		delete[] val_c;
	val_c = NULL;
	switch (type) {
		case DATA_REAL:
			delete (double*) value;
		case DATA_INT:
			// do nothing
			break;
		case DATA_STR:
			delete[] (char*) value;
			break;
		case DATA_ARRAY:
			delete (TArray*) value;
			break;
		case DATA_OBJECT:
			deleteObject((TScriptObject*) value);
			break;
		case DATA_PROC:
			delete (TScriptProc*) value;
		case DATA_NONE:
		case DATA_COUNT:
			// do nothing
			break;
	}
	type = DATA_NONE;
	flags = 0;
	codeType = 0;
}

void TValue::setValue(int val) {
	clear();
	type = DATA_INT;
		value = reinterpret_cast<void*>(static_cast<intptr_t>(val));
}

void TValue::setValue(double val) {
	clear();
	type = DATA_REAL;
	double *fl = new double;
	*fl = val;
	value = fl;
}

void TValue::setValue(const char *val, bool code) {
	clear();
	type = DATA_STR;
	value = new char[strlen(val)+1];
	strcpy((char*)value, val);
	if(code)
		flags |= FLG_CODE;
}

void TValue::setValueStr(char *val) {
	clear();
	type = DATA_STR;
	value = val;
}

void TValue::setValue(TScriptProc *val) {
	clear();
	type = DATA_PROC;
	value = val;
}

void TValue::setValue(TScriptObject *val) {
	clear();
	type = DATA_OBJECT;
	value = copyObject(val);
}

void TValue::makeArray() {
	clear();
	type = DATA_ARRAY;
	value = new TArray();
}

const char *TValue::toStr() {
	switch (type) {
		case DATA_INT:
			if (!val_c) {
				val_c = new char[32];
				int_to_str(reinterpret_cast<intptr_t>(value), val_c);
			}
			return val_c;
		case DATA_STR:
			return (char*) value;
		case DATA_REAL:
			if (!val_c) {
				val_c = new char[32];
				sprintf(val_c, "%g", *(double*) value);
			}
			return val_c;
		case DATA_ARRAY:
			return ((TArray*) value)->toStr();
		case DATA_OBJECT:
			return ((TScriptObject*) value)->getName()->toStr();
		case DATA_PROC: {
			return "method";
		}
		default:
			// do nothing
			break;
	}
	return "";
}

const char *TValue::toCode() {
	if(!(flags & FLG_CODE)) {
		if(type == DATA_STR) {
			if(val_c)
				delete[] val_c;
            const auto escaped = escapeCppString(static_cast<char*>(value));
            val_c = new char[escaped.size() + 2 * strlen(stringLexem) + 1];
            sprintf(val_c, "%s%s%s", stringLexem, escaped.c_str(), stringLexem);
			return val_c;
		}
		else if(type == DATA_OBJECT)
			return ((TScriptObject*) value)->getName()->toCode();
		else if(type == DATA_ARRAY)
			return ((TArray*) value)->toCode();
	}
	
	return toStr();
}

int TValue::toInt() {
	switch (type) {
		case DATA_INT:
		case DATA_PROC:
				return static_cast<int>(reinterpret_cast<intptr_t>(value));
		case DATA_STR:
			return atoi((char*) value);
		case DATA_REAL:
			return (int) (*(double*) value);
		case DATA_ARRAY:
			return 0;
		case DATA_OBJECT:
			return ((TScriptObject*) value)->getName()->toInt();
		default:
			// do nothing
			break;
	}
	return 0;
}

intptr_t TValue::toIntPtr() {
	if(type == DATA_INT || type == DATA_PROC)
		return reinterpret_cast<intptr_t>(value);
	return toInt();
}

double TValue::toReal() {
	switch (type) {
		case DATA_INT:
		case DATA_PROC:
				return static_cast<int>(reinterpret_cast<intptr_t>(value));
		case DATA_STR:
			return atof((char*) value);
		case DATA_REAL:
			return *(double*) value;
		case DATA_ARRAY:
			return 0;
		case DATA_OBJECT:
			return ((TScriptObject*) value)->getName()->toReal();
		default:
			// do nothing
			break;
	}
	return 0;
}

bool TValue::toBool() {
	switch (type) {
		case DATA_INT:
		case DATA_PROC:
			return value != NULL;
		case DATA_STR:
			return value != NULL && strlen((char*) value) > 0;
		case DATA_REAL:
			return *(double*) value != 0.0;
		case DATA_ARRAY:
			return !((TArray*) value)->empty();
		case DATA_OBJECT:
			return ((TScriptObject*) value)->getName()->toBool();
		default:
			// do nothing
			break;
	}
	return false;
}

void TValue::copy(TValue *value) {
	switch (value->type) {
		case DATA_INT:
			clear();
			type = DATA_INT;
			this->value = value->value;
			break;
		case DATA_STR:
			setValue(value->toStr());
			break;
		case DATA_REAL:
			setValue(value->toReal());
			break;
		case DATA_ARRAY:
			if (type != DATA_ARRAY)
				makeArray();
			((TArray*)this->value)->copy(value->toArr());
			break;
		case DATA_OBJECT:
			setValue(value->toScriptObj());
			break;
		case DATA_PROC:
			setValue(new TScriptProc(value->toProc()));
			break;
		default:
			type = DATA_NONE;
			break;
	}
	flags = value->flags;
	codeType = value->codeType;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TArray::~TArray() {
	clear();
}

void TArray::clear() {
	for(iterator i = begin(); i != end(); i++)
		TValue::free(*i);
	std::vector<TValue *>::clear();
}

void TArray::add(TValue *item, bool concat) {
	int index = size();
	if(item->isArray())
		merge(item->toArr());
	else
		push_back(new TValue(item));
	if(!concat) {
		TValue *v = at(index);
		v->flags |= FLG_WO_OP;
	}
}

const char *TArray::toStr() {
	buf.clear();
	for(iterator i = begin(); i != end(); i++)
		buf.append((*i)->toStr());
	return buf.c_str();
}

const char *TArray::toCode() {
	buf.clear();
//	printf("convert to code\n");
	bool open = false;
	for(int i = 0; i < size(); i++) {
		TValue *v = at(i);
		
		if(v->getType() == DATA_STR && !(v->flags & FLG_CODE)) {
			if(!open) {
				if(i && !(v->flags & FLG_WO_OP)) {
					buf.append(concatLexem);
				}
				buf.append(stringLexem);
				open = true;
			}
		}
		else if(i) {
			if(open) {
				buf.append(stringLexem);
				open = false;
			}
			if(!(v->flags & FLG_WO_OP))
				buf.append(concatLexem);
		}
        buf.append(v->getType() == DATA_STR && !(v->flags & FLG_CODE) ? escapeCppString(v->toStr()) : v->toStr());
	}
	if(open)
		buf.append(stringLexem);
	return buf.c_str();
}

void TArray::copy(TArray *arr) {
	clear();
	merge(arr);
}

void TArray::merge(TArray *arr) {
	for(int i = 0; i < arr->size(); i++)
		add(arr->at(i), true);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TVarsList::~TVarsList() {
	for (iterator v = begin(); v != end(); v++)
		delete *v;
	clear();
}

TVarItem *TVarsList::add(char *name) {
	TVarItem *item = new TVarItem(name);
	push_back(item);
	return item;
}

TVarItem *TVarsList::add(TValue *value) {
	cg_assert_type(value, NULL)
	TVarItem *item = new TVarItem();
	item->value = value;
	push_back(item);
	return item;
}

TVarItem* TVarsList::findVarByName(const char *name) {
	for (iterator v = begin(); v != end(); v++)
		if ((*v)->name.compare(name) == 0)
			return *v;
	return NULL;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TScriptProc::TScriptProc(TScriptObject *_obj, script_proc _proc) {
	obj = copyObject(_obj);
	proc = reinterpret_cast<void*>(static_cast<uintptr_t>(_proc));
	type = PROC_OBJECT;
}

TScriptProc::TScriptProc(TScriptProc *p) {
	obj = p->obj;
	proc = p->proc;
	type = p->type;
}

TScriptProc::TScriptProc(internal_proc _proc) {
	obj = NULL;
	proc = reinterpret_cast<void*>(reinterpret_cast<uintptr_t>(_proc));
	type = PROC_INTERNAL;
}

TScriptProc::TScriptProc(TTreeNode *node) {
	obj = node;
	proc = NULL;
	type = PROC_SCRIPT;
}

TScriptProc::~TScriptProc() {
	if(type == PROC_OBJECT)
		deleteObject((TScriptObject*)obj);
}

TValue *TScriptProc::run(TTreeNode *node, TArgs *args, Context &context) {
	CG_LOG_BEGIN

	TValue *val = NULL;
	switch (type) {
		case PROC_OBJECT: {
			Context c(context.element, context.entry, args, context.func);
		val = ((TScriptObject*)obj)->execMethod(node, static_cast<long>(reinterpret_cast<intptr_t>(proc)), c);
			break;
		}
		case PROC_INTERNAL:
			val = ((internal_proc)proc)(node, args, context);
			break;
		case PROC_SCRIPT: {
			Context c(context.element, context.entry, args);
			val = ((TFuncNode*) obj)->run(c);
			break;
		}
		default:
			CG_LOG_INFO("Unsupported proc type");
	}

	CG_LOG_RETURN(val)
}

TValue *TScriptProc::read(TTreeNode *node, Context &context) {
	CG_LOG_BEGIN

	if(type == PROC_OBJECT && ((TScriptObject*)obj)->isField(static_cast<script_proc>(reinterpret_cast<intptr_t>(proc)), context))
		CG_LOG_RETURN(((TScriptObject*)obj)->execMethod(node, static_cast<long>(reinterpret_cast<intptr_t>(proc)), context))
	
	CG_LOG_RETURN(new TValue(procNames[type]))
}
