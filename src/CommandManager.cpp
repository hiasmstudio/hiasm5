/*
 * CommandManager.cpp
 *
 *  Created on: 07.05.2010
 *      Author: dilma
 */

#include "CommandManager.h"
#include "MainDataBase.h"
#include "commands.h"

CommandManager *cmdMan;

CommandManager::CommandManager():
	on_command(this, CBT_RUN_COMMAND),
	on_cmd_enabled(this, CBT_CMD_ENABLED)
{
	loadCommandList();
}

TypePixbuf CommandManager::loadIcon(const ustring &name) {
	ustring icon_file = dataDir + ICONS_PATH + name + ".png";
	if(Glib::file_test(icon_file, Glib::FILE_TEST_EXISTS))
		return Gdk::Pixbuf::create_from_file(icon_file);

	return TypePixbuf();
}

void CommandManager::loadCommandList() {
	void *id = mdb.begin_read_commands();
	CommandInfo cmd;
	while(mdb.read_commands(id, cmd)) {
		CommandItem *ci = new CommandItem();
		ci->info = cmd.info;
		ci->name = cmd.name;
		ci->icon = loadIcon(cmd.name);
		cmds.push_back(ci);
	}
	mdb.end_read(id);
}

CommandItem *CommandManager::findByName(const ustring &name) {
	for(Commands::iterator c = cmds.begin(); c != cmds.end(); c++)
		if((*c)->name == name)
			return *c;
	return NULL;
}

Image *createImageByCmd(CommandItem *ci) {
	if(ci->name == CMD_ZOOMIN)
		return new Image(Gtk::Stock::ZOOM_IN, IconSize(ICON_SIZE_MENU));
	else if(ci->name == CMD_ZOOMOUT)
		return new Image(Gtk::Stock::ZOOM_OUT, IconSize(ICON_SIZE_MENU));
	else if(ci->name == CMD_SELECTALL)
		return new Image(Gtk::Stock::SELECT_ALL, IconSize(ICON_SIZE_MENU));
	else if(ci->name == CMD_SEARCH)
		return new Image(Gtk::Stock::FIND, IconSize(ICON_SIZE_MENU));
	else if(ci->name == CMD_FULLSCREEN)
		return new Image(Gtk::Stock::FULLSCREEN, IconSize(ICON_SIZE_MENU));
//	else if(ci->name == CMD_BUILD)
//		return new Image(Gtk::Stock::EXECUTE, IconSize(ICON_SIZE_MENU));
	else if(ci->icon)
		return new Image(ci->icon);

	return NULL;
}

Toolbar *CommandManager::createToolbar(const ustring &name) {
	Toolbar *tb = new Toolbar();
	tb->set_toolbar_style(TOOLBAR_ICONS);

	void *id = mdb.begin_read_command_menu(name);
	CommandInfo old, cmd;
	ToolButton *btn;
	Menu *menu = NULL;
	CM_ImageMenuItem *mi;
	while(mdb.read_command_menu(id, cmd)) {
		btn = NULL;
		mi = NULL;
		if(cmd.name == "(") {
			btn = new CM_MenuToolButton(this, old.name, old.info);
			menu = new Menu();
			((MenuToolButton*)btn)->set_menu(*menu);
		}
		else {
			if(old.name == "-") {
				if(menu) {
					SeparatorMenuItem *m = new SeparatorMenuItem();
					m->show();
					menu->append(*m);
				}
				else
					tb->add(*(new SeparatorToolItem()));
			}
			else if(old.name.length() > 1) {
				if(menu) {
					mi = new CM_ImageMenuItem(this, old.name, old.info);
					CommandItem *ci = findByName(old.name);
					if(ci) {
						ci->widgets.push_back(mi);
						if(Image *i = createImageByCmd(ci))
							mi->set_image(*i);
					}
					menu->append(*mi);
				}
				else {
					if(old.name == CMD_BUILD)
						btn = new CM_MenuToolButton(this, old.name, old.info);
					else
						if(old.name == CMD_FORMEDIT)
							btn = new CM_ToggleToolButton(this, old.name, old.info);
						else
							btn = new CM_ToolButton(this, old.name, old.info);
				}
			}
		}

		if(btn) {
			btn->set_tooltip_text(old.info);
			CommandItem *ci = findByName(old.name);
			if(ci) {
				ci->widgets.push_back(btn);
				if(Image *i = createImageByCmd(ci))
					btn->set_icon_widget(*i);
			}
			tb->add(*btn);
		}

		if(old.name == ")") {
			menu = NULL;
		}
		old = cmd;
	}
	mdb.end_read(id);

	widgets.push_back(tb);

	return tb;
}

