/* 
 * File:   ElementData.h
 * Author: dilma
 *
 * Created on May 13, 2011, 3:31 PM
 */

#ifndef ELEMENTDATA_H
#define	ELEMENTDATA_H

#include <vector>
#include "FuncArgs.h"
#include "CodeTree.h"

typedef std::vector<TVarItem*> FiledsList;

class ElementData {
protected:
public:
	FiledsList fields;
	int id;
	TUnitNode *codeTree;
	
	ElementData(TUnitNode *codeTree);
	virtual ~ElementData();
	
	TValue *getField(const char *name);
	TValue *addField(const char *name);
	
	int getFieldIndex(const char *name);
	
	static void init();
};

#endif	/* ELEMENTDATA_H */

