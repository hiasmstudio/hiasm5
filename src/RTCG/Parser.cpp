#include <stdio.h>
#include <fcntl.h>

#include "Parser.h"
#include "direct.h"
#include "share.h"
#include "IntFunc.h"
#include "CodeTree.h"

TParser::TParser() {
	CG_LOG_BEGIN

	text = NULL;
	line = NULL;
	curLine = 0;
//	curPosition = 0;
	section = 0;
	langLevel = 0;
	curSwitch = NULL;
	element = NULL;
	rtoken = NULL;
	token = NULL;
	curNode = NULL;
	curFunc = NULL;
}

TParser::~TParser() {
	if (text)
		delete[] text;

	CG_LOG_END
}

int TParser::readLine() {
	CG_LOG_BEGIN

	char c = *line;
	line++;
	if (c == '\r' && *line == '\n') // unicode text support
		line++;
	curLine++;
	curPosition = 0;

#ifdef _LOG_
	char *t = line;
	while(*t != '\0' && *t != '\n' && *t != '\r')
		t++;
	int l = t - line;
	char *_buf = new char[l+1];
	strncpy(_buf, line, l);
	_buf[l] = '\0';
	CG_LOG_INFO(_buf)
	delete _buf;
#endif

	CG_LOG_RETURN(0)
}

int TParser::putToken() {
	CG_LOG_BEGIN

	line = old_line;
	curLine = old_curLine;
	curPosition = old_curPosition;

	CG_LOG_RETURN(0)
}

#define IS_NL(t) (t == '\r' || t == '\n')

int TParser::getToken() {
	CG_LOG_BEGIN

	int i, err = 0;
	char *buf;
	oldTokType = tokType;
	tokType = 0;
	old_line = line;
	old_curLine = curLine;
	old_curPosition = curPosition;
	do {
		if (*line == ' ' || *line == '\t')
			line++;
		else if ((*line >= 'a' && *line <= 'z') || (*line >= 'A' && *line <= 'Z') || *line == '_') {
			buf = line;
			do {
				line++;
			} while ((*line >= 'a' && *line <= 'z') || (*line >= 'A' && *line <= 'Z') || *line == '_' || (*line >= '0' && *line <= '9'));
			i = (int) (line - buf);
			if (token == rtoken)
				token = NULL;
			rtoken = (char *) realloc(rtoken, sizeof (char) *(i + 1));
			token = (char *) realloc(token, sizeof (char) *(i + 1));
			strncpy(rtoken, buf, i);
			rtoken[i] = '\0';
			lowerCase(token, rtoken);
			tokType = TokName;
		} else if (*line >= '0' && *line <= '9') {
			buf = line;
			do {
				line++;
			} while (*line >= '0' && *line <= '9');
			if(*line == '.') {
				tokType = TokReal;
				do {
					line++;
				} while (*line >= '0' && *line <= '9');
			}
			else {
				tokType = TokNumber;
			}
			i = (int) (line - buf);
			if (token == rtoken)
				token = NULL;
			rtoken = (char *) realloc(rtoken, sizeof (char) *(i + 1));
			token = rtoken;
			strncpy(rtoken, buf, i);
			rtoken[i] = '\0';
		} else if (*line == '[') {
			tokType = TokArrOpen;
			line++;
		} else if (*line == ']') {
			tokType = TokArrClose;
			line++;
		} else if (*line == '$') {
			tokType = TokFVar;
			line++;
		} else if (*line == '.') {
			tokType = TokDot;
			line++;
		} else if (*line == '(' || *line == ')' || *line == ',' || *line == ':' || *line == '?' || *line == ';') {
			if (token == rtoken)
				token = NULL;
			rtoken = (char *) realloc(rtoken, sizeof (char) *2);
			token = rtoken;
			rtoken[0] = *line;
			rtoken[1] = '\0';
			line++;
			tokType = TokSymbol;
		} else if (*line == '=') {
			line++;
			if (*line == '=') {
				line++;
				tokType = TokSymEq;
			} else
				tokType = TokAssign;
		} else if (*line == '>' && *(line + 1) == '=') {
			tokType = TokSymLesThenEq;
			line += 2;
		} else if (*line == '>' && *(line + 1) == '>') {
			tokType = TokShiftRight;
			line += 2;
		} else if (*line == '<' && *(line + 1) == '<') {
			tokType = TokShiftLeft;
			line += 2;
		} else if (*line == '>') {
			tokType = TokSymLesThen;
			line++;
		} else if ((*line == '!' && *(line + 1) == '=') || (*line == '<' && *(line + 1) == '>')) {
			tokType = TokSymNEq;
			line += 2;
		} else if (*line == '<' && *(line + 1) == '=') {
			tokType = TokSymSmalThenEq;
			line += 2;
		} else if (*line == '<') {
			tokType = TokSymSmalThen;
			line++;
		} else if (*line == '&') {
			line++;
			if(*line == '&') {
				line++;
				if(*line == '=') {
					line++;
					tokType = TokComDAndEq;
				}
				else
					tokType = TokComDAnd;
			}
			else if(*line == '=') {
				line++;
				tokType = TokComAndEq;
			}
			else
				tokType = TokComAnd;
		} else if (*line == '+') {
			line++;
			if (*line == '+') {
				line++;
				tokType = TokMtInc;
			} else if (*line == '=') {
				line++;
				tokType = TokMtAddEq;
			} else
				tokType = TokMtAdd;
		} else if (*line == '-') {
			line++;
			if (*line == '-') {
				line++;
				tokType = TokMtDec;
			} else if (*line == '=') {
				line++;
				tokType = TokMtSubEq;
			} else
				tokType = TokMtSub;
		} else if (*line == '*') {
			line++;
			if (*line == '=') {
				line++;
				tokType = TokMtMulEq;
			} else
				tokType = TokMtMul;
		} else if (*line == '/') {
			line++;
			if (*line == '/') {
				while (*line != '\n' && *line != '\0')
					line++;
			} else if (*line == '=') {
				line++;
				tokType = TokMtDivEq;
			} else
				tokType = TokMtDiv;
		} else if (*line == '"') {
			line++;
			buf = line;
			i = 0;
			while (*line != '"' && *line != '\0') {
				if (*line == '\\') {
					char nc = *(line + 1);
					if (nc == 'r' || nc == 'n' || nc == '"' || nc == '\\') {
						line++;
					}
				}
				i++;
				line++;
			}
			line = buf;
			rtoken = (char *) realloc(rtoken, sizeof (char) *(i + 1));
			token = rtoken;
			while (*line != '"' && *line != '\0') {
				if (*line == '\\' && *(line + 1) == 'r') {
					*token = '\r';
					line++;
				} else if (*line == '\\' && *(line + 1) == '"') {
					*token = '"';
					line++;
				} else if (*line == '\\' && *(line + 1) == 'n') {
					*token = '\n';
					line++;
				} else if (*line == '\\' && *(line + 1) == '\\') {
					*token = '\\';
					line++;
				} else *token = *line;
				token++;
				line++;
			}
			*token = '\0';
			line++;
			token = rtoken;
			tokType = TokString;
		} else if (*line == '\'') {
			line++;
			buf = line;
			i = 0;
			while (*line != '\'' && *line != '\0') {
				if (*line == '\\') {
					char nc = *(line + 1);
					if (nc == 'r' || nc == 'n' || nc == '\'' || nc == '\\') {
						line++;
					}
				}
				else if(*line == '\n') {
					curLine++;
				}
				i++;
				line++;
			}
			line = buf;
			rtoken = (char *) realloc(rtoken, sizeof (char) *(i + 1));
			token = rtoken;
			while (*line != '\'' && *line != '\0') {
				if (*line == '\\' && *(line + 1) == 'r') {
					*token = '\r';
					line++;
				} else if (*line == '\\' && *(line + 1) == '\'') {
					*token = '\'';
					line++;
				} else if (*line == '\\' && *(line + 1) == 'n') {
					*token = '\n';
					line++;
				} else if (*line == '\\' && *(line + 1) == '\\') {
					*token = '\\';
					line++;
				} else *token = *line;
				token++;
				line++;
			}
			*token = '\0';
			line++;
			token = rtoken;
			tokType = TokCode;
		} else if (IS_NL(*line)) {
			readLine();
		}
		else if (*line == '\0')
			CG_LOG_RETURN(0)
		else
			err = 1;
	} while (!tokType && !err);

	curPosition += line - old_line;
//	printf("TOKEN: %s\n", token);
	
	switch (err) {
		case 1:
			// add error to output
			CG_LOG_INFO("unknown token")
			int i = line - text;
			printf("LINE: %d\n", i);
			CG_LOG_RETURN(SYN_UNK_SYMBOL)
	}

	CG_LOG_RETURN(0)
}

