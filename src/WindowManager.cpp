/*
 * WindowManager.cpp
 *
 *  Created on: 11.05.2010
 *      Author: dilma
 */

#include "WindowManager.h"
#include "MainDataBase.h"
#include "commands.h"
#include "SHADialogs.h"
#include "SettingsManager.h"
#include "RecentFileManager.h"
#include "main_gui.h"

SI_ImageMenuItem::SI_ImageMenuItem(Element *e, int count):
	ImageMenuItem(e->tpl->name + " (" + int_to_str(count) + ")")
{
	element = e;
	// TODO remove?
	Image *img = new Image(e->tpl->image);
	set_image(*img);
	show();
}

void SI_ImageMenuItem::on_activate() {
	SelectManager *sm = element->parent->selMan;
	std::list<Element *> list;
	for(ElementsList::iterator e = sm->elements.begin(); e != sm->elements.end(); e++)
		if((*e)->tpl == element->tpl)
			list.push_back(*e);
	sm->clear();

	for(std::list<Element *>::iterator e = list.begin(); e != list.end(); e++)
		sm->add(*e);
}

//------------------------------------------------------------------------------

void WM_Tab::setChanged(bool value) {
	if(changed != value) {
		changed = value;
		parent->changeTab(this);
	}
}

bool WM_Tab::close() {
	if(!changed)
		return true;

	MessageDialog msg(ustring("File ") + fileName() + " has not been saved! Save project before close?", false, MESSAGE_QUESTION, BUTTONS_YES_NO);
	msg.set_title("Warning");
	msg.add_button(Stock::CANCEL, 0);

	int i = msg.run();
	if(i == -8) {
		processCommand(CMD_SAVE, "");
	}

	return i;
}

//------------------------------------------------------------------------------

class MEvent : public EventBox {
	private:
		gdouble sx, sy;
		gdouble cx, cy;
		guint timerID;
		guint button;

		static gboolean lo_timer_proc(void *data) {
			((MEvent*)data)->move();
			return false;
		}
		int get_hscroll(ScrolledWindow *sc) {
			GtkRange* sb = (GtkRange*)gtk_scrolled_window_get_hscrollbar(const_cast< GtkScrolledWindow* >(sc->gobj()));
			return gtk_range_get_value(sb);
		}
		void set_hscroll(ScrolledWindow *sc, int value) {
			GtkRange* sb = (GtkRange*)gtk_scrolled_window_get_hscrollbar(const_cast< GtkScrolledWindow* >(sc->gobj()));
			gtk_range_set_value(sb, value);
		}
		int get_vscroll(ScrolledWindow *sc) {
			GtkRange* sb = (GtkRange*)gtk_scrolled_window_get_vscrollbar(const_cast< GtkScrolledWindow* >(sc->gobj()));
			return gtk_range_get_value(sb);
		}
		void set_vscroll(ScrolledWindow *sc, int value) {
			GtkRange* sb = (GtkRange*)gtk_scrolled_window_get_vscrollbar(const_cast< GtkScrolledWindow* >(sc->gobj()));
			gtk_range_set_value(sb, value);
		}
	public:
		double dx, dy;

		MEvent() { timerID = 0; }

		virtual bool on_button_press_event(GdkEventButton* event) {
			//DEBUG_MSG("BUTTON - DOWN")
			button = event->button;
			if(button == BTN_MIDDLE) {
				ScrolledWindow *sc = (ScrolledWindow *)get_child();
				dx = get_hscroll(sc);
				dy = get_vscroll(sc);
				sx = event->x - dx;
				sy = event->y - dy;
			}
			return true;
		}
		virtual bool on_button_release_event(GdkEventButton* event) {
			//DEBUG_MSG("BUTTON - UP")
			return true;
		}
		virtual bool on_motion_notify_event(GdkEventMotion* event) {
			//DEBUG_MSG("BUTTON - MOVE")
			if(button == BTN_MIDDLE) {
				ScrolledWindow *sc = (ScrolledWindow *)get_child();
				cx = event->x - get_hscroll(sc);
				cy = event->y - get_vscroll(sc);
				//printf("cx = %f\n", event->x);
				if(timerID) return true;

				timerID = gdk_threads_add_timeout(2, lo_timer_proc, this);
				//ScrolledWindow *sc = (ScrolledWindow *)get_child();
				//sc->get_vscrollbar()->set_value(dy + cy - sc->get_vscrollbar()->get_value() - sy);
				//sc->get_hscrollbar()->set_value(dx + cx - sc->get_hscrollbar()->get_value() - sx);
			}
			return true;
		}
		void move() {
			//printf("%f : %f\n",dx, cx - sx);
			ScrolledWindow *sc = (ScrolledWindow *)get_child();
			set_hscroll(sc, dx + sx - cx);
			set_vscroll(sc, dy + sy - cy);
			g_source_remove(timerID);
			timerID = 0;
		}
};

