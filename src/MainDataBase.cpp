/*
 * MainDataBase.cpp
 *
 *  Created on: 07.05.2010
 *      Author: dilma
 */

#include "MainDataBase.h"
#include "Localization.h"
#include <glib.h>
#include <vector>

MainDataBase mdb;

void MainDataBase::open() {
	close();
	if(sqlite3_open(databaseFile.c_str(), &db) != SQLITE_OK) {
		std::cerr << "Unable to open main database \"" << databaseFile.c_str() << "\": "
			<< (db ? sqlite3_errmsg(db) : "SQLite open failed") << std::endl;
		close();
		return;
	}
	const char *required[][10] = {
		{"localization", "pref", "active", NULL},
		{"commands", "name", "info_id", "key", "shift", "ctrl", NULL},
		{"menus", "name", "info_id", NULL},
		{"menu_commands", "menu_id", "cmd_id", "pos", NULL},
		{"packs", "codename", "name", "info_id", "units", NULL},
		{"projects", "pack_id", "entry", "name_id", "info_id", "ext", "make", NULL},
		{"params", "id", "pid", "name", "title_id", "info_id", "value", "type", "list", NULL},
		{"compilers", "name", "cmd", "path", "ext", NULL},
		{"pack_compilers", "pack_id", "cmp_id", "active", NULL}
	};
	for(const auto &table : required) {
		sqlite3_stmt *check = NULL;
		int rc = sqlite3_prepare_v2(db, "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?1", -1, &check, NULL);
		if(rc != SQLITE_OK) {
			std::cerr << "Main database \"" << databaseFile.c_str()
				<< "\" schema check failed while looking for table " << table[0] << ": "
				<< sqlite3_errmsg(db) << std::endl;
			if(check) sqlite3_finalize(check);
			close();
			return;
		}
		rc = sqlite3_bind_text(check, 1, table[0], -1, SQLITE_STATIC);
		if(rc != SQLITE_OK) {
			std::cerr << "Main database \"" << databaseFile.c_str()
				<< "\" schema check failed for table " << table[0] << ": "
				<< sqlite3_errmsg(db) << std::endl;
			sqlite3_finalize(check);
			close();
			return;
		}
		rc = sqlite3_step(check);
		if(rc != SQLITE_ROW) {
			std::cerr << "Main database \"" << databaseFile.c_str()
				<< "\" is missing required table " << table[0];
			if(rc != SQLITE_DONE)
				std::cerr << " (SQLite: " << sqlite3_errmsg(db) << ")";
			std::cerr << std::endl;
			sqlite3_finalize(check);
			close();
			return;
		}
		sqlite3_finalize(check);

		for(size_t i = 1; table[i]; ++i) {
			check = NULL;
			rc = sqlite3_prepare_v2(db,
				"SELECT 1 FROM pragma_table_info(?1) WHERE name = ?2",
				-1, &check, NULL);
			if(rc == SQLITE_OK)
				rc = sqlite3_bind_text(check, 1, table[0], -1, SQLITE_STATIC);
			if(rc == SQLITE_OK)
				rc = sqlite3_bind_text(check, 2, table[i], -1, SQLITE_STATIC);
			if(rc != SQLITE_OK) {
				std::cerr << "Main database \"" << databaseFile.c_str()
					<< "\" schema check failed for " << table[0] << "." << table[i]
					<< ": " << sqlite3_errmsg(db) << std::endl;
				if(check) sqlite3_finalize(check);
				close();
				return;
			}
			rc = sqlite3_step(check);
			if(rc != SQLITE_ROW) {
				std::cerr << "Main database \"" << databaseFile.c_str()
					<< "\" is missing required column " << table[0] << "." << table[i];
				if(rc != SQLITE_DONE)
					std::cerr << " (SQLite: " << sqlite3_errmsg(db) << ")";
				std::cerr << std::endl;
				sqlite3_finalize(check);
				close();
				return;
			}
			sqlite3_finalize(check);
		}
	}
	if(!migrateRuntime())
		std::cerr << "Main database migration was not completed; continuing with existing data" << std::endl;
    if(initializedUserProfile && databaseFile == homeDir + DATABASE_FILE) {
        setLanguage(ui_system_language());
        initializedUserProfile = false;
    }
	ustring loc = read_loc();
	if(loc == "info_id_en") ui_set_language("en");
	else ui_set_language("ru");
}

