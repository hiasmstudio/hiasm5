/*
 * ProjectBuilder.h
 *
 *  Created on: 08.08.2010
 *      Author: dilma
 */

#ifndef PROJECTBUILDER_H_
#define PROJECTBUILDER_H_

#include "share.h"
#include "SDK.h"

class ProjectBuilder {
	private:
		void buildIneternal(MSDK *sdk, const ustring &name);
		void buildWithCodeGen(MSDK *sdk, const ustring &name);

		ustring getOutputProjectName(MSDK *sdk);
	public:
		ProjectBuilder();

		/**
		 * Build spheme
		 * @param sdk pointer to scheme
		 */
		void build(MSDK *sdk);
		/**
		 * Run project
		 * @param sdk pointer to scheme
		 */
		void run(MSDK *sdk);
};

#endif /* PROJECTBUILDER_H_ */
