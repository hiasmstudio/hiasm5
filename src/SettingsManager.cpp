/*
 * SettingsManager.cpp
 *
 *  Created on: Jul 15, 2010
 *      Author: dilma
 */

#include "SettingsManager.h"
#include "MainDataBase.h"
#include "Elements.h"
#include "SHADialogs.h"

SettingsManager *settingsMan = NULL;
std::map<int, void*> values;

SettingsManager::SettingsManager():on_change_settings(this, CBT_CHANGE_SETTINGS) {
	set_title("Settings manager");
	menu.set_size_request(160, 200);

	pFixed.add(props);
	pFixed.signal_size_allocate().connect(sigc::mem_fun(*this, &SettingsManager::props_on_size_allocate));
	pFixed.signal_button_press_event().connect(sigc::mem_fun(props, &PropEditor::on_button_press_event));
	pFixed.signal_button_release_event().connect(sigc::mem_fun(props, &PropEditor::on_button_release_event));
	pFixed.signal_motion_notify_event().connect(sigc::mem_fun(props, &PropEditor::on_motion_notify_event));
	pScroll.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	pScroll.add(pFixed);
//	pScroll.set_size_request(300, 200);

	info.set_size_request(100, 40);
	info.set_editable(false);
	info.set_wrap_mode(WRAP_WORD);

	hpaned.pack1(menu, false, false);
	hpaned.pack2(vpaned, true, true);
	vpaned.pack1(pScroll, true, true);
	vpaned.pack2(info, false, false);

	hpaned.show_all();
	get_vbox()->pack_start(hpaned, true, true);

	menuStore = Gtk::ListStore::create(menuColumns);
	menu.set_model(menuStore);
	menu.append_column("icon", menuColumns.icon);
	menu.append_column("name", menuColumns.name);
	menu.set_headers_visible(false);
	menu.signal_cursor_changed().connect(sigc::mem_fun(*this, &SettingsManager::on_show_ep_row_activated));

	add_button(Gtk::Stock::ADD, 4);
	add_button(Gtk::Stock::OPEN, 3);
	add_button(Gtk::Stock::SAVE, 2);
	add_button(Gtk::Stock::OK, 1);

	set_icon_from_file(dataDir + ICONS_PATH + "tools.png");

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
		void *tmp = values[param.id];
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
			p->data = (void*)param.id;
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
		switch(settingsMan->run()) {
			case 4: {
				ElementCoreTransmitter *t = shaDlg.run("new_param");
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
			mdb.save_settings_value((long)((PropertyItem*)data)->data, v);
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
	DataType __types[] = {data_null, data_int, data_real, data_str, data_color, data_combo};
	void *id = mdb.begin_read_settings_value();
	ParamInfo param;
	while(mdb.read_settings_value(id, param)) {
		void *v = DataValue::createValue(__types[param.type]);
		DataValue::parse(param.value, __types[param.type], v);
		values.insert(std::pair<int, void*>(param.id, v));
	}
	mdb.end_read(id);
}

int SettingsManager::readInt(SettingsIDS id) { return *(int*)values[id]; }
double SettingsManager::readReal(SettingsIDS id) { return *(double*)values[id]; }
ustring &SettingsManager::readStr(SettingsIDS id) { return *(ustring*)values[id]; }
TypeColor SettingsManager::readColor(SettingsIDS id) { return *(TypeColor*)values[id]; }

ustring SettingsManager::readProjectsDir() {
	ustring pdir(readStr(IDS_PROJECTS_DIR));
	int p = pdir.find("%home%");
	if(p != -1)
		pdir.replace(p, 6, g_get_home_dir());

	if(!file_test(pdir, FILE_TEST_IS_DIR))
		g_mkdir_with_parents(pdir.c_str(), 0777);

	return pdir;
}
