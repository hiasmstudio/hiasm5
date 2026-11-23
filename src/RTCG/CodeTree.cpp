#include <list>

#include "CodeTree.h"
#include "IntFunc.h"
#include "ElementData.h"

const char *tnodeNames[ND_COUNT] = {
	"UNKNOWN", // ND_UNK
	"UNIT", // ND_UNIT
	"FUNC", // ND_FUNC
	"BLOCK", // ND_BLOCK
	"INTEGER", // ND_INT
	"STRING", // ND_STR
	"ADD", // ND_ADD
	"SUB", // ND_SUB
	"MUL", // ND_MUL
	"DIV", // ND_DIV
	"IF", // ND_IF
	"==", // ND_EQ
	"!=", // ND_NEQ
	">", // ND_LESTHEN
	"<", // ND_SMALTHEN
	">=", // ND_LESTHEN_EQ
	"<=", // ND_SMALTHEN_EQ
	"REAL", // ND_REAL
	"OR", // ND_OR
	"AND", // ND_AND
	"NOT", // ND_NOT
	"VAR", // ND_VAR
	"ASSIGN", // ND_ASSIGN
	"COMAND", // ND_COMAND
	"ARRINDEX", // ND_ARRINDEX
	"RETURN", // ND_RETURN
	"FOR", // ND_FOR
	"INC", // ND_INC
	"DEC", // ND_DEC
	"INT_OBJECT", // ND_INT_OBJECT
	"DOT", // ND_DOT
	"CALL", // ND_CALL
	"USERVAR", // ND_USERVAR
	"PROC", // ND_PROC
	"EXPIF", // ND_EXPIF
	"+=", // ND_ADDEQ
	"-=", // ND_SUBEQ
	"*=", // ND_MULEQ
	"/=", // ND_DIVEQ
	"CAST", // ND_CAST
	"B_AND", //ND_BIT_AND
	"B_OR", //ND_BIT_OR
	"<<", //ND_SHIFT_LEFT
	">>", //ND_SHIFT_RIGHT
	"METHOD", //ND_METHOD
	"NEW", //ND_NEW
	"SWITCH", //ND_SWITCH
	"CASE", //ND_CASE
	"DEFAULT", //ND_DEFAULT
};

TTreeNode::TTreeNode(NODE_TYPES type) {
	childs = NULL;
	count = 0;
	value = NULL;
	nodeType = type;
	parent = NULL;
}

TTreeNode::~TTreeNode() {
	if (count) {
		for (int i = 0; i < count; i++)
			delete childs[i];
		free(childs);
	}
	if (value)
		TValue::free(value);
}

TTreeNode* TTreeNode::addNode(TTreeNode *node) {
	count++;
	childs = (TTreeNode**) realloc(childs, sizeof (TTreeNode*) * count);
	childs[count - 1] = node;
	node->parent = this;

	return node;
}

void TTreeNode::removeNode(TTreeNode *node) {
	CG_LOG_BEGIN

	CG_LOG_END
}

bool TTreeNode::hasParent() {
	return parent != NULL;
}

bool TTreeNode::hasChild() {
	return childs != NULL;
}

TUnitNode *TTreeNode::getRoot() {
	TTreeNode *ret = parent;
	while (ret && ret->parent)
		ret = ret->parent;
	return dynamic_cast<TUnitNode *>(ret);
}

void TTreeNode::dump_args(HANDLE f) {
	// do nothing
}

void TTreeNode::dump(HANDLE f, int tab) {
	for (int i = 0; i < tab; i++)
		fputs("\t", f);

	fputs(tnodeNames[nodeType], f);
	dump_args(f);
	fputs("\n", f);
	for (int i = 0; i < count; i++)
		childs[i]->dump(f, tab + 1);
}

TTreeNode *TTreeNode::next(int &curNode) {
	TTreeNode *nd;
	if (curNode < count) {
		nd = childs[curNode];
		curNode++;
	} else nd = NULL;
	return nd;
}

