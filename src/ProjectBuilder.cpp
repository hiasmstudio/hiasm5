/*
 * ProjectBuilder.cpp
 *
 *  Created on: 08.08.2010
 *      Author: dilma
 */

#include "ProjectBuilder.h"
#include "CodeGen.h"
#include "LegacyBuild.h"
#include "settings.h"
#include "RunnerPayload.h"
#include <glib/gstdio.h>
#include <algorithm>
#include <memory>
#include <set>
#include <string>
#include <vector>
#ifdef G_OS_WIN32
#include <windows.h>
#else
#include <signal.h>
#endif

#ifdef G_OS_WIN32
#define HIASM_RUNNER_SUFFIX ".exe"
#else
#define HIASM_RUNNER_SUFFIX ""
#endif

#ifndef CFG_CONSOLE_BUILDER
#include "SettingsManager.h"
#endif

ProjectBuilder::ProjectBuilder():on_run_state(this, CBT_SDK_RUN_CHANGE)
{

}

ProjectBuilder::~ProjectBuilder() {
	if(childWatch) g_source_remove(childWatch);
	if(process) {
		// A detached program may outlive its editor tab; reap it without a
		// callback into a destroyed ProjectBuilder.
		auto paths = new std::vector<ustring>{crossPidFile, crossStopFile};
		g_child_watch_add(process, [](GPid pid, gint, gpointer data) {
			auto paths = static_cast<std::vector<ustring>*>(data);
			for(const auto &path : *paths) if(!path.empty()) g_unlink(path.c_str());
			delete paths; g_spawn_close_pid(pid);
		}, paths);
	}
}

void ProjectBuilder::clearCrossRun() {
	if(!crossPidFile.empty()) g_unlink(crossPidFile.c_str());
	if(!crossStopFile.empty()) g_unlink(crossStopFile.c_str());
	crossPidFile.clear(); crossStopFile.clear(); crossStopCommand.clear();
}

void ProjectBuilder::stop() {
	if(!process) return;
#ifdef G_OS_WIN32
	if(!crossStopCommand.empty()) {
		// The marker closes the race with a WSL wrapper that has not yet
		// published its PID. Kill only the process group created for this run.
		g_file_set_contents(crossStopFile.c_str(), "", 0, nullptr);
		std::vector<gchar*> args;
		for(auto &arg : crossStopCommand) args.push_back(const_cast<gchar*>(arg.c_str()));
		args.push_back(nullptr);
		GError *error = nullptr;
		g_spawn_async(nullptr, args.data(), nullptr, G_SPAWN_SEARCH_PATH,
			nullptr, nullptr, nullptr, &error);
		if(error) { cgt_on_debug.run(error->message); g_error_free(error); }
		return;
	}
	TerminateProcess(process, 0);
#else
	kill(-process, SIGTERM);
#endif
}

// GCC/ld on Windows disagree about non-ASCII output arguments. Compile in an
// isolated directory with ASCII basenames, then move the result with GIO.
class CompilerWorkspace {
public:
	ustring dir, source, output;
	~CompilerWorkspace() {
		if(!source.empty()) g_unlink(source.c_str());
		if(!output.empty()) g_unlink(output.c_str());
		if(!dir.empty()) g_rmdir(dir.c_str());
	}
	bool create(const ustring &target, const ustring &input) {
		gchar *parent = g_path_get_dirname(target.c_str());
		std::string pattern = std::string(parent) + G_DIR_SEPARATOR_S ".hiasm-compile-XXXXXX";
		g_free(parent);
		if(!g_mkdtemp(&pattern[0])) return false;
		dir = pattern;
		source = dir + "/source.cpp";
		output = dir + "/program" HIASM_RUNNER_SUFFIX;
		GFile *from = g_file_new_for_path(input.c_str()), *to = g_file_new_for_path(source.c_str());
		const bool ok = g_file_copy(from, to, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);
		g_object_unref(from); g_object_unref(to);
		return ok;
	}
	bool publish(const ustring &target) {
		GFile *from = g_file_new_for_path(output.c_str()), *to = g_file_new_for_path(target.c_str());
		const bool ok = g_file_move(from, to, G_FILE_COPY_OVERWRITE, NULL, NULL, NULL, NULL);
		g_object_unref(from); g_object_unref(to);
		return ok;
	}
};

// Keep the compiler environment local to each child; opening/building a
// project must not rewrite the editor's PATH or pkg-config configuration.
class CppToolchain {
    gchar **environment = nullptr;
public:
    ustring root;
    bool llvm = false;
    ~CppToolchain() { g_strfreev(environment); }
    bool configure(bool gtk = false) {
#ifdef G_OS_WIN32
        const gchar *configured = g_getenv("HIASM_CPP_TOOLCHAIN");
        const auto bundled = dataDir + "packs/cpp/compiler/llvm-mingw";
        root = configured && *configured ? configured : gtk ? dataDir + "toolchains/mingw64" : bundled;
        if(!gtk && (!configured || !*configured) && !g_file_test((root + "/bin/clang++.exe").c_str(), G_FILE_TEST_IS_REGULAR)) {
            const auto bin = g_getenv("HIASM_QT_COMPILER_DIR");
            if(bin && *bin) {
                auto *parent = g_path_get_dirname(bin);
                root = parent;
                g_free(parent);
            } else root = "C:/Qt/Tools/llvm-mingw1706_64";
        }
        llvm = g_file_test((root + "/bin/clang++.exe").c_str(), G_FILE_TEST_IS_REGULAR);
        if(!llvm && !g_file_test((root + "/bin/g++.exe").c_str(), G_FILE_TEST_IS_REGULAR)) {
            root.clear();
            if(configured && *configured) {
                cgt_on_debug.run("!HIASM_CPP_TOOLCHAIN does not contain bin/clang++.exe or bin/g++.exe");
                return false;
            }
            if(gtk) return true;
            cgt_on_debug.run("!LLVM-MinGW is missing. Install the C++ toolchain or set HIASM_CPP_TOOLCHAIN.");
            return false;
        }
        gchar *absolute = g_canonicalize_filename(root.c_str(), nullptr);
        root = absolute; g_free(absolute);
        environment = g_get_environ();
        const gchar *path = g_environ_getenv(environment, "PATH");
        const std::string childPath = root.raw() + "/bin;" + dataDir.raw() + ";" + (path ? path : "");
        environment = g_environ_setenv(environment, "PATH", childPath.c_str(), TRUE);
        environment = g_environ_setenv(environment, "PKG_CONFIG_PATH", "", TRUE);
        environment = g_environ_setenv(environment, "PKG_CONFIG_LIBDIR",
            (root + "/lib/pkgconfig;" + root + "/share/pkgconfig").c_str(), TRUE);
#endif
        return true;
    }
    gchar **env() { return environment; }
    ustring compiler(const ustring &configured) const {
        return !root.empty() && (configured == "g++" || configured == "g++.exe")
            ? root + (llvm ? "/bin/clang++.exe" : "/bin/g++.exe") : configured;
    }
    ustring pkgConfig() const { return root.empty() ? "pkg-config" : root + "/bin/pkg-config.exe"; }
};

