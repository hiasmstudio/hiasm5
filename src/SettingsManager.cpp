/*
 * SettingsManager.cpp
 *
 *  Created on: Jul 15, 2010
 *      Author: dilma
 */

#include <cstdint>

#include "SettingsManager.h"
#include "MainDataBase.h"
#include "Elements.h"
#include "SHADialogs.h"

SettingsManager *settingsMan = NULL;
std::map<int, void*> values;
static std::map<int, DataType> valueTypes;
namespace {
int themeMode = 0; // system, light, dark
bool formBindings[3] = {false, false, false};
const char *bindingKeys[] = {"Edges", "Center", "Padding"};
bool systemDarkTheme() {
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
    DWORD light = 1, size = sizeof(light);
    if(HMODULE module = LoadLibraryW(L"advapi32.dll")) {
        using ReadValue = LSTATUS (WINAPI *)(HKEY, LPCWSTR, LPCWSTR, DWORD, LPDWORD, PVOID, LPDWORD);
        auto read = reinterpret_cast<ReadValue>(GetProcAddress(module, "RegGetValueW"));
        if(read) read(HKEY_CURRENT_USER, L"Software\\Microsoft\\Windows\\CurrentVersion\\Themes\\Personalize",
            L"AppsUseLightTheme", RRF_RT_REG_DWORD, nullptr, &light, &size);
        FreeLibrary(module);
    }
    return !light;
#else
    const gchar *theme = g_getenv("GTK_THEME");
    return theme && strstr(theme, "dark");
#endif
}
void applyAppearance() {
#ifdef HIASM_GTK4
    themeSetDark(themeMode == 2 || (themeMode == 0 && systemDarkTheme()));
#endif
}
}

int SettingsManager::appearanceMode() { return themeMode; }
bool SettingsManager::formBinding(FormBinding mode) {
    return mode >= BindEdges && mode <= BindPadding && formBindings[mode];
}
void SettingsManager::setFormBinding(FormBinding mode, bool enabled) {
    if(mode < BindEdges || mode > BindPadding) return;
    formBindings[mode] = enabled;
    GKeyFile *file = g_key_file_new();
    g_key_file_load_from_file(file, (homeDir + "appearance.ini").c_str(), G_KEY_FILE_NONE, nullptr);
    g_key_file_set_boolean(file, "FormEditor", bindingKeys[mode], enabled);
    gsize size = 0;
    gchar *data = g_key_file_to_data(file, &size, nullptr);
    GError *error = nullptr;
    if(!g_file_set_contents((homeDir + "appearance.ini").c_str(), data, size, &error)) {
        std::cerr << "Unable to save form bindings: " << (error ? error->message : "unknown error") << std::endl;
        g_clear_error(&error);
    }
    g_free(data); g_key_file_unref(file);
}
void SettingsManager::loadAppearance() {
    GKeyFile *file = g_key_file_new();
    GError *error = nullptr;
    themeMode = 0;
    if(g_key_file_load_from_file(file, (homeDir + "appearance.ini").c_str(), G_KEY_FILE_NONE, &error))
        themeMode = g_key_file_get_integer(file, "Interface", "Theme", nullptr);
    g_clear_error(&error);
    g_key_file_unref(file);
    if(themeMode < 0 || themeMode > 2) themeMode = 0;
    applyAppearance();
}
void SettingsManager::setAppearance(int mode) {
    if(mode < 0 || mode > 2) return;
    themeMode = mode;
    applyAppearance();
    GKeyFile *file = g_key_file_new();
    g_key_file_load_from_file(file, (homeDir + "appearance.ini").c_str(), G_KEY_FILE_NONE, nullptr);
    g_key_file_set_integer(file, "Interface", "Theme", mode);
    gsize size = 0;
    gchar *data = g_key_file_to_data(file, &size, nullptr);
    GError *error = nullptr;
    if(!g_file_set_contents((homeDir + "appearance.ini").c_str(), data, size, &error)) {
        std::cerr << "Unable to save appearance: " << (error ? error->message : "unknown error") << std::endl;
        g_clear_error(&error);
    }
    g_free(data);
    g_key_file_unref(file);
}

