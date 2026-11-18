#include "Pack.h"
#include "MainDataBase.h"
#include <glib/gstdio.h>
#include <iostream>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)
int main(int argc, char **argv) {
    if(argc != 2) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.pack-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs();
    databaseFile = ustring(argv[1]) + "/int/hiasm.db";
    mdb.open();
    compilerSet = new CompilerCollection();
    dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    PackCollection shipped;
    size_t templates = 0;
    bool allValid = true;
    for(Pack *pack : shipped) {
        if(pack->isLegacy()) continue; // External installations have a separate compatibility test.
        pack->ensureLoaded();
        if(pack->name == "cpp") {
            size_t cppElements = 0;
            // The maintained C++17 group has 31 concrete elements: 24 logic and
            // GTK4 elements, the 5 extra project types (WinDll, Plugin,
            // ComExt, WinService, CplApp), and the restored If and Write.  They
            // intentionally use the short names consumed by existing .sha
            // examples (Entry, Data, Counter, ...).
            for(auto group : pack->groups)
                if(group->name == "RTCG")
                    for(auto element : group->elements)
                        if(element->name.size() && element->name.raw()[0] != '*') ++cppElements;
            CHECK(cppElements == 31);
            for(const char *name : {"Code", "Entry", "Gui", "Library"}) {
                bool registered = false;
                for(auto element : pack->elements)
                    if(element->name == name) { registered = true; break; }
                CHECK(registered);
            }
        }
        // ProjectManager loads these directly, without Pack::addElement's
        // extra fallback. A missing project icon must not throw on selection.
        for(auto project : pack->projects) {
            const auto path = pack->pathElementIcon(project->entry);
            CHECK(g_file_test(path.c_str(),G_FILE_TEST_IS_REGULAR));
            CHECK(Gdk::Pixbuf::create_from_file(path));
        }
        GDir *configs = g_dir_open((pack->path() + "conf").c_str(), 0, nullptr);
        if(!configs) continue;
        while(const gchar *file = g_dir_read_name(configs)) {
            if(!g_str_has_suffix(file, ".ini")) continue;
            const ustring name = ustring(file).substr(0, strlen(file)-4);
            bool registered = false;
            for(auto *element : pack->elements) if(element->name == name) { registered = true; break; }
            if(!registered) continue; // Obsolete unregistered templates are not active pack entries.
            ElementConfig config(pack, pack->path() + "conf/" + file);
            if(!config.valid) allValid = false;
            ++templates;
        }
        g_dir_close(configs);
    }
    CHECK(templates > 200);
    CHECK(allValid);
    gchar *folder = g_dir_make_tmp("hiasm-pack-test-XXXXXX", nullptr);
    CHECK(folder);
    const ustring dir(folder); g_free(folder);
    dataDir = dir + "/";
    CHECK(g_mkdir_with_parents((dir + "/packs/test").c_str(), 0700) == 0);
    CHECK(g_mkdir_with_parents((dir + "/packs/_base/icon").c_str(), 0700) == 0);
    gchar *icon = nullptr; gsize size = 0;
    CHECK(g_file_get_contents((ustring(argv[1]) + "/packs/_base/icon/nil.png").c_str(), &icon, &size, nullptr));
    CHECK(g_file_set_contents((dataDir + ELEMENTS_NIL_FILE).c_str(), icon, size, nullptr));
    g_free(icon);
    sqlite3 *db = nullptr;
    const ustring path = dir + "/packs/test/elements.db";
    CHECK(sqlite3_open(path.c_str(), &db) == SQLITE_OK);
    CHECK(sqlite3_exec(db,
        "CREATE TABLE groups(id INTEGER PRIMARY KEY, name TEXT, pos INTEGER);"
        "CREATE TABLE elements(id INTEGER PRIMARY KEY, name TEXT, info TEXT, tab INTEGER, pos INTEGER);"
        "INSERT INTO groups VALUES(1,'O''Brien 世界',0);", nullptr, nullptr, nullptr) == SQLITE_OK);
    sqlite3_close(db);
    Pack pack("test", "test");
    const ustring tab = u8"O'Brien 世界", name = u8"Элемент 'quote' 世界", info = u8"Описание '); DROP TABLE elements; -- 世界";
    pack.ensureLoaded();
    CHECK(pack.groups.size() == 1 && pack.groups.front()->elements.empty());
    CHECK(pack.addElement(tab, name, info));
    CHECK(!pack.addElement(tab, name, info));
    CHECK(pack.elements.size() == 1 && pack.elements.front()->name == name && pack.elements.front()->info == info);
    CHECK(!pack.addElement("missing", name, info));
    CHECK(!pack.addGroup(tab, "  "));
    CHECK(pack.addGroup(tab, u8"Группа '世界'"));
    CHECK(!pack.addGroup(tab, u8"Группа '世界'"));
    CHECK(pack.groups.front()->elements.size() == 2);
    CHECK(sqlite3_open(path.c_str(), &db) == SQLITE_OK);
    sqlite3_stmt *stmt = nullptr;
    CHECK(sqlite3_prepare_v2(db, "SELECT name,info,pos FROM elements ORDER BY pos", -1, &stmt, nullptr) == SQLITE_OK);
    CHECK(sqlite3_step(stmt) == SQLITE_ROW);
    CHECK(name == reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    CHECK(info == reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1)));
    CHECK(sqlite3_column_int(stmt, 2) == 0);
    CHECK(sqlite3_step(stmt) == SQLITE_ROW);
    CHECK(ustring(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0))) == "*" + tab + u8"_Группа '世界'");
    CHECK(sqlite3_column_int(stmt, 2) == 1);
    CHECK(sqlite3_step(stmt) == SQLITE_DONE);
    sqlite3_finalize(stmt); sqlite3_close(db);
    ustring error;
    CHECK(!pack.createElement("../escape", tab, info, error) && !error.empty());
    CHECK(!pack.createElement("9invalid", tab, info, error));
    CHECK(!pack.createElement("UserElement", "missing", info, error));
    CHECK(!pack.createElement("UserElement", tab, "line1\nClass=Bad", error));
    CHECK(pack.createElement("UserElement", tab, info, error) && error.empty());
    auto created = pack.getElementByName("UserElement");
    CHECK(created && created->conf->name == "UserElement" && created->conf->info == info);
    CHECK(created->conf->mtd.size() == 2 && created->conf->cClass == "Element");
    CHECK(!pack.createElement("UserElement", tab, info, error));
    const auto fixture = [&](const char *name, const std::string &body) {
        const auto path = pack.path() + "conf/" + name + ".ini";
        return g_file_set_contents(path.c_str(), body.data(), body.size(), nullptr);
    };
    const std::string longInfo(8192, 'x');
    CHECK(fixture("Long", "\xef\xbb\xbf[Type]\r\nClass=Element\r\nInfo=" + longInfo + u8"世界\r\n\r\n[Property]\r\nText=Left||Right|2|value\r\n\r\n[Methods]\r\nonEvent=Event|2|0"));
    auto longConfig = pack.getElementByName("Long");
    CHECK(longConfig && longConfig->conf->valid && longConfig->conf->info == longInfo + u8"世界");
    CHECK(longConfig->conf->findPropByName("Text")->info == "Left|Right");
    CHECK(fixture("NamedType", "[Methods]\nText=description|4|str\nEvent=description|2|"));
    CHECK(pack.getElementByName("NamedType")->conf->mtd.front()->dataType == data_str);
    CHECK(fixture("Malformed", "[Property]\nText=missing delimiters"));
    CHECK(!pack.getElementByName("Malformed"));
    CHECK(fixture("InvalidType", "[Property]\nText=description|999|value"));
    CHECK(!pack.getElementByName("InvalidType"));
    CHECK(fixture("InvalidPoint", "[Methods]\nonEvent=description|2|bad"));
    CHECK(!pack.getElementByName("InvalidPoint"));
    CHECK(fixture("InvalidUtf8", "[Type]\nInfo=\xff"));
    CHECK(!pack.getElementByName("InvalidUtf8"));
    CHECK(fixture("Self", "[Type]\nInherit=Self"));
    CHECK(!pack.getElementByName("Self"));
    CHECK(fixture("CycleA", "[Type]\nInherit=CycleB"));
    CHECK(fixture("CycleB", "[Type]\nInherit=CycleA"));
    CHECK(!pack.getElementByName("CycleA"));
    CHECK(fixture("MissingBase", "[Type]\nInherit=DoesNotExist"));
    CHECK(!pack.getElementByName("MissingBase"));
    CHECK(fixture("Grouped", "[Property]\n##First group\nOne=description|1|1"));
    CHECK(fixture("Ungrouped", "[Property]\nTwo=description|1|2"));
    CHECK(pack.getElementByName("Grouped")->conf->prop.front()->group == "First group");
    CHECK(pack.getElementByName("Ungrouped")->conf->prop.front()->group.empty());
    for(const char *name : {"Long", "NamedType", "Malformed", "InvalidType", "InvalidPoint", "InvalidUtf8", "Self", "CycleA", "CycleB", "MissingBase", "Grouped", "Ungrouped"})
        g_unlink(pack.pathElementConf(name).c_str());
    CHECK(g_file_set_contents(pack.pathElementCode("Occupied").c_str(), "preserve", -1, nullptr));
    CHECK(!pack.createElement("Occupied", tab, info, error));
    CHECK(!g_file_test((pack.path() + "conf/Occupied.ini").c_str(), G_FILE_TEST_EXISTS));
    gchar *existing = nullptr;
    CHECK(g_file_get_contents(pack.pathElementCode("Occupied").c_str(), &existing, nullptr, nullptr));
    CHECK(std::string(existing) == "preserve"); g_free(existing);
    CHECK(sqlite3_open(path.c_str(), &db) == SQLITE_OK);
    CHECK(sqlite3_exec(db, "CREATE TRIGGER reject_new BEFORE INSERT ON elements BEGIN SELECT RAISE(ABORT,'read-only fixture'); END;", nullptr, nullptr, nullptr) == SQLITE_OK);
    sqlite3_close(db);
    CHECK(!pack.createElement("Rollback", tab, info, error));
    CHECK(!g_file_test((pack.path() + "conf/Rollback.ini").c_str(), G_FILE_TEST_EXISTS));
    CHECK(!g_file_test(pack.pathElementCode("Rollback").c_str(), G_FILE_TEST_EXISTS));
    for(const char *file : {"conf/UserElement.ini", "code/hiUserElement.hws", "code/hiOccupied.hws"})
        g_unlink((pack.path() + file).c_str());
    g_rmdir((pack.path() + "conf").c_str()); g_rmdir(pack.pathCode().c_str());
    g_unlink(path.c_str()); g_unlink((dataDir + ELEMENTS_NIL_FILE).c_str());
    g_rmdir((dir + "/packs/_base/icon").c_str()); g_rmdir((dir + "/packs/_base").c_str());
    g_rmdir((dir + "/packs/test").c_str()); g_rmdir((dir + "/packs").c_str()); g_rmdir(dir.c_str());
    std::cout << "OK pack database: Unicode/quotes/injection, missing tabs, group duplicates, cached palette" << std::endl;
}
