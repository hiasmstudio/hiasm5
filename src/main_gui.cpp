/*! \mainpage HiAsm Studio 5
 *
 * \section intro_sec Introduction
 *
 * <IMG src="http://upload.wikimedia.org/wikipedia/commons/thumb/f/fa/Hilogo.jpg/64px-Hilogo.jpg">
 * <p>HiAsm is a practical example of the implementation of the approach model-oriented architecture,
 * also known as "the development of the model. The significance of this approach is abstracted
 * from the platforms and architectures hardware vendors and system software (mathematical) software.</p>
 *
 * <p>When designing the user does not require knowledge of programming languages and operating features
 * of operating system that allows you to create applications by controlling their model using an
 * intuitive graphical interface.</p>
 *
 * \section install_sec Installation
 *
 */

#include <gtkmm-3.0/gtkmm/stylecontext.h>
#include <gtkmm-2.4/gtkmm/viewport.h>

#include "main_gui.h"
#include "MainDataBase.h"
#include "RecentFileManager.h"
#include "SettingsManager.h"
#include "CodeGen.h"
#include "Element.h"
#include "Compiler.h"

const char *appName = "HiAsm Studio 5";

Window *form;

TypePixbuf _stockRender(const ustring &name) {
	return form->get_style_context()->lookup_icon_set(name)->render_icon_pixbuf(form->get_style_context(), ICON_SIZE_MENU);
}

MainGUI::MainGUI():Window() {
	init();
}