SettingsManager::SettingsManager():on_change_settings(this, CBT_CHANGE_SETTINGS) {
	set_title(ui_tr("Settings manager"));
#ifdef HIASM_GTK4
    appearanceLabel.set_text(ui_tr("Interface theme"));
    appearanceChoice.append(ui_tr("System"));
    appearanceChoice.append(ui_tr("Light"));
    appearanceChoice.append(ui_tr("Dark"));
    appearanceChoice.set_active(themeMode);
    appearanceRow.append(appearanceLabel);
    appearanceRow.append(appearanceChoice);
    get_content_area()->append(appearanceRow);
    appearanceChoice.signal_changed().connect([this]() { setAppearance(appearanceChoice.get_active_row_number()); });
    languageLabel.set_text(ui_tr("Language"));
    languageChoice.append("Русский");
    languageChoice.append("English");
    languageChoice.set_active(ui_language() == "en" ? 1 : 0);
    languageRow.append(languageLabel);
    languageRow.append(languageChoice);
    get_content_area()->append(languageRow);
    languageChoice.signal_changed().connect([this]() {
        if(mdb.setLanguage(languageChoice.get_active_row_number() == 1 ? "en" : "ru"))
            info.get_buffer()->set_text(ui_tr("Restart HiAsm to apply the language."));
    });
#endif
	menu.set_size_request(160, 200);

	pFixed.add(props);
	connect_size_allocate(pFixed, sigc::mem_fun(*this, &SettingsManager::props_on_size_allocate));
	connect_button_press(pFixed, sigc::mem_fun(props, &PropEditor::on_button_press_event));
	pFixed.signal_button_release_event().connect(sigc::mem_fun(props, &PropEditor::on_button_release_event));
	connect_motion_notify(pFixed, sigc::mem_fun(props, &PropEditor::on_motion_notify_event));
	pScroll.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrolled_set_child(pScroll, pFixed);
//	pScroll.set_size_request(300, 200);

	info.set_size_request(100, 40);
	info.set_editable(false);
	info.set_wrap_mode(WRAP_WORD);

	hpaned.pack1(menu, false, false);
	hpaned.pack2(vpaned, true, true);
	vpaned.pack1(pScroll, true, true);
	vpaned.pack2(info, false, false);

	hpaned.show();
	box_pack_start(*get_content_area(), hpaned, true, true, 0);

	menuStore = Gtk::ListStore::create(menuColumns);
	menu.set_model(menuStore);
	menu.append_column("icon", menuColumns.icon);
	menu.append_column("name", menuColumns.name);
	menu.set_headers_visible(false);
	menu.signal_cursor_changed().connect(sigc::mem_fun(*this, &SettingsManager::on_show_ep_row_activated));

	add_button(Stock::ADD, 4);
	add_button(Stock::OPEN, 3);
	add_button(Stock::SAVE, 2);
	add_button(Stock::OK, 1);

#ifndef HIASM_GTK4
	set_icon_from_file(dataDir + ICONS_PATH + "tools.png");
#endif

	void *id = mdb.begin_read_settings(0);
	ParamInfo param;
	while(mdb.read_settings(id, param)) {
		Gtk::TreeRow row = *(menuStore->append());
		row[menuColumns.name] = param.title;
		row[menuColumns.info] = param.info;
		row[menuColumns.icon] = Gdk::Pixbuf::create_from_file(dataDir + ICONS_PATH + param.list + ".png");
		row[menuColumns.id] = param.id;
	}
	mdb.end_read(id);

	props.on_change_property += this;
	props.on_select_property += this;
}

void SettingsManager::fillProps(int pid, PropertyItem *group) {
	void *id = mdb.begin_read_settings(pid);
	ParamInfo param;
	PropertyItem *p;
	while(mdb.read_settings(id, param)) {
		p = NULL;
		auto found = values.find(param.id);
		void *tmp = found == values.end() ? nullptr : found->second;
		if(param.type != 0 && !tmp) continue;
		switch(param.type) {
			case 1: p = new PropertyInteger(param.title, tmp); break;
			case 2:	p = new PropertyDouble(param.title, tmp); break;
			case 3:	p = new PropertyString(param.title, tmp); break;
			case 4:	p = new PropertyColor(param.title, tmp); break;
			case 5:	p = new PropertyEnum(param.title, tmp, param.list); break;
			case 0:
				p = new PropertyGroup(param.title);
				fillProps(param.id, p);
				break;
		}
		if(p) {
			p->data = reinterpret_cast<void *>(static_cast<intptr_t>(param.id));
			p->info = param.info;
			if(group)
				group->add(p);
			else
				props.addProperty(p);
		}
	}
	mdb.end_read(id);
}

void SettingsManager::props_on_size_allocate(Allocation &alloc) {
	props.changeSize(alloc.get_width());
}

