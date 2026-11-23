#include "IntFunc.h"
#include "Runner.h"
#include "ElementData.h"
#include "Code.h"
#include <map>
#include <regex>
#include <string>

#define RET_EMPTY return new TValue("empty");

TUserTypes userTypes;

// ---------------------------------------------------------------- nested schemes

bool rtcgTransparent(id_element e) {
	if(!e) return false;
	const int cls = cgt->elGetClassIndex(e);
	if(cls != CI_MultiElement && cls != CI_EditMulti) return false;
	char *codePath = cgt->ReadStrParam(PARAM_CODE_PATH, e);
	// Links are resolved many times per build: remember the answer per template folder and class.
	static std::map<std::string, bool> known;
	const std::string key = std::string(codePath ? codePath : "") + "|" + cgt->elGetClassName(e);
	auto found = known.find(key);
	if(found == known.end())
		found = known.emplace(key, !(codePath && hasTemplateUnit(codePath, cgt->elGetClassName(e)))).first;
	delete[] codePath;
	return found->second;
}

// The frame (EditMulti) of a container's scheme.
static id_element frameOf(id_element container) {
	id_sdk sdk = cgt->elGetSDK(container);
	if(!sdk) return nullptr;
	for(int i = 0; i < cgt->sdkGetCount(sdk); i++) {
		id_element e = cgt->sdkGetElement(sdk, i);
		if(cgt->elGetClassIndex(e) == CI_EditMulti) return e;
	}
	return nullptr;
}

// The index-th point of a type on an element, as ptGetIndex counts them.
static id_point pointOfType(id_element e, int type, int index) {
	int k = 0;
	for(int i = 0; i < cgt->elGetPtCount(e); i++) {
		id_point p = cgt->elGetPt(e, i);
		if(cgt->ptGetType(p) == type && k++ == index) return p;
	}
	return nullptr;
}

// A frame point faces the inner scheme: its work point is the container's event point, its var point is the
// container's data point and back (ElementEditMulti::changePoints).
static int mirrorType(int type) {
	switch(type) {
		case pt_Work: return pt_Event;
		case pt_Event: return pt_Work;
		case pt_Var: return pt_Data;
		default: return pt_Var;
	}
}

id_point rtcgRealLink(id_point p) {
	id_point rp = p ? cgt->ptGetRLinkPoint(p) : nullptr;
	// Each step crosses one scheme border; the bound only stops a damaged scheme that wires a frame to itself.
	for(int step = 0; rp && step < 256; step++) {
		id_element owner = cgt->ptGetParent(rp);
		if(!rtcgTransparent(owner)) return rp;
		id_element across = cgt->elGetClassIndex(owner) == CI_MultiElement ? frameOf(owner)
			: cgt->sdkGetParent(cgt->elGetParent(owner));
		if(!across || !rtcgTransparent(across)) return rp;
		id_point mirror = pointOfType(across, mirrorType(cgt->ptGetType(rp)), cgt->ptGetIndex(rp));
		rp = mirror ? cgt->ptGetRLinkPoint(mirror) : nullptr;
	}
	return nullptr;
}

