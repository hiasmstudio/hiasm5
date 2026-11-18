// Builds new/MainForm.sha of the HiAsm 4 Delphi package through
// src/hiasm4_cgbridge and the package compiler (src/LegacyBuild.cpp), runs
// the program and checks its window, then checks a custom icon resource.
#include "Pack.h"
#include "SDK.h"
#include "MainDataBase.h"
#include "ElementProperty.h"
#include "LegacyBuild.h"
#include "ProjectBuilder.h"
#include "CodeGen.h"
#include "WindowManager.h"
#include "SettingsManager.h"
#include <glib/gstdio.h>
#include <gio/gio.h>
#include <cstring>
#include <iostream>
#include <string>
#include <windows.h>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)

struct DebugLog : CallBack {
	void callback(void *, CallbackType, const void *data) override { if(data) std::cout << (const char*)data << std::endl; }
};

static std::string readFile(const std::string &path) {
	gchar *text = nullptr; gsize size = 0;
	if(!g_file_get_contents(path.c_str(), &text, &size, nullptr)) return std::string();
	std::string result(text, size); g_free(text); return result;
}

static void removeTree(const std::string &dir) {
	if(GDir *d = g_dir_open(dir.c_str(), 0, nullptr)) {
		while(const gchar *name = g_dir_read_name(d)) {
			const std::string path = dir + G_DIR_SEPARATOR_S + name;
			if(g_file_test(path.c_str(), G_FILE_TEST_IS_DIR)) removeTree(path); else g_remove(path.c_str());
		}
		g_dir_close(d);
	}
	g_rmdir(dir.c_str());
}

static std::string makeWork() {
	gchar *tmp = g_dir_make_tmp("hiasm-cgbridge-XXXXXX", nullptr);
	std::string work(tmp ? tmp : ""); g_free(tmp); return work;
}

// HiAsm 4 scheme text of a resource: "[ZIP" + size (LE) + zlib stream, in hex.
static std::string zipResource(const std::string &data) {
	GConverter *zip = G_CONVERTER(g_zlib_compressor_new(G_ZLIB_COMPRESSOR_FORMAT_ZLIB, -1));
	std::string packed(data.size() + 1024, '\0'); gsize read = 0, written = 0;
	const auto result = g_converter_convert(zip, data.data(), data.size(), &packed[0], packed.size(),
		G_CONVERTER_INPUT_AT_END, &read, &written, nullptr);
	g_object_unref(zip);
	if(result != G_CONVERTER_FINISHED) return std::string();
	packed.resize(written);
	const guint32 size = (guint32)data.size();
	std::string bytes((const char*)&size, 4); bytes += packed;
	std::string text = "[ZIP";
	for(unsigned char c : bytes) { char hex[3]; g_snprintf(hex, sizeof(hex), "%02X", c); text += hex; }
	return text + "]";
}
struct WindowSearch { DWORD pid; HWND found; };

// Main window of the program: visible, top level, not a message box (#32770).
static BOOL CALLBACK findWindow(HWND hwnd, LPARAM param) {
	auto search = (WindowSearch*)param;
	DWORD pid = 0; GetWindowThreadProcessId(hwnd, &pid);
	wchar_t cls[64] = {};
	GetClassNameW(hwnd, cls, 64);
	if(pid == search->pid && IsWindowVisible(hwnd) && wcscmp(cls, L"#32770") != 0) { search->found = hwnd; return FALSE; }
	return TRUE;
}

