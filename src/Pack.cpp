/*
 * Pack.cpp
 *
 *  Created on: 03.05.2010
 *      Author: dilma
 */

#include <stdio.h>
#include <string.h>
#include <glib-2.0/glib/gstdio.h>

#include "Pack.h"
#include "MainDataBase.h"
#include "settings.h"

PackCollection *packSet;

ustring PackProject::getTemplateFile() {
	return ustring::compose("%1%2%3/%4%5.sha", dataDir.c_str(), ELEMENTS_PATH, pack->name, ELEMENTS_NEW_PATH, entry);
}

typedef struct {
    void *result;
    const char *prjFilename;
    const char *compiler;
} TBuildMakePrjRec;
typedef int (*TbuildMakePrj)(TBuildMakePrjRec *params);

typedef struct {
    const char *FileName;
    int Mode;
    int ServerPort;
    int ClientPort;
    void *data;
} TBuildRunRec;

typedef int (*TbuildRunProc)(TBuildRunRec *params);

void PackProject::make(const ustring &project, void *result) {
	ustring mdir = pack->pathMake() + lib;

	GModule *handle  = g_module_open(mdir.c_str(), G_MODULE_BIND_LAZY);
	TbuildMakePrj buildMakePrj;
	if(g_module_symbol(handle, "buildMakePrj", (gpointer *)&buildMakePrj)) {
		ustring prj_name = project + "." + ext;
		TBuildMakePrjRec prj;
		prj.compiler = "none";
		prj.prjFilename = prj_name.c_str();
		prj.result = result;
		g_chdir(pack->pathCode().c_str());
		int x = (*buildMakePrj)(&prj);
//		unlink(prj.prjFilename);
		printf("Return code: %dn\n",x);
	}
	else {
		printf("proc not found\n");
	}
	g_module_close(handle);
}

void PackProject::run(const ustring &project) {
	ustring mdir = pack->pathMake() + lib;

	GModule *handle  = g_module_open(mdir.c_str(), G_MODULE_BIND_LAZY);
	TbuildRunProc buildRunProc;
	if(g_module_symbol(handle, "buildRunProc", (gpointer *)&buildRunProc)) {
		TBuildRunRec prj;
		prj.ClientPort = 0;
		prj.ServerPort = 0;
		prj.Mode = 0;
		prj.FileName = project.c_str();
		g_chdir(pack->pathCode().c_str());
		int x = (*buildRunProc)(&prj);
		printf("Return code: %dn\n",x);
	}
	else {
		printf("proc not found\n");
	}
	g_module_close(handle);
}

//------------------------------------------------------------------------------------

Pack::Pack(const ustring &name, const ustring &info) {
	TRACE_PROC
	this->name = name;
	this->info = info;
	title = name;

	ProjectInfo prj;
	void *id = mdb.begin_read_projects(name);
	while(mdb.read_projects(id, prj)) {
		PackProject *p = new PackProject(this);
		p->entry = prj.entry;
		p->ext = prj.ext;
		p->info = prj.info;
		p->lib = prj.make;
		p->name = prj.name;
		projects.push_back(p);
	}
	mdb.end_read(id);

//	load();
}

PackElement *Pack::addElement(const ustring &name, const ustring &info) {
	PackElement *pe = new PackElement();
	pe->name = name;
	pe->info = info;
	pe->conf = NULL;

#ifdef CFG_USE_ELEMENT_ICONS
	ustring icon_name = pathElementIcon(name);
	if(!Glib::file_test(icon_name, FILE_TEST_EXISTS)) {
		icon_name = dataDir + ELEMENTS_NIL_FILE;
	}

	pe->image = Gdk::Pixbuf::create_from_file(icon_name);
	if(!pe->image)
		std::cout << "Error loading icon file: " << icon_name << std::endl;
#endif

	elements.push_back(pe);

	return pe;
}

ustring Pack::path() {
	return ustring::compose("%1%2%3/", dataDir.c_str(), ELEMENTS_PATH, name);
}