bool MainDataBase::migrateRuntime() {
	if(!db) return false;
	char *error = NULL;
	if(sqlite3_exec(db, "BEGIN IMMEDIATE", NULL, NULL, &error) != SQLITE_OK) {
		std::cerr << "Unable to begin main database migration: " << (error ? error : sqlite3_errmsg(db)) << std::endl;
		if(error) sqlite3_free(error);
		return false;
	}
	auto fail = [&](const char *message) {
		std::cerr << "Main database migration failed: " << message << std::endl;
		sqlite3_exec(db, "ROLLBACK", NULL, NULL, NULL);
		return false;
	};
	// Older profiles contain Russian text encoded as Windows-1251 bytes in TEXT.
	sqlite3_stmt *select = NULL;
	if(sqlite3_prepare_v2(db, "SELECT id, info FROM info_id_ru", -1, &select, NULL) != SQLITE_OK)
		return fail(sqlite3_errmsg(db));
	std::vector<std::pair<int, std::string>> repairs;
	while(sqlite3_step(select) == SQLITE_ROW) {
		const int id = sqlite3_column_int(select, 0);
		const unsigned char *text = sqlite3_column_text(select, 1);
		const int bytes = sqlite3_column_bytes(select, 1);
		if(!text || bytes <= 0 || g_utf8_validate(reinterpret_cast<const gchar*>(text), bytes, NULL)) continue;
		GError *conversionError = NULL;
		gsize written = 0;
		gchar *converted = g_convert(reinterpret_cast<const gchar*>(text), bytes, "UTF-8", "WINDOWS-1251", NULL, &written, &conversionError);
		if(!converted) {
			std::cerr << "Unable to convert Russian translation id " << id << ": "
				<< (conversionError ? conversionError->message : "invalid CP1251") << std::endl;
			g_clear_error(&conversionError);
			sqlite3_finalize(select);
			return fail("invalid Russian translation encoding");
		}
		repairs.emplace_back(id, std::string(converted, written));
		g_free(converted);
	}
	sqlite3_finalize(select);
	sqlite3_stmt *update = NULL;
	if(sqlite3_prepare_v2(db, "UPDATE info_id_ru SET info=?1 WHERE id=?2", -1, &update, NULL) != SQLITE_OK)
		return fail(sqlite3_errmsg(db));
	for(const auto &repair : repairs) {
		sqlite3_reset(update);
		sqlite3_clear_bindings(update);
		if(sqlite3_bind_text(update, 1, repair.second.c_str(), (int)repair.second.size(), SQLITE_TRANSIENT) != SQLITE_OK ||
			sqlite3_bind_int(update, 2, repair.first) != SQLITE_OK || sqlite3_step(update) != SQLITE_DONE) {
			sqlite3_finalize(update);
			return fail(sqlite3_errmsg(db));
		}
	}
	sqlite3_finalize(update);
	// These entries were added after the original Russian catalog was shipped.
	const char *missing[] = {
        "UPDATE info_id_ru SET info='Репозиторий проекта' WHERE id=540",
        "UPDATE info_id_en SET info='Project repository' WHERE id=540",
		"INSERT OR IGNORE INTO info_id_ru(id,info) SELECT id,info FROM info_id_en WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE info_id_ru.id=info_id_en.id)",
		"INSERT OR IGNORE INTO info_id_ru(id,info) VALUES(543,'Настройка параметров нового проекта')",
		"INSERT OR IGNORE INTO info_id_ru(id,info) VALUES(544,'Маска имени')",
		"INSERT OR IGNORE INTO info_id_ru(id,info) VALUES(545,'Маска, по которой будет построено имя нового проекта: %num% - первый свободный порядковый номер, %name% - текущее имя пользователя')",
		"INSERT OR IGNORE INTO info_id_ru(id,info) VALUES(548,'Создать проект')",
		"INSERT OR IGNORE INTO info_id_ru(id,info) VALUES(549,'Создать проект после загрузки конструктора')",
        "UPDATE info_id_en SET info='Errors' WHERE id=276",
        "UPDATE info_id_ru SET info='Ошибки' WHERE id=276",
        "INSERT OR IGNORE INTO commands(id,name,info_id,key,shift,ctrl) VALUES(167,'pn_terminal',900007,NULL,NULL,NULL)",
        "INSERT OR IGNORE INTO info_id_en(id,info) VALUES(900007,'Terminal')",
        "INSERT OR IGNORE INTO info_id_ru(id,info) VALUES(900007,'Терминал')",
        // Profiles created while the form project also used 900007: give the id back to the terminal.
        "UPDATE projects SET name_id=900008 WHERE entry='MainForm' AND name_id=900007",
        "UPDATE info_id_en SET info='Terminal' WHERE id=900007 AND info='GTK4 form application'",
        "UPDATE info_id_ru SET info='Терминал' WHERE id=900007 AND info LIKE 'Приложение с формой%'",
        "UPDATE menu_commands SET pos=pos+1 WHERE menu_id=4 AND pos>=76 AND NOT EXISTS(SELECT 1 FROM menu_commands WHERE menu_id=4 AND cmd_id=167)",
        "INSERT INTO menu_commands(menu_id,cmd_id,pos) SELECT 4,167,76 WHERE NOT EXISTS(SELECT 1 FROM menu_commands WHERE menu_id=4 AND cmd_id=167)"
	};
	for(const char *statement : missing)
		if(sqlite3_exec(db, statement, NULL, NULL, &error) != SQLITE_OK) {
			const std::string message = error ? error : sqlite3_errmsg(db);
			if(error) sqlite3_free(error);
			return fail(message.c_str());
		}
	// The first statement above is deliberately before the C++ package script:
	// all localized joins remain complete even when an old profile had a short
	// Russian catalog.
	// Install the C++ package into existing profiles. Its marked section in the
	// main SQL dump is the single source of truth for package ids, projects and
	// compiler registration; the schema and the base rows must not be replayed.
	gchar *sql = NULL; gsize length = 0;
	const ustring packageSql = dataDir + INT_PATH "hiasm.sql";
	if(g_file_get_contents(packageSql.c_str(), &sql, &length, NULL)) {
		const std::string dump(sql, length);
		const std::string beginMarker = "-- BEGIN CPP PACKAGE REGISTRATION";
		const std::string endMarker = "-- END CPP PACKAGE REGISTRATION";
		const size_t marker = dump.find(beginMarker);
		const size_t begin = marker == std::string::npos ? marker : marker + beginMarker.size();
		const size_t end = begin == std::string::npos ? begin : dump.find(endMarker, begin);
		if(end == std::string::npos) {
			g_free(sql);
			return fail("C++ package registration section not found in hiasm.sql");
		}
		const std::string migration = dump.substr(begin, end - begin);
		g_free(sql);
		if(sqlite3_exec(db, migration.c_str(), NULL, NULL, &error) != SQLITE_OK) {
			const std::string message = error ? error : sqlite3_errmsg(db);
			if(error) sqlite3_free(error);
			return fail(message.c_str());
		}
	}
	// user_version is used only as a migration marker. Settings and compiler
	// paths remain untouched; INSERT OR IGNORE makes this safe on every startup.
	if(sqlite3_exec(db, "PRAGMA user_version = 1", NULL, NULL, &error) != SQLITE_OK) {
		const std::string message = error ? error : sqlite3_errmsg(db);
		if(error) sqlite3_free(error);
		return fail(message.c_str());
	}
	if(sqlite3_exec(db, "SELECT 1 FROM localization WHERE pref='ru' LIMIT 1", NULL, NULL, &error) != SQLITE_OK) {
		if(error) sqlite3_free(error);
		return fail("localization table is invalid");
	}
	if(sqlite3_exec(db, "COMMIT", NULL, NULL, &error) != SQLITE_OK) {
		if(error) sqlite3_free(error);
		return false;
	}
	return true;
}