// Starts the program, waits for its window and closes it; returns the title.
static bool runProgram(const std::string &exe, std::string &title) {
	gunichar2 *wide = g_utf8_to_utf16(exe.c_str(), -1, nullptr, nullptr, nullptr);
	std::wstring cmd = L"\"" + std::wstring((wchar_t*)wide) + L"\"";
	g_free(wide);
	STARTUPINFOW si = {}; si.cb = sizeof(si);
	PROCESS_INFORMATION pi = {};
	if(!CreateProcessW(nullptr, &cmd[0], nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) return false;
	WindowSearch search = {pi.dwProcessId, nullptr};
	for(int i = 0; i < 100 && !search.found && WaitForSingleObject(pi.hProcess, 0) == WAIT_TIMEOUT; ++i) {
		Sleep(100);
		EnumWindows(findWindow, (LPARAM)&search);
	}
	bool ok = search.found != nullptr;
	if(ok) {
		wchar_t text[256] = {};
		GetWindowTextW(search.found, text, 256);
		gchar *utf8 = g_utf16_to_utf8((const gunichar2*)text, -1, nullptr, nullptr, nullptr);
		title = utf8 ? utf8 : ""; g_free(utf8);
		PostMessageW(search.found, WM_CLOSE, 0, 0);
	}
	DWORD code = 1;
	if(WaitForSingleObject(pi.hProcess, 10000) != WAIT_OBJECT_0) { TerminateProcess(pi.hProcess, 1); ok = false; }
	else GetExitCodeProcess(pi.hProcess, &code);
	CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
	return ok && code == 0;
}
int main(int argc, char **argv) {
	if(argc < 4) return 2;
	const std::string root = argv[3], delphi = root + "/Elements/delphi/";
	if(root.empty() || !g_file_test((delphi + "CodeGen.dll").c_str(), G_FILE_TEST_IS_REGULAR)) {
		std::cout << "HiAsm 4 Delphi package is not configured (HIASM4_TEST_ROOT)" << std::endl;
		return 77;
	}
#ifdef HIASM_GTK4
	auto app = Gtk::Application::create("com.hiasm.cgbridge-test", Gio::Application::Flags::NON_UNIQUE);
	app->register_application();
#else
	Gtk::Main kit(argc, argv);
#endif
	initDirs(); dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
	databaseFile = dataDir + "int/hiasm.db"; mdb.open();
	compilerSet = new CompilerCollection();
	g_setenv("HIASM4_ROOTS", root.c_str(), TRUE);
	PackCollection collection; packSet = &collection;
	Pack *pack = collection.getPackByName("delphi"); CHECK(pack && pack->isLegacy());
	pack->ensureLoaded();
	DebugLog log; cgt_on_debug += &log;

	LegacyBuild build;
	build.root = root; build.packDir = root + "/Elements/delphi"; build.bridge = argv[2];
	CHECK(legacyActiveCompiler(root, "delphi", build.compiler));
	CHECK(build.compiler.path.lowercase().find("dcc32.exe") != ustring::npos);
	CHECK(legacyProjectMaker(root, "delphi", "MainForm", build.make) && build.make == "make_exe.dll");

	MSDK form; CHECK(form.loadFromFile(pack->path() + "new/MainForm.sha"));
	CHECK(form.elements.size() == 1);
	// A non-ASCII caption checks the UTF-8 to Windows-1251 path into CodeGen.
	ElementProperty *caption = form.elements.front()->props.getByName("Caption"); CHECK(caption);
	*(ustring*)caption->value = u8"Тест";
	const std::string work = makeWork(); CHECK(!work.empty());
	build.work = work; build.output = work + "\\out\\Test.exe";
	CHECK(buildLegacyProject(&form, build));

	const std::string dpr = readFile(work + "/HiAsm.dpr"), marker = "ClassMain := TClass";
	CHECK(dpr.find("Program HiAsm;") != std::string::npos);
	const auto start = dpr.find(marker); CHECK(start != std::string::npos);
	const auto end = dpr.find(".Create;", start); CHECK(end != std::string::npos);
	const std::string name = dpr.substr(start + marker.size(), end - start - marker.size());
	const std::string unit = readFile(work + "/hi" + name + ".pas");
	CHECK(unit.find("TClass" + name + " = class") != std::string::npos);
	// The caption must reach the source as Windows-1251 (raw or #nnn), never as UTF-8.
	CHECK(unit.find("\xD2\xE5\xF1\xF2") != std::string::npos || unit.find("#210#229#241#242") != std::string::npos);
	CHECK(unit.find("\xD0\xA2") == std::string::npos);
	// An empty icon becomes the HiAsm application icon, as in HiAsm 4.
	CHECK(unit.find("LoadIcon(hInstance,'ASMA')") != std::string::npos);
	CHECK(readFile(work + "/allres.rc").find("ASMA ICON main.ico") != std::string::npos);
	// HiAsm 4 defaults: clBtnFace and MS Sans Serif 8 (src/ElementProperty.cpp, legacy parsing).
	CHECK(unit.find("_prop_Color := -16777201;") != std::string::npos);
	CHECK(unit.find("hiCreateFont('MS Sans Serif',8,0,0,1)") != std::string::npos);
	std::string title;
	CHECK(runProgram(build.output, title));
	std::cout << "Window title: " << title << std::endl;
	// KOL creates ANSI windows; the title converts back to Unicode only with Windows-1251.
	if(GetACP() == 1251) CHECK(title == u8"Тест");
	// HIASM_TEST_KEEP leaves the generated project for inspection.
	if(g_getenv("HIASM_TEST_KEEP")) std::cout << "Kept " << work << std::endl;
	else removeTree(work);

	// A custom icon goes to allres.rc as ICON0 and replaces ASMA.
	const std::string icon = readFile(root + "/Int/main.ico"), payload = zipResource(icon);
	CHECK(!icon.empty() && !payload.empty());
	ElementProperty *iconProp = form.elements.front()->props.getByName("Icon"); CHECK(iconProp);
	*(ustring*)iconProp->value = payload;
	const std::string work2 = makeWork(); CHECK(!work2.empty());
	build.work = work2; build.output = work2 + "\\Test.exe";
	CHECK(buildLegacyProject(&form, build));
	const std::string rc = readFile(work2 + "/allres.rc");
	CHECK(rc.find("ICON0 ICON ICON0.ico") != std::string::npos);
	CHECK(rc.find("ASMA") == std::string::npos);
	CHECK(readFile(work2 + "/ICON0.ico") == icon);
	// The code name contains the bridge's object address, so read it again.
	const std::string dpr2 = readFile(work2 + "/HiAsm.dpr");
	const auto start2 = dpr2.find(marker); CHECK(start2 != std::string::npos);
	const std::string name2 = dpr2.substr(start2 + marker.size(), dpr2.find(".Create;", start2) - start2 - marker.size());
	CHECK(readFile(work2 + "/hi" + name2 + ".pas").find("LoadIcon(hInstance,'ICON0')") != std::string::npos);
	removeTree(work2);
	// The HiAsm 4 example "Hello world" (Example/Info.sha): a button linked to a message, info tips and
	// a main form. It is built from the package's own scheme and must start with its window.
	MSDK example; CHECK(example.loadFromFile(build.packDir + "/Example/Info.sha"));
	CHECK(example.elements.size() >= 4);
	const std::string work3 = makeWork(); CHECK(!work3.empty());
	build.work = work3; build.output = work3 + "\\Info.exe";
	CHECK(buildLegacyProject(&example, build));
	std::string title3;
	CHECK(runProgram(build.output, title3));
	std::cout << "Example window: " << title3 << std::endl;
	removeTree(work3);
	// The IDE path: ProjectBuilder builds a copy of the scheme beside itself (build/Info.exe) and the
	// result starts from the window of the build. The copy keeps the installation read-only.
	const std::string ide = makeWork(); CHECK(!ide.empty());
	const std::string schemeText = readFile(build.packDir + "/Example/Info.sha"); CHECK(!schemeText.empty());
	CHECK(g_file_set_contents((ide + "/Info.sha").c_str(), schemeText.data(), schemeText.size(), nullptr));
	MSDK ideScheme; CHECK(ideScheme.loadFromFile(ide + "/Info.sha"));
	ProjectBuilder builder;
	CHECK(builder.build(&ideScheme));
	const std::string ideExe = ide + "/build/Info.exe";
	CHECK(g_file_test(ideExe.c_str(), G_FILE_TEST_IS_REGULAR));
	std::string ideTitle;
	CHECK(runProgram(ideExe, ideTitle));
	std::cout << "IDE build window: " << ideTitle << std::endl;
	removeTree(ide);
	// FTCG elements: the package generator FTCG_CodeGen.dll is loaded by CodeGen from the bridge host folder.
	// SpeedTest is built and started. ReplaceChars (char arrays) and TechSpeedReport (InlineCode units)
	// are open issues, see MARKDOWN/X86_CODEGEN_BRIDGE_PLAN.md.
	MSDK ftcg; CHECK(ftcg.loadFromFile(build.packDir + "/Example/FTCG/SpeedTest.sha"));
	const std::string work4 = makeWork(); CHECK(!work4.empty());
	build.work = work4; build.output = work4 + "\\SpeedTest.exe";
	CHECK(buildLegacyProject(&ftcg, build));
	std::string title4;
	CHECK(runProgram(build.output, title4));
	std::cout << "FTCG example SpeedTest window: " << title4 << std::endl;
	removeLegacyWorkDirectory(work4);
	// A .NET package (CNet): the project maker names the assembly after the scheme and leaves the result in
	// the work folder, which the build copies to the output. Package codenames match without case (PDA, pda).
	LegacyBuild net;
	net.root = root; net.packDir = root + "/Elements/CNET"; net.bridge = argv[2];
	CHECK(legacyActiveCompiler(root, "cnet", net.compiler));
	CHECK(legacyProjectMaker(root, "cnet", "EntryPoint", net.make) && net.make == "make_cnet.dll");
	LegacyCompiler pdaCompiler;
	CHECK(legacyActiveCompiler(root, "pda", pdaCompiler));
	CHECK(pdaCompiler.path.lowercase().find("delphi_2.88") != ustring::npos);
	MSDK helloNet; CHECK(helloNet.loadFromFile(net.packDir + "/Example/HelloWorld.sha"));
	const std::string work5 = makeWork(); CHECK(!work5.empty());
	net.work = work5; net.output = work5 + "\\out\\HelloWorld.exe";
	CHECK(buildLegacyProject(&helloNet, net));
	CHECK(readFile(net.output).substr(0, 2) == "MZ");
	CHECK(readFile(work5 + "/scheme.json").find("\"projectName\":\"HelloWorld\"") != std::string::npos);
	removeLegacyWorkDirectory(work5);
	cgt_on_debug -= &log;
	std::cout << "HiAsm 4 MainForm generated, compiled and started; custom icon exported" << std::endl;
	return 0;
}
