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
#include <string>
#include <vector>

class ProjectBuilder {
	private:
		bool buildIneternal(MSDK *sdk, const ustring &name);
		bool buildWithCodeGen(MSDK *sdk, const ustring &name);

		GPid process = 0;
		guint childWatch = 0;
		std::vector<std::string> crossStopCommand;
		ustring crossPidFile, crossStopFile;
		void clearCrossRun();
	public:
		ProjectBuilder();
		~ProjectBuilder();
		Event on_run_state;
		bool isRunning() const { return process != 0; }
		void stop();
		// Used by build, run and callers locating the generated artifact.
		ustring getOutputProjectName(MSDK *sdk);

		/**
		 * Build spheme
		 * @param sdk pointer to scheme
		 */
		bool build(MSDK *sdk);
		/**
		 * Run project
		 * @param sdk pointer to scheme
		 */
		void run(MSDK *sdk);
};

#endif /* PROJECTBUILDER_H_ */
