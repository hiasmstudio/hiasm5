#include <string>
#include <list>
#include <stdexcept>
#include <memory>

#include "Objects.h"
#include "IntFunc.h"
#include "ElementData.h"
#include "Runner.h"

TScriptObject **objs = NULL;
int count = 0;

TBlockObject *cgBlockObject = NULL;
id_sdk cgSdk = NULL;

TBlockItemObject *currentBlockItem() {
	return cgBlockObject ? cgBlockObject->currentBlock() : NULL;
}

#define RET_EMPTY return new TValue("");

//#ifdef BUILDING_DLL
//#define _NEW_LINE_ "\r\n"
//#else
#define _NEW_LINE_ "\n"
//#endif

#define OBJ_COUNT 1

typedef struct _TObjectInstaces {
	const char *name;
	TScriptObject* (*createInstance)();
	
	_TObjectInstaces(const char *name, TScriptObject* (*createInstance)()) {
		this->name = name;
		this->createInstance = createInstance;
	}
} TObjectInstaces;

TObjectInstaces objInstances[OBJ_COUNT] = {
	TObjectInstaces("array", TArrayObject::createInstance)
};

TScriptObject::TScriptObject(const char *_name) {
	name = new char[strlen(_name) + 1];
	strcpy(name, _name);
	ref = 0;
	nameValue = NULL;
	mtdNames = NULL;
	count = 0;
}

TScriptObject::~TScriptObject() {
	free(mtdNames);
	delete[] name;
	TValue::free(nameValue);
}

TValue* TScriptObject::getName() {
	if(!nameValue)
		nameValue = new TValue(name);
	return nameValue;
}

TSORecord TScriptObject::makeMethod(const char *n, int c) {
	TSORecord r;
	r.name = n;
	r.count = c;
	r.field = false;
	return r;
}

TSORecord TScriptObject::makeField(const char *n) {
	TSORecord r;
	r.name = n;
	r.count = 0;
	r.field = true;
	return r;
}

script_proc TScriptObject::getProc(TValue *val, Context &context) {
	const char *fld = val->toStr();

	for (int i = 0; i < count; i++) {
		if (strcasecmp(fld, mtdNames[i].name) == 0) {
			return i;
		}
	}

	return -1;
}

