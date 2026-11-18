/*
 * ElementConfig.cpp
 *
 *  Created on: 05.05.2010
 *      Author: dilma
 */

#include "ElementConfig.h"
#include "LegacyPackCatalog.h"
#include "Localization.h"
#include <glib/gstdio.h>
#include <stdexcept>
#include <string>
#include <vector>
#include <map>

namespace {
std::string trimText(std::string value) {
    const auto first = value.find_first_not_of(" \t\r");
    if(first == std::string::npos) return {};
    const auto last = value.find_last_not_of(" \t\r");
    return value.substr(first, last - first + 1);
}

// Package translations are overlays, not alternate element definitions.  A
// .lang file maps the English phrase stored in conf/*.ini to its localized
// replacement, one phrase per line: English=translated.  A package that has no
// copy of an element takes its translation from the package that owns the
// .ini file (Core for the hc* elements), as HiAsm 4 does; the package's own
// .lang file always wins.
void applyLanguageOverlay(ElementConfig &config, const ustring &file_name) {
    if(!config.pack || config.pack->isLegacy()) return;
    const auto language = ui_language();
    // file_name is <package>/conf/<name>.ini; the package is two levels up.
    gchar *conf = g_path_get_dirname(file_name.c_str());
    gchar *owner = g_path_get_dirname(conf);
    std::vector<ustring> bases = {config.pack->path() + "lang/"};
    if(owner && ustring(owner) + "/" != config.pack->path()) bases.push_back(ustring(owner) + "/lang/");
    g_free(conf); g_free(owner);
    std::vector<ustring> candidates;
    for(const auto &base : bases) candidates.push_back(base + language + "/conf/" + config.name + ".lang");
    if(language != "en")
        for(const auto &base : bases) candidates.push_back(base + "en/conf/" + config.name + ".lang");
    gchar *contents = nullptr; gsize size = 0;
    std::string text;
    for(const auto &candidate : candidates) {
        if(g_file_get_contents(candidate.c_str(), &contents, &size, nullptr)) {
            if(g_utf8_validate(contents, size, nullptr)) text.assign(contents, size);
            g_free(contents); contents = nullptr;
            if(!text.empty()) break;
        }
    }
    if(text.empty()) return;
    std::map<std::string, std::string> translations;
    size_t offset = 0;
    while(offset <= text.size()) {
        const auto end = text.find('\n', offset);
        std::string line = text.substr(offset, end == std::string::npos ? end : end - offset);
        if(!line.empty() && line.back() == '\r') line.pop_back();
        offset = end == std::string::npos ? text.size() + 1 : end + 1;
        const auto eq = line.find('=');
        if(eq == std::string::npos) continue;
        const auto key = trimText(line.substr(0, eq));
        const auto value = trimText(line.substr(eq + 1));
        if(!key.empty()) translations[key] = value;
    }
    auto translate = [&translations](ustring &value) {
        const auto found = translations.find(value.raw());
        if(found != translations.end()) value = found->second;
    };
    translate(config.info);
    for(auto *property : config.prop) translate(property->info);
    for(auto *method : config.mtd) translate(method->info);
    for(auto *method : config.def) translate(method->info);
}

int pointDataType(const char *text) {
	std::string value(text);
	const auto extra = value.find('|');
	if(extra != std::string::npos) value.erase(extra);
	const auto first = value.find_first_not_of(" \t");
	if(first == std::string::npos) return data_null;
	value = value.substr(first, value.find_last_not_of(" \t") - first + 1);
	if(value == "str") return data_str;
	if(value == "int") return data_int;
	if(value == "arr") return data_array;
	if(value == "data") return data_data;
	char *end = nullptr;
	const gint64 number = g_ascii_strtoll(value.c_str(), &end, 10);
	// HiAsm 4 packages use their own type codes above the standard ones (CNet: .NET types such as 104). Such a point is
	// kept without a type; canConnect() never compares point data types, so connections are not affected.
	if(end == value.c_str() || *end != 0 || number < 0) return -1;
	return number < data_count ? static_cast<int>(number) : data_null;
}
}