bool MainDataBase::setLanguage(const ustring &language) {
	if(!db || (language != "ru" && language != "en")) return false;
	sqlite3_stmt *stmt = query("UPDATE localization SET active = CASE WHEN pref=?1 THEN 1 ELSE 0 END WHERE pref IN ('ru','en')");
	if(!stmt) return false;
	bool ok = sqlite3_bind_text(stmt, 1, language.c_str(), language.bytes(), SQLITE_TRANSIENT) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_DONE;
	if(!ok) std::cerr << "Unable to set interface language: " << sqlite3_errmsg(db) << std::endl;
	sqlite3_finalize(stmt);
	if(ok) ui_set_language(language.raw());
	return ok;
}

void MainDataBase::close() {
	if(db) {
		sqlite3_close(db);
		db = NULL;
	}
}

sqlite3_stmt *MainDataBase::query(const ustring &sql) {
	if(!db)
		return NULL;

	sqlite3_stmt *result = NULL;
	if(sqlite3_prepare_v2(db, sql.c_str(), -1, &result, NULL) != SQLITE_OK) {
		std::cerr << "SQL prepare failed: " << sqlite3_errmsg(db) << " ["
			<< sql.c_str() << "]" << std::endl;
		if(result)
			sqlite3_finalize(result);
		return NULL;
	}
	return result;
}

