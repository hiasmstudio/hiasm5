#ifndef _FUNC_ARGS_
#define _FUNC_ARGS_

#include "share.h"
#include "direct.h"
#include <string>
#include <vector>

extern int globalRef;

typedef enum {
	DATA_NONE, DATA_INT, DATA_STR, DATA_REAL, DATA_ARRAY, DATA_OBJECT, DATA_PROC, DATA_COUNT
} DATA_TYPES;

typedef enum {
	PROC_SCRIPT, PROC_OBJECT, PROC_INTERNAL, PROC_COUNT
} PROC_TYPES;

// string is code (i.e. 'hello world')
#define FLG_CODE  1
// concat string wo language concatenation (such as 'run'( && 1 && ')' )
#define FLG_WO_OP 2

typedef int script_proc;
class TValue;
class TArgs;
struct Context;

typedef TValue*(*internal_proc)(void *obj, TArgs *args, Context &context);

class TArray;
class TScriptProc;
class TScriptObject;

class TValue {
private:
	char *val_c;
	DATA_TYPES type;
	void *value;

	int ref;

	void init();
public:
	int flags;
	int codeType;

	TValue();
	TValue(TValue *val);
	TValue(const char *val, bool code = false);
	TValue(int val);
	TValue(void *val);
	TValue(bool val);
	TValue(double val);
	TValue(TScriptObject *val);
	TValue(TScriptProc *val);
	~TValue();

	const char *toStr();
	const char *toCode();
	int toInt();
	intptr_t toIntPtr();
	bool toBool();
	double toReal();

	TScriptObject *toScriptObj() {
		return isObject() ? (TScriptObject*)value : NULL;
	}

	TScriptProc *toProc() {
		return isProc() ? (TScriptProc*) value : NULL;
	}

	inline TArray *toArr() {
		return isArray() ? (TArray*) value : NULL;
	}

	inline DATA_TYPES getType() {
		return type;
	}
	inline int getCodeType() {
		return codeType;
	}
	inline void setCodeType(int type) {
		codeType = type;
	}
	
	void setValue(int val);
	void setValue(const char *val, bool code = false);
	void setValue(double val);
	void setValue(TScriptProc *val);
	void setValue(TScriptObject *val);
	void setValueStr(char *val);
	void copy(TValue *value);
	void makeArray();
	void clear();

	inline bool isArray() {
		return type == DATA_ARRAY;
	}

	inline bool isObject() {
		return type == DATA_OBJECT;
	}

	inline bool isProc() {
		return type == DATA_PROC;
	}

	TValue *duplicate() {
		ref++;
		return this;
	}
	static void free(TValue *value) {
		if(!value) return;
		
		value->ref--;
		if(value->ref == 0) {
			delete value;
		}
	}
	static void _dump() {
		printf("Global ref counter: %d\n", globalRef);
	}
	
	static TValue* fromProperty(id_element e, id_prop prop);
};

class TArray : public std::vector<TValue *> {
private:
	std::string buf;
protected:
public:
	virtual ~TArray();
	void add(TValue *item, bool concat);
	const char *toStr();
	const char *toCode();
	void clear();
	void copy(TArray *arr);
	void merge(TArray *arr);
};

class TVarItem {
public:
	std::string name;
	TValue *value;

	TVarItem() { value = NULL; }
	TVarItem(const char *name) { value = NULL; this->name = name; }
	~TVarItem() { if(value) TValue::free(value); }
};

class TVarsList : public std::vector<TVarItem *> {
public:
	virtual ~TVarsList();
	TVarItem *add(char *name);
	TVarItem *add(TValue *value);
	
	inline TValue *value(int i) { return at(i)->value; }
	TVarItem* findVarByName(const char *name);
};

class TArgs : public TVarsList {
private:
public:
};

class TTreeNode;

class TScriptProc {
private:
	void *obj;
	void *proc;
	PROC_TYPES type;
public:

	TScriptProc(TScriptObject *_obj, script_proc _proc);
	TScriptProc(TScriptProc *p);
	TScriptProc(internal_proc _proc);
	TScriptProc(TTreeNode *node);
	~TScriptProc();
	
	TValue *run(TTreeNode *node, TArgs *args, Context &context);
	TValue *read(TTreeNode *node, Context &context);
};

extern TVarsList *gvars;
extern const char *stringLexem;
extern bool cppStringEscapes;
extern const char *concatLexem;

#endif