ConfMethod *ConfMethods::getByName(const ustring &name) {
	for(iterator m = begin(); m != end(); m++)
		if((*m)->name == name)
			return *m;
	return NULL;
}

//---------------------------------------------------------------------

ElementConfig::ElementConfig(Pack *pack, const ustring &file_name) {
	this->pack = pack;
	viewFontHeight = 8;
	icon = 0;
	valid = load(file_name);
}

ElementConfig::~ElementConfig() {
	for(auto *p : prop) delete p;
	for(auto *m : mtd) delete m;
	for(auto *m : def) delete m;
}

void ElementConfig::parseAbout(char *buf) {

}

void ElementConfig::inheritFrom(char *buf) {
	inherit = buf;
	char *nm;
	do {
		nm = getTok(&buf, ',');
		PackElement *e = pack->getElementByName(nm);
		if(e) {
			sub = e->conf->sub;
			interfaces = e->conf->interfaces;
			assignProp(e->conf->prop);
			assignMtd(e->conf->mtd);
			assignDef(e->conf->def);
		}
		// A HiAsm 4 package may inherit from a class that the installation does not have (CNet: DbCommand,
		// TextWriter, ToolStripLabel). HiAsm 4 keeps the element with what it defines itself.
		else if(pack && pack->isLegacy())
			std::cerr << "HiAsm 4 config " << pack->name.raw() << ": inherited element not in the installation, skipped: " << nm << std::endl;
		else throw std::runtime_error(std::string("Missing or invalid inherited element: ") + nm);
	} while (*buf);
}

void ElementConfig::assignProp(ConfPropertys &a_prop) {
	for(ConfPropertys::iterator p = a_prop.begin(); p != a_prop.end(); p++)
		prop.push_back(new ConfProperty(**p));
}

void ElementConfig::assignMtd(ConfMethods &a_mtd) {
	for(ConfMethods::iterator m = a_mtd.begin(); m != a_mtd.end(); m++)
		mtd.push_back(new ConfMethod(**m));
}

void ElementConfig::assignDef(ConfDefMethods &a_def) {
	for(ConfDefMethods::iterator m = a_def.begin(); m != a_def.end(); m++)
		def.push_back(new ConfMethod(**m));
}

void ElementConfig::parseType(char *buf) {
	if(strncmp(buf, "Class", 5) == 0)
		cClass = buf + 6;
	else if(strncmp(buf, "Inherit", 7) == 0)
		inheritFrom(buf + 8);
	else if(strncmp(buf, "Sub", 3) == 0)
		sub = buf + 4;
	else if(strncmp(buf, "Info", 4) == 0)
		info = buf + 5;
	else if(strncmp(buf, "Icon", 4) == 0)
		iconic = buf + 5;
	else if(strncmp(buf, "View", 4) == 0) {
		getTok(&buf, '=');
		view = getTok(&buf, ',');
		viewFontHeight = atoi(buf) - 4;
	  //s = it;
	  //s.GetTok(_his('='));
	  //viewProp = s.GetTok(_his(','));
	  //viewFontHeight = s;
	}
	else if(strncmp(buf, "Tab", 3) == 0)
		tab = buf + 4;
	else if(strncmp(buf, "Interfaces", 10) == 0)
		interfaces = buf + 11;
}

void ElementConfig::parseEdit(char *buf) {
	if(strncmp(buf, "Class=", 6) == 0)
		editClassName = buf + 6;
	else if(const char *eq = strchr(buf, '='))
		editProps[std::string(buf, eq - buf)] = eq + 1;
}

ConfProperty *ElementConfig::findPropByName(const ustring &name) {
	for(ConfPropertys::iterator p = prop.begin(); p != prop.end(); p++)
		if((*p)->name == name)
			return *p;
	return NULL;
}