MenuShell *CommandManager::fillMenu(MenuShell *menu, const ustring &name) {
	void *id = mdb.begin_read_command_menu(name);
	CommandInfo cmd;
	CM_ImageMenuItem *mi = NULL;
	std::list<MenuShell*> menus;
	while(mdb.read_command_menu(id, cmd)) {
		if(cmd.name == "(") {
			menus.push_front(menu);
			menu = new Menu();
			mi->set_submenu(*(Menu*)menu);
		}
		else if(cmd.name == ")") {
			menu = *menus.begin();
			menus.pop_front();
		}
		else if(cmd.name == "-") {
			SeparatorMenuItem *m = new SeparatorMenuItem();
			m->show();
			menu->append(*m);
		}
		else {
			mi = new CM_ImageMenuItem(this, cmd.name, cmd.info);
			CommandItem *ci = findByName(cmd.name);
			if(ci) {
				ci->widgets.push_back(mi);
				if(Image *i = createImageByCmd(ci))
					mi->set_image(*i);
			}
			menu->append(*mi);
		}
	}
	mdb.end_read(id);

	widgets.push_back(menu);

	return menu;
}

void CommandManager::command(const ustring &name) {
	on_command.run(&name);
}

void CommandManager::beginUpdate() {
	for(Commands::iterator c = cmds.begin(); c != cmds.end(); c++) {
		(*c)->checked = false;
		//(*c)->enabled = (*c)->info.empty();
		(*c)->enabled = false;
	}

	on_cmd_enabled.run(this);
}

CommandItem *CommandManager::getComandByName(CMD_TYPE codeName) {
	for(Commands::iterator c = cmds.begin(); c != cmds.end(); c++)
		if((*c)->name == codeName)
			return *c;
	return NULL;
}

bool CommandManager::enable(CMD_TYPE cmd) {
	CommandItem *ci = getComandByName(cmd);
	if(ci)
		return ci->enabled = true;

	DEBUG_MSG("WARNING: command " << cmd << " not found!")
	return false;
}

bool CommandManager::isEnabled(CMD_TYPE cmd) {
	CommandItem *ci = getComandByName(cmd);
	if(ci)
		return ci->enabled;

	DEBUG_MSG("WARNING: command " << cmd << " not found!")
	return false;
}

bool CommandManager::check(CMD_TYPE cmd) {
	CommandItem *ci = getComandByName(cmd);
	if(ci)
		return ci->checked = true;

	DEBUG_MSG("WARNING: command " << cmd << " not found!")
	return false;
}

void CommandManager::endUpdate() {
	for(Commands::iterator c = cmds.begin(); c != cmds.end(); c++) {
		for(CommandWidgets::iterator w = (*c)->widgets.begin(); w != (*c)->widgets.end(); w++) {
			(*w)->set_sensitive((*c)->enabled);
			CM_ToggleToolButton *b = dynamic_cast<CM_ToggleToolButton*>(*w);
			if(b)
				b->check((*c)->checked);
		}
	}
}

void CommandManager::attachMenu(CMD_TYPE name, Menu &menu) {
	CommandItem *cmd = getComandByName(name);
	if(cmd) {
		// only one attach...
		for(CommandWidgets::iterator w = cmd->widgets.begin(); w != cmd->widgets.end(); w++) {
			CM_MenuToolButton *b = dynamic_cast<CM_MenuToolButton*>(*w);
			if(b) {
				b->set_menu(menu);
				return;
			}
			ImageMenuItem *m = dynamic_cast<ImageMenuItem*>(*w);
			if(m) {
				m->set_submenu(menu);
				return;
			}
		}
	}
}

//---------------------------------------------------------------------------------------------------------------

CM_ToolButton::CM_ToolButton(CommandManager *parent, const ustring &name, const ustring &info):ToolButton(info) {
	this->cmd = name;
	this->parent = parent;
}

void CM_ToolButton::on_clicked() {
	parent->command(cmd);
}

CM_ToggleToolButton::CM_ToggleToolButton(CommandManager *parent, const ustring &name, const ustring &info):ToggleToolButton(info) {
	this->cmd = name;
	this->parent = parent;
	_change = false;
}

void CM_ToggleToolButton::on_clicked() {
	if(!_change)
		parent->command(cmd);
}

void CM_ToggleToolButton::check(bool value) {
	_change = true;
	set_active(value);
	_change = false;
}

CM_MenuToolButton::CM_MenuToolButton(CommandManager *parent, const ustring &name, const ustring &info):MenuToolButton(info) {
	this->cmd = name;
	this->parent = parent;
}

void CM_MenuToolButton::on_clicked() {
	parent->command(cmd);
}

CM_ImageMenuItem::CM_ImageMenuItem(CommandManager *parent, const ustring &name, const ustring &info):ImageMenuItem(info) {
	this->cmd = name;
	this->parent = parent;
	show();
}

void CM_ImageMenuItem::on_activate() {
	parent->command(cmd);
}
