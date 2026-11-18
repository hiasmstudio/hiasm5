#include "MainDataBase.h"
#include "SettingsManager.h"
#include <glib/gstdio.h>
#include <iostream>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)
int main(int argc, char **argv) {
    if(argc != 2) return 2;
    gchar *folder = g_dir_make_tmp("hiasm-settings-XXXXXX", nullptr);
    CHECK(folder);
    const ustring dir(folder); g_free(folder);
    homeDir = dir + G_DIR_SEPARATOR_S;
    databaseFile = dir + "/hiasm.db";
    gchar *contents = nullptr; gsize size = 0;
    CHECK(g_file_get_contents((ustring(argv[1]) + "/int/hiasm.db").c_str(), &contents, &size, nullptr));
    CHECK(g_file_set_contents(databaseFile.c_str(), contents, size, nullptr));
    g_free(contents);
    mdb.open();
    const ustring expected = u8"C:/O'Brien/Проекты/世界/#raw";
    mdb.save_settings_value(IDS_PROJECTS_DIR, expected);
    SettingsManager::init();
    CHECK(SettingsManager::readStr(IDS_PROJECTS_DIR) == expected);
    SettingsManager::setFormBinding(SettingsManager::BindEdges, true);
    SettingsManager::setFormBinding(SettingsManager::BindCenter, true);
    SettingsManager::init();
    CHECK(SettingsManager::formBinding(SettingsManager::BindEdges));
    CHECK(SettingsManager::formBinding(SettingsManager::BindCenter));
    CHECK(!SettingsManager::formBinding(SettingsManager::BindPadding));
    SettingsManager::setFormBinding(SettingsManager::BindEdges, false);
    SettingsManager::init();
    CHECK(!SettingsManager::formBinding(SettingsManager::BindEdges));
    CHECK(SettingsManager::formBinding(SettingsManager::BindCenter));
    mdb.close();
    sqlite3 *db = nullptr;
    CHECK(sqlite3_open(databaseFile.c_str(), &db) == SQLITE_OK);
    CHECK(sqlite3_exec(db, "UPDATE params SET type=999 WHERE id=134; UPDATE params SET value='bad integer' WHERE id=136;", nullptr, nullptr, nullptr) == SQLITE_OK);
    sqlite3_close(db);
    mdb.open();
    SettingsManager::init();
    CHECK(SettingsManager::readStr(IDS_PROJECTS_DIR) == expected);
    CHECK(SettingsManager::readInt(IDS_NEW_PROJECT) == 0);
    CHECK(SettingsManager::readInt(static_cast<SettingsIDS>(999999)) == 0);
    CHECK(SettingsManager::readStr(static_cast<SettingsIDS>(999999)).empty());
    CHECK(SettingsManager::readReal(static_cast<SettingsIDS>(999999)) == 0);
    mdb.save_settings_value(IDS_PROJECTS_DIR, "#not-a-serialized-property");
    SettingsManager::init();
    CHECK(SettingsManager::readStr(IDS_PROJECTS_DIR) == "#not-a-serialized-property");
    mdb.close(); g_unlink(databaseFile.c_str()); g_unlink((homeDir + "appearance.ini").c_str()); g_rmdir(dir.c_str());
    std::cout << "OK settings: Unicode, apostrophes, raw strings, invalid types and missing values" << std::endl;
}