bool ProjectBuilder::buildIneternal(MSDK *sdk, const ustring &name) {
	gchar *cont = NULL;
	gsize len = 0;
	if(!g_file_get_contents((dataDir + "runner" HIASM_RUNNER_SUFFIX).c_str(), &cont, &len, NULL) || len == 0) {
		cgt_on_debug.run("!Scheme runner not found!");
		if(cont)
			g_free(cont);
		return false;
	}
	std::string executable(cont, len);
	g_free(cont);
	ustring text;
	sdk->saveToText(text, ustring());
	text = "Make(_base)" LINE_END + text;
	if(!RunnerPayload::append(executable, text.raw(), dataDir.raw())) {
		cgt_on_debug.run("!Internal scheme exceeds runner payload limits");
		return false;
	}
	GError *error = NULL;
	if(!g_file_set_contents(name.c_str(), executable.data(), executable.size(), &error)) {
		cgt_on_debug.run((ustring("!Failed to write executable: ") + error->message).c_str());
		g_error_free(error);
		return false;
	}
	return true;
}

typedef struct {
    void *cgt;
    MSDK *sdk;
    void *result;
} TBuildProcessRec;
static_assert(sizeof(TBuildProcessRec) == sizeof(void *) * 3,
              "CodeGen build record ABI mismatch");

typedef int(*TbuildPrepareProc)(void *);
typedef int(*TbuildProcessProc)(TBuildProcessRec *);

static bool cppTarget(MSDK *sdk, const char *entry) {
    return sdk->pack->name == "cpp" && sdk->packProject && sdk->packProject->entry == entry;
}

// Projects that open a GTK4 window: the box-chain entry and the form-based one.
static bool guiTarget(MSDK *sdk) {
    return cppTarget(sdk, "Gui") || cppTarget(sdk, "MainForm");
}

static ElementProperty *guiBackend(MSDK *sdk) {
    if(cppTarget(sdk, "MainForm")) {
        for(auto element : sdk->elements)
            if(element->tpl->name == "MainForm" && (element->flag & ELEMENT_FLG_IS_PARENT))
                return element->props.getByName("Backend");
    }
    return nullptr;
}

static bool releaseBuild(MSDK *sdk);

static bool qtTarget(MSDK *sdk) {
    const auto backend = guiBackend(sdk);
    return backend && backend->readInt() == 1;
}

class QtToolchain {
public:
    std::string prefix, compilerBin, toolsBin, compiler;
    gchar **environment = nullptr;

    QtToolchain() {
        if(const char *dir = g_getenv("HIASM_QT_DIR")) prefix = dir;
        if(const char *dir = g_getenv("HIASM_QT_COMPILER_DIR")) compilerBin = dir;
#ifdef G_OS_WIN32
        const auto bundle = dataDir.raw() + "packs/cpp/compiler/";
        if(g_file_test((bundle + "tools/bin/cmake.exe").c_str(), G_FILE_TEST_IS_REGULAR)) toolsBin = bundle + "tools/bin";
        if(prefix.empty()) prefix = g_file_test((bundle + "qt6/lib/cmake/Qt6/Qt6Config.cmake").c_str(), G_FILE_TEST_IS_REGULAR)
            ? bundle + "qt6" : "C:/Qt/6.9.3/llvm-mingw_64";
        if(compilerBin.empty() && !prefix.empty()) {
            if(prefix == bundle + "qt6") compilerBin = bundle + "llvm-mingw/bin";
            else if(g_str_has_suffix(prefix.c_str(), "llvm-mingw_64")) compilerBin = prefix + "/../../Tools/llvm-mingw1706_64/bin";
            else compilerBin = prefix + "/../../Tools/mingw1310_64/bin";
        }
        if(toolsBin.empty()) {
            const auto installed = prefix + "/../../Tools/CMake_64/bin";
            if(g_file_test((installed + "/cmake.exe").c_str(), G_FILE_TEST_IS_REGULAR)) toolsBin = installed;
        }
#endif
        auto absolute = [](std::string &path) {
            if(path.empty()) return;
            gchar *value = g_canonicalize_filename(path.c_str(), nullptr);
            path = value;
            g_free(value);
        };
        absolute(prefix);
        absolute(compilerBin);
        absolute(toolsBin);
        if(!compilerBin.empty()) compiler = compilerBin +
            (g_file_test((compilerBin + "/clang++" HIASM_RUNNER_SUFFIX).c_str(), G_FILE_TEST_IS_REGULAR)
             ? "/clang++" HIASM_RUNNER_SUFFIX : "/g++" HIASM_RUNNER_SUFFIX);
        environment = g_get_environ();
        std::string path;
        if(!prefix.empty()) path += prefix + "/bin" G_SEARCHPATH_SEPARATOR_S;
        if(!compilerBin.empty()) path += compilerBin + G_SEARCHPATH_SEPARATOR_S;
        if(!toolsBin.empty()) path += toolsBin + G_SEARCHPATH_SEPARATOR_S;
        if(const char *existing = g_environ_getenv(environment, "PATH")) path += existing;
        environment = g_environ_setenv(environment, "PATH", path.c_str(), TRUE);
#ifdef G_OS_WIN32
        environment = g_environ_setenv(environment, "QT_PLUGIN_PATH", (prefix + "/plugins").c_str(), TRUE);
#endif
    }

