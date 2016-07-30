/*
 * ElementConfig.cpp
 *
 *  Created on: 05.05.2010
 *      Author: dilma
 */

#include "ElementConfig.h"

ConfMethod *ConfMethods::getByName(const ustring &name) {
	for(iterator m = begin(); m != end(); m++)
		if((*m)->name == name)
			return *m;
	return NULL;
}

//---------------------------------------------------------------------

ElementConfig::ElementConfig(Pack *pack, const ustring &file_name) {
	this->pack = pack;
	load(file_name);
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
	if(strncmp(buf, "Class", 5) == 0)
		editClassName = buf + 6;
}

ConfProperty *ElementConfig::findPropByName(const ustring &name) {
	for(ConfPropertys::iterator p = prop.begin(); p != prop.end(); p++)
		if((*p)->name == name)
			return *p;
	return NULL;
}

void ElementConfig::parseProperty(char *buf) {
	int fl = 0;
	static ustring g;

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
		g = buf;
	}
	else {
		ConfProperty *prop = new ConfProperty();
		prop->flags = fl;
		prop->name = getTok(&buf, '=');
		prop->info.clear();
		do {
			prop->info += getTok(&buf, '|');
			if(*buf == '|') {
				prop->info += "|";
				buf++;
			}
		} while(*(buf-1));
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
		prop->group = g;

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
		mt->prop = g;
	}
	else
		mt->name = g;

	mt->info = getTok(&buf, '|');
	mt->mType = atoi(getTok(&buf, '|'));
	mt->dataType = atoi(buf);
}

bool ElementConfig::load(const ustring &file_name) {
	TRACE_PROC

	DEBUG_MSG("load config: " << file_name.c_str())

	FILE *f = fopen(file_name.c_str(), "r");
	if(!f) {
		ERROR_MSG("Error: config file not found!")
		return false;
	}

	int sl = 0, pt = 0;
	const char *cbuf = file_name.c_str();
	for(int i = file_name.length()-1; i > 0; i--)
		if(cbuf[i] == '.')
			pt = i;
		else if(cbuf[i] == PATH_SLASH[0]) {
			sl = i;
			break;
		}
	name = ustring(file_name, sl + 1, pt - sl - 1);

	int State = 0;
	char buf[1025];

	while(!feof(f)) {
		buf[0] = '\0';
		fgets(buf, 1024, f);
		if(!*buf || *buf == '\n' || *buf == '\r') {
			State = 0;
			continue;
		}

		int l = strlen(buf);
		if(buf[l-1] == '\n') {
			if(buf[l-2] == '\r') // windows format support
				buf[l-2] = '\0';
			                     // unix format
			buf[l-1] = '\0';
		}

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
		else
			switch(State) {
				case 1: parseEdit(buf); break;
				case 2: parseProperty(buf); break;
				case 3: parseMethod(buf); break;
				case 4: parseType(buf); break;
				case 5: parseAbout(buf); break;
			}
	}
	fclose(f);
//	  view = prop->Find(ViewProp);

	return true;
}