int TParser::level1(TTreeNode *node, TTreeNode **rnode) {
	CG_LOG_BEGIN

	*rnode = NULL;
	if (getToken()) CG_LOG_RETURN(EXP_ERROR)

	int ret = level_assign(node, rnode);
	putToken();

	CG_LOG_RETURN(ret)
}

int TParser::level_assign(TTreeNode *node, TTreeNode **rnode) { // =, -=, +=, *=, /=
	CG_LOG_BEGIN

	TTreeNode *op = NULL;
	int ret = level_concat(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	while (tokType == TokAssign || tokType == TokMtAddEq || tokType == TokComAndEq || tokType == TokComDAndEq || tokType == TokMtSubEq || tokType == TokMtMulEq || tokType == TokMtDivEq) {
		op = *rnode;
		switch(tokType) {
			case TokAssign:
				*rnode = new TAssignNode();
				break;
			case TokMtAddEq:
				*rnode = new TAddEqNode();
				break;
			case TokComAndEq:
				*rnode = new TComAndEqNode(true);
				break;
			case TokComDAndEq:
				*rnode = new TComAndEqNode(false);
				break;
			case TokMtSubEq:
				*rnode = new TSubEqNode();
				break;
			case TokMtMulEq:
				*rnode = new TMulEqNode();
				break;
			case TokMtDivEq:
				*rnode = new TDivEqNode();
				break;
		}
		(*rnode)->addNode(op);
		if (getToken()) CG_LOG_RETURN(SYN_ERROR)
		ret = level_concat(node, &op);

		if (ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
		(*rnode)->addNode(op);
	}
	CG_LOG_RETURN(ret)
}

int TParser::level_ternary(TTreeNode *node, TTreeNode **rnode) { // ?
	TTreeNode *op = NULL;
	int ret = level_shift(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	while (tokType == TokSymbol && *token == '?') {
		op = *rnode;
		*rnode = new TExpIfNode();
		(*rnode)->addNode(op);
		ret = level1(node, &op);
		if (ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		(*rnode)->addNode(op);
		if (getToken() || tokType != TokSymbol || *token != ':') CG_LOG_RETURN(EXP_ERROR)
		ret = level1(node, &op);

		if (ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
		(*rnode)->addNode(op);
	}
	CG_LOG_RETURN(ret)
}

int TParser::level_concat(TTreeNode *node, TTreeNode **rnode) { // & &&
	CG_LOG_BEGIN

	TTreeNode *op = NULL;
	int ret = level_or(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	while (tokType == TokComAnd || tokType == TokComDAnd) {
		op = *rnode;
		*rnode = new TComAndNode(tokType == TokComAnd);
		(*rnode)->addNode(op);
		if (getToken()) CG_LOG_RETURN(SYN_ERROR)
		ret = level_or(node, &op);

		if (ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
		(*rnode)->addNode(op);
		// calc expression type
	}
	CG_LOG_RETURN(ret)
}

int TParser::level_or(TTreeNode *node, TTreeNode **rnode) { // or
	CG_LOG_BEGIN

	TTreeNode *op = NULL;
	int ret = level_and(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	while (tokType == TokName && strcmp(rtoken, "or") == 0) {
		op = *rnode;
		*rnode = new TOrNode();
		(*rnode)->addNode(op);
		if (getToken()) CG_LOG_RETURN(SYN_ERROR)
		ret = level_and(node, &op);

		if (ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
		(*rnode)->addNode(op);
		// calc expression type
	}
	CG_LOG_RETURN(ret)
}

int TParser::level_and(TTreeNode *node, TTreeNode **rnode) { // and
	CG_LOG_BEGIN

	TTreeNode *op = NULL;
	int ret = level_compare(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	while (tokType == TokName && strcmp(rtoken, "and") == 0) {
		op = *rnode;
		*rnode = new TAndNode();
		(*rnode)->addNode(op);
		if (getToken()) CG_LOG_RETURN(SYN_ERROR)
		ret = level_compare(node, &op);

		if (ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
		(*rnode)->addNode(op);
		// calc expression type
	}
	CG_LOG_RETURN(ret)
}

int TParser::level_compare(TTreeNode *node, TTreeNode **rnode) { // =,<,>,<=,>=,!=
	CG_LOG_BEGIN

	TTreeNode *op = NULL;
	int ret = level_addsub(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	while ((tokType == TokSymEq && node) || tokType == TokSymLesThen || tokType == TokSymSmalThen ||
			tokType == TokSymLesThenEq || tokType == TokSymSmalThenEq || tokType == TokSymNEq) {
		op = *rnode;
		switch (tokType) {
			case TokSymEq: *rnode = new TEqNode();
				break; // =
			case TokSymSmalThen: *rnode = new TSmallNode();
				break; // <
			case TokSymLesThen: *rnode = new TLessNode();
				break; // >
			case TokSymSmalThenEq: *rnode = new TSmallEqNode();
				break; // <=
			case TokSymLesThenEq: *rnode = new TLessEqNode();
				break; // >=
			case TokSymNEq: *rnode = new TNeqNode();
				break; // !=
		}
		(*rnode)->addNode(op);
		if (getToken()) CG_LOG_RETURN(SYN_ERROR)
		ret = level_addsub(node, &op);

		if (ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
		(*rnode)->addNode(op);
		// calc expression type
	}
	CG_LOG_RETURN(ret)
}

int TParser::level_addsub(TTreeNode *node, TTreeNode **rnode) { // - +
	CG_LOG_BEGIN

	TTreeNode *op = NULL;
	int ret = level_muldiv(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	while (tokType == TokMtSub || tokType == TokMtAdd) {
		op = *rnode;
		if (tokType == TokMtAdd)
			*rnode = new TAddNode();
		else *rnode = new TSubNode();

		(*rnode)->addNode(op);
		if (getToken()) CG_LOG_RETURN(SYN_ERROR)
		ret = level_muldiv(node, &op);

		if (ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
		(*rnode)->addNode(op);
		// calc expression type
	}
	CG_LOG_RETURN(ret)
}

int TParser::level_muldiv(TTreeNode *node, TTreeNode **rnode) { // * /
	CG_LOG_BEGIN

	TTreeNode *op = NULL;
	int ret = level_ternary(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	while (tokType == TokMtMul || tokType == TokMtDiv) {
		op = *rnode;
		if (tokType == TokMtMul)
			*rnode = new TMulNode();
		else *rnode = new TDivNode();
		(*rnode)->addNode(op);
		if (getToken()) CG_LOG_RETURN(EXP_ERROR)
		ret = level_ternary(node, &op);

		if (ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
		(*rnode)->addNode(op);
		// calc expression type
	}
	CG_LOG_RETURN(ret)
}

int TParser::level_shift(TTreeNode *node, TTreeNode **rnode) { // << >>
	CG_LOG_BEGIN

	TTreeNode *op = NULL;
	int ret = level_bit(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	while (tokType == TokShiftLeft || tokType == TokShiftRight) {
		op = *rnode;
		if (tokType == TokShiftLeft)
			*rnode = new TShiftLeftNode();
		else *rnode = new TShiftRightNode();
		(*rnode)->addNode(op);
		if (getToken()) CG_LOG_RETURN(EXP_ERROR)
		ret = level_bit(node, &op);

		if (ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
		(*rnode)->addNode(op);
		// calc expression type
	}
	CG_LOG_RETURN(ret)
}

int TParser::level_bit(TTreeNode *node, TTreeNode **rnode) { // _and_ _or_
	CG_LOG_BEGIN

	TTreeNode *op = NULL;
	int ret = level_not(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	while (tokType == TokName && (strcmp(rtoken, "_and_") == 0 || strcmp(rtoken, "_or_") == 0)) {
		op = *rnode;
		if (strcmp(rtoken, "_and_") == 0)
			*rnode = new TBitAndNode();
		else *rnode = new TBitOrNode();
		(*rnode)->addNode(op);
		if (getToken()) CG_LOG_RETURN(EXP_ERROR)
		ret = level_not(node, &op);

		if (ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
		(*rnode)->addNode(op);
	}
	CG_LOG_RETURN(ret)
}

int TParser::level_not(TTreeNode *node, TTreeNode **rnode) { // !, not, -, $
	CG_LOG_BEGIN

	TTreeNode *nd;
	if (tokType == TokFVar) {
		if (getToken()) CG_LOG_RETURN(EXP_ERROR)
		*rnode = new TVarNode(curFunc->addVar(token));
		token = rtoken;
		if (getToken()) CG_LOG_RETURN(EXP_ERROR)
		CG_LOG_RETURN(EXP_EXP)
		//int ret = level9(node, rnode);
	}
	else if (tokType == TokMtSub) {
		if (getToken()) CG_LOG_RETURN(EXP_ERROR)
		if (level_incdec(node, rnode) & (EXP_ERROR | EXP_EMPTY)) CG_LOG_RETURN(EXP_ERROR)
		nd = *rnode;
		*rnode = new TSubNode();
		(*rnode)->addNode(nd);

		CG_LOG_RETURN(EXP_EXP)
	}
	else if (tokType == TokName && strcmp(token, "not") == 0) {
		if (getToken()) CG_LOG_RETURN(EXP_ERROR)
		if (level_incdec(node, rnode) & (EXP_ERROR | EXP_EMPTY)) CG_LOG_RETURN(EXP_ERROR)
		nd = *rnode;
		*rnode = new TNotNode();
		(*rnode)->addNode(nd);

		CG_LOG_RETURN(EXP_EXP)
	}
	CG_LOG_RETURN(level_incdec(node, rnode))
}

int TParser::level_incdec(TTreeNode *node, TTreeNode **rnode) { // ++, --
	CG_LOG_BEGIN

	int ret = level_callfunc(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	if(tokType == TokMtInc) {
		TTreeNode *nd = *rnode;
		*rnode = new TIncNode();
		(*rnode)->addNode(nd);
		if (getToken()) CG_LOG_RETURN(EXP_ERROR)
	}
	else if(tokType == TokMtDec) {
		TTreeNode *nd = *rnode;
		*rnode = new TDecNode();
		(*rnode)->addNode(nd);
		if (getToken()) CG_LOG_RETURN(EXP_ERROR)
	}
	CG_LOG_RETURN(ret)
}

int TParser::level_callfunc(TTreeNode *node, TTreeNode **rnode) { // ()
	CG_LOG_BEGIN

	int ret = level_index(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	
	int r;
	if((r = level_call(node, rnode)))
		ret = r;
	
	CG_LOG_RETURN(ret)
}

int TParser::level_call(TTreeNode *node, TTreeNode **rnode) { // ()
	CG_LOG_BEGIN

	int ret = 0;
	if (tokType == TokSymbol && *token == '(') {
		TTreeNode *nd = *rnode;
		*rnode = new TCallNode(curLine);
		(*rnode)->addNode(nd);
		putToken();

		if (parseArgs(*rnode, -1) || getToken()) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
	}

	CG_LOG_RETURN(ret)
}

int TParser::level_index(TTreeNode *node, TTreeNode **rnode) { // .
	CG_LOG_BEGIN

	TTreeNode *op = NULL;
	int ret = level12(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	while (tokType == TokDot) {
		op = *rnode;
		*rnode = new TDotNode();
		(*rnode)->addNode(op);
		if (getToken() || tokType != TokName) CG_LOG_RETURN(EXP_ERROR)

		(*rnode)->addNode(new TStringNode(token));	//TODO mb special node for this?
		token = rtoken = NULL;
		/*
		if (((TIntObjectNode*) (op))->getField(token, &index, &type)) {
			(*rnode)->addNode(new TStringNode(index));
			ret = EXP_EXP;
		} else {
			ret = EXP_ERROR;
			cg_log("field not forund")
		}
		*/

		if (getToken()) CG_LOG_RETURN(EXP_ERROR)
		ret = level_call(node, rnode);
		if(ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
		
//		if (getToken() || tokType != TokName) return EXP_ERROR;
	}
	CG_LOG_RETURN(ret)
}

int TParser::level12(TTreeNode *node, TTreeNode **rnode) { // :

	return level_dot(node, rnode);
}

int TParser::level_dot(TTreeNode *node, TTreeNode **rnode) { // []
	CG_LOG_BEGIN

	int ret = level_var(node, rnode);
	if (ret & EXP_ERROR) CG_LOG_RETURN(EXP_ERROR)
	while (tokType == TokArrOpen) {
		TArrIndexNode *arr = new TArrIndexNode();
		if(*rnode == NULL)
			printf("is NULL\n");
		arr->addNode(*rnode);
		
		TTreeNode *op = NULL;
		ret = level1(node, &op);
		if (ret & (EXP_EMPTY | EXP_ERROR)) CG_LOG_RETURN(EXP_ERROR)
		arr->addNode(op);

		if (getToken() || tokType != TokArrClose || getToken()) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
		
		*rnode = arr;
	}
	CG_LOG_RETURN(ret)
}

bool TParser::isInternalFunc(const char *name, int *index) {
	CG_LOG_BEGIN

	for (int i = 0; i < func_map_size; i++)
		if (strcmp(name, func_map[i].name) == 0) {
			*index = i;

			CG_LOG_RETURN(true)
		}
	CG_LOG_RETURN(false)
}

bool TParser::isScriptFunc(const char *name, TTreeNode **node) {
	CG_LOG_BEGIN

	TTreeNode *root = curNode->getRoot();
	cg_assert(root)
	for (int i = 0; i < root->count; i++)
		if (strcmp(name, ((TFuncNode*) root->childs[i])->name) == 0) {
			TFuncNode *fn = (TFuncNode*) root->childs[i];
			if(!fn->matchesSection(cgSection)) continue;
			*node = root->childs[i];
			CG_LOG_RETURN(true)
		}
	CG_LOG_RETURN(false)
}

bool TParser::isArgLexem(const char *name, TVarItem **value) {
	CG_LOG_BEGIN

	CG_LOG_RETURN((*value = curFunc->args->findVarByName(name)))
}

bool TParser::isVarLexem(const char *name, TVarItem **value) {
	CG_LOG_BEGIN

	CG_LOG_RETURN((*value = curFunc->vars->findVarByName(name)))
}

bool TParser::isGVarLexem(const char *name, TVarItem **value) {
	CG_LOG_BEGIN

	CG_LOG_RETURN((*value = gvars->findVarByName(name)))
}

bool TParser::isLangLexem(const char *name, TVarItem **value) {
	CG_LOG_BEGIN

	TUnitNode *unit = (TUnitNode*) curNode->getRoot();
	CG_LOG_RETURN((*value = unit->langVars ? unit->langVars->findVarByName(name) : NULL))
}

bool TParser::isObjectLexem(const char *name, TScriptObject **obj) {
	CG_LOG_BEGIN

	*obj = searchObject(name);

	CG_LOG_RETURN(*obj != NULL)
}

bool TParser::isUserTypeLexem(const char *name, int *type) {
	CG_LOG_BEGIN

	for(TUserTypes::iterator u = userTypes.begin(); u != userTypes.end(); u++)
		if(strcasecmp(name, (*u).name.c_str()) == 0) {
			*type = (*u).type;
			CG_LOG_RETURN(true)
		}

	CG_LOG_RETURN(false)
}

int TParser::level_var(TTreeNode *node, TTreeNode **rnode) {
	CG_LOG_BEGIN

	int ret, index;
	TVarItem *varitem;
	TScriptObject *obj;

	if (tokType == TokNumber) {
		*rnode = new TIntegerNode(atoi(token));
		ret = EXP_CONST;
	}
	else if (tokType == TokReal) {
		*rnode = new TRealNode(atof(token));
		ret = EXP_CONST;
	} else if (tokType == TokString) {
		*rnode = new TStringNode(token);
		token = rtoken = NULL;
		ret = EXP_CONST;
	} else if (tokType == TokCode) {
		*rnode = new TStringNode(token, true);
		token = rtoken = NULL;
		ret = EXP_CONST;
	} else if (tokType == TokSymbol && *rtoken == '(') {
		ret = level1(node, rnode);
		if (getToken()) CG_LOG_RETURN(EXP_ERROR)
		if (tokType != TokSymbol || *rtoken != ')')
			CG_LOG_RETURN(EXP_ERROR)
	} else if (tokType == TokName && (isArgLexem(token, &varitem) || isVarLexem(token, &varitem) || isGVarLexem(token, &varitem) || isLangLexem(token, &varitem))) {
		*rnode = new TVarNode(varitem);
		ret = EXP_EXP;
	} else if (tokType == TokName && strcmp(token, "isdef") == 0) {
		if (isdefLexem(rnode)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
	} else if (tokType == TokName && strcmp(token, "isndef") == 0) {
		if (isdefLexem(rnode, true)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
	} else if (tokType == TokName && strcmp(token, "isset") == 0) {
		if (issetLexem(rnode)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
	} else if (tokType == TokName && strcmp(token, "isnset") == 0) {
		if (issetLexem(rnode, true)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
	} else if (tokType == TokName && strcmp(token, "isprop") == 0) {
		if (ispropLexem(rnode)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
	} else if (tokType == TokName && isInternalFunc(token, &index)) {
		*rnode = new TProcNode(new TScriptProc(func_map[index].proc));
		ret = EXP_EXP;
	} else if (tokType == TokName && isScriptFunc(token, rnode)) {
		*rnode = new TProcNode(new TScriptProc(*rnode));
		ret = EXP_EXP;
	} else if (tokType == TokName && isObjectLexem(token, &obj)) {
		*rnode = new TIntObjectNode(obj);
		ret = EXP_EXP;
	} else if (tokType == TokName && strcmp(token, "new") == 0) {
		if (newLexem(rnode)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
	} else if (tokType == TokName && isUserTypeLexem(token, &index)) {
		if (argUserTypeLexem(rnode, index)) CG_LOG_RETURN(EXP_ERROR)
		ret = EXP_EXP;
	} else if (tokType == TokName) {
		// Bare identifiers in HiAsm scripts are point/property values, not
		// implicit locals. Resolve them through the same get() used by the
		// explicit syntax.
		CG_LOG_INFO("Resolve value")
		char *name = strdup(token);
		TCallNode *call = new TCallNode(curLine);
		call->addNode(new TProcNode(new TScriptProc(map_get)));
		call->addNode(new TStringNode(name));
		*rnode = call;
		token = rtoken;
		ret = EXP_EXP;
	} else ret = EXP_EMPTY;
	if (ret != EXP_EMPTY && getToken()) CG_LOG_RETURN(EXP_ERROR)

	CG_LOG_RETURN(ret)
}

// A call argument may name the language section its point runs in: event(name:java, args).
// The argument becomes map_section_arg(value, "java"); the event then runs in that section.
int TParser::argSection(TTreeNode **rnode) {
	CG_LOG_BEGIN

	if (getToken()) {
		putToken();
		CG_LOG_RETURN(0)
	}
	if (tokType != TokSymbol || *token != ':') {
		putToken();
		CG_LOG_RETURN(0)
	}
	if (getToken() || tokType != TokName) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if (cgSectionByName(token) < 0) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	TCallNode *call = new TCallNode(curLine);
	call->addNode(new TProcNode(new TScriptProc(map_section_arg)));
	call->addNode(*rnode);
	call->addNode(new TStringNode(strdup(token)));
	*rnode = call;

	CG_LOG_RETURN(0)
}

int TParser::parseArgs(TTreeNode *node, int argsCount) {
	CG_LOG_BEGIN

	TTreeNode *rnode = NULL;
	int count = 0;
	if (getToken() || tokType != TokSymbol || *token != '(') CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	int ret = level1(node, &rnode);
	if (ret & EXP_ERROR)
		CG_LOG_RETURN(SYN_ARGS_FAILED)
	else if (!(ret & EXP_EMPTY)) {
		if (int r = argSection(&rnode)) CG_LOG_RETURN(r)
		node->addNode(rnode);
	}
	while (!getToken() && (tokType != TokSymbol || token[0] != ')')) {
		count++;
		if (tokType == TokSymbol && token[0] == ',') {
			if (level1(node, &rnode) & (EXP_ERROR | EXP_EMPTY))
				CG_LOG_RETURN(SYN_ARGS_FAILED)
			if (int r = argSection(&rnode)) CG_LOG_RETURN(r)
			node->addNode(rnode);
		} else
			CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	}
	count++;

	CG_LOG_RETURN((argsCount == -1 || count == argsCount) ? 0 : SYN_ARGS_COUNT)
}

int TParser::Parse() {
	CG_LOG_BEGIN

	int ret;
	TTreeNode *node;

	while ((ret = getToken()) == 0 && *line) {
		// ... how to optimized this ...
		if (tokType == TokSymbol && *token == ';') {
			ret = SYN_UNK_LEXEM;
			break;
		} else if (strcmp(token, "func") == 0) {
			if ((ret = funcLexem())) break;
		} else if (strcmp(token, "section") == 0) {
			if ((ret = sectionLexem())) break;
		} else if (strcmp(token, "lang") == 0) {
			if ((ret = langLexem())) break;
		} else if (strcmp(token, "switch") == 0) {
			if ((ret = switchLexem())) break;
		} else if (strcmp(token, "case") == 0) {
			if ((ret = caseLexem())) break;
		} else if (strcmp(token, "default") == 0) {
			if ((ret = defaultLexem())) break;
		} else if (strcmp(token, "end") == 0) {
			if ((ret = endLexem())) break;
		} else if (strcmp(token, "if") == 0) {
			if ((ret = ifLexem())) break;
		} else if (strcmp(token, "else") == 0) {
			if ((ret = elseLexem())) break;
		} else if (strcmp(token, "elseif") == 0) {
			if ((ret = elseIfLexem())) break;
		} else if (strcmp(token, "fvar") == 0) {
			if ((ret = fvarLexem())) break;
		} else if (strcmp(token, "var") == 0) {
			if ((ret = fvarLexem())) break;
		} else if (strcmp(token, "gvar") == 0) {
			if ((ret = gvarLexem())) break;
		} else if (strcmp(token, "return") == 0) {
			if ((ret = returnLexem())) break;
		} else if (strcmp(token, "for") == 0) {
			if ((ret = forLexem())) break;
		} else if (strcmp(token, "while") == 0) {
			if ((ret = whileLexem())) break;
		} else if (strcmp(token, "include") == 0) {
			if ((ret = includeLexem())) break;
		} else if (strcmp(token, "register") == 0) {
			if ((ret = registerLexem())) break;
		} else if (strcmp(token, "method") == 0) {
			if ((ret = methodLexem())) break;
		} else {
			putToken();
			if (!(level1(NULL, &node) & (EXP_ERROR | EXP_EMPTY))) {
				if ((ret = expLexem(node))) break;
			} else {
				char error[128];
				sprintf(error, "Unknown lexem[%d]: %s", curLine, parseFile);
				CG_LOG_INFO(error)
				ret = SYN_UNK_LEXEM;
				break;
			}
		}
	}

	CG_LOG_RETURN(ret)
}

int TParser::expLexem(TTreeNode *node) {
	CG_LOG_BEGIN

	int ret;
	TTreeNode *rnode, *nd = NULL;

	if ((ret = getToken())) CG_LOG_RETURN(ret)
	switch (tokType) {
		case TokAssign:
			if (level1(curNode, &rnode) & (EXP_ERROR | EXP_EMPTY)) CG_LOG_RETURN(1)
			nd = new TAssignNode();
			nd->addNode(node);
			nd->addNode(rnode);
			break;
		case TokMtInc:
			nd = new TIncNode();
			nd->addNode(node);
			break;
		case TokMtDec:
			nd = new TDecNode();
			nd->addNode(node);
			break;
		default:
			putToken();
	}
	curNode->addNode(nd ? nd : node);

	CG_LOG_RETURN(ret)
}

int TParser::funcLexem() {
	CG_LOG_BEGIN

	if (getToken() || tokType != TokName) CG_LOG_RETURN(SYN_FUNC_NAME)
	TFuncNode * fnode = new TFuncNode(token);
    free(token); // TFuncNode owns an independent new[] copy, lexer uses realloc.
	token = rtoken;
	curNode->addNode(fnode);
	curNode = fnode;
	curFunc = fnode;

	int ret = 0;
	if (getToken()) CG_LOG_RETURN(SYN_FUNC_NAME)
	if (tokType == TokSymbol && *rtoken == '(') {
		if (getToken()) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
		if (tokType == TokSymbol && *rtoken == ')') {
			if (getToken()) CG_LOG_RETURN(SYN_FUNC_NAME)
		}
		else {
		for (;;) {
			if (tokType != TokName) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
			curFunc->addArg(token);
			free(token); // TArgs stores the name in std::string.
			token = rtoken;
			if (getToken()) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
			if (tokType == TokSymbol && *rtoken == ')') break;
			if (tokType != TokSymbol || *rtoken != ',') CG_LOG_RETURN(SYN_ARGS_SYNTAX)
			if (getToken()) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
		}
		if (getToken()) CG_LOG_RETURN(SYN_FUNC_NAME)
		}
	}

	// Optional language list: func name:java,html or func name:any.
	fnode->sections = section >= 0 && section < 31 ? (1u << section) : 0xffffffffu;
	if (tokType == TokSymbol && *rtoken == ':') {
		if (getToken()) CG_LOG_RETURN(SYN_ERROR)
		for (;;) {
			if (tokType != TokName) CG_LOG_RETURN(SYN_ERROR)
			if (strcmp(token, "any") == 0)
				fnode->sections = 0xffffffffu;
			else {
				const int sec = cgSectionByName(token);
				if (sec < 0) CG_LOG_RETURN(SYN_ERROR)
				if (sec < 31) fnode->sections |= (1u << sec);
			}
			if (getToken()) CG_LOG_RETURN(SYN_ERROR)
			if (tokType == TokSymbol && *rtoken == ',') {
				if (getToken()) CG_LOG_RETURN(SYN_ERROR)
				continue;
			}
			break;
		}
		putToken();
	}
	else putToken();

	CG_LOG_RETURN(0)
}

int TParser::sectionLexem() {
	CG_LOG_BEGIN

	if (getToken() || tokType != TokName) CG_LOG_RETURN(SYN_ERROR)
	const int sec = cgSectionByName(token);
	if (sec < 0) CG_LOG_RETURN(SYN_ERROR)
	section = sec;

	CG_LOG_RETURN(0)
}

int TParser::langLexem() {
	CG_LOG_BEGIN

	TUnitNode *unit = (TUnitNode*) curNode->getRoot();
	if(!unit) CG_LOG_RETURN(SYN_ERROR)
	if (getToken() && (tokType != TokSymbol || *rtoken != '(')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	do {
		if (getToken() || tokType != TokName) CG_LOG_RETURN(SYN_ERROR)
		char *name = strdup(token);
		TVarItem *item = unit->langVars->add(name);
		item->value = new TValue();
		std::string mask = cgSections.empty() ? "%n%i" : cgSections[cgSection].mask;
		if(mask.empty()) mask = "%n%i";
		size_t pos = 0;
		while((pos = mask.find("%n", pos)) != std::string::npos) {
			mask.replace(pos, 2, name);
			pos += strlen(name);
		}
		const char *codeName = element ? cgt->elGetCodeName(element) : "";
		pos = 0;
		while((pos = mask.find("%i", pos)) != std::string::npos) {
			mask.replace(pos, 2, codeName ? codeName : "");
			pos += strlen(codeName ? codeName : "");
		}
		item->value->setValue(mask.c_str(), true);
		free(name);
		if (getToken()) CG_LOG_RETURN(SYN_ERROR)
		if (tokType == TokSymbol && *rtoken == ':') {
			if (getToken() || tokType != TokName) CG_LOG_RETURN(SYN_ERROR)
			if (getToken()) CG_LOG_RETURN(SYN_ERROR)
		}
	} while (tokType == TokSymbol && *rtoken == ',');
	if (tokType != TokSymbol || *rtoken != ')') CG_LOG_RETURN(SYN_ARGS_SYNTAX)

	CG_LOG_RETURN(0)
}

int TParser::switchLexem() {
	CG_LOG_BEGIN

	TSwitchNode *node = (TSwitchNode*) curNode->addNode(new TSwitchNode());
	curSwitch = node;
	int ret = parseArgs(node, -1);
	if(ret) CG_LOG_RETURN(ret)
	curNode = node;

	CG_LOG_RETURN(0)
}

int TParser::caseLexem() {
	CG_LOG_BEGIN

	if(!curSwitch) CG_LOG_RETURN(SYN_ERROR)
	curNode = curSwitch;
	TTreeNode *value = NULL;
	int ret = level1(curSwitch, &value);
	if (ret & (EXP_ERROR | EXP_EMPTY)) CG_LOG_RETURN(SYN_ERROR)
	if (getToken() || tokType != TokSymbol || *token != ':') CG_LOG_RETURN(SYN_ERROR)
	TCaseNode *item = (TCaseNode*) curSwitch->addNode(new TCaseNode());
	item->addNode(value);
	curNode = item->addNode(new TBlockNode());

	CG_LOG_RETURN(0)
}

int TParser::defaultLexem() {
	CG_LOG_BEGIN

	if(!curSwitch) CG_LOG_RETURN(SYN_ERROR)
	curNode = curSwitch;
	if (getToken() || tokType != TokSymbol || *token != ':') CG_LOG_RETURN(SYN_ERROR)
	TDefaultNode *item = (TDefaultNode*) curSwitch->addNode(new TDefaultNode());
	curNode = item->addNode(new TBlockNode());

	CG_LOG_RETURN(0)
}

int TParser::ifLexem() {
	CG_LOG_BEGIN

	curNode = curNode->addNode(new TIfNode());
	int ret = parseArgs(curNode, -1);
	curNode = curNode->addNode(new TBlockNode());

	CG_LOG_RETURN(ret)
}

int TParser::elseLexem() {
	CG_LOG_BEGIN

	if (curNode->parent->nodeType != ND_IF) CG_LOG_RETURN(SYN_IF_EXT)

	curNode = curNode->parent->addNode(new TBlockNode());

	CG_LOG_RETURN(0)
}

int TParser::elseIfLexem() {
	CG_LOG_BEGIN

	if (curNode->parent->nodeType != ND_IF) CG_LOG_RETURN(SYN_IF_EXT)

	curNode = curNode->parent->addNode(new TIfNode());
	int ret = parseArgs(curNode, -1);
	curNode = curNode->addNode(new TBlockNode());

	CG_LOG_RETURN(ret)
}

int TParser::fvarLexem() {
	CG_LOG_BEGIN

	if (!curFunc) CG_LOG_RETURN(SYN_ERROR)

	if (getToken() && (tokType != TokSymbol || *rtoken != '(')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)

	do {
		if (getToken()) CG_LOG_RETURN(SYN_ARGS_FAILED)
		curFunc->addVar(token);
		token = rtoken;
		if (getToken()) CG_LOG_RETURN(SYN_ARGS_FAILED)
		if (tokType != TokSymbol || (*rtoken != ')' && *rtoken != ',')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	} while (tokType == TokSymbol && *rtoken == ',');

	CG_LOG_RETURN(0)
}

int TParser::gvarLexem() {
	CG_LOG_BEGIN

	if (!curFunc) CG_LOG_RETURN(SYN_ERROR)

	if (getToken() && (tokType != TokSymbol || *rtoken != '(')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)

	do {
		if (getToken()) CG_LOG_RETURN(SYN_ARGS_FAILED)
		gvars->add(token)->value = new TValue();
		token = rtoken;
		if (getToken()) CG_LOG_RETURN(SYN_ARGS_FAILED)
		if (tokType != TokSymbol || (*rtoken != ')' && *rtoken != ',')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	} while (tokType == TokSymbol && *rtoken == ',');

	CG_LOG_RETURN(0)
}

int TParser::returnLexem() {
	CG_LOG_BEGIN

	int ret = parseArgs(curNode->addNode(new TReturnNode()), -1);

	CG_LOG_RETURN(ret)
}

int TParser::forLexem() {
	CG_LOG_BEGIN

	int ret;
	TTreeNode *node;
	TForNode *fornode = new TForNode();
	if (getToken() && (tokType != TokSymbol || *rtoken != '(')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if ((ret = level1(curNode, &node)) & EXP_ERROR) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if (getToken() && (tokType != TokSymbol || *rtoken != ';')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if (!(ret & EXP_EMPTY))
		curNode->addNode(node);

	curNode->addNode(fornode);
	curNode = fornode;

	if ((ret = level1(curNode, &node)) & EXP_ERROR) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if (getToken() && (tokType != TokSymbol || *rtoken != ';')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	curNode->addNode(node);

	if ((ret = level1(curNode, &node)) & EXP_ERROR) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if (getToken() && (tokType != TokSymbol || *rtoken != ')')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	curNode->addNode(node);

	TBlockNode * bnode = new TBlockNode();
	curNode->addNode(bnode);
	curNode = bnode;

	CG_LOG_RETURN(0)
}

int TParser::whileLexem() {
	CG_LOG_BEGIN

	int ret;
	TTreeNode *node;
	TForNode *fornode = new TForNode();
	curNode->addNode(fornode);
	curNode = fornode;
	if (getToken() && (tokType != TokSymbol || *rtoken != '(')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if ((ret = level1(curNode, &node)) & EXP_ERROR) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	curNode->addNode(node);
	curNode->addNode(new TBlockNode());
	if (getToken() && (tokType != TokSymbol || *rtoken != ')')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)

	TBlockNode * bnode = new TBlockNode();
	curNode->addNode(bnode);
	curNode = bnode;

	CG_LOG_RETURN(0)
}

int TParser::includeLexem() {
	CG_LOG_BEGIN

	if (getToken() && (tokType != TokSymbol || *rtoken != '(')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if (getToken() || (tokType != TokString && tokType != TokCode)) CG_LOG_RETURN(SYN_ERROR)

	TParser *iParser = new TParser();
	iParser->curFunc = curFunc;
	iParser->createCodeTree(curNode, parsePath, token);
	delete iParser;

	if (getToken() && (tokType != TokSymbol || *rtoken != ')')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)

	CG_LOG_RETURN(0)
}

int TParser::registerLexem() {
	CG_LOG_BEGIN

	TUserType ut;
	if (getToken() && (tokType != TokSymbol || *rtoken != '(')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if (getToken() || tokType != TokNumber) CG_LOG_RETURN(SYN_ERROR)
	ut.type = atoi(token);
	if (getToken() && (tokType != TokSymbol || *rtoken != ',')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if (getToken() || tokType != TokName) CG_LOG_RETURN(SYN_ERROR)
	ut.name = token;
	userTypes.push_back(ut);
	if (getToken() && (tokType != TokSymbol || *rtoken != ')')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)

	CG_LOG_RETURN(0)
}

int TParser::methodLexem() {
	CG_LOG_BEGIN
	
	int ret;
	TTreeNode *node;
	TMethodNode *mtdnode = new TMethodNode();
	curNode->addNode(mtdnode);
	curNode = mtdnode;
	if (getToken() && (tokType != TokSymbol || *rtoken != '(')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	do {
		if ((ret = level1(curNode, &node)) & EXP_ERROR) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
		mtdnode->addNode(node);
		if (getToken() && (tokType != TokSymbol || *rtoken != ')' || *rtoken != ',')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	} while(*rtoken == ',');

	if(mtdnode->count == 0) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	
	TBlockNode * bnode = new TBlockNode();
	mtdnode->addNode(bnode);
	curNode = bnode;
	
	CG_LOG_RETURN(0)
}

static TTreeNode *makeNameCall(int line, internal_proc proc, const char *name) {
	TCallNode *call = new TCallNode(line);
	call->addNode(new TProcNode(new TScriptProc(proc)));
	call->addNode(new TStringNode(strdup(name)));
	return call;
}

int TParser::issetLexem(TTreeNode **node, bool negate) {
	CG_LOG_BEGIN

	if (getToken() && (tokType != TokSymbol || *rtoken != '(')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if (getToken() || tokType != TokName) CG_LOG_RETURN(SYN_ERROR)
	char *name = strdup(token);
	TVarItem *it = NULL;
	bool known = (curFunc && (isArgLexem(name, &it) || isVarLexem(name, &it))) || isGVarLexem(name, &it);
	if(known)
		*node = new TIntegerNode(negate ? 0 : 1);
	else {
		TCallNode *get = new TCallNode(curLine);
		get->addNode(new TProcNode(new TScriptProc(map_get)));
		get->addNode(new TStringNode(strdup(name)));
		TCallNode *call = new TCallNode(curLine);
		call->addNode(new TProcNode(new TScriptProc(negate ? map_isnset : map_isset)));
		call->addNode(get);
		*node = call;
	}
	free(name);
	if (getToken() && (tokType != TokSymbol || *rtoken != ')')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)

	CG_LOG_RETURN(0)
}

int TParser::isdefLexem(TTreeNode **node, bool negate) {
	CG_LOG_BEGIN

	if (getToken() && (tokType != TokSymbol || *rtoken != '(')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if (getToken() || tokType != TokName) CG_LOG_RETURN(SYN_ERROR)
	*node = makeNameCall(curLine, negate ? map_isndef : map_isdef, token);
	if (getToken() && (tokType != TokSymbol || *rtoken != ')')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)

	CG_LOG_RETURN(0)
}

int TParser::ispropLexem(TTreeNode **node) {
	CG_LOG_BEGIN

	if (getToken() && (tokType != TokSymbol || *rtoken != '(')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	if (getToken() || tokType != TokName) CG_LOG_RETURN(SYN_ERROR)
	*node = makeNameCall(curLine, map_isprop, token);
	if (getToken() && (tokType != TokSymbol || *rtoken != ')')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)

	CG_LOG_RETURN(0)
}

int TParser::newLexem(TTreeNode **node) {
	CG_LOG_BEGIN

	if (getToken() || tokType != TokName) CG_LOG_RETURN(SYN_ERROR)
	*node = new TNewNode(token);
	token = rtoken = NULL;
	
	CG_LOG_RETURN(parseArgs(*node, -1))
}

int TParser::argFuncLexem(TVarItem *value) {
	CG_LOG_BEGIN

	TVarNode *varnode = new TVarNode(value);
	curNode->addNode(varnode);

	CG_LOG_RETURN(0)
}

int TParser::argUserTypeLexem(TTreeNode **node, int type) {
	CG_LOG_BEGIN

	if (getToken()) CG_LOG_RETURN(SYN_ERROR)

	if (tokType == TokSymbol && *rtoken == '(') {
		*node = new TCastNode(type);

		TTreeNode *nd;
		int ret;
		if ((ret = level1(*node, &nd)) & EXP_ERROR) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
		(*node)->addNode(nd);
		if (getToken() && (tokType != TokSymbol || *rtoken != ')')) CG_LOG_RETURN(SYN_ARGS_SYNTAX)
	}
	else {
		putToken();
		*node = new TIntegerNode(type);
	}	

	CG_LOG_RETURN(0)
}

int TParser::endLexem() {
	CG_LOG_BEGIN

	if(curSwitch) {
		TTreeNode *node = curNode;
		while(node && node != curSwitch) {
			if(node->nodeType == ND_IF || node->nodeType == ND_FOR)
				break;
			node = node->parent;
		}
		if(node == curSwitch) {
			curNode = curSwitch->parent;
			curSwitch = NULL;
			CG_LOG_RETURN(0)
		}
	}
	if (curNode->hasParent()) {
		curNode = curNode->returnBlock();
	} else {
		CG_LOG_RETURN(SYN_END_LEXEM)
	}

	CG_LOG_RETURN(0)
}

int TParser::createCodeTree(TTreeNode *node, const char *path, const char *file, id_element element) {
	CG_LOG_BEGIN

	errCode = 0;
	this->element = element;
	parseFile = file;
	parsePath = path;
	std::string fileName(path);
	HANDLE f = openUtf8(fileName.append(file).append(".hws").c_str(), "rb");
	if (!f) {
		CG_LOG_INFO("File not found");
		cgt->error(("Script not found: " + fileName).c_str());
		errCode = 1;
		CG_LOG_RETURN(errCode)
	}

	fseek(f, 0L, SEEK_END);
	codeLength = ftell(f);
	fseek(f, 0L, SEEK_SET);

	if (text)
		delete[] text;
	text = new char[codeLength + 1];
	fread(text, codeLength, 1, f);
	text[codeLength] = '\0';
	line = text;
	fclose(f);

#ifdef _LOG_
	char _buf[128];
	sprintf(_buf, "Read: %s, size - %d", file, codeLength);
	CG_LOG_INFO(_buf)
#endif

	curNode = node;

	if ((errCode = Parse())) {
		char buf[512];
		sprintf(buf, "%s[%d, %d]: parse failed, code: %d", file, curLine+1, curPosition, errCode);
		cgt->error(buf);
		CG_LOG_INFO("Parse process failed");
	}

	CG_LOG_RETURN(errCode)
}