    ~QtToolchain() { g_strfreev(environment); }
};

static bool compileQt(MSDK *sdk, CompilerWorkspace &workspace, const ustring &target) {
    QtToolchain qt;
    const auto recipe = sdk->pack->path() + "qt";
    gchar *absoluteRecipe = g_canonicalize_filename(recipe.c_str(), nullptr);
    gchar *absoluteSource = g_canonicalize_filename(workspace.source.c_str(), nullptr);
    const std::string cmake = qt.toolsBin.empty() ? "cmake" HIASM_RUNNER_SUFFIX : qt.toolsBin + "/cmake" HIASM_RUNNER_SUFFIX;
    const std::string config = releaseBuild(sdk) ? "Release" : "Debug";
    std::vector<std::string> configure{cmake, "-S", absoluteRecipe, "-B", "cmake-build", "-G", "Ninja",
        "-DCMAKE_BUILD_TYPE=" + config, std::string("-DHIASM_SOURCE=") + absoluteSource};
    g_free(absoluteRecipe);
    g_free(absoluteSource);
    if(!qt.prefix.empty()) configure.push_back("-DCMAKE_PREFIX_PATH=" + qt.prefix);
    if(!qt.compiler.empty()) configure.push_back("-DCMAKE_CXX_COMPILER=" + qt.compiler);
    if(const char *ninja = g_getenv("HIASM_NINJA")) {
        if(*ninja) configure.push_back(std::string("-DCMAKE_MAKE_PROGRAM=") + ninja);
    }
#ifdef G_OS_WIN32
    else if(g_file_test((qt.toolsBin + "/ninja.exe").c_str(), G_FILE_TEST_IS_REGULAR))
        configure.push_back("-DCMAKE_MAKE_PROGRAM=" + qt.toolsBin + "/ninja.exe");
    else if(!qt.prefix.empty()) {
        const auto ninja = qt.prefix + "/../../Tools/Ninja/ninja.exe";
        if(g_file_test(ninja.c_str(), G_FILE_TEST_IS_REGULAR)) configure.push_back("-DCMAKE_MAKE_PROGRAM=" + ninja);
    }
#endif
    auto invoke = [&](const std::vector<std::string> &command) {
        std::vector<gchar*> argv;
        for(const auto &arg : command) argv.push_back(const_cast<gchar*>(arg.c_str()));
        argv.push_back(nullptr);
        gchar *out = nullptr, *err = nullptr;
        gint status = -1;
        GError *error = nullptr;
        const bool ok = g_spawn_sync(workspace.dir.c_str(), argv.data(), qt.environment, G_SPAWN_SEARCH_PATH_FROM_ENVP,
            nullptr, nullptr, &out, &err, &status, &error) && g_spawn_check_wait_status(status, &error);
        if(out && *out) cgt_on_debug.run(out);
        if(err && *err) cgt_on_debug.run(err);
        if(error) { cgt_on_debug.run(error->message); g_error_free(error); }
        g_free(out);
        g_free(err);
        return ok;
    };
    cgt_on_debug.run("@Building with Qt Widgets and CMake");
    const auto buildDir = workspace.dir + "/cmake-build";
    workspace.output = buildDir + "/program" HIASM_RUNNER_SUFFIX;
    const bool ok = invoke(configure) && invoke({cmake, "--build", "cmake-build"}) && workspace.publish(target);
    removeLegacyWorkDirectory(buildDir);
    if(!ok) cgt_on_debug.run("!Qt build failed. Check CMake, Ninja, the Qt 6.9+ Widgets SDK (HIASM_QT_DIR), and its matching C++ compiler (HIASM_QT_COMPILER_DIR for MinGW).");
    return ok;
}

// Projects built as a module that another program loads: a host (Library, Plugin),
// the Windows shell (WinDll, ComExt, CplApp). They are never run from the editor.
static bool sharedTarget(MSDK *sdk) {
    return cppTarget(sdk, "Library") || cppTarget(sdk, "Plugin") || cppTarget(sdk, "WinDll") ||
           cppTarget(sdk, "ComExt") || cppTarget(sdk, "CplApp");
}

// Windows API import libraries that the Windows-only project types need beyond the defaults.
static bool windowsTarget(MSDK *sdk);
static std::vector<std::string> systemLibraries(MSDK *sdk) {
    if(cppTarget(sdk, "ComExt")) return {"-lole32", "-luuid", "-ladvapi32"};
    if(cppTarget(sdk, "WinService")) return {"-ladvapi32"};
    // The GTK4 form calls GDI (regions, icon, bitmaps) and USER (taskbar, shadow, flash) on Windows.
    if(guiTarget(sdk) && windowsTarget(sdk)) return {"-lgdi32", "-luser32"};
    return {};
}

static bool releaseBuild(MSDK *sdk) {
    if(!sdk->packProject) return false;
    for(Element *element : sdk->elements) {
        if(element->tpl->name == sdk->packProject->entry) {
            auto mode = element->props.getByName("BuildMode");
            return mode && mode->readInt() == 1;
        }
    }
    return false;
}