TValue *map_replace(void *node, TArgs *args, Context &context) {
	std::string str = (const char*) args->value(0)->toStr();
	std::string from = (const char*) args->value(1)->toStr();
	std::string to = (const char*) args->value(2)->toStr();
	size_t start_pos = 0;
	while((start_pos = str.find(from, start_pos)) != std::string::npos) {
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
	char *buf = new char[str.size() + 1];
	std::copy(str.begin(), str.end(), buf);
	buf[str.size()] = '\0';
	TValue *val = new TValue(buf, false);
	delete[] buf;
	CG_LOG_RETURN(val)
}

TValue *map_lower(void *node, TArgs *args, Context &context) {
	const char *src = args->value(0)->toStr();
	char *buf = new char[strlen(src)+1];
	lowerCase(buf, src);
	TValue *val = new TValue(buf, args->value(0)->flags & FLG_CODE);
	delete[] buf;
	CG_LOG_RETURN(val)
}

TValue *map_upper(void *node, TArgs *args, Context &context) {
	const char *src = args->value(0)->toStr();
	char *buf = new char[strlen(src)+1];
	upperCase(buf, src);
	TValue *val = new TValue(buf, args->value(0)->flags & FLG_CODE);
	delete[] buf;
	CG_LOG_RETURN(val)
}

TValue *map_copy(void *node, TArgs *args, Context &context) {
	std::string s = args->value(0)->toStr();
	int pos = args->value(1)->toInt();
	int len = args->value(2)->toInt();
	const std::string buf = s.substr(pos, std::max(len, 0));
	CG_LOG_RETURN(new TValue(buf.c_str(), false))
}

TValue *map_pos(void *node, TArgs *args, Context &context) {
	std::string s = args->value(0)->toStr();
	int pos = args->size() == 3 ? args->value(2)->toInt() : 0;
	CG_LOG_RETURN(new TValue((int)s.find(args->value(1)->toStr(), pos)))
}

// Delphi Delete(s, position, length): the position is 1-based; positions before the start count as 1.
TValue *map_delete(void *node, TArgs *args, Context &context) {
	std::string s = args->value(0)->toStr();
	int pos = args->value(1)->toInt();
	const int len = args->value(2)->toInt();
	if (pos < 1) pos = 1;
	if (len > 0 && pos <= static_cast<int>(s.size()))
		s.erase(pos - 1, len);
	return new TValue(s.c_str());
}

TValue *map_split(void *node, TArgs *args, Context &context) {
	std::string str = (const char*) args->value(0)->toStr();
	std::string delimiter = (const char*) args->value(1)->toStr();
	std::string token;
	size_t pos = 0;
	TArrayObject *aobj = new TArrayObject();
	while ((pos = str.find(delimiter)) != std::string::npos) {
		token = str.substr(0, pos);
		aobj->add(new TValue((char *) token.c_str()));
		str.erase(0, pos + delimiter.length());
	}
	if (str.length() > 0)
		aobj->add(new TValue((char *) str.c_str()));

	CG_LOG_RETURN(new TValue(aobj))
}

TValue *map_len(void *node, TArgs *args, Context &context) {
	std::string str = (const char*) args->value(0)->toStr();
	CG_LOG_RETURN(new TValue((int)str.length()))
}

TValue *map_print(void *node, TArgs *args, Context &context) {
	if(TBlockItemObject *block = currentBlockItem()) block->print(args);
	return new TValue();
}

TValue *map_println(void *node, TArgs *args, Context &context) {
	if(TBlockItemObject *block = currentBlockItem()) block->println(args);
	return new TValue();
}

TValue *map_trace(void *node, TArgs *args, Context &context) {	CG_LOG_BEGIN

	CALL_TRACE("*trace")
	std::string buf;
	for (TArgs::iterator a = args->begin(); a != args->end(); a++) {
		buf += (*a)->value->toStr();
		buf += " ";
	}
	cgt->trace(buf.c_str());

	CG_LOG_RETURN(args->value(0)->duplicate())
}

TValue *map_error(void *node, TArgs *args, Context &context) {
	TTreeNode *n = (TTreeNode*)node;
	while(n->parent && n->nodeType != ND_UNIT) {
		n = n->parent;
	}
	char buf[64];
	sprintf(buf, "%d", ((TCallNode*)node)->line+1);
	std::string s = ((TUnitNode*)n)->codeTree->name;
	s += "[";
	s += buf;
	s += "]: ";
	for (TArgs::iterator a = args->begin(); a != args->end(); a++) {
		s += (*a)->value->toStr();
		s += " ";
	}
	cgt->error(s.c_str());
	erun->failed = true;
	RET_EMPTY
}

TValue *map_code(void *node, TArgs *args, Context &context) {
	TValue *val = args->value(0)->duplicate();
	if(val->getType() == DATA_STR) {
		val->flags |= FLG_CODE;
	}
	else {
		TValue *v = new TValue(val->toCode(), true);
		TValue::free(val);
		val = v;
	}
	CG_LOG_RETURN(val)
}

TValue *map_string(void *node, TArgs *args, Context &context) {
	TValue *val = args->value(0)->duplicate();
	if(val->getType() == DATA_STR) {
		if(val->flags & FLG_CODE)
			val->flags ^= FLG_CODE;
	}
	else {
		TValue *v = new TValue(val->toCode());
		TValue::free(val);
		val = v;
	}
	
	CG_LOG_RETURN(val)
}

TValue *map_number(void *node, TArgs *args, Context &context) {
	CG_LOG_RETURN(new TValue(args->at(0)->value->toReal()))
}

int loopCounter = 0;

// The name:section argument built by the parser; the section is applied by map_event.
TValue *map_section_arg(void *node, TArgs *args, Context &context) {
	std::string value(args->value(0)->toStr());
	value += ':';
	value += args->value(1)->toStr();
	return new TValue(value.c_str());
}

TValue *map_event(void *node, TArgs *args, Context &context) {
	CG_LOG_BEGIN

	CALL_TRACE_BEGIN("*event")
	// "name:section" runs the handler in that language section; the section is restored after it.
	std::string spec(args->at(0)->value->toStr());
	const size_t colon = spec.find(':');
	struct SectionRestore { int saved; ~SectionRestore() { cgUseSection(saved); } } restore{cgSection};
	if (colon != std::string::npos) {
		const int sec = cgSectionByName(spec.substr(colon + 1).c_str());
		spec.erase(colon);
		if (sec >= 0) cgUseSection(sec);
	}
	const char *pname = spec.c_str();
	id_point p = cgt->elGetPtName(context.element, pname);
	if(p) {
		id_point rp = rtcgRealLink(p);
		if (rp) {
			id_element pe = cgt->ptGetParent(rp);
			int type = cgt->ptGetDataType(p);
			TArgs *_args = new TArgs;
			// dynamic point index
			const char *event_name = cgt->pt_dpeGetName(rp);
			if(strlen(event_name) == 0)
				event_name = cgt->ptGetName(rp);
			else {
				if(cgt->elGetClassIndex(cgt->ptGetParent(rp)) == CI_DPLElement) {
					_args->add(new TValue(cgt->ptGetName(rp)));
				}
				else {
					_args->add(new TValue(cgt->ptGetIndex(rp)));
				}
			}
			// event args
			for (int i = 1; i < args->size(); i++) {
				TValue *v = new TValue(args->value(i));
				if(type)
					v->setCodeType(type);
				_args->add(v);
			}
			
#ifdef BUILDING_DLL
			std::string event_name_copy(event_name ? event_name : "");
			event_name = const_cast<char *>(event_name_copy.c_str());
#endif
			TValue *ret;
			if(loopCounter < 256) {
				loopCounter++;
				ret = erun->run(pe, event_name, _args);
				loopCounter--;
			}
			else {
				char buf[256];
				sprintf(buf, "Loop detected: %s.%s -> %s, %d", cgt->elGetClassName(context.element), pname, event_name, loopCounter);
				cgt->error(buf);
				ret = new TValue();
				loopCounter = 0;
			}
			delete _args;
			
			CALL_TRACE_END("*event")
			CG_LOG_RETURN(ret)
		}
	}
	else {
		CG_LOG_INFO("Point not found ")
	}

	CG_LOG_RETURN(new TValue())
}

TValue *map_typeof(void *node, TArgs *args, Context &context) {
	return new TValue(args->at(0)->value->flags & FLG_CODE ? 0 : args->at(0)->value->getType());
}

TValue *map_expof(void *node, TArgs *args, Context &context) {
	return new TValue(args->at(0)->value->getCodeType());
}

TValue *map_isnull(void *node, TArgs *args, Context &context) {
	return new TValue(args->at(0)->value->getType() == DATA_NONE ? 1 : 0);
}

TValue *convert(TValue *val, int type, Context &context) {
	if(type == 0 || !erun || !erun->tools)
		return val;
	
	TArgs *args = new TArgs();
	args->add(val);
	args->add(new TValue(type));
	val = erun->tools->run(context.element, "to_type", args);
	delete args;
	
	return val;
}

TValue *map_d(void *node, TArgs *args, Context &context) {
	CG_LOG_BEGIN

	CALL_TRACE("*d")
	const char *name = args->value(0)->toStr();
	id_point p = cgt->elGetPtName(context.element, name);
	int type = 0;
	if(p && cgt->ptGetType(p) == pt_Event) {
		CG_LOG_RETURN(new TValue(cgt->ptGetName(p)))
	}
	if(p && cgt->ptGetType(p) == pt_Data) {
		type = cgt->ptGetDataType(p);
		id_point link = rtcgRealLink(p);
		if(link) {
			TArgs *args = new TArgs();
			const char *event_name = cgt->pt_dpeGetName(link);
			if(strlen(event_name) == 0)
				event_name = cgt->ptGetName(link);
			else {
				if(cgt->elGetClassIndex(cgt->ptGetParent(link)) == CI_DPLElement) {
					args->add(new TValue(cgt->ptGetName(link)));
				}
				else {
					args->add(new TValue(cgt->ptGetIndex(link)));
				}
			}
#ifdef BUILDING_DLL
			std::string event_name_copy(event_name ? event_name : "");
			event_name = const_cast<char *>(event_name_copy.c_str());
#endif
			TValue *ret = erun->run(cgt->ptGetParent(link), event_name, args);
			delete args;
			int t = cgt->ptGetDataType(link);
			if(t)
				ret->setCodeType(t);
			
			CG_LOG_RETURN(convert(ret, type, context))
		}
	}

	id_prop prop = cgt->getPropByName(context.element, name);
	if(prop && (!cgt->isDefProp(context.element, prop) || !context.haveData()))
		CG_LOG_RETURN(TValue::fromProperty(context.element, prop))

	CG_LOG_RETURN(convert(context.data(), type, context))
}

TValue *map_linked(void *node, TArgs *args, Context &context) {
	id_point p = cgt->elGetPtName(context.element, args->value(0)->toStr());
	bool result = p && rtcgRealLink(p) != nullptr;
	if(p)
		return new TValue(result);
	return new TValue(false);
}

static TValue *map_mt_arg(void *, TArgs *args, Context &context) {
	const int index = args->value(0)->toInt();
	if(index < 0 || index >= context.incomingArgs->size()) return new TValue();
	return context.incomingArgs->value(index)->duplicate();
}

static TValue *map_mt_count(void *, TArgs *, Context &context) {
	return new TValue(static_cast<int>(context.incomingArgs->size()));
}

TValue *map_isdef(void *node, TArgs *args, Context &context) {
	id_prop prop = cgt->getPropByName(context.element, args->value(0)->toStr());
	if(prop)
		CG_LOG_RETURN((new TValue(cgt->isDefProp(context.element, prop))))
	return new TValue(false);
}

static bool valueIsEmpty(TValue *value) {
	switch(value->getType()) {
		case DATA_STR:
			return value->toStr()[0] == '\0';
		case DATA_INT:
		case DATA_REAL:
		case DATA_ARRAY:
			return false;
		default:
			return true;
	}
}

TValue *map_isndef(void *node, TArgs *args, Context &context) {
	TValue *value = map_isdef(node, args, context);
	bool defined = value->toBool();
	TValue::free(value);
	return new TValue(!defined);
}

TValue *map_isset(void *node, TArgs *args, Context &context) {
	if(args->size() == 0) return new TValue(false);
	return new TValue(!valueIsEmpty(args->value(0)));
}

TValue *map_isnset(void *node, TArgs *args, Context &context) {
	TValue *value = map_isset(node, args, context);
	bool set = value->toBool();
	TValue::free(value);
	return new TValue(!set);
}

TValue *map_isprop(void *node, TArgs *args, Context &context) {
	id_prop prop = cgt->getPropByName(context.element, args->value(0)->toStr());
	return new TValue(prop != NULL);
}

TValue *map_issec(void *node, TArgs *args, Context &context) {
	const char *name = args->value(0)->toStr();
	if(cgSections.empty()) return new TValue(false);
	const char *current = cgSections[cgSection].name.c_str();
	return new TValue(strcasecmp(name, current) == 0);
}

TValue *map_count(void *node, TArgs *args, Context &context) {
	if(args->size() == 0) return new TValue(0);
	TValue *value = args->value(0);
	if(value->getType() == DATA_ARRAY)
		return new TValue(static_cast<int>(value->toArr()->size()));
	return new TValue(1);
}

static TValue *wrapCode(const char *func, TValue *value) {
	std::string code = func;
	code += "(";
	code += value->toCode();
	code += ")";
	return new TValue(code.c_str(), true);
}

TValue *map_e_int(void *node, TArgs *args, Context &context) {
	if(args->size() == 0) return new TValue(0);
	TValue *value = args->value(0);
	if(value->getType() == DATA_STR && (value->flags & FLG_CODE)) {
		if(value->getCodeType() == DATA_STR) return wrapCode("str_to_int", value);
		if(value->getCodeType() == DATA_REAL) return wrapCode("real_to_int", value);
		return value->duplicate();
	}
	return new TValue(value->toInt());
}

TValue *map_e_str(void *node, TArgs *args, Context &context) {
	if(args->size() == 0) return new TValue("");
	TValue *value = args->value(0);
	if(value->getType() == DATA_STR && (value->flags & FLG_CODE)) {
		if(value->getCodeType() == DATA_INT) return wrapCode("int_to_str", value);
		if(value->getCodeType() == DATA_REAL) return wrapCode("real_to_str", value);
		return value->duplicate();
	}
	if(value->getType() == DATA_INT || value->getType() == DATA_REAL)
		return new TValue(value->toStr());
	return value->duplicate();
}

TValue *map_e_real(void *node, TArgs *args, Context &context) {
	if(args->size() == 0) return new TValue(0.0);
	TValue *value = args->value(0);
	if(value->getType() == DATA_STR && (value->flags & FLG_CODE)) {
		if(value->getCodeType() == DATA_STR) return wrapCode("str_to_real", value);
		return value->duplicate();
	}
	return new TValue(value->toReal());
}

TValue *map_sub(void *node, TArgs *args, Context &context) {
	if(args->size() == 0)
		return new TValue(0);
	
	TValue *var = args->value(0);
	if(args->size() == 1)
		return new TValue(var->getCodeType());
	
	var->setCodeType(args->value(1)->toInt());
	
	return var->duplicate();
}

TValue *map_get(void *node, TArgs *args, Context &context) {
	if(args->size() == 0)
		return new TValue();
	
	const char* name = args->value(0)->toStr();
	if(strcmp(name, "_data_") == 0) {
		if(context.func && context.func->args->size() > 0)
			return context.func->args->value(0)->duplicate();
		return new TValue();
	}
	if(strcmp(name, "_event_count_") == 0 || strcmp(name, "_work_count_") == 0 ||
	   strcmp(name, "_data_count_") == 0 || strcmp(name, "_var_count_") == 0) {
		const int type = name[1] == 'e' ? pt_Event : name[1] == 'w' ? pt_Work :
		                 name[1] == 'd' ? pt_Data : pt_Var;
		int total = 0;
		for(int i = 0; i < cgt->elGetPtCount(context.element); i++)
			if(cgt->ptGetType(cgt->elGetPt(context.element, i)) == type) total++;
		return new TValue(total);
	}
	if(strcmp(name, "_id_") == 0)
		return new TValue((int)cgt->elGetEID(context.element));
	// is local var
	TVarItem *item = context.func->vars->findVarByName(name);
	if(item)
		return item->value->duplicate();
	
	// is func args
	item = context.func->args->findVarByName(name);
	if(item)
		return item->value->duplicate();

	// is global var (globals may be declared after the referring function)
	if(gvars) {
		item = gvars->findVarByName(name);
		if(item) {
			return item->value->duplicate();
		}
	}
	
	// is element property
	id_prop prop = cgt->getPropByName(context.element, name);
	if(prop) {
		return TValue::fromProperty(context.element, prop);
	}

	// is element point
	id_point point = cgt->elGetPtName(context.element, name);
	if(point) {
		const int pointType = cgt->ptGetType(point);
		if(pointType == pt_Event)
			return new TValue(cgt->ptGetName(point));
		if(pointType == pt_Data) {
			id_point link = rtcgRealLink(point);
			if(link) {
				TArgs *linkArgs = new TArgs();
				const char *event_name = cgt->pt_dpeGetName(link);
				if(strlen(event_name) == 0) event_name = cgt->ptGetName(link);
				else if(cgt->elGetClassIndex(cgt->ptGetParent(link)) == CI_DPLElement)
					linkArgs->add(new TValue(cgt->ptGetName(link)));
				else linkArgs->add(new TValue(cgt->ptGetIndex(link)));
				TValue *ret = erun->run(cgt->ptGetParent(link), event_name, linkArgs);
				delete linkArgs;
				return ret;
			}
		}
	}
	
	// is this field
	TElementObject *obj = new TElementObject(context.element);
	script_proc proc = obj->getProc(args->value(0), context);
	if(proc != -1)
		return obj->execMethod((TTreeNode*)node, proc, context);
	
	return new TValue();
}

char *_stringLexem = NULL;
char *_concatLexem = NULL;

void init_buffer(char* &buf, const char *value) {
	if(buf)
		delete[] buf;
	int len = strlen(value);
	buf = new char[len+1];
	buf[len] = '\0';
	strcpy(buf, value);
}

TValue *map_param(void *node, TArgs *args, Context &context) {
	const char* name = args->value(0)->toStr();
    if(strcmp(name, "cpp_string_escapes") == 0) {
        if(args->size() == 1) return new TValue(cppStringEscapes);
        cppStringEscapes = args->value(1)->toBool();
    }
	if(strcmp(name, "string_lexem") == 0) {
		if(args->size() == 1)
			return new TValue(stringLexem, true);
		
		init_buffer(_stringLexem, args->value(1)->toStr());
		stringLexem = _stringLexem;
	}
	if(strcmp(name, "concat_lexem") == 0) {
		if(args->size() == 1)
			return new TValue(concatLexem, true);
		
		init_buffer(_concatLexem, args->value(1)->toStr());
		concatLexem = _concatLexem;
	}
	return new TValue();
}

TValue *map_regex_replace(void *node, TArgs *args, Context &context) {
	std::regex r(args->value(1)->toStr());
	std::string result = std::regex_replace(args->value(0)->toStr(), r, args->value(2)->toStr());
	return new TValue((char*)result.c_str());
}

TValue *map_fopen(void *node, TArgs *args, Context &context) {

	RET_EMPTY
}

TValue *map_fputs(void *node, TArgs *args, Context &context) {

	RET_EMPTY
}

TValue *map_fgets(void *node, TArgs *args, Context &context) {

	RET_EMPTY
}

TValue *map_fclose(void *node, TArgs *args, Context &context) {

	RET_EMPTY
}

TValue *map_project_dir(void *node, TArgs *args, Context &context) {
	char *s = cgt->ReadStrParam(PARAM_PROJECT_NAME, context.element);
	std::string p(s);
	int pos = p.find_last_of('/');
	TValue *val = new TValue(p.substr(0, pos+1).c_str(), true);
	delete[] s;
	return val;
}

TValue *map_gui_project(void *node, TArgs *args, Context &context) {
	id_sdk sdk = (id_sdk)cgt->elGetSDK(context.element);
	for(int depth = 0; sdk && depth < 16; depth++) {
		id_element parent = cgt->sdkGetParent(sdk);
		if(!parent) break;
		sdk = (id_sdk)cgt->elGetSDK(parent);
	}
	if(sdk) {
		int count = cgt->sdkGetCount(sdk);
		for(int i = 0; i < count; i++) {
			id_element e = cgt->sdkGetElement(sdk, i);
			if(!e) continue;
			const char *cls = cgt->elGetClassName(e);
			if(cls && (!strcmp(cls, "MainForm") || !strcmp(cls, "Gui")))
				return new TValue(true);
		}
	}
	return new TValue(false);
}

TValue *map_qt_project(void *, TArgs *, Context &context) {
	id_sdk sdk = cgt->elGetParent(context.element);
	while(sdk) {
		id_element parent = cgt->sdkGetParent(sdk);
		if(!parent) break;
		sdk = cgt->elGetParent(parent);
	}
	for(int i = 0; sdk && i < cgt->sdkGetCount(sdk); ++i) {
		id_element element = cgt->sdkGetElement(sdk, i);
		if((cgt->elGetFlag(element) & ELEMENT_FLG_IS_PARENT) && !strcmp(cgt->elGetClassName(element), "MainForm")) {
			id_prop backend = cgt->getPropByName(element, "Backend");
			return new TValue(backend && cgt->propToInteger(backend) == 1);
		}
	}
	return new TValue(false);
}

TValue *map_cstr(void *node, TArgs *args, Context &context) {
	if(args->size() == 0) return new TValue("\"\"", true);
	TValue *value = args->value(0);
	if(value->getType() == DATA_NONE) return new TValue("\"\"", true);
	const char *code = value->toCode();
	return new TValue(code && *code ? code : "\"\"", true);
}

TValue *map_project_name(void *node, TArgs *args, Context &context) {
	char *s = cgt->ReadStrParam(PARAM_PROJECT_NAME, context.element);
	std::string p(s);
	int pos = p.find_last_of('/');
	TValue *val = new TValue(p.substr(pos + 1, p.length() - pos - 5).c_str(), true);
	delete[] s;
	return val;
}

const TFuncMap func_map[] = {
	// ------------ STRINGS ----------------------------
	{ "replace", 3, map_replace, "str, dst, src"},
	{ "lower", 1, map_lower, "str"},
	{ "upper", 1, map_upper, "str"},
	{ "copy", 3, map_copy, "str, position, length"},
	{ "pos", 3, map_pos, "substr, str, start"},
	{ "delete", 3, map_delete, "str, position, length"},
	{ "split", 2, map_split, "str, delimiter"},
	{ "len", 1, map_len, "str"},

	// ------------ SYSTEM -----------------------------
	{ "print", -1, map_print, "text"},
	{ "println", -1, map_println, "text"},
	{ "trace", -1, map_trace, "text"},
	{ "error", 1, map_error, "text"},
	{ "code", 1, map_code, "string"},
	{ "string", 1, map_string, "string"},
	{ "number", 1, map_number, "string"},
	{ "event", -1, map_event, "ename[, args, ...]"},
	{ "typeof", 1, map_typeof, "var"},
	{ "expof", 1, map_expof, "var"},
	{ "isnull", 1, map_isnull, "var"},
	{ "d", 1, map_d, "name"},
	{ "mt_arg", 1, map_mt_arg, "index"},
	{ "mt_count", 0, map_mt_count, ""},
	{ "linked", 1, map_linked, "name"},
	{ "isdef", 1, map_isdef, "name"},
	{ "isndef", 1, map_isndef, "name"},
	{ "isset", 1, map_isset, "var"},
	{ "isnset", 1, map_isnset, "var"},
	{ "isprop", 1, map_isprop, "name"},
	{ "issec", 1, map_issec, "section"},
	{ "str", 1, map_string, "value"},
	{ "count", 1, map_count, "array"},
	{ "point", 1, map_get, "name"},
	{ "e_int", 1, map_e_int, "value"},
	{ "e_str", 1, map_e_str, "value"},
	{ "e_real", 1, map_e_real, "value"},
	{ "sub", -1, map_sub, "name[, type]"},
	{ "get", 1, map_get, "name"},
	{ "param", 1, map_param, "name[,value]"},
	
	{ "regex_replace", 3, map_regex_replace, "source, expression, format"},

	// ------------ FILES ------------------------------
	{ "fopen", 2, map_fopen, "filename, mode"},
	{ "fputs", 2, map_fputs, "id, str"},
	{ "fgets", 1, map_fgets, "id"},
	{ "fclose", 1, map_fclose, "id"},

	// ------------ ENVEROUMENT ------------------------
	{ "project_dir", 0, map_project_dir, ""},
	{ "project_name", 0, map_project_name, ""},
	{ "is_gui_project", 0, map_gui_project, ""},
	{ "is_qt_project", 0, map_qt_project, ""},
	{ "cstr", 1, map_cstr, "value"}
};

const int func_map_size = sizeof(func_map) / sizeof(func_map[0]);
