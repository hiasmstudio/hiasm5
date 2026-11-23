-- Register the native C++17 (RTCG) package.
INSERT INTO info_id_en(id,info) SELECT 900001,'C++ application' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900001);
INSERT INTO info_id_ru(id,info) SELECT 900001,'Приложение на C++' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900001);
INSERT INTO packs(codename,name,info_id,units) SELECT 'cpp','Приложение на C++',900001,'hws' WHERE NOT EXISTS(SELECT 1 FROM packs WHERE codename='cpp');
-- BEGIN CPP PACKAGE REGISTRATION
-- Idempotent registration of the RTCG C++17 project types.
INSERT INTO info_id_en(id,info) SELECT 900001,'C++ application' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900001);
INSERT INTO info_id_ru(id,info) SELECT 900001,'Приложение на C++' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900001);
INSERT INTO info_id_en(id,info) SELECT 900002,'Console application' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900002);
INSERT INTO info_id_ru(id,info) SELECT 900002,'Консольное приложение' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900002);
INSERT INTO info_id_en(id,info) SELECT 900003,'Portable C++17 application (GCC)' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900003);
INSERT INTO info_id_ru(id,info) SELECT 900003,'Кроссплатформенное приложение C++17 (GCC)' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900003);
INSERT INTO info_id_en(id,info) SELECT 900004,'GTK4 GUI application' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900004);
INSERT INTO info_id_ru(id,info) SELECT 900004,'Приложение с интерфейсом GTK4' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900004);
INSERT INTO info_id_en(id,info) SELECT 900005,'Shared library' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900005);
INSERT INTO info_id_ru(id,info) SELECT 900005,'Динамическая библиотека' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900005);
INSERT INTO info_id_en(id,info) SELECT 900006,'Background process' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900006);
INSERT INTO info_id_ru(id,info) SELECT 900006,'Фоновый процесс' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900006);
-- The C++ package is a native RTCG package; keep Core (`_base`) and register
-- `cpp` so its project types and palette are available.
INSERT INTO packs(codename,name,info_id,units)
 SELECT 'cpp','Приложение на C++',900001,'hws' WHERE NOT EXISTS(SELECT 1 FROM packs WHERE codename='cpp');
-- Rename legacy Cpp17 project entries, preserving their ids when possible.
UPDATE projects SET entry=substr(entry,6)
 WHERE pack_id IN (SELECT id FROM packs WHERE codename='cpp')
 AND entry IN ('Cpp17Entry','Cpp17Gui','Cpp17Library','Cpp17Daemon')
 AND NOT EXISTS(SELECT 1 FROM projects AS current
                WHERE current.pack_id=projects.pack_id AND current.entry=substr(projects.entry,6));
-- Profiles which already have short entries can discard the duplicate registrations.
DELETE FROM projects WHERE pack_id IN (SELECT id FROM packs WHERE codename='cpp')
 AND entry IN ('Cpp17Entry','Cpp17Gui','Cpp17Library','Cpp17Daemon');
INSERT INTO projects(pack_id,entry,name_id,info_id,ext,make)
 SELECT id,'Entry',900002,900003,'cpp','make_text.so' FROM packs WHERE codename='cpp'
 AND NOT EXISTS(SELECT 1 FROM projects WHERE entry='Entry' AND pack_id=packs.id);
INSERT INTO projects(pack_id,entry,name_id,info_id,ext,make)
 SELECT id,'Console',900002,900003,'cpp','make_text.so' FROM packs WHERE codename='cpp'
 AND NOT EXISTS(SELECT 1 FROM projects WHERE entry='Console' AND pack_id=packs.id);
INSERT INTO projects(pack_id,entry,name_id,info_id,ext,make)
 SELECT id,'Service',900006,900003,'cpp','make_text.so' FROM packs WHERE codename='cpp'
 AND NOT EXISTS(SELECT 1 FROM projects WHERE entry='Service' AND pack_id=packs.id);
INSERT INTO projects(pack_id,entry,name_id,info_id,ext,make)
 SELECT id,'Gui',900004,900003,'cpp','make_text.so' FROM packs WHERE codename='cpp'
 AND NOT EXISTS(SELECT 1 FROM projects WHERE entry='Gui' AND pack_id=packs.id);
-- 900007 belongs to the "Terminal" command (src/MainDataBase.cpp); the form project uses 900008.
INSERT INTO info_id_en(id,info) SELECT 900008,'Qt 6.9 form application' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900008);
INSERT INTO info_id_ru(id,info) SELECT 900008,'Приложение с формой (Qt 6.9)' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900008);
UPDATE info_id_en SET info='Qt 6.9 form application' WHERE id=900008 AND info='GTK4 form application';
UPDATE info_id_ru SET info='Приложение с формой (Qt 6.9)' WHERE id=900008 AND info='Приложение с формой (GTK4)';
INSERT INTO projects(pack_id,entry,name_id,info_id,ext,make)
 SELECT id,'MainForm',900008,900003,'cpp','make_text.so' FROM packs WHERE codename='cpp'
 AND NOT EXISTS(SELECT 1 FROM projects WHERE entry='MainForm' AND pack_id=packs.id);
INSERT INTO projects(pack_id,entry,name_id,info_id,ext,make)
 SELECT id,'Library',900005,900003,'cpp','make_text.so' FROM packs WHERE codename='cpp'
 AND NOT EXISTS(SELECT 1 FROM projects WHERE entry='Library' AND pack_id=packs.id);