static int targetPlatform(MSDK *sdk) {
    if(sdk->pack->name != "cpp" || !sdk->packProject) return 0;
    for(auto element : sdk->elements) if(element->tpl->name == sdk->packProject->entry) {
        auto property = element->props.getByName("TargetPlatform");
        return property ? property->readInt() : 0;
    }
    return 0;
}
static bool windowsTarget(MSDK *sdk) {
    const int target = targetPlatform(sdk);
#ifdef G_OS_WIN32
    return target != 2;
#else
    return target == 1;
#endif
}
static bool crossTarget(MSDK *sdk) {
#ifdef G_OS_WIN32
    return targetPlatform(sdk) == 2;
#else
    return targetPlatform(sdk) == 1;
#endif
}

// Cross builds never invoke a shell or interpolate project paths into a shell
// command. The workspace has fixed ASCII source/output basenames.
static bool crossCompile(MSDK *sdk, CompilerWorkspace &workspace) {
    std::vector<std::string> prefix;
#ifdef G_OS_WIN32
    prefix.push_back("wsl.exe");
    if(const gchar *distro = g_getenv("HIASM_WSL_DISTRO")) {
        if(*distro) { prefix.push_back("--distribution"); prefix.push_back(distro); }
    }
    prefix.insert(prefix.end(), {"--cd", workspace.dir.raw(), "--exec"});
    const std::string compiler="g++",pkgConfig="pkg-config";
#else
    const gchar *configured=g_getenv("HIASM_MINGW_CXX");
    std::string compiler=configured && *configured ? configured : "x86_64-w64-mingw32-g++";
    const gchar *pkg=g_getenv("HIASM_MINGW_PKG_CONFIG");
    std::string pkgConfig=pkg && *pkg ? pkg : "x86_64-w64-mingw32-pkg-config";
    if(const gchar *root=g_getenv("HIASM_CPP_WINDOWS_TOOLCHAIN")) if(*root) {
        if(!g_path_is_absolute(root) || !g_file_test((ustring(root)+"/bin/g++.exe").c_str(),G_FILE_TEST_IS_REGULAR)) {
            cgt_on_debug.run("!HIASM_CPP_WINDOWS_TOOLCHAIN must contain the Windows toolchain at an absolute Linux path");
            return false;
        }
        prefix.push_back("sh");
        compiler=dataDir.raw()+"tools/cpp_wine_gxx.sh";
        pkgConfig=dataDir.raw()+"tools/cpp_wine_pkg_config.sh";
    }
#endif
    auto invoke=[&](const std::vector<std::string> &command, std::string &stdoutText) {
        std::vector<gchar*> args;
        for(const auto &arg : command) args.push_back(const_cast<gchar*>(arg.c_str()));
        args.push_back(nullptr);
        gchar *out=nullptr,*err=nullptr; gint status=-1; GError *error=nullptr;
        bool ok=g_spawn_sync(workspace.dir.c_str(),args.data(),nullptr,G_SPAWN_SEARCH_PATH,nullptr,nullptr,&out,&err,&status,&error)
            && g_spawn_check_wait_status(status,&error);
        stdoutText=out?out:"";
        if(err && *err) cgt_on_debug.run(err);
        if(error) { cgt_on_debug.run(error->message); g_error_free(error); }
        g_free(out); g_free(err); return ok;
    };
    auto args=prefix;
    args.insert(args.end(),{compiler,"-std=c++17","-finput-charset=UTF-8","-fexec-charset=UTF-8","source.cpp","-o",windowsTarget(sdk)?"target.exe":"target"});
    if(releaseBuild(sdk)) args.insert(args.end(),{"-O2","-DNDEBUG"}); else args.insert(args.end(),{"-O0","-g"});
    if(sharedTarget(sdk)) {
        args.push_back("-shared"); if(!windowsTarget(sdk)) args.push_back("-fPIC");
    }
    if(windowsTarget(sdk) && !guiTarget(sdk)) args.push_back("-static");
    if(windowsTarget(sdk) && (guiTarget(sdk) || cppTarget(sdk,"Daemon") || cppTarget(sdk,"Service") || cppTarget(sdk,"WinService"))) args.push_back("-mwindows");
    // The legacy form setting builds GUI windows without a frame (see hiMainForm.hws).
    for(const auto &library : systemLibraries(sdk)) args.push_back(library);
    if(guiTarget(sdk)) {
        auto query=prefix; query.insert(query.end(),{pkgConfig,"--cflags","--libs","gtk4"});
        std::string flags;
        if(!invoke(query,flags)) { cgt_on_debug.run("!Target GTK4 development libraries are missing"); return false; }
        gchar **parsed=nullptr; GError *error=nullptr;
        if(!g_shell_parse_argv(flags.c_str(),nullptr,&parsed,&error)) {
            if(error) { cgt_on_debug.run(error->message); g_error_free(error); } return false;
        }
        for(gchar **flag=parsed; *flag; ++flag) args.push_back(*flag);
        g_strfreev(parsed);
    }
    std::string output;
    cgt_on_debug.run(windowsTarget(sdk)?"@Cross compiling for Windows x64":"@Cross compiling for Linux x64 in WSL");
    const auto result=workspace.dir+(windowsTarget(sdk)?"/target.exe":"/target");
    g_unlink(workspace.output.c_str()); workspace.output=result;
    if(!invoke(args,output)) {
        if(!output.empty()) cgt_on_debug.run(output.c_str());
        cgt_on_debug.run("!Cross compilation failed; check the target compiler and development libraries");
        return false;
    }
    return g_file_test(result.c_str(),G_FILE_TEST_IS_REGULAR);
}