Widget *WM_SDK_Editor::open(const ustring &file_name, bool asNew) {
	TRACE_PROC
	sde = new SDK_Editor();
	sde->load(file_name);
	if(asNew) {
		ustring fn;
		int i = 1;
		ustring mask = SettingsManager::readStr(IDS_MASK_OF_NAME), m;
		int u = mask.find("%name%");
		if(u != -1) {
			mask.replace(u, 6, g_get_user_name());
		}
		int p = mask.find("%num%");
		if(p != -1)
			while(file_test(fn = (m = mask).replace(p, 5, int_to_str(i)), FILE_TEST_EXISTS) || parent->checkProjectName(Glib::filename_display_basename(fn)))
				i++;
		sde->sdk->fileName = fn;
	}

	sde->csdk->selMan->on_selection_change += this;

	sde->on_popup_menu += this;
	sde->on_key_down += this;
	sde->sdk->on_run_state += this;
	sde->on_change_sdk += this;
	sde->on_change_scheme += this;

	sdkPath = new Label(sde->sdk->getPath());
	sdkPath->set_alignment(0, 0);

	box = new VBox();
	box->add(*sde);

	scroll.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scroll.add(*box);
	MEvent *ebs = new MEvent();
	ebs->add(scroll);

	box = new VBox();
	box->add(*ebs);

	elementsList = new Label("(none)");
	elementsList->set_size_request(150, 0);
	EventBox *eb = new EventBox();
	eb->add(*elementsList);
	eb->signal_button_release_event().connect(sigc::mem_fun(*this, &WM_SDK_Editor::on_sel_info_click));

	zoomSlider.set_draw_value(true);
	zoomSlider.set_value_pos(POS_LEFT);
	zoomSlider.set_size_request(130, 16);
	zoomSlider.set_value(100);
	zoomSlider.signal_value_changed().connect(sigc::mem_fun(*this, &WM_SDK_Editor::on_change_value));

	HBox *hb = new HBox();
	hb->pack_start(*sdkPath, true, true);
	hb->pack_end(zoomSlider, false, false);
	hb->pack_end(*eb, false, false);

	box->pack_end(*hb, false, false);

	saveDlg = NULL;
	formEdit = NULL;

	return box;
}

WM_SDK_Editor::~WM_SDK_Editor() {
	if(saveDlg) delete saveDlg;
	closeFormEditor();

	delete sdkPath;
	delete sde;

	// TODO scroll and 2 x box
}

