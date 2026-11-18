#ifndef _RUNNER_
#define _RUNNER_

#include "Runner.h"
#include "direct.h"
#include "Objects.h"
#include "share.h"
#include "ElementData.h"

TElementRunner *erun;
extern PCodeGenTools cgt;

TElementRunner::TElementRunner() {
	CG_LOG_BEGIN

	tools = NULL;
	failed = false;
}

TElementRunner::~TElementRunner() {
	for(TCode *item : codeList)
		delete item;
	codeList.clear();
	if(tools)
		delete tools;

	CG_LOG_END
}

int TElementRunner::Add(TCode *code) {
	CG_LOG_BEGIN

	codeList.push_back(code);

	CG_LOG_RETURN(0)
}

TCode *TElementRunner::find(const char *unit) {
	CG_LOG_BEGIN

	ASSERT(unit, "unit is NULL")

	for(TCode *item : codeList)
		if (strcasecmp(item->name, unit) == 0)
			CG_LOG_RETURN(item)

	CG_LOG_RETURN(NULL)
}

int TElementRunner::init(id_element e) {
	CG_LOG_BEGIN

	if(cgt->elGetData(e))
		CG_LOG_RETURN(0)
	
	const char *name = cgt->elGetClassName(e);
	TCode *code = find(name);
	if (!code) {
		code = new TCode();
		if(code->parseUnit(e, name) != 0) {
			failed = true;
			delete code;
			return -1;
		}
		Add(code);
	}
	
	ElementData *data = new ElementData(code->root);
	cgt->elSetData(e, data);
	TArgs *args = new TArgs();
	TValue::free(code->run(e, "init", args));
	delete args;

	CG_LOG_RETURN(0)
}

TValue* TElementRunner::run(id_element e, const char *entry, TArgs *args) {
	CG_LOG_BEGIN

	if(!e || init(e))
		CG_LOG_RETURN(NULL)
	
	TCode *code = find(cgt->elGetClassName(e));
	ASSERT(code, "runner not found")
	
	TValue *ret = code->run(e, entry, args);

	CG_LOG_RETURN(ret)
}

int TElementRunner::createTools(id_element e) {
	CG_LOG_BEGIN

	// Packs do not have to ship a Sys unit (code/Sys.hws or code/hiSys.hws). The built-in sys/lng
	// objects already provide the common operations, so the missing unit is not an error.
	char codePath[512];
	*(void**)codePath = e;
	cgt->GetParam(PARAM_CODE_PATH, codePath);
	if(!hasTemplateUnit(codePath, "Sys")) {
		tools = NULL;
		CG_LOG_RETURN(0)
	}

	tools = new TCode();
	if(tools->parseUnit(e, "Sys") != 0) {
		// Some packages do not ship a hiSys.hws. The built-in sys/lng objects
		// already provide the common operations, so generation can continue.
		delete tools;
		tools = NULL;
		CG_LOG_RETURN(0)
	}
	TSysObject *t = (TSysObject*)searchObject("sys");
	t->root = tools->root;
	
	TArgs *args = new TArgs();
	args->add(new TValue(cgt->elGetClassName(e)));
	TValue::free(tools->run(e, "create", args));
	delete args;
	
	CG_LOG_RETURN(0)
}

int TElementRunner::destroyTools(id_element e) {
	CG_LOG_BEGIN

	if(!tools)
		CG_LOG_RETURN(0)

	TArgs *args = new TArgs();
	args->add(new TValue(cgt->elGetClassName(e)));
	TValue::free(tools->run(e, "destroy", args));
	delete args;
	
	CG_LOG_RETURN(0)
}

#endif