static ustring outputSuffix(MSDK *sdk) {
    if(sdk->pack->isLegacy()) {
        // The extension of a HiAsm 4 project (exe, dll, cpl) is stored in Int/hiasm.db, not in the package.
        ustring ext;
        if(sdk->packProject && legacyProjectExtension(sdk->pack->legacyRoot(), sdk->pack->name, sdk->packProject->entry, ext) && !ext.empty())
            return "." + ext;
        return ustring();
    }
    if(cppTarget(sdk, "Library") || cppTarget(sdk, "Plugin")) {
        return windowsTarget(sdk) ? ".dll" : ".so";
    }
    if(cppTarget(sdk, "WinDll") || cppTarget(sdk, "ComExt")) return ".dll";
    if(cppTarget(sdk, "CplApp")) return ".cpl";
    if(sdk->pack->name == "cpp") return windowsTarget(sdk) ? ".exe" : "";
    if(sdk->pack->name == "_base" || sdk->pack->name == "cpp") return HIASM_RUNNER_SUFFIX;
    return sdk->packProject && !sdk->packProject->ext.empty() ? "." + sdk->packProject->ext : ustring();
}

// Names of the elements (nested schemes included) that have no code in the package.
static void collectUncompiled(Pack *pack, SDK *sdk, std::set<std::string> &missing) {
	for(Element *element : sdk->elements) {
		const unsigned char kind = element->cIndex;
		const bool wiring = kind == CI_EditMulti || kind == CI_EditMultiEx || (element->flag & ELEMENT_FLG_IS_SYSTEM);
		if(!wiring && !pack->compiles(element->tpl->name)) missing.insert(element->tpl->name.raw());
		if(element->sdk) collectUncompiled(pack, element->sdk, missing);
	}
}

// HiAsm 4 Delphi packages (legacy): the 32-bit bridge runs the package CodeGen and project maker,
// GoRC and the package compiler build the program (src/LegacyBuild.cpp). The result goes to the
// build directory beside the scheme, as for the other project types.
static bool buildLegacyScheme(MSDK *sdk, const ustring &output) {
	const ustring root = sdk->pack->legacyRoot();
	if(root.empty() || !sdk->packProject) {
		cgt_on_debug.run("!The HiAsm 4 installation or the project entry of this package is unknown");
		return false;
	}
	LegacyBuild build;
	build.root = root;
	build.packDir = sdk->pack->path();
	build.bridge = dataDir + "hiasm4_cgbridge.exe";
	if(!g_file_test(build.bridge.c_str(), G_FILE_TEST_IS_REGULAR)) {
		cgt_on_debug.run("!The 32-bit CodeGen bridge is not built; configure with -DHIASM_BUILD_CGBRIDGE=ON");
		return false;
	}
	if(!legacyActiveCompiler(root, sdk->pack->name, build.compiler)) {
		cgt_on_debug.run("!The HiAsm 4 package has no active compiler in Int/hiasm.db");
		return false;
	}
	if(!legacyProjectMaker(root, sdk->pack->name, sdk->packProject->entry, build.make)) {
		cgt_on_debug.run(("!The HiAsm 4 package has no project maker for " + sdk->packProject->entry.raw()).c_str());
		return false;
	}
	gchar *work = g_dir_make_tmp("hiasm-legacy-XXXXXX", nullptr);
	if(!work) {
		cgt_on_debug.run("!Cannot create a work directory for the HiAsm 4 build");
		return false;
	}
	build.work = work;
	build.output = output;
	const bool built = buildLegacyProject(sdk, build);
	// A failed build keeps its work directory so that the generated files can be inspected.
	if(built) removeLegacyWorkDirectory(build.work);
	else cgt_on_debug.run(("@Work directory kept for inspection: " + build.work.raw()).c_str());
	g_free(work);
	return built;
}