void SettingsManager::on_show_ep_row_activated() {
	TreeModel::Path p;
	TreeViewColumn *c;
	menu.get_cursor(p, c);

	TreeModel::Row r = *menuStore->get_iter(p);

	info.get_buffer()->set_text(r[menuColumns.info]);

	props.clear();
	fillProps(r[menuColumns.id]);
}

void SettingsManager::show() {
	if(!settingsMan)
		settingsMan = new SettingsManager();
	for(bool ok = true; ok;)
		switch(dialog_run(*settingsMan)) {
			case 4: {
				ElementCoreTransmitter *t = shaDlg.run("new_param");
				if(!t) {
					DEBUG_MSG("Settings dialog scheme new_param.sha is unavailable")
					ok = false;
					break;
				}
				TData dt(databaseFile);
				dt = t->run(dt);

				break;
			}
			default: ok = false;
		}
	settingsMan->hide();
}

void SettingsManager::callback(void *owner, CallbackType type, const void *data) {
	switch(type) {
		case CBT_CHANGE_PROPERTY: {
			ustring v;
			if(((PropertyItem*)data)->type == data_str)
				v = *(ustring*)((PropertyItem*)data)->value;
			else
				v = DataValue::serialize(((PropertyItem*)data)->type, ((PropertyItem*)data)->value);
			mdb.save_settings_value((long)(intptr_t)((PropertyItem*)data)->data, v);
			break;
		}
		case CBT_SELECT_PROPERTY:
			if(data)
				info.get_buffer()->set_text(((PropertyItem*)data)->info);
			break;
		default:;
	}
}

void SettingsManager::init() {
    GKeyFile *file = g_key_file_new();
    g_key_file_load_from_file(file, (homeDir + "appearance.ini").c_str(), G_KEY_FILE_NONE, nullptr);
    for(int i = 0; i < 3; ++i) formBindings[i] = g_key_file_get_boolean(file, "FormEditor", bindingKeys[i], nullptr);
    g_key_file_unref(file);
	const DataType types[] = {data_null, data_int, data_real, data_str, data_color, data_combo};
	void *id = mdb.begin_read_settings_value();
	ParamInfo param;
	while(mdb.read_settings_value(id, param)) {
		if(param.type <= 0 || param.type >= int(sizeof(types) / sizeof(types[0]))) {
			std::cerr << "Invalid setting type " << param.type << " for id " << param.id << std::endl;
			continue;
		}
		const DataType type = types[param.type];
		void *v = DataValue::createValue(type);
		// Settings strings are raw DB text, not serialized scheme properties.
		if(type == data_str) *(ustring*)v = param.value;
		else if(!DataValue::parse(param.value, type, v))
			std::cerr << "Invalid value for setting " << param.id << std::endl;
		auto found = values.find(param.id);
		if(found != values.end()) {
			if(valueTypes[param.id] == type) {
				DataValue::copyValue(type, found->second, v);
				DataValue::destroyValue(type, v);
				continue;
			}
			DataValue::destroyValue(valueTypes[param.id], found->second);
		}
		values[param.id] = v;
		valueTypes[param.id] = type;
	}
	mdb.end_read(id);
}

int SettingsManager::readInt(SettingsIDS id) {
	auto item = values.find(id);
	return item != values.end() && (valueTypes[id] == data_int || valueTypes[id] == data_combo) ? *(int*)item->second : 0;
}
double SettingsManager::readReal(SettingsIDS id) {
	auto item = values.find(id);
	return item != values.end() && valueTypes[id] == data_real ? *(double*)item->second : 0;
}
ustring &SettingsManager::readStr(SettingsIDS id) {
	static ustring empty;
	auto item = values.find(id);
	return item != values.end() && valueTypes[id] == data_str ? *(ustring*)item->second : empty;
}
TypeColor SettingsManager::readColor(SettingsIDS id) {
	auto item = values.find(id);
	return item != values.end() && valueTypes[id] == data_color ? *(TypeColor*)item->second : TypeColor();
}

ustring SettingsManager::readProjectsDir() {
	ustring pdir(readStr(IDS_PROJECTS_DIR));
	int p = pdir.find("%home%");
	if(p != -1)
		pdir.replace(p, 6, g_get_home_dir());

	if(!file_test(pdir, FILE_TEST_IS_DIR))
		g_mkdir_with_parents(pdir.c_str(), 0777);

	return pdir;
}
