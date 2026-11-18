/*
 * share.cpp
 *
 *  Created on: 14.05.2010
 *      Author: dilma
 */

#include "share.h"

#ifdef G_OS_WIN32
#include <glib-2.0/glib/gwin32.h>
#endif

static gchar _version_[32] = {0};

gchar *HIASM_VERSION() {
	if(!_version_[0])
		sprintf(_version_, "%d.%d.%d.%d", HIASM_VERSION_MAJOR, HIASM_VERSION_MINOR,
			HIASM_VERSION_BUILD, HIASM_VERSION_TWEAK);
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

namespace {
// Default Windows system palette (COLOR_SCROLLBAR..COLOR_MENUBAR) as $RRGGBB.
// Used only to preview Delphi system colors; CodeGen gets the TColor itself.
const unsigned systemColorPreview[] = {
	0xC8C8C8, 0x000000, 0x99B4D1, 0xBFCDDB, 0xF0F0F0, 0xFFFFFF, 0x646464, 0x000000,
	0x000000, 0x000000, 0xB4B4B4, 0xF4F7FC, 0xABABAB, 0x0078D7, 0xFFFFFF, 0xF0F0F0,
	0xA0A0A0, 0x6D6D6D, 0x000000, 0x000000, 0xFFFFFF, 0x696969, 0xE3E3E3, 0x000000,
	0xFFFFE1, 0xB5B5B5, 0x0066CC, 0xB9D1EA, 0xD7E4F2, 0x3399FF, 0xF0F0F0,
};
}

TypeColor TypeColor::fromDelphi(int value) {
	unsigned rgb = value & 0xFFFFFF;	// $00BBGGRR, also VCL ColorToRGB for clNone/clDefault
	if(value < 0) {	// system color: GetSysColor(value and $FF), as in VCL/KOL
		const unsigned index = value & 0xFF;
		const unsigned preview = index < G_N_ELEMENTS(systemColorPreview) ? systemColorPreview[index] : 0x808080;
		rgb = ((preview & 0xFF) << 16) | (preview & 0xFF00) | (preview >> 16);
	}
	TypeColor color;
	color.set_rgba_u((rgb & 0xFF) * 257, ((rgb >> 8) & 0xFF) * 257, ((rgb >> 16) & 0xFF) * 257);
	color.delphi = true;
	color.delphiValue = value;
	color.loaded = *color.gobj();
	return color;
}

int TypeColor::toDelphi() const {
	if(delphi && gdk_rgba_equal(&loaded, gobj())) return delphiValue;
	return (get_red_u() >> 8) | ((get_green_u() >> 8) << 8) | ((get_blue_u() >> 8) << 16);
}

bool TypeColor::operator==(const TypeColor &other) const {
	if(delphi || other.delphi) return toDelphi() == other.toDelphi();
	return static_cast<const Gdk::RGBA&>(*this) == static_cast<const Gdk::RGBA&>(other);
}

char *getTok(char **buf, char tok) {
	if(**buf == '\0')
		return *buf;

	char *s = *buf;
	while(**buf != tok && **buf)
		(*buf)++;
	if(!**buf)
		return s;
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
bool initializedUserProfile = false;

void initDirs() {

#ifdef G_OS_WIN32
	gchar *workingDir = g_get_current_dir();
	dataDir = ustring(workingDir) + G_DIR_SEPARATOR_S;
	g_free(workingDir);
	if(!file_test(dataDir + INT_PATH + DATABASE_FILE, FILE_TEST_EXISTS)) {
		gchar *exe_dir = g_win32_get_package_installation_directory_of_module(NULL);
		if(exe_dir) {
			dataDir = ustring(exe_dir) + G_DIR_SEPARATOR_S;
			g_free(exe_dir);
		}
	}
#else
	#ifdef G_OS_MACOS
		gchar *workingDir = g_get_current_dir();
		dataDir = ustring(workingDir) + G_DIR_SEPARATOR_S;
		g_free(workingDir);
	#else
		dataDir = "/usr/share/hiasm/";
		// Portable builds keep resources next to the executable, independent
		// of the caller's working directory. Keep the packaged Unix fallback.
		gchar *executable = g_file_read_link("/proc/self/exe", nullptr);
		if(executable) {
			gchar *directory = g_path_get_dirname(executable);
			const ustring candidate = ustring(directory) + G_DIR_SEPARATOR_S;
			if(file_test(candidate + INT_PATH + DATABASE_FILE, FILE_TEST_IS_REGULAR)) dataDir = candidate;
			g_free(directory);
			g_free(executable);
		}
	#endif
#endif
	if(const gchar *overrideDir = g_getenv("HIASM_DATA_DIR")) {
		gchar *absolute = g_canonicalize_filename(overrideDir, nullptr);
		const ustring candidate = ustring(absolute) + G_DIR_SEPARATOR_S;
		g_free(absolute);
		if(file_test(candidate + INT_PATH + DATABASE_FILE, FILE_TEST_IS_REGULAR)) dataDir = candidate;
		else std::cerr << "HIASM_DATA_DIR has no int/hiasm.db: " << candidate << std::endl;
	}

	homeDir = ustring(g_get_home_dir()) + G_DIR_SEPARATOR_S".hiasm" G_DIR_SEPARATOR_S;
	if(!file_test(homeDir, FILE_TEST_IS_DIR)) {
		g_mkdir_with_parents(homeDir.c_str(), 0777);
		DEBUG_MSG("Create dir " << homeDir.c_str())
	}
	databaseFile = homeDir + DATABASE_FILE;
	if(!file_test(databaseFile, FILE_TEST_EXISTS)) {
		GFile *f2 = g_file_new_for_path(databaseFile.c_str());
		ustring locDB = dataDir + INT_PATH"" DATABASE_FILE;
		GFile *f1 = g_file_new_for_path(locDB.c_str());
		DEBUG_MSG("Copy " << locDB.c_str() << " to " << databaseFile.c_str())
		GError *error = nullptr;
		initializedUserProfile = g_file_copy(f1, f2, G_FILE_COPY_NONE, NULL, NULL, NULL, &error);
		if(!initializedUserProfile) {
			std::cerr << "Unable to initialize settings database: " << error->message << std::endl;
			g_clear_error(&error);
		}
		g_object_unref(f1);
		g_object_unref(f2);
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