bool WM_SDK_Editor::processCommand(const ustring &cmd, const ustring &arg) {
	if(cmd == CMD_RUN) {
		pbuilder.build(sde->sdk);
		run();
	}
	else if(cmd == CMD_STOP) {
		//sde->sdk->stop();
	}
	else if(cmd == CMD_ZOOMIN) {
		sde->setZoom(sde->getZoom() + 0.2);
	}
	else if(cmd == CMD_ZOOMOUT) {
		sde->setZoom(sde->getZoom() - 0.2);
	}
	else if(cmd == CMD_DELETE) {
		sde->csdk->selMan->erase();
	}
	else if(cmd == CMD_SHA_INI) {
		// TODO optimize
		for(ElementsList::iterator e = sde->csdk->selMan->elements.begin(); e != sde->csdk->selMan->elements.end(); e++) {
			parent->open(sde->sdk->pack->pathElementConf((*e)->tpl->name));
		}
	}
	else if(cmd == CMD_SHA_PAS) {
		for(ElementsList::iterator e = sde->csdk->selMan->elements.begin(); e != sde->csdk->selMan->elements.end(); e++) {
			ustring f = sde->sdk->pack->pathElementCode((*e)->tpl->name);
			if(file_test(f, FILE_TEST_EXISTS))
				parent->open(f);
		}
	}
	else if(cmd == CMD_SAVE) {
		if(Glib::file_test(sde->sdk->fileName, Glib::FILE_TEST_EXISTS)) {
			saveToFile(sde->sdk->fileName);
			setChanged(false);
		}
		else
			processCommand(CMD_SAVEAS, arg);
	}
	else if(cmd == CMD_SAVEAS)
		saveToFileAs();
	else if(cmd == CMD_FREEZ) {
		if(sde->csdk->selMan->checkFlag(ELEMENT_FLG_IS_FREEZE))
			sde->csdk->selMan->unsetFlag(ELEMENT_FLG_IS_FREEZE);
		else
			sde->csdk->selMan->setFlag(ELEMENT_FLG_IS_FREEZE);
		sde->reDraw();
	}
	else if(cmd == CMD_ERASE)
		sde->csdk->selMan->clearLinks();
	else if(cmd == CMD_SENDTOBACK)
		sde->csdk->moveToDown(sde->csdk->selMan->getFirst());
	else if(cmd == CMD_BRINGTOFRONT)
		sde->csdk->moveToTop(sde->csdk->selMan->getFirst());
	else if(cmd == CMD_SELECTALL)
		sde->csdk->selMan->selectAll();
	else if(cmd == CMD_PASTE_DEBUG)
		sde->pasteElementOnLine("Debug");
	else if(cmd == CMD_COPY)
		sde->csdk->selMan->copy();
	else if(cmd == CMD_CUT) {
		sde->csdk->selMan->copy();
		sde->csdk->selMan->erase();
	}
	else if(cmd == CMD_PASTE)
		sde->csdk->selMan->paste();
	else if(cmd == CMD_FORMEDIT) {
		if(formEdit)
			closeFormEditor();
		else
			openFormEditor();
	}
	else if(cmd == CMD_COPY_LINK)
		sde->csdk->selMan->copyAsLink();
	else if(cmd == CMD_FORWARD) {
		sde->endOperation();
		bool fe = formEdit;
		if(fe) closeFormEditor();
		sde->enterSDK(sde->csdk->selMan->getFirst());
		if(fe) openFormEditor();
	}
	else if(cmd == CMD_BACK) {
		sde->endOperation();
		bool fe = formEdit;
		if(fe) closeFormEditor();
		sde->leaveSDK();
		if(fe) openFormEditor();
	}
	else if(cmd == CMD_BUILD)
		pbuilder.build(sde->sdk);
	else if(cmd == CMD_EDITOR) {
		TData dt;
		if(sde->csdk->selMan->getFirst())
			dt = sde->csdk->selMan->getFirst()->sysProps.getByName("Hint")->readStr();
		ElementCoreTransmitter *t = shaDlg.run("text_editor");
		ustring s = t->run(dt).toStr();
		SelectManager *sm = sde->csdk->selMan;
		for(ElementsList::iterator e = sm->elements.begin(); e != sm->elements.end(); e++)
			(*e)->sysProps.getByName("Hint")->setValue(&s);
	}
	else if(cmd == CMD_LINECOLOR) {
		ColorSelectionDialog cd("Change element color");
		cd.get_color_selection()->set_has_palette(true);
		TypeColor  c;
		if(sde->csdk->selMan->getFirst()) {
			c = *(TypeColor*)sde->csdk->selMan->getFirst()->sysProps.getByName("Color")->value;
			cd.get_color_selection()->set_previous_rgba(c);
			cd.get_color_selection()->set_current_rgba(c);
		}
		if(cd.run()) {
			SelectManager *sm = sde->csdk->selMan;
			c = cd.get_color_selection()->get_current_rgba();
			for(ElementsList::iterator e = sm->elements.begin(); e != sm->elements.end(); e++)
				(*e)->sysProps.getByName("Color")->setValue(&c);
		}
	}
	else if(cmd == CMD_MAKEHINT)
		sde->beginOperation(MS_ADD_LINK_HINT);
	else if(cmd == CMD_REMOVE_LH)
		sde->beginOperation(MS_REMOVE_LINK_HINT);
	else if(cmd == CMD_SLIDEDOWN)
		sde->beginOperation(MS_SELECT_BY_EDGE,(void*)(int)(SelectManager::SBE_EDGE_VERTICAL));
	else if(cmd == CMD_SLIDERIGHT)
		sde->beginOperation(MS_SELECT_BY_EDGE,(void*)(int)(SelectManager::SBE_EDGE_HORIZONTAL));
	else if(cmd == CMD_SLIDEDIAG)
		sde->beginOperation(MS_SELECT_BY_EDGE,(void*)(int)(SelectManager::SBE_EDGE_BOTH));
	else return false;

	return true;
}