void MainGUI::init() {
	TRACE_PROC

	form = this;
	stockRender = _stockRender;

	initDirs();

	splash.view();
//	sleep(5);

	rfMan.load();

	/*
	DEBUG_MSG(g_get_current_dir())
	DEBUG_MSG(g_get_home_dir())
	DEBUG_MSG(g_get_tmp_dir())
	DEBUG_MSG(g_get_user_data_dir())
	DEBUG_MSG(g_get_user_config_dir())
	DEBUG_MSG(g_get_user_cache_dir())
	DEBUG_MSG(g_get_system_config_dirs()[0])
	DEBUG_MSG(g_get_system_data_dirs()[0])
	//*/

	set_title(appName);

	DEBUG_MSG("open hiasm.db...")
	mdb.open();
	DEBUG_MSG("load SettingsManager...")
	SettingsManager::init();
	DEBUG_MSG("load CompilerCollection...")
	compilerSet = new CompilerCollection();
	DEBUG_MSG("load PackCollection...")
	packSet = new PackCollection();
	DEBUG_MSG("load CommandManager...")
	cmdMan = new CommandManager();
	DEBUG_MSG("load WindowManager...")
	wmp = new WindowManager();
	DEBUG_MSG("load Palette...")
	pal = new Palette();

	//---- temp
	set_position(WIN_POS_CENTER);
	set_default_size(1000, 700);

	DEBUG_MSG("load menus...")
	main_tools = cmdMan->createToolbar(MENUNAME_TOOLBAR);
	MenuBar *menu = cmdMan->createMenuBar(MENUNAME_MAIN);
	cmdMan->attachMenu(CMD_BUILD, compilers);

	ScrolledWindow *sc_debug = new ScrolledWindow();
	sc_debug->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	sc_debug->add(debug);
	sc_debug->set_size_request(100, 70);

	HBox *tbox = new HBox();
	tbox->pack_start(*menu, PACK_SHRINK);
	tbox->add(*main_tools);
	//box1.pack_start(main_tools, PACK_SHRINK);
	//box1.add(*menu);
	box1.pack_start(*tbox, PACK_SHRINK);

	box1.add(pan1);
	pan1.pack1(pan2, true, true);
	pan1.pack2(*sc_debug, false, false);

	sc_debug = new ScrolledWindow();
	sc_debug->set_policy(POLICY_NEVER, POLICY_AUTOMATIC);
	sc_debug->add(*pal);
	sc_debug->set_size_request(110, 120);

	pan2.pack1(*sc_debug, false, false);
	pan2.add2(pan3);
	pan3.pack1(*wmp, true, true);
	sc_debug = new ScrolledWindow();
	sc_debug->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	sc_debug->set_size_request(150, 100);

	propsFix.add(props);
	propsFix.signal_button_press_event().connect(sigc::mem_fun(props, &PropEditor::on_button_press_event));
	propsFix.signal_button_release_event().connect(sigc::mem_fun(props, &PropEditor::on_button_release_event));
	propsFix.signal_motion_notify_event().connect(sigc::mem_fun(props, &PropEditor::on_motion_notify_event));
	VBox *vb_props_points = new VBox();
	vb_props_points->pack_start(propsFix, true, true);
	vb_props_points->pack_end(pointsList, true, true);

	sc_debug->add(*vb_props_points);
	sc_debug->signal_size_allocate().connect(sigc::mem_fun(*this, &MainGUI::props_on_size_allocate));
	
	VPaned *vp = new VPaned();
	VBox *vb = new VBox();
	prop_tools = new Toolbar();
	fillPropTools();
	prop_tools->set_toolbar_style(TOOLBAR_ICONS);
	vb->pack_start(*prop_tools, false, false);
	vb->pack_end(*sc_debug, true, true);
	vp->pack1(*vb, true, true);
	propInfo.set_size_request(100, 50);
	propInfo.set_editable(false);
	propInfo.set_wrap_mode(WRAP_WORD);

	DEBUG_MSG("load gtk theme...")
	gtk_rc_parse((dataDir + "gtkrc").c_str());
//	propInfo.get_style()->set_bg(STATE_NORMAL, get_style()->get_bg(STATE_NORMAL));

	vp->pack2(propInfo, false, false);
	pan3.pack2(*vp, false, false);

	pal->on_element_select += this;
	props.on_change_property += this;
	props.on_select_property += this;
	props.on_check_property += this;
	props.on_edit_property += this;
	wmp->on_tab_change += this;
	wmp->on_tab_close += this;
	cmdMan->on_command += this;
	cmdMan->on_cmd_enabled += this;
	cgt_on_debug += this;

	box1.show_all();
	pointsList.hide();

	add(box1);

	set_icon(Gdk::Pixbuf::create_from_file(dataDir + INT_PATH"hiasm.png"));

	curSelMan = NULL;
	sde = NULL;
	showSystemProps = false;

	lightColor = pal->hint.get_style_context()->get_color(STATE_FLAG_NORMAL);
	darkColor = pal->hint.get_style_context()->get_border_color(STATE_FLAG_NORMAL);
	hintColor = pal->hint.get_style_context()->get_background_color(STATE_FLAG_NORMAL);

	// --- points list initialize
	pointsListStore = Gtk::ListStore::create(pointsListColumns);

	pointsList.set_model(pointsListStore);
	pointsList.append_column_editable("check", pointsListColumns.visible);
	pointsList.append_column("icon", pointsListColumns.icon);
	pointsList.append_column("param", pointsListColumns.param);
	pointsList.set_headers_visible(false);
	pointsList.signal_cursor_changed().connect(sigc::mem_fun(*this, &MainGUI::on_show_ep_row_activated));
	dynamic_cast<CellRendererToggle*>(pointsList.get_column_cell_renderer(0))->
				signal_toggled().connect(sigc::mem_fun(*this, &MainGUI::on_show_element_point));

	DEBUG_MSG("main class init successful")

	//load settings
	int size = 0;
	StringList list(homeDir + APP_SETTINGS);
	for(StringList::iterator line = list.begin(); line != list.end(); line++) {
		ustring l = *line;
		if(sscanf((*line).c_str(), "palette=%d", &size))
			pan2.set_position(size);
		else if(sscanf((*line).c_str(), "debug=%d", &size))
			pan1.set_position(size);
		else if(sscanf((*line).c_str(), "properties=%d", &size))
			pan3.set_position(size);
	}
}

MainGUI::~MainGUI() {
	TRACE_PROC
	mdb.close();

	//save settings
	pal->save();
	wmp->saveTabs();

	StringList list;
	list << ustring::compose("palette=%1", pan2.get_position());
	list << ustring::compose("debug=%1", pan1.get_position());
	list << ustring::compose("properties=%1", pan3.get_position());
	list.saveToFile(homeDir + APP_SETTINGS);
}

void MainGUI::on_show() {
	Gtk::Widget::on_show();

	if(!sysColors.load) sysColors.init(&props);

	if(!curList) {
		DEBUG_MSG("load CursorList...")
		curList = new CursorList(get_window()->get_display());
		DEBUG_MSG("load CursorList ok")
	}

	splash.hide();

	wmp->init();
	DEBUG_MSG("load ok.")
}

