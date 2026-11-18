/*
 * RecentFileManager.h
 *
 *  Created on: Jul 17, 2010
 *      Author: dilma
 */

#ifndef RECENTFILEMANAGER_H_
#define RECENTFILEMANAGER_H_

#include "share.h"

struct RFileInfo {
	ustring name;
	TypePixbuf icon;

	RFileInfo(const ustring &name);
	bool operator == (const RFileInfo &val) { return name == val.name; }
};

class RecentFileManager : public std::list<RFileInfo> {
	private:
		bool changed;
		ustring fileListName;
	public:
		Event on_change;

		RecentFileManager();
		virtual ~RecentFileManager();

		void load();
		void add(const ustring &name);
};

extern RecentFileManager rfMan;

#endif /* RECENTFILEMANAGER_H_ */
