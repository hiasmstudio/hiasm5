/*
 * LegacyBuild.cpp
 *
 * HiAsm 4 Delphi package build: src/hiasm4_cgbridge for CodeGen and the
 * project maker, GoRC for allres.rc, the compiler command from Int/hiasm.db.
 */

#include "LegacyBuild.h"
#include "CodeGen.h"
#include "LegacyPackCatalog.h"
#include "SdkSerializer.h"

#include <gio/gio.h>
#include <glib/gstdio.h>
#include <sqlite3.h>
#include <cstring>
#include <string>
#include <vector>
#ifdef G_OS_WIN32
#include <windows.h>
#endif

namespace {

// One row of a query on Int/hiasm.db; strings are stored in Windows-1251.
std::vector<std::string> queryRow(const ustring &root, const char *sql, const std::vector<std::string> &params) {
	std::vector<std::string> row;
	sqlite3 *db = nullptr;
	const std::string file = root + "/Int/hiasm.db";
	if(sqlite3_open_v2(file.c_str(), &db, SQLITE_OPEN_READONLY, nullptr) == SQLITE_OK) {
		sqlite3_stmt *stmt = nullptr;
		if(sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) == SQLITE_OK) {
			for(size_t i = 0; i < params.size(); ++i)
				sqlite3_bind_text(stmt, (int)i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
			if(sqlite3_step(stmt) == SQLITE_ROW)
				for(int i = 0; i < sqlite3_column_count(stmt); ++i) {
					const char *text = (const char*)sqlite3_column_text(stmt, i);
					row.push_back(text ? LegacyPackCatalog::decodeText(text, sqlite3_column_bytes(stmt, i)) : std::string());
				}
		}
		sqlite3_finalize(stmt);
	}
	sqlite3_close(db);
	return row;
}

void replaceAll(std::string &text, const std::string &key, const std::string &value) {
	for(std::string::size_type pos = 0; (pos = text.find(key, pos)) != std::string::npos; pos += value.size())
		text.replace(pos, key.size(), value);
}

std::string nativePath(std::string path) {
	for(char &c : path) if(c == '/') c = G_DIR_SEPARATOR;
	return path;
}
// Copies one file, replacing an existing one.
bool copyFile(const std::string &from, const std::string &to) {
	GFile *a = g_file_new_for_path(from.c_str()), *b = g_file_new_for_path(to.c_str());
	const gboolean ok = g_file_copy(a, b, G_FILE_COPY_OVERWRITE, nullptr, nullptr, nullptr, nullptr);
	g_object_unref(a); g_object_unref(b);
	return ok;
}

// The package code directory with subdirectories (units link .obj files relatively).
bool copyTree(const std::string &from, const std::string &to) {
	if(g_mkdir_with_parents(to.c_str(), 0700) != 0) return false;
	GDir *dir = g_dir_open(from.c_str(), 0, nullptr);
	if(!dir) return false;
	bool ok = true;
	while(const gchar *name = g_dir_read_name(dir)) {
		const std::string src = from + G_DIR_SEPARATOR_S + name, dst = to + G_DIR_SEPARATOR_S + name;
		if(g_file_test(src.c_str(), G_FILE_TEST_IS_DIR)) { ok = copyTree(src, dst) && ok; continue; }
		GFile *a = g_file_new_for_path(src.c_str()), *b = g_file_new_for_path(dst.c_str());
		ok = g_file_copy(a, b, G_FILE_COPY_OVERWRITE, nullptr, nullptr, nullptr, nullptr) && ok;
		g_object_unref(a); g_object_unref(b);
	}
	g_dir_close(dir);
	return ok;
}

// Console tools of HiAsm 4 print in the ANSI code page; the bridge prints UTF-8.
void report(const char *text) {
	if(!text || !*text) return;
	gchar *utf8 = g_utf8_validate(text, -1, nullptr) ? g_strdup(text) : g_locale_to_utf8(text, -1, nullptr, nullptr, nullptr);
	gchar **lines = g_strsplit_set(utf8 ? utf8 : text, "\r\n", -1);
	for(gchar **line = lines; *line; ++line) if(**line) cgt_on_debug.run(*line);
	g_strfreev(lines); g_free(utf8);
}

#ifdef G_OS_WIN32
// AssemblyName written by the project maker of a .NET project (empty for other projects).
std::string assemblyName(const std::string &project) {
	gchar *text = nullptr; gsize size = 0;
	std::string name;
	if(g_file_get_contents(project.c_str(), &text, &size, nullptr)) {
		const std::string xml(text, size);
		const auto begin = xml.find("<AssemblyName>");
		const auto end = begin == std::string::npos ? begin : xml.find("</AssemblyName>", begin);
		if(end != std::string::npos) name = xml.substr(begin + 14, end - begin - 14);
	}
	g_free(text);
	return name;
}

// The compiled file: HiAsm.<ext> of a Delphi project, or <AssemblyName>.exe/.dll of a .NET project.
std::string compiledResult(const std::string &work, const std::string &project, const std::string &output) {
	const char *dot = strrchr(output.c_str(), '.');
	const std::string ext = dot && !strchr(dot, G_DIR_SEPARATOR) ? dot : ".exe";
	std::vector<std::string> stems{"HiAsm"};
	const std::string assembly = assemblyName(project);
	if(!assembly.empty()) stems.push_back(assembly);
	for(const auto &stem : stems)
		for(const std::string suffix : {ext, std::string(".exe"), std::string(".dll")}) {
			const std::string candidate = work + G_DIR_SEPARATOR_S + stem + suffix;
			if(g_file_test(candidate.c_str(), G_FILE_TEST_IS_REGULAR)) return candidate;
		}
	return std::string();
}
#endif

bool run(const std::vector<std::string> &args, const std::string &cwd) {
	std::vector<gchar*> argv;
	for(const auto &arg : args) argv.push_back(const_cast<gchar*>(arg.c_str()));
	argv.push_back(nullptr);
	gchar *out = nullptr, *err = nullptr; gint status = 0; GError *error = nullptr;
	const bool ok = g_spawn_sync(cwd.c_str(), argv.data(), nullptr, G_SPAWN_DEFAULT, nullptr, nullptr,
		&out, &err, &status, &error) && g_spawn_check_wait_status(status, nullptr);
	report(out); report(err);
	if(error) { cgt_on_debug.run((std::string("!") + error->message).c_str()); g_error_free(error); }
	g_free(out); g_free(err);
	return ok;
}

#ifdef G_OS_WIN32
// HiAsm 4 command templates are Windows command lines. dcc32 cannot load
// RLINK32.DLL when started through glib's spawn helper (checked 2026-11-05),
// so the compiler is started with CreateProcess, as HiAsm 4 does.
bool runCommandLine(const std::string &line, const std::string &cwd) {
	SECURITY_ATTRIBUTES sa = {sizeof(sa), nullptr, TRUE};
	HANDLE readPipe = nullptr, writePipe = nullptr;
	if(!CreatePipe(&readPipe, &writePipe, &sa, 0)) return false;
	SetHandleInformation(readPipe, HANDLE_FLAG_INHERIT, 0);
	gunichar2 *cmd = g_utf8_to_utf16(line.c_str(), -1, nullptr, nullptr, nullptr);
	gunichar2 *dir = g_utf8_to_utf16(cwd.c_str(), -1, nullptr, nullptr, nullptr);
	STARTUPINFOW si = {}; si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES; si.hStdOutput = si.hStdError = writePipe;
	PROCESS_INFORMATION pi = {};
	const bool started = cmd && dir && CreateProcessW(nullptr, (LPWSTR)cmd, nullptr, nullptr, TRUE,
		CREATE_NO_WINDOW, nullptr, (LPCWSTR)dir, &si, &pi);
	CloseHandle(writePipe);
	g_free(cmd); g_free(dir);
	std::string output;
	char buffer[4096]; DWORD read = 0;
	while(ReadFile(readPipe, buffer, sizeof(buffer), &read, nullptr) && read) output.append(buffer, read);
	CloseHandle(readPipe);
	DWORD code = 1;
	if(started) {
		WaitForSingleObject(pi.hProcess, INFINITE);
		GetExitCodeProcess(pi.hProcess, &code);
		CloseHandle(pi.hThread); CloseHandle(pi.hProcess);
	} else cgt_on_debug.run("!Cannot start the HiAsm 4 compiler");
	report(output.c_str());
	return started && code == 0;
}
#endif

}
bool legacyActiveCompiler(const ustring &root, const ustring &pack, LegacyCompiler &compiler) {
	const auto row = queryRow(root, "SELECT c.path, c.cmd FROM pack_compilers pc JOIN packs p ON p.id = pc.pack_id "
		"JOIN compilers c ON c.id = pc.cmp_id WHERE lower(p.codename) = lower(?1) AND pc.active = 1", {pack.raw()});
	if(row.size() != 2 || row[0].empty()) return false;
	compiler.path = row[0]; compiler.cmd = row[1];
	return true;
}