bool ProjectBuilder::buildWithCodeGen(MSDK *sdk, const ustring &name) {
	if(sdk->pack->isLegacy()) return buildLegacyScheme(sdk, name);
	if(qtTarget(sdk)) {
		std::set<std::string> unsupported;
		for(auto *element : sdk->elements) {
			const auto kind = element->tpl->name.raw();
			if(kind == "MainForm" && !(element->flag & ELEMENT_FLG_IS_PARENT)) {
				unsupported.insert("additional MainForm");
            } else if(kind == "MainForm" || kind == "Button" || kind == "Label" || kind == "Edit" || kind == "CheckBox") {
				for(auto *point : element->points) {
					if(!point->point) continue;
					const auto name = point->name.raw();
					const bool supported = kind == "MainForm" ?
						(name == "onCreate" || name == "onClose" || name == "doCaption" || name == "doClose" ||
						 name == "doMinimize" || name == "doRestore" || name == "doVisible" ||
						 name == "doEnabled" || name == "doHint") : kind == "Button" ? name == "onClick" :
                        kind == "Label" ? (name == "Caption" || name == "doText" || name == "Text") :
                        kind == "CheckBox" ? (name == "doCheck" || name == "onCheck" || name == "onClick" || name == "Checked") :
						(name == "onChange" || name == "onEnter" || name == "Text" || name == "Str" ||
						 name == "doText" || name == "doText2" || name == "doSelectAll" || name == "doPosition" || name == "Position");
					if(!supported) unsupported.insert(kind + "." + name);
				}
			} else if(kind != "InfoTip") unsupported.insert(kind);
		}
		if(!unsupported.empty()) {
			std::string list;
			for(const auto &item : unsupported) list += (list.empty() ? "" : ", ") + item;
			cgt_on_debug.run(("!Qt MainForm does not support: " + list).c_str());
			return false;
		}
	}
	if(sdk->pack->name == "cpp") {
		// A program must never silently lose an element: say which ones cannot be compiled.
		std::set<std::string> missing;
		collectUncompiled(sdk->pack, sdk, missing);
		if(!missing.empty()) {
			std::string list;
			for(const auto &item : missing) list += (list.empty() ? "" : ", ") + item;
			cgt_on_debug.run(("!The C++ package has no code for: " + list +
				". Remove these elements or edit the scheme as a Core document").c_str());
			return false;
		}
	}
	ustring cgen(sdk->pack->path() + "compiler" PATH_SLASH);
#ifdef G_OS_WIN32
	cgen += "CodeGen.dll";
#else
	cgen += "libCodeGen.so";
#endif
	if(!file_test(cgen, FILE_TEST_EXISTS)) {
		cgt_on_debug.run((ustring("!Codegen not found: ") + cgen).c_str());
		return false;
	}

	GModule *handle  = g_module_open(cgen.c_str(), G_MODULE_BIND_LAZY);
	if(!handle) {
		cgt_on_debug.run((ustring("!Error load codegen: ") + cgen).c_str());
		return false;
	}
	// The result must be released before the plugin module is unloaded.
	std::unique_ptr<GModule, decltype(&g_module_close)> module(handle, g_module_close);
	typedef void (*FreeBuildResult)(void *);
	FreeBuildResult freeResult = nullptr;
	g_module_symbol(handle, "freeBuildResult", (gpointer *)&freeResult);
	TbuildPrepareProc buildPrepareProc;
	if(g_module_symbol(handle, "buildPrepareProc", (gpointer *)&buildPrepareProc)) {
		int x = (*buildPrepareProc)(0);
		if(x) {
			cgt_on_debug.run("!Error buildPrepareProc");
			return false;
		}
	}
	else {
		cgt_on_debug.run("!Proc buildPrepareProc not found");
		return false;
	}

	resources.init();
	bool success = false;

	TbuildProcessProc buildProcessProc;
	if(g_module_symbol(handle, "buildProcessProc", (gpointer *)&buildProcessProc)) {
		TBuildProcessRec rec = {};
		rec.cgt = GetCGTools;
		rec.sdk = sdk;
		int x = (*buildProcessProc)(&rec);
		// Older generators can return borrowed storage: only use an explicit export.
		auto release = [freeResult](void *result) {
			if(freeResult && result) freeResult(result);
		};
		std::unique_ptr<void, decltype(release)> generated(rec.result, release);
		if(x)
			cgt_on_debug.run("!Error buildProcessProc");
        if(!x && rec.result && sdk->packProject) {
            ustring stem = name;
            const ustring suffix = outputSuffix(sdk);
            if(!suffix.empty() && stem.size() >= suffix.size() &&
               stem.substr(stem.size() - suffix.size()) == suffix)
                stem.erase(stem.size() - suffix.size());
            if(sdk->packProject->make(stem, rec.result)) {
                if(qtTarget(sdk)) {
                    CompilerWorkspace workspace;
                    if(!workspace.create(name, stem + ".cpp")) {
                        cgt_on_debug.run("!Cannot create Qt compiler workspace");
                        return false;
                    }
                    return compileQt(sdk, workspace, name);
                }
                Compiler *compiler = sdk->pack->getActiveCompiler();
                if(compiler) {
                    CompilerWorkspace workspace;
                    const bool cpp = sdk->pack->name == "cpp";
                    if(cpp && !workspace.create(name, stem + ".cpp")) {
                        cgt_on_debug.run("!Cannot create compiler workspace");
                        return false;
                    }
                    if(cpp && crossTarget(sdk)) {
                        success = crossCompile(sdk,workspace) && workspace.publish(name);
                        return success;
                    }
                    ustring program = compiler->path;
                    CppToolchain toolchain;
                    if(cpp) {
                        if(!toolchain.configure(guiTarget(sdk))) return false;
                        program = toolchain.compiler(program);
                    }
#ifndef G_OS_WIN32
                    std::string nativeProgram = program.raw();
                    std::replace(nativeProgram.begin(), nativeProgram.end(), '\\', '/');
                    program = nativeProgram;
#endif
                    auto replace = [](ustring &text, const ustring &key, const ustring &value) {
                        ustring::size_type pos = 0;
                        while((pos = text.find(key, pos)) != ustring::npos) {
                            text.replace(pos, key.size(), value); pos += value.size();
                        }
                    };
                    const ustring source = cpp ? "source.cpp" : stem + "." + sdk->packProject->ext;
                    ustring arguments = compiler->cmd;
                    if(cpp) {
                        if(!toolchain.root.empty() && !toolchain.llvm) {
                            gchar *sysroot = g_shell_quote(("--sysroot=" + toolchain.root).c_str());
                            arguments += " " + ustring(sysroot); g_free(sysroot);
                        }
                        arguments += releaseBuild(sdk) ? " -O2 -DNDEBUG" : " -O0 -g";
                        if(toolchain.llvm && !guiTarget(sdk)) arguments += " -static";
                        if(sharedTarget(sdk)) {
                            arguments += " -shared";
#ifndef G_OS_WIN32
                            arguments += " -fPIC";
#endif
                        }
#ifdef G_OS_WIN32
                        if(guiTarget(sdk) || cppTarget(sdk, "Daemon") || cppTarget(sdk, "Service") || cppTarget(sdk, "WinService"))
                            arguments += " -mwindows";
#endif
                        for(const auto &library : systemLibraries(sdk)) arguments += " " + ustring(library.c_str());
                        if(guiTarget(sdk)) {
                            gchar *flags = NULL, *errors = NULL;
                            gint status = 0;
                            const auto pkgConfig = toolchain.pkgConfig();
                            gchar *pkgArgs[] = {const_cast<gchar*>(pkgConfig.c_str()), const_cast<gchar*>("--cflags"),
                                const_cast<gchar*>("--libs"), const_cast<gchar*>("gtk4"), NULL};
                            GError *error = NULL;
                            bool ok = g_spawn_sync(NULL, pkgArgs, toolchain.env(), G_SPAWN_SEARCH_PATH, NULL, NULL,
                                &flags, &errors, &status, &error) && g_spawn_check_wait_status(status, &error);
                            if(!ok) {
                                cgt_on_debug.run("!GTK4 development files and pkg-config are required for a GUI project");
                                if(error) { cgt_on_debug.run(error->message); g_error_free(error); }
                                if(errors && *errors) cgt_on_debug.run(errors);
                            } else arguments += " " + ustring(flags ? flags : "");
                            g_free(flags); g_free(errors);
                            if(!ok) return false;
                        }
                    }
                    for(ustring *text : {&program, &arguments}) {
                        replace(*text, "%fname%", source);
                        replace(*text, "%oname%", cpp ? "program" HIASM_RUNNER_SUFFIX : name);
                        replace(*text, "%opath%", stem);
                        replace(*text, "%upath%", sdk->pack->pathCode());
                        replace(*text, "%curdir%", dataDir);
                    }
                    if(!g_path_is_absolute(program.c_str()) &&
                       (program.find('/') != ustring::npos || program.find('\\') != ustring::npos))
                        program = sdk->pack->path() + program;
                    gchar *quoted = g_shell_quote(program.c_str());
                    ustring command = ustring(quoted) + " " + arguments;
                    g_free(quoted);
                    gchar **argv = NULL;
                    GError *error = NULL;
                    if(g_shell_parse_argv(command.c_str(), NULL, &argv, &error)) {
                        gchar *output = NULL, *errors = NULL;
                        gint status = 0;
                        const ustring workingDir = cpp ? workspace.dir : sdk->projectDir();
                        cgt_on_debug.run(command.c_str());
                        const bool spawned = g_spawn_sync(workingDir.c_str(), argv, cpp ? toolchain.env() : nullptr,
                            G_SPAWN_SEARCH_PATH, NULL, NULL, &output, &errors, &status, &error);
                        if(output && *output) cgt_on_debug.run(output);
                        if(errors && *errors) cgt_on_debug.run(errors);
                        success = spawned && g_spawn_check_wait_status(status, &error);
                        g_free(output); g_free(errors); g_strfreev(argv);
                    }
                    if(error) {
                        cgt_on_debug.run((ustring("!Compiler failed: ") + error->message).c_str());
                        g_error_free(error);
                    }
                    if(success && cpp) success = workspace.publish(name);
                    success = success && g_file_test(name.c_str(), G_FILE_TEST_IS_REGULAR);
                    if(!success) cgt_on_debug.run("!Compiler did not produce the expected output");
                } else {
                    success = g_file_test(name.c_str(), G_FILE_TEST_IS_REGULAR);
                }
            }
        } else if(!x) cgt_on_debug.run("!Project or generated source not set");
	}
	else {
		cgt_on_debug.run("!Proc buildProcessProc not found");
	}

//	for(Resources::iterator fn = resources.begin(); fn != resources.end(); fn++)
//		unlink(fn->c_str());

	return success;
}

