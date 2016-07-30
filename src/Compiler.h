/*
 * Compiler.h
 *
 *  Created on: Dec 5, 2010
 *      Author: dilma
 */

#ifndef COMPILER_H_
#define COMPILER_H_

#include "share.h"
#include "sqlite3.h"

class Compiler {
	private:

	public:
		int id;
		ustring name;
		ustring cmd;
		ustring path;
		ustring ext;

		Compiler();
};

class CompilerCollection : public std::list<Compiler*> {
	private:
		void load();
	public:
		CompilerCollection();
		Compiler *getById(int id);
};

extern CompilerCollection *compilerSet;

#endif /* COMPILER_H_ */