void WM_SDK_Editor::update() {
	static const gchar *staticCommands[] = {
		CMD_SDK_EDITOR, CMD_MN_RUN, CMD_SHA_PAS, CMD_SHA_INI,
		CMD_SAVEAS, CMD_SELECTALL, CMD_PASTE, CMD_FORMEDIT,
		CMD_SLIDEDOWN, CMD_SLIDERIGHT, CMD_SLIDEDIAG, CMD_MAKEHINT, CMD_REMOVE_LH,
		NULL
	};
	for(const gchar **c = staticCommands; *c; c++)
		cmdMan->enable(*c);

	changed && cmdMan->enable(CMD_SAVE);

	bool sel = !sde->csdk->selMan->elements.empty();

	(sde->getZoom() < 4.0) && cmdMan->enable(CMD_ZOOMIN);
	(sde->getZoom() > 0.4) && cmdMan->enable(CMD_ZOOMOUT);

	sde->csdk->parent && cmdMan->enable(CMD_BACK);

	(sde->csdk->selMan->canDelete()) && cmdMan->enable(CMD_DELETE) && cmdMan->enable(CMD_CUT);

	formEdit && cmdMan->check(CMD_FORMEDIT);

	if(isRun())
		cmdMan->enable(CMD_STOP);
	else {
		cmdMan->enable(CMD_RUN);
		cmdMan->enable(CMD_BUILD);
	}

	// if selection not empty
	if(sel) {
		cmdMan->enable(CMD_ERASE);
		cmdMan->enable(CMD_FREEZ);
		cmdMan->enable(CMD_COPY);
		cmdMan->enable(CMD_EDITOR);
		cmdMan->enable(CMD_LINECOLOR);

		// ... and select only single element
		if(Element *e = sde->csdk->selMan->getFirst()) {
			(!e->isNoDelete()) && cmdMan->enable(CMD_BRINGTOFRONT)
							   && cmdMan->enable(CMD_SENDTOBACK)
							   && cmdMan->enable(CMD_COPY_LINK);

			e->sdk && cmdMan->enable(CMD_FORWARD);
		}
	}

	cmdMan->enable(CMD_PASTE_DEBUG);

	sel && sde->csdk->selMan->checkFlag(ELEMENT_FLG_IS_FREEZE) && cmdMan->check(CMD_FREEZ);
	formEdit && cmdMan->check(CMD_FORMEDIT);
}

void WM_SDK_Editor::callback(void *owner, CallbackType type, const void *data) {
	switch(type) {
		case CBT_CHANGE_SDK: {
			ChangeSDKInfo *sdi = (ChangeSDKInfo*)data;
			if(sdi->change) {
				sdi->sdk->selMan->on_selection_change += this;
				sdkPath->set_text(sdi->sdk->getPath());
				scroll.get_hadjustment()->set_value(sdi->sdk->scrollX);
				scroll.get_vadjustment()->set_value(sdi->sdk->scrollY);
			}
			else {
				sdi->sdk->scrollX = scroll.get_hadjustment()->get_value();
				sdi->sdk->scrollY = scroll.get_vadjustment()->get_value();
				sdi->sdk->selMan->on_selection_change -= this;
			}
			updateSelectionLabel();
		}
		case CBT_SDK_RUN_CHANGE:
			parent->update();
			break;
		case CBT_SDK_CHANGE_SELECT:
			updateSelectionLabel();
			parent->update();
			break;
		case CBT_POPUP_MENU:
			parent->showMenu((SDKObjectType)((int*)data)[0], ((int*)data)[1]);
			break;
		case CBT_KEY_DOWN:
			parent->translateKey((GdkEventKey*)data);
			break;
		case CBT_CHANGE_SCHEME:
			setChanged(true);
			break;
		default:;
	}
}

void WM_SDK_Editor::saveToFile(const ustring &file_name) {
	sde->saveToFile(file_name);
	parent->update();
}

void WM_SDK_Editor::saveToFileAs() {
	if(!saveDlg) {
		saveDlg = new FileChooserDialog("Save file", FILE_CHOOSER_ACTION_SAVE);
		saveDlg->add_button(Gtk::Stock::CANCEL, 0);
		saveDlg->add_button(Gtk::Stock::OK, 1);
		//saveDlg->set_extra_widget(*(new Button("test")));
		Glib::RefPtr<FileFilter> f = FileFilter::create();
		f->add_pattern("*.sha");
		f->set_name("Source HiAsm files");
		saveDlg->set_filter(f);
	}
	if(file_test(sde->sdk->fileName, FILE_TEST_EXISTS))
		saveDlg->set_filename(sde->sdk->fileName);
	else {
		saveDlg->set_current_name(g_filename_display_basename(sde->sdk->fileName.c_str()));
		saveDlg->set_current_folder(SettingsManager::readProjectsDir());
	}

	if(saveDlg->run() == 1) {
		saveToFile(saveDlg->get_filename());
		setChanged(false);
	}
	saveDlg->hide();
}

void WM_SDK_Editor::_on_end_edit(WM_SDK_Editor *obj) {
	obj->closeFormEditor();
}

