#include "SDK.h"
#include "CodeGen.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "ProjectBuilder.h"
#include <glib/gstdio.h>

struct BuildRecord { void *cgt; MSDK *sdk; void *result; };
typedef int (*Generate)(BuildRecord *);
struct DebugLog : CallBack {
    std::string log;
    void callback(void *, CallbackType, const void *data) override {
        if(data) { log += (const char*)data; log += '\n'; }
    }
};
static bool runGtkProbe(const ustring &directory, const std::string &probe, std::initializer_list<std::string> expected) {
    const auto source=directory+"/probe.cpp";
#ifdef G_OS_WIN32
    const auto executable=directory+"/probe.exe";
#else
    const auto executable=directory+"/probe";
#endif
    if(!g_file_set_contents(source.c_str(),probe.c_str(),probe.size(),nullptr)) return false;
    ustring compiler="g++",pkg="pkg-config",extra;
#ifdef G_OS_WIN32
    if(const gchar *root=g_getenv("HIASM_CPP_TOOLCHAIN")) {
        compiler=ustring(root)+"/bin/g++.exe"; pkg=ustring(root)+"/bin/pkg-config.exe";
        gchar *quoted=g_shell_quote((ustring("--sysroot=")+root).c_str()); extra=quoted; g_free(quoted);
    } else if(g_file_test((dataDir+"toolchains/mingw64/bin/g++.exe").c_str(),G_FILE_TEST_IS_REGULAR)) {
        const auto root=dataDir+"toolchains/mingw64";
        compiler=root+"/bin/g++.exe"; pkg=root+"/bin/pkg-config.exe";
        gchar *quoted=g_shell_quote(("--sysroot="+root).c_str()); extra=quoted; g_free(quoted);
    }
#endif
    gchar *pkgArgs[]={const_cast<gchar*>(pkg.c_str()),const_cast<gchar*>("--cflags"),const_cast<gchar*>("--libs"),const_cast<gchar*>("gtk4"),nullptr};
    gchar *flags=nullptr; gint status=-1;
    if(!g_spawn_sync(nullptr,pkgArgs,nullptr,G_SPAWN_SEARCH_PATH,nullptr,nullptr,&flags,nullptr,&status,nullptr)||!g_spawn_check_wait_status(status,nullptr)) return false;
    gchar *quoted=g_shell_quote(compiler.c_str());
    // Windows GTK4 forms call GDI and USER (regions, icon, taskbar), as ProjectBuilder's systemLibraries adds.
#ifdef G_OS_WIN32
    const ustring windowsLibraries=" -lgdi32 -luser32";
#else
    const ustring windowsLibraries;
#endif
    const ustring command=ustring(quoted)+" -std=c++17 -g "+extra+" probe.cpp -o probe "+(flags?flags:"")+windowsLibraries;
    g_free(quoted); g_free(flags);
    gchar **args=nullptr;
    if(!g_shell_parse_argv(command.c_str(),nullptr,&args,nullptr)) return false;
    bool ok=g_spawn_sync(directory.c_str(),args,nullptr,G_SPAWN_SEARCH_PATH,nullptr,nullptr,nullptr,nullptr,&status,nullptr)&&g_spawn_check_wait_status(status,nullptr);
    g_strfreev(args);
    if(ok) {
        gchar *launch[]={const_cast<gchar*>(executable.c_str()),nullptr}; gchar *out=nullptr; GError *error=nullptr;
        gchar **environment = nullptr;
#ifdef G_OS_WIN32
        environment = g_get_environ();
        const auto path = dataDir.raw()+";"+(g_getenv("PATH")?g_getenv("PATH"):"");
        environment = g_environ_setenv(environment,"PATH",path.c_str(),true);
#endif
        ok=g_spawn_sync(directory.c_str(),launch,environment,G_SPAWN_DEFAULT,nullptr,nullptr,&out,nullptr,&status,&error)&&g_spawn_check_wait_status(status,&error);
        g_strfreev(environment);
        const std::string output=out?out:""; g_free(out);
        if(!ok) std::cerr << "GTK4 probe failed: " << (error?error->message:"") << "; output: " << output << std::endl;
        g_clear_error(&error);
        for(const auto &value : expected) ok=ok && output.find(value)!=std::string::npos;
        if(!ok) std::cerr << "GTK4 probe output: " << output << std::endl;
    }
    if(ok) {
        g_unlink(source.c_str()); g_unlink(executable.c_str()); g_unlink((executable+".exe").c_str());
    } else std::cerr << "Retained failed probe for debugging: " << executable << std::endl;
    return ok;
}
static bool probeCorePlugin(const ustring &directory, const std::string &name) {
    // The plugs demos are MainForm projects: every control is a named widget of the form
    // (Edit<id>, Button<id>, Memo<id>, ...), so the probe addresses them by name.
    const auto probe = std::string(R"CPP(
#define main hiasm_generated_main
#include ")CPP") + name + R"CPP(.cpp"
#undef main
int main() {
  auto app = gtk_application_new("com.hiasm.core-plugin-probe", G_APPLICATION_NON_UNIQUE);
  if(!g_application_register(G_APPLICATION(app), nullptr, nullptr)) return 1;
  activate(app, nullptr);
  auto windows = gtk_application_get_windows(app);
  if(!windows || g_list_length(windows) != 1) return 2;
  auto window = GTK_WINDOW(windows->data);
  if(!GTK_IS_FIXED(gtk_window_get_child(window))) return 3;
)CPP";
    std::string checks;
    if(name == "chat") checks = R"CPP(
  auto entry = hiasm_widget("Edit2").get(), send = hiasm_widget("Button3").get();
  if(!GTK_IS_EDITABLE(entry) || !GTK_IS_BUTTON(send)) return 5;
  for(const char* message : {"#REG:Alice", "#JOIN:room", "#TEXT:hello", "#BAD:value"}) {
    gtk_editable_set_text(GTK_EDITABLE(entry), message);
    g_signal_emit_by_name(send, "clicked");
  }
  if(hiasm_view_text(hiasm_text_view("Memo5")) != "Registered: Alice\nJoined: room\nMessage: hello\nUnknown command: #BAD") return 6;
)CPP";
    else if(name == "panex") checks = R"CPP(
  auto entry = hiasm_widget("Edit2").get(), check = hiasm_widget("Button3").get(), clear = hiasm_widget("Button6").get();
  if(!GTK_IS_EDITABLE(entry) || !GTK_IS_BUTTON(check) || !GTK_IS_BUTTON(clear)) return 5;
  // panex.cpp is the generated source in the working directory: a regular file.
  gtk_editable_set_text(GTK_EDITABLE(entry), "missing-test-file.wav");
  g_signal_emit_by_name(check, "clicked");
  gtk_editable_set_text(GTK_EDITABLE(entry), "");
  g_signal_emit_by_name(check, "clicked");
  gtk_editable_set_text(GTK_EDITABLE(entry), "panex.cpp");
  g_signal_emit_by_name(check, "clicked");
  const std::string log = hiasm_view_text(hiasm_text_view("Memo5"));
  if(log.find("Не найден: missing-test-file.wav\nПуть не задан\nНайден: panex.cpp, ") != 0) return 6;
  g_signal_emit_by_name(clear, "clicked");   // Hub: Memo.doClear and Edit.doText
  if(!hiasm_view_text(hiasm_text_view("Memo5")).empty()) return 7;
  if(gtk_editable_get_text(GTK_EDITABLE(entry))[0]) return 8;
)CPP";
    else if(name == "std") checks = R"CPP(
  auto up = hiasm_widget("Button3").get(), down = hiasm_widget("Button4").get(), reset = hiasm_widget("Button5").get();
  auto show = hiasm_widget("CheckBox10").get(), memo = hiasm_widget("Memo7").get();
  if(!GTK_IS_BUTTON(up) || !GTK_IS_BUTTON(down) || !GTK_IS_BUTTON(reset) || !GTK_IS_CHECK_BUTTON(show)) return 5;
  if(!gtk_widget_get_visible(memo)) return 6;
  g_signal_emit_by_name(up, "clicked");
  g_signal_emit_by_name(up, "clicked");
  g_signal_emit_by_name(down, "clicked");
  if(hiasm_view_text(hiasm_text_view("Memo7")) != "Счётчик: 1\nСчётчик: 2\nСчётчик: 1") return 7;
  g_signal_emit_by_name(reset, "clicked");   // back to Default without a journal line
  g_signal_emit_by_name(up, "clicked");
  if(hiasm_view_text(hiasm_text_view("Memo7")) != "Счётчик: 1\nСчётчик: 2\nСчётчик: 1\nСчётчик: 1") return 8;
  gtk_check_button_set_active(GTK_CHECK_BUTTON(show), FALSE);
  if(gtk_widget_get_visible(memo)) return 9;
  gtk_check_button_set_active(GTK_CHECK_BUTTON(show), TRUE);
  if(!gtk_widget_get_visible(memo)) return 10;
)CPP";
    else return false;
    return runGtkProbe(directory, probe + checks + R"CPP(
  gtk_window_destroy(window); g_object_unref(app);
  std::cout << "Core GTK4 plugin OK" << std::endl;
  return 0;
}
)CPP", {"Core GTK4 plugin OK"});
}
extern int _Debug(const char *, int);
extern int GetParam(short, void *);

