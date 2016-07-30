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

class MainGUI : public Window, public CallBack {
	private:
		Palette *pal;
		PropEditor props;
		EventBox propsFix;
		DebugPanel debug;
		Toolbar *main_tools;
		TextView propInfo;
		WindowManager *wmp;
		Toolbar *prop_tools;
		Menu compilers;

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
