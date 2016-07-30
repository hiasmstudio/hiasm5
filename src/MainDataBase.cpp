/*
 * MainDataBase.cpp
 *
 *  Created on: 07.05.2010
 *      Author: dilma
 */

#include "MainDataBase.h"

MainDataBase mdb;

void MainDataBase::open() {
	if(sqlite3_open(databaseFile.c_str(), &db) != SQLITE_OK)
		std::cout << "Error open main database!" << std::endl;
}

void MainDataBase::close() {
	sqlite3_close(db);
}

sqlite3_stmt *MainDataBase::query(const ustring &sql) {
	sqlite3_stmt *result;
	sqlite3_prepare(db, sql.c_str(), -1, &result, NULL);
	if(!result)
		std::cout << "Sql query " << sql.c_str() << " return empty result!" << std::endl;
	return result;
}

void MainDataBase::exec(const ustring &sql) {
	sqlite3_exec(db, sql.c_str(), NULL, NULL, NULL);
}

ustring MainDataBase::read_loc() {
	sqlite3_stmt *r = query("select pref from localization where active = 1");
	sqlite3_step(r);
	ustring result = ustring("info_id_") + (const char *)sqlite3_column_text(r, 0);
	sqlite3_finalize(r);
	return result;
}

int MainDataBase::readInt(void *id, int i) {
	return sqlite3_column_int((sqlite3_stmt*)id, i);
}

ustring MainDataBase::readStr(void *id, int i) {
	if(!sqlite3_column_text((sqlite3_stmt*)id, i))
		return ustring();
	return (const char *)sqlite3_column_text((sqlite3_stmt*)id, i);
}

void *MainDataBase::begin_read_commands() {
	ustring loc = read_loc();
	return query(ustring("select commands.name,") + loc + ".info from commands," + loc + " where commands.info_id = " + loc + ".id order by " + loc + ".info");
}

bool MainDataBase::read_commands(void *id, CommandInfo &cmd) {
	if(sqlite3_step((sqlite3_stmt*)id) != SQLITE_DONE) {
		cmd.name = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 0);
		cmd.info = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 1);
		return true;
	}
	return false;
}

void MainDataBase::end_read(void *id) {
	sqlite3_finalize((sqlite3_stmt*)id);
}

void *MainDataBase::begin_read_command_menu(const ustring &name) {
	sqlite3_stmt *r = query(ustring("select id from menus where name = '") + name + "'");
	sqlite3_step(r);
	int m = sqlite3_column_int(r, 0);
	char buf[8];
	sprintf(buf, "%d", m);
	sqlite3_finalize(r);

	ustring loc = read_loc();
	return query(ustring("select commands.name,") + loc + ".info from menu_commands,commands," + loc + " where menu_commands.menu_id = " + buf + " AND menu_commands.cmd_id = commands.id AND commands.info_id = " + loc + ".id order by menu_commands.pos");
}

void *MainDataBase::begin_read_command_by_key(int key, bool shift, bool ctrl) {
	ustring s(shift?"1":"0");
	ustring c(ctrl?"1":"0");
	char buf[8];
	sprintf(buf, "%d", key);
	ustring sql(ustring("select name, (select '') from commands where key = ") + buf + " and shift = " + s + " and ctrl = " + c);
	return  query(sql);
}

void *MainDataBase::begin_read_packs() {
	ustring loc = read_loc();
	return query(ustring("select packs.name,") + loc + ".info,packs.codename,packs.units,packs.id from packs," + loc + " where packs.info_id = " + loc + ".id order by packs.id");
}

bool MainDataBase::read_packs(void *id, PackInfo &pack) {
	if(sqlite3_step((sqlite3_stmt*)id) != SQLITE_DONE) {
		pack.name = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 0);
		pack.info = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 1);
		pack.codeName = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 2);
		pack.units = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 3);
		pack.id = readInt(id, 4);
		return true;
	}
	return false;
}

void *MainDataBase::begin_read_projects(const ustring &name) {
	sqlite3_stmt *r = query(ustring("select id from packs where codename = '") + name + "'");
	sqlite3_step(r);
	int m = sqlite3_column_int(r, 0);
	sqlite3_finalize(r);

	ustring loc = read_loc();
	return query(ustring("select projects.entry, def.info as name, pinf.info as info, projects.ext, projects.make ") +
            "from projects, " + loc + " as pinf, " + loc + " as def " +
            "where projects.pack_id = " + int_to_str(m) + " AND pinf.id = projects.info_id AND def.id = projects.name_id");
}

bool MainDataBase::read_projects(void *id, ProjectInfo &project) {
	if(sqlite3_step((sqlite3_stmt*)id) != SQLITE_DONE) {
		project.entry = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 0);
		project.name = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 1);
		project.info = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 2);
		project.ext = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 3);
		project.make = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 4);
		return true;
	}
	return false;
}

void *MainDataBase::begin_read_settings(int pid) {
	ustring loc = read_loc();
	return query(ustring("select p.id, p.pid, p.name, ti.info as title, ii.info as info, p.value, p.type, p.list ") +
            "from params as p, " + loc + " as ti, " + loc + " as ii " +
            "where p.pid = " + int_to_str(pid) + " AND ii.id = p.info_id AND ti.id = p.title_id");
}

bool MainDataBase::read_settings(void *id, ParamInfo &param) {
	if(sqlite3_step((sqlite3_stmt*)id) != SQLITE_DONE) {
		param.id = readInt(id, 0);
		param.pid = readInt(id, 1);
		param.name = readStr(id, 2);
		param.title = readStr(id, 3);
		param.info = readStr(id, 4);
		param.value = readStr(id, 5);
		param.type = readInt(id, 6);
		param.list = readStr(id, 7);
		return true;
	}
	return false;
}

void *MainDataBase::begin_read_settings_value() {
	return query(ustring("select id, value, type from params where type > 0"));
}

bool MainDataBase::read_settings_value(void *id, ParamInfo &param) {
	if(sqlite3_step((sqlite3_stmt*)id) != SQLITE_DONE) {
		param.id = readInt(id, 0);
		param.value = readStr(id, 1);
		param.type = readInt(id, 2);
		return true;
	}
	return false;
}

void MainDataBase::save_settings_value(int id, const ustring &value) {
	exec(ustring("update params set value = '") + value + "' where id = " + int_to_str(id));
}

void *MainDataBase::begin_read_compilers() {
	return query(ustring("select id, name, cmd, path, ext from compilers order by id"));
}

bool MainDataBase::read_compilers(void *id, CompilerInfo &compiler) {
	if(sqlite3_step((sqlite3_stmt*)id) != SQLITE_DONE) {
		compiler.id = readInt(id, 0);
		compiler.name = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 1);
		compiler.cmd = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 2);
		compiler.path = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 3);
		compiler.ext = (const char *)sqlite3_column_text((sqlite3_stmt*)id, 4);
		return true;
	}
	return false;
}

void *MainDataBase::begin_read_pack_compilers(const ustring &name) {
	sqlite3_stmt *r = query(ustring("select id from packs where codename = '") + name + "'");
	sqlite3_step(r);
	int m = sqlite3_column_int(r, 0);
	sqlite3_finalize(r);

	return query(ustring("SELECT cmp_id, active FROM pack_compilers WHERE pack_id = " + int_to_str(m)));
}

bool MainDataBase::read_pack_compilers(void *id, PackCompilerInfo &compiler) {
	if(sqlite3_step((sqlite3_stmt*)id) != SQLITE_DONE) {
		compiler.compiler = readInt(id, 0);
		compiler.active = readInt(id, 1) == 1;
		return true;
	}
	return false;
}
