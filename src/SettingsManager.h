/*
 * SettingsManager.h
 *
 *  Created on: Jul 15, 2010
 *      Author: dilma
 */

#ifndef SETTINGSMANAGER_H_
#define SETTINGSMANAGER_H_

#include "share.h"
#include "PropEditor.h"

enum SettingsIDS {
	IDS_MASK_OF_NAME = 134,
	IDS_PROJECTS_DIR = 135,
	IDS_NEW_PROJECT  = 136,
};

class SettingsManager : public Dialog, public CallBack {
	private:
		TreeView menu;
		PropEditor props;
		EventBox pFixed;
		ScrolledWindow pScroll;
		HPaned hpaned;
		VPaned vpaned;
		TextView info;

		Glib::RefPtr<ListStore> menuStore;
		const struct MenuModelColumns : public TreeModelColumnRecord {
			Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> > icon;
			Gtk::TreeModelColumn<ustring> name;
			Gtk::TreeModelColumn<ustring> info;
			Gtk::TreeModelColumn<int> id;

			MenuModelColumns() { add(icon); add(name); add(info); add(id); }
		} menuColumns;

		virtual void props_on_size_allocate(Allocation &alloc);
		virtual void on_show_ep_row_activated();

		virtual void callback(void *owner, CallbackType type, const void *data);

		void fillProps(int pid, PropertyItem *group = NULL);
	public:
		Event on_change_settings;

		SettingsManager();

		static void show();
		static void init();

		static int readInt(SettingsIDS id);
		static double readReal(SettingsIDS id);
		static ustring &readStr(SettingsIDS id);
		static TypeColor readColor(SettingsIDS id);

		static ustring readProjectsDir();
};

extern SettingsManager *settingsMan;

#endif /* SETTINGSMANAGER_H_ */