void MainDataBase::exec(const ustring &sql) {
	if(!db)
		return;
	char *error = NULL;
	if(sqlite3_exec(db, sql.c_str(), NULL, NULL, &error) != SQLITE_OK) {
		std::cerr << "SQL execute failed: " << (error ? error : sqlite3_errmsg(db))
			<< " [" << sql.c_str() << "]" << std::endl;
		if(error) sqlite3_free(error);
	}
}

bool MainDataBase::nextRow(void *id, const char *context) {
	if(!id)
		return false;
	int rc = sqlite3_step((sqlite3_stmt*)id);
	if(rc == SQLITE_ROW)
		return true;
	if(rc != SQLITE_DONE)
		std::cerr << "SQL read failed in " << context << ": "
			<< sqlite3_errmsg(db) << std::endl;
	return false;
}

ustring MainDataBase::read_loc() {
	sqlite3_stmt *r = query("select pref from localization where active = 1");
	if(!r)
		return ustring();

	ustring result;
	if(nextRow(r, "localization")) {
		const unsigned char *pref = sqlite3_column_text(r, 0);
		if(pref && (!strcmp(reinterpret_cast<const char *>(pref), "ru") ||
			!strcmp(reinterpret_cast<const char *>(pref), "en")))
			result = ustring("info_id_") + (const char *)pref;
	}
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
	if(loc.empty())
		return NULL;
	return query(ustring("select commands.name,") + loc + ".info from commands," + loc + " where commands.info_id = " + loc + ".id order by " + loc + ".info");
}

bool MainDataBase::read_commands(void *id, CommandInfo &cmd) {
	if(!nextRow(id, "commands"))
		return false;
	cmd.name = readStr(id, 0);
	cmd.info = readStr(id, 1);
	return true;
}

void MainDataBase::end_read(void *id) {
	if(id)
		sqlite3_finalize((sqlite3_stmt*)id);
}

void *MainDataBase::begin_read_command_menu(const ustring &name) {
	sqlite3_stmt *r = query(ustring("select id from menus where name = '") + name + "'");
	if(!r)
		return NULL;
	int m = 0;
	if(sqlite3_step(r) == SQLITE_ROW)
		m = sqlite3_column_int(r, 0);
	sqlite3_finalize(r);
	if(!m)
		return NULL;

	ustring loc = read_loc();
	if(loc.empty())
		return NULL;
	return query(ustring::compose("select commands.name,%1.info from menu_commands,commands,%1 where menu_commands.menu_id = %2 AND menu_commands.cmd_id = commands.id AND commands.info_id = %1.id order by menu_commands.pos", loc, m));
}

void *MainDataBase::begin_read_command_by_key(int key, bool shift, bool ctrl) {
	ustring sql(ustring::compose("select name, (select '') from commands where key = %1 and shift = %2 and ctrl = %3",
		key, shift ? 1 : 0, ctrl ? 1 : 0));
	return  query(sql);
}

void *MainDataBase::begin_read_packs() {
	ustring loc = read_loc();
	if(loc.empty())
		return NULL;
	return query(ustring("select packs.name,") + loc + ".info,packs.codename,packs.units,packs.id from packs," + loc + " where packs.info_id = " + loc + ".id order by packs.id");
}

