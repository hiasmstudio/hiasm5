/*! \mainpage HiAsm Studio
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

#include <gtkmm/stylecontext.h>
#include <gtkmm/viewport.h>

#include "main_gui.h"
#include "MainDataBase.h"
#include "RecentFileManager.h"
#include "SettingsManager.h"
#include "CodeGen.h"
#include "FormEditor.h"
#include "Element.h"
#include "Compiler.h"
#include "LegacyPackCatalog.h"
#include "Localization.h"

const char *appName = "HiAsm Studio";


Window *form;

TypePixbuf _stockRender(const ustring &name) {
	return stock_pixbuf(name, ICON_SIZE_MENU);
}

MainGUI::MainGUI():Window() {
	// On Windows, let GTK use the Win32 non-client frame for the main form.
	compat_standard_frame(*this);
	init();
}

void MainGUI::init() {
	TRACE_PROC

	form = this;
	stockRender = _stockRender;

	initDirs();
    SettingsManager::loadAppearance();

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
	showSystemProps = false;

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
#ifdef HIASM_GTK4
    signal_close_request().connect([this]() { return !wmp->canCloseAll(); }, false);
#else
    signal_delete_event().connect([this](GdkEventAny*) { return !wmp->canCloseAll(); }, false);
#endif
#ifdef HIASM_GTK4
	// Window commands must work on startup tabs, text views and property
	// fields too. Capture only global commands; leave editing keys to the
	// focused widget and scheme-specific keys to SDK_Editor.
	auto keys = Gtk::EventControllerKey::create();
	keys->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
	keys->signal_key_pressed().connect([this](guint value, guint code, Gdk::ModifierType modifiers) {
		GdkEventKey event = {};
		event.keyval = value; event.hardware_keycode = code; event.state = guint(modifiers);
		return wmp->translateKey(&event, true);
	}, false);
	add_controller(keys);
#else
	signal_key_press_event().connect([this](GdkEventKey *event) { return wmp->translateKey(event, true); }, false);
#endif
	DEBUG_MSG("load Palette...")
	pal = new Palette();
	pal->on_open_file = [this](const ustring &file) { wmp->open(file); };

	//---- temp
#ifndef HIASM_GTK4
	set_position(WIN_POS_CENTER);
#endif
	set_default_size(1000, 700);

	DEBUG_MSG("load menus...")
	main_tools = cmdMan->createToolbar(MENUNAME_TOOLBAR);
	MenuBar *menu = cmdMan->createMenuBar(MENUNAME_MAIN);
	cmdMan->attachMenu(CMD_BUILD, compilers);

	ScrolledWindow *sc_debug = new ScrolledWindow();
	sc_debug->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrolled_set_child(*sc_debug, debug);
	sc_debug->set_size_request(100, 70);

	HBox *tbox = new HBox();
	tbox->pack_start(*menu, PACK_SHRINK);
	tbox->add(*main_tools);
	// GTK4 does not always derive a useful natural height for the legacy
	// menu/toolbar wrappers before the first allocation. Give the command row
	// a stable client-area height so the editor panes cannot collapse to zero.
	tbox->set_size_request(-1, 32);
	menu->set_size_request(-1, 28);
	main_tools->set_size_request(-1, 28);
	tbox->set_hexpand(true);
	tbox->set_vexpand(false);
	//box1.pack_start(main_tools, PACK_SHRINK);
	//box1.add(*menu);
	box1.pack_start(*tbox, PACK_SHRINK);

#ifdef HIASM_GTK4
	// The editor, debug area and panels share one main-window widget tree.
	debugDockContent = sc_debug;
    auto errorsScroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    errorsScroll->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    errorsScroll->set_child(errors);
    errorsDockContent = errorsScroll;
	sc_debug->set_vexpand(false);
	sc_debug->set_size_request(-1, 110);

	propsFix.add(props);
	connect_button_press(propsFix, sigc::mem_fun(props, &PropEditor::on_button_press_event));
	propsFix.signal_button_release_event().connect(sigc::mem_fun(props, &PropEditor::on_button_release_event));
	connect_motion_notify(propsFix, sigc::mem_fun(props, &PropEditor::on_motion_notify_event));
	VBox *vb_props_points = new VBox();
	vb_props_points->pack_start(propsFix, true, true);
	vb_props_points->pack_end(pointsList, true, true);
	connect_size_allocate(propsFix, sigc::mem_fun(*this, &MainGUI::props_on_size_allocate));

	VPaned *vp = new VPaned();
	VBox *vb = new VBox();
	prop_tools = new Toolbar();
	fillPropTools();
	prop_tools->set_toolbar_style(TOOLBAR_ICONS);
	vb->pack_start(*prop_tools, false, false);
	ScrolledWindow *propertyScroll = new ScrolledWindow();
	propertyScroll->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	scrolled_set_child(*propertyScroll, *vb_props_points);
	vb->pack_end(*propertyScroll, true, true);
	paned_pack1(*vp, *vb, true, true);
	propInfo.set_size_request(100, 50);
	propInfo.set_editable(false);
	propInfo.set_wrap_mode(WRAP_WORD);
	paned_pack2(*vp, propInfo, false, false);
	propertyDockContent = vp;
	{
		auto css = Gtk::CssProvider::create();
		css->load_from_path(dataDir + "hiasm.css");
		Gtk::StyleProvider::add_provider_for_display(Gdk::Display::get_default(), css,
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}
#else
	box1.add(pan1);
	box1.set_hexpand(true);
	box1.set_vexpand(true);
	pan1.set_hexpand(true);
	pan1.set_vexpand(true);
	pan1.pack1(pan2, true, true);
	pan1.pack2(*sc_debug, false, false);

	sc_debug = new ScrolledWindow();
	sc_debug->set_policy(POLICY_NEVER, POLICY_AUTOMATIC);
	scrolled_set_child(*sc_debug, *pal);
	sc_debug->set_size_request(110, 120);

	pan2.pack1(*sc_debug, false, false);
	pan2.add2(pan3);
	pan3.pack1(*wmp, true, true);
	sc_debug = new ScrolledWindow();
	sc_debug->set_policy(POLICY_AUTOMATIC, POLICY_AUTOMATIC);
	sc_debug->set_size_request(150, 100);

	propsFix.add(props);
	connect_button_press(propsFix, sigc::mem_fun(props, &PropEditor::on_button_press_event));
	propsFix.signal_button_release_event().connect(sigc::mem_fun(props, &PropEditor::on_button_release_event));
	connect_motion_notify(propsFix, sigc::mem_fun(props, &PropEditor::on_motion_notify_event));
	VBox *vb_props_points = new VBox();
	vb_props_points->pack_start(propsFix, true, true);
	vb_props_points->pack_end(pointsList, true, true);

	scrolled_set_child(*sc_debug, *vb_props_points);
	connect_size_allocate(propsFix, sigc::mem_fun(*this, &MainGUI::props_on_size_allocate));
	
	VPaned *vp = new VPaned();
	VBox *vb = new VBox();
	prop_tools = new Toolbar();
	fillPropTools();
	prop_tools->set_toolbar_style(TOOLBAR_ICONS);
	vb->pack_start(*prop_tools, false, false);
	vb->pack_end(*sc_debug, true, true);
	paned_pack1(*vp, *vb, true, true);
	propInfo.set_size_request(100, 50);
	propInfo.set_editable(false);
	propInfo.set_wrap_mode(WRAP_WORD);

	DEBUG_MSG("load gtk theme...")
#ifdef HIASM_GTK4
	{
		// GTK4 ignores gtkrc; the theme is resources/hiasm.css.
		Glib::RefPtr<Gtk::CssProvider> css = Gtk::CssProvider::create();
		css->load_from_path((dataDir + "hiasm.css").c_str());
		Gtk::StyleProvider::add_provider_for_display(Gdk::Display::get_default(), css,
			GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
	}
#else
	gtk_rc_parse((dataDir + "gtkrc").c_str());
#endif
//	propInfo.get_style()->set_bg(STATE_NORMAL, get_style()->get_bg(STATE_NORMAL));

	paned_pack2(*vp, propInfo, false, false);
	pan3.pack2(*vp, false, false);
#endif

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

	box1.show();
	compat_hide_no_show_all(pointsList);

#ifdef HIASM_GTK4
	createDocks();
	set_child(box1);
	set_icon_name("hiasm");
	// GTK4 no longer propagates the old GTK3 show-all/expand defaults through
	// compatibility containers. Keep the editor surface allocated below the
	// toolbar and explicitly expose the complete widget tree before mapping.
	wmp->set_hexpand(true);
	wmp->set_vexpand(true);
	compat_show_all(box1);
	tbox->show();
	menu->show();
	main_tools->show();
	wmp->show();
#else
	add(box1);
	set_icon_name("hiasm");
#endif

	curSelMan = NULL;
	sde = NULL;
    lightColor = themeBase(pal->hint.get_style_context());
    darkColor = themeMix(lightColor, themeText(pal->hint.get_style_context()), 0.4);
    hintColor = themeText(pal->hint.get_style_context());

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
	HStringList list(homeDir + APP_SETTINGS);
	for(HStringList::iterator line = list.begin(); line != list.end(); line++) {
		ustring l = *line;
		if(sscanf((*line).c_str(), "palette=%d", &size))
			pan2.set_position(size);
		else if(sscanf((*line).c_str(), "debug=%d", &size))
			pan1.set_position(size);
		else if(sscanf((*line).c_str(), "properties=%d", &size))
			pan3.set_position(size);
#ifdef HIASM_GTK4
		else if(sscanf((*line).c_str(), "palette_visible=%d", &size))
			paletteDockVisible = size != 0;
		else if(sscanf((*line).c_str(), "properties_visible=%d", &size))
			propertyDockVisible = size != 0;
		else if(sscanf((*line).c_str(), "palette_attached=%d", &size))
			paletteDockAttached = size != 0;
		else if(sscanf((*line).c_str(), "properties_attached=%d", &size))
			propertyDockAttached = size != 0;
		else if(sscanf((*line).c_str(), "palette_side=%d", &size))
			paletteDockSide = (size >= 0 && size <= 2) ? size : 0;
		else if(sscanf((*line).c_str(), "properties_side=%d", &size))
			propertyDockSide = (size >= 0 && size <= 2) ? size : 1;
        else if(sscanf((*line).c_str(), "repository_visible=%d", &size))
            repositoryDockVisible = size != 0;
        else if(sscanf((*line).c_str(), "repository_attached=%d", &size))
            repositoryDockAttached = size != 0;
        else if(sscanf((*line).c_str(), "repository_side=%d", &size))
            repositoryDockSide = (size >= 0 && size <= 2) ? size : 1;
        else if(sscanf((*line).c_str(), "debug_visible=%d", &size))
            debugDockVisible = size != 0;
        else if(sscanf((*line).c_str(), "debug_attached=%d", &size))
            debugDockAttached = size != 0;
        else if(sscanf((*line).c_str(), "examples_visible=%d", &size))
            examplesDockVisible = size != 0;
        else if(sscanf((*line).c_str(), "examples_attached=%d", &size))
            examplesDockAttached = size != 0;
        else if(sscanf((*line).c_str(), "examples_side=%d", &size))
            examplesDockSide = (size >= 0 && size <= 2) ? size : 1;
        else if(sscanf((*line).c_str(), "build_settings_visible=%d", &size))
            buildSettingsDockVisible = size != 0;
        else if(sscanf((*line).c_str(), "build_settings_attached=%d", &size))
            buildSettingsDockAttached = size != 0;
        else if(sscanf((*line).c_str(), "errors_visible=%d", &size))
            errorsDockVisible = size != 0;
        else if(sscanf((*line).c_str(), "errors_attached=%d", &size))
            errorsDockAttached = size != 0;
        else if(sscanf((*line).c_str(), "terminal_visible=%d", &size))
            terminalDockVisible = size != 0;
        else if(sscanf((*line).c_str(), "terminal_attached=%d", &size))
            terminalDockAttached = size != 0;
        else if(sscanf((*line).c_str(), "palette_width=%d", &size))
            paletteDockWidth = (std::max)(120, size);
        else if(sscanf((*line).c_str(), "properties_width=%d", &size))
            propertyDockWidth = (std::max)(120, size);
        else if(sscanf((*line).c_str(), "repository_width=%d", &size))
            repositoryDockWidth = (std::max)(120, size);
        else if(sscanf((*line).c_str(), "examples_width=%d", &size))
            examplesDockWidth = (std::max)(120, size);
        else if(sscanf((*line).c_str(), "build_settings_width=%d", &size))
            buildSettingsDockWidth = (std::max)(120, size);
        else if(sscanf((*line).c_str(), "debug_height=%d", &size))
            debugDockHeight = (std::max)(100, size);
        else if(sscanf((*line).c_str(), "errors_height=%d", &size))
            errorsDockHeight = (std::max)(100, size);
        else if(sscanf((*line).c_str(), "terminal_height=%d", &size))
            terminalDockHeight = (std::max)(100, size);
        else if(sscanf((*line).c_str(), "palette_float_x=%lf", &paletteFloatX)) {}
        else if(sscanf((*line).c_str(), "palette_float_y=%lf", &paletteFloatY)) {}
        else if(sscanf((*line).c_str(), "properties_float_x=%lf", &propertyFloatX)) {}
        else if(sscanf((*line).c_str(), "properties_float_y=%lf", &propertyFloatY)) {}
        else if(sscanf((*line).c_str(), "repository_float_x=%lf", &repositoryFloatX)) {}
        else if(sscanf((*line).c_str(), "repository_float_y=%lf", &repositoryFloatY)) {}
        else if(sscanf((*line).c_str(), "examples_float_x=%lf", &examplesFloatX)) {}
        else if(sscanf((*line).c_str(), "examples_float_y=%lf", &examplesFloatY)) {}
        else if(sscanf((*line).c_str(), "build_settings_float_x=%lf", &buildSettingsFloatX)) {}
        else if(sscanf((*line).c_str(), "build_settings_float_y=%lf", &buildSettingsFloatY)) {}
        else if(sscanf((*line).c_str(), "debug_float_x=%lf", &debugFloatX)) {}
        else if(sscanf((*line).c_str(), "debug_float_y=%lf", &debugFloatY)) {}
        else if(sscanf((*line).c_str(), "errors_float_x=%lf", &errorsFloatX)) {}
        else if(sscanf((*line).c_str(), "errors_float_y=%lf", &errorsFloatY)) {}
        else if(sscanf((*line).c_str(), "terminal_float_x=%lf", &terminalFloatX)) {}
        else if(sscanf((*line).c_str(), "terminal_float_y=%lf", &terminalFloatY)) {}
#endif
	}
#ifdef HIASM_GTK4
	if(paletteDock) paletteDock->set_preferred_width(paletteDockWidth);
	if(propertyDock) propertyDock->set_preferred_width(propertyDockWidth);
	if(repositoryDock) repositoryDock->set_preferred_width(repositoryDockWidth);
	if(examplesDock) examplesDock->set_preferred_width(examplesDockWidth);
	if(buildSettingsDock) buildSettingsDock->set_preferred_width(buildSettingsDockWidth);
	if(debugDock) debugDock->set_preferred_height(debugDockHeight);
    if(errorsDock) errorsDock->set_preferred_height(errorsDockHeight);
    if(terminalDock) terminalDock->set_preferred_height(terminalDockHeight);
	applyDockAttachment();
	applyDockVisibility();
#endif
}

MainGUI::~MainGUI() {
	TRACE_PROC
	mdb.close();

	//save settings
	pal->save();
	wmp->saveTabs();

	HStringList list;
	list << ustring::compose("palette=%1", pan2.get_position());
	list << ustring::compose("debug=%1", pan1.get_position());
	list << ustring::compose("properties=%1", pan3.get_position());
#ifdef HIASM_GTK4
	if(dockWorkspace) syncDockState();
	list << ustring::compose("palette_visible=%1", paletteDockVisible ? 1 : 0);
	list << ustring::compose("properties_visible=%1", propertyDockVisible ? 1 : 0);
	list << ustring::compose("palette_attached=%1", paletteDockAttached ? 1 : 0);
	list << ustring::compose("properties_attached=%1", propertyDockAttached ? 1 : 0);
	list << ustring::compose("palette_side=%1", paletteDockSide);
	list << ustring::compose("properties_side=%1", propertyDockSide);
    list << ustring::compose("repository_visible=%1", repositoryDockVisible ? 1 : 0);
    list << ustring::compose("repository_attached=%1", repositoryDockAttached ? 1 : 0);
    list << ustring::compose("repository_side=%1", repositoryDockSide);
    list << ustring::compose("debug_visible=%1", debugDockVisible ? 1 : 0);
    list << ustring::compose("debug_attached=%1", debugDockAttached ? 1 : 0);
    list << ustring::compose("examples_visible=%1", examplesDockVisible ? 1 : 0);
    list << ustring::compose("examples_attached=%1", examplesDockAttached ? 1 : 0);
    list << ustring::compose("examples_side=%1", examplesDockSide);
    list << ustring::compose("build_settings_visible=%1", buildSettingsDockVisible ? 1 : 0);
    list << ustring::compose("build_settings_attached=%1", buildSettingsDockAttached ? 1 : 0);
    if(paletteDock) paletteDockWidth = paletteDock->preferred_width();
    if(propertyDock) propertyDockWidth = propertyDock->preferred_width();
    if(repositoryDock) repositoryDockWidth = repositoryDock->preferred_width();
    if(examplesDock) examplesDockWidth = examplesDock->preferred_width();
    if(buildSettingsDock) buildSettingsDockWidth = buildSettingsDock->preferred_width();
    if(debugDock) debugDockHeight = debugDock->preferred_height();
    list << ustring::compose("palette_width=%1", paletteDockWidth);
    list << ustring::compose("properties_width=%1", propertyDockWidth);
    list << ustring::compose("repository_width=%1", repositoryDockWidth);
    list << ustring::compose("examples_width=%1", examplesDockWidth);
    list << ustring::compose("build_settings_width=%1", buildSettingsDockWidth);
    list << ustring::compose("debug_height=%1", debugDockHeight);
    list << ustring::compose("palette_float_x=%1", paletteFloatX);
    list << ustring::compose("palette_float_y=%1", paletteFloatY);
    list << ustring::compose("properties_float_x=%1", propertyFloatX);
    list << ustring::compose("properties_float_y=%1", propertyFloatY);
    list << ustring::compose("repository_float_x=%1", repositoryFloatX);
    list << ustring::compose("repository_float_y=%1", repositoryFloatY);
    list << ustring::compose("examples_float_x=%1", examplesFloatX);
    list << ustring::compose("examples_float_y=%1", examplesFloatY);
    list << ustring::compose("build_settings_float_x=%1", buildSettingsFloatX);
    list << ustring::compose("build_settings_float_y=%1", buildSettingsFloatY);
    list << ustring::compose("debug_float_x=%1", debugFloatX);
    list << ustring::compose("debug_float_y=%1", debugFloatY);
    if(errorsDock) errorsDockHeight = errorsDock->preferred_height();
    if(terminalDock) terminalDockHeight = terminalDock->preferred_height();
    if(errorsDock) errorsDockHeight = errorsDock->preferred_height();
    if(terminalDock) terminalDockHeight = terminalDock->preferred_height();
    list << ustring::compose("errors_visible=%1", errorsDockVisible ? 1 : 0);
    list << ustring::compose("errors_attached=%1", errorsDockAttached ? 1 : 0);
    list << ustring::compose("errors_height=%1", errorsDockHeight);
    list << ustring::compose("errors_float_x=%1", errorsFloatX);
    list << ustring::compose("errors_float_y=%1", errorsFloatY);
    list << ustring::compose("terminal_visible=%1", terminalDockVisible ? 1 : 0);
    list << ustring::compose("terminal_attached=%1", terminalDockAttached ? 1 : 0);
    list << ustring::compose("terminal_height=%1", terminalDockHeight);
    list << ustring::compose("terminal_float_x=%1", terminalFloatX);
    list << ustring::compose("terminal_float_y=%1", terminalFloatY);
#endif
	list.saveToFile(homeDir + APP_SETTINGS);
}

#ifdef HIASM_GTK4
void MainGUI::createDocks() {
    dockWorkspace = Gtk::make_managed<DockWorkspace>(*wmp, *debugDockContent);
    box1.pack_start(*dockWorkspace, true, true);
    auto paletteScroll = Gtk::make_managed<Gtk::ScrolledWindow>();
    paletteScroll->set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    paletteScroll->set_child(*pal);
    paletteDock = &dockWorkspace->add_panel(ui_tr("Elements"), *paletteScroll, DockSide::Left, 240);
    propertyDock = &dockWorkspace->add_panel(ui_tr("Element property"), *propertyDockContent, DockSide::Right, 300);
    debugDock = &dockWorkspace->add_panel(ui_tr("Debug"), *debugDockContent, DockSide::Bottom, 300, "debug-symbolic");
    errorsDock = &dockWorkspace->add_panel(ui_tr("Errors"), *errorsDockContent, DockSide::Bottom, 300, "dialog-error-symbolic");
    terminalPanel = Gtk::make_managed<TerminalPanel>();
    terminalDock = &dockWorkspace->add_panel(ui_tr("Terminal"), *terminalPanel, DockSide::Bottom, 300, "utilities-terminal-symbolic");
    examplesPanel = Gtk::make_managed<ExamplesCatalogPanel>();
    examplesDock = &dockWorkspace->add_panel(ui_tr("Project catalog"), *examplesPanel, DockSide::Left, 260, "folder-documents-symbolic");
    repositoryPanel = Gtk::make_managed<RepositoryPanel>();
    repositoryDock = &dockWorkspace->add_panel(ui_tr("Project repository"), *repositoryPanel, DockSide::Right, 300);
    buildSettingsPanel = Gtk::make_managed<BuildSettingsPanel>();
    buildSettingsDock = &dockWorkspace->add_panel(ui_tr("Build settings"), *buildSettingsPanel, DockSide::Right, 280, "applications-system-symbolic");
    buildSettingsPanel->on_build = [this]() { if(cmdMan) cmdMan->command(CMD_BUILD); };
    paletteDock->set_preferred_width(paletteDockWidth);
    propertyDock->set_preferred_width(propertyDockWidth);
    repositoryDock->set_preferred_width(repositoryDockWidth);
    examplesDock->set_preferred_width(examplesDockWidth);
    buildSettingsDock->set_preferred_width(buildSettingsDockWidth);
    debugDock->set_preferred_height(debugDockHeight);
    errorsDock->set_preferred_height(errorsDockHeight);
    terminalDock->set_preferred_height(terminalDockHeight);
    paletteDock->set_float_geometry(paletteFloatX, paletteFloatY, paletteDock->float_width(), paletteDock->float_height());
    propertyDock->set_float_geometry(propertyFloatX, propertyFloatY, propertyDock->float_width(), propertyDock->float_height());
    repositoryDock->set_float_geometry(repositoryFloatX, repositoryFloatY, repositoryDock->float_width(), repositoryDock->float_height());
    examplesDock->set_float_geometry(examplesFloatX, examplesFloatY, examplesDock->float_width(), examplesDock->float_height());
    buildSettingsDock->set_float_geometry(buildSettingsFloatX, buildSettingsFloatY, buildSettingsDock->float_width(), buildSettingsDock->float_height());
    debugDock->set_float_geometry(debugFloatX, debugFloatY, debugDock->float_width(), debugDock->float_height());
    errorsDock->set_float_geometry(errorsFloatX, errorsFloatY, errorsDock->float_width(), errorsDock->float_height());
    terminalDock->set_float_geometry(terminalFloatX, terminalFloatY, terminalDock->float_width(), terminalDock->float_height());
    examplesPanel->add_source({ui_tr("My projects"), SettingsManager::readProjectsDir().raw(), "user-projects"});
    std::vector<std::string> diagnostics;
    for(const auto &location : LegacyPackCatalog::discover(LegacyPackCatalog::configurationPath(dataDir.raw()), diagnostics)) {
        std::string examples = location.directory + "/Example";
        if(g_file_test(examples.c_str(), G_FILE_TEST_IS_DIR))
            examplesPanel->add_source({location.name, examples,
                                        "legacy-examples:" + location.directory});
    }
    repositoryPanel->on_open_file = [this](const std::string &file) { wmp->open(file); };
    examplesPanel->on_open_file = [this](const std::string &file) {
        // Language trees mirror package files.  Keep the catalog hierarchy
        // complete by showing the base examples, but open a localized mirror
        // when one exists for the active interface language.
        std::string selected = file;
        for(Pack *pack : *packSet) {
            if(pack->isLegacy()) continue;
            const std::string exampleRoot = pack->path().raw() + "example/";
            if(selected.rfind(exampleRoot, 0) != 0) continue;
            const std::string relative = selected.substr(exampleRoot.size());
            const std::string localized = pack->path().raw() + "lang/" + ui_language() + "/example/" + relative;
            if(g_file_test(localized.c_str(), G_FILE_TEST_IS_REGULAR)) selected = localized;
            break;
        }
        wmp->open(selected);
    };
    for(Pack *pack : *packSet) {
        if(pack->isLegacy()) continue;
        std::string path = pack->path().raw() + "/example";
        if(!g_file_test(path.c_str(), G_FILE_TEST_IS_DIR))
            path = pack->path().raw() + "/Example";
        if(g_file_test(path.c_str(), G_FILE_TEST_IS_DIR))
            examplesPanel->add_source({pack->title.raw(), path, "pack:" + pack->name});
    }
    for(const auto &location : LegacyPackCatalog::discover(LegacyPackCatalog::configurationPath(dataDir.raw()), diagnostics)) {
        // HiAsm 4 packages are represented by their Elements/<package>/Example
        // directory above; do not add the package root as a second tree.
        (void)location;
    }
    examplesPanel->refresh();
    repositoryPanel->on_save_copy = [this](const std::string &file, std::string &error) {
        auto tab = dynamic_cast<WM_SDK_Editor *>(wmp->getCurrent());
        if(!tab || !tab->sde || !tab->sde->sdk->pack) { error = ui_tr("Open a scheme before saving to the repository."); return false; }
        ustring text;
        if(!tab->sde->sdk->info.empty()) text = "*" + tab->sde->sdk->info + "\n";
        text += "Make(" + tab->sde->sdk->pack->name + ")\n";
        tab->sde->sdk->saveToText(text);
        GError *failure = nullptr;
        GFile *destination = g_file_new_for_path(file.c_str());
        auto stream = g_file_create(destination, G_FILE_CREATE_NONE, nullptr, &failure);
        g_object_unref(destination);
        bool ok = stream != nullptr;
        if(stream) {
            ok = g_output_stream_write_all(G_OUTPUT_STREAM(stream), text.c_str(), text.bytes(), nullptr, nullptr, &failure);
            GError *closeFailure = nullptr;
            if(!g_output_stream_close(G_OUTPUT_STREAM(stream), nullptr, &closeFailure)) ok = false;
            if(!failure) { failure = closeFailure; closeFailure = nullptr; }
            g_clear_error(&closeFailure); g_object_unref(stream);
        }
        if(failure) { error = failure->message; g_error_free(failure); }
        return ok;
    };
    repositoryPanel->on_directory_changed = [this](const std::string &directory, bool) {
        if(examplesPanel)
            examplesPanel->set_source("current-project", ui_tr("Current project"), directory);
    };
    dockWorkspace->signal_changed().connect([this]() { syncDockState(); updateDockCommands(); });
    applyDockAttachment();
}

void MainGUI::syncDockState() {
    if(!paletteDock || !propertyDock || !repositoryDock || !debugDock || !errorsDock || !terminalDock || !examplesDock || !buildSettingsDock) return;
    paletteDockVisible = paletteDock->is_panel_visible();
    propertyDockVisible = propertyDock->is_panel_visible();
    paletteDockAttached = paletteDock->is_pinned();
    propertyDockAttached = propertyDock->is_pinned();
    paletteDockSide = int(paletteDock->side());
    propertyDockSide = int(propertyDock->side());
    repositoryDockVisible = repositoryDock->is_panel_visible();
    repositoryDockAttached = repositoryDock->is_pinned();
    repositoryDockSide = int(repositoryDock->side());
    debugDockVisible = debugDock->is_panel_visible();
    debugDockAttached = debugDock->is_pinned();
    errorsDockVisible = errorsDock->is_panel_visible();
    errorsDockAttached = errorsDock->is_pinned();
    terminalDockVisible = terminalDock->is_panel_visible();
    terminalDockAttached = terminalDock->is_pinned();
    examplesDockVisible = examplesDock->is_panel_visible();
    examplesDockAttached = examplesDock->is_pinned();
    examplesDockSide = int(examplesDock->side());
    buildSettingsDockVisible = buildSettingsDock->is_panel_visible();
    buildSettingsDockAttached = buildSettingsDock->is_pinned();
    paletteDockWidth = paletteDock->preferred_width();
    propertyDockWidth = propertyDock->preferred_width();
    repositoryDockWidth = repositoryDock->preferred_width();
    examplesDockWidth = examplesDock->preferred_width();
    buildSettingsDockWidth = buildSettingsDock->preferred_width();
    debugDockHeight = debugDock->preferred_height();
    errorsDockHeight = errorsDock->preferred_height();
    terminalDockHeight = terminalDock->preferred_height();
    paletteFloatX = paletteDock->float_x(); paletteFloatY = paletteDock->float_y();
    propertyFloatX = propertyDock->float_x(); propertyFloatY = propertyDock->float_y();
    repositoryFloatX = repositoryDock->float_x(); repositoryFloatY = repositoryDock->float_y();
    examplesFloatX = examplesDock->float_x(); examplesFloatY = examplesDock->float_y();
    buildSettingsFloatX = buildSettingsDock->float_x(); buildSettingsFloatY = buildSettingsDock->float_y();
    debugFloatX = debugDock->float_x(); debugFloatY = debugDock->float_y();
    errorsFloatX = errorsDock->float_x(); errorsFloatY = errorsDock->float_y();
    terminalFloatX = terminalDock->float_x(); terminalFloatY = terminalDock->float_y();
}

void MainGUI::applyDockVisibility() { applyDockAttachment(); }

void MainGUI::applyDockAttachment() {
    if(!dockWorkspace || !paletteDock || !propertyDock || !repositoryDock || !debugDock || !errorsDock || !terminalDock || !examplesDock || !buildSettingsDock) return;
    dockWorkspace->set_state(*paletteDock, paletteDockVisible, paletteDockAttached, DockSide(paletteDockSide));
    dockWorkspace->set_state(*propertyDock, propertyDockVisible, propertyDockAttached, DockSide(propertyDockSide));
    dockWorkspace->set_state(*repositoryDock, repositoryDockVisible, repositoryDockAttached, DockSide(repositoryDockSide));
    dockWorkspace->set_state(*debugDock, debugDockVisible, debugDockAttached, DockSide::Bottom);
    dockWorkspace->set_state(*errorsDock, errorsDockVisible, errorsDockAttached, DockSide::Bottom);
    dockWorkspace->set_state(*terminalDock, terminalDockVisible, terminalDockAttached, DockSide::Bottom);
    dockWorkspace->set_state(*examplesDock, examplesDockVisible, examplesDockAttached, DockSide(examplesDockSide));
    dockWorkspace->set_state(*buildSettingsDock, buildSettingsDockVisible, buildSettingsDockAttached, DockSide::Right);
    dockWorkspace->update_layout();
    updateDockCommands();
}

void MainGUI::updateDockCommands() {
    if(!cmdMan) return;
    cmdMan->uncheck(CMD_ELEMENTS);
    cmdMan->uncheck(CMD_PROPERTY);
    cmdMan->uncheck(CMD_PANELS);
    cmdMan->uncheck(CMD_PN_REP);
    cmdMan->uncheck(CMD_PN_DEBUG);
    cmdMan->uncheck(CMD_PN_ERRORS);
    cmdMan->uncheck(CMD_PN_TERMINAL);
    cmdMan->uncheck(CMD_EXAMPLES);
    cmdMan->uncheck(CMD_COMPILERS);
    cmdMan->enable(CMD_ELEMENTS);
    cmdMan->enable(CMD_PROPERTY);
    cmdMan->enable(CMD_PANELS);
    cmdMan->enable(CMD_PN_REP);
    cmdMan->enable(CMD_PN_DEBUG);
    cmdMan->enable(CMD_PN_ERRORS);
    cmdMan->enable(CMD_PN_TERMINAL);
    cmdMan->enable(CMD_EXAMPLES);
    cmdMan->enable(CMD_COMPILERS);
    if(repositoryDockVisible) cmdMan->check(CMD_PN_REP);
    if(debugDockVisible) cmdMan->check(CMD_PN_DEBUG);
    if(errorsDockVisible) cmdMan->check(CMD_PN_ERRORS);
    if(terminalDockVisible) cmdMan->check(CMD_PN_TERMINAL);
    if(examplesDockVisible) cmdMan->check(CMD_EXAMPLES);
    if(paletteDockVisible) cmdMan->check(CMD_ELEMENTS);
    if(propertyDockVisible) cmdMan->check(CMD_PROPERTY);
    if(buildSettingsDockVisible) cmdMan->check(CMD_COMPILERS);
    if(paletteDockVisible && propertyDockVisible && repositoryDockVisible && debugDockVisible && errorsDockVisible && terminalDockVisible && examplesDockVisible && buildSettingsDockVisible) cmdMan->check(CMD_PANELS);
    cmdMan->endUpdate();
}

void MainGUI::updateRepositoryTarget(WM_Tab *tab) {
    if(!repositoryPanel) return;
    const bool fileTab = tab && tab->type != WMT_NEW_PROJECT;
    const std::string file = fileTab ? tab->fileName().raw() : std::string();
    repositoryPanel->set_project(file, !file.empty() && packSet->isLegacyFile(file));
    repositoryPanel->set_can_save(tab && tab->type == WMT_SDK_EDITOR);
    if(terminalPanel) {
        std::string directory;
        if(!file.empty()) {
            gchar *parent = g_path_get_dirname(file.c_str());
            if(parent) { directory = parent; g_free(parent); }
        }
        if(directory.empty()) directory = repositoryPanel->directory();
        terminalPanel->set_working_directory(directory);
    }
    if(examplesPanel)
        examplesPanel->set_source("current-project", ui_tr("Current project"), repositoryPanel->directory());
}
#endif

void MainGUI::on_show() {
	Gtk::Widget::on_show();

	if(!sysColors.load) sysColors.init(&props);

	if(!curList) {
		DEBUG_MSG("load CursorList...")
		curList = new CursorList(Gdk::Display::get_default());
		DEBUG_MSG("load CursorList ok")
	}

	splash.hide();

	wmp->init();
	compat_show_all(*wmp);
	wmp->show();
	DEBUG_MSG("load ok.")
}

void MainGUI::callback(void *owner, CallbackType type, const void *data) {
//	std::cout << "Event receive: " << type << std::endl;

	switch(type) {
		case CBT_SELECT_ELEMENT_PALETTE:
			if(WidgetEditorEx *we = WidgetEditorEx::getActive(sde))
				we->beginInsert((const char*)data);
			else
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
#ifdef HIASM_GTK4
				if(buildSettingsPanel) buildSettingsPanel->set_pack(sde->sdk->pack);
#endif

				pal->loadPack(sde->sdk->pack);
			}
			else {
				fillProps(NULL);
				pal->clear();
				sde = NULL;
#ifdef HIASM_GTK4
				if(buildSettingsPanel) buildSettingsPanel->set_pack(nullptr);
#endif
			}
			set_title(ustring(appName) + (data ? (" [" + ((WM_Tab*)data)->fileName() + "]") : ustring()));
#ifdef HIASM_GTK4
            updateRepositoryTarget(const_cast<WM_Tab *>(static_cast<const WM_Tab *>(data)));
#endif
			break;
		case CBT_RUN_COMMAND:
			if(pal->processCommand(*(const ustring *)data)) break;
#ifdef HIASM_GTK4
            if(*(const ustring *)data == CMD_PN_REP) {
                repositoryDockVisible = !repositoryDockVisible;
                applyDockVisibility();
                break;
            }
            if(*(const ustring *)data == CMD_PN_DEBUG) {
                debugDockVisible = !debugDockVisible;
                applyDockVisibility();
                break;
            }
            if(*(const ustring *)data == CMD_PN_ERRORS) {
                errorsDockVisible = !errorsDockVisible;
                applyDockVisibility();
                if(errorsDockVisible) dockWorkspace->activate_panel(*errorsDock);
                break;
            }
            if(*(const ustring *)data == CMD_PN_TERMINAL) {
                terminalDockVisible = !terminalDockVisible;
                applyDockVisibility();
                if(terminalDockVisible) dockWorkspace->activate_panel(*terminalDock);
                break;
            }
            if(*(const ustring *)data == CMD_EXAMPLES) {
                examplesDockVisible = !examplesDockVisible;
                applyDockVisibility();
                break;
            }
            if(*(const ustring *)data == CMD_SAVETOREP) {
                repositoryDockVisible = true; applyDockVisibility();
                repositoryPanel->save_to_repository();
                break;
            }
			if(*(const ustring *)data == CMD_ELEMENTS) {
				paletteDockVisible = !paletteDockVisible;
				applyDockVisibility();
				break;
			}
			if(*(const ustring *)data == CMD_PROPERTY) {
				propertyDockVisible = !propertyDockVisible;
				applyDockVisibility();
				break;
			}
			if(*(const ustring *)data == CMD_COMPILERS) {
				buildSettingsDockVisible = !buildSettingsDockVisible;
				applyDockVisibility();
				break;
			}
			if(*(const ustring *)data == CMD_PANELS) {
                const bool visible = !(paletteDockVisible && propertyDockVisible && repositoryDockVisible && debugDockVisible && errorsDockVisible && terminalDockVisible && examplesDockVisible && buildSettingsDockVisible);
				paletteDockVisible = visible;
				propertyDockVisible = visible;
                repositoryDockVisible = visible;
                debugDockVisible = visible;
                errorsDockVisible = visible;
                terminalDockVisible = visible;
                examplesDockVisible = visible;
				buildSettingsDockVisible = visible;
				buildSettingsDockVisible = visible;
				applyDockVisibility();
				break;
			}
			if(*(const ustring *)data == CMD_QPAN_SHOWALL) {
				paletteDockVisible = true;
				propertyDockVisible = true;
                repositoryDockVisible = true;
                debugDockVisible = true;
                errorsDockVisible = true;
                terminalDockVisible = true;
                examplesDockVisible = true;
				buildSettingsDockVisible = true;
				buildSettingsDockVisible = true;
				applyDockVisibility();
				break;
			}
			if(*(const ustring *)data == CMD_QPAN_HIDEALL) {
				paletteDockVisible = false;
				propertyDockVisible = false;
                repositoryDockVisible = false;
                debugDockVisible = false;
                errorsDockVisible = false;
                terminalDockVisible = false;
                examplesDockVisible = false;
				buildSettingsDockVisible = false;
				buildSettingsDockVisible = false;
				applyDockVisibility();
				break;
			}
#endif
			if(*(const ustring *)data == CMD_FULLSCREEN) {
				if(window_is_fullscreen(*this))
					unfullscreen();
				else
					fullscreen();
			}
			else if(*(const ustring *)data == CMD_ADD_GROUP) {
				if(!sde || !pal->currentTab || sde->sdk->pack->isLegacy()) break;
				Dialog dialog("Add palette group");
				dialog.set_transient_for(*this);
				Entry name;
				name.set_activates_default(true);
				box_pack_start(*dialog.get_content_area(), name, false, false, 6);
				dialog.add_button(Stock::CANCEL, 0);
				dialog.add_button(Stock::ADD, 1);
				dialog.set_default_response(1);
				if(dialog_run(dialog) == 1) {
					ustring group = name.get_text();
					if(!group.empty()) {
						if(sde->sdk->pack->addGroup(pal->currentTab->caption, group))
							pal->addGroup(group);
						else
							debug.error("Could not add palette group: the name is empty, already exists, or the package database is not writable.");
					}
				}
			}

			break;
		case CBT_DEBUG_INFO:
			if(data) {
				if(*(const char*)data == '~')
					debug.text((const char*)data+1);
				else if( *(const char*)data == '!') {
#ifdef HIASM_GTK4
                    errors.error((const char*)data+1);
                    errorsDockVisible = true;
                    if(dockWorkspace && errorsDock) { applyDockVisibility(); dockWorkspace->activate_panel(*errorsDock); }
#else
                    debug.error((const char*)data+1);
#endif
                }
				else if( *(const char*)data == '@')
					debug.info((const char*)data+1);
				else if( *(const char*)data == '#')
					debug.trace((const char*)data+1);
				else
					debug.text((const char*)data);
			}
			else {
				debug.clear();
                errors.clear();
            }
			break;
		case CBT_CMD_ENABLED:
			if(pal->pack && !pal->pack->isLegacy()) cmdMan->enable(CMD_ADD_GROUP);
			pal->updateCommands();
#ifdef HIASM_GTK4
            updateRepositoryTarget(wmp->getCurrent());
            if(wmp->getCurrent() && wmp->getCurrent()->type == WMT_SDK_EDITOR) cmdMan->enable(CMD_SAVETOREP);
			updateDockCommands();
#endif
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
		// radio buttons: "property" shows the property editor, "points" shows the points list
		if(i)
			btn->signal_clicked().connect([this]() { pointsList.show(); propsFix.hide(); });
		else
			btn->signal_clicked().connect([this]() { pointsList.hide(); propsFix.show(); props.queue_resize(); props.queue_draw(); });
		prop_tools->add(*btn);
	}
	SeparatorToolItem *sp = new SeparatorToolItem();
	prop_tools->add(*sp);
	ToggleToolButton *btn = new ToggleToolButton();
	Image *img = new Image(Gdk::Pixbuf::create_from_file(dataDir + ICONS_PATH + "switch.png"));
	btn->set_icon_widget(*img);
	btn->set_tooltip_text(ui_tr("Show system properties"));
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
			if(!t)
				break;
			TData dt((TypePixbuf*)prop->value);
			dt = t->run(dt);
			((ElementProperty *)prop->data)->setValue(dt.toPixbuf());
			break;
		}
		case data_str:
		case data_list: {
			ElementCoreTransmitter *t = shaDlg.run("text_editor");
			if(!t)
				break;
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

class CompilerMenuItem : public RadioMenuItem {
	private:
		Compiler *compiler;
		Pack *pack;
	public:
		CompilerMenuItem(RadioButtonGroup group, Pack *pack, Compiler *compiler):RadioMenuItem(group, compiler->name) {
			this->compiler = compiler;
			this->pack = pack;
		}
		virtual void on_activate() {
			this->pack->setActiveCompiler(this->compiler->name);
		}
};

void MainGUI::fillCompilers(Pack *pack) {
	std::vector<Widget*> wlist = compilers.get_children();

	RadioButtonGroup group;
	for(std::vector<Widget*>::iterator item = wlist.begin(); item != wlist.end(); item++)
		compilers.remove(**item.base());

	if(pack) {
		for(Pack::CompilerItems::iterator c = pack->compilers.begin(); c != pack->compilers.end(); c++) {
			CompilerMenuItem *m = new CompilerMenuItem(group, pack, c->compiler);
			group = m->get_group();
			m->set_active(compilers.get_children().empty());
			compilers.append(*m);
		}
	}
	compilers.show();
}
