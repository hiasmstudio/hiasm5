/*
 * main_gui.h
 *
 *  Created on: 01.05.2010
 *      Author: dilma
 */

#ifndef MAIN_GUI_H_
#define MAIN_GUI_H_

#include "share.h"
#include "commands.h"
#include "SDK_Editor.h"
#include "PropEditor.h"
#include "Palette.h"
#include "DebugPanel.h"
#include "Pack.h"
#include "SHADialogs.h"
#include "WindowManager.h"
#include "SplashScreen.h"
#include "ArrayEditor.h"
#include "DockWorkspace.h"
#ifdef HIASM_GTK4
#include "RepositoryPanel.h"
#include "ExamplesCatalogPanel.h"
#include "BuildSettingsPanel.h"
#include "TerminalPanel.h"
#endif

class MainGUI : public Window, public CallBack {
	private:
		Palette *pal;
		PropEditor props;
		EventBox propsFix;
		DebugPanel debug;
        DebugPanel errors;
		Toolbar *main_tools;
		TextView propInfo;
		WindowManager *wmp;
		Toolbar *prop_tools;
		Menu compilers;

#ifdef HIASM_GTK4
        DockWorkspace *dockWorkspace = nullptr;
        DockPanel *paletteDock = nullptr;
        DockPanel *propertyDock = nullptr;
        DockPanel *repositoryDock = nullptr;
        DockPanel *debugDock = nullptr;
        DockPanel *examplesDock = nullptr;
        DockPanel *buildSettingsDock = nullptr;
        DockPanel *errorsDock = nullptr;
        DockPanel *terminalDock = nullptr;
        Gtk::Widget *propertyDockContent = nullptr;
        Gtk::Widget *debugDockContent = nullptr;
        Gtk::Widget *errorsDockContent = nullptr;
        RepositoryPanel *repositoryPanel = nullptr;
        ExamplesCatalogPanel *examplesPanel = nullptr;
        BuildSettingsPanel *buildSettingsPanel = nullptr;
        TerminalPanel *terminalPanel = nullptr;
        int paletteDockSide = 0;
        int propertyDockSide = 1;
        int repositoryDockSide = 1;
        int examplesDockSide = 1;
        bool paletteDockVisible = true;
        bool propertyDockVisible = true;
        bool paletteDockAttached = true;
        bool propertyDockAttached = true;
        bool repositoryDockVisible = true;
        bool repositoryDockAttached = true;
        bool debugDockVisible = true;
        bool debugDockAttached = true;
        bool examplesDockVisible = true;
        bool examplesDockAttached = true;
        bool buildSettingsDockVisible = true;
        bool buildSettingsDockAttached = true;
        bool errorsDockVisible = false;
        bool errorsDockAttached = true;
        bool terminalDockVisible = false;
        bool terminalDockAttached = true;
        int paletteDockWidth = 240;
        int propertyDockWidth = 300;
        int repositoryDockWidth = 300;
        int examplesDockWidth = 260;
        int buildSettingsDockWidth = 280;
        int debugDockHeight = 180;
        int errorsDockHeight = 180;
        int terminalDockHeight = 220;
        double paletteFloatX = 40, paletteFloatY = 40;
        double propertyFloatX = 40, propertyFloatY = 40;
        double repositoryFloatX = 40, repositoryFloatY = 40;
        double examplesFloatX = 40, examplesFloatY = 40;
        double buildSettingsFloatX = 40, buildSettingsFloatY = 40;
        double debugFloatX = 40, debugFloatY = 40;
        double errorsFloatX = 40, errorsFloatY = 40;
        double terminalFloatX = 40, terminalFloatY = 40;
        void createDocks();
        void applyDockVisibility();
        void applyDockAttachment();
        void updateDockCommands();
        void syncDockState();
        void updateRepositoryTarget(WM_Tab *tab);
#endif

		SDK_Editor *sde;
		TreeView pointsList;

		ArrayEditor arrEditor;

		SelectManager *curSelMan;

		SplashScreen splash;

		ustring lastSelect;			/**< contain path of last selected property in PropEditor */

		bool showSystemProps;

		Glib::RefPtr<ListStore> pointsListStore;
		const struct PointsListModelColumns : public TreeModelColumnRecord {
			Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > icon;
			Gtk::TreeModelColumn<bool> visible;
			Gtk::TreeModelColumn<ustring> param;
			Gtk::TreeModelColumn<ConfMethod*> mtd;

			PointsListModelColumns() { add(icon); add(visible); add(param); add(mtd); }
		} pointsListColumns;

		//-----interface--------------------------------------------------------------------------------------
		VBox box1;     // box for (toolbar) + (sdk and bottom panels)
		VPaned pan1;   // paned for (sdk) + (bottom panels)
		HPaned pan2;   // paned for (palette) + (sdk and property panel)
		HPaned pan3;   // paned for (sdk) + (property panel)
		//-------------------------------------------------------------------------------------------

		virtual void props_on_size_allocate(Allocation &alloc);
		virtual void on_show();
		virtual void on_prop_tools_click();
		virtual void on_show_sysprops_click();

		virtual void on_show_element_point(const Glib::ustring &path);
		virtual void on_show_ep_row_activated();

		void fillProps(SelectManager *selMan);
		void changeProps(const PropertyItem *prop);
		void editProps(const PropertyItem *prop);

		void fillPropTools();

		void fillCompilers(Pack *pack);
	protected:
		virtual void callback(void *owner, CallbackType type, const void *data);
	public:
		MainGUI();
		~MainGUI();
		void init();
};

extern Window *form;

#endif /* MAIN_GUI_H_ */