bool MainDataBase::read_packs(void *id, PackInfo &pack) {
	if(!nextRow(id, "packs"))
		return false;
	pack.name = readStr(id, 0);
	pack.info = readStr(id, 1);
	pack.codeName = readStr(id, 2);
	pack.units = readStr(id, 3);
	pack.id = readInt(id, 4);
	return true;
}

void *MainDataBase::begin_read_projects(const ustring &name) {
	sqlite3_stmt *r = query(ustring("select id from packs where codename = '") + name + "'");
	if(!r)
		return NULL;
	int m = 0;
	if(sqlite3_step(r) == SQLITE_ROW)
		m = sqlite3_column_int(r, 0);
	sqlite3_finalize(r);
	if(!m)
		return NULL;

	ustring loc = read_loc();
	if(loc.empty())
		return NULL;
	return query(ustring("select projects.entry, def.info as name, pinf.info as info, projects.ext, projects.make ") +
            "from projects, " + loc + " as pinf, " + loc + " as def " +
            "where projects.pack_id = " + int_to_str(m) + " AND pinf.id = projects.info_id AND def.id = projects.name_id");
}

bool MainDataBase::read_projects(void *id, ProjectInfo &project) {
	if(!nextRow(id, "projects"))
		return false;
	project.entry = readStr(id, 0);
	project.name = readStr(id, 1);
	project.info = readStr(id, 2);
	project.ext = readStr(id, 3);
	project.make = readStr(id, 4);
	return true;
}

void *MainDataBase::begin_read_settings(int pid) {
	ustring loc = read_loc();
	if(loc.empty())
		return NULL;
	return query(ustring("select p.id, p.pid, p.name, ti.info as title, ii.info as info, p.value, p.type, p.list ") +
            "from params as p, " + loc + " as ti, " + loc + " as ii " +
            "where p.pid = " + int_to_str(pid) + " AND ii.id = p.info_id AND ti.id = p.title_id");
}

bool MainDataBase::read_settings(void *id, ParamInfo &param) {
	if(nextRow(id, "settings")) {
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
	if(nextRow(id, "settings values")) {
		param.id = readInt(id, 0);
		param.value = readStr(id, 1);
		param.type = readInt(id, 2);
		return true;
	}
	return false;
}

void MainDataBase::save_settings_value(int id, const ustring &value) {
	sqlite3_stmt *stmt = query("UPDATE params SET value=?1 WHERE id=?2");
	if(!stmt) return;
	int result = sqlite3_bind_text(stmt, 1, value.c_str(), value.bytes(), SQLITE_TRANSIENT);
	if(result == SQLITE_OK) result = sqlite3_bind_int(stmt, 2, id);
	if(result == SQLITE_OK) result = sqlite3_step(stmt);
	if(result != SQLITE_DONE)
		std::cerr << "Unable to save setting " << id << ": " << sqlite3_errmsg(db) << std::endl;
	sqlite3_finalize(stmt);
}

void *MainDataBase::begin_read_compilers() {
	return query(ustring("select id, name, cmd, path, ext from compilers order by id"));
}

bool MainDataBase::read_compilers(void *id, CompilerInfo &compiler) {
	if(!nextRow(id, "compilers"))
		return false;
	compiler.id = readInt(id, 0);
	compiler.name = readStr(id, 1);
	compiler.cmd = readStr(id, 2);
	compiler.path = readStr(id, 3);
	compiler.ext = readStr(id, 4);
	return true;
}

void *MainDataBase::begin_read_pack_compilers(const ustring &name) {
	sqlite3_stmt *r = query(ustring("select id from packs where codename = '") + name + "'");
	if(!r)
		return NULL;
	int m = 0;
	if(sqlite3_step(r) == SQLITE_ROW)
		m = sqlite3_column_int(r, 0);
	sqlite3_finalize(r);
	if(!m)
		return NULL;

	return query(ustring("SELECT cmp_id, active FROM pack_compilers WHERE pack_id = " + int_to_str(m)));
}

bool MainDataBase::read_pack_compilers(void *id, PackCompilerInfo &compiler) {
	if(!nextRow(id, "pack_compilers"))
		return false;
	compiler.compiler = readInt(id, 0);
	compiler.active = readInt(id, 1) == 1;
	return true;
}