ustring ProjectBuilder::getOutputProjectName(MSDK *sdk) {
	ustring source = sdk->fileName;
	ustring base = Glib::filename_display_basename(source);
	if(base.length() >= 4 && base.substr(base.length() - 4) == ".sha")
		base.erase(base.length() - 4, 4);

	// Keep generated binaries beside the scheme, under a predictable build/
	// directory. This also prevents a project opened from another directory
	// from writing into the process working directory or global user settings.
	gchar *raw_dir = g_path_get_dirname(source.c_str());
	ustring source_dir = raw_dir ? raw_dir : ".";
	if(raw_dir)
		g_free(raw_dir);
	ustring output_dir = source_dir;
	if(!output_dir.empty() && output_dir[output_dir.length() - 1] != PATH_SLASH[0])
		output_dir += PATH_SLASH;
	output_dir += "build" PATH_SLASH;
	if(sdk->pack->name == "cpp") {
#ifdef G_OS_WIN32
		output_dir += windowsTarget(sdk) ? "windows-mingw_x64-" : "linux-x64-";
#else
		output_dir += windowsTarget(sdk) ? "windows-mingw_x64-" : "linux-x64-";
#endif
		if(qtTarget(sdk)) output_dir += "qt-";
		output_dir += releaseBuild(sdk) ? "release" PATH_SLASH : "debug" PATH_SLASH;
	}
	g_mkdir_with_parents(output_dir.c_str(), 0777);

	ustring result = output_dir + base;
    result += outputSuffix(sdk);
	return result;
}

bool ProjectBuilder::build(MSDK *sdk) {
	if(!sdk || !sdk->pack) return false;
    if(sdk->isCoreDocument()) {
        cgt_on_debug.run("!Core documents are tutorials; open a C++ lesson to build or run it");
        return false;
    }
	if(targetPlatform(sdk) < 0 || targetPlatform(sdk) > 2) {
		cgt_on_debug.run("!Unknown C++ TargetPlatform; select Native, Windows or Linux");
		return false;
	}
	if(const auto backend = guiBackend(sdk); backend && backend->readInt() != 0 && backend->readInt() != 1) {
		cgt_on_debug.run("!Unknown MainForm Backend; select GTK4 or Qt 6.9");
		return false;
	}
	if(qtTarget(sdk) && crossTarget(sdk)) {
		cgt_on_debug.run("!Qt MainForm supports native builds only; set TargetPlatform to Native");
		return false;
	}
	cgt_on_debug.run(NULL);
	cgt_on_debug.run("~Begin build project");

	ustring n = getOutputProjectName(sdk);
	cgt_on_debug.run((ustring("~project name: ") + n).c_str());

	//-------------------------------------------------------------
	bool success = sdk->pack->name == "_base" ? buildIneternal(sdk, n) : buildWithCodeGen(sdk, n);
	if(!success) {
		cgt_on_debug.run("!Build failed");
		return false;
	}
	//-------------------------------------------------------------

#ifndef G_PLATFORM_WIN32
	cgt_on_debug.run("@set permissions...");
	chmod(n.c_str(), 0777);
#endif
	cgt_on_debug.run("~End build.");
	return true;
}