ustring Pack::pathElementsDb() {
	return path() + "elements.db";
}

ustring Pack::pathElementConf(const ustring &name) {
	ustring conf = ustring::compose("%1%2%3/%4%5.ini", dataDir.c_str(), ELEMENTS_PATH, this->name, ELEMENTS_CONF_PATH, name);
	if(file_test(conf, FILE_TEST_EXISTS))
		return conf;
	return ustring::compose("%1%2%3/%4%5.ini", dataDir.c_str(), ELEMENTS_PATH, "_base", ELEMENTS_CONF_PATH, name);
}

ustring Pack::pathElementCode(const ustring &name) {
	return ustring::compose("%1%2%3/%4hi%5.hws", dataDir.c_str(),ELEMENTS_PATH, this->name, ELEMENTS_CODE_PATH, name);
}

ustring Pack::pathCode() {
	return ustring::compose("%1%2%3/%4", dataDir.c_str(),ELEMENTS_PATH, this->name, ELEMENTS_CODE_PATH);
}

ustring Pack::pathMake() {
	return ustring::compose("%1%2%3/%4", dataDir.c_str(),ELEMENTS_PATH, this->name, ELEMENTS_MAKE_PATH);
}

ustring _iconPath(const ustring &pack, const ustring &name) {
	ustring f = ustring::compose("%1%2%3/%4%5.ico", dataDir.c_str(), ELEMENTS_PATH, pack, ELEMENTS_ICON_PATH, name);
	if(file_test(f, FILE_TEST_EXISTS))
		return f;
	return ustring::compose("%1%2%3/%4%5.png", dataDir.c_str(), ELEMENTS_PATH, pack, ELEMENTS_ICON_PATH, name);
}

ustring Pack::pathElementIcon(const ustring &name) {
	ustring f;
	if(file_test(f = _iconPath(this->name, name), FILE_TEST_EXISTS))
		return f;

	return _iconPath("_base", name);
}

bool Pack::load() {
	TRACE_PROC
	ustring dir(pathElementsDb());

	PackGroup *pk;
	sqlite3 *ppDb = NULL;
	sqlite3_open(dir.c_str(), &ppDb);
	sqlite3_stmt *ppStmt;
	sqlite3_prepare(ppDb, "SELECT name FROM groups ORDER BY pos", -1, &ppStmt, NULL);
	while(sqlite3_step(ppStmt) != SQLITE_DONE) {
		pk = new PackGroup();
		pk->name = (const char*)sqlite3_column_text(ppStmt, 0);
		groups.push_back(pk);
	}
	sqlite3_finalize(ppStmt);

	for(std::list<PackGroup*>::iterator p = groups.begin(); p != groups.end(); p++) {
		sqlite3_prepare(ppDb, (ustring("SELECT e.name, e.info FROM elements e, groups g WHERE g.name = '") + (*p)->name + "' AND e.tab = g.id ORDER BY e.pos").c_str(), -1, &ppStmt, NULL);
		while(sqlite3_step(ppStmt) != SQLITE_DONE) {
			(*p)->elements.push_back(addElement((const char*)sqlite3_column_text(ppStmt, 0), (const char*)sqlite3_column_text(ppStmt, 1)));
		}
		sqlite3_finalize(ppStmt);
	}
	sqlite3_close(ppDb);

	void *id = mdb.begin_read_pack_compilers(name);
	PackCompilerInfo pc_info;
	while(mdb.read_pack_compilers(id, pc_info)) {
		Compiler *cmp = compilerSet->getById(pc_info.compiler);
		if(cmp) {
			compilers.push_back(CompilerItem(cmp, pc_info.active));
		}
	}
	mdb.end_read(id);

	return true;
}

