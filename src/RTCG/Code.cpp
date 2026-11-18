#include "Code.h"
#include "direct.h"
#include "Parser.h"
#include "share.h"
#include "IntFunc.h"

//#include <windows.h>

TCode::TCode() {
	CG_LOG_BEGIN

	name = NULL;
	root = new TUnitNode(this);
}

TCode::~TCode() {
	if (name)
		delete[] name;
	delete root;
	
	CG_LOG_END
}

static bool templateFileExists(const char *codePath, const std::string &unit) {
	const std::string path = std::string(codePath) + unit + ".hws";
	if (FILE *f = openUtf8(path.c_str(), "rb")) {
		fclose(f);
		return true;
	}
	return false;
}

std::string templateUnitName(const char *codePath, const char *unit) {
	return templateFileExists(codePath, unit) ? std::string(unit) : std::string("hi") + unit;
}

bool hasTemplateUnit(const char *codePath, const char *unit) {
	return templateFileExists(codePath, unit) || templateFileExists(codePath, std::string("hi") + unit);
}

int TCode::parseUnit(id_element e, const char *unit) {
	CG_LOG_BEGIN

	name = new char[strlen(unit) + 1];
	strcpy(name, unit);

	int ret;
	*(void**)codePath = e;
	cgt->GetParam(PARAM_CODE_PATH, codePath);
	std::string fileName = templateUnitName(codePath, unit);
	CG_LOG_INFO(fileName.c_str())

	TParser *parser = new TParser();
	parser->createCodeTree(root, codePath, fileName.c_str(), e);
#ifdef DUMP_CODE_TREE
	dump();
#endif
	if ((ret = parser->errCode))
		cgt->trace(parser->errCode);
	delete parser;

	CG_LOG_RETURN(ret)
}

TValue* TCode::run(id_element e, const char *entry, TArgs *args) {
	CG_LOG_BEGIN

	Context c(e, entry, args);
	TValue  *ret = root->run(c);

	CG_LOG_RETURN(ret)
}

void TCode::dump() {
	CG_LOG_BEGIN

	std::string s(codePath);
	HANDLE f = fopen(s.append("log/").append(name).append(".log").c_str(), "w+");
	root->dump(f, 0);
	fclose(f);
	
	CG_LOG_END
}
