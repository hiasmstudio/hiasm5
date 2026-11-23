// Build probe for HiAsm 4 package schemes: copies each scheme to a work folder (the installation stays
// read-only), builds it through ProjectBuilder as the IDE does and prints the whole build log.
// Usage: legacy_build_probe <build dir> <scheme.sha>...
// Not part of ctest: it needs an installed HiAsm 4 with its packages.
#include <iostream>
#include <string>
#include "SDK.h"
#include "Pack.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "ProjectBuilder.h"
#include "CodeGen.h"
#include <glib/gstdio.h>

struct DebugLog : CallBack {
	void callback(void *, CallbackType, const void *data) override {
		if(data) std::cout << (const char*)data << std::endl;
	}
};

int main(int argc, char **argv) {
	if(argc < 3) return 2;
	initDirs();
	dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
	databaseFile = dataDir + INT_PATH + DATABASE_FILE;
	mdb.open();
	SettingsManager::init();
	compilerSet = new CompilerCollection();
	packSet = new PackCollection();

	DebugLog log;
	cgt_on_debug += &log;
	int failed = 0;
	for(int i = 2; i < argc; i++) {
		std::cout << "== " << argv[i] << std::endl;
		gchar *work = g_dir_make_tmp("legacy-build-XXXXXX", nullptr);
		gchar *base = g_path_get_basename(argv[i]);
		const std::string copy = std::string(work) + G_DIR_SEPARATOR_S + base;
		GFile *from = g_file_new_for_path(argv[i]), *to = g_file_new_for_path(copy.c_str());
		const bool copied = g_file_copy(from, to, G_FILE_COPY_OVERWRITE, nullptr, nullptr, nullptr, nullptr);
		g_object_unref(from); g_object_unref(to);
		MSDK sdk;
		if(!copied || !sdk.loadFromFile(copy)) {
			std::cout << "LOAD FAILED " << argv[i] << std::endl;
			++failed;
		} else {
			ProjectBuilder builder;
			const bool ok = builder.build(&sdk);
			std::cout << "RESULT " << (ok ? "built" : "FAILED") << " " << argv[i] << std::endl;
			failed += ok ? 0 : 1;
		}
		g_free(base); g_free(work);
	}
	cgt_on_debug -= &log;
	return failed ? 1 : 0;
}