void ElementConfig::parseProperty(char *buf) {
	int fl = 0;

	if(*buf == '+') {
		buf++;
		fl |= VF_DBLOPEN;
	}

	if(*buf == '@') {
		buf++;
		fl |= VF_MAKEPOINT;
	}

	if(buf[0] == '#' && buf[1] == '#') {
		buf += 2;
		propertyGroup = buf;
	}
	else {
		ConfProperty *prop = new ConfProperty();
		prop->flags = fl;
		prop->name = getTok(&buf, '=');
		prop->info.clear();
		while(true) {
			prop->info += getTok(&buf, '|');
			if(*buf == '|' && !prop->info.empty()) {
				prop->info += "|";
				buf++;
			}
			else break;
		}
		prop->type = (DataType)atoi(getTok(&buf, '|'));
		prop->value = getTok(&buf, '|');
		int l = 0;
		while((l = prop->value.find("#13#10", l)) != -1) {
			prop->value.replace(l, 6, "\n");
			l++;
		}
		if(prop->type == data_combo || prop->type == data_comboEx || prop->type == data_element || prop->type == data_flags) {
			prop->list = buf;
			int i = 0;
			while((i = prop->list.find("||", i)) != -1) {
				prop->list.replace(i, 2, "|");
				i++;
			}
		}
		prop->group = propertyGroup;

		if(ConfProperty *ep = findPropByName(prop->name)) {
			ep->value = prop->value;
			delete prop;
		}
		else
			this->prop.push_back(prop);
	}
}

void ElementConfig::parseMethod(char *buf) {
	ConfMethod *mt = new ConfMethod();

	if(*buf == '*') {
		def.push_back(mt);
		buf++;
	}
	else
		mtd.push_back(mt);

	char *g = getTok(&buf, '=');
	if(strchr(g, '%')) {
		mt->name = getTok(&g, '%');
		// doStart%Precision%: the property name is between the second pair of percent signs.
		mt->prop = getTok(&g, '%');
	}
	else
		mt->name = g;

	do {
		mt->info += getTok(&buf, '|');
		if(*buf != '|') break;
		mt->info += '|'; ++buf;
	} while(true);
	mt->mType = atoi(getTok(&buf, '|'));
	mt->dataType = pointDataType(buf);
}

