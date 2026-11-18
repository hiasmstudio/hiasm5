#ifndef _SHARE_H_
#define _SHARE_H_

#include <string>
#include <vector>

extern void _log_text_(std::string text);
extern void _log_clear_();
extern std::string __level;
extern std::string __call_level;

#ifndef NULL
#define NULL 0
#endif

#define LOG_
#define CALL_TRACE_

#define _DUMP_CODE_TREE

#ifdef _LOG_
#define cg_log(_t)  std::string __buf; _log_text_(_t);
#define CG_LOG_BEGIN {cg_log(__buf.append(__level).append("+ ").append(__PRETTY_FUNCTION__).append("[").append("").append("]\n")) inclevel(); }
#define CG_LOG_END {declevel(); cg_log(__buf.append(__level).append("- ").append(__PRETTY_FUNCTION__).append("[").append("").append("]\n")) }
#define CG_LOG_RETURN(_t) { CG_LOG_END return _t;}
#define CG_LOG_INFO(_t) { cg_log(__buf.append(__level).append("...").append(_t).append("\n")) }
#else
#define cg_log(_t)
#define CG_LOG_BEGIN
#define CG_LOG_END
#define CG_LOG_RETURN(_t) return _t;
#define CG_LOG_INFO(_t)
#endif

#ifdef _CALL_TRACE_
#	define CALL_TRACE(name) printf("%s: %s\n", __call_level.c_str(), name);
#	define CALL_TRACE_BEGIN(name) {CALL_TRACE(name) __call_level.append("  "); }
#	define CALL_TRACE_END(name) {__call_level.resize(__call_level.size()-2); CALL_TRACE(name) }
#else
#	define CALL_TRACE(name)
#	define CALL_TRACE_BEGIN(name)
#	define CALL_TRACE_END(name)
#endif

#define cg_assert_type(_x, _t) if(!(_x)) {cg_log("assert error: \n") return _t;}
#define cg_assert(_x) cg_assert_type(_x, -1)
#define cg_assertv(_x) cg_assert_type(_x, NULL)

#define ASSERT(x, msg) if(!x) { printf("FATAL[%s,%s:%d]: %s\n", __FILE__, __FUNCTION__, __LINE__, msg); }

extern void inclevel();
extern void declevel();

typedef struct {
    const char *entry;
    const char *name;
	
    const char *str_del_o;
    const char *str_del_c;
    const char *op_del;
    const char *var_mask;
    void (*tostr_proc)(char *s);
} TLangRec;

// Code generation profile. Packs that need language sections describe them in
// code/codegen.ini; legacy packs without the file keep the old C++ behaviour.
struct TCGSection {
    std::string entry;
    std::string name;
    std::string quote;
    std::string concat;
    std::string mask;
    std::string escape;
};

extern std::vector<TCGSection> cgSections;
extern int cgSection;

bool cgLoadSections(const char *codePath);
int cgFindSection(const char *entry);
int cgSectionByName(const char *name);
void cgUseSection(int index);

#endif
