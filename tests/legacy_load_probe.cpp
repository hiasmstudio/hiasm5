// Load probe for HiAsm 4 example schemes: loads every .sha under one folder of the HiAsm 4 installation with the
// scheme loader, captures the loader messages and writes one line per scheme to a TSV report.
// Usage: legacy_load_probe <build dir> <folder to scan> <report.tsv>
// Not part of ctest: it reads the installed HiAsm 4 examples.
#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "SDK.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include <glib/gstdio.h>

static void collect(const std::string &folder, std::vector<std::string> &files) {
    GDir *dir = g_dir_open(folder.c_str(), 0, nullptr);
    if(!dir) return;
    std::vector<std::string> names;
    while(const gchar *name = g_dir_read_name(dir)) names.push_back(name);
    g_dir_close(dir);
    std::sort(names.begin(), names.end());
    for(const auto &name : names) {
        const std::string path = folder + "/" + name;
        if(g_file_test(path.c_str(), G_FILE_TEST_IS_DIR)) collect(path, files);
        else if(g_str_has_suffix(name.c_str(), ".sha")) files.push_back(path);
    }
}

int main(int argc, char **argv) {
    if(argc != 4) return 2;
    initDirs();
    dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open();
    SettingsManager::init();
    compilerSet = new CompilerCollection();
    packSet = new PackCollection();

    std::vector<std::string> files;
    collect(argv[2], files);
    std::ofstream report(argv[3]);
    report << "file\tpack\tloaded\telements\tmessages\n";
    size_t loaded = 0;
    for(const auto &file : files) {
        std::ostringstream captured;
        auto *oldOut = std::cout.rdbuf(captured.rdbuf());
        auto *oldErr = std::cerr.rdbuf(captured.rdbuf());
        bool ok = false;
        size_t elements = 0;
        std::string pack = "-";
        {
            MSDK sdk;
            ok = sdk.loadFromFile(file);
            elements = sdk.elements.size();
            if(sdk.pack) pack = sdk.pack->name.raw();
        }
        std::cout.rdbuf(oldOut);
        std::cerr.rdbuf(oldErr);
        std::string messages = captured.str();
        std::replace(messages.begin(), messages.end(), '\n', ' ');
        std::replace(messages.begin(), messages.end(), '\t', ' ');
        loaded += ok;
        report << file << '\t' << pack << '\t' << (ok ? 1 : 0) << '\t' << elements << '\t' << messages << '\n';
    }
    std::cout << "PROBE " << argv[2] << " files=" << files.size() << " loaded=" << loaded << std::endl;
    return 0;
}
