#ifndef _CODETREE_H_
#define _CODETREE_H_

#include "direct.h"
#include "FuncArgs.h"
#include "Objects.h"
#include "CGTShare.h"
#include "share.h"
#include "Code.h"

//#include <windows.h>

typedef enum {
	ND_UNK = 0, ND_UNIT, ND_FUNC, ND_BLOCK, ND_INT, ND_STR, ND_ADD, ND_SUB,
	ND_MUL, ND_DIV, ND_IF, ND_EQ, ND_NEQ, ND_LESTHEN, ND_SMALTHEN, ND_LESTHEN_EQ, ND_SMALTHEN_EQ,
	ND_REAL, ND_OR, ND_AND, ND_NOT, ND_VAR, ND_ASSIGN, ND_COMAND,
	ND_ARRINDEX, ND_RETURN, ND_FOR, ND_INC, ND_DEC, ND_INT_OBJECT, ND_DOT, ND_CALL, ND_USERVAR, ND_PROC,
	ND_EXPIF, ND_ADDEQ, ND_SUBEQ, ND_MULEQ, ND_DIVEQ, ND_CAST, ND_BIT_AND, ND_BIT_OR, ND_SHIFT_LEFT, ND_SHIFT_RIGHT, 
	ND_METHOD, ND_NEW, ND_SWITCH, ND_CASE, ND_DEFAULT, ND_COUNT
} NODE_TYPES;

extern const char *tnodeNames[ND_COUNT];

class TValue;
struct Context;

class TUnitNode;
class TTreeNode {
private:
protected:
	virtual void dump_args(HANDLE f);

	virtual int first(Context &context) { return 0; }
	virtual TTreeNode *next(int &curNode);
public:
	NODE_TYPES nodeType;
	TTreeNode *parent;
	TTreeNode **childs;
	int count;
	TValue *value;

	TTreeNode(NODE_TYPES type);
	virtual ~TTreeNode();
	TTreeNode* addNode(TTreeNode *node);
	void removeNode(TTreeNode *node);
	bool hasParent();
	bool hasChild();
	TUnitNode *getRoot();

	virtual void _continue() {
	}

	virtual TTreeNode *returnBlock() {
		return parent;
	}

	virtual bool isConst() {
		return false;
	}

	inline TValue *getValue() {
		return value;
	}

	virtual TValue *run(Context &context);
	
	int indexOf(TTreeNode *node) { for(int i = 0; i < count; i++) if(node == childs[i]) return i; return -1; }

	void dump(HANDLE f, int tab);
};

class TBlockNode : public TTreeNode {
public:
	TBlockNode() : TTreeNode(ND_BLOCK) { CG_LOG_BEGIN }
	~TBlockNode() { CG_LOG_END }

	TTreeNode *returnBlock() {
		return parent->returnBlock();
	}
};

class TCode;
class TFuncNode;
class TUnitNode : public TTreeNode {
public:
	TCode *codeTree;
	TVarsList *langVars;
	
	TUnitNode(TCode *_codeTree);
	~TUnitNode();

	TValue *run(Context &context);
	TFuncNode *getFunc(const char *name);
};

class TFuncNode : public TTreeNode {
protected:
	virtual void dump_args(HANDLE f);
	TValue *result;
	TTreeNode *body;
public:
	TVarsList *vars;
	TArgs *args;
	char *name;
	unsigned sections;

	TFuncNode(const char *_name);
	TFuncNode(const char *_name, TTreeNode *body);
	~TFuncNode();
	TValue *run(Context &context);
	void addArg(char *name);
	TVarItem* addVar(char *name);
	
	void returnValue(TValue *value);
	bool matchesSection(int section) const {
		return sections == 0xffffffffu || (section >= 0 && section < 32 && (sections & (1u << section)));
	}
};

class TElseNode;

class TIfNode : public TTreeNode {
public:
	TIfNode():TTreeNode(ND_IF) { CG_LOG_BEGIN }
	~TIfNode() { CG_LOG_END }
	TValue *run(Context &context);
	TTreeNode *returnBlock();
};