bool ElementConfig::load(const ustring &file_name) {
	TRACE_PROC

	DEBUG_MSG("load config: " << file_name.c_str())

	gchar *contents = nullptr;
	gsize size = 0;
	GError *error = nullptr;
	if(!g_file_get_contents(file_name.c_str(), &contents, &size, &error)) {
		std::cerr << "Cannot read config " << file_name << ": "
			<< (error ? error->message : "unknown error") << std::endl;
		if(error) g_error_free(error);
		return false;
	}
	const bool readable = size <= 16 * 1024 * 1024 && (g_utf8_validate(contents, size, nullptr) || (pack && pack->isLegacy()));
	std::string text;
	if(readable) text = pack && pack->isLegacy() ? LegacyPackCatalog::decodeText(contents, size) : std::string(contents, size);
	g_free(contents);
	if(!readable) {
		std::cerr << "Invalid UTF-8 or oversized config: " << file_name << std::endl;
		return false;
	}
	if(text.compare(0, 3, "\xef\xbb\xbf") == 0) text.erase(0, 3);

	gchar *basename = g_path_get_basename(file_name.c_str());
	name = basename; g_free(basename);
	const auto dot = name.rfind('.');
	if(dot != ustring::npos) name.erase(dot);

	int State = 0;
	size_t offset = 0, lineNumber = 0;
	auto integer = [](const std::string &field, int low, int high) {
		char *end = nullptr;
		const gint64 value = g_ascii_strtoll(field.c_str(), &end, 10);
		return !field.empty() && end != field.c_str() && *end == 0 && value >= low && value <= high;
	};

	try { while(offset < text.size()) {
		const size_t end = text.find('\n', offset);
		std::string line = text.substr(offset, end == std::string::npos ? end : end - offset);
		offset = end == std::string::npos ? text.size() : end + 1;
		++lineNumber;
		if(!line.empty() && line.back() == '\r') line.pop_back();
		const auto first = line.find_first_not_of(" \t");
		if(first == std::string::npos) {
			State = 0;
			continue;
		}
		if(line[first] == ';' || line.compare(first, 2, "//") == 0) continue;
		if(line[first] != '[' && State) {
			if(State == 2 && line.compare(0, 2, "##") == 0) {
				propertyGroup = line.substr(2); continue;
			}
			const auto eq = line.find('=');
			if(eq == std::string::npos || eq == 0) throw std::runtime_error("expected key=value");
			if(State == 2 || State == 3) {
				size_t pipe = eq + 1;
				for(; pipe < line.size(); ++pipe) {
					if(line[pipe] != '|') continue;
					if(pipe > eq + 1 && pipe + 1 < line.size() && line[pipe+1] == '|') ++pipe;
					else break;
				}
				const auto next = line.find('|', pipe + 1);
				if(pipe == line.size() || (State == 3 && next == std::string::npos)) throw std::runtime_error("missing field separator");
				const auto typeField = line.substr(pipe+1, next == std::string::npos ? next : next-pipe-1);
				// v4 uses point type 0 to remove an inherited optional point.
				if(State == 3 && pack && pack->isLegacy() && integer(typeField, 0, 0)) {
					std::string pointName = line.substr(0, eq);
					if(!pointName.empty() && pointName[0] == '*') pointName.erase(0, 1);
					for(ConfMethods *methods : {&mtd, &def})
						for(auto it = methods->begin(); it != methods->end();)
							if((*it)->name == pointName) { delete *it; it = methods->erase(it); }
							else ++it;
					continue;
				}
				// Inherited property overrides leave the description and type blank.
				const bool overrideProperty = State == 2 && typeField.find_first_not_of(" \t") == std::string::npos;
				std::string propertyName = line.substr(0, eq);
				propertyName.erase(0, propertyName.find_first_not_of("+@"));
				if(overrideProperty && !findPropByName(propertyName)) {
					// HiAsm 4 packages override properties of classes that may be missing (see inheritFrom): the
					// override has no description or type to define the property, so the line is skipped.
					if(pack && pack->isLegacy()) continue;
					throw std::runtime_error("override of unknown property");
				}
				if(!overrideProperty && !integer(typeField, State == 2 ? 0 : 1, State == 2 ? data_count-1 : 4))
					throw std::runtime_error("invalid property or point type");
				if(State == 3 && pointDataType(line.c_str()+next+1) < 0)
					throw std::runtime_error("invalid point data type");
			}
		}
		std::vector<char> buffer(line.begin(), line.end());
		buffer.push_back(0);
		char *buf = buffer.data();

		// TODO new line configs wrong
		//std::cout << "[" << State << "]: " << buf << " LEN: " << strlen(buf) << std::endl;

		if(strcmp(buf, "[Edit]") == 0)
			State = 1;
		else if(strcmp(buf, "[Property]") == 0)
			State = 2;
		else if(strcmp(buf, "[Methods]") == 0)
			State = 3;
		else if(strcmp(buf, "[Type]") == 0)
			State = 4;
		else if(strcmp(buf, "[About]") == 0)
			State = 5;
		else if(*buf == '[')
			State = 0;
		else
			switch(State) {
				case 1: parseEdit(buf); break;
				case 2: parseProperty(buf); break;
				case 3: parseMethod(buf); break;
				case 4: parseType(buf); break;
				case 5: parseAbout(buf); break;
			}
	} } catch(const std::exception &failure) {
		std::cerr << "Invalid config " << file_name << ":" << lineNumber << ": " << failure.what() << std::endl;
		return false;
	}
	// Apply package-local translations only after the complete configuration
	// (including inherited properties and methods) has been parsed.
	applyLanguageOverlay(*this, file_name);

//	  view = prop->Find(ViewProp);

	return true;
}
