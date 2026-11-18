// Palette audit of the "C++ application" package: every element the palette
// offers has a real icon, connection points and RTCG code, so a project never
// compiles to an empty program because an element was silently skipped.
// Elements without C++ code are not offered (Pack::compiles); the Delphi elements
// that are not implemented yet live in the `planned_elements` table, and that
// table is checked here too.
#include "Pack.h"
#include "SDK.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "Localization.h"
#include <cctype>
#include <iostream>
#include <map>
#include <set>

static int failures = 0;
static void fail(const std::string &text) { std::cerr << text << std::endl; ++failures; }

int main(int argc, char **argv) {
	if(argc != 2) return 2;
#ifdef HIASM_GTK4
	auto app = Gtk::Application::create("com.hiasm.cpp-package-test", Gio::Application::Flags::NON_UNIQUE);
	app->register_application();
#else
	Gtk::Main kit(argc, argv);
#endif
	initDirs(); dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
	databaseFile = dataDir + INT_PATH + DATABASE_FILE; mdb.open();
	compilerSet = new CompilerCollection();
	PackCollection collection; packSet = &collection;
	Pack *pack = collection.getPackByName("cpp");
	if(!pack) { std::cerr << "C++ package not found" << std::endl; return 1; }
	pack->ensureLoaded();
	// Package-local .lang overlays replace English descriptions without
	// changing the element identifiers or their executable configuration.
	ui_set_language("ru");
	if(Pack *core = collection.getPackByName("_base")) {
		core->ensureLoaded();
		PackElement *timer = core->getElementByName("hcTimer");
		if(!timer || !timer->conf || timer->conf->info.find("Генерирует") == ustring::npos)
			fail("Core .lang overlay did not translate hcTimer");
	}
	// The C++ package has no copy of hcTimer: its config and translation come from Core,
	// as in HiAsm 4 (packs/cpp/conf and packs/cpp/lang hold only the package's own files).
	if(PackElement *linked = pack->getElementByName("hcTimer")) {
		if(!linked->conf || linked->conf->info.find("Генерирует") == ustring::npos)
			fail("C++ package does not take hcTimer from Core with its translation");
	} else fail("C++ package lost hcTimer");
	// Core owns configs, icons and translations that the C++ package shares. A C++ file
	// repeating a Core file byte for byte would drift from it, so such copies are rejected.
	if(Pack *core = collection.getPackByName("_base")) {
		for(const char *folder : {"conf", "icon", "lang/ru/conf"}) {
			GDir *dir = g_dir_open((pack->path() + folder).c_str(), 0, nullptr);
			if(!dir) continue;
			while(const gchar *file = g_dir_read_name(dir)) {
				gchar *own = nullptr, *shared = nullptr; gsize ownSize = 0, sharedSize = 0;
				if(g_file_get_contents((pack->path() + folder + "/" + file).c_str(), &own, &ownSize, nullptr) &&
				   g_file_get_contents((core->path() + folder + "/" + file).c_str(), &shared, &sharedSize, nullptr) &&
				   ownSize == sharedSize && memcmp(own, shared, ownSize) == 0)
					fail(std::string("C++ copy of a Core file: ") + folder + "/" + file);
				g_free(own); g_free(shared);
			}
			g_dir_close(dir);
		}
	}
	// Containers keep their points on the inner editor element; helpers have none by design.
	const std::set<std::string> pointless = {"HubEx", "GetDataEx", "InfoTip"};
	const ustring nil = dataDir + ELEMENTS_NIL_FILE;
	size_t shown = 0;
	std::set<std::string> notCompiled, offered, hiddenGroup;
	for(PackGroup *group : pack->groups)
		for(PackElement *listed : group->elements) {
			const std::string name = listed->name.raw();
			if(name[0] == '*') continue;	// section header
			if(group->name.empty()) { hiddenGroup.insert(name); continue; }	// bucket that the palette never shows
			if(!pack->compiles(listed->name)) { notCompiled.insert(name); continue; }
			++shown;
			offered.insert(name);
			std::string problems;
			PackElement *element = pack->getElementByName(listed->name);
			if(!element || !element->conf || !element->conf->valid) problems += " invalid-config";
			if(pack->pathElementIcon(listed->name) == nil) problems += " placeholder-icon";
			// One scheme per element: a second interface widget in one scheme shows a modal error.
			MSDK scheme; scheme.pack = pack;
			Element *instance = element ? scheme.add(listed->name, 0, 0) : nullptr;
			if(!instance) problems += " cannot-create";
			else if(!pointless.count(name)) {
				size_t points = instance->points.size();
				if(instance->sdk)	// containers: points of the inner editor element
					for(Element *inner : instance->sdk->elements) points += inner->points.size();
				if(!points) problems += " no-points";
			}
			if(!problems.empty()) fail(group->name.raw() + "/" + name + ":" + problems);
		}
	if(!notCompiled.empty()) {
		std::cout << "In the database but without C++ code (not offered):";
		for(const auto &item : notCompiled) std::cout << " " << item;
		std::cout << std::endl;
	}

	// The hidden bucket keeps what old schemes need: the templates of the Core form, the project form and the
	// hcTpl* controls that the Delphi-named controls of the Widgets group replace. All of them still load by name.
	const std::map<std::string, std::string> replaced = {
		{"hcTplButton", "Button"}, {"hcTplLabel", "Label"}, {"hcTplEdit", "Edit"}, {"hcTplMemo", "Memo"},
		{"hcTplCheckBox", "CheckBox"}, {"hcTplListBox", "ListBox"}, {"hcTplComboBox", "ComboBox"}, {"hcTplProgressBar", "ProgressBar"}};
	for(const auto &pair : replaced) {
		if(offered.count(pair.first)) fail("duplicate still offered: " + pair.first);
		if(!hiddenGroup.count(pair.first)) fail("duplicate missing from the hidden bucket: " + pair.first);
		if(!offered.count(pair.second)) fail("replacement not offered: " + pair.second);
		if(!pack->compiles(pair.first) || !pack->getElementByName(pair.first)) fail("old schemes with " + pair.first + " no longer load");
	}
	// Panel is the inner element of ChildPanel, as in HiAsm 4: hidden, but it must load by name.
	for(const char *required : {"MainForm", "hcTplForm", "hcTplDialog", "EditMulti", "PointElement", "Panel"})
		if(!hiddenGroup.count(required) || !pack->getElementByName(required)) fail(std::string("missing from the hidden bucket: ") + required);
	if(!pack->getProjectByEntry("MainForm")) fail("project MainForm is not registered");

	// What the palette must offer.
	for(const char *required : {"Label", "Edit", "CheckBox", "RadioButton", "Button", "ComboBox", "ListBox", "Memo", "TrackBar",
	                            "ProgressBar", "ChildPanel", "ChildForm", "GroupBox", "Timer", "Message", "HubEx", "GetDataEx", "InfoTip",
	                            "Entry", "Gui", "Library", "Daemon", "Data", "Hub", "Math",
	                            "hcHub", "hcMath", "hcStrCat", "hcTimer", "hcTplScrollBar", "hcTrace"})
		if(!offered.count(required)) fail(std::string("missing from the C++ palette: ") + required);

	// Group layout: RTCG (native C++17 elements) and Core exist, WinCE is gone, no FTCG leftovers.
	std::set<std::string> groupNames;
	for(PackGroup *group : pack->groups) groupNames.insert(group->name.raw());
	for(const char *required : {"Widgets", "Controls", "Helpers", "RTCG", "Core"})
		if(!groupNames.count(required)) fail(std::string("group missing: ") + required);
	if(groupNames.count("WinCE")) fail("WinCE group is still in the database");
	for(PackGroup *group : pack->groups)
		for(PackElement *listed : group->elements)
			if(listed->name.raw().find("FTCG") != std::string::npos) fail("FTCG name left: " + listed->name.raw());

	// Core is a separate, recovered HiAsm 4 package, not a C++ palette alias.
	if(Pack *core = collection.getPackByName("_base")) {
		core->ensureLoaded();
		std::set<std::string> coreGroups;
		for(PackGroup *group : core->groups) coreGroups.insert(group->name.raw());
		if(coreGroups.count("RTCG") || coreGroups.count("Core") || !coreGroups.count("Tools") || !coreGroups.count("Widgets")) fail("Core package catalog was replaced by the C++ palette");
		if(!core->getElementByName("hcTplButton")) fail("Core lost hcTplButton");
		if(!core->getElementByName("hcHub") || !core->getElementByName("hcStrCat")) fail("Core logic elements are missing");
	}

	// planned_elements: names, descriptions and icon file names of what is not implemented yet.
	sqlite3 *db = nullptr;
	size_t planned = 0, plannedWithIcon = 0;
	if(sqlite3_open_v2(pack->pathElementsDb().c_str(), &db, SQLITE_OPEN_READONLY, nullptr) != SQLITE_OK) fail("cannot open elements.db");
	else {
		sqlite3_stmt *st = nullptr;
		if(sqlite3_prepare_v2(db, "SELECT name, info, group_name, icon FROM planned_elements", -1, &st, nullptr) != SQLITE_OK)
			fail(std::string("planned_elements: ") + sqlite3_errmsg(db));
		else {
			while(sqlite3_step(st) == SQLITE_ROW) {
				++planned;
				const std::string name = (const char*)sqlite3_column_text(st, 0);
				const std::string group = sqlite3_column_text(st, 2) ? (const char*)sqlite3_column_text(st, 2) : "";
				const std::string icon = sqlite3_column_text(st, 3) ? (const char*)sqlite3_column_text(st, 3) : "";
				if(name.empty() || name[0] == '*') fail("planned_elements has a section row: " + name);
				if(group == "WinCE") fail("WinCE element planned: " + name);
				if(offered.count(name) || hiddenGroup.count(name) || notCompiled.count(name)) fail("both implemented and planned: " + name);
				if(!icon.empty()) {
					++plannedWithIcon;
					if(!file_test(pack->path() + "icon" PATH_SLASH + icon, FILE_TEST_EXISTS) &&
                           !file_test(dataDir + ELEMENTS_PATH "_base" PATH_SLASH "icon" PATH_SLASH + icon, FILE_TEST_EXISTS))
                            fail("planned icon file missing: " + icon);
				}
			}
			sqlite3_finalize(st);
		}
		sqlite3_close(db);
	}
	if(planned < 100) fail("planned_elements looks empty: " + std::to_string(planned));
	GDir *icons = g_dir_open((pack->path() + "icon").c_str(), 0, nullptr);
	while(icons) {
		const gchar *file = g_dir_read_name(icons);
		if(!file) break;
		const std::string icon = file;
		if(icon.size() > 2 && icon.compare(0, 2, "Ce") == 0 && std::isupper((unsigned char)icon[2])) fail("WinCE icon left: " + icon);
	}
	if(icons) g_dir_close(icons);

	std::cout << "C++ palette audit: " << shown - failures << "/" << shown << " shown elements complete; " << planned
	          << " planned (" << plannedWithIcon << " with icons)" << std::endl;
	return failures ? 1 : 0;
}
