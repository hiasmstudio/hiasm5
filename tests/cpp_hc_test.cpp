// Builds small schemes made of Core hc* elements with the RTCG generator,
// compiles them with g++ and compares the program output with the behaviour
// of the Core interpreter (src/Elements.cpp).
#include "SDK.h"
#include "CodeGen.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "ProjectBuilder.h"
#include <glib/gstdio.h>
#include <algorithm>
#include <functional>
#include <iostream>

struct DebugLog : CallBack {
    std::string log;
    void callback(void *, CallbackType, const void *data) override {
        if(data) { log += (const char*)data; log += '\n'; }
    }
};

struct Scheme {
    MSDK sdk;
    Element *entry;
    explicit Scheme(Pack *pack, const char *project = "Entry") {
        sdk.pack = pack;
        sdk.packProject = pack->getProjectByEntry(project);
        entry = sdk.add(project, 0, 0);
        if(entry) entry->flag |= ELEMENT_FLG_IS_PARENT;
    }
    int next = 1;
    Element *add(const char *name) {
        Element *e = sdk.add(name, 100 * next++, 0);
        if(!e) { std::cerr << "cannot create " << name << std::endl; failed = true; }
        return e;
    }
    bool failed = false;
    void set(Element *e, const char *prop, const ustring &text) {
        ElementProperty *p = e ? e->props.getByName(prop) : nullptr;
        if(!p || !p->initFromText(text)) { std::cerr << "cannot set " << prop << "=" << text << std::endl; failed = true; }
    }
    void link(Element *a, const char *from, Element *b, const char *to) {
        ElementPoint *pa = a ? a->points.getByName(from) : nullptr, *pb = b ? b->points.getByName(to) : nullptr;
        if(!pa || !pb) { std::cerr << "cannot link " << from << " -> " << to << std::endl; failed = true; return; }
        pa->connect(pb);
    }
    // Hidden points (`*` in the config) exist only after the user adds them, like Point(name) in a scheme file.
    void reveal(Element *e, const char *name) {
        ConfMethod *method = e ? e->tpl->conf->def.getByName(name) : nullptr;
        if(!method) { std::cerr << "no hidden point " << name << std::endl; failed = true; return; }
        if(!e->points.getByName(name)) { e->addPoint(method->name, method->info, method->mType); e->rePosPoints(); }
    }
    void start(Element *b, const char *to) { link(entry, "onStart", b, to); }
};

static ProjectBuilder *builder;
static DebugLog debug;
static ustring workDir;
static Pack *cppPack;
static const char *onlyCase = nullptr;
static int failures = 0, executed = 0;

static std::string execute(const ustring &artifact) {
    gchar *args[] = {const_cast<gchar*>(artifact.c_str()), nullptr}, *out = nullptr;
    gint status = -1;
    if(!g_spawn_sync(nullptr, args, nullptr, G_SPAWN_DEFAULT, nullptr, nullptr, &out, nullptr, &status, nullptr) ||
       !g_spawn_check_wait_status(status, nullptr)) { g_free(out); return "<run failed>"; }
    std::string text = out ? out : "";
    g_free(out);
    text.erase(std::remove(text.begin(), text.end(), '\r'), text.end());
    return text;
}

// Builds the scheme produced by `make`, runs it and compares its output.
static void check(const char *name, const std::function<void(Scheme&)> &make, const std::string &expected,
                  const char *project = "Entry") {
    if(onlyCase && strcmp(onlyCase, name)) return;
    ++executed;
    Scheme scheme(cppPack, project);
    make(scheme);
    if(scheme.failed) { ++failures; std::cerr << "FAIL " << name << ": scheme setup" << std::endl; return; }
    scheme.sdk.fileName = workDir + "/" + name + ".sha";
    debug.log.clear();
    cgt_on_debug += &debug;
    const bool built = builder->build(&scheme.sdk);
    cgt_on_debug -= &debug;
    if(!built) { ++failures; std::cerr << "FAIL " << name << ": build failed\n" << debug.log << std::endl; return; }
    const ustring artifact = builder->getOutputProjectName(&scheme.sdk);
    const std::string actual = execute(artifact);
    gchar *parent = g_path_get_dirname(artifact.c_str());
    const ustring source = ustring(parent) + "/" + name + ".cpp";
    g_free(parent);
    if(actual != expected) {
        ++failures;
        std::cerr << "FAIL " << name << "\n--- expected\n" << expected << "--- actual\n" << actual << "---" << std::endl;
        gchar *code = nullptr;
        if(onlyCase && g_file_get_contents(source.c_str(), &code, nullptr, nullptr)) {
            const char *entryPoint = strstr(code, "int main");
            std::cerr << "--- generated\n" << (entryPoint ? entryPoint : code) << std::endl;
            g_free(code);
        }
        return;
    }
    g_unlink(artifact.c_str());
    g_unlink(source.c_str());
}

static void checkTools();
static void checkLogic();
static void checkStrings();
static void checkForms();
static void checkMore();

int main(int argc, char **argv) {
    if(argc < 2) return 2;
    if(argc > 2) onlyCase = argv[2];
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.cpp-hc-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs();
    dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
#ifdef G_OS_WIN32
    const auto runtimePath = dataDir.raw() + ";" + dataDir.raw() + "toolchains/mingw64/bin;" + (g_getenv("PATH") ? g_getenv("PATH") : "");
    g_setenv("PATH", runtimePath.c_str(), true);
#endif
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    compilerSet = new CompilerCollection(); packSet = new PackCollection();
    cppPack = packSet->getPackByName("cpp");
    if(!cppPack) return 3;
    gchar *temp = g_dir_make_tmp("hiasm-hc-test-XXXXXX", nullptr);
    if(!temp) return 4;
    workDir = temp; g_free(temp);
    ProjectBuilder projectBuilder;
    builder = &projectBuilder;
    checkTools();
    checkLogic();
    checkStrings();
    checkForms();
    checkMore();
    std::cout << "cpp hc test: " << executed << " cases, " << failures << " failed" << std::endl;
    if(onlyCase && !executed) { std::cerr << "no such case: " << onlyCase << std::endl; return 5; }
    return failures ? 1 : 0;
}

// ---------------------------------------------------------------- cases

static void checkTools() {
    // DoData puts its property into the thread; Console prints the thread data.
    check("hub_console", [](Scheme &s) {
        auto hub = s.add("hcHub"), first = s.add("hcDoData"), second = s.add("hcDoData");
        auto out1 = s.add("hcConsole"), out2 = s.add("hcConsole");
        s.set(first, "Data", "String(Hello)");
        s.set(second, "Data", "String(World)");
        s.set(out1, "Count", "1");
        s.set(out2, "Count", "1");
        s.start(hub, "doEvent1");
        s.link(hub, "onEvent1", first, "doData");
        s.link(hub, "onEvent2", second, "doData");
        s.link(first, "onEventData", out1, "doPrint");
        s.link(second, "onEventData", out2, "doPrint");
    }, "Hello\nWorld\n");
    check("strcat", [](Scheme &s) {
        auto cat = s.add("hcStrCat"), out = s.add("hcConsole");
        s.set(cat, "Str1", "Left ");
        s.set(cat, "Str2", "Right");
        s.set(out, "Count", "1");
        s.start(cat, "doStrCat");
        s.link(cat, "onStrCat", out, "doPrint");
        s.link(cat, "Result", out, "Text1");
    }, "Left Right\n");
}

#include "cpp_hc_logic.inc"
#include "cpp_hc_strings.inc"
#include "cpp_hc_forms.inc"
#include "cpp_hc_tools.inc"