void WM_SDK_Editor::openFormEditor() {
	if(sde->sdk->pack->name == "_base")
		formEdit = new FE_Internal();
	else
		formEdit = new FE_ExternalTcp();
	formEdit->start(sde);
	parent->update();
	formEdit->on_end_edit = EVENT(_on_end_edit);
}

void WM_SDK_Editor::closeFormEditor() {
	if(!formEdit) return;

	formEdit->stop();
	delete formEdit;
	formEdit = NULL;
	parent->update();
}

void WM_SDK_Editor::on_run_thread() {
	pbuilder.run(sde->sdk);
	runThread = NULL;
	gdk_threads_enter();
	parent->update();
	gdk_threads_leave();
}

void WM_SDK_Editor::run() {
	runThread = Glib::Thread::create(sigc::mem_fun(*this, &WM_SDK_Editor::on_run_thread), true);
	parent->update();
}

typedef struct _elementInfo {
	int count;
	Element *e;
	_elementInfo(Element *e) { count = 1; this->e = e; }
} elementInfo;

void WM_SDK_Editor::updateSelectionLabel() {
	elementsList->set_text(sde->csdk->selMan->empty() ? "(none)" : (ustring("Selected: ") + int_to_str(sde->csdk->selMan->elements.size())));
}

void WM_SDK_Editor::fillSelectionMenu() {
//	selInfoMenu.items().clear();

	std::list<elementInfo> list;
	int c;
	for(ElementsList::iterator e = sde->csdk->selMan->elements.begin(); e != sde->csdk->selMan->elements.end(); e++) {
		c = 0;
		for(std::list<elementInfo>::iterator s = list.begin(); s != list.end(); s++)
			if((*s).e->tpl == (*e)->tpl) {
				(*s).count++;
				c = 1;
				break;
			}
		if(c == 0)
			list.push_back(elementInfo(*e));
	}

	for(std::list<elementInfo>::iterator s = list.begin(); s != list.end(); s++) {
		SI_ImageMenuItem *item = new SI_ImageMenuItem((*s).e, (*s).count);
		selInfoMenu.append(*item);
	}
}

bool WM_SDK_Editor::on_sel_info_click(GdkEventButton *event) {
	if(sde->csdk->selMan->elements.size()) {
		fillSelectionMenu();
		selInfoMenu.popup(event->button, event->time);
	}
	return false;
}

void WM_SDK_Editor::on_change_value() {
	sde->setZoom(zoomSlider.get_value() / 100);
}

//---------------------------------------------------------------

Widget *WM_Text_Editor::open(const ustring &file_name, bool asNew) {
	TRACE_PROC

#ifdef USE_GTK_SOURCE_VIEW
	memo = new Gsv::View();
	memo->set_show_line_numbers(true);
	Glib::RefPtr<Gsv::LanguageManager> sm = Gsv::LanguageManager::create();
	//StringArrayHandle arr = sm->get_search_path();
	//for(StringArrayHandle::iterator sa = arr.begin(); sa != arr.end(); sa++)
	//	DEBUG_MSG((*sa).c_str())
	ustring ext = ustring(file_name, file_name.length() - 3);
	if(ext ==  "pas" || ext == "dpr")
		ext = "pascal";
	else if(ext ==  "hws")
		ext = "hiasm";
	Glib::RefPtr<Gsv::Language> lng = sm->get_language(ext);
	if(lng) {
		std::cout << "SELECT HL: " << ext << "\n";
		Glib::RefPtr<Gsv::Buffer> sb = Gsv::Buffer::create(lng);
		sb->set_highlight_syntax(true);
		memo->set_source_buffer(sb);
	}
#else
	memo = new TextView();
#endif

	if(file_test(file_name, FILE_TEST_EXISTS)) {
		gchar *cont;
		gsize len;
		g_file_get_contents(file_name.c_str(), &cont, &len, NULL);
		if(len > 0) {
			memo->get_buffer()->set_text(ustring(cont));
			g_free(cont);
		}
	}
	memo->get_buffer()->signal_changed().connect(sigc::mem_fun(*this, &WM_Text_Editor::on_edit_changed));
	fName = file_name;

	scroll = new ScrolledWindow();
	scroll->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scroll->add(*memo);

	changed = false;

	return scroll;
}

void WM_Text_Editor::on_edit_changed() {
	setChanged(true);
}

bool WM_Text_Editor::processCommand(const ustring &cmd, const ustring &arg) {
	if(cmd == CMD_SAVE) {
		g_file_set_contents(fName.c_str(), memo->get_buffer()->get_text(false).c_str(), -1, NULL);
		setChanged(false);
	}
	else if(cmd == CMD_SAVEAS) {

	}
	else return false;

	return true;
}