void MainGUI::callback(void *owner, CallbackType type, const void *data) {
//	std::cout << "Event receive: " << type << std::endl;

	switch(type) {
		case CBT_SELECT_ELEMENT_PALETTE:
			sde->beginOperation(MS_INSERT_ELEMENT, data);
			break;
		case CBT_END_OPERATION:
			pal->unSelect();
			break;
		case CBT_SDK_CHANGE_SELECT:
			fillProps((SelectManager *)owner);
			break;
		case CBT_CHANGE_PROPERTY:
			changeProps((const PropertyItem *)data);
			sde->changeScheme();
			break;
		case CBT_SELECT_PROPERTY:
			lastSelect = props.getPath();
			propInfo.get_buffer()->set_text(data ? ((PropertyItem *)data)->info : ustring());
			break;
		case CBT_CHECK_PROPERTY:
			if(((PropertyItem *)data)->checked)
				((ElementProperty *)((PropertyItem *)data)->data)->makePoint();
			else
				((ElementProperty *)((PropertyItem *)data)->data)->removePoint();
			sde->changeScheme();
			break;
		case CBT_EDIT_PROPERTY:
			editProps((const PropertyItem *)data);
			break;
		case CBT_CHANGE_SDK:
			if(((ChangeSDKInfo*)data)->change) {
				((ChangeSDKInfo*)data)->sdk->selMan->on_selection_change += this;
				fillProps(((ChangeSDKInfo*)data)->sdk->selMan);
			}
			else {
				((ChangeSDKInfo*)data)->sdk->selMan->on_selection_change -= this;
			}
			break;
		case CBT_TAB_CHANGE:
			if(sde) {
				sde->on_end_operation -= this;
				sde->csdk->selMan->on_selection_change -= this;
				sde->on_change_sdk -= this;
			}
			if(data && ((WM_Tab*)data)->type == WMT_SDK_EDITOR) {
				sde = ((WM_SDK_Editor*)data)->sde;
				sde->on_end_operation += this;
				sde->on_change_sdk += this;
				sde->csdk->selMan->on_selection_change += this;
				fillProps(sde->csdk->selMan);
				fillCompilers(sde->sdk->pack);

				pal->loadPack(sde->sdk->pack);
			}
			else {
				fillProps(NULL);
				pal->clear();
				sde = NULL;
			}
			set_title(ustring(appName) + (data ? (" [" + ((WM_Tab*)data)->fileName() + "]") : ustring()));
			break;
		case CBT_RUN_COMMAND:
			if(*(const ustring *)data == CMD_FULLSCREEN) {
				if(get_window()->get_state()&Gdk::WINDOW_STATE_FULLSCREEN)
					unfullscreen();
				else
					fullscreen();
			}
			else if(*(const ustring *)data == CMD_ADD_GROUP) {
				ElementCoreTransmitter *t = shaDlg.run("new_palette_group");
				TData dt;
				dt = t->run(dt);
				if(!dt.empty()) {
					pal->addGroup(dt.toStr());
					sde->sdk->pack->addGroup(pal->currentTab->caption, dt.toStr());
				}
			}

			break;
		case CBT_DEBUG_INFO:
			if(data) {
				if(*(const char*)data == '~')
					debug.text((const char*)data+1);
				else if( *(const char*)data == '!')
					debug.error((const char*)data+1);
				else if( *(const char*)data == '@')
					debug.info((const char*)data+1);
				else if( *(const char*)data == '#')
					debug.trace((const char*)data+1);
				else
					debug.text((const char*)data);
			}
			else
				debug.clear();
			break;
		case CBT_CMD_ENABLED:
			cmdMan->enable(CMD_ADD_GROUP);
			break;
		default:
			;
	}
}

void MainGUI::props_on_size_allocate(Allocation &alloc) {
	props.changeSize(alloc.get_width());
}

void MainGUI::on_prop_tools_click() {
	if(pointsList.get_visible()) {
		pointsList.hide();
		propsFix.show();
	}
	else {
		pointsList.show();
		propsFix.hide();
	}
}

void MainGUI::on_show_sysprops_click() {
	showSystemProps = !showSystemProps;
	fillProps(curSelMan);
}

