/*
 * ProjectManager.h
 *
 *  Created on: 18.05.2010
 *      Author: dilma
 */

#ifndef PROJECTMANAGER_H_
#define PROJECTMANAGER_H_

#include "share.h"
#include "Pack.h"

class ProjectManager : public Dialog {
	private:
		IconView *packs;
		IconView *projects;
		TextView *info;
		Button *okBtn;

		class ModelColumns : public Gtk::TreeModelColumnRecord {
			public:
				Gtk::TreeModelColumn<std::string> name;
				Gtk::TreeModelColumn<Glib::ustring> info;
				Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> >  pixbuf;
				Gtk::TreeModelColumn<Glib::ustring> codeName;

				ModelColumns() { add(name); add(info); add(pixbuf); add(codeName); }
		};
		const ModelColumns m_columns;
		Glib::RefPtr<ListStore> packList;

		class PrjModelColumns : public Gtk::TreeModelColumnRecord {
			public:
				Gtk::TreeModelColumn<std::string> name;
				Gtk::TreeModelColumn<Glib::ustring> info;
				Gtk::TreeModelColumn< Glib::RefPtr<Gdk::Pixbuf> >  pixbuf;
				Gtk::TreeModelColumn<std::string> entry;

				PrjModelColumns() { add(name); add(info); add(pixbuf); add(entry); }
		};
		const PrjModelColumns m_prj_columns;
		Glib::RefPtr<ListStore> prjList;

		/**
		 * Create manager
		 */
		void init();

		virtual void on_selection_pack();
		virtual void on_selection_project();
	public:
		Pack *selectPack;
		PackProject *selectProject;

		ProjectManager();

		/**
		 * Create manager if not exists and show it
		 * @return 1 if user select valid project
		 */
		int open();
};

#endif /* PROJECTMANAGER_H_ */
