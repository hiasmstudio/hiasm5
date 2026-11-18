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
	set_title(ui_tr("Project manager"));
	set_modal(true);
	set_size_request(400, 360);

	VBox *vb = new VBox();
	vb->set_spacing(2);

	packs = new IconView();
//	packs->set_size_request(100, 183);

	packList = Gtk::ListStore::create(m_columns);
	// Core is a native package as well: keep it in the project manager instead
	// of hiding it, so its hcTplForm/hcTplDialog templates remain creatable.
	for(PackCollection::iterator p = packSet->begin(); p != packSet->end(); p++) {
		Gtk::TreeRow row = *(packList->append());
		row[m_columns.name] = (*p)->name == "cpp" ? ui_tr("C++ package") : (*p)->title;
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
	scrolled_set_child(*sc, *packs);
	sc->set_size_request(-1, 90);

	projects = new IconView();
	prjList = Gtk::ListStore::create(m_prj_columns);
	projects->set_column_spacing(0);
	projects->set_model(prjList);
	projects->set_text_column(m_prj_columns.name);
	projects->set_pixbuf_column(m_prj_columns.pixbuf);
	projects->signal_selection_changed().connect(sigc::mem_fun(*this, &ProjectManager::on_selection_project));

	// The projects list needs its own scroller: a bare icon view reports the width of all its items in one row
	// and would stretch the dialog to that width.
	ScrolledWindow *scProjects = new ScrolledWindow();
	scProjects->set_policy(POLICY_NEVER, POLICY_AUTOMATIC);
	scrolled_set_child(*scProjects, *projects);
	scProjects->set_size_request(-1, 190);

	// The packs and the projects are separated by a horizontal splitter: the user drags it to give either list more room.
	VPaned *split = new VPaned();
	split->set_wide_handle(true);
	paned_pack1(*split, *sc, false, false);
	paned_pack2(*split, *scProjects, true, false);
	split->set_position(120);
	vb->pack_start(*split, true, true);

	/*
	HButtonBox *bb = new HButtonBox();
	bb->set_layout(BUTTONBOX_END);
	bb->set_spacing(2);
	Button *b = new Button(Stock::CANCEL);
	b->signal_clicked().connect(sigc::mem_fun(*this, &ProjectManager::on_cancel_click));
	bb->add(*b);
	okBtn = new Button(Stock::OK);
	okBtn->set_sensitive(false);
	okBtn->signal_clicked().connect(sigc::mem_fun(*this, &ProjectManager::on_ok_click));
	bb->add(*okBtn);
	vb->pack_end(*bb, false, false);
	*/

	add_button(Stock::CANCEL, 0);
	okBtn = add_button(Stock::OK, 1);
	okBtn->set_sensitive(false);
	// GTK4: action area removed

	info = new TextView();
	info->set_size_request(380, 44);
	// Without wrapping, a long pack description sets the natural width of the dialog and it grows to
	// the whole description line.
	info->set_wrap_mode(Gtk::WrapMode::WORD);
	info->set_left_margin(4);
	info->set_right_margin(4);
	vb->pack_end(*info, false, false);

	// Select a pack right away: the projects list then shows its creation types instead of an empty
	// pane, so the splitter between the packs and the projects is visible as soon as the dialog opens.
	// The C++ package comes first because it is the package the IDE itself builds.
	Gtk::TreeModel::iterator defaultPack = packList->children().empty() ? Gtk::TreeModel::iterator()
			: packList->children().begin();
	for(Gtk::TreeModel::iterator row = packList->children().begin(); row != packList->children().end(); row++)
		if((*row)[m_columns.codeName] == "cpp") { defaultPack = row; break; }
	if(defaultPack)
		packs->select_path(packList->get_path(defaultPack));

	box_pack_start(*get_content_area(), *vb, true, true, 0);
	vb->show();
}

void ProjectManager::on_selection_pack() {
	selectPack = NULL;
	selectProject = NULL;
	okBtn->set_sensitive(false);
	prjList->clear();
	if(packs->get_selected_items().empty()) {
		info->get_buffer()->set_text("");
	}
	else {
		TreePath p = (*packs->get_selected_items().begin());
		Gtk::TreeRow row = *packList->get_iter(p);
		info->get_buffer()->set_text(row[m_columns.info]);

		selectPack = packSet->getPackByName(row[m_columns.codeName]);
		Gtk::TreeModel::iterator defaultProject;
		for(PackProjects::iterator p = selectPack->projects.begin(); p != selectPack->projects.end(); p++) {
			// The IDE has no plugin loader yet, so the HiAsm plugin library is not
			// offered as a creation type. Gui cannot host the standard widgets (its runtime
			// lacks their helpers); the C++ GUI creation type is MainForm.
			if(selectPack->name == "cpp" && ((*p)->entry == "Plugin" || (*p)->entry == "Gui")) continue;
			Gtk::TreeModel::iterator current = prjList->append();
			Gtk::TreeRow row = *current;
			row[m_prj_columns.name] = (*p)->name;
			row[m_prj_columns.info] = (*p)->info;
			row[m_prj_columns.pixbuf] = Gdk::Pixbuf::create_from_file(selectPack->pathElementIcon((*p)->entry));
			row[m_prj_columns.entry] = (*p)->entry;
			if(selectPack->name == "cpp" && (*p)->entry == "MainForm")
				defaultProject = current;
		}
		if(defaultProject) {
			projects->select_path(prjList->get_path(defaultProject));
			on_selection_project();
		}
	}
}

void ProjectManager::on_selection_project() {
	selectProject = NULL;
	okBtn->set_sensitive(false);

	if(projects->get_selected_items().empty()) {
		info->get_buffer()->set_text("");
	}
	else {
		TreePath p = (*projects->get_selected_items().begin());
		Gtk::TreeRow row = *prjList->get_iter(p);
		selectProject = selectPack->getProjectByEntry(ustring(row[m_prj_columns.entry]));
		okBtn->set_sensitive(selectProject != NULL);
		info->get_buffer()->set_text(row[m_prj_columns.info]);
	}
}

int ProjectManager::open() {
	if(!packs)
		init();
	const int response = dialog_run(*this);
	return response == 1 && selectProject ? 1 : 0;
}
