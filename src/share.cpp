/*
 * share.cpp
 *
 *  Created on: 14.05.2010
 *      Author: dilma
 */

#include "share.h"

static gchar _version_[32] = {0};

gchar *HIASM_VERSION() {
	if(!_version_[0])
		sprintf(_version_, "%d.%d.%d",  HIASM_VERSION_MAJOR, HIASM_VERSION_MINOR, HIASM_VERSION_BUILD);
	return _version_;
}

ustring int_to_str(int value) {
	char buf[16];
	sprintf(buf, "%d", value);
	return buf;
}

ustring double_to_str(double value) {
	std::ostringstream s;
	s << value;
	return s.str();
}

char *getTok(char **buf, char tok) {
	if(**buf == '\0')
		return *buf;

	char *s = *buf;
	while(**buf != tok && **buf)
		(*buf)++;
	**buf = '\0';
	(*buf)++;
	return s;
}

const char *dataNames[] = {
	"NULL",
	"Integer",
	"String",
	"Data",
	"List", //Combo
	"StrList", //data_list
	"Icon",
	"Real",
	"Color",
	"Script", //data_script = 9;
	"Stream",
	"Bitmap",
	"Wave",
	"Array", //data_array = 13;
	"ListEx", //combo Ex
	"Font", // font
	"Matrix",  // matrix
	"Jpeg", //jpeg
	"Menu", //menu
	"Code",
	"LinkElement",
	"Flags",
	"StockID",
	"Pixbuf"
};

//-----------------------------------------------------------------------------

Glib::RefPtr<Gdk::Pixbuf> getPointIcon(int type) {
	static const char *names[] = {"sc_func", "sc_event", "sc_var", "sc_prop"};
	static Glib::RefPtr<Gdk::Pixbuf> pIcons[4];

	int i = type - 1;
	if(!pIcons[i])
		pIcons[i] = Gdk::Pixbuf::create_from_file(dataDir + ICONS_PATH + names[i] + ".png");
	return pIcons[i];
}

ustring databaseFile;
ustring dataDir;
ustring homeDir;

void initDirs() {

#ifdef G_OS_WIN32
	dataDir = ustring(g_get_current_dir()) + G_DIR_SEPARATOR_S;
#else
	#ifdef G_OS_MACOS
		dataDir = ustring(g_get_current_dir()) + G_DIR_SEPARATOR_S;
	#else
		dataDir = "/usr/share/hiasm/";
	#endif
#endif

	homeDir = ustring(g_get_home_dir()) + G_DIR_SEPARATOR_S".hiasm"G_DIR_SEPARATOR_S;
	if(!file_test(homeDir, FILE_TEST_IS_DIR)) {
		g_mkdir_with_parents(homeDir.c_str(), 0777);
		DEBUG_MSG("Create dir " << homeDir.c_str())
	}
	databaseFile = homeDir + DATABASE_FILE;
	if(!file_test(databaseFile, FILE_TEST_EXISTS)) {
		GFile *f2 = g_file_new_for_path(databaseFile.c_str());
		ustring locDB = dataDir + INT_PATH""DATABASE_FILE;
		GFile *f1 = g_file_new_for_path(locDB.c_str());
		DEBUG_MSG("Copy " << locDB.c_str() << " to " << databaseFile.c_str())
		g_file_copy(f1, f2, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL);
	}
}

void parseHintName(const ustring &text, ustring &name, ustring &hint) {
	int p = text.find("=");
	if(p == -1) {
		name = text;
		hint = ustring();
	}
	else {
		name = ustring(text, 0, p);
		hint = ustring(text, p + 1);
	}
}
