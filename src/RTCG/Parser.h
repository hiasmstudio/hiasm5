#ifndef _PARSER_H_
#define _PARSER_H_

#include "CodeTree.h"
#include "Objects.h"

  // token types
#define  TokName    1
#define  TokNumber  2
#define  TokReal    3
#define  TokString  4
#define  TokSymbol  5
#define  TokMath    6
#define  TokCode    7
#define  TokSymEq   8
#define  TokSymNEq  9
#define  TokSymLesThen  10
#define  TokSymSmalThen 11 
#define  TokSymLesThenEq  12
#define  TokSymSmalThenEq 13
#define  TokMtAdd   14
#define  TokMtSub   15
#define  TokMtMul   16
#define  TokMtDiv   17
#define  TokMtAddEq 18
#define  TokMtSubEq 19
#define  TokMtMulEq 20
#define  TokMtDivEq 21
#define  TokMtInc   22
#define  TokMtDec   23
#define  TokComAnd  24
#define  TokComDAnd 25
#define  TokArrOpen  26
#define  TokArrClose 27
#define  TokFVar     28
#define  TokDot      29
#define  TokAssign   30
#define  TokShiftLeft   31
#define  TokShiftRight   32
#define  TokComAndEq     33
#define  TokComDAndEq    34

  // error codes
#define SYN_FUNC_NAME   1
#define SYN_END_LEXEM   2
#define SYN_ARGS_SYNTAX 3
#define SYN_ARGS_FAILED 4
#define SYN_ARGS_COUNT  5
#define SYN_UNK_LEXEM   6
#define SYN_ERROR       7
#define SYN_IF_EXT      8
#define SYN_OBJ_FIELD   9
#define SYN_UNK_SYMBOL  10

  // levelxxx returned
#define EXP_EMPTY 0x01
#define EXP_CONST 0x02
#define EXP_VAR   0x04
#define EXP_EXP   0x08
#define EXP_ERROR 0x10

class TParser {
  private:
    char *text;
    char *line;
    int codeLength;
    int curLine;
    int curPosition;
    int section;
    int langLevel;
    TTreeNode *curSwitch;
    id_element element;
    char *old_line;
    int old_curLine;
    int old_curPosition;
    int tokType;
	int oldTokType;
    char *token;
    char *rtoken;
    TTreeNode *curNode;
    TFuncNode *curFunc;
	const char *parsePath;
	const char *parseFile;
    
    int level1(TTreeNode *node, TTreeNode **rnode);
    int level_assign(TTreeNode *node, TTreeNode **rnode);  // =
    int level_ternary(TTreeNode *node, TTreeNode **rnode);  // ?
    int level_concat(TTreeNode *node, TTreeNode **rnode);  // & &&
    int level_or(TTreeNode *node, TTreeNode **rnode);  // or
    int level_and(TTreeNode *node, TTreeNode **rnode);  // and
    int level_compare(TTreeNode *node, TTreeNode **rnode);  // ==,<,>,<=,>=,!=
    int level_addsub(TTreeNode *node, TTreeNode **rnode);  // - +
    int level_muldiv(TTreeNode *node, TTreeNode **rnode);  // * /
    int level_shift(TTreeNode *node, TTreeNode **rnode);  // << >>
    int level_bit(TTreeNode *node, TTreeNode **rnode);  // _or_ _and_
    int level_not(TTreeNode *node, TTreeNode **rnode);  // !, not, -, $
    int level_incdec(TTreeNode *node, TTreeNode **rnode);  // ++, --
	int level_call(TTreeNode *node, TTreeNode **rnode); // ()
    int level_callfunc(TTreeNode *node, TTreeNode **rnode); // ()
    int level_index(TTreeNode *node, TTreeNode **rnode); // []
    int level12(TTreeNode *node, TTreeNode **rnode); // :
    int level_dot(TTreeNode *node, TTreeNode **rnode); // .
    int level_var(TTreeNode *node, TTreeNode **rnode);
    
    bool isInternalFunc(const char *name, int *index);
    bool isScriptFunc(const char *name, TTreeNode **node);
    bool isArgLexem(const char *name, TVarItem **value);
    bool isVarLexem(const char *name, TVarItem **value);
    bool isGVarLexem(const char *name, TVarItem **value);
    bool isLangLexem(const char *name, TVarItem **value);
    bool isObjectLexem(const char *name, TScriptObject **obj);
	bool isUserTypeLexem(const char *name, int *type);
    
    int expLexem(TTreeNode *node);
    
	/**
	 * func Name[(arg1,arg2...argN)]
     * @return error code
     */
    int funcLexem();
    int sectionLexem();
    int langLexem();
    int switchLexem();
    int caseLexem();
    int defaultLexem();
	/**
	 * end
     * @return error code
     */
    int endLexem();
	/**
	 * if(expression)
     * @return error code
     */
    int ifLexem();
	/**
	 * else
     * @return error code
     */
    int elseLexem();
	/**
	 * elseif(expression)
     * @return error code
     */
    int elseIfLexem();
	/**
	 * fvar(var1,var2...varN)
	 * all undefined names is here
     * @return error code
     */
    int fvarLexem();
	/**
	 * gvar(var1,var2...varN)
	 * this variables visible for all units
     * @return error code
     */
    int gvarLexem();
	/**
	 * return([expression1,expression2...expressionN])
     * @return error code
     */
    int returnLexem();
	/**
	 * for(expression;expression;expression)
     * @return error code
     */
    int forLexem();
	/**
	 * while(expression)
     * @return error code
     */
    int whileLexem();
	/**
	 * include(unit_name)
     * @return error code
     */
	int includeLexem();
	/**
	 * register(type_name, type_id)
	 * register user type
     * @return error code
     */
	int registerLexem();
	/**
	 * method(name[, args])
     * @return error code
     */
	int methodLexem();
	
    int issetLexem(TTreeNode **node, bool negate = false);
    int isdefLexem(TTreeNode **node, bool negate = false);
    int ispropLexem(TTreeNode **node);
    int newLexem(TTreeNode **node);
    int argFuncLexem(TVarItem *value);
	int argUserTypeLexem(TTreeNode **node, int type);
    int argSection(TTreeNode **rnode);
    int parseArgs(TTreeNode *node, int argsCount);
    int Parse();
	
    int readLine();
    int putToken();
    int getToken();
  public:
    int errCode;
    
    TParser();
    ~TParser();
    int createCodeTree(TTreeNode *node, const char *path, const char *file, id_element element = NULL);
};

#endif