int main(int argc, char **argv) {
    if(argc != 3) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.rtcg-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs();
    dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
#ifdef G_OS_WIN32
    // Direct fixture launches need the same runtime search path as the
    // editor's child launcher, including when PATH contains only Windows.
    const auto runtimePath=dataDir.raw()+";"+dataDir.raw()+"toolchains/mingw64/bin;"+(g_getenv("PATH")?g_getenv("PATH"):"");
    g_setenv("PATH",runtimePath.c_str(),true);
#endif
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    compilerSet = new CompilerCollection(); packSet = new PackCollection();
    GModule *module = g_module_open(argv[2], G_MODULE_BIND_LAZY);
    Generate generate = NULL;
    if(!module || !g_module_symbol(module, "buildProcessProc", (gpointer*)&generate)) return 3;
    // No parent, no script and no result must never report success.
    MSDK sdk;
    sdk.pack = packSet->getPackByName("cpp");
    // The legacy C++ package is optional; Core-only installations still build
    // the editor but cannot run the C++ codegen fixtures.
    if(!sdk.pack) { std::cout << "SKIP: cpp package is not registered" << std::endl; return 77; }
    // The empty project template generates main(); the hello example also prints.
    for(const auto &fixture : {std::make_pair("new/Entry.sha", 1u),
                              std::make_pair("example/hello.sha", 3u)}) {
        MSDK example;
        if(!example.loadFromFile(sdk.pack->path() + fixture.first) || !example.packProject ||
           example.pack != sdk.pack || example.elements.size() != fixture.second) return 20;
        BuildRecord sample = {GetCGTools, &example, nullptr};
        if(generate(&sample) || !sample.result) return 21;
        const std::string source((char*)sample.result);
        if(source.find("int main(int argc, char **argv)") == std::string::npos ||
            (fixture.second == 3 &&
             (source.find("std::puts") == std::string::npos ||
              source.find("Hello from C++17") == std::string::npos))) return 21;
    }
    sdk.fileName = "regression.sha";
    BuildRecord rec = {GetCGTools, &sdk, NULL};
    if(generate(&rec) == 0 || rec.result) return 4;
    Element *entry = sdk.add("Entry", 0, 0);
    Element *message = sdk.add("DoData", 100, 0);
    Element *print = sdk.add("hcConsole", 200, 0);
    if(!entry || !message || !print) return 5;
    TData greeting(ustring("Hello from HiAsm"));
    message->props.getByName("Data")->setValue(&greeting);
    int lines = 1;
    print->props.getByName("Count")->setValue(&lines);
    {
        struct { char text[CGT_PARAM_TEXT_CAPACITY]; char guard[16]; } buffer;
        memset(&buffer, 0x5a, sizeof(buffer));
        const ustring previousName = sdk.fileName;
        sdk.fileName = std::string(1024, 'x');
        memcpy(buffer.text, &entry, sizeof(entry));
        if(!GetParam(PARAM_PROJECT_NAME, buffer.text) || buffer.text[0]) return 17;
        for(char guard : buffer.guard) if(guard != 0x5a) return 18;
        if(!GetParam(PARAM_PROJECT_NAME, nullptr)) return 19;
        sdk.fileName = previousName;
    }
    entry->flag |= ELEMENT_FLG_IS_PARENT;
    entry->points.getByName("onStart")->connect(message->points.getByName("doData"));
    message->points.getByName("onEventData")->connect(print->points.getByName("doPrint"));
    for(int i = 0; i < 30; ++i) {
        rec.result = NULL;
        if(generate(&rec) != 0 || !rec.result) return 6;
        const std::string source((char*)rec.result);
        if(source.find("std::puts(hiasm_text(\"Hello from HiAsm\").c_str())") == std::string::npos ||
            source.find("int main(int argc, char **argv)") == std::string::npos) {
            std::cerr << source << std::endl; return 7;
        }
        if(entry->userData || message->userData || print->userData) return 8;
    }
    // Version templates can omit tokens and repeat them.
    char version[256] = "literal";
    if(GetParam(PARAM_HIASM_VERSION, version) || strcmp(version, "literal")) return 9;
    strcpy(version, "%mj.%mn.%bl/%mj");
    if(GetParam(PARAM_HIASM_VERSION, version) || strchr(version, '%')) return 10;
    strcpy(version, "uninitialized");
    GetParam(PARAM_USER_MAIL, version);
    if(*version) return 11;
    const std::string diagnostic(4096, 'x');
    _Debug(diagnostic.c_str(), 0xff0000);
    gchar *temp = g_dir_make_tmp("hiasm-cpp-test-XXXXXX", NULL);
    if(!temp) return 13;
    const ustring dir(temp); g_free(temp);
    ProjectBuilder builder;
    DebugLog debug;
    {
        MSDK welcome;
        const auto intro = dataDir + "packs/_base/example/tutorial/intro.sha";
        if(!welcome.loadFromFile(intro) || !welcome.isCoreDocument() ||
           welcome.elements.empty()) return 93;
        welcome.fileName = dir + "/intro.sha";
        debug.log.clear(); cgt_on_debug += &debug;
        const bool built = builder.build(&welcome);
        cgt_on_debug -= &debug;
        if(built || debug.log.find("Core documents") == std::string::npos) return 94;
    }
    {
        struct Lesson {
            const char *folder; const char *name; const char *expected; bool library; bool gui;
            Lesson(const char *folder, const char *name, const char *expected, bool library, bool gui = false)
                : folder(folder), name(name), expected(expected), library(library), gui(gui) {}
        };
        const Lesson lessons[] = {
            {"tutorial", "CableHelp", "cable: red=42, green=20\n", false},
            {"tutorial", "Check", "sound branch enabled\nalways connected\n", false},
            {"tutorial", "DLL", "library entry\n", true}, {"tutorial", "Element", "intro.sha\nsecond.sha\n", false},
            {"tutorial", "EventFromData", "before request: 0\n10\n20\nrequests: 2\n", false},
            {"tutorial", "GlobalVar", "shared state\n", false}, {"tutorial", "Hub", "first\nsecond\nthird\n", false},
            {"tutorial", "Index", "Intro, Hub, Check, EventFromData, GlobalVar, CableHelp, Element, MultiElementEx, MultiThread, Stack, Types, Informer, DLL\n", false},
            {"tutorial", "Informer", "information\n", false},
            {"tutorial", "MultiElementEx", "independent: 7,3\ncount after delete: 1, remaining: 3\ncount after clear: 0\n", false},
            {"tutorial", "MultiThread", "first\n42\nlast\n", false},
            {"tutorial", "Stack", "20\n10\nempty: 1\n", false},
            {"tutorial", "Types", "Person: Alice, 21, Moscow\nwrong type rejected: 1\nfields: 2\n", false},
            {"plugs", "chat", "", false, true}, {"plugs", "panex", "", false, true}, {"plugs", "std", "", false, true}
        };
        for(const auto &lesson : lessons) {
            MSDK sample;
            const ustring source = sdk.pack->path() + "../_base/example/" + lesson.folder + "/" + lesson.name + ".sha";
            if(!sample.loadFromFile(source) || sample.pack != sdk.pack || sample.packProject == nullptr ||
               sample.packProject->entry != (lesson.library ? "Library" : lesson.gui ? "MainForm" : "Entry")) {
                std::cerr << "Cannot load migrated lesson: " << source << std::endl;
                return 99;
            }
            sample.fileName = dir + "/" + lesson.name + ".sha";
            debug.log.clear(); cgt_on_debug += &debug;
            const bool built = builder.build(&sample);
            cgt_on_debug -= &debug;
            if(!built) { std::cerr << "Migrated lesson failed: " << lesson.name << "\n" << debug.log << std::endl; return 100; }
            const auto artifact = builder.getOutputProjectName(&sample);
            if(lesson.gui) {
                gchar *parent = g_path_get_dirname(artifact.c_str());
                const bool probed = probeCorePlugin(parent, lesson.name);
                g_free(parent);
                if(!probed) return 101;
            } else if(lesson.library) {
                GModule *library = g_module_open(artifact.c_str(), G_MODULE_BIND_LAZY);
                int (*run)() = nullptr;
                const bool loaded = library && g_module_symbol(library, "hiasm_run", (gpointer*)&run) && run && run() == 0;
                if(library) g_module_close(library);
                if(!loaded) return 101;
            } else {
                gchar *args[] = {const_cast<gchar*>(artifact.c_str()), nullptr}, *out = nullptr;
                gint status = -1;
                if(!g_spawn_sync(nullptr, args, nullptr, G_SPAWN_DEFAULT, nullptr, nullptr, &out, nullptr, &status, nullptr) ||
                   !g_spawn_check_wait_status(status, nullptr)) return 102;
                std::string output = out ? out : "";
                g_free(out);
                output.erase(std::remove(output.begin(), output.end(), '\r'), output.end());
                if(output != lesson.expected) {
                    std::cerr << "Unexpected migrated lesson output for " << lesson.name << ": " << output << std::endl;
                    return 103;
                }
            }
            g_unlink(artifact.c_str());
            gchar *parent = g_path_get_dirname(artifact.c_str());
            g_unlink((ustring(parent) + "/" + lesson.name + ".cpp").c_str());
            g_free(parent);
        }
    }
    {
        MSDK flow;
        if(!flow.loadFromFile(sdk.pack->path()+"example/control_flow.sha") || flow.elements.size()!=12) return 40;
        flow.fileName = dir+"/control_flow.sha";
        cgt_on_debug += &debug;
        if(!builder.build(&flow)) { std::cerr << debug.log << std::endl; return 41; }
        cgt_on_debug -= &debug;
        const auto artifact = builder.getOutputProjectName(&flow);
        gchar *args[] = {const_cast<gchar*>(artifact.c_str()), nullptr};
        gchar *out = nullptr; gint status = -1;
        if(!g_spawn_sync(nullptr,args,nullptr,G_SPAWN_DEFAULT,nullptr,nullptr,&out,nullptr,&status,nullptr) || !g_spawn_check_wait_status(status,nullptr)) return 42;
        const std::string output = out ? out : ""; g_free(out);
        if(output != u8"Hello 世界\n42\n0\n1\n2\nGTK4\n" && output != u8"Hello 世界\r\n42\r\n0\r\n1\r\n2\r\nGTK4\r\n") {
            std::cerr << "Unexpected control flow output: " << output << std::endl; return 43;
        }
        g_unlink(artifact.c_str());
        gchar *parent = g_path_get_dirname(artifact.c_str());
        g_unlink((ustring(parent)+"/control_flow.cpp").c_str()); g_free(parent);
    }
    {
        MSDK state;
        if(!state.loadFromFile(sdk.pack->path()+"example/core_state.sha") || state.elements.size()!=26) return 78;
        // The stateful elements and dynamic connections survive scheme IO.
        const auto saved=dir+u8"/Память и данные.sha";
        if(!state.saveToFile(saved)) return 79;
        MSDK reloaded;
        if(!reloaded.loadFromFile(saved) || reloaded.elements.size()!=26 ||
           reloaded.getElementById(11)->points.getByName("Op1")->point != reloaded.getElementById(9)->points.getByName("Count")) return 80;
        state.fileName=dir+"/core_state.sha";
        debug.log.clear(); cgt_on_debug+=&debug;
        const bool built=builder.build(&state);
        cgt_on_debug-=&debug;
        if(!built) { std::cerr<<debug.log<<std::endl; return 81; }
        const auto artifact=builder.getOutputProjectName(&state);
        gchar *args[]={const_cast<gchar*>(artifact.c_str()),nullptr};
        gchar *out=nullptr; gint status=-1;
        if(!g_spawn_sync(nullptr,args,nullptr,G_SPAWN_DEFAULT,nullptr,nullptr,&out,nullptr,&status,nullptr)||!g_spawn_check_wait_status(status,nullptr)) return 82;
        std::string output=out?out:""; g_free(out);
        output.erase(std::remove(output.begin(),output.end(),'\r'),output.end());
        if(output!=u8"Stored 世界\nStored 世界\nDefault\n1\n2\n3\n0\n1\n3\ndivision by zero\n3\n3\nDefault checked\nDefault checked\n") {
            std::cerr<<"Unexpected stateful output: "<<output<<std::endl; return 83;
        }
        g_unlink(artifact.c_str());g_unlink(saved.c_str());
        gchar *parent=g_path_get_dirname(artifact.c_str());
        g_unlink((ustring(parent)+"/core_state.cpp").c_str());g_free(parent);
    }
    {
        MSDK math;
        if(!math.loadFromFile(sdk.pack->path()+"example/math_functions.sha")) return 90;
        math.fileName=dir+"/math_functions.sha";
        debug.log.clear(); cgt_on_debug+=&debug;
        const bool built=builder.build(&math); cgt_on_debug-=&debug;
        if(!built) { std::cerr<<debug.log<<std::endl; return 91; }
        const auto artifact=builder.getOutputProjectName(&math);
        gchar *args[]={const_cast<gchar*>(artifact.c_str()),nullptr},*out=nullptr; gint status=-1;
        if(!g_spawn_sync(nullptr,args,nullptr,G_SPAWN_DEFAULT,nullptr,nullptr,&out,nullptr,&status,nullptr)||!g_spawn_check_wait_status(status,nullptr)) return 92;
        std::string actual=out?out:"";g_free(out);
        actual.erase(std::remove(actual.begin(),actual.end(),'\r'),actual.end());
        if(actual!="1\n3\ndomain error\n7\n") { std::cerr<<"Math functions: "<<actual<<std::endl; return 93; }
        g_unlink(artifact.c_str());gchar *parent=g_path_get_dirname(artifact.c_str());
        g_unlink((ustring(parent)+"/math_functions.cpp").c_str());g_free(parent);
    }
    {
        MSDK literal;
        literal.pack=sdk.pack; literal.packProject=sdk.pack->getProjectByEntry("Entry");
        literal.fileName=dir+"/literal.sha";
        auto entry=literal.add("Entry",0,0), hub=literal.add("Hub",100,0);
        auto data=literal.add("Data",200,0), memory=literal.add("Memory",300,0);
        auto output=literal.add("hcConsole",400,0), print=literal.add("Print",500,0);
        if(!entry || !hub || !data || !memory || !output || !print) return 86;
        entry->flag |= ELEMENT_FLG_IS_PARENT;
        const ustring text=u8"Quoted \"世界\" C:\\new\\test\tend\nnext\r\nlast";
        TData value(text);
        hub->props.getByName("OutCount")->initFromText("3");
        data->props.getByName("Data")->setValue(&value);
        memory->props.getByName("Default")->setValue(&value);
        print->props.getByName("Text")->setValue(const_cast<ustring*>(&text));
        int count = 1;
        output->props.getByName("Count")->setValue(&count);
        entry->points.getByName("onStart")->connect(hub->points.getByName("doEvent1"));
        hub->points.getByName("onEvent1")->connect(data->points.getByName("doData"));
        hub->points.getByName("onEvent2")->connect(memory->points.getByName("doClear"));
        hub->points.getByName("onEvent3")->connect(print->points.getByName("doPrint"));
        data->points.getByName("onEventData")->connect(memory->points.getByName("doValue"));
        memory->points.getByName("onData")->connect(output->points.getByName("doPrint"));
        debug.log.clear(); cgt_on_debug+=&debug;
        const bool built=builder.build(&literal); cgt_on_debug-=&debug;
        if(!built) { std::cerr<<debug.log<<std::endl; return 87; }
        const auto artifact=builder.getOutputProjectName(&literal);
        gchar *args[]={const_cast<gchar*>(artifact.c_str()),nullptr},*out=nullptr; gint status=-1;
        if(!g_spawn_sync(nullptr,args,nullptr,G_SPAWN_DEFAULT,nullptr,nullptr,&out,nullptr,&status,nullptr)||!g_spawn_check_wait_status(status,nullptr)) return 88;
        std::string actual=out?out:"";g_free(out);
        std::string expected=text.raw()+"\n"+text.raw()+"\n"+text.raw()+"\n";
        actual.erase(std::remove(actual.begin(),actual.end(),'\r'),actual.end());
        expected.erase(std::remove(expected.begin(),expected.end(),'\r'),expected.end());
        if(actual!=expected) { std::cerr<<"String literal escaping changed data: "<<actual<<std::endl; return 89; }
        g_unlink(artifact.c_str());gchar *parent=g_path_get_dirname(artifact.c_str());
        g_unlink((ustring(parent)+"/literal.cpp").c_str());g_free(parent);
    }
    sdk.fileName = dir + u8"/Пример с пробелами.sha";
    sdk.packProject = sdk.pack->getProjectByEntry("Entry");
    cgt_on_debug += &debug;
    gchar *before = g_get_current_dir();
    if(!builder.build(&sdk)) { std::cerr << debug.log << std::endl; return 14; }
    cgt_on_debug -= &debug;
    gchar *after = g_get_current_dir();
    if(strcmp(before, after)) return 15;
    g_free(before); g_free(after);
    const ustring executable = builder.getOutputProjectName(&sdk);
    if(executable.find("-debug/") == ustring::npos && executable.find("-release/") == ustring::npos &&
       executable.find("-debug\\") == ustring::npos && executable.find("-release\\") == ustring::npos) return 22;
    gchar *args[] = {const_cast<gchar*>(executable.c_str()), NULL};
    gchar *output = NULL;
    gint status = 0;
    if(!g_spawn_sync(NULL, args, NULL, G_SPAWN_DEFAULT, NULL, NULL, &output, NULL, &status, NULL) ||
       !g_spawn_check_wait_status(status, NULL) || !output || !strstr(output, "Hello from HiAsm")) return 16;
    g_free(output);
    g_unlink(executable.c_str());
    gchar *buildDirRaw = g_path_get_dirname(executable.c_str());
    const ustring buildDir(buildDirRaw); g_free(buildDirRaw);
    g_unlink((buildDir + u8"/Пример с пробелами.cpp").c_str());
    for(const char *target : {"Library", "Gui", "Daemon", "MainForm"}) {
        MSDK project;
        if(!project.loadFromFile(sdk.pack->path() + "new/" + target + ".sha") ||
           !project.packProject || project.packProject->entry != target) return 23;
        project.fileName = dir + "/" + target + ".sha";
        cgt_on_debug += &debug;
        if(!builder.build(&project)) { std::cerr << debug.log << std::endl; return 24; }
        cgt_on_debug -= &debug;
        const ustring artifact = builder.getOutputProjectName(&project);
        if(!g_file_test(artifact.c_str(), G_FILE_TEST_IS_REGULAR)) return 25;
        if(project.packProject->entry == "Library") {
            GModule *library = g_module_open(artifact.c_str(), G_MODULE_BIND_LAZY);
            int (*entry)() = nullptr;
            if(!library || !g_module_symbol(library, "hiasm_run", (gpointer*)&entry) || entry() != 0) return 26;
            builder.run(&project);
            if(builder.isRunning()) return 27;
            g_module_close(library);
        } else if(project.packProject->entry == "Gui" || project.packProject->entry == "MainForm") {
#ifdef G_OS_WIN32
            const std::string oldPath = g_getenv("PATH") ? g_getenv("PATH") : "";
            g_setenv("PATH", (dataDir.raw() + ";" + oldPath).c_str(), true);
            gchar *bytes = nullptr; gsize length = 0;
            if(!g_file_get_contents(artifact.c_str(), &bytes, &length, nullptr)) return 28;
            const unsigned char *pe = reinterpret_cast<unsigned char*>(bytes);
            if(length < 64) return 29;
            const guint32 offset = pe[60] | (guint32(pe[61]) << 8) | (guint32(pe[62]) << 16) | (guint32(pe[63]) << 24);
            const bool gui = offset + 94 < length && pe[offset+92] == 2 && pe[offset+93] == 0;
            g_free(bytes);
            if(!gui) return 30;
#endif
            builder.run(&project);
            const gint64 deadline = g_get_monotonic_time() + G_TIME_SPAN_SECOND;
            while(g_get_monotonic_time() < deadline) {
                while(g_main_context_iteration(NULL, false)) {}
                g_usleep(1000);
            }
            if(!builder.isRunning()) return 31;
            builder.stop();
            const gint64 stopped = g_get_monotonic_time() + 3*G_TIME_SPAN_SECOND;
            while(builder.isRunning() && g_get_monotonic_time() < stopped) {
                while(g_main_context_iteration(NULL, false)) {}
                g_usleep(1000);
            }
            if(builder.isRunning()) return 32;
#ifdef G_OS_WIN32
            g_setenv("PATH", oldPath.c_str(), true);
#endif
        } else {
            gchar *backgroundArgs[] = {const_cast<gchar*>(artifact.c_str()), NULL};
            if(!g_spawn_sync(NULL, backgroundArgs, NULL, G_SPAWN_DEFAULT, NULL, NULL, NULL, NULL, &status, NULL) ||
               !g_spawn_check_wait_status(status, NULL)) return 33;
        }
        g_unlink(artifact.c_str());
        g_unlink((buildDir + "/" + target + ".cpp").c_str());
    }
    g_rmdir(buildDir.c_str());
    int release = 1;
    entry->props.getByName("BuildMode")->setValue(&release);
    cgt_on_debug += &debug;
    if(!builder.build(&sdk)) { std::cerr << debug.log << std::endl; return 34; }
    cgt_on_debug -= &debug;
    const ustring optimized = builder.getOutputProjectName(&sdk);
    if(optimized == executable || optimized.find("release") == ustring::npos ||
       debug.log.find("-O2 -DNDEBUG") == std::string::npos) return 35;
    gchar *releaseDir = g_path_get_dirname(optimized.c_str());
    g_unlink(optimized.c_str());
    g_unlink((ustring(releaseDir) + u8"/Пример с пробелами.cpp").c_str());
    g_rmdir(releaseDir); g_free(releaseDir);
    if(g_getenv("HIASM_TEST_CROSS")) {
#ifdef G_OS_WIN32
        int target=2;
#else
        int target=1;
#endif
        entry->props.getByName("TargetPlatform")->setValue(&target);
        debug.log.clear(); cgt_on_debug += &debug;
        if(!builder.build(&sdk)) { std::cerr << debug.log << std::endl; return 47; }
        cgt_on_debug -= &debug;
        const auto artifact=builder.getOutputProjectName(&sdk);
        gchar *bytes=nullptr; gsize length=0;
        if(!g_file_get_contents(artifact.c_str(),&bytes,&length,nullptr)) return 48;
#ifdef G_OS_WIN32
        const bool signature=length>4 && memcmp(bytes,"\177ELF",4)==0;
#else
        const bool signature=length>2 && memcmp(bytes,"MZ",2)==0;
#endif
        g_free(bytes); if(!signature) return 49;
        // A long delay detects leaked target processes: a working Stop must
        // finish well before the generated program's natural five-second exit.
        if(g_getenv("HIASM_TEST_STOP_CROSS")) {
            auto delay=sdk.add("Sleep",50,50);
            if(!delay) return 54;
            int milliseconds=5000;
            delay->props.getByName("Delay")->setValue(&milliseconds);
            auto start=entry->points.getByName("onStart");
            start->clear(); start->connect(delay->points.getByName("doSleep"));
            delay->points.getByName("onSleep")->connect(message->points.getByName("doData"));
            if(!builder.build(&sdk)) return 55;
        }
        if(
#ifdef G_OS_WIN32
            true
#else
            g_getenv("HIASM_TEST_WINE")
#endif
        ) {
            // Exercise the editor's own runner, including a stop immediately
            // after launch (before the WSL wrapper can publish its PID).
            for(int attempt=0; attempt<3; ++attempt) {
                builder.run(&sdk);
                if(!builder.isRunning()) return 51;
                const auto started=g_get_monotonic_time();
                if(attempt || g_getenv("HIASM_TEST_STOP_CROSS")) {
                    if(attempt==2) g_usleep(150000);
                    builder.stop();
                }
                const auto deadline=g_get_monotonic_time()+8*G_TIME_SPAN_SECOND;
                while(builder.isRunning() && g_get_monotonic_time()<deadline) {
                    while(g_main_context_iteration(nullptr,false)) {}
                    g_usleep(1000);
                }
                if(builder.isRunning()) return 52;
                if(g_getenv("HIASM_TEST_STOP_CROSS") && g_get_monotonic_time()-started>3*G_TIME_SPAN_SECOND) return 56;
            }
        }
#ifdef G_OS_WIN32
        gchar *parent=g_path_get_dirname(artifact.c_str()),*base=g_path_get_basename(artifact.c_str());
        const auto relative="./"+std::string(base);
        std::vector<std::string> crossArgs{"wsl.exe"};
        if(const gchar *distro=g_getenv("HIASM_WSL_DISTRO")) if(*distro) crossArgs.insert(crossArgs.end(),{"--distribution",distro});
        crossArgs.insert(crossArgs.end(),{"--cd",parent,"--exec",relative});
        std::vector<gchar*> launch; for(auto &arg:crossArgs) launch.push_back(const_cast<gchar*>(arg.c_str())); launch.push_back(nullptr);
        gchar *out=nullptr;
        const bool ran=g_spawn_sync(nullptr,launch.data(),nullptr,G_SPAWN_SEARCH_PATH,nullptr,nullptr,&out,nullptr,&status,nullptr)&&g_spawn_check_wait_status(status,nullptr);
        const std::string output=out?out:""; g_free(out); g_free(parent); g_free(base);
        if(!ran||output.find("Hello from HiAsm")==std::string::npos) return 50;
#endif
        if(g_getenv("HIASM_TEST_WINE")) {
            const char *wine=g_getenv("HIASM_WINE");
            gchar *wineArgs[]={const_cast<gchar*>(wine?wine:"wine"),const_cast<gchar*>(artifact.c_str()),nullptr};
            gchar **environment=g_get_environ();
            environment=g_environ_setenv(environment,"LC_ALL","C.UTF-8",true);
            if(const char *prefix=g_getenv("HIASM_WINE_PREFIX")) environment=g_environ_setenv(environment,"WINEPREFIX",prefix,true);
            gchar *out=nullptr,*err=nullptr;
            const bool ran=g_spawn_sync(nullptr,wineArgs,environment,G_SPAWN_SEARCH_PATH,nullptr,nullptr,&out,&err,&status,nullptr)&&g_spawn_check_wait_status(status,nullptr);
            const std::string output=out?out:"";
            if(!ran) std::cerr<<"Wine output: "<<output<<"; "<<(err?err:"")<<std::endl;
            g_free(out);g_free(err);g_strfreev(environment);
            if(!ran||output.find("Hello from HiAsm")==std::string::npos) return 53;
        }
        g_unlink(artifact.c_str());
        gchar *folder=g_path_get_dirname(artifact.c_str());
        g_unlink((ustring(folder)+u8"/Пример с пробелами.cpp").c_str());g_rmdir(folder);g_free(folder);
        std::cout<<"OK cross compilation: target signature and available execution"<<std::endl;
    }
    {
        // The runtime header is read by block.load; a missing file must be a
        // diagnostic rather than feof(nullptr). Script and header paths must
        // also work when the installation has a non-ASCII name.
        MSDK timer;
        if(!timer.loadFromFile(sdk.pack->path()+"new/Gui.sha")) return 59;
        const auto originalDataDir=dataDir;
        const auto fixture=dir+u8"/Ресурсы RTCG/";
        const auto code=fixture+"packs/cpp/code/";
        if(g_mkdir_with_parents(code.c_str(),0700)) return 60;
        GDir *scripts=g_dir_open((originalDataDir+"packs/cpp/code").c_str(),0,nullptr);
        if(!scripts) return 61;
        std::vector<std::string> files;
        while(auto name=g_dir_read_name(scripts)) {
            gchar *bytes=nullptr; gsize size=0;
            if(!g_file_get_contents((originalDataDir+"packs/cpp/code/"+name).c_str(),&bytes,&size,nullptr)) return 62;
            const bool copied=g_file_set_contents((code+name).c_str(),bytes,size,nullptr);
            g_free(bytes); if(!copied) return 63;
            files.emplace_back(name);
        }
        g_dir_close(scripts);
        dataDir=fixture;
        BuildRecord sample={GetCGTools,&timer,nullptr};
        const bool unicode=generate(&sample)==0 && sample.result;
        // The cpp templates no longer load header files, so the missing-file case uses a probe template
        // that loads a header which does not exist; the block loader must report it.
        const char *probeTemplate="func doStart\n  fvar(runtime)\n  runtime = block.reg(\"probe\")\n  runtime.load(\"missing_runtime.h\")\nend\n";
        if(!g_file_set_contents((code+"hiGui.hws").c_str(),probeTemplate,strlen(probeTemplate),nullptr)) return 66;
        debug.log.clear(); cgt_on_debug+=&debug;
        const bool missing=generate(&sample)!=0 && !sample.result;
        cgt_on_debug-=&debug;
        dataDir=originalDataDir;
        if(!unicode || !missing || debug.log.find("Cannot read block file:")==std::string::npos) {
            std::cerr << "RTCG resource handling: unicode=" << unicode << ", missing=" << missing << "; " << debug.log << std::endl;
            return 64;
        }
        for(const auto &name : files) g_unlink((code+name).c_str());
        g_rmdir(code.c_str()); g_rmdir((fixture+"packs/cpp").c_str());
        g_rmdir((fixture+"packs").c_str()); g_rmdir(fixture.c_str());
        sample.result=nullptr;
        if(generate(&sample)!=0 || !sample.result) return 65;
    }
    g_rmdir((dir + "/build").c_str()); g_rmdir(dir.c_str());
    g_module_close(module);
    std::cout << "OK RTCG: repeated generations, console output, GTK4 GUI lifecycle, library export, daemon, Unicode paths" << std::endl;
    return 0;
}
