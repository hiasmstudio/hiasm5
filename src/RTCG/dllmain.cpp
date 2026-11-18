#include "dll.h"

#include "share.h"
#include "direct.h"
#include "Runner.h"
#include "FuncArgs.h"
#include "IntFunc.h"
#include "ElementData.h"

PCodeGenTools cgt;

DLLIMPORT int buildPrepareProc(TBuildPrepareRec *params) {
	CG_LOG_BEGIN

	_log_clear_();

	CG_LOG_RETURN(CG_SUCCESS)
}

void eraseData(id_sdk sdk) {
	CG_LOG_BEGIN

	for(int i = 0; i < cgt->sdkGetCount(sdk); i++) {
		id_element e = cgt->sdkGetElement(sdk, i);
		ElementData *data = (ElementData *)cgt->elGetData(e);
		if(data) {
			delete data;
			cgt->elSetData(e, NULL);
		}
		
		if(cgt->elGetClassIndex(e) == CI_MultiElement) {
			eraseData(cgt->elGetSDK(e));
		}
	}

	CG_LOG_END
}

DLLIMPORT int buildProcessProc(TBuildProcessRec *params) {
	CG_LOG_BEGIN
	if(!params || !params->cgt || !params->sdk) return CG_BUILD_FAILED;
	params->result = NULL;
    cppStringEscapes = false;
	static std::string result;
	result.clear();
	cgt = params->cgt;
	cgSdk = params->sdk;
	initObjects();
	gvars = new TVarsList();
	// PChar is a codegen type, not a property type. The legacy Modules
	// direct.inc registered it at startup; RTCG keeps the name available for
	// scripts that use PChar(expression).
	userTypes.clear();
	TUserType pcharType;
	pcharType.name = "PChar";
	pcharType.type = 100;
	userTypes.push_back(pcharType);
	erun = new TElementRunner();
	TArgs *args = new TArgs();
	int status = CG_ENTRY_POINT_NF;
	ElementData::init();
	try {
	for (int i = 0; i < params->cgt->sdkGetCount(params->sdk); i++) {
		id_element e = params->cgt->sdkGetElement(params->sdk, i);
		if(cgt->elGetFlag(e) & ELEMENT_FLG_IS_PARENT) {
			status = CG_BUILD_FAILED;
			char *codePath = cgt->ReadStrParam(PARAM_CODE_PATH, e);
			cgLoadSections(codePath);
			cgUseSection(cgFindSection(cgt->elGetClassName(e)));
			delete[] codePath;
			if(erun->createTools(e) == 0) {
				TValue::free(erun->run(e, "dostart", args));
				erun->destroyTools(e);
				if(!erun->failed) status = CG_SUCCESS;
			}
			break;
		}
	}
	TBlockObject *block = (TBlockObject*)searchObject("block");
	TBlockItemObject *item = block->searchBlock("result");
	if(!item) item = block->currentBlock();
	if(status == CG_SUCCESS && item) {
		result = item->block->asCode();
		params->result = const_cast<char *>(result.c_str());
	}
	else if(status == CG_SUCCESS) {
		cgt->error("block 'result' not found");
		status = CG_BUILD_FAILED;
	}
	} catch(const std::exception &error) {
		cgt->error(error.what());
		status = CG_BUILD_FAILED;
		params->result = NULL;
	}
	if(status == CG_ENTRY_POINT_NF) cgt->error("Project entry element not found");
	eraseData(params->sdk);
	delete args;
	delete erun;
	erun = NULL;
	delete gvars;
	gvars = NULL;
	cgSdk = NULL;
	clearObjects();
	TValue::_dump();
	CG_LOG_RETURN(status)
}

DLLIMPORT int CheckVersionProc(THiAsmVersion *params) {
	CG_LOG_BEGIN
//	if ((params->major >= 3) && (params->minor >= 63) && (params->build >= 162))
//		return CG_SUCCESS;
	CG_LOG_RETURN(CG_SUCCESS)
}

// RTCG returns a process-lifetime static buffer; the host uses this symbol to
// release plugin results when available.
DLLIMPORT void freeBuildResult(void *result) {
	(void)result;
}

/*
BOOL APIENTRY DllMain (HINSTANCE hInst, DWORD reason, LPVOID reserved) {
	switch (reason) {
	  case DLL_PROCESS_ATTACH:
		break;

	  case DLL_PROCESS_DETACH:
		break;

	  case DLL_THREAD_ATTACH:
		break;

	  case DLL_THREAD_DETACH:
		break;
	}

	return TRUE;
}
 */

int main() {
	printf("LOAD LIB\n");
	return 0;
}
