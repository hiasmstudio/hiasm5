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
#include "CodeGen.h"
#include "LegacyPackCatalog.h"
#include <algorithm>

PackCollection *packSet;

ustring PackProject::getTemplateFile() {
	return pack->path() + ELEMENTS_NEW_PATH + entry + ".sha";
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

static ustring modulePath(Pack *pack, ustring lib) {
#ifdef G_OS_WIN32
	if(lib.size() > 3 && lib.substr(lib.size() - 3) == ".so")
		lib.replace(lib.size() - 3, 3, ".dll");
#else
	if(lib.size() > 4 && lib.substr(lib.size() - 4) == ".dll")
		lib.replace(lib.size() - 4, 4, ".so");
#endif
	return pack->pathMake() + lib;
}

bool PackProject::make(const ustring &project, void *result) {
	if(pack->isLegacy()) {
		cgt_on_debug.run("!HiAsm 4 compilation requires a separate 32-bit Delphi CodeGen bridge; this package is available for inspection only");
		return false;
	}
	// make_mod.dll only writes the generated project text to a file. Doing it
	// here keeps the Modules package usable in 64-bit and cross-platform builds
	// instead of loading the original 32-bit Windows DLL.
	gchar *base_name = g_path_get_basename(lib.c_str());
	ustring base = base_name ? ustring(base_name).lowercase() : ustring();
	g_free(base_name);
	if(base == "make_mod.dll" || base == "make_mod.so" || base == "make_mod") {
		if(!result) {
			cgt_on_debug.run("!Project maker returned no data");
			return false;
		}
		const char *text = static_cast<const char *>(result);
		const ustring target = pack->pathCode() + project + "." + ext;
		GError *error = nullptr;
		if(!g_file_set_contents(target.c_str(), text, strlen(text), &error)) {
			cgt_on_debug.run((ustring("!Unable to write project file: ") + (error ? error->message : "unknown error")).c_str());
			if(error) g_error_free(error);
			return false;
		}
		return true;
	}

	ustring mdir = modulePath(pack, lib);

	GModule *handle  = g_module_open(mdir.c_str(), G_MODULE_BIND_LAZY);
	if(!handle) {
		cgt_on_debug.run((ustring("!Unable to load project builder: ") + g_module_error()).c_str());
		return false;
	}
	TbuildMakePrj buildMakePrj;
	bool success = false;
	if(g_module_symbol(handle, "buildMakePrj", (gpointer *)&buildMakePrj)) {
		ustring prj_name = project + "." + ext;
		TBuildMakePrjRec prj = {};
		prj.compiler = "none";
		prj.prjFilename = prj_name.c_str();
		prj.result = result;
		gchar *previous = g_get_current_dir();
		if(g_chdir(pack->pathCode().c_str()) != 0) {
			g_free(previous);
			g_module_close(handle);
			cgt_on_debug.run("!Cannot enter package code directory");
			return false;
		}
		int x = (*buildMakePrj)(&prj);
		if(g_chdir(previous) != 0) cgt_on_debug.run("!Cannot restore working directory");
		g_free(previous);
		success = x == 0;
		if(!success) cgt_on_debug.run("!Project maker failed");
	}
	else {
		cgt_on_debug.run("!Project maker entry point not found");
	}
	g_module_close(handle);
	return success;
}

bool PackProject::run(const ustring &project) {
	if(pack->isLegacy()) {
		cgt_on_debug.run("!HiAsm 4 execution requires the optional legacy bridge (and Wine on Linux)");
		return false;
	}
	ustring mdir = modulePath(pack, lib);

	GModule *handle  = g_module_open(mdir.c_str(), G_MODULE_BIND_LAZY);
	if(!handle) {
		cgt_on_debug.run((ustring("!Unable to load project runner: ") + g_module_error()).c_str());
		return false;
	}
	TbuildRunProc buildRunProc;
	bool success = false;
	if(g_module_symbol(handle, "buildRunProc", (gpointer *)&buildRunProc)) {
		TBuildRunRec prj = {};
		prj.ClientPort = 0;
		prj.ServerPort = 0;
		prj.Mode = 0;
		prj.FileName = project.c_str();
		int x = (*buildRunProc)(&prj);
		success = x == 0;
		if(!success) cgt_on_debug.run("!Project launch failed");
	}
	else {
		cgt_on_debug.run("!Project runner entry point not found");
	}
	g_module_close(handle);
	return success;
}

//------------------------------------------------------------------------------------

Pack::Pack(const ustring &name, const ustring &info, const ustring &directory, const ustring &installation)
	: externalDirectory(directory), legacyInstallation(installation) {
	TRACE_PROC
	this->name = name;
	this->info = info;
	title = name;
	this->id = 0;
	if(isLegacy()) {
		ext = "pas";
		title = "HiAsm 4: " + name;
		GDir *templates = g_dir_open((path() + ELEMENTS_NEW_PATH).c_str(), 0, nullptr);
		if(templates) {
			std::vector<std::string> names;
			while(const gchar *file = g_dir_read_name(templates))
				if(g_str_has_suffix(file, ".sha")) names.emplace_back(file, strlen(file)-4);
			g_dir_close(templates); std::sort(names.begin(), names.end());
			for(const auto &entry : names) {
				auto *project = new PackProject(this);
				project->entry = project->name = entry;
				project->info = "HiAsm 4 template; inspection only, legacy compiler bridge is not installed";
				projects.push_back(project);
			}
		}
		return;
	}

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

namespace {
// A broken or missing icon file must not stop a package from loading: the caller gets a null image.
TypePixbuf loadIconFile(const ustring &file) {
	try {
		return Gdk::Pixbuf::create_from_file(file);
	} catch(const Glib::Error &error) {
		std::cout << "Error loading icon file: " << file << ": " << error.what() << std::endl;
		return TypePixbuf();
	}
}
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

	// The element draws its image unconditionally, so a broken file falls back to the placeholder and then to an
	// empty transparent image.
	pe->image = loadIconFile(icon_name);
	if(!pe->image)
		pe->image = loadIconFile(dataDir + ELEMENTS_NIL_FILE);
	if(!pe->image)
		pe->image = Gdk::Pixbuf::create(static_cast<Gdk::Colorspace>(GDK_COLORSPACE_RGB), true, 8, 16, 16);
#endif

	elements.push_back(pe);

	return pe;
}

ustring Pack::path() {
	if(isLegacy()) return externalDirectory + "/";
	const ustring directory = name == "_base" &&
		!file_test(dataDir + ELEMENTS_PATH"_base/elements.db", FILE_TEST_IS_REGULAR) ? "cpp" : name;
	return ustring::compose("%1%2%3/", dataDir.c_str(), ELEMENTS_PATH, directory.c_str());
}

ustring Pack::pathElementsDb() {
	if(isLegacy() && file_test(path() + "Elements.db", FILE_TEST_EXISTS)) return path() + "Elements.db";
	return path() + "elements.db";
}

ustring Pack::pathElementConf(const ustring &name) {
	ustring conf = path() + ELEMENTS_CONF_PATH + name + ".ini";
	if(file_test(conf, FILE_TEST_EXISTS))
		return conf;
	if(isLegacy()) {
		conf = legacyInstallation + "/Elements/_base/conf/" + name + ".ini";
		if(file_test(conf, FILE_TEST_EXISTS)) return conf;
	}
	return ustring::compose("%1%2%3/%4%5.ini", dataDir.c_str(), ELEMENTS_PATH,
		file_test(dataDir + ELEMENTS_PATH"_base/elements.db", FILE_TEST_IS_REGULAR) ? "_base" : "cpp", ELEMENTS_CONF_PATH, name);
}

ustring Pack::pathElementCode(const ustring &name) {
	// Templates are named like their configuration: conf/MainForm.ini -> code/MainForm.hws.
	// Packs written earlier keep the hi prefix (code/hiListBox.hws), which is still found.
	if(isLegacy()) return pathCode() + "hi" + name + ".pas";
	const ustring plain = pathCode() + name + ".hws";
	if(file_test(plain, FILE_TEST_EXISTS)) return plain;
	const ustring prefixed = pathCode() + "hi" + name + ".hws";
	if(file_test(prefixed, FILE_TEST_EXISTS)) return prefixed;
	return plain;
}

bool Pack::compiles(const ustring &name) {
	// An element is offered only when its configuration loads: an element of the database without a configuration
	// (CNet: FileSecurity, web: LineBreak) or with an inherited class that is missing cannot be created, and the
	// palette must not offer what would be dropped on the scheme.
	if(this->name != "cpp") return getElementByName(name) != nullptr;
	if(file_test(pathElementCode(name), FILE_TEST_EXISTS)) return true;
	PackElement *element = getElementByName(name);
	if(!element || !element->conf) return false;
	const ustring &cls = element->conf->cClass;
	// HubEx/GetDataEx/ITElement route links; a MultiElement container (ChildPanel, ChildForm) has no code of its
	// own either: RTCG creates its scheme and passes links through its frame (rtcgRealLink, src/RTCG/IntFunc.cpp).
	return cls == "HubEx" || cls == "GetDataEx" || cls == "ITElement" || cls == "MultiElement";
}

ustring Pack::pathCode() {
	return path() + ELEMENTS_CODE_PATH;
}

ustring Pack::pathMake() {
	return path() + ELEMENTS_MAKE_PATH;
}

ustring _iconPath(const ustring &pack, const ustring &name) {
	ustring f = ustring::compose("%1%2%3/%4%5.ico", dataDir.c_str(), ELEMENTS_PATH, pack, ELEMENTS_ICON_PATH, name);
	if(file_test(f, FILE_TEST_EXISTS))
		return f;
	return ustring::compose("%1%2%3/%4%5.png", dataDir.c_str(), ELEMENTS_PATH, pack, ELEMENTS_ICON_PATH, name);
}

ustring Pack::pathElementIcon(const ustring &name) {
	ustring f;
	if(isLegacy()) {
		for(const auto &base : {path(), legacyInstallation + "/Elements/_base/"})
			for(const char *suffix : {".ico", ".png"}) {
				f = base + ELEMENTS_ICON_PATH + name + suffix;
				if(file_test(f, FILE_TEST_EXISTS)) return f;
			}
		return dataDir + ELEMENTS_NIL_FILE;
	}
	if(file_test(f = _iconPath(this->name, name), FILE_TEST_EXISTS))
		return f;

	f = _iconPath(file_test(dataDir + ELEMENTS_PATH"_base/elements.db", FILE_TEST_IS_REGULAR) ? "_base" : "cpp", name);
    return file_test(f, FILE_TEST_IS_REGULAR) ? f : dataDir + ELEMENTS_NIL_FILE;
}

bool Pack::load() {
	TRACE_PROC
	ustring dir(pathElementsDb());

	sqlite3 *ppDb = NULL;
	if(sqlite3_open_v2(dir.c_str(), &ppDb, SQLITE_OPEN_READONLY, NULL) != SQLITE_OK) {
		std::cerr << "Unable to open pack database \"" << dir.c_str() << "\": "
			<< (ppDb ? sqlite3_errmsg(ppDb) : "SQLite open failed") << std::endl;
		if(ppDb)
			sqlite3_close(ppDb);
		return false;
	}

	auto fail = [&](const std::string &what) {
		std::cerr << "Unable to load pack \"" << name.c_str() << "\" from \""
			<< dir.c_str() << "\": " << what << std::endl;
		if(ppDb)
			sqlite3_close(ppDb);
		ppDb = NULL;
		for(std::list<PackGroup*>::iterator p = groups.begin(); p != groups.end(); ++p)
			delete *p;
		groups.clear();
		for(std::list<PackElement*>::iterator p = elements.begin(); p != elements.end(); ++p) {
			delete (*p)->conf;
			delete *p;
		}
		elements.clear();
		return false;
	};

	// A missing or truncated database must fail before any rows are consumed.
	// `planned_elements` is optional metadata for the curated C++ palette and
	// must not make the two required catalogs invalid.
	sqlite3_stmt *schema = NULL;
	if(sqlite3_prepare_v2(ppDb,
		"SELECT name FROM sqlite_master WHERE type='table' AND name IN ('groups','elements')",
		-1, &schema, NULL) != SQLITE_OK) {
		return fail(sqlite3_errmsg(ppDb));
	}
	bool has_groups = false, has_elements = false;
	int schema_rc = SQLITE_ROW;
	while((schema_rc = sqlite3_step(schema)) == SQLITE_ROW) {
		const unsigned char *table = sqlite3_column_text(schema, 0);
		if(table) {
			const std::string name((const char*)table);
			if(name == "groups") has_groups = true;
			else if(name == "elements") has_elements = true;
		}
	}
	if(schema_rc != SQLITE_DONE) {
		std::string error = sqlite3_errmsg(ppDb);
		sqlite3_finalize(schema);
		return fail(error);
	}
	sqlite3_finalize(schema);
	if(!has_groups || !has_elements)
		return fail("required tables groups/elements are missing");

	sqlite3_stmt *ppStmt = NULL;
	if(sqlite3_prepare_v2(ppDb, "SELECT id, name FROM groups ORDER BY pos", -1, &ppStmt, NULL) != SQLITE_OK) {
		return fail(sqlite3_errmsg(ppDb));
	}
	int row_rc = SQLITE_ROW;
	std::vector<std::pair<PackGroup*, int>> groupIds;
	while((row_rc = sqlite3_step(ppStmt)) == SQLITE_ROW) {
		// A NULL name is the same hidden bucket as an empty one (HiAsm 4 Python keeps its bucket as NULL).
		const unsigned char *name = sqlite3_column_text(ppStmt, 1);
		PackGroup *pk = new PackGroup();
		if(!name)
			pk->name = std::string();
		else
			pk->name = isLegacy() ? LegacyPackCatalog::decodeText((const char*)name, sqlite3_column_bytes(ppStmt, 1)) : std::string((const char*)name);
		groupIds.emplace_back(pk, sqlite3_column_int(ppStmt, 0));
		// Empty group names are present in the shipped databases as hidden
		// buckets. Palette::loadPack() intentionally skips them, so only a
		// NULL name is malformed here.
		groups.push_back(pk);
	}
	if(row_rc != SQLITE_DONE) {
		std::string error = sqlite3_errmsg(ppDb);
		sqlite3_finalize(ppStmt);
		return fail(error);
	}
	sqlite3_finalize(ppStmt);

	for(const auto &group : groupIds) {
		PackGroup *current = group.first;
		if(sqlite3_prepare_v2(ppDb, "SELECT name, info FROM elements WHERE tab = ?1 ORDER BY pos", -1, &ppStmt, NULL) != SQLITE_OK) {
			return fail(ustring("group \"") + current->name + "\": " + sqlite3_errmsg(ppDb));
		}
		if(sqlite3_bind_int(ppStmt, 1, group.second) != SQLITE_OK) {
			std::string error = sqlite3_errmsg(ppDb);
			sqlite3_finalize(ppStmt);
			return fail(ustring("group \"") + current->name + "\": " + error);
		}
		row_rc = SQLITE_ROW;
		while((row_rc = sqlite3_step(ppStmt)) == SQLITE_ROW) {
			const unsigned char *elementName = sqlite3_column_text(ppStmt, 0);
			if(!elementName)
				{ sqlite3_finalize(ppStmt); return fail(ustring("group \"") + current->name + "\": elements.name contains NULL"); }
			const auto decode = [this, ppStmt](int column) {
				const char *text = (const char*)sqlite3_column_text(ppStmt, column);
				return !text ? std::string() : isLegacy() ? LegacyPackCatalog::decodeText(text, sqlite3_column_bytes(ppStmt, column)) : std::string(text);
			};
			current->elements.push_back(addElement(decode(0), decode(1)));
		}
		if(row_rc != SQLITE_DONE) {
			std::string error = sqlite3_errmsg(ppDb);
			sqlite3_finalize(ppStmt);
			return fail(ustring("group \"") + current->name + "\": " + error);
		}
		sqlite3_finalize(ppStmt);
	}
	// Planned elements keep old schemes loadable without offering unfinished
	// palette entries. They are registered by name but deliberately absent from
	// every group.
	sqlite3_stmt *planned = NULL;
	if(sqlite3_prepare_v2(ppDb, "SELECT name, info FROM planned_elements", -1, &planned, NULL) == SQLITE_OK) {
		const auto plannedDecode = [this, planned](int column) {
			const char *text = (const char*)sqlite3_column_text(planned, column);
			return !text ? std::string() : isLegacy()
				? LegacyPackCatalog::decodeText(text, sqlite3_column_bytes(planned, column))
				: std::string(text);
		};
		int planned_rc = SQLITE_ROW;
		while((planned_rc = sqlite3_step(planned)) == SQLITE_ROW) {
			const unsigned char *elementName = sqlite3_column_text(planned, 0);
			if(!elementName) {
				std::string error = "planned_elements.name contains NULL";
				sqlite3_finalize(planned);
				return fail(error);
			}
			addElement(plannedDecode(0), plannedDecode(1));
		}
		if(planned_rc != SQLITE_DONE) {
			std::string error = sqlite3_errmsg(ppDb);
			sqlite3_finalize(planned);
			return fail(error);
		}
		sqlite3_finalize(planned);
	}
	sqlite3_close(ppDb);

	if(isLegacy()) return true;
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

bool Pack::addElement(const ustring &tab, const ustring &name, const ustring &info) {
	TRACE_PROC
	if(isLegacy()) return false;
	ensureLoaded();
	for(PackElement *element : elements) if(element->name == name) return false;
	ustring dir(pathElementsDb());

	sqlite3 *ppDb = NULL;
	if(sqlite3_open(dir.c_str(), &ppDb) != SQLITE_OK || !ppDb) {
		if(ppDb) sqlite3_close(ppDb);
		return false;
	}
	sqlite3_stmt *ppStmt = NULL;
	if(sqlite3_prepare_v2(ppDb, "SELECT id FROM groups WHERE name = ?1", -1, &ppStmt, NULL) != SQLITE_OK ||
		sqlite3_bind_text(ppStmt, 1, tab.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
		if(ppStmt) sqlite3_finalize(ppStmt);
		sqlite3_close(ppDb);
		return false;
	}
	int gid = 0;
	if(sqlite3_step(ppStmt) == SQLITE_ROW) {
		gid = sqlite3_column_int(ppStmt, 0);
	}
	sqlite3_finalize(ppStmt);
	if(!gid) {
		sqlite3_close(ppDb);
		return false;
	}

	ppStmt = NULL;
	if(sqlite3_prepare_v2(ppDb, "SELECT pos FROM elements WHERE tab = ?1 ORDER BY pos DESC LIMIT 1", -1, &ppStmt, NULL) != SQLITE_OK ||
		sqlite3_bind_int(ppStmt, 1, gid) != SQLITE_OK) {
		if(ppStmt) sqlite3_finalize(ppStmt);
		sqlite3_close(ppDb);
		return false;
	}
	int pos = 0;
	if(sqlite3_step(ppStmt) == SQLITE_ROW) {
		pos = sqlite3_column_int(ppStmt, 0) + 1;
	}
	sqlite3_finalize(ppStmt);

	if(sqlite3_prepare_v2(ppDb,
		"INSERT INTO elements(name, info, tab, pos) VALUES(?1, ?2, ?3, ?4)",
		-1, &ppStmt, NULL) != SQLITE_OK ||
		sqlite3_bind_text(ppStmt, 1, name.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
		sqlite3_bind_text(ppStmt, 2, info.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK ||
		sqlite3_bind_int(ppStmt, 3, gid) != SQLITE_OK ||
		sqlite3_bind_int(ppStmt, 4, pos) != SQLITE_OK) {
		if(ppStmt) sqlite3_finalize(ppStmt);
		sqlite3_close(ppDb);
		return false;
	}
	const bool inserted = sqlite3_step(ppStmt) == SQLITE_DONE;
	sqlite3_finalize(ppStmt);

	sqlite3_close(ppDb);
	if(inserted) {
		// Keep a loaded palette in sync when the user switches tabs/packs.
		for(PackGroup *group : groups) {
			if(group->name == tab) {
				group->elements.push_back(addElement(name, info));
				break;
			}
		}
	}
	return inserted;
}

bool Pack::addGroup(const ustring &tab, const ustring &group) {
	TRACE_PROC
	if(group.find_first_not_of(" \t\r\n") == ustring::npos) return false;
	ensureLoaded();
	const ustring marker = ustring("*") + tab + "_" + group;
	for(PackElement *element : elements)
		if(element->name == marker) return false;
	return addElement(tab, ustring("*") + tab + "_" + group, group);
}

bool Pack::createElement(const ustring &name, const ustring &tab, const ustring &info, ustring &error) {
	error.clear();
	if(isLegacy()) { error = "External HiAsm 4 packages are read-only."; return false; }
	const std::string identifier = name.raw();
	if(identifier.empty() || !(g_ascii_isalpha(identifier[0]) || identifier[0] == '_') ||
	   identifier.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789_") != std::string::npos) {
		error = "Use a Latin element name starting with a letter or underscore."; return false;
	}
	ensureLoaded();
	bool found = false;
	for(PackGroup *group : groups) if(group->name == tab) found = true;
	if(!found) { error = "Select an existing palette tab."; return false; }
	for(PackElement *element : elements)
		if(element->name == name) { error = "An element with this name already exists."; return false; }
	if(info.find_first_of("\r\n") != ustring::npos || tab.find_first_of("\r\n") != ustring::npos) {
		error = "Description and palette tab must be single-line text."; return false;
	}
	const ustring config = "[About]\nVersion=1.0\n\n[Type]\nClass=Element\nInfo=" + info +
		"\nTab=" + tab + "\n\n[Methods]\ndoWork=Input|1|0\nonEvent=Output|2|0\n";
	const ustring source = "func doWork(_data)\n  event(\"onEvent\", _data)\nend\n";
	const ustring conf = path() + ELEMENTS_CONF_PATH + name + ".ini";
	const ustring code = pathElementCode(name);
	for(const ustring &dir : {path() + ELEMENTS_CONF_PATH, pathCode()})
		if(g_mkdir_with_parents(dir.c_str(), 0700) != 0) { error = "Cannot create the package directories."; return false; }
	bool confCreated = false, codeCreated = false;
	auto writeNew = [&](const ustring &path, const ustring &text, bool &created) {
		GError *failure = nullptr;
		GFile *file = g_file_new_for_path(path.c_str());
		GFileOutputStream *stream = g_file_create(file, G_FILE_CREATE_NONE, nullptr, &failure);
		g_object_unref(file);
		bool ok = stream != nullptr;
		if(stream) {
			created = true;
			ok = g_output_stream_write_all(G_OUTPUT_STREAM(stream), text.c_str(), text.bytes(), nullptr, nullptr, &failure);
			GError *closeError = nullptr;
			if(!g_output_stream_close(G_OUTPUT_STREAM(stream), nullptr, &closeError)) ok = false;
			if(!failure) { failure = closeError; closeError = nullptr; }
			g_clear_error(&closeError); g_object_unref(stream);
		}
		if(failure) { error = failure->message; g_error_free(failure); }
		return ok;
	};
	bool ok = writeNew(conf, config, confCreated) && writeNew(code, source, codeCreated);
	if(ok) {
		ElementConfig parsed(this, conf);
		ok = parsed.valid && parsed.cClass == "Element" && !parsed.mtd.empty();
		if(!ok) error = "Define Class=Element and at least one method in the configuration.";
	}
	if(ok && !(ok = addElement(tab, name, info))) error = "Cannot register the element in the package database.";
	if(!ok) {
		if(codeCreated) g_unlink(code.c_str());
		if(confCreated) g_unlink(conf.c_str());
	}
	return ok;
}

PackElement *Pack::getElementByName(const ustring &name) {
	TRACE_PROC
	if(loadingConfigs.count(name)) {
		std::cerr << "Cyclic element inheritance: " << name << std::endl;
		return nullptr;
	}
	auto loadConfig = [this, &name](PackElement *element) -> PackElement* {
		if(!element->conf) {
			loadingConfigs.insert(name);
			try { element->conf = new ElementConfig(this, pathElementConf(name)); }
			catch(...) { loadingConfigs.erase(name); throw; }
			loadingConfigs.erase(name);
			if(!element->conf->valid) {
				delete element->conf; element->conf = nullptr; return nullptr;
			}
		}
		return element;
	};
	if(elements.empty())
		load();
	for(std::list<PackElement*>::iterator e = elements.begin(); e != elements.end(); e++)
		if((*e)->name == name) {
			return loadConfig(*e);
		}
	std::cout << "not found in db: " << name.c_str() << std::endl;
	ustring fconf = pathElementConf(name);
	if(!file_test(fconf, FILE_TEST_EXISTS)) return NULL;


	PackElement *pe = addElement(name, info);
	return loadConfig(pe);
}

void Pack::loadIcons() {
	if(!icon) {
		try {
			ustring filename = isLegacy() ? path() + name + ".ico" : path() + PACK_ICON_FILE;
			if(!file_test(filename, FILE_TEST_EXISTS)) filename = dataDir + ELEMENTS_NIL_FILE;
			icon = Gdk::Pixbuf::create_from_file(filename);
			if(icon)
				icon_small = icon->scale_simple(16, 16, Gdk::INTERP_BILINEAR);
		} catch(const Glib::Error &error) {
			std::cout << "Error loading pack icon: " << error.what() << std::endl;
		}
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

void *Pack::setActiveCompiler(const ustring &name) {
	for(CompilerItems::iterator ci = compilers.begin(); ci != compilers.end(); ci++)
		ci->active = ci->compiler->name == name;
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
			pk->title = pack.codeName == "cpp" ? ui_tr("C++ package") : pack.name;
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
	// Keep the historical pack identity for internal Make(_base) schemes while
	// using the C++ package files after packs/_base was removed.
	if(!getPackByName("_base") && getPackByName("cpp")) {
		Pack *base = new Pack("_base", getPackByName("cpp")->info);
		base->title = "Core";
		base->ext = "hws";
		base->id = getPackByName("cpp")->id;
		push_back(base);
	}
	std::vector<std::string> diagnostics;
	const auto legacy = LegacyPackCatalog::discover(LegacyPackCatalog::configurationPath(dataDir.raw()), diagnostics);
	for(const auto &location : legacy) {
		if(getPackByName(location.name)) {
			diagnostics.push_back("Native package takes precedence over HiAsm 4 package: " + location.name);
			continue;
		}
		push_back(new Pack(location.name, "External HiAsm 4 package (read-only; compiler bridge required)",
			location.directory, location.installation));
	}
	for(const auto &message : diagnostics) std::cerr << "HiAsm 4: " << message << std::endl;
}

Pack *PackCollection::getPackByName(const ustring &name) {
	// HiAsm 4 writes the package name as it was typed ("Make(Delphi)") while its folder and code are
	// lower case ("delphi"): package names are not case sensitive.
	for(iterator p = begin(); p != end(); p++)
		if(g_ascii_strcasecmp((*p)->name.c_str(), name.c_str()) == 0)
			return *p;
	return NULL;
}

bool PackCollection::isLegacyFile(const ustring &file) {
	gchar *absolute = g_canonicalize_filename(file.c_str(), nullptr);
	std::string candidate(absolute); g_free(absolute);
	for(Pack *pack : *this) {
		if(!pack->isLegacy()) continue;
		absolute = g_canonicalize_filename(pack->path().c_str(), nullptr);
		std::string prefix(absolute); g_free(absolute);
#ifdef G_OS_WIN32
		std::replace(candidate.begin(), candidate.end(), '\\', '/');
		std::replace(prefix.begin(), prefix.end(), '\\', '/');
		std::transform(candidate.begin(), candidate.end(), candidate.begin(), [](unsigned char c) { return g_ascii_tolower(c); });
		std::transform(prefix.begin(), prefix.end(), prefix.begin(), [](unsigned char c) { return g_ascii_tolower(c); });
#endif
		if(!prefix.empty() && prefix.back() != G_DIR_SEPARATOR && prefix.back() != '/') prefix += '/';
		if(candidate.compare(0, prefix.size(), prefix) == 0) return true;
	}
	return false;
}
