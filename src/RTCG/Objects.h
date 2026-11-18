#ifndef _OBJECTS_H_
#define _OBJECTS_H_

#include "FuncArgs.h"
#include "CodeTree.h"
#include "CGTShare.h"
#include <string>
#include <list>

typedef struct {
	const char *name;
	int count;
	bool field;
} TSORecord;

class TArgs;

class TBlockObject;

class TScriptObject {
protected:
	int ref;
	char *name;
	TSORecord *mtdNames;
	int count;
	TValue *nameValue;
	
	TSORecord makeMethod(const char *n, int c);
	TSORecord makeField(const char *n);
public:
	TScriptObject(const char *_name);
	virtual ~TScriptObject();

	inline void incRef() {
		ref++;
	}

	inline void decRef() {
		ASSERT(ref, "Try to remove object with ref = 0")
		ref--;
	}

	inline bool isDeletable() {
		return ref != -1;
	}

	inline bool isDelete() {
		return ref == 0;
	}
	
	inline int refs() {
		return ref;
	}
	
	bool isObject(const char *name) { return strcasecmp(this->name, name) == 0; }

	virtual script_proc getProc(TValue *val, Context &context);
	virtual bool isField(script_proc index, Context &context);
	virtual TValue *execMethod(TTreeNode *node, long index, Context &context) = 0;
	
	virtual TValue *getName();
	
	virtual TValue *putStream(TTreeNode *node, TValue *value, Context &context) { return new TValue(); }
	
	virtual void create(TTreeNode *node, Context &context) {}
};

class TBlockItem {
private:
	int expand;
public:
	char *name;
	TValue **items;
	int count;

	TBlockItem(const char *name);
	~TBlockItem();

	void addValue(TValue *item);
	void clear();
	std::string asText();
	std::string asCode();
};

class TBlockItemObject : public TScriptObject {
private:
	std::string level;
	bool firstPrint;
	bool levelEnabled;
	TBlockObject *owner;

	void save(const char *fname, bool resource);
	void load(const char *fname);
public:
	TBlockItem *block;

	TBlockItemObject(const char *name, TBlockObject *owner = nullptr);
	~TBlockItemObject();

	void print(TArgs *args);
	void println(TArgs *args);
	void copyHere(TArgs *args);
	void copyTo(TArgs *args);
	void mergeFrom(TArgs *args);
	void incLevel();
	void decLevel();
	int getLevel() const { return static_cast<int>(level.size()); }
	void levelOff() { levelEnabled = false; }
	void levelOn() { levelEnabled = true; }
	bool levelEnabledFlag() const { return levelEnabled; }
	void saveToFile(const char *fname, bool resource) { save(fname, resource); }

	TValue *execMethod(TTreeNode *node, long index, Context &context);
	TValue *putStream(TTreeNode *node, TValue *value, Context &context);
};

class TBlockObject : public TScriptObject {
private:
	typedef std::list<TBlockItemObject *> TBlockItems;
	TBlockItems items;
	int regIndex;
	TBlockItemObject *current;

	TBlockItemObject *regBlock(const char *name);
	bool removeBlock(const char *name);
public:
	TBlockObject();
	~TBlockObject();

	TBlockItemObject *searchBlock(const char *name);
	TBlockItemObject *currentBlock() { return current; }
	TValue *selectBlock(const char *name);

	TValue *execMethod(TTreeNode *node, long index, Context &context);
};

class TLngObject : public TScriptObject {
private:
public:
	TLngObject();

	TValue *execMethod(TTreeNode *node, long index, Context &context);
};

class TCGTObject : public TScriptObject {
public:
	TCGTObject();
	TValue *execMethod(TTreeNode *node, long index, Context &context);
};

extern TBlockObject *cgBlockObject;
extern id_sdk cgSdk;
TBlockItemObject *currentBlockItem();

class TUnitNode;
class TSysObject : public TScriptObject {
private:
public:
	TUnitNode *root;
	
	TSysObject();
	~TSysObject() { CG_LOG_END }
	TValue *execMethod(TTreeNode *node, long index, Context &context);
	script_proc getProc(TValue *val, Context &context);
	bool isField(script_proc index, Context &context);
};

class TElementObject : public TScriptObject {
private:
	id_element element;
	
	void initMap();
public:
	TElementObject();
	TElementObject(id_element element);
	~TElementObject() { CG_LOG_END }

	TValue *execMethod(TTreeNode *node, long index, Context &context);
	script_proc getProc(TValue *val, Context &context);
	bool isField(script_proc index, Context &context);
};

class TPointObject : public TScriptObject {
private:
	id_point point;
public:
	TPointObject(id_point point);
	~TPointObject() { CG_LOG_END }

	TValue *execMethod(TTreeNode *node, long index, Context &context);
};

class TPropertyObject : public TScriptObject {
private:
	id_prop prop;
	id_element element;
	
	TValue *read();
public:
	TPropertyObject(id_element parent, id_prop prop);
	~TPropertyObject() { CG_LOG_END }

	TValue *execMethod(TTreeNode *node, long index, Context &context);
	TValue *getName();
};

class TSDKObject : public TScriptObject {
private:
	id_sdk sdk;

	void initMap();
	void initAll();
	void connectAll();
public:
	TSDKObject(id_sdk sdk);
	~TSDKObject() { CG_LOG_END }

	TValue *execMethod(TTreeNode *node, long index, Context &context);
};

class TArgsObject : public TScriptObject {
private:
public:
	TArgsObject();

	TValue *execMethod(TTreeNode *node, long index, Context &context);
	script_proc getProc(TValue *val, Context &context);
	bool isField(script_proc index, Context &context);
};

class TArrayObject : public TScriptObject {
private:
	std::vector<TValue*> list;
	
	void clear();
	TValue *join(TValue *delimiter);
	bool contain(TValue *value);
public:
	TArrayObject();
	~TArrayObject();

	TValue *execMethod(TTreeNode *node, long index, Context &context);
	void create(TTreeNode *node, Context &context);
	
	TValue *putStream(TTreeNode *node, TValue *value, Context &context);
	
	static TScriptObject *createInstance() { return new TArrayObject();  }
	
	void add(TValue *value);
	TValue *get(int index);
};

extern TScriptObject **objs;

void initObjects();
TScriptObject *searchObject(const char *name);
TScriptObject *copyObject(TScriptObject *obj);

void deleteObject(TScriptObject *obj);
void clearObjects();
TScriptObject *createObject(const char *name, TTreeNode *node, Context &context);

#endif