class TSwitchNode : public TTreeNode {
public:
	TSwitchNode():TTreeNode(ND_SWITCH) { CG_LOG_BEGIN }
	~TSwitchNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TCaseNode : public TTreeNode {
public:
	TCaseNode():TTreeNode(ND_CASE) { CG_LOG_BEGIN }
	~TCaseNode() { CG_LOG_END }
};

class TDefaultNode : public TTreeNode {
public:
	TDefaultNode():TTreeNode(ND_DEFAULT) { CG_LOG_BEGIN }
	~TDefaultNode() { CG_LOG_END }
};

class TForNode : public TTreeNode {
public:
	TForNode():TTreeNode(ND_FOR) { CG_LOG_BEGIN }
	~TForNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TFVarNode : public TTreeNode {
public:
	TFVarNode();
	~TFVarNode();
};

class TEqNode : public TTreeNode {
public:
	TEqNode():TTreeNode(ND_EQ) { CG_LOG_BEGIN }
	~TEqNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TNeqNode : public TTreeNode {
public:
	TNeqNode():TTreeNode(ND_NEQ) { CG_LOG_BEGIN }
	~TNeqNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TLessNode : public TTreeNode {
public:
	TLessNode():TTreeNode(ND_LESTHEN) { CG_LOG_BEGIN }
	~TLessNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TSmallNode : public TTreeNode {
public:
	TSmallNode():TTreeNode(ND_SMALTHEN) { CG_LOG_BEGIN }
	~TSmallNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TLessEqNode : public TTreeNode {
public:
	TLessEqNode():TTreeNode(ND_LESTHEN_EQ) { CG_LOG_BEGIN }
	~TLessEqNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TSmallEqNode : public TTreeNode {
public:
	TSmallEqNode():TTreeNode(ND_SMALTHEN_EQ) { CG_LOG_BEGIN }
	~TSmallEqNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TOrNode : public TTreeNode {
public:
	TOrNode():TTreeNode(ND_OR) { CG_LOG_BEGIN }
	~TOrNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TAndNode : public TTreeNode {
public:
	TAndNode():TTreeNode(ND_AND) { CG_LOG_BEGIN }
	~TAndNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TNotNode : public TTreeNode {
public:
	TNotNode():TTreeNode(ND_NOT) { CG_LOG_BEGIN }
	~TNotNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TMathOpNode : public TTreeNode {
public:
	TMathOpNode(NODE_TYPES type):TTreeNode(type) {}
	bool isConst() { return childs[0]->isConst() && childs[1]->isConst(); }
};

class TAddNode : public TMathOpNode {
public:
	TAddNode():TMathOpNode(ND_ADD) { CG_LOG_BEGIN }
	~TAddNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TSubNode : public TMathOpNode {
public:
	TSubNode():TMathOpNode(ND_SUB) { CG_LOG_BEGIN }
	~TSubNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TMulNode : public TMathOpNode {
public:
	TMulNode():TMathOpNode(ND_MUL) { CG_LOG_BEGIN }
	~TMulNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TDivNode : public TMathOpNode {
public:
	TDivNode():TMathOpNode(ND_DIV) { CG_LOG_BEGIN }
	~TDivNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TConstNode : public TTreeNode {
protected:
	void dump_args(HANDLE f) { fprintf(f, "(%s)", value->toStr()); }
public:
	TConstNode(NODE_TYPES type) : TTreeNode(type) {}
	bool isConst() { return true; }
	TValue *run(Context &context) { return value->duplicate(); }
};

class TIntegerNode : public TConstNode {
public:
	TIntegerNode(int val = 0) : TConstNode(ND_INT) { value = new TValue(val); }
};

class TRealNode : public TConstNode {
public:
	TRealNode(float val = 0) : TConstNode(ND_REAL) { value = new TValue(val); }
};

class TStringNode : public TConstNode {
public:
	TStringNode(char *val = NULL, bool code = false) : TConstNode(ND_STR) {
		value = new TValue();
		value->setValueStr(val);
		if (code) value->flags |= FLG_CODE;
	}
	TValue *run(Context &context);
};

class TVarNode : public TTreeNode {
protected:
	TVarItem *var;
public:
	TVarNode(TVarItem *_var):TTreeNode(ND_VAR) { var = _var; }
	TValue *run(Context &context) { return var->value->duplicate(); }
};

class TScriptFunc : public TTreeNode {
private:
	TTreeNode *fnode;
public:
	TScriptFunc(TTreeNode *node);
	TValue *run(Context &context);
};

class TAssignNode : public TTreeNode {
public:
	TAssignNode():TTreeNode(ND_ASSIGN) { CG_LOG_BEGIN }
	~TAssignNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TAddEqNode : public TTreeNode {
public:
	TAddEqNode():TTreeNode(ND_ADDEQ) { CG_LOG_BEGIN }
	~TAddEqNode() { CG_LOG_END }
	TValue *run(Context &context);
};

// s &= x is s = s & x: the text stays a list of pieces, as TComAndNode builds it.
class TComAndEqNode : public TTreeNode {
	bool concat;
public:
	TComAndEqNode(bool concat):TTreeNode(ND_ADDEQ) { CG_LOG_BEGIN this->concat = concat; }
	~TComAndEqNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TSubEqNode : public TTreeNode {
public:
	TSubEqNode():TTreeNode(ND_SUBEQ) { CG_LOG_BEGIN }
	~TSubEqNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TMulEqNode : public TTreeNode {
public:
	TMulEqNode():TTreeNode(ND_MULEQ) { CG_LOG_BEGIN }
	~TMulEqNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TDivEqNode : public TTreeNode {
public:
	TDivEqNode():TTreeNode(ND_DIVEQ) { CG_LOG_BEGIN }
	~TDivEqNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TIncNode : public TTreeNode {
public:
	TIncNode():TTreeNode(ND_INC) { CG_LOG_BEGIN }
	~TIncNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TDecNode : public TTreeNode {
public:
	TDecNode():TTreeNode(ND_DEC) { CG_LOG_BEGIN }
	~TDecNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TComAndNode : public TTreeNode {
private:
	bool concat;
public:
	TComAndNode(bool concat):TTreeNode(ND_COMAND) { CG_LOG_BEGIN this->concat = concat;}
	~TComAndNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TArrIndexNode : public TTreeNode {
public:
	TArrIndexNode():TTreeNode(ND_ARRINDEX) { CG_LOG_BEGIN }
	~TArrIndexNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TReturnNode : public TTreeNode {
public:
	TReturnNode():TTreeNode(ND_RETURN) { CG_LOG_BEGIN }
	~TReturnNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TIntObjectNode : public TTreeNode {
public:
	TIntObjectNode(TScriptObject *obj);
	TValue *run(Context &context);
};

class TDotNode : public TTreeNode {
public:
	TDotNode():TTreeNode(ND_DOT) { CG_LOG_BEGIN }
	~TDotNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TCallNode : public TTreeNode {
public:
	int line;
	
	TCallNode(int line):TTreeNode(ND_CALL) { this->line = line; CG_LOG_BEGIN }
	~TCallNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TProcNode : public TTreeNode {
public:
	TProcNode(TScriptProc *proc);
	TValue *run(Context &context);
};

class TExpIfNode : public TTreeNode {
public:
	TExpIfNode():TTreeNode(ND_EXPIF) { CG_LOG_BEGIN }
	~TExpIfNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TCastNode : public TTreeNode {
private:
	int type;
public:
	TCastNode(int type):TTreeNode(ND_CAST) { CG_LOG_BEGIN this->type = type; }
	~TCastNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TBitAndNode : public TTreeNode {
public:
	TBitAndNode():TTreeNode(ND_BIT_AND) { CG_LOG_BEGIN }
	~TBitAndNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TBitOrNode : public TTreeNode {
public:
	TBitOrNode():TTreeNode(ND_BIT_OR) { CG_LOG_BEGIN }
	~TBitOrNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TShiftLeftNode : public TTreeNode {
public:
	TShiftLeftNode():TTreeNode(ND_SHIFT_LEFT) { CG_LOG_BEGIN }
	~TShiftLeftNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TShiftRightNode : public TTreeNode {
public:
	TShiftRightNode():TTreeNode(ND_SHIFT_RIGHT) { CG_LOG_BEGIN }
	~TShiftRightNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TMethodNode : public TTreeNode {
public:
	TMethodNode():TTreeNode(ND_METHOD) { CG_LOG_BEGIN }
	~TMethodNode() { CG_LOG_END }
	TValue *run(Context &context);
};

class TNewNode : public TTreeNode {
protected:
	const char *name;
	void dump_args(HANDLE f) { fprintf(f, "(%s)", name); }
public:
	TNewNode(const char *name):TTreeNode(ND_NEW) { CG_LOG_BEGIN this->name = name; }
	~TNewNode() { CG_LOG_END }
	TValue *run(Context &context);
};

struct Context {
	id_element element;
	const char *entry;
	TArgs *args;
	TFuncNode *func;
	int breakLevel;
	int dataArgsIndex;
	
	Context(id_element element, const char *entry, TArgs *args, TFuncNode *func = NULL) {
		this->element = element;
		this->args = args;
		this->entry = entry;
		this->func = func;
		breakLevel = 0;
		dataArgsIndex = 0;
	}
	
	bool needBreak() { if(breakLevel) { breakLevel--; return true; } return false; }
	void _break(int level) { breakLevel = level; }
	void _exit() { breakLevel = 0xFFFF; }
	
	TValue* data() {
		if(this->haveData())
			return args->value(dataArgsIndex++)->duplicate();

		return new TValue();
	}
	bool haveData() {
		return dataArgsIndex < args->size() && args->value(dataArgsIndex)->getType() != DATA_NONE;
	}
};

#endif
