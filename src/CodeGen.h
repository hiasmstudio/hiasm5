/*
 * CodeGen.h
 *
 *  Created on: Nov 9, 2010
 *      Author: dilma
 */

#ifndef CODEGEN_H_
#define CODEGEN_H_

#include "SDK.h"

typedef enum {
  PARAM_CODE_PATH         = 0,
  PARAM_DEBUG_MODE,
  PARAM_DEBUG_SERVER_PORT,
  PARAM_DEBUG_CLIENT_PORT,
  PARAM_PROJECT_PATH,
  PARAM_HIASM_VERSION,
  PARAM_USER_NAME,
  PARAM_USER_MAIL,
  PARAM_PROJECT_NAME,
  PARAM_SDE_WIDTH,
  PARAM_SDE_HEIGHT,
  PARAM_COMPILER
} CGT_PARAMS;


#define CGT_SIZE 85

#define GetCGTools CGT
#define GetCGT_Count CGT_SIZE

extern void *CGT[CGT_SIZE];

extern Event cgt_on_debug;

class Resources : public std::list<ustring> {
	public:
		int icons;

		void init() { clear(); icons = 0; }
		inline int nextIcon() { return icons++; }
};

extern Resources resources;

#endif /* CODEGEN_H_ */