void Pack::addElement(const ustring &tab, const ustring &name, const ustring &info) {
	TRACE_PROC
	ustring dir(pathElementsDb());

	sqlite3 *ppDb = NULL;
	sqlite3_open(dir.c_str(), &ppDb);
	sqlite3_stmt *ppStmt;
	sqlite3_prepare(ppDb, (ustring("SELECT id FROM groups WHERE name = '") + tab + "'").c_str(), -1, &ppStmt, NULL);
	int gid = 0;
	if(sqlite3_step(ppStmt) != SQLITE_DONE) {
		gid = sqlite3_column_int(ppStmt, 0);
	}
	sqlite3_finalize(ppStmt);

	sqlite3_prepare(ppDb, (ustring("SELECT pos FROM elements WHERE tab = ") + int_to_str(gid) + " ORDER BY pos DESC LIMIT 1").c_str(), -1, &ppStmt, NULL);
	int pos = 0;
	if(sqlite3_step(ppStmt) != SQLITE_DONE) {
		pos = sqlite3_column_int(ppStmt, 0) + 1;
	}
	sqlite3_finalize(ppStmt);

	ustring sql = ustring::compose("INSERT INTO elements(name, info, tab, pos) VALUES('%1', '%2', %3, %4)", name, info, gid, pos);
	//DEBUG_MSG("Try to add: " << sql.c_str())
	sqlite3_exec(ppDb, sql.c_str(), NULL, NULL, NULL);

	sqlite3_close(ppDb);
}

void Pack::addGroup(const ustring &tab, const ustring &group) {
	TRACE_PROC
	addElement(tab, ustring("*") + tab + "_" + group, group);
}

PackElement *Pack::getElementByName(const ustring &name) {
	TRACE_PROC
	if(elements.empty())
		load();
	for(std::list<PackElement*>::iterator e = elements.begin(); e != elements.end(); e++)
		if((*e)->name == name) {
			if(!(*e)->conf)
				(*e)->conf = new ElementConfig(this, pathElementConf(name));
			return *e;
		}
	std::cout << "not found in db: " << name.c_str() << std::endl;
	ustring fconf = pathElementConf(name);
	if(!file_test(fconf, FILE_TEST_EXISTS)) return NULL;


	PackElement *pe = addElement(name, info);
	pe->conf = new ElementConfig(this, fconf);
	return pe;
}

void Pack::loadIcons() {
	if(!icon) {
		icon = Gdk::Pixbuf::create_from_file(ustring::compose("%1%2%3/"PACK_ICON_FILE, dataDir.c_str(), ELEMENTS_PATH, name));
		icon_small = icon->scale_simple(16, 16, Gdk::INTERP_BILINEAR);
	}
}

TypePixbuf Pack::getIcon() {
	loadIcons();
	return icon;
}

TypePixbuf Pack::getSmallIcon() {
	loadIcons();
	return icon_small;
}

PackProject *Pack::getProjectByEntry(const ustring &name) {
	for(PackProjects::iterator p = projects.begin(); p != projects.end(); p++)
		if((*p)->entry == name)
			return *p;
	return NULL;
}

Compiler *Pack::getActiveCompiler() {
	for(CompilerItems::iterator ci = compilers.begin(); ci != compilers.end(); ci++)
		if(ci->active)
			return ci->compiler;
	return NULL;
}

//-----------------------------------------------------------------------------------------------------------------------

PackCollection::PackCollection() {
	TRACE_PROC
	load();
 }

void PackCollection::load() {
	PackInfo pack;

	void *id = mdb.begin_read_packs();
	while(mdb.read_packs(id, pack)) {
		DEBUG_MSG("Find pack: " << pack.codeName.c_str())
		Pack *pk = new Pack(pack.codeName, pack.info);
		if(pk->isExists()) {
			pk->title = pack.name;
			pk->ext = pack.units;
			pk->id = pack.id;
			push_back(pk);
		}
		else {
			DEBUG_MSG("Pack not found: " << pack.codeName.c_str())
			delete pk;
		}
	}
	mdb.end_read(id);
}

Pack *PackCollection::getPackByName(const ustring &name) {
	for(iterator p = begin(); p != end(); p++)
		if((*p)->name == name)
			return *p;
	return NULL;
}