INSERT INTO projects(pack_id,entry,name_id,info_id,ext,make)
 SELECT id,'Daemon',900006,900003,'cpp','make_text.so' FROM packs WHERE codename='cpp'
 AND NOT EXISTS(SELECT 1 FROM projects WHERE entry='Daemon' AND pack_id=packs.id);
-- Project types of the C++ package: creation list entries and their names.
INSERT INTO info_id_en(id,info) SELECT 900009,'Windows DLL' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900009);
INSERT INTO info_id_ru(id,info) SELECT 900009,'Динамическая библиотека Windows (.dll)' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900009);
INSERT INTO info_id_en(id,info) SELECT 900010,'Windows-only dynamic library with DllMain and the hiasm_run export' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900010);
INSERT INTO info_id_ru(id,info) SELECT 900010,'Только для Windows: DllMain и экспорт hiasm_run' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900010);
INSERT INTO projects(pack_id,entry,name_id,info_id,ext,make)
 SELECT id,'WinDll',900009,900010,'cpp','make_text.so' FROM packs WHERE codename='cpp'
 AND NOT EXISTS(SELECT 1 FROM projects WHERE entry='WinDll' AND pack_id=packs.id);
INSERT INTO info_id_en(id,info) SELECT 900011,'HiAsm plugin' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900011);
INSERT INTO info_id_ru(id,info) SELECT 900011,'Плагин HiAsm' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900011);
INSERT INTO info_id_en(id,info) SELECT 900012,'Shared library with plugin info and run exports (ABI 1)' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900012);
INSERT INTO info_id_ru(id,info) SELECT 900012,'Динамическая библиотека с экспортами hiasm_plugin_info и hiasm_plugin_run (ABI 1)' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900012);
INSERT INTO projects(pack_id,entry,name_id,info_id,ext,make)
 SELECT id,'Plugin',900011,900012,'cpp','make_text.so' FROM packs WHERE codename='cpp'
 AND NOT EXISTS(SELECT 1 FROM projects WHERE entry='Plugin' AND pack_id=packs.id);
INSERT INTO info_id_en(id,info) SELECT 900013,'COM object (Explorer extension)' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900013);
INSERT INTO info_id_ru(id,info) SELECT 900013,'COM-объект (расширение Проводника)' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900013);
INSERT INTO info_id_en(id,info) SELECT 900014,'Windows in-process COM server with DllGetClassObject and registration' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900014);
INSERT INTO info_id_ru(id,info) SELECT 900014,'Только для Windows: COM-сервер в процессе (DllGetClassObject, регистрация)' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900014);
INSERT INTO projects(pack_id,entry,name_id,info_id,ext,make)
 SELECT id,'ComExt',900013,900014,'cpp','make_text.so' FROM packs WHERE codename='cpp'
 AND NOT EXISTS(SELECT 1 FROM projects WHERE entry='ComExt' AND pack_id=packs.id);
INSERT INTO info_id_en(id,info) SELECT 900015,'Windows service' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900015);
INSERT INTO info_id_ru(id,info) SELECT 900015,'Сервис Windows' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900015);
INSERT INTO info_id_en(id,info) SELECT 900016,'Runs under the Service Control Manager until it asks to stop' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900016);
INSERT INTO info_id_ru(id,info) SELECT 900016,'Работает под управлением SCM до запроса остановки' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900016);
INSERT INTO projects(pack_id,entry,name_id,info_id,ext,make)
 SELECT id,'WinService',900015,900016,'cpp','make_text.so' FROM packs WHERE codename='cpp'
 AND NOT EXISTS(SELECT 1 FROM projects WHERE entry='WinService' AND pack_id=packs.id);
INSERT INTO info_id_en(id,info) SELECT 900017,'Control Panel applet' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900017);
INSERT INTO info_id_ru(id,info) SELECT 900017,'Приложение для Панели управления' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900017);
INSERT INTO info_id_en(id,info) SELECT 900018,'Windows .cpl applet exporting CPlApplet' WHERE NOT EXISTS(SELECT 1 FROM info_id_en WHERE id=900018);
INSERT INTO info_id_ru(id,info) SELECT 900018,'Только для Windows: .cpl-апплет с экспортом CPlApplet' WHERE NOT EXISTS(SELECT 1 FROM info_id_ru WHERE id=900018);
INSERT INTO projects(pack_id,entry,name_id,info_id,ext,make)
 SELECT id,'CplApp',900017,900018,'cpp','make_text.so' FROM packs WHERE codename='cpp'
 AND NOT EXISTS(SELECT 1 FROM projects WHERE entry='CplApp' AND pack_id=packs.id);
INSERT INTO compilers(name,cmd,path,ext)
 SELECT 'GCC C++17','-std=c++17 -finput-charset=UTF-8 -fexec-charset=UTF-8 "%fname%" -o "%oname%"','g++','cpp'
 WHERE NOT EXISTS(SELECT 1 FROM compilers WHERE name='GCC C++17');
INSERT INTO pack_compilers(pack_id,cmp_id,active)
 SELECT p.id,c.id,1 FROM packs p,compilers c WHERE p.codename='cpp' AND c.name='GCC C++17'
 AND NOT EXISTS(SELECT 1 FROM pack_compilers WHERE pack_id=p.id AND cmp_id=c.id);
