#include "SDK.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "ProjectBuilder.h"
#include "RunnerPayload.h"
#include "SchemeSyntax.h"
#include <glib/gstdio.h>
#include <vector>

static bool writeFile(const ustring &path, const std::string &data) {
    return g_file_set_contents(path.c_str(), data.data(), data.size(), NULL);
}
int main(int argc, char **argv) {
    if(argc != 2) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.scheme-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs();
    dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open();
    SettingsManager::init();
    compilerSet = new CompilerCollection();
    packSet = new PackCollection();
    gchar *tmp = g_dir_make_tmp("hiasm-scheme-test-XXXXXX", NULL);
    if(!tmp) return 3;
    const ustring dir(tmp);
    g_free(tmp);
    const ustring input = dir + u8"/Схема 世界.sha";
    const std::string text = u8"Make(_base)\nAdd(hcTplForm,1,21,105)\n{\nCaption=Привет, 世界\n}";
    if(!writeFile(input, text)) return 4;
    MSDK sdk;
    if(!sdk.loadFromFile(input) || sdk.elements.size() != 1 ||
       sdk.elements.front()->props.getByName("Caption")->readStr() != u8"Привет, 世界") return 5;
    const ustring saved = dir + u8"/Сохранено.sha";
    if(!sdk.saveToFile(saved)) return 6;
    {
        MSDK restored;
        if(!restored.loadFromFile(saved) || restored.elements.size() != 1 ||
           restored.elements.front()->props.getByName("Caption")->readStr() != u8"Привет, 世界") return 7;
    }
    const ustring previous = sdk.fileName;
    bool unexpectedSave = sdk.saveToFile(dir + "/missing/subdir/file.sha");
    if(unexpectedSave || sdk.fileName.raw() != previous.raw()) {
        std::cerr << "Save failure test: returned=" << unexpectedSave << " before=" << previous
                  << " after=" << sdk.fileName << " path=" << dir << std::endl;
        return 8;
    }
    const std::vector<std::string> invalid = {
        "", "}", "BEGIN_SDK", "Point(doWork)", "AddHint(0,0,0,0,Name)",
        "link(a,1:b,[])", "Make(unknown_pack)", "Add(NoPack,1,0,0)",
        "Make(_base)\nAdd(__missing_element__,1,0,0)\n{\n}",
        "Make(_base)\nAdd(hcTplForm,1,0,0)\n{\nCaption",
        "Make(_base)\nEND_SDK", "Make(_base)\nAdd(hcTplForm,1,0,0)",
        "Make(_base)\nAdd(hcTplForm,1,0,0)\n{\nCaption=abc",
        "Make(_base)\nAdd(hcTplForm,1,0)\n{\n}",
        "Make(_base)\nAdd(hcTplForm,1,0,0)\n{\nlink(a,1:b,[]\n}",
        "Make(_base)\nAdd(hcTplForm,1,0,0)\n{\nWidth=bad\n}",
        "Make(_base)\nAdd(hcTplForm,1,0,0)\n{\nCaption=#999999999999999999:x|\n}",
        std::string("Make(_base)\n// invalid UTF-8 ") + char(0xff)
    };
    const ustring bad = dir + "/bad.sha";
    for(const auto &data : invalid) {
        if(!writeFile(bad, data)) return 9;
        MSDK broken;
        if(broken.loadFromFile(bad)) { std::cerr << "Accepted malformed scheme: " << data; return 10; }
    }
    { MSDK missing; if(missing.loadFromFile(dir + "/missing.sha")) return 11; }
    // A file cannot attach two wires to one endpoint (or use the wrong
    // direction). Reject before connecting any of the pending wires.
    const std::string connected = "Make(_base)\nAdd(hcDoData,1,0,0)\n{\nlink(onEventData,2:doData,[(50,6)])\n}\nAdd(hcDoData,2,100,0)\n{\n}\nAdd(hcDoData,3,200,0)\n{\n";
    for(const char *link : {"link(onEventData,2:doData,[])",
                           "link(doData,1:onEventData,[])",
                           "link(onEventData,2:onEventData,[])",
                           "link(onEventData,999:doData,[])",
                           "link(__missing__,2:doData,[])",
                           "link(onEventData,2:__missing__,[])"}) {
        if(!writeFile(bad, connected + link + "\n}")) return 23;
        MSDK broken;
        if(broken.loadFromFile(bad)) return 24;
        for(auto item : broken.elements)
            for(auto point : item->points)
                if(point->point || point->pos->next || point->pos->prev) return 25;
    }
    {
        MSDK wired; wired.pack = sdk.pack;
        auto first = wired.add("hcDoData", 0, 0), last = wired.add("hcDoData", 210, 0);
        auto middle = wired.add("hcDoData", 105, 0);
        if(!first || !last || !middle) return 26;
        auto from = first->points.getByName("onEventData"), to = last->points.getByName("doData");
        auto occupied = middle->points.getByName("onEventData");
        if(from->connect(nullptr) || from->connect(from) || from->connect(occupied)) return 27;
        if(!from->connect(to)) return 28;
        from->createPath();
        const auto route = from->serializePath();
        if(occupied->connect(to) || from->connect(middle->points.getByName("doData")) ||
           from->point != to || to->point != from || from->serializePath() != route) return 29;
        // Inserting an element must split a live wire without losing its
        // path. This operation intentionally reassigns both old endpoints.
        middle->insertInLine(from, from->pos);
        if(from->point != middle->points.getByName("doData") ||
           occupied->point != to || to->point != occupied) return 30;
        middle->removeFromLine(true, false);
        if(from->point != to || to->point != from) return 31;
        from->clear();
        if(from->point || to->point || from->pos->next || to->pos->prev) return 32;
        from->createPath(); // Safe for an unconnected point, too.
        if(!from->serializePath().empty()) return 33;
        auto hub1 = wired.add("HubEx", 70, 70), hub2 = wired.add("HubEx", 140, 70);
        if(!hub1 || !hub2) return 34;
        if(!from->connect(hub1->points.getByName("doWork1")) ||
           !hub1->points.getByName("onEvent")->connect(hub2->points.getByName("doWork1")) ||
           !hub2->points.getByName("onEvent")->connect(hub1->points.getByName("doWork2"))) return 35;
        if(from->point->getRealPointWithPath()) return 36;
        hub2->points.getByName("onEvent")->clear();
        hub2->points.getByName("onEvent")->connect(to);
        if(from->point->getRealPointWithPath() != to) return 37;
        auto third = wired.add("hcDoData", 280, 70), fourth = wired.add("hcDoData", 350, 70);
        if(!third || !fourth) return 38;
        third->points.getByName("onEventData")->connect(hub1->points.getByName("doWork2"));
        middle->points.getByName("onEventData")->connect(hub1->points.getByName("doWork3"));
        // A full hub, or an already connected source, must be a no-op.
        hub1->connectToPoint(fourth->points.getByName("onEventData"));
        hub2->connectToPoint(from);
        if(fourth->points.getByName("onEventData")->point ||
           from->point != hub1->points.getByName("doWork1")) return 39;
    }
    // Pasting an element with the same class as the first existing element
    // must insert a new object, not reuse and move the existing one.
    {
        MSDK pasted;
        pasted.pack = sdk.pack;
        Element *original = pasted.add("hcDoData", 0, 0);
        if(!original) return 22;
        pasted.selMan->select(original);
        const ustring fragment = "Add(hcDoData,1,50,60)\n{\n}";
        for(int i = 0; i < 20; ++i) {
            if(!pasted.selMan->pasteText(fragment) || pasted.elements.size() != 2 ||
               pasted.selMan->getFirst() == original) return 17;
            pasted.selMan->erase();
        }
        pasted.selMan->select(original);
        if(pasted.selMan->pasteText("Add(hcDoData,1,0,0)\n{")) return 18;
        if(pasted.selMan->getFirst() != original || pasted.elements.size() != 1) return 19;
        if(pasted.selMan->pasteText(fragment + "\nAdd(__missing__,2,0,0)\n{\n}")) return 20;
        if(pasted.selMan->getFirst() != original || pasted.elements.size() != 1) return 21;
        if(pasted.selMan->pasteText("Add(hcDoData,1,50,60)\n{\nlink(onEventData,999:doData,[])\n}")) return 40;
        if(pasted.selMan->getFirst() != original || pasted.elements.size() != 1 ||
           original->points.getByName("onEventData")->point) return 41;
    }
    ProjectBuilder builder;
    if(!builder.build(&sdk)) return 12;
#ifdef G_OS_WIN32
    const ustring output = dir + u8"/build/Сохранено.exe";
#else
    const ustring output = dir + u8"/build/Сохранено";
#endif
    gchar *bytes = NULL; gsize size = 0;
    if(!g_file_get_contents(output.c_str(), &bytes, &size, NULL)) return 13;
    std::string embedded, runtime;
    bool valid = RunnerPayload::extract(std::string(bytes, size), embedded, runtime);
    g_free(bytes);
    if(!valid || embedded.find(u8"Привет, 世界") == std::string::npos || runtime != dataDir.raw()) return 14;
    ustring realDataDir = dataDir;
    dataDir = dir + "/missing/";
    const ustring builtName = sdk.fileName;
    sdk.fileName = dir + "/no_runner.sha";
    if(builder.build(&sdk)) return 15;
    dataDir = realDataDir;
    sdk.fileName = builtName;
    std::cout << "OK scheme Unicode round trip, no final newline, malformed files, write errors, runner build" << std::endl;
    g_unlink(input.c_str()); g_unlink(saved.c_str()); g_unlink(bad.c_str());
    g_unlink(output.c_str()); g_rmdir((dir + "/build").c_str()); g_rmdir(dir.c_str());
    return 0;
}