void ProjectBuilder::run(MSDK *sdk) {
	if(!sdk || !sdk->pack || isRunning()) return;
    if(sdk->isCoreDocument()) return;
	if(sharedTarget(sdk)) {
		cgt_on_debug.run("!A library or extension project is loaded by its host and cannot be run from the editor");
		return;
	}
	if(cppTarget(sdk, "WinService")) {
		cgt_on_debug.run("!A Windows service is installed with sc.exe and cannot be started from the editor");
		return;
	}
	ustring n = getOutputProjectName(sdk);
	if(sdk->pack->name == "_base" || sdk->pack->name == "cpp" || sdk->pack->isLegacy()) {
		if(!g_file_test(n.c_str(), G_FILE_TEST_EXISTS)) {
			cgt_on_debug.run(("!Executable not found: " + n).c_str());
			return;
		}
        std::vector<std::string> command;
        gchar **environment = nullptr;
        gchar *parent = g_path_get_dirname(n.c_str());
        const ustring directory(parent); g_free(parent);
        clearCrossRun();
        if(crossTarget(sdk)) {
#ifdef G_OS_WIN32
            command.push_back("wsl.exe");
            if(const gchar *distro = g_getenv("HIASM_WSL_DISTRO"))
                if(*distro) command.insert(command.end(), {"--distribution", distro});
            command.insert(command.end(), {"--cd", directory.raw(), "--exec"});
            crossStopCommand = command;
            gchar *uuid = g_uuid_string_random();
            const std::string token = std::string(".hiasm-run-") + uuid;
            g_free(uuid);
            const auto pid = token + ".pid", stop = token + ".stop";
            crossPidFile = directory + "/" + pid;
            crossStopFile = directory + "/" + stop;
            // Project paths are separate argv entries, never shell source.
            command.insert(command.end(), {"setsid", "--wait", "sh", "-c",
                "printf '%s\n' \"$$\" > \"$1\"; if test -e \"$2\"; then exit 0; fi; shift 2; exec \"$@\"",
                "hiasm-run", pid, stop});
            gchar *base = g_path_get_basename(n.c_str());
            command.push_back(std::string("./") + base); g_free(base);
            crossStopCommand.insert(crossStopCommand.end(), {"sh", "-c",
                "if IFS= read -r pid < \"$1\"; then case \"$pid\" in ''|*[!0-9]*) exit 1;; esac; /bin/kill -TERM -- \"-$pid\" 2>/dev/null || :; fi",
                "hiasm-stop", pid});
#else
            const gchar *wine = g_getenv("HIASM_WINE");
            command.push_back(wine && *wine ? wine : "wine");
            command.push_back(n.raw());
            environment = g_get_environ();
            environment = g_environ_setenv(environment, "LC_ALL", "C.UTF-8", true);
            if(const gchar *prefix = g_getenv("HIASM_WINE_PREFIX")) {
                environment = g_environ_setenv(environment, "WINEPREFIX", prefix, true);
            }
            if(const gchar *root=g_getenv("HIASM_CPP_WINDOWS_TOOLCHAIN")) if(*root) {
                const auto bin=ustring(root)+"/bin";
                const char *winepath=g_getenv("HIASM_WINEPATH");
                gchar *query[]={const_cast<gchar*>(winepath && *winepath ? winepath : "winepath"),
                    const_cast<gchar*>("-w"),const_cast<gchar*>(bin.c_str()),nullptr};
                gchar *out=nullptr; gint status=-1;
                if(!g_spawn_sync(nullptr,query,environment,G_SPAWN_SEARCH_PATH,nullptr,nullptr,&out,nullptr,&status,nullptr) ||
                   !g_spawn_check_wait_status(status,nullptr)) {
                    cgt_on_debug.run("!Cannot resolve the Windows toolchain runtime path with winepath");
                    g_free(out); g_strfreev(environment); return;
                }
                const char *previous=g_environ_getenv(environment,"WINEPATH");
                const std::string path=std::string(g_strstrip(out))+(previous && *previous ? ";"+std::string(previous) : "");
                environment=g_environ_setenv(environment,"WINEPATH",path.c_str(),true);
                g_free(out);
            }
#endif
        } else command.push_back(n.raw());
        std::vector<gchar*> args;
        for(auto &arg : command) args.push_back(const_cast<gchar*>(arg.c_str()));
        args.push_back(nullptr);
        GError *error = NULL;
        CppToolchain toolchain;
        std::unique_ptr<QtToolchain> qt;
        if(qtTarget(sdk)) qt = std::make_unique<QtToolchain>();
        else if(!crossTarget(sdk) && sdk->pack->name == "cpp" && !toolchain.configure(guiTarget(sdk))) return;
        GSpawnChildSetupFunc setup = nullptr;
#ifndef G_OS_WIN32
        setup = [](gpointer) { setpgid(0, 0); };
#endif
        if(!g_spawn_async(directory.c_str(), args.data(), qt ? qt->environment : (environment ? environment : toolchain.env()),
            GSpawnFlags(G_SPAWN_DO_NOT_REAP_CHILD | G_SPAWN_SEARCH_PATH), setup, NULL, &process, &error)) {
            cgt_on_debug.run((ustring("!Launch failed: ") + error->message).c_str());
            g_error_free(error);
            process = 0;
            clearCrossRun();
        } else {
            childWatch = g_child_watch_add(process, [](GPid pid, gint, gpointer data) {
                auto builder = static_cast<ProjectBuilder*>(data);
                g_spawn_close_pid(pid);
                builder->process = 0;
                builder->childWatch = 0;
                builder->clearCrossRun();
                builder->on_run_state.run(NULL);
            }, this);
            on_run_state.run(NULL);
        }
        g_strfreev(environment);
	}
	else {
		if(sdk->packProject) sdk->packProject->run(n);
	}
}
