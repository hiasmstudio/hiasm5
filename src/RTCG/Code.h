#ifndef _CODE_
#define _CODE_

#include "FuncArgs.h"
#include "CGTShare.h"
#include "CodeTree.h"

// Template units of a pack are code/<unit>.hws, named like their configuration (conf/<unit>.ini).
// Packs written before that keep the hi prefix (code/hi<unit>.hws), which is still read.
std::string templateUnitName(const char *codePath, const char *unit);
bool hasTemplateUnit(const char *codePath, const char *unit);

class TUnitNode;

class TCode {
  private:
  public:
    char *name;
	char codePath[512];
	TUnitNode *root;
  
    TCode();
    ~TCode();
	
    TValue* run(id_element e, const char *entry, TArgs *args);
    int parseUnit(id_element e, const char *unit);
	
	void dump();
};

#endif
