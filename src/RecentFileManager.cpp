/*
 * RecentFileManager.cpp
 *
 *  Created on: Jul 17, 2010
 *      Author: dilma
 */

#include "RecentFileManager.h"
#include "StringList.h"

RecentFileManager rfMan;

RFileInfo::RFileInfo(const ustring &name) {
	ustring ext = name.substr(name.length()-3, 3).lowercase();
	if(ext == "sha")
		icon = Gdk::Pixbuf::create_from_file(dataDir + ICONS_PATH"components.png");
	else
		icon = Gdk::Pixbuf::create_from_file(dataDir + ICONS_PATH"text.png");
	this->name = name;
}

RecentFileManager::RecentFileManager():on_change(this, CBT_CHANGE_RECENT_LIST) {
	changed = false;
}

void RecentFileManager::load() {
	TRACE_PROC
	fileListName = homeDir + "files.lst";
	if(file_test(fileListName, FILE_TEST_EXISTS)) {
		StringList lst(fileListName);
		for(StringList::iterator l = lst.begin(); l != lst.end(); l++)
			add(*l);
	}
}

RecentFileManager::~RecentFileManager() {
	if(!changed) return;

	StringList lst;
	for(iterator f = begin(); f != end(); f++)
		lst << f->name;
	lst.saveToFile(fileListName);
}

void RecentFileManager::add(const ustring &name) {
	if(!file_test(name, FILE_TEST_EXISTS)) return;

	for(iterator f = begin(); f != end(); f++)
		if(f->name == name) {
			remove(*f);
			break;
		}
	push_back(RFileInfo(name));
	changed = true;

	if(size() > 10)
		remove(*begin());

	on_change.run(NULL);
}
