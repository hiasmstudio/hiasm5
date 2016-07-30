/*
 * ProjectManager.cpp
 *
 *  Created on: 18.05.2010
 *      Author: dilma
 */

#include "ProjectManager.h"

ProjectManager::ProjectManager() {
	packs = NULL;
	selectPack = NULL;
	selectProject = NULL;
}

void ProjectManager::init() {
	set_title("Project manager");
	set_modal(true);
	set_size_request(400, 300);

	VBox *vb = new VBox();
	vb->set_spacing(2);

	packs = new IconView();
//	packs->set_size_request(100, 183);

	packList = Gtk::ListStore::create(m_columns);
	for(PackCollection::iterator p = packSet->begin(); p != packSet->end(); p++) {
		Gtk::TreeRow row = *(packList->append());
		row[m_columns.name] = (*p)->title;
		row[m_columns.info] = (*p)->info;
		row[m_columns.pixbuf] = (*p)->getIcon();
		row[m_columns.codeName] = (*p)->name;
	}
	packs->set_column_spacing(0);
	packs->set_model(packList);
	packs->set_text_column(m_columns.name);
	packs->set_pixbuf_column(m_columns.pixbuf);
	packs->signal_selection_changed().connect(sigc::mem_fun(*this, &ProjectManager::on_selection_pack));

	ScrolledWindow *sc = new ScrolledWindow();
	sc->set_policy(POLICY_NEVER, POLICY_AUTOMATIC);
	sc->add(*packs);
	sc->set_size_request(-1, 80);
	vb->pack_start(*sc, false, false);

	projects = new IconView();
	prjList = Gtk::ListStore::create(m_prj_columns);
	projects->set_column_spacing(0);
	projects->set_model(prjList);
	projects->set_text_column(m_prj_columns.name);
	projects->set_pixbuf_column(m_prj_columns.pixbuf);
	projects->signal_selection_changed().connect(sigc::mem_fun(*this, &ProjectManager::on_selection_project));
	projects->set_size_request(-1, 160);
	vb->add(*projects);

	/*
	HButtonBox *bb = new HButtonBox();
	bb->set_layout(BUTTONBOX_END);
	bb->set_spacing(2);
	Button *b = new Button(Gtk::Stock::CANCEL);
	b->signal_clicked().connect(sigc::mem_fun(*this, &ProjectManager::on_cancel_click));
	bb->add(*b);
	okBtn = new Button(Gtk::Stock::OK);
	okBtn->set_sensitive(false);
	okBtn->signal_clicked().connect(sigc::mem_fun(*this, &ProjectManager::on_ok_click));
	bb->add(*okBtn);
	vb->pack_end(*bb, false, false);
	*/

	add_button(Gtk::Stock::CANCEL, 0);
	add_button(Gtk::Stock::OK, 1);
	(*get_action_area()->get_children().begin())->set_sensitive(false);

	info = new TextView();
	info->set_size_request(100, 30);
	vb->pack_end(*info, false, false);

	get_content_area()->add(*vb);
	vb->show_all();
}

void ProjectManager::on_selection_pack() {
	prjList->clear();
	if(packs->get_selected_items().empty()) {
		info->get_buffer()->set_text("");
	}
	else {
		TreePath p = (*packs->get_selected_items().begin());
		Gtk::TreeRow row = *packList->get_iter(p);
		info->get_buffer()->set_text(row[m_columns.info]);

		selectPack = packSet->getPackByName(row[m_columns.codeName]);
		for(PackProjects::iterator p = selectPack->projects.begin(); p != selectPack->projects.end(); p++) {
			//std::cout << (*p)->name.c_str() << std::endl;
			Gtk::TreeRow row = *(prjList->append());
			row[m_prj_columns.name] = (*p)->name;
			row[m_prj_columns.info] = (*p)->info;
			row[m_prj_columns.pixbuf] = Gdk::Pixbuf::create_from_file(selectPack->pathElementIcon((*p)->entry));
			row[m_prj_columns.entry] = (*p)->entry;
		}
	}
}

void ProjectManager::on_selection_project() {
	(*get_action_area()->get_children().begin())->set_sensitive(!projects->get_selected_items().empty());

	if(projects->get_selected_items().empty()) {
		info->get_buffer()->set_text("");
	}
	else {
		TreePath p = (*projects->get_selected_items().begin());
		Gtk::TreeRow row = *prjList->get_iter(p);
		selectProject = selectPack->getProjectByEntry(ustring(row[m_prj_columns.entry]));
		info->get_buffer()->set_text(row[m_prj_columns.info]);
	}
}

int ProjectManager::open() {
	if(!packs)
		init();
	return run();
}