void WM_Text_Editor::update() {
	changed && cmdMan->enable(CMD_SAVE);
	cmdMan->enable(CMD_SAVEAS);
}

//---------------------------------------------------------------

WM_New_Project::~WM_New_Project() {
	rfMan.on_change -= this;
}

bool WM_New_Project::close() {
	return parent->tabs.size() > 1;
}

Widget *WM_New_Project::open(const ustring &file_name, bool asNew) {
	rfMan.on_change += this;

	files.set_size_request(100, 400);
	fileStore = Gtk::ListStore::create(fileColumns);
	files.set_model(fileStore);
	files.append_column("*", fileColumns.icon);
	files.append_column("Path", fileColumns.name);
	files.append_column("Size", fileColumns.size);
	files.append_column("Date", fileColumns.date);

	files.signal_row_activated().connect(sigc::mem_fun(*this, &WM_New_Project::on_row_activate));

	fillFileList();

	filesScroll.add(files);
	filesScroll.set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);

	paned.pack1(filesScroll, false, false);
	paned.pack2(memo, true, true);
	return &paned;
}

void WM_New_Project::on_row_activate(const TreeModel::Path &path, TreeViewColumn *column) {
	TreeModel::Row r = *fileStore->get_iter(path);
	parent->open(r[fileColumns.name]);
}

void WM_New_Project::fillFileList() {
	fileStore->clear();
	for(RecentFileManager::iterator f = rfMan.begin(); f != rfMan.end(); f++) {
		Gtk::TreeRow row = *(fileStore->append());
		row[fileColumns.name] = f->name;
		row[fileColumns.icon] = f->icon;

		GFile *file = g_file_new_for_path(f->name.c_str());
		GFileInfo *inf = g_file_query_info(file, "*", G_FILE_QUERY_INFO_NONE, NULL, NULL);
		row[fileColumns.size] = int_to_str(g_file_info_get_size(inf)) + "b";

		GTimeVal tv;
		g_file_info_get_modification_time(inf, &tv);
		row[fileColumns.date] = g_time_val_to_iso8601(&tv);

		g_object_unref(file);
		g_object_unref(inf);

		//row[fileColumns.ctl];
	}
}

void WM_New_Project::callback(void *owner, CallbackType type, const void *data) {
	fillFileList();
}

//-----------------------------------------------------------------------------------------------------
//-----------------------------------------------------------------------------------------------------

WindowManager::WindowManager():Notebook(),
	on_tab_open(this, CBT_TAB_OPEN),
	on_tab_change(this, CBT_TAB_CHANGE),
	on_tab_close(this, CBT_TAB_CLOSE)
{
	cmdMan->on_command += this;
	set_scrollable(true);
	openDlg = NULL;
	aboutDlg = NULL;

	oldTab = NULL;

	for(int i = 0; i < SDK_OBJ_COUNT; i++) popupMenus[i] = NULL;

#ifdef USE_GTK_SOURCE_VIEW
	Gsv::init();
#endif
}

WM_Tab *WindowManager::open(const ustring &file_name, bool asNew) {
	TRACE_PROC
	WM_Tab *tab;

	DEBUG_MSG("Open file: " << file_name.c_str());

	Glib::ustring ext = file_name.empty() ? "" : file_name.substr(file_name.length()-3, 3).lowercase();
	if(file_name.empty())
		tab = new WM_New_Project(this);
	else if(ext == "sha")
		tab = new WM_SDK_Editor(this);
//	else if(ext == "ini") {
//		if(oldTab != NULL && oldTab->type == WMT_SDK_EDITOR) {
//			//((WM_SDK_Editor*)oldTab)->sde->sdk->pack->addElement("Helpers", )
//		}
//	}
	else
		tab = new WM_Text_Editor(this);

	Widget *child = tab->open(file_name, asNew);
	child->show_all();
	tab->caption = new Label(Glib::filename_display_basename(tab->fileName()));

	HBox *box = new HBox();
	Image *img = new Image(tab->icon());
	// TODO memory?
	box->add(*img);
	box->add(*tab->caption);
	Image *close = new Image(Gdk::Pixbuf::create_from_file(dataDir + INT_PATH"close.png"));
	close->set_tooltip_text("Close");

	tab->closeBtn = new ToolButton();
	tab->closeBtn->set_size_request(16, 16);
	tab->closeBtn->set_icon_widget(*close);
	//tab->close->set_name("btn_close");
	tab->closeBtn->signal_clicked().connect(sigc::mem_fun(*this, &WindowManager::on_close_click));

	box->add(*tab->closeBtn);
	box->set_spacing(5);
	box->show_all();

	tabs.push_back(tab);
	on_tab_open.run(tab);
	append_page(*child, *box);
	setCurrent(tab);

	update();

	if(!asNew) rfMan.add(file_name);

	return tab;
}