bool legacyProjectMaker(const ustring &root, const ustring &pack, const ustring &entry, ustring &make) {
	const auto row = queryRow(root, "SELECT pr.make FROM projects pr JOIN packs p ON p.id = pr.pack_id "
		"WHERE lower(p.codename) = lower(?1) AND pr.entry = ?2", {pack.raw(), entry.raw()});
	if(row.size() != 1 || row[0].empty()) return false;
	make = row[0];
	return true;
}

bool legacyProjectExtension(const ustring &root, const ustring &pack, const ustring &entry, ustring &ext) {
	const auto row = queryRow(root, "SELECT pr.ext FROM projects pr JOIN packs p ON p.id = pr.pack_id "
		"WHERE lower(p.codename) = lower(?1) AND pr.entry = ?2", {pack.raw(), entry.raw()});
	if(row.size() != 1) return false;
	ext = row[0];
	return true;
}

bool buildLegacyProject(MSDK *sdk, const LegacyBuild &build) {
#ifndef G_OS_WIN32
	cgt_on_debug.run("!HiAsm 4 packages can be built on Windows only");
	return false;
#else
	if(!sdk) return false;
	const std::string root = nativePath(build.root), work = nativePath(build.work);
	// Pack::path() of a legacy package ends with a slash.
	std::string pack = nativePath(build.packDir);
	while(pack.size() > 1 && pack.back() == G_DIR_SEPARATOR) pack.pop_back();
	const std::string project = work + "\\HiAsm.dpr", output = nativePath(build.output);
	cgt_on_debug.run("@HiAsm 4: generating the project");
	if(!copyTree(pack + "\\code", work)) { cgt_on_debug.run("!Cannot copy the package code directory"); return false; }
	if(!sdkSerialize(sdk, work + "\\scheme.json", work)) return false;
	// CodeGen loads Elements\Delphi\FTCG_CodeGen.dll below GetStartDir, the folder of the bridge. The
	// bridge therefore runs from a host folder laid out as HiAsm 4: <work>\host\hiasm4_cgbridge.exe and
	// <work>\host\Elements\Delphi\FTCG_CodeGen.dll (the FTCG elements of the package).
	const std::string host = work + "\\host", hostDelphi = host + "\\Elements\\Delphi";
	const std::string bridge = host + "\\hiasm4_cgbridge.exe";
	if(g_mkdir_with_parents(hostDelphi.c_str(), 0700) != 0 || !copyFile(build.bridge, bridge)) {
		cgt_on_debug.run("!Cannot prepare the host folder of the CodeGen bridge");
		return false;
	}
	if(g_file_test((pack + "\\FTCG_CodeGen.dll").c_str(), G_FILE_TEST_IS_REGULAR) &&
	   !copyFile(pack + "\\FTCG_CodeGen.dll", hostDelphi + "\\FTCG_CodeGen.dll")) {
		cgt_on_debug.run("!Cannot copy FTCG_CodeGen.dll to the host folder of the CodeGen bridge");
		return false;
	}
	if(!run({bridge, "--codegen", pack + "\\CodeGen.dll", "--make", pack + "\\make\\" + build.make.raw(),
			"--scheme", work + "\\scheme.json", "--work", work, "--project", project,
			"--rc", work + "\\allres.rc", "--default-icon", root + "\\Int\\main.ico"}, work)) {
		cgt_on_debug.run("!HiAsm 4 code generation failed");
		return false;
	}
	if(!run({root + "\\compiler\\GoRC.exe", "/r", "allres.rc"}, work) ||
	   !g_file_test((work + "\\allres.res").c_str(), G_FILE_TEST_IS_REGULAR)) {
		cgt_on_debug.run("!HiAsm 4 resources were not compiled");
		return false;
	}
	// Substitutions as in HiAsm 4; the work copy stands in for the package code directory.
	std::string program = build.compiler.path.raw(), cmd = build.compiler.cmd.raw();
	replaceAll(program, "%curdir%", root + "\\");
	if(!g_path_is_absolute(program.c_str())) program = root + "\\" + program;
	program = nativePath(program);
	if(!g_file_test(program.c_str(), G_FILE_TEST_IS_REGULAR)) {
		cgt_on_debug.run(("!The HiAsm 4 compiler of this package is not installed: " + program).c_str());
		return false;
	}
	gchar *base = g_path_get_basename(output.c_str());
	replaceAll(cmd, "%fname%", project);
	replaceAll(cmd, "%opath%", work + "\\");
	replaceAll(cmd, "%upath%", work);
	replaceAll(cmd, "%curdir%", root + "\\");
	replaceAll(cmd, "%oname%", base);
	g_free(base);
	const std::string line = "\"" + program + "\" " + cmd;
	gchar *compilerDir = g_path_get_dirname(program.c_str());
	cgt_on_debug.run(line.c_str());
	// Relative switches in the templates (e.g. -FDtools\) refer to the compiler directory.
	const bool compiled = runCommandLine(line, compilerDir);
	g_free(compilerDir);
	const std::string artifact = compiledResult(work, project, output);
	if(!compiled || artifact.empty()) {
		cgt_on_debug.run("!HiAsm 4 compiler did not build the project");
		return false;
	}
	gchar *target = g_path_get_dirname(output.c_str());
	g_mkdir_with_parents(target, 0700);
	g_free(target);
	g_unlink(output.c_str());
	if(!run({bridge, "--complete", "--make", pack + "\\make\\" + build.make.raw(),
			"--project", project, "--app", output}, work)) {
		cgt_on_debug.run("!HiAsm 4 project maker failed");
		return false;
	}
	// Delphi project makers move the result to the output; the .NET maker leaves it in the work folder.
	if(!g_file_test(output.c_str(), G_FILE_TEST_IS_REGULAR) && !copyFile(artifact, output)) {
		cgt_on_debug.run("!HiAsm 4 project maker did not place the result");
		return false;
	}
	return true;
#endif
}

void removeLegacyWorkDirectory(const ustring &work) {
	const std::string dir = nativePath(work.raw());
	if(GDir *d = g_dir_open(dir.c_str(), 0, nullptr)) {
		while(const gchar *name = g_dir_read_name(d)) {
			const std::string path = dir + G_DIR_SEPARATOR_S + name;
			if(g_file_test(path.c_str(), G_FILE_TEST_IS_DIR)) removeLegacyWorkDirectory(path);
			else g_remove(path.c_str());
		}
		g_dir_close(d);
	}
	g_rmdir(dir.c_str());
}