void MainGUI::on_show_element_point(const Glib::ustring &path) {
	TreeModel::Path tp(path);
	TreeModel::Row r = *pointsListStore->get_iter(tp);

	ConfMethod *m = r[pointsListColumns.mtd];
	Element *e = curSelMan->getFirst();
	if(r[pointsListColumns.visible]) {
		e->addPoint(m->name, m->info, m->mType);
	}
	else {
		e->removePoint(e->getPointByName(m->name));
	}
	e->rePosPoints();
	e->invalidate();

	sde->changeScheme();
}

void MainGUI::on_show_ep_row_activated() {
	if(!curSelMan) return;

	TreeModel::Path p;
	TreeViewColumn *c;
	pointsList.get_cursor(p, c);

	TreeModel::Row r = *pointsListStore->get_iter(p);

	propInfo.get_buffer()->set_text(((ConfMethod *)r[pointsListColumns.mtd])->info);
}

void MainGUI::fillPropTools() {
	static const char *names[] = {"property", "points"};
	RadioButtonGroup grp;
	for(int i = 0; i < 2; i++) {
		RadioToolButton *btn = (i == 0) ? new RadioToolButton() : new RadioToolButton(grp);
		grp = btn->get_group();
		Image *img = new Image(Gdk::Pixbuf::create_from_file(dataDir + ICONS_PATH + names[i] + ".png"));
		btn->set_icon_widget(*img);
		if(i)
			btn->signal_clicked().connect(sigc::mem_fun(*this, &MainGUI::on_prop_tools_click));
		prop_tools->add(*btn);
	}
	SeparatorToolItem *sp = new SeparatorToolItem();
	prop_tools->add(*sp);

	ToggleToolButton *btn = new ToggleToolButton();
	Image *img = new Image(Gdk::Pixbuf::create_from_file(dataDir + ICONS_PATH + "switch.png"));
	btn->set_icon_widget(*img);
	btn->signal_clicked().connect(sigc::mem_fun(*this, &MainGUI::on_show_sysprops_click));
	prop_tools->add(*btn);
}

void updateDefaultFlag(PropertyItem *pi, ElementProperty *ep) {
	pi->flag |= PROP_FLG_NDEF;
	if(ep->isDefault())
		pi->flag ^= PROP_FLG_NDEF;
}

void MainGUI::changeProps(const PropertyItem *prop) {
	ElementProperty *ep = (ElementProperty *)prop->data;
	if(!ep) return;

	ep->change();
	updateDefaultFlag((PropertyItem *)prop, ep);
}

void MainGUI::editProps(const PropertyItem *prop) {
	switch(prop->type) {
		case data_pixbuf: {
			ElementCoreTransmitter *t = shaDlg.run("image_editor");
			TData dt((TypePixbuf*)prop->value);
			dt = t->run(dt);
			((ElementProperty *)prop->data)->setValue(dt.toPixbuf());
			break;
		}
		case data_str:
		case data_list: {
			ElementCoreTransmitter *t = shaDlg.run("text_editor");
			ustring s = *(ustring*)prop->value;
			/*
			if(prop->type == data_str) {
				int p = 0;
				while((p = s.find("\\n")) != -1)
					s = s.replace(p, 2, "\n");
			}
			*/
			TData dt(s);
			dt = t->run(dt);
			s = dt.toStr();
			/*
			if(prop->type == data_str) {
				int p = 0;
				while((p = s.find("\n")) != -1)
					s = s.replace(p, 1, "\\n");
			}
			*/
			((ElementProperty *)prop->data)->setValue(&s);
			break;
		}
		case data_array:
			arrEditor.edit(prop->value);
			break;
		default:;
	}
	updateDefaultFlag((PropertyItem *)prop, (ElementProperty *)prop->data);
	prop->parent->update();
}