bool TScriptObject::isField(script_proc index, Context &context) {
	if(index < count)
		return mtdNames[index].field;
	
	return false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TBlockItem::TBlockItem(const char *name) {
	items = NULL;
	count = 0;
	expand = 0;

	this->name = new char[strlen(name) + 1];
	strcpy(this->name, name);
}

TBlockItem::~TBlockItem() {
	for (int i = 0; i < count; i++)
		TValue::free(items[i]);
	free(items);
	delete[] name;
}

void TBlockItem::clear() {
	for (int i = 0; i < count; i++)
		TValue::free(items[i]);
	free(items);
	items = NULL;
	count = 0;
	expand = 0;
}

void TBlockItem::addValue(TValue *item) {
	count++;
	if (count > expand) {
		expand = (int) (1.5 * expand + 1);
		items = (TValue**) realloc(items, sizeof (TValue*) * expand);
	}
	items[count - 1] = item;
}

std::string TBlockItem::asText() {
	std::string str;
	for (int i = 0; i < count; i++)
		str.append(items[i]->toStr());
	return str;
}

std::string TBlockItem::asCode() {
	std::string str;
	for (int i = 0; i < count; i++)
		str.append(items[i]->toCode());
	return str;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TBlockItemObject::TBlockItemObject(const char *name, TBlockObject *owner) : TScriptObject(name) {
	CG_LOG_BEGIN

	ref = 0;
	count = 18;
	firstPrint = true;
	levelEnabled = true;
	this->owner = owner;
	mtdNames = (TSORecord*) malloc(sizeof (TSORecord) * count);
	mtdNames[0] = makeMethod("print", -1);
	mtdNames[1] = makeMethod("println", -1);
	mtdNames[2] = makeMethod("astext", 0);
	mtdNames[3] = makeMethod("save", 1);
	mtdNames[4] = makeMethod("inc", 0);
	mtdNames[5] = makeMethod("dec", 0);
	mtdNames[6] = makeMethod("ascode", 0);
	mtdNames[7] = makeMethod("copyhere", 1);
	mtdNames[8] = makeMethod("empty", 0);
	mtdNames[9] = makeMethod("build", 0);
	mtdNames[10] = makeField("name");
	mtdNames[11] = makeMethod("load", 1);
	mtdNames[12] = makeMethod("inclvl", 0);
	mtdNames[13] = makeMethod("declvl", 0);
	mtdNames[14] = makeMethod("getlvl", 0);
	mtdNames[15] = makeMethod("lvloff", 0);
	mtdNames[16] = makeMethod("lvlon", 0);
	mtdNames[17] = makeMethod("lvlenabled", 0);

	block = new TBlockItem(name);
}

TBlockItemObject::~TBlockItemObject() {
	delete block;
	CG_LOG_END
}

void TBlockItemObject::print(TArgs *args) {
	if(firstPrint && levelEnabled && level.size()) {
		block->addValue(new TValue(level.c_str(), true));
		firstPrint = false;
	}
	for(TArgs::iterator a = args->begin(); a != args->end(); a++)
		block->addValue(new TValue((*a)->value));
}

void TBlockItemObject::println(TArgs *args) {
	print(args);
	block->addValue(new TValue(_NEW_LINE_, true));
	firstPrint = true;
}

void TBlockItemObject::copyHere(TArgs *args) {
	TBlockItemObject *blk = dynamic_cast<TBlockItemObject *>(args->value(0)->toScriptObj());
	if(!blk && owner) blk = owner->searchBlock(args->value(0)->toStr());
	if(blk) {
		for(int i = 0; i < blk->block->count; i++)
			block->addValue(blk->block->items[i]->duplicate());
	}
}

void TBlockItemObject::copyTo(TArgs *args) {
	if(!owner) return;
	TBlockItemObject *blk = owner->searchBlock(args->value(0)->toStr());
	if(!blk || blk == this) return;
	for(int i = 0; i < block->count; i++)
		blk->block->addValue(block->items[i]->duplicate());
}

void TBlockItemObject::mergeFrom(TArgs *args) {
	if(!owner) return;
	TBlockItemObject *blk = owner->searchBlock(args->value(0)->toStr());
	if(!blk || blk == this) return;
	for(int i = 0; i < blk->block->count; i++)
		block->addValue(blk->block->items[i]->duplicate());
}

void TBlockItemObject::incLevel() {
	// Two spaces per level, as the HiAsm 4 web codegen indents (FTCG lsize = 2).
	level.append("  ");
}

void TBlockItemObject::decLevel() {
	if(level.size() >= 2)
		level.resize(level.size() - 2);
}

void TBlockItemObject::save(const char *fname, bool resource = true) {
	std::unique_ptr<FILE, decltype(&fclose)> file(openUtf8(fname, "wb"), &fclose);
	if(!file) throw std::runtime_error(std::string("Cannot write generated block: ") + fname);
	std::string text = block->asText();
	if(fwrite(text.data(), 1, text.size(), file.get()) != text.size() || fflush(file.get()))
		throw std::runtime_error(std::string("Cannot write generated block: ") + fname);
	if(resource) {
		cgt->resAddFile(fname);
	}
}

void TBlockItemObject::load(const char *fname) {
	std::unique_ptr<FILE, decltype(&fclose)> file(openUtf8(fname, "rb"), &fclose);
	if(!file) throw std::runtime_error(std::string("Cannot read block file: ") + fname);
	std::string text;
	char buffer[4096];
	size_t length;
	while((length = fread(buffer, 1, sizeof(buffer), file.get())) != 0) text.append(buffer, length);
	if(ferror(file.get())) throw std::runtime_error(std::string("Cannot read block file: ") + fname);
	block->clear();
	block->addValue(new TValue(text.c_str(), true));
}

TValue *TBlockItemObject::execMethod(TTreeNode *node, long index, Context &context) {
	CG_LOG_BEGIN

	CG_LOG_INFO(mtdNames[index].name)
	switch(index) {
		case 0:
			print(context.args);
			break;
		case 1:
			println(context.args);
			break;
		case 2:
			CG_LOG_RETURN(new TValue(block->asText().c_str()))
		case 3: {
			char *s = cgt->ReadStrParam(PARAM_CODE_PATH, context.element);
			std::string file(s);
			delete[] s;
			file.append(context.args->value(0)->toStr());
			save(file.c_str());
			break;
		}
		case 4:
			incLevel();
			break;
		case 5:
			decLevel();
			break;
		case 6:
			CG_LOG_RETURN(new TValue(block->asCode().c_str(), true))
		case 7:
			copyHere(context.args);
			break;
		case 8:
			CG_LOG_RETURN(new TValue(block->count == 0))
		case 9: {
			char *s = cgt->ReadStrParam(PARAM_PROJECT_NAME, context.element);
			std::string p(s);
			int pos = p.find_last_of('/');
			std::string file(p.substr(0, pos+1).c_str());
			delete[] s;
			file.append(context.args->value(0)->toStr());
			save(file.c_str(), false);
			break;
		}
		case 10:
			CG_LOG_RETURN(new TValue(block->name))
		case 11: {
			char *s = cgt->ReadStrParam(PARAM_CODE_PATH, context.element);
			std::string file(s);
			delete[] s;
			file.append(context.args->value(0)->toStr());
			load(file.c_str());
			break;
		}
		case 12:
			incLevel();
			break;
		case 13:
			decLevel();
			break;
		case 14:
			CG_LOG_RETURN(new TValue(getLevel()))
		case 15:
			levelOff();
			break;
		case 16:
			levelOn();
			break;
		case 17:
			CG_LOG_RETURN(new TValue(levelEnabledFlag()))
	}
	
	CG_LOG_RETURN(new TValue((TScriptObject *)this))
}

TValue *TBlockItemObject::putStream(TTreeNode *node, TValue *value, Context &context) {
	TArgs *args = new TArgs();
    args->add(value->duplicate());
	print(args);
	delete args;
	
	return new TValue((TScriptObject *)this);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TBlockObject::TBlockObject() : TScriptObject("block") {
	CG_LOG_BEGIN

	ref = -1;
	count = 20;
	current = NULL;
	mtdNames = (TSORecord*) malloc(sizeof (TSORecord) * count);
	mtdNames[0] = makeMethod("reg", 1);
	mtdNames[1] = makeMethod("reggen", 0);
	mtdNames[2] = makeMethod("delete", 1);
	mtdNames[3] = makeMethod("get", 1);
	mtdNames[4] = makeMethod("select", 1);
	mtdNames[5] = makeMethod("cur", 0);
	mtdNames[6] = makeMethod("copyto", 1);
	mtdNames[7] = makeMethod("mergefrom", 1);
	mtdNames[8] = makeMethod("inclvl", 0);
	mtdNames[9] = makeMethod("declvl", 0);
	mtdNames[10] = makeMethod("getlvl", 0);
	mtdNames[11] = makeMethod("lvloff", 0);
	mtdNames[12] = makeMethod("lvlon", 0);
	mtdNames[13] = makeMethod("lvlenabled", 0);
	mtdNames[14] = makeMethod("isempty", 1);
	mtdNames[15] = makeMethod("copyhere", 1);
	mtdNames[16] = makeMethod("empty", 0);
	mtdNames[17] = makeMethod("astext", 0);
	mtdNames[18] = makeMethod("ascode", 0);
	mtdNames[19] = makeMethod("save", 1);
	
	regIndex = 1;
}

TBlockObject::~TBlockObject() {
	for (TBlockItems::iterator block = items.begin(); block != items.end(); block++)
		deleteObject(*block);
	items.clear();
	
	CG_LOG_END
}

TBlockItemObject *TBlockObject::regBlock(const char *name) {
	TBlockItemObject *blk = searchBlock(name);
	if(!blk)
		blk = new TBlockItemObject(name, this);
	items.push_back((TBlockItemObject *)copyObject(blk));
	if(!current) current = blk;
	return blk;
}

TBlockItemObject *TBlockObject::searchBlock(const char *name) {
	for (TBlockItems::iterator block = items.begin(); block != items.end(); block++)
		if((*block)->isObject(name))
			return *block;
	return NULL;
}

bool TBlockObject::removeBlock(const char *name) {
	TBlockItemObject *blk = searchBlock(name);
	if(blk) {
		deleteObject(blk);
		items.remove(blk);
		return true;
	}
	return false;
}

TValue *TBlockObject::execMethod(TTreeNode *node, long index, Context &context) {
	CG_LOG_BEGIN

	CG_LOG_INFO(mtdNames[index].name)
	TArgs *args = context.args;
	switch(index) {
		case 0:
			CG_LOG_RETURN(new TValue(regBlock(args->value(0)->toStr())))
		case 1: {
			char buf[64];
			sprintf(buf, "block_%d", regIndex++);
			CG_LOG_RETURN(new TValue(regBlock(buf)))
		}
		case 2:
			CG_LOG_RETURN(new TValue(removeBlock(args->value(0)->toStr())))
		case 3: {
			TBlockItemObject *blk = searchBlock(args->value(0)->toStr());
			if(blk)
				CG_LOG_RETURN(new TValue(blk))
			CG_LOG_RETURN(new TValue(0))
		}
		case 4: {
			TValue *old = current ? current->getName()->duplicate() : new TValue("");
			selectBlock(args->value(0)->toStr());
			CG_LOG_RETURN(old)
		}
		case 5:
			CG_LOG_RETURN(current ? current->getName() : new TValue(""))
		case 6:
			if(current) current->copyTo(args);
			break;
		case 7:
			if(current) current->mergeFrom(args);
			break;
		case 8:
			if(current) current->incLevel();
			break;
		case 9:
			if(current) current->decLevel();
			break;
		case 10:
			CG_LOG_RETURN(new TValue(current ? current->getLevel() : 0))
		case 11:
			if(current) current->levelOff();
			break;
		case 12:
			if(current) current->levelOn();
			break;
		case 13:
			CG_LOG_RETURN(new TValue(current && current->levelEnabledFlag()))
		case 14: {
			TBlockItemObject *blk = searchBlock(args->value(0)->toStr());
			CG_LOG_RETURN(new TValue(!blk || blk->block->count == 0))
		}
		case 15:
			if(current) current->copyHere(args);
			break;
		case 16:
			CG_LOG_RETURN(new TValue(!current || current->block->count == 0))
		case 17:
			CG_LOG_RETURN(new TValue(current ? current->block->asText().c_str() : ""))
		case 18:
			CG_LOG_RETURN(new TValue(current ? current->block->asCode().c_str() : "", true))
		case 19:
			if(current) current->saveToFile(args->value(0)->toStr(), true);
			break;
	}

	CG_LOG_RETURN(new TValue(this))
}

TValue *TBlockObject::selectBlock(const char *name) {
	TBlockItemObject *blk = searchBlock(name);
	if(blk) current = blk;
	return new TValue(blk != NULL);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TCGTObject::TCGTObject() : TScriptObject("cgt") {
	ref = -1;
	count = 21;
	mtdNames = (TSORecord*) malloc(sizeof (TSORecord) * count);
	mtdNames[0] = makeMethod("pt_get_parent", 1);
	mtdNames[1] = makeMethod("pt_get_link_point", 1);
	mtdNames[2] = makeMethod("pt_get_rlink_point", 1);
	mtdNames[3] = makeMethod("pt_get_name", 1);
	mtdNames[4] = makeMethod("pt_get_name_byid", 1);
	mtdNames[5] = makeMethod("pt_arr_work", 1);
	mtdNames[6] = makeMethod("pt_arr_event", 1);
	mtdNames[7] = makeMethod("pt_arr_var", 1);
	mtdNames[8] = makeMethod("pt_arr_data", 1);
	mtdNames[9] = makeMethod("pt_get_type", 1);
	mtdNames[10] = makeMethod("pt_get_data_type", 1);
	mtdNames[11] = makeMethod("el_get_class_name", 1);
	mtdNames[12] = makeMethod("el_get_code_name", 1);
	mtdNames[13] = makeMethod("el_get_child_id", 1);
	mtdNames[14] = makeMethod("el_get_parent_id", 0);
	mtdNames[15] = makeMethod("el_get_point_name", 2);
	mtdNames[16] = makeMethod("el_link_is", 0);
	mtdNames[17] = makeMethod("el_link_main", 0);
	mtdNames[18] = makeMethod("el_get_parent", 1);
	mtdNames[19] = makeMethod("get_point_id", 1);
	mtdNames[20] = makeMethod("el_get_child_count", 0);
}

static id_point pointByType(id_element element, int type, int index) {
	if(!element) return NULL;
	int found = 0;
	for(int i = 0; i < cgt->elGetPtCount(element); i++) {
		id_point p = cgt->elGetPt(element, i);
		if(cgt->ptGetType(p) == type && found++ == index) return p;
	}
	return NULL;
}

TValue *TCGTObject::execMethod(TTreeNode *node, long index, Context &context) {
	TArgs *args = context.args;
	switch(index) {
		case 0: {
			id_point p = (id_point)args->value(0)->toIntPtr();
			return new TValue((void *)cgt->ptGetParent(p));
		}
		case 1: {
			id_point p = (id_point)args->value(0)->toIntPtr();
			return new TValue((void *)cgt->ptGetLinkPoint(p));
		}
		case 2: {
			id_point p = (id_point)args->value(0)->toIntPtr();
			return new TValue((void *)cgt->ptGetRLinkPoint(p));
		}
		case 3: {
			id_point p = (id_point)args->value(0)->toIntPtr();
			return new TValue(cgt->ptGetName(p));
		}
		case 4: {
			id_point p = (id_point)args->value(0)->toIntPtr();
			return new TValue(cgt->ptGetName(p));
		}
		case 5: return new TValue((void *)pointByType(context.element, pt_Work, args->value(0)->toInt()));
		case 6: return new TValue((void *)pointByType(context.element, pt_Event, args->value(0)->toInt()));
		case 7: return new TValue((void *)pointByType(context.element, pt_Var, args->value(0)->toInt()));
		case 8: return new TValue((void *)pointByType(context.element, pt_Data, args->value(0)->toInt()));
		case 9: {
			id_point p = (id_point)args->value(0)->toIntPtr();
			return new TValue(cgt->ptGetType(p));
		}
		case 10: {
			id_point p = (id_point)args->value(0)->toIntPtr();
			return new TValue(cgt->ptGetDataType(p));
		}
		case 11: {
			id_element e = (id_element)args->value(0)->toIntPtr();
			return new TValue(cgt->elGetClassName(e));
		}
		case 12: {
			id_element e = (id_element)args->value(0)->toIntPtr();
			return new TValue(cgt->elGetCodeName(e));
		}
		case 13: {
			id_sdk sdk = cgt->elGetSDK(context.element);
			id_element e = cgt->sdkGetElement(sdk, args->value(0)->toInt());
			return new TValue(cgt->elGetEID(e));
		}
		case 14: {
			id_sdk sdk = cgt->elGetSDK(context.element);
			id_element e = cgt->sdkGetParent(sdk);
			return new TValue(cgt->elGetEID(e));
		}
		case 15: {
			id_element e = (id_element)args->value(0)->toIntPtr();
			id_point p = cgt->elGetPtName(e, args->value(1)->toStr());
			return new TValue(p ? cgt->ptGetName(p) : "");
		}
		case 16:
			return new TValue(cgt->elLinkIs(context.element));
		case 17:
			return new TValue(cgt->elGetEID(cgt->elLinkMain(context.element)));
		case 18: {
			id_element e = (id_element)args->value(0)->toIntPtr();
			id_sdk sdk = cgt->elGetSDK(e);
			return new TValue(cgt->elGetEID(cgt->sdkGetParent(sdk)));
		}
		case 19: {
			id_point p = cgt->elGetPtName(context.element, args->value(0)->toStr());
			return new TValue((void *)p);
		}
		case 20: {
			id_sdk sdk = cgt->elGetSDK(context.element);
			return new TValue(sdk ? cgt->sdkGetCount(sdk) : 0);
		}
	}
	return new TValue();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TLngObject::TLngObject() : TScriptObject("lng") {
	ref = -1;
	count = 5;
	mtdNames = (TSORecord*) malloc(sizeof (TSORecord) * count);
	mtdNames[0] = makeMethod("linked", 1);
	mtdNames[1] = makeMethod("begin", 0);
	mtdNames[2] = makeMethod("end", 0);
	mtdNames[3] = makeMethod("decl_loc_var", 2);
	mtdNames[4] = makeMethod("decl_priv_var", 2);
}

static bool sectionIsCpp() {
	return !cgSections.empty() && cgSections[cgSection].name == "cpp";
}

static void declareBlockVar(TArgs *args, bool isPrivate) {
	if(!gvars || !cgBlockObject) return;
	const char *target = isPrivate ? "blk_priv_var" : "blk_func";
	TVarItem *item = gvars->findVarByName(target);
	if(!item || !item->value) return;
	TBlockItemObject *old = currentBlockItem();
	TBlockItemObject *dest = cgBlockObject->searchBlock(item->value->toStr());
	if(!dest) return;
	cgBlockObject->selectBlock(item->value->toStr());
	// The declaration line is only emitted for the C++ target, matching the
	// legacy Modules direct.inc behaviour.
	if(sectionIsCpp()) {
		TArgs line;
		line.add(new TValue(args->value(1)->toStr()));
		line.add(new TValue(" "));
		line.add(new TValue(args->value(0)->toStr()));
		line.add(new TValue(";"));
		dest->print(&line);
	}
	TArgs empty;
	dest->println(&empty);
	if(old) {
		TValue *name = old->getName();
		cgBlockObject->selectBlock(name->toStr());
	}
}

TValue *TLngObject::execMethod(TTreeNode *node, long index, Context &context) {
	switch(index) {
		case 0: {
			id_point p = (id_point)context.args->value(0)->toIntPtr();
			return new TValue((void *)rtcgRealLink(p));
		}
		case 1:
			if(currentBlockItem()) {
				if(sectionIsCpp()) {
					TArgs line;
					line.add(new TValue("{"));
					currentBlockItem()->print(&line);
				}
				currentBlockItem()->incLevel();
				TArgs empty;
				currentBlockItem()->println(&empty);
			}
			break;
		case 2:
			if(currentBlockItem()) {
				currentBlockItem()->decLevel();
				if(sectionIsCpp()) {
					TArgs line;
					line.add(new TValue("}"));
					currentBlockItem()->print(&line);
				}
				TArgs empty;
				currentBlockItem()->println(&empty);
			}
			break;
		case 3:
			declareBlockVar(context.args, false);
			break;
		case 4:
			declareBlockVar(context.args, true);
			break;
	}
	RET_EMPTY
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TSysObject::TSysObject() : TScriptObject("sys") {
	CG_LOG_BEGIN

	ref = -1;
	count = 11;
	mtdNames = (TSORecord*) malloc(sizeof (TSORecord) * count);
	mtdNames[0] = makeField("procs");
	mtdNames[1] = makeMethod("selectelement", 1);
	mtdNames[2] = makeMethod("curelement", 0);
	mtdNames[3] = makeMethod("hi_version", 1);
	mtdNames[4] = makeMethod("time", 1);
	mtdNames[5] = makeMethod("setindex", 1);
	mtdNames[6] = makeMethod("codedir", 0);
	mtdNames[7] = makeMethod("event", -1);
	mtdNames[8] = makeMethod("run_element", 2);
	mtdNames[9] = makeMethod("run_all", 1);
	mtdNames[10] = makeMethod("inner_parent", 0);
}

static void runElement(id_element e, const char *entry) {
	TArgs *args = new TArgs();
	TValue::free(erun->run(e, entry, args));
	delete args;
}

// The parent element of a nested scheme: the Sub of the container (Panel of ChildPanel, MainForm of ChildForm).
static id_element innerParent(id_sdk sdk) {
	for(int i = 0; sdk && i < cgt->sdkGetCount(sdk); i++) {
		id_element e = cgt->sdkGetElement(sdk, i);
		if(cgt->elGetFlag(e) & ELEMENT_FLG_IS_PARENT) return e;
	}
	return nullptr;
}

// Runs an entry of the elements of a scheme. A transparent container runs as part of the scheme it stands in:
// its parent element runs the entry (and creates the rest of its scheme itself, see sys.inner_parent), a nested
// scheme without one runs all its elements. Transparent frames have no code.
static void runScheme(id_sdk sdk, id_element skip, const char *entry) {
	for(int i = 0; i < cgt->sdkGetCount(sdk); i++) {
		id_element child = cgt->sdkGetElement(sdk, i);
		if(child == skip || (cgt->elGetFlag(child) & ELEMENT_FLG_IS_SYSTEM)) continue;
		if(rtcgTransparent(child)) {
			if(cgt->elGetClassIndex(child) != CI_MultiElement) continue;
			id_sdk inner = cgt->elGetSDK(child);
			if(id_element parent = innerParent(inner)) runElement(parent, entry);
			else if(inner) runScheme(inner, nullptr, entry);
			continue;
		}
		runElement(child, entry);
	}
}

TValue *TSysObject::execMethod(TTreeNode *node, long index, Context &context) {
	CG_LOG_BEGIN

	if(index < count) {
		CG_LOG_INFO(mtdNames[index].name)
		switch(index) {
			case 0:
				CG_LOG_RETURN(new TValue(root->count))
			case 1:
				CG_LOG_RETURN(new TValue(0))
			case 2:
				CG_LOG_RETURN(new TValue((void *)context.element))
			case 3: {
				std::string mask = context.args->value(0)->toStr();
				const std::pair<const char *, const char *> values[] = {
					{"%mj", "6"}, {"%mn", "0"}, {"%bl", "0"}
				};
				for(const auto &value : values) {
					size_t pos = 0;
					while((pos = mask.find(value.first, pos)) != std::string::npos) {
						mask.replace(pos, strlen(value.first), value.second);
						pos += strlen(value.second);
					}
				}
				CG_LOG_RETURN(new TValue(mask.c_str()))
			}
			case 4: {
				char buf[256] = {};
				time_t now = time(nullptr);
				struct tm *tm = localtime(&now);
				strftime(buf, sizeof(buf), context.args->value(0)->toStr(), tm);
				CG_LOG_RETURN(new TValue(buf))
			}
			case 5: {
				static int indexCounter = 0;
				indexCounter += context.args->value(0)->toInt();
				CG_LOG_RETURN(new TValue(indexCounter))
			}
			case 6: {
				char *s = cgt->ReadStrParam(PARAM_CODE_PATH, context.element);
				TValue *value = new TValue(s);
				delete[] s;
				CG_LOG_RETURN(value)
			}
			case 7:
				CG_LOG_RETURN(new TValue())
			case 8: {
				id_element child = (id_element) context.args->value(0)->toIntPtr();
				TArgs *childArgs = new TArgs();
				TValue *value = erun ? erun->run(child, context.args->value(1)->toStr(), childArgs) : NULL;
				delete childArgs;
				CG_LOG_RETURN(value ? value : new TValue())
			}
			case 9: {
				// The scheme of the calling element: the project scheme for the main form, the nested scheme for the
				// parent element of a container (sys.inner_parent).
				if(!cgSdk || !erun) CG_LOG_RETURN(new TValue())
				const std::string entry = context.args->value(0)->toStr();
				id_sdk sdk = context.element ? cgt->elGetParent(context.element) : nullptr;
				runScheme(sdk ? sdk : cgSdk, context.element, entry.c_str());
				CG_LOG_RETURN(new TValue())
			}
			case 10: {
				// The element is the parent element of a nested scheme (ChildPanel, ChildForm): it creates the elements
				// of that scheme inside itself with sys.run_all.
				id_element e = context.element;
				id_sdk sdk = e ? cgt->elGetParent(e) : nullptr;
				const bool inner = e && (cgt->elGetFlag(e) & ELEMENT_FLG_IS_PARENT) && sdk && sdk != cgSdk &&
					rtcgTransparent(cgt->sdkGetParent(sdk));
				CG_LOG_RETURN(new TValue(inner ? 1 : 0))
			}
		}
	}
	else {
		CG_LOG_RETURN(root->childs[index - 100]->run(context))
	}
	CG_LOG_RETURN(NULL)
}

script_proc TSysObject::getProc(TValue *val, Context &context) {
	script_proc i = TScriptObject::getProc(val, context);
	if(i == -1) {
		TFuncNode *f = root->getFunc(val->toStr());
		if(f) {
			i = root->indexOf(f) + 100;
		}
	}
	
	return i;
}

bool TSysObject::isField(script_proc index, Context &context) {
	if(index < count)
		return TScriptObject::isField(index,context);
	
	return false;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TElementObject::TElementObject() : TScriptObject("this") {
	CG_LOG_BEGIN

	ref = -1;
	element = NULL;
	initMap();
}

TElementObject::TElementObject(id_element element) : TScriptObject(cgt->elGetClassName(element)) {
	CG_LOG_BEGIN

	this->element = element;
	ref = 0;
	initMap();
}

void TElementObject::initMap() {
	count = 22;
	mtdNames = (TSORecord*) malloc(sizeof (TSORecord) * count);
	mtdNames[0] = makeField("name");
	mtdNames[1] = makeField("numpoints");
	mtdNames[2] = makeMethod("points", 1);
	mtdNames[3] = makeField("numprops");
	mtdNames[4] = makeMethod("props", 1);
	mtdNames[5] = makeField("id");
	mtdNames[6] = makeField("codename");
	mtdNames[7] = makeField("parent");
	mtdNames[8] = makeMethod("setfield", 2);
	mtdNames[9] = makeField("numworks");
	mtdNames[10] = makeField("numevents");
	mtdNames[11] = makeField("numvars");
	mtdNames[12] = makeField("numdatas");
	mtdNames[13] = makeMethod("pt_work", 1);
	mtdNames[14] = makeMethod("pt_event", 1);
	mtdNames[15] = makeMethod("pt_var", 1);
	mtdNames[16] = makeMethod("pt_data", 1);
	mtdNames[17] = makeMethod("get_sdk", 1);
	mtdNames[18] = makeField("parent_element");
	mtdNames[19] = makeField("is_link");
	mtdNames[20] = makeField("is_main_link");
	mtdNames[21] = makeField("link");
}

int numPointsByType(id_element e, int type) {
	int c = 0;
	for(int i = 0; i < cgt->elGetPtCount(e); i++)
		if(cgt->ptGetType(cgt->elGetPt(e, i)) == type)
			c++;
	return c;
}

id_point getPointByType(id_element e, int type, int index) {
	int c = 0;
	for(int i = 0; i < cgt->elGetPtCount(e); i++) {
		id_point point = cgt->elGetPt(e, i);
		if(cgt->ptGetType(point) == type) {
			if(c == index)
				return point;
			c++;
		}
	}
	return NULL;
}

TValue *TElementObject::execMethod(TTreeNode *node, long index, Context &context) {
	CG_LOG_BEGIN

	id_element e = element ? element : context.element;
	TArgs *args = context.args;
	
	if(!e) {
		CG_LOG_INFO("internal error: element not found")
		CG_LOG_RETURN(new TValue())
	}
	ElementData *data = (ElementData *)cgt->elGetData(e);
	if(!data && index != 4) {
		CG_LOG_INFO("internal error: element data is NULL")
		CG_LOG_RETURN(new TValue())
	}
	
	if(index >= 0xff) {
		TFuncNode *f = (TFuncNode *)data->codeTree->childs[index - 0xff];
		if(!f) {
			CG_LOG_INFO("internal error: element method not found")
			CG_LOG_RETURN(new TValue())
		}
		Context c(e, f->name, context.args);
		CG_LOG_RETURN(f->run(c))
	}
	else {
		switch(index) {
			case 0:
				CG_LOG_RETURN(new TValue(cgt->elGetClassName(e), true))
			case 1:
				CG_LOG_RETURN(new TValue(cgt->elGetPtCount(e)))
			case 2: {
				id_point p = NULL;
				if(args->value(0)->getType() == DATA_INT) {
					p = cgt->elGetPt(e, args->value(0)->toInt());
				}
				else {
					p = cgt->elGetPtName(e, args->value(0)->toStr());
				}
				if(p)
					CG_LOG_RETURN(new TValue(new TPointObject(cgt->elGetPtName(e, args->value(0)->toStr()))))
				CG_LOG_RETURN(new TValue("undefined point"))
			}
			case 3:
				CG_LOG_RETURN(new TValue(cgt->elGetPropCount(e)))
			case 4: {
				id_prop prop = cgt->getPropByName(e, args->value(0)->toStr());
				if(prop)
					CG_LOG_RETURN(new TValue(new TPropertyObject(e, prop)))
				
				CG_LOG_RETURN(new TValue("undefined property"))
			}
			case 5:
				CG_LOG_RETURN(new TValue(data->id))
			case 6: {
				std::string cname(cgt->elGetClassName(e));
				char buf[32];
				int_to_str(data->id, buf);
				CG_LOG_RETURN(new TValue(cname.append(buf).c_str(), true))
			}
			case 7:
				CG_LOG_RETURN(new TValue(new TSDKObject(cgt->elGetParent(e))))
			case 8: {
				const char *fieldName = args->value(0)->toStr();
				TValue *field = data->getField(fieldName);
				if(!field)
					field = data->addField(fieldName);
				field->copy(args->value(1));
				break;
			}
			case 9:
			case 10:
			case 11:
			case 12:
				CG_LOG_RETURN(new TValue(numPointsByType(e, index - 9 + pt_Work)))
			case 13:
			case 14:
			case 15:
			case 16: {
				id_point point = getPointByType(e, index - 13 + pt_Work, args->value(0)->toInt());
				CG_LOG_RETURN(point ? new TValue(new TPointObject(point)) : new TValue(0))
			}
			case 17:
				CG_LOG_RETURN(new TValue(new TSDKObject(cgt->elGetSDKByIndex(e, args->value(0)->toInt()))))
			case 18:
				CG_LOG_RETURN(new TValue(new TElementObject(cgt->elGetSDK(e))))
			case 19:
				CG_LOG_RETURN(new TValue(cgt->elLinkIs(e) ? 1 : 0))
			case 20:
				CG_LOG_RETURN(new TValue(cgt->elLinkMain(e) == e ? 1 : 0))
			case 21:
				CG_LOG_RETURN(new TValue(new TElementObject(cgt->elLinkMain(e))))
			default: {
				CG_LOG_RETURN(data->fields.at(index - count)->value->duplicate())
			}

			}
		}
		CG_LOG_RETURN(new TValue((TScriptObject *)this))
}

script_proc TElementObject::getProc(TValue *val, Context &context) {
	CG_LOG_BEGIN

	script_proc i = TScriptObject::getProc(val, context);
	if(i != -1)
		CG_LOG_RETURN(i)
	
	id_element e = element ? element : context.element;
	ElementData *data = (ElementData *)cgt->elGetData(e);
	const char *name = val->toStr();
	
	if(!data)
		CG_LOG_RETURN(-1)
	
	TFuncNode *f;
	if((f = data->codeTree->getFunc(name)))
		CG_LOG_RETURN(data->codeTree->indexOf(f) + 0xff)

	i = data->getFieldIndex(name);
	if(i == -1) {
		data->addField(name);
		i = data->fields.size()-1;
	}
	i += count;
	
	CG_LOG_RETURN(i)
}

bool  TElementObject::isField(script_proc index, Context &context) {
	if(index < count)
		return TScriptObject::isField(index,context);
	else if(index >= 0xff)
		return false;
	
	id_element e = element ? element : context.element;
	ElementData *data = (ElementData *)cgt->elGetData(e);
	index -= count;
	return (index < data->fields.size());
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TPointObject::TPointObject(id_point point) : TScriptObject(cgt->ptGetName(point)) {
	CG_LOG_BEGIN

	this->point = point;
	
	ref = 0;
	count = 5;
	mtdNames = (TSORecord*) malloc(sizeof (TSORecord) * count);
	mtdNames[0] = makeField("name");
	mtdNames[1] = makeField("parent");
	mtdNames[2] = makeField("point");
	mtdNames[3] = makeField("type");
	mtdNames[4] = makeField("datatype");
}

TValue *TPointObject::execMethod(TTreeNode *node, long index, Context &context) {
	CG_LOG_BEGIN

	switch(index) {
		case 0:
			CG_LOG_RETURN(new TValue(cgt->ptGetName(point)))
		case 1: {
			id_element e = cgt->ptGetParent(point);
			erun->init(e);
			CG_LOG_RETURN(new TValue(new TElementObject(e)))
		}
		case 2: {
			id_point p = rtcgRealLink(point);
			if(p)
				CG_LOG_RETURN(new TValue(new TPointObject(p)))
			CG_LOG_RETURN(new TValue(0))
		}
		case 3:
			CG_LOG_RETURN(new TValue(cgt->ptGetType(point)))
		case 4:
			CG_LOG_RETURN(new TValue(cgt->ptGetDataType(point)))
	}
	CG_LOG_RETURN(new TValue())
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TPropertyObject::TPropertyObject(id_element parent, id_prop prop) : TScriptObject(cgt->propGetName(prop)) {
	CG_LOG_BEGIN

	this->prop = prop;
	element	= parent;
	
	ref = 0;
	count = 4;
	mtdNames = (TSORecord*) malloc(sizeof (TSORecord) * count);
	mtdNames[0] = makeField("name");
	mtdNames[1] = makeField("value");
	mtdNames[2] = makeField("type");
	mtdNames[3] = makeField("isdef");
}

TValue *TPropertyObject::execMethod(TTreeNode *node, long index, Context &context) {
	CG_LOG_BEGIN

	switch(index) {
		case 0:
			CG_LOG_RETURN(new TValue(cgt->propGetName(prop)))
		case 1:
			CG_LOG_RETURN(TValue::fromProperty(element, prop))
		case 2:
			CG_LOG_RETURN(new TValue(cgt->propGetType(prop)))
		case 3:
			CG_LOG_RETURN(new TValue(cgt->isDefProp(element, prop)))
	}
	CG_LOG_RETURN(new TValue())
}

TValue *TPropertyObject::getName() {
	TValue::free(nameValue);
	return nameValue = TValue::fromProperty(element, prop);
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TSDKObject::TSDKObject(id_sdk sdk) : TScriptObject("sdk") {
	CG_LOG_BEGIN

	ASSERT(sdk, "sdk is NULL")
	ref = 0;
	this->sdk = sdk;
	initMap();
}

void TSDKObject::initMap() {
	count = 10;
	mtdNames = (TSORecord*) malloc(sizeof (TSORecord) * count);
	mtdNames[9] = makeMethod("run_child", 2);
	mtdNames[8] = makeMethod("run_all", 1);
	mtdNames[7] = makeMethod("connectall", 0);
	mtdNames[0] = makeMethod("initall", 0);
	mtdNames[1] = makeMethod("child", 1);
	mtdNames[2] = makeMethod("get_parent_element", 0);
	mtdNames[3] = makeMethod("get_parent_sdk", 0);
	mtdNames[4] = makeField("numelements");
	mtdNames[5] = makeField("id");
	mtdNames[6] = makeField("counter");
}

void TSDKObject::initAll() {
	id_element e;
	for(int i = 0; i < cgt->sdkGetCount(sdk); i++) {
		e = cgt->sdkGetElement(sdk, i);
		// Core elements have their own .hws code in packs that compile them
		// (cpp: hc*), so they take part in the initial pass like any other.
		// System helpers (HubEx, GetDataEx, InfoTip) are wiring only.
		if(!(cgt->elGetFlag(e) & ELEMENT_FLG_IS_SYSTEM))
			erun->init(e);
	}
}

// Second pass after initall: elements whose code defines `connect` hook up
// their signal handlers once every other element already exists.
void TSDKObject::connectAll() {
	for(int i = 0; i < cgt->sdkGetCount(sdk); i++) {
		id_element e = cgt->sdkGetElement(sdk, i);
		if(cgt->elGetFlag(e) & ELEMENT_FLG_IS_SYSTEM)
			continue;
		TArgs *args = new TArgs();
		TValue::free(erun->run(e, "connect", args));
		delete args;
	}
}

TValue *TSDKObject::execMethod(TTreeNode *node, long index, Context &context) {
	CG_LOG_BEGIN

	CG_LOG_INFO(mtdNames[index].name)
	switch(index) {
		case 0:
			initAll();
			break;
		case 7:
			connectAll();
			break;
		case 8: {
			if(sdk && erun) {
				const std::string entry = context.args->value(0)->toStr();
				runScheme(sdk, nullptr, entry.c_str());
			}
			break;
		}
		case 9: {
			const int child = context.args->value(0)->toInt();
			if(sdk && erun && child >= 0 && child < cgt->sdkGetCount(sdk)) {
				const std::string entry = context.args->value(1)->toStr();
				runElement(cgt->sdkGetElement(sdk, child), entry.c_str());
			}
			break;
		}
		case 1:
			CG_LOG_RETURN(new TValue(new TElementObject(cgt->sdkGetElement(sdk, context.args->at(0)->value->toInt()))))
		case 2: {
			id_element e = cgt->sdkGetParent(sdk);
			if(e)
				CG_LOG_RETURN(new TValue(new TElementObject(e)))
			else
				CG_LOG_RETURN(new TValue(0))
		}
		case 3: {
			if(cgt->sdkGetParent(sdk)) {
				id_element e = cgt->sdkGetElement(sdk, 1);
				CG_LOG_RETURN(new TValue(new TElementObject(e)))
			}
			CG_LOG_RETURN(new TValue(0))
		}
		case 4:
			CG_LOG_RETURN(new TValue(cgt->sdkGetCount(this->sdk)))
		case 5:
			CG_LOG_RETURN(new TValue(static_cast<int>(reinterpret_cast<intptr_t>(this->sdk))))
		case 6: {
			id_element e = cgt->sdkGetElement(sdk, 0);
			if(!e)
				e = cgt->sdkGetParent(sdk);
			CG_LOG_RETURN(new TValue(cgt->getBuildCounter(e)))
		}
	}
	CG_LOG_RETURN(new TValue())
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TArgsObject::TArgsObject() : TScriptObject("args") {
	CG_LOG_BEGIN

	ref = -1;
	count = 3;
	mtdNames = (TSORecord*) malloc(sizeof (TSORecord) * count);
	mtdNames[0] = makeMethod("size", 0);
	mtdNames[1] = makeMethod("name", 1);
	mtdNames[2] = makeMethod("value", 1);
}

TValue *TArgsObject::execMethod(TTreeNode *node, long index, Context &context) {
	CG_LOG_BEGIN

	if(index < count) {
		CG_LOG_INFO(mtdNames[index].name)
	}
	switch(index) {
		case 0:
			CG_LOG_RETURN(new TValue((int)context.func->args->size()))
		case 1:
			CG_LOG_RETURN(new TValue(context.func->args->at(context.args->at(0)->value->toInt())->name.c_str()))
		case 2:
			CG_LOG_RETURN(context.func->args->at(context.args->at(0)->value->toInt())->value->duplicate())
		default:
			CG_LOG_RETURN(context.func->args->at(index - count)->value->duplicate())
	}
	CG_LOG_RETURN(new TValue())
}

script_proc TArgsObject::getProc(TValue *val, Context &context) {
	CG_LOG_BEGIN

	script_proc i = TScriptObject::getProc(val, context);
	if(i != -1)
		CG_LOG_RETURN(i)

	const char *name = val->toStr();
	for(int f = 0; f < context.func->args->size(); f++) {
		if(context.func->args->at(f)->name.compare(name) == 0)
			CG_LOG_RETURN(count + f)
	}
		
	CG_LOG_RETURN(-1)
}

bool  TArgsObject::isField(script_proc index, Context &context) {
	if(index < count)
		return TScriptObject::isField(index,context);
	
	return true;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TArrayObject::TArrayObject() : TScriptObject("array") {
	CG_LOG_BEGIN

	ref = 0;
	count = 10;
	mtdNames = (TSORecord*) malloc(sizeof (TSORecord) * count);
	mtdNames[0] = makeMethod("add", -1);
	mtdNames[1] = makeMethod("insert", 2);
	mtdNames[2] = makeMethod("remove", 1);
	mtdNames[3] = makeMethod("get", 1);
	mtdNames[4] = makeMethod("clear", 0);
	mtdNames[5] = makeMethod("size", 0);
	mtdNames[6] = makeMethod("join", 1);
	mtdNames[7] = makeMethod("contain", 1);
	mtdNames[8] = makeMethod("set", 2);
	mtdNames[9] = makeMethod("fill", 2);
}

TArrayObject::~TArrayObject() {
	clear();
}

void TArrayObject::clear() {
	for(std::vector<TValue*>::iterator item = list.begin(); item != list.end(); item++)
		TValue::free(*item);
	list.clear();
}

TValue *TArrayObject::join(TValue *delimiter) {
	TValue *result = new TValue();
	result->makeArray();
	TArray *arr = result->toArr();
	for(std::vector<TValue*>::iterator item = list.begin(); item != list.end(); item++) {
		if(item != list.begin())
			arr->add(delimiter, false);
		arr->add(*item, false);
	}
	
	return result;
}

bool TArrayObject::contain(TValue *value) {
	const char *search = value->toStr();
	for(std::vector<TValue*>::iterator item = list.begin(); item != list.end(); item++) {
		if(strcasecmp(search, (*item)->toStr()) == 0)
			return true;
	}
	return false;
}

void TArrayObject::add(TValue *value) {
	list.push_back(value);
}

TValue *TArrayObject::get(int index) {
	if(index >= 0 && index < list.size()) {
		return list.at(index)->duplicate();
	}
	else {
		return new TValue();
	}
}

void TArrayObject::create(TTreeNode *node, Context &context) {
	for(int i = 0; i < context.args->size(); i++)
		add(new TValue(context.args->value(i)));
}

TValue *TArrayObject::execMethod(TTreeNode *node, long index, Context &context) {
	CG_LOG_BEGIN

	switch(index) {
		case 0:
			for(int i = 0; i < context.args->size(); i++)
				add(new TValue(context.args->value(i)));
			CG_LOG_RETURN(new TValue(this))
		case 1: {
			int i = context.args->value(0)->toInt();
			if(i >= 0 && i < list.size()) {
				std::vector<TValue*>::iterator it = list.begin();
				while(i--) it++;
				list.insert(it, new TValue(context.args->value(1)));
				CG_LOG_RETURN(new TValue(1))
			}
			else {
				CG_LOG_RETURN(new TValue(0))
			}
		}
		case 2: {
			int i = context.args->value(0)->toInt();
			if(i >= 0 && i < list.size()) {
				std::vector<TValue*>::iterator it = list.begin();
				while(i--) it++;
				TValue *val = *it;
				list.erase(it);
				CG_LOG_RETURN(val)
			}
			else {
				CG_LOG_RETURN(new TValue(0))
			}
		}
		case 3: {
			CG_LOG_RETURN(this->get(context.args->value(0)->toInt()))
		}
		case 4:
			clear();
			break;
		case 5:
			CG_LOG_RETURN(new TValue((int)list.size()))
		case 6:
			CG_LOG_RETURN(join(context.args->value(0)))
		case 7:
			CG_LOG_RETURN(new TValue(contain(context.args->value(0))))
		case 8: {
			int i = context.args->value(0)->toInt();
			TValue *value = context.args->value(1);
			if(i >= 0 && i < list.size()) {
				TValue::free(list.at(i));
				list.at(i) = new TValue(value);
			}
			CG_LOG_RETURN(value)
		}
		case 9: {
			clear();
			int amount = context.args->value(0)->toInt();
			TValue *value = context.args->value(1);
			for(int i = 0; i < amount; i++) {
				add(new TValue(value));
			}
			CG_LOG_RETURN(new TValue(this))
		}
	}
	CG_LOG_RETURN(new TValue())
}

TValue *TArrayObject::putStream(TTreeNode *node, TValue *value, Context &context) {
	add(new TValue(value));
	
	return new TValue((TScriptObject *)this);
}


//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TScriptObject *searchObject(const char *name) {
	for (int i = 0; i < count; i++)
		if(objs[i]->isObject(name))
			return objs[i];
	return NULL;
}

TScriptObject *addObject(TScriptObject *obj) {
	count++;
	objs = (TScriptObject **) realloc(objs, sizeof (TScriptObject *) * count);
	objs[count - 1] = obj;
	return obj;
}

TScriptObject *copyObject(TScriptObject *obj) {
	if (obj->isDeletable())
		obj->incRef();
	return obj;
}

void deleteObject(TScriptObject *obj) {
	if (obj->isDeletable()) {
		obj->decRef();
		if (obj->isDelete())
			delete obj;
	}
}

void initObjects() {
	CG_LOG_BEGIN

	count = 0;
	objs = NULL;
	cgBlockObject = new TBlockObject();
	addObject(cgBlockObject);
	addObject(new TLngObject());
	addObject(new TElementObject());
	addObject(new TCGTObject());
	addObject(new TSysObject());
	addObject(new TArgsObject());
	
	CG_LOG_END
}

void clearObjects() {
	for(int i = 0; i < count; i++)
		delete objs[i];
	free(objs);
	objs = NULL;
	count = 0;
	cgBlockObject = NULL;
}

TScriptObject *createObject(const char *name, TTreeNode *node, Context &context) {
	for(int i = 0; i < OBJ_COUNT; i++)
		if(strcasecmp(name, objInstances[i].name) == 0) {
			TScriptObject *obj = objInstances[i].createInstance();
			obj->create(node, context);
			return obj;
		}
	return NULL;
}
