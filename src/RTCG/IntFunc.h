#ifndef _INTFUNC_H_
#define _INTFUNC_H_

#include "CodeTree.h"
#include "CGTShare.h"

extern PCodeGenTools cgt;

typedef struct {
	const char *name;
	int count;
	internal_proc proc;
	const char *ainfo;
} TFuncMap;

extern const TFuncMap func_map[];
extern const int func_map_size;

typedef struct {
	std::string name;
	int type;
} TUserType;
typedef std::list<TUserType> TUserTypes;

extern TUserTypes userTypes;
extern TValue *convert(TValue *val, int type, Context &context);
extern TValue *map_get(void *node, TArgs *args, Context &context);
extern TValue *map_section_arg(void *node, TArgs *args, Context &context);
extern TValue *map_isdef(void *node, TArgs *args, Context &context);
extern TValue *map_isndef(void *node, TArgs *args, Context &context);
extern TValue *map_isset(void *node, TArgs *args, Context &context);
extern TValue *map_isnset(void *node, TArgs *args, Context &context);
extern TValue *map_isprop(void *node, TArgs *args, Context &context);
extern TValue *map_issec(void *node, TArgs *args, Context &context);
extern TValue *map_count(void *node, TArgs *args, Context &context);
extern TValue *map_e_int(void *node, TArgs *args, Context &context);
extern TValue *map_e_str(void *node, TArgs *args, Context &context);
extern TValue *map_e_real(void *node, TArgs *args, Context &context);

/** A container (MultiElement) or its frame (EditMulti) without a code template of its own: the C++ package
 *  generates one program for all schemes, so such elements only route links. Packages with templates for them
 *  (web: hiMultiElement, hiEditMulti) keep handling the links in those templates. */
extern bool rtcgTransparent(id_element e);
/** The real point linked to p (CGT ptGetRLinkPoint), followed through transparent containers and frames:
 *  a point of a container leads to the element wired to the matching frame point inside, a frame point leads
 *  to the element wired to the matching container point outside. nullptr when nothing is wired there. */
extern id_point rtcgRealLink(id_point p);

#endif