void MainGUI::fillProps(SelectManager *selMan) {
	props.clear();
	propInfo.get_buffer()->set_text("");
	curSelMan = NULL;
	pointsListStore->clear();

	curSelMan = selMan;

	if(!selMan) return;

	if(selMan->elements.size() == 1) {
		Element *e = *selMan->elements.begin();
		PropertyItem *prop;
		PropertyGroup *group = NULL;

		if(showSystemProps && !e->props.empty())
			props.addProperty(new PropertyTitle("Element properties", "List of element properties"));
		for(ElementProperties::iterator ep = e->props.begin(); ep != e->props.end(); ep++) {
			switch((*ep)->type) {
				case data_int:
					prop = new PropertyInteger((*ep)->name, (*ep)->value);
					break;
				case data_stock:
					prop = new PropertyStock((*ep)->name, (*ep)->value);
					break;
				case data_str:
				case data_list:
					prop = new PropertyString((*ep)->name, (*ep)->value);
					break;
				case data_real:
					prop = new PropertyDouble((*ep)->name, (*ep)->value);
					break;
				case data_data:
					prop = new PropertyTData((*ep)->name, (*ep)->value);
					break;
				case data_combo:
				case data_comboEx:
					prop = new PropertyEnum((*ep)->name, (*ep)->value, (*ep)->list);
					break;
				case data_pixbuf:
					prop = new PropertyPixbuf((*ep)->name, (*ep)->value);
					break;
				case data_color:
					prop = new PropertyColor((*ep)->name, (*ep)->value);
					break;
				case data_array:
					prop = new PropertyArray((*ep)->name, (*ep)->value);
					break;
				case data_font:
					prop = new PropertyFont((*ep)->name, (*ep)->value);
					break;
				case data_flags:
					prop = new PropertyFlags((*ep)->name, (*ep)->value, (*ep)->list);
					break;
				case data_element: {
					ustring m = e->parent->getListOfManagers((*ep)->list), s(empty);
					if(!m.empty())
						s = s + "," + m;
					//s += "New...";
					prop = new PropertyEnumText((*ep)->name, (*ep)->value, s);
					break;
				}
				default:
					;
			}
			prop->info = (*ep)->info;
			prop->data = *ep;
			if((*ep)->group.empty())
				props.addProperty(prop);
			else {
				if(!group || (*ep)->group != group->name) {
					group = new PropertyGroup((*ep)->group);
					props.addProperty(group);
				}
				group->add(prop);
			}

			if(!(*ep)->isDefault())
				prop->flag |= PROP_FLG_NDEF;
			if((*ep)->flag & VF_DBLOPEN)
				prop->flag |= PROP_FLG_MAIN;
			if((*ep)->flag & VF_MAKEPOINT) {
				prop->flag |= PROP_FLG_CHECK;
				prop->checked = (*ep)->checkPoint();
			}
		}

		if(showSystemProps) {
			props.addProperty(new PropertyTitle("System properties", "List of internal system properties"));
			for(ElementProperties::iterator ep = e->sysProps.begin(); ep != e->sysProps.end(); ep++) {
				switch((*ep)->type) {
					case data_int:
						prop = new PropertyInteger((*ep)->name, (*ep)->value);
						break;
					case data_str:
					case data_list:
						prop = new PropertyString((*ep)->name, (*ep)->value);
						break;
					case data_real:
						prop = new PropertyDouble((*ep)->name, (*ep)->value);
						break;
					case data_color:
						prop = new PropertyColor((*ep)->name, (*ep)->value);
						break;
					case data_stock:
						prop = new PropertyStock((*ep)->name, (*ep)->value);
						break;
					default: ;
				}
				prop->data = *ep;
				prop->info = (*ep)->info;
				props.addProperty(prop);

				if(!(*ep)->isDefault())
					prop->flag |= PROP_FLG_NDEF;
			}
		}

		// fill points
		for(ConfDefMethods::iterator dm = e->tpl->conf->def.begin(); dm != e->tpl->conf->def.end(); dm++) {
			Gtk::TreeRow row = *(pointsListStore->append());
			row[pointsListColumns.visible] = e->points.getByName((*dm)->name);
			row[pointsListColumns.icon] = getPointIcon((*dm)->mType);
			row[pointsListColumns.param] = (*dm)->name;
			row[pointsListColumns.mtd] = *dm;
		}
	}
	props.selectPropertyByPath(lastSelect);
}

void MainGUI::fillCompilers(Pack *pack) {
	//compilers.items().clear();
	if(pack) {
		for(Pack::CompilerItems::iterator c = pack->compilers.begin(); c != pack->compilers.end(); c++) {
			MenuItem *m = new MenuItem(c->compiler->name);
			compilers.append(*m);
		}
	}
	compilers.show_all_children();
}