void WindowManager::on_close_click() {
	if(oldTab->close()) {
		WM_Tab *tb = oldTab;
		oldTab = NULL;
		for(WM_Tabs::iterator t = tabs.begin(); t != tabs.end(); t++)
			if(*t == tb) {
				tabs.erase(t);
				break;
			}
		remove_page(*tb->widget());

		if(tabs.empty()) {
			on_tab_change.run(NULL);
			open("", true);
		}
		else
			update();
		delete tb;
	}
}

void WindowManager::setCurrent(WM_Tab *tab) {
	int i = page_num(*tab->widget());
	if(i != -1) {
		set_current_page(i);
		tab->select();
	}
}

void WindowManager::on_switch_page(Gtk::Widget *page, guint index) {
	Notebook::on_switch_page(page, index);

	DEBUG_MSG("switch page " << index)
	switchToTab(tabs[index]);
}

void WindowManager::switchToTab(WM_Tab *tab) {
	if(tab == oldTab) return;

	on_tab_change.run(tab);
	if(oldTab)
		oldTab->closeBtn->hide();
	oldTab = tab;
	oldTab->closeBtn->show();
	update();
}

void WindowManager::callback(void *owner, CallbackType type, const void *data) {
	switch(type) {
		case CBT_RUN_COMMAND:
			processCommand(*(ustring*)data, ustring());
			break;
		default:;
	}
}

WM_Tab *WindowManager::getCurrent() {
	int i = get_current_page();
	return (i == -1) ? NULL : tabs[i];
}

void WindowManager::processCommand(const ustring &cmd, const ustring &args) {
	std::cout << "command: " << cmd.c_str() << std::endl;

	WM_Tab *tab = getCurrent();
	if(tab && tab->processCommand(cmd, args))
		return;

	if(cmd == CMD_OPEN) {
		if(!openDlg) {
			openDlg = new FileChooserDialog("Open file");
			openDlg->add_button(Gtk::Stock::CANCEL, 0);
			openDlg->add_button(Gtk::Stock::OK, 1);
			//openDlg->set_extra_widget(*(new Button("test")));
			//openDlg->set_filter(FileFilter());
			for(PackCollection::iterator p = packSet->begin(); p != packSet->end(); p++) {
//				DEBUG_MSG((dataDir + "elements/" + (*p)->name + "/examples").c_str())
				openDlg->add_shortcut_folder(dataDir + "elements/" + (*p)->name + "/examples");
			}
		}
		if(openDlg->run() == 1)
			open(openDlg->get_filename());
		openDlg->hide();
	}
	else if(cmd == CMD_NEW) {
		if(prjMan.open() == 1)
			open(prjMan.selectProject->getTemplateFile(), true);
		prjMan.hide();
	}
	else if(cmd == CMD_EXIT)
		exit(0);
	else if(cmd == CMD_FORUM)
		gtk_show_uri(form->get_screen()->gobj(), "http://forum.hiasm.com", GDK_CURRENT_TIME, NULL);
	else if(cmd == CMD_MAIL)
		gtk_show_uri(form->get_screen()->gobj(), "mailto:support@hiasm.com", GDK_CURRENT_TIME, NULL);
//	else if(cmd == CMD_EXAMPLES)
//		gtk_show_uri(form->get_screen()->gobj(), "file:///", GDK_CURRENT_TIME, NULL);
	else if(cmd == CMD_ABOUT) {
		if(!aboutDlg) {
			aboutDlg = new AboutDialog();
			aboutDlg->set_version(HIASM_VERSION());
			aboutDlg->set_program_name("HiAsm");
			aboutDlg->set_comments("a graphic programming studio");
			aboutDlg->set_copyright("Copyright (C) 2004-2011");
			aboutDlg->set_logo(form->get_icon());
			aboutDlg->set_website("http://hiasm.com");
			const gchar *authors[] = {"Project owner:", "   Dilma <support@hiasm.com>", "", "Developers and testers:", "   Assasin <iassasin@yandex.ru>", "   Nic <nic@savelovo.net>", NULL};
			gtk_about_dialog_set_authors(aboutDlg->gobj(), authors);
			const gchar *artists[] = {"Nic <nic@savelovo.net>", NULL};
			gtk_about_dialog_set_artists(aboutDlg->gobj(), artists);
			gchar *cont;
			gsize len;
			g_file_get_contents((dataDir + INT_PATH"license").c_str(), &cont, &len, NULL);
			aboutDlg->set_license(ustring(cont, len));
			g_free(cont);
		}

		aboutDlg->run();
		aboutDlg->hide();
	}
	else if(cmd == CMD_ECREATOR) {
		if(oldTab != NULL && oldTab->type == WMT_SDK_EDITOR) {
			WM_SDK_Editor *wmSDK = (WM_SDK_Editor *)oldTab;
			ElementCoreTransmitter *t = shaDlg.run("element_creator");
			TData dt(wmSDK->sde->sdk->pack->path());
			dt = t->run(dt);
			if(!dt.empty()) {
				ElementConfig ec(wmSDK->sde->sdk->pack, wmSDK->sde->sdk->pack->pathElementConf(dt.toStr()));
				//pal->addGroup(dt.toStr());
				DEBUG_MSG(ec.tab.c_str() << " : " << ec.name.c_str())
				ec.pack->addElement(ec.tab, ec.name, ec.info);
			}
		}
	}
	else if(cmd == CMD_TOOLS)
		SettingsManager::show();

	update();
}

