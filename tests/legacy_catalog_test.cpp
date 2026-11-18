#include "LegacyPackCatalog.h"
#include <glib/gstdio.h>
#include <iostream>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)
int main() {
    gchar *tmp = g_dir_make_tmp("hiasm-legacy-catalog-XXXXXX", nullptr);
    CHECK(tmp);
    const std::string root(tmp); g_free(tmp);
    const std::string first = root + "/first", second = root + "/second";
    for(const auto &dir : {first + "/Elements/Delphi", first + "/Elements/Web", second + "/Elements/Delphi"}) {
        CHECK(g_mkdir_with_parents((dir + "/conf").c_str(), 0700) == 0);
        CHECK(g_file_set_contents((dir + "/Elements.db").c_str(), "", 0, nullptr));
    }
    CHECK(g_mkdir_with_parents((first + "/Elements/Invalid name/conf").c_str(), 0700) == 0);
    CHECK(g_file_set_contents((first + "/Elements/Invalid name/Elements.db").c_str(), "", 0, nullptr));
    const std::string config = root + "/paths.ini";
    CHECK(g_mkdir_with_parents((root + "/packs/_hiasm4").c_str(), 0700) == 0);
    CHECK(g_file_set_contents((root + "/packs/_hiasm4/paths.ini").c_str(), "", 0, nullptr));
    CHECK(LegacyPackCatalog::configurationPath(root) == root + "/packs/_hiasm4/paths.ini");
    CHECK(LegacyPackCatalog::containsPath(first, first + "/Elements/Delphi"));
    CHECK(!LegacyPackCatalog::containsPath(first, first + "-unrelated/Elements/Delphi"));
    CHECK(!LegacyPackCatalog::containsPath(first, first + "/../second"));
    g_setenv("HIASM_CATALOG_TEST_ROOT", second.c_str(), TRUE);
    const std::string body = "\xef\xbb\xbf# comment\r\n\"first\"\r\nfirst\n%HIASM_CATALOG_TEST_ROOT%\n[ignored]\n";
    CHECK(g_file_set_contents(config.c_str(), body.data(), body.size(), nullptr));
    std::vector<std::string> diagnostics;
    auto packs = LegacyPackCatalog::discover(config, diagnostics);
    CHECK(packs.size() == 2 && packs[0].name == "delphi" && packs[1].name == "web");
    CHECK(packs[0].directory.find("Elements") != std::string::npos);
    CHECK(!diagnostics.empty());
    const char cp1251[] = "\xc4\xe5\xeb\xfc\xf4\xe8";
    CHECK(LegacyPackCatalog::decodeText(cp1251, sizeof(cp1251)-1) == u8"Дельфи");
    const char utf8[] = u8"GTK4 世界";
    CHECK(LegacyPackCatalog::decodeText(utf8, sizeof(utf8)-1) == utf8);
    const gchar *prior = g_getenv("HIASM4_ROOTS");
    const std::string saved = prior ? prior : ""; const bool had = prior != nullptr;
    g_setenv("HIASM4_ROOTS", (second + ";" + first).c_str(), TRUE);
    diagnostics.clear();
    packs = LegacyPackCatalog::discover(root + "/missing.ini", diagnostics);
    CHECK(packs.size() == 2);
    gchar *normalized = g_canonicalize_filename(second.c_str(), nullptr);
    CHECK(packs.front().installation == normalized); g_free(normalized);
    if(had) g_setenv("HIASM4_ROOTS", saved.c_str(), TRUE); else g_unsetenv("HIASM4_ROOTS");
    g_unsetenv("HIASM_CATALOG_TEST_ROOT");
    for(const auto &dir : {first + "/Elements/Delphi", first + "/Elements/Web", second + "/Elements/Delphi", first + "/Elements/Invalid name"}) {
        g_unlink((dir + "/Elements.db").c_str());
        g_rmdir((dir + "/conf").c_str()); g_rmdir(dir.c_str());
    }
    for(const auto &dir : {first, second}) { g_rmdir((dir + "/Elements").c_str()); g_rmdir(dir.c_str()); }
    g_unlink(config.c_str()); g_rmdir(root.c_str());
    g_unlink((root + "/packs/_hiasm4/paths.ini").c_str());
    g_rmdir((root + "/packs/_hiasm4").c_str()); g_rmdir((root + "/packs").c_str()); g_rmdir(root.c_str());
    std::cout << "OK legacy catalog: relative paths, CP1251, env override, duplicates, invalid directories" << std::endl;
}