TValue *TTreeNode::run(Context &context) {
	TTreeNode *item;
	for (int curNode = first(context); (item = next(curNode)) && !context.needBreak();)
		TValue::free(item->run(context));
	return NULL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TUnitNode::TUnitNode(TCode *_codeTree) : TTreeNode(ND_UNIT) {
	CG_LOG_BEGIN

	codeTree = _codeTree;
	langVars = new TVarsList();
}

TUnitNode::~TUnitNode() {
	delete langVars;
	CG_LOG_END
}

TValue *TUnitNode::run(Context &context) {
	CG_LOG_BEGIN

	CALL_TRACE_BEGIN(codeTree->name)

	TValue *val = NULL;
	TFuncNode *fnode = getFunc(context.entry);
	if(fnode)
		val = fnode->run(context);
	
	CALL_TRACE_END(codeTree->name)

	CG_LOG_RETURN(val)
}

TFuncNode *TUnitNode::getFunc(const char *name) {
	for (int i = 0; i < count; i++) {
		if (childs[i]->nodeType == ND_FUNC && strcasecmp(((TFuncNode*) childs[i])->name, name) == 0) {
			TFuncNode *fn = (TFuncNode*) childs[i];
			if(fn->matchesSection(cgSection)) return fn;
		}
	}
	return NULL;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TFuncNode::TFuncNode(const char *_name) : TTreeNode(ND_FUNC), body(NULL) {
	CG_LOG_BEGIN

	name = new char[strlen(_name)+1];
	strcpy(name, _name);
	sections = 0xffffffffu;
	vars = new TVarsList();
	args = new TArgs();
}

TFuncNode::TFuncNode(const char *_name, TTreeNode *body) : TTreeNode(ND_FUNC) {
	CG_LOG_BEGIN

	name = new char[strlen(_name)+1];
	strcpy(name, _name);
	sections = 0xffffffffu;
	vars = new TVarsList();
	args = new TArgs();
	
	this->body = body;
}

TFuncNode::~TFuncNode() {
	if (name)
		delete[] name;
	delete vars;
	delete args;

	CG_LOG_END
}

TValue *TFuncNode::run(Context &context) {
	CG_LOG_BEGIN

	CALL_TRACE(name)

	std::vector<TValue*> targs;
	for (int i = 0; i < args->size(); i++) { //TODO !!! add byref
		targs.push_back(args->at(i)->value);
		if(i < context.args->size()) {
			args->at(i)->value = new TValue(context.args->value(i));
		}
		else
			args->at(i)->value = new TValue();
	}
	std::vector<TValue*> tvars;
	for (int i = 0; i < vars->size(); i++) {
		tvars.push_back(vars->at(i)->value);
		vars->at(i)->value = new TValue();
	}

	result = NULL;
	Context c(context.element, context.entry, args, this);
	c.incomingArgs = context.args;
	if(body)
		body->run(c);
	else
		TTreeNode::run(c);
	
	for (int i = 0; i < args->size(); i++) {
		TValue::free(args->value(i));
		args->at(i)->value = targs.at(i);
	}
	for (int i = 0; i < vars->size(); i++) {
		TValue::free(vars->value(i));
		vars->at(i)->value = tvars.at(i);
	}

	CG_LOG_RETURN(result ? result : new TValue())
}

void TFuncNode::returnValue(TValue *value) {
	result = value;
}

void TFuncNode::dump_args(HANDLE f) {
	fprintf(f, "(%s)", name);
}

void TFuncNode::addArg(char *name) {
	args->add(name);
}

TVarItem* TFuncNode::addVar(char *name) {
	return vars->add(name);
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TIfNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *cont = childs[0]->run(context);
	if(cont->toBool()) {
		TValue::free(childs[1]->run(context));
	}
	else if(count == 3) {
		TValue::free(childs[2]->run(context));
	}
	TValue::free(cont);

	CG_LOG_RETURN(NULL)
}

TTreeNode *TIfNode::returnBlock() {
	if(parent->nodeType == ND_IF && parent->count == 3 && parent->childs[2] == this)
		return parent->returnBlock();
	return parent;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TSwitchNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *value = childs[0]->run(context);
	TDefaultNode *fallback = NULL;
	for(int i = 1; i < count; i++) {
		if(childs[i]->nodeType == ND_DEFAULT) {
			fallback = (TDefaultNode*) childs[i];
			continue;
		}
		TCaseNode *item = (TCaseNode*) childs[i];
		TValue *caseValue = item->childs[0]->run(context);
		bool equal = strcmp(value->toStr(), caseValue->toStr()) == 0;
		TValue::free(caseValue);
		if(equal) {
			TValue::free(item->childs[1]->run(context));
			fallback = NULL;
			break;
		}
	}
	if(fallback)
		TValue::free(fallback->childs[0]->run(context));
	TValue::free(value);

	CG_LOG_RETURN(NULL)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TForNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *cont;
	while ((cont = childs[0]->run(context))->toBool() && !context.needBreak()) {
		TValue::free(cont);
		TValue::free(childs[2]->run(context));
		TValue::free(childs[1]->run(context));
	}
	TValue::free(cont);

	CG_LOG_RETURN(NULL)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TEqNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = childs[0]->run(context);
	TValue *op2 = childs[1]->run(context);
	TValue *result = new TValue();
	switch (op1->getType()) {
		case DATA_INT:
			result->setValue(op1->toInt() == op2->toInt());
			break;
		case DATA_STR:
			result->setValue(strcmp(op1->toStr(), op2->toStr()) == 0);
			break;
		case DATA_REAL:
			result->setValue(op1->toReal() == op2->toReal());
			break;
		default:
			result->setValue(false);
	}
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TNeqNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = childs[0]->run(context);
	TValue *op2 = childs[1]->run(context);
	TValue *result = new TValue();
	switch (op1->getType()) {
		case DATA_INT:
			result->setValue(op1->toInt() != op2->toInt());
			break;
		case DATA_STR:
			result->setValue(strcmp(op1->toStr(), op2->toStr()) != 0);
			break;
		case DATA_REAL:
			result->setValue(op1->toReal() != op2->toReal());
			break;
		default:
			result->setValue(false);
	}
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TLessNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = childs[0]->run(context);
	TValue *op2 = childs[1]->run(context);
	TValue *result = new TValue();
	switch (op1->getType()) {
		case DATA_INT:
			result->setValue(op1->toInt() > op2->toInt());
			break;
		case DATA_STR:
			result->setValue(strcmp(op1->toStr(), op2->toStr()) > 0);
			break;
		case DATA_REAL:
			result->setValue(op1->toReal() > op2->toReal());
			break;
		default:
			result->setValue(false);
	}
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TSmallNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = childs[0]->run(context);
	TValue *op2 = childs[1]->run(context);
	TValue *result = new TValue();
	switch (op1->getType()) {
		case DATA_INT:
			result->setValue(op1->toInt() < op2->toInt());
			break;
		case DATA_STR:
			result->setValue(strcmp(op1->toStr(), op2->toStr()) < 0);
			break;
		case DATA_REAL:
			result->setValue(op1->toReal() < op2->toReal());
			break;
		default:
			result->setValue(false);
	}
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TLessEqNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = childs[0]->run(context);
	TValue *op2 = childs[1]->run(context);
	TValue *result = new TValue();
	switch (op1->getType()) {
		case DATA_INT:
			result->setValue(op1->toInt() >= op2->toInt());
			break;
		case DATA_STR:
			result->setValue(strcmp(op1->toStr(), op2->toStr()) >= 0);
			break;
		case DATA_REAL:
			result->setValue(op1->toReal() >= op2->toReal());
			break;
		default:
			result->setValue(false);
	}
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TSmallEqNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = childs[0]->run(context);
	TValue *op2 = childs[1]->run(context);
	TValue *result = new TValue();
	switch (op1->getType()) {
		case DATA_INT:
			result->setValue(op1->toInt() <= op2->toInt());
			break;
		case DATA_STR:
			result->setValue(strcmp(op1->toStr(), op2->toStr()) <= 0);
			break;
		case DATA_REAL:
			result->setValue(op1->toReal() <= op2->toReal());
			break;
		default:
			result->setValue(false);
	}
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TOrNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = NULL;
	TValue *op2 = NULL;
	TValue *result = new TValue((op1 = childs[0]->run(context))->toBool() || (op2 = childs[1]->run(context))->toBool());
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TAndNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = NULL;
	TValue *op2 = NULL;
	TValue *result = new TValue((op1 = childs[0]->run(context))->toBool() && (op2 = childs[1]->run(context))->toBool());
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TNotNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *cond = childs[0]->run(context);
	TValue *result = new TValue(!cond->toBool());
	TValue::free(cond);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void _addValue(TValue *result, TValue *op1, TValue *op2) {
	switch (op1->getType()) {
		case DATA_INT:
			result->setValue(op1->toInt() + op2->toInt());
			break;
		case DATA_STR: {
			const char *ch1 = op1->toCode();
			const char *ch2 = op2->toCode();
			int i = strlen(ch1);
			char *c = new char[i + strlen(ch2) + 1];
			strcpy(c, ch1);
			strcpy(c + i, ch2);
			result->setValueStr(c);
			result->flags = op1->flags;
			break;
		}
		case DATA_REAL:
			result->setValue(op1->toReal() + op2->toReal());
			break;
		default:
			;
	}
}

TValue *TAddNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = childs[0]->run(context);
	TValue *op2 = childs[1]->run(context);
	TValue *result = new TValue();
	_addValue(result, op1, op2);
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void _subValue(TValue *result, TValue *op1, TValue *op2) {
	switch (op1->getType()) {
		case DATA_INT:
			result->setValue(op1->toInt() - op2->toInt());
			break;
		case DATA_REAL:
			result->setValue(op1->toReal() - op2->toReal());
			break;
		default:
			;
	}
}

TValue *TSubNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *result = new TValue();
	if(this->count == 1) {
		TValue *op1 = childs[0]->run(context);
		switch (op1->getType()) {
			case DATA_INT:
				result->setValue(-op1->toInt());
				break;
			case DATA_REAL:
				result->setValue(-op1->toReal());
				break;
			default:
				break;
		}
		TValue::free(op1);
	}
	else {
		TValue *op1 = childs[0]->run(context);
		TValue *op2 = childs[1]->run(context);
		_subValue(result, op1, op2);
		TValue::free(op1);
		TValue::free(op2);
	}

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void _mulValue(TValue *result, TValue *op1, TValue *op2) {
	switch (op1->getType()) {
		case DATA_INT:
			result->setValue(op1->toInt() * op2->toInt());
			break;
		case DATA_REAL:
			result->setValue(op1->toReal() * op2->toReal());
			break;
		default:
			;
	}
}

TValue *TMulNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = childs[0]->run(context);
	TValue *op2 = childs[1]->run(context);
	TValue *result = new TValue();
	_mulValue(result, op1, op2);
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

void _divValue(TValue *result, TValue *op1, TValue *op2) {
	switch (op1->getType()) {
		case DATA_INT:
			result->setValue((float) op1->toInt() / op2->toInt());
			break;
		case DATA_REAL:
			result->setValue(op1->toReal() / op2->toReal());
			break;
		default:
			;
	}
}

TValue *TDivNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = childs[0]->run(context);
	TValue *op2 = childs[1]->run(context);
	TValue *result = new TValue();
	_divValue(result, op1, op2);
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TStringNode::run(Context &context) {
	if((this->value->flags & FLG_CODE) == 0)
		return TConstNode::run(context);

	std::string s = this->value->toStr();
	int i = s.find("${");
	if(i < 0)
		return TConstNode::run(context);

	TValue *result = new TValue();
	result->makeArray();
	int p = 0;
	while(i >= 0) {
		if(i - p) {
			const std::string buf = s.substr(p, i - p);
			result->toArr()->add(new TValue(buf.c_str(), true), false);
		}
		p = s.find("}", i+2);
		const int len = std::max(p - i - 2, 0);
		const std::string name = s.substr(i + 2, len);
		TArgs *args = new TArgs();
		args->add(new TValue(name.c_str()));
		TValue *val = map_get(this, args, context);
		delete args;
		
		result->toArr()->add(val, false);
		TValue::free(val);
		
		i = s.find("${", p++);
	}
	if(p < s.size()) {
		i = s.size();
		const std::string buf = s.substr(p, i - p);
		result->toArr()->add(new TValue(buf.c_str(), true), false);
	}
	
	return result;
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TAssignNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *var = childs[0]->run(context);
	TValue *val = childs[1]->run(context);
	if(!var) {
		printf("Node VAR return NULL: %d\n", childs[0]->nodeType);
	}
	else if(!val) {
		printf("Node VAL return NULL: %d\n", childs[1]->nodeType);
	}
	else {
		var->copy(val);
	}

	TValue::free(val);

	CG_LOG_RETURN(var)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TAddEqNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *var = childs[0]->run(context);
	TValue *val = childs[1]->run(context);
	_addValue(var, var, val);
	TValue::free(val);

	CG_LOG_RETURN(var)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TComAndEqNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *var = childs[0]->run(context);
	TValue *val = childs[1]->run(context);
	TValue *result = new TValue();
	result->makeArray();
	result->toArr()->add(var, true);
	result->toArr()->add(val, concat);
	var->copy(result);
	TValue::free(result);
	TValue::free(val);

	CG_LOG_RETURN(var)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TSubEqNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *var = childs[0]->run(context);
	TValue *val = childs[1]->run(context);
	_subValue(var, var, val);
	TValue::free(val);

	CG_LOG_RETURN(var)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TMulEqNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *var = childs[0]->run(context);
	TValue *val = childs[1]->run(context);
	_mulValue(var, var, val);
	TValue::free(val);

	CG_LOG_RETURN(var)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TDivEqNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *var = childs[0]->run(context);
	TValue *val = childs[1]->run(context);
	_divValue(var, var, val);
	TValue::free(val);

	CG_LOG_RETURN(var)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TIncNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *val = childs[0]->run(context);
	TValue *result = new TValue(val);
	val->setValue(val->toInt() + 1);
	TValue::free(val);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TDecNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *val = childs[0]->run(context);
	val->setValue(val->toInt() - 1);

	CG_LOG_RETURN(val)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TComAndNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *result = new TValue();
	result->makeArray();
	TValue *op1 = childs[0]->run(context);
	TValue *op2 = childs[1]->run(context);
	result->toArr()->add(op1, true);
	result->toArr()->add(op2, concat);
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TArrIndexNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *val = childs[0]->run(context);
	TValue *index = childs[1]->run(context);
	TValue *result;
	switch (val->getType()) {
		case DATA_STR: {
			char *ch = new char[2];
			const char *s = val->toStr();
			int i = index->toInt();
			if(i < strlen(s)) {
				ch[0] = s[i];
				ch[1] = '\0';
				result = new TValue();
				result->setValueStr(ch);
			}
			break;
		}
		case DATA_ARRAY: {
			int i = index->toInt();
			if(i < val->toArr()->size())
				result = val->toArr()->at(i)->duplicate();
			else
				result = new TValue();
			break;
		}
		case DATA_OBJECT: {
			TArrayObject *obj = dynamic_cast<TArrayObject*>(val->toScriptObj());
			if(obj) {
				result = obj->get(index->toInt());
			}
			break;
		}
		default:
			result = new TValue();
	}
	TValue::free(index);
	TValue::free(val);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TReturnNode::run(Context &context) {
	CG_LOG_BEGIN

	if(count) {
		TValue *ret = childs[0]->run(context);
		context.func->returnValue(new TValue(ret));
		TValue::free(ret);
	}
	context._exit();

	CG_LOG_RETURN(NULL)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TIntObjectNode::TIntObjectNode(TScriptObject *obj) : TTreeNode(ND_INT_OBJECT) {
	value = new TValue(obj);
}

TValue *TIntObjectNode::run(Context &context) {
	return value->duplicate();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TDotNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *object = childs[0]->run(context);
	TValue *field = childs[1]->run(context);
	TValue *result = NULL;

	TScriptObject *obj = object->toScriptObj();
	if(obj) {
		script_proc proc = obj->getProc(field, context);
		if(proc != -1) {
			if((parent->nodeType == ND_CALL && parent->childs[0] == this) || !obj->isField(proc, context)) {
				CALL_TRACE(field->toStr())
				result = new TValue(new TScriptProc(obj, proc));
			}
			else {
				result = obj->execMethod(this, proc, context);
			}
		}
		else {
			result = new TValue("Field undefined");
			CG_LOG_INFO("Field undefined")
		}
	}
	else {
		result = new TValue("Object undefined");
		CG_LOG_INFO("Object undefined")
	}

	TValue::free(object);
	TValue::free(field);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TCallNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *procValue = childs[0]->run(context);

	TArgs *args = new TArgs();
	TScriptProc *proc = procValue->toProc();
	TValue *val = NULL;
	if(proc) {
		for (int i = 1; i < count; i++)
			args->add(childs[i]->run(context));
		val = proc->run(this, args, context);
	}
	else {
		val = new TValue("Proc undefined");
	}
	delete args;

	TValue::free(procValue);

	CG_LOG_RETURN(val)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TProcNode::TProcNode(TScriptProc *proc) : TTreeNode(ND_PROC) {
	value = new TValue(proc);
}

TValue *TProcNode::run(Context &context) {
	return value->duplicate();
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TExpIfNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *r = childs[0]->run(context);
	TValue *val = r->toBool() ? childs[1]->run(context) : childs[2]->run(context);
	TValue::free(r);

	CG_LOG_RETURN(val)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TCastNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *r = childs[0]->run(context);
	TValue *val = convert(r, type, context);
//	TValue::free(r);

	CG_LOG_RETURN(val)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TBitAndNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = NULL;
	TValue *op2 = NULL;
	TValue *result = new TValue((op1 = childs[0]->run(context))->toInt() & (op2 = childs[1]->run(context))->toInt());
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TBitOrNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = NULL;
	TValue *op2 = NULL;
	TValue *result = new TValue((op1 = childs[0]->run(context))->toInt() | (op2 = childs[1]->run(context))->toInt());
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TShiftLeftNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = childs[0]->run(context);
	TValue *op2 = childs[1]->run(context);
	TValue *result = NULL;
	if(op1->isObject()) {
		result = op1->toScriptObj()->putStream(this, op2, context);
	}
	else {
		result = new TValue(op1->toInt() << op2->toInt());
	}
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TShiftRightNode::run(Context &context) {
	CG_LOG_BEGIN

	TValue *op1 = NULL;
	TValue *op2 = NULL;
	TValue *result = new TValue((op1 = childs[0]->run(context))->toInt() >> (op2 = childs[1]->run(context))->toInt());
	TValue::free(op1);
	TValue::free(op2);

	CG_LOG_RETURN(result)
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

char *make_str_copy(const char *s) {
	char *res = new char[strlen(s)+1];
	strcpy(res, s);
	return res;
}

TValue *TMethodNode::run(Context &context) {
	CG_LOG_BEGIN

	ElementData *data = (ElementData *)cgt->elGetData(context.element);
	if(data) {
		TValue *fname = childs[0]->run(context);
		const char *_fname = fname->toStr();
		TFuncNode *fnode = data->codeTree->getFunc(_fname);
		if(!fnode) {
			fnode = new TFuncNode(_fname, childs[count-1]);
			data->codeTree->addNode(fnode);
			for(int i = 1; i < count-1; i++) {
				TValue *arg = childs[i]->run(context);
                fnode->args->add(const_cast<char*>(arg->toStr()));
				TValue::free(arg);
			}
		}
		TValue::free(fname);
	}

	CG_LOG_RETURN(new TValue())
}

// ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

TValue *TNewNode::run(Context &context) {
	CG_LOG_BEGIN

	TArgs *args = new TArgs();
	for(int i = 0; i < count; i++) {
		TValue *arg = childs[i]->run(context);
		args->add(arg);
	}
	Context c(context.element, context.entry, args, context.func);
	TScriptObject *obj = createObject(name, this, c);
	delete args;
	
	if(obj == NULL)
		CG_LOG_RETURN(new TValue("Class not found"))

	CG_LOG_RETURN(new TValue(obj))
}