void WindowManager::update() {
	static const gchar *staticCommands[] = {
		CMD_OPEN, CMD_ABOUT, CMD_NEW, CMD_EXIT, CMD_TOOLS, CMD_FULLSCREEN,
		CMD_MN_FILE, CMD_MN_EDIT, CMD_MN_VIEW, CMD_MN_HELP, CMD_MN_SERVICE,
		CMD_FORUM, CMD_MAIL, CMD_ECREATOR,
		NULL
	};

	cmdMan->beginUpdate();

	if(WM_Tab *tab = getCurrent()) tab->update();

	for(const gchar **c = staticCommands; *c; c++)
		cmdMan->enable(*c);

	cmdMan->endUpdate();
}

void WindowManager::showMenu(SDKObjectType type, guint32 time) {
	Menu **m = &popupMenus[type];
	if(!*m) {
		const gchar *name = NULL;

		switch(type) {
			case SDK_OBJ_ELEMENT: name = MENUNAME_ELEMENT; break;
			case SDK_OBJ_NONE: name = MENUNAME_SDK;	break;
			case SDK_OBJ_LINE: name = MENUNAME_LINK; break;
			default: return;
		}
		*m = cmdMan->createMenu(name);
		update();
	}

	(*m)->popup(BTN_RIGHT, time);
}

void WindowManager::translateKey(GdkEventKey *key) {
	std::cout << "key  h:" << key->hardware_keycode << std::endl;
	std::cout << "key  val:" << key->keyval << std::endl;
	std::cout << "key  state:" << key->state << std::endl;

	if(key->keyval == 109) {
		DEBUG_MSG("start test...")
		MSDK *sdk = ((WM_SDK_Editor*)getCurrent())->sde->sdk;
		for(int i = 0; 1 < 10000; i++) {
			Element *e = sdk->add("hcTimer", 0, 0);
			sdk->remove(e);
		}
		DEBUG_MSG("test end.")
	}

	void *id = mdb.begin_read_command_by_key(key->keyval, key->state&0x01, key->state&0x04);
	CommandInfo cmd;
	while(mdb.read_command_by_key(id, cmd))
		if(cmdMan->isEnabled(cmd.name.c_str()))
			cmdMan->command(cmd.name);
	mdb.end_read(id);
}

void WindowManager::changeTab(WM_Tab *tab) {
	tab->caption->set_text((tab->changed ? ustring("*") : ustring()) + Glib::filename_display_basename(tab->fileName()));
	update();
}

bool WindowManager::checkProjectName(const ustring &name) {
	for(WM_Tabs::iterator t = tabs.begin(); t != tabs.end(); t++)
		if((*t)->caption->get_text() == name)
			return true;
	return false;
}

void WindowManager::init() {
	if(settingsMan->readInt(IDS_NEW_PROJECT)) {
		loadTabs();
		if(tabs.size() == 0)
			open("", true);
	}
	else {
		open((*(*packSet->begin())->projects.begin())->getTemplateFile(), true);
	}
	update();
}

void WindowManager::saveTabs() {
	StringList desktop;
	for(WM_Tabs::iterator t = tabs.begin(); t != tabs.end(); t++)
		desktop << (*t)->fileName();
	desktop.saveToFile(homeDir + "desktop.lst");
}

void WindowManager::loadTabs() {
	StringList desktop(homeDir + "desktop.lst");
	for(StringList::iterator line = desktop.begin(); line != desktop.end(); line++) {
		if(*line == "startup")
			open("", true);
		else if(file_test(*line, FILE_TEST_EXISTS))
			open(*line, false);
	}
}
