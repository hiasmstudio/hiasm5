/*
 * SDK.cpp
 *
 *  Created on: 02.05.2010
 *      Author: dilma
 */

#include "SDK.h"
#include "Elements.h"

Element *SDK_Lib::getElementByID(int id) {
	for(iterator e = begin(); e != end(); e++)
		if((*e)->id == id)
			return *e;
	return NULL;
}

void SDK_Lib::add(Element *element) {
	for(iterator e = begin(); e != end(); e++)
		if(*e == element)
			return;
	push_back(element);
}

//------------------------------------------------------------------------------------------

SDK::SDK():
	on_redraw_rect(this, CBT_SDK_REDRAW_RECT),
	on_add_element(this, CBT_SDK_ADD_ELEMENT),
	on_remove_element(this, CBT_SDK_REMOVE_ELEMENT)
{
	parent = NULL;

	pasteX = 0;
	pasteY = 0;

	scrollX = 0;
	scrollY = 0;

	selMan = new SelectManager(this);
}

SDK::~SDK() {
	clear();
	delete selMan;
}

void SDK::clear() {
	TRACE_PROC
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		delete *e;
	elements.clear();
}

void SDK::remove(Element *element) {
	elements.remove(element);
	on_remove_element.run(element);
	element->remove();
	delete element;
}

ustring SDK::getPath() {
	ustring res;
	SDK *sdk = this;
	while(sdk->parent) {
		res = ustring("/") + sdk->parent->getName() + res;
		sdk = sdk->parent->parent;
	}
	return ustring("ROOT") + res;
}

Element *SDK::getElementByName(const ustring &name) {
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++) {
		if((*e)->tpl->name == name)
			return *e;
	}
	return NULL;
}

Element *SDK::getElementById(int id) {
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++) {
		if((*e)->id == id)
			return *e;
	}
	return NULL;
}

Element *SDK::getGlobalElementById(int id) {
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++) {
		if((*e)->id == id)
			return *e;
		if((*e)->sdk) {
			// TODO: search in all child SDK
			Element *fe = (*e)->sdk->getGlobalElementById(id);
			if(fe)
				return fe;
		}
	}
	return NULL;
}

void SDK::draw(DrawContext dc, double zoom) {
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++) {
		(*e)->drawLinks(dc);
	}
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++) {
		(*e)->draw(dc, zoom);
	}
}

Element *SDK::add(const ustring &name, gdouble x, gdouble y) {
	TRACE_PROC

	Element *e = createElement(name, this, x, y);
	e->storePropPointers();
	e->id = getMSDK()->genNextID();
	elements.push_back(e);

	Gdk::Rectangle r = e->drawRect();
	on_redraw_rect.run(&r);

	on_add_element.run(e);

	return e;
}

MSDK *SDK::getMSDK() {
	SDK *msdk = this;
	while(msdk->parent) msdk = msdk->parent->parent;
	return (MSDK*)msdk;
}

bool SDK::getObjectAtPos(gdouble x, gdouble y, ObjectType *obj) {
	obj->type = SDK_OBJ_NONE;
	obj->obj1 = NULL;
	obj->obj2 = NULL;

	for(ElementsList::reverse_iterator e = elements.rbegin(); e != elements.rend(); e++) {
		if((*e)->getObjectAtPos(x, y, obj))
			return true;
	}

	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++) {
		if((*e)->findLine(x, y, obj))
			return true;
	}

	return false;
}

void SDK::getSchemeGeometry(gdouble &min_x, gdouble &min_y, gdouble &max_x, gdouble &max_y) {
	min_x = 1 << 8;
	min_y = min_x;
	max_x = -min_x;
	max_y = max_x;

	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++) {
		if((*e)->x < min_x) min_x = (*e)->x;
		if((*e)->y < min_y) min_y = (*e)->y;
		if((*e)->x + (*e)->width > max_x) max_x = (*e)->x + (*e)->width;
		if((*e)->y + (*e)->height > max_y) max_y = (*e)->y + (*e)->height;
	}
}

void SDK::saveToText(ustring &text, ustring offset) {
	TRACE_PROC
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++) {
		(*e)->saveToText(text, offset, ELEMENT_SAVE_CHANGED);
	}
}

void SDK::moveToTop(Element *e) {
	if(!e) return;

	elements.remove(e);
	elements.push_back(e);

	e->invalidate();
}

void SDK::moveToDown(Element *e) {
	if(!e) return;

	for(ElementsList::iterator el = elements.begin(); el != elements.end(); el++)
		if(*el != e && !(*el)->isNoDelete()) {
			elements.remove(e);
			elements.insert(el, e);
			break;
		}

	e->invalidate();
}

gchar *getToken(gchar *&text, int &len) {
	len = -1;

	while(*text == ' ' || *text == '\t' || *text == '\r' || *text == '\n')
		text++;

	if(!*text)
		return text;

	gchar *tok = text;
	if((*text >= 'a' && *text <= 'z') || (*text >= 'A' && *text <= 'Z') || *text == '_') { //name
		while((*text >= 'a' && *text <= 'z') || (*text >= 'A' && *text <= 'Z') || *text == '_' || (*text >= '0' && *text <= '9'))
			text++;
	}
	else if(*text >= '0' && *text <= '9') { //numeric
		while(*text >= '0' && *text <= '9')
			text++;
		if(*text == '.') { // double
			text++;
			while(*text >= '0' && *text <= '9')
				text++;
		}
	}
	else if(*text == '"') { // string
		tok++;
		text++;
		while(*text && *text != '"' && *text != '\r' && *text != '\n')
			text++;
		len = text - tok;
		text++;
	}
	else if(*text == '#') { //string
		while(*text && *text != '\r' && *text != '\n')
			text++;
	}
	else { // symbols
		text++;
	}

	if(len == -1) len = text - tok;
	return tok;
}

struct _ElementInfo {
	Element *e;
	int id;
	_ElementInfo(Element *_e, int _id) { e = _e; id = _id; }
};

struct _PostConnect {
	Element *e1;
	ustring p1;
	int e2;
	ustring p2;
	ustring path;
};

struct _ElementLink {
	Element *el;
	int link_id;
	_ElementLink(Element *_e, int _id) { el = _e; link_id = _id; }
};

typedef std::list<_PostConnect> _PostConnectList;
typedef std::list<_ElementInfo> _ElementInfoList;
typedef std::list<_ElementLink> _ElementLinkList;

Element *_getElementById(_ElementInfoList &einf, int id) {
	for(_ElementInfoList::iterator e = einf.begin(); e != einf.end(); e++)
		if((*e).id == id)
			return (*e).e;
	return NULL;
}

void nextLine(char *&text) {
	while(*text != '\r' && *text != '\n')
		text++;
}

SDKParseError SDK::loadFromText(gchar *&text, int &line, int flag) {
	TRACE_PROC

	gchar *tok = NULL;
	Element *e = NULL;
	int len;

	bool first = line == 1;

	static _ElementInfoList einf;
	static _PostConnectList pcon;
	static _ElementLinkList elink;

	while(*text) {
		tok = getToken(text, len);
		if(len == -1) break;
		//std::cout << ustring(tok, len).c_str() << std::endl;

		if(len == 3 && strncmp(tok, "Add", len) == 0) {
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
			ustring name(getTok(&text, ','));
			int id = atoi(getTok(&text, ','));
			gdouble x = atof(getTok(&text, ','));
			gdouble y = atof(getTok(&text, ')'));

			if(e || elements.empty())
				e = add(name, x, y);
			else if((*elements.begin())->tpl->name == name) {
				e = *elements.begin();
				e->x = x;
				e->y = y;
				e->rePosPoints();
				e->clearLinks();
			}
			else
				e = add(name, x, y);

			// TODO check it wo element add
			Element *pe = parent ? parent->getParentElement() : NULL;
			if(elements.size() > 2 && pe != NULL && pe->tpl->name == e->tpl->name) {
				Element *t = parent->getParentElement();
				t->move(e->x - t->x, e->y - t->y);
				remove(e);
				e = t;
				e->clearLinks();
			}

			if(flag&ELEMENT_LOAD_PASTE) {
				selMan->add(e);
				DEBUG_MSG("old: " << id << " new: " << e->id)
				einf.push_back(_ElementInfo(e, id));
			}
			else {
				e->id = id;
			}
		}
		else if(*tok == '{') {
			if(!e) return SDK_PERR_SYNTAX;
		}
		else if(*tok == '}') {
			e->init();
			//e = NULL;
		}
		else if(len == 9 && strncmp(tok, "BEGIN_SDK", len) == 0) {
			DEBUG_MSG("Load sub SDK")
			//e->sdk->clear();
			SDKParseError err = e->sdk->loadFromText(text, line, flag);
			if(err) return err;
		}
		else if(len == 7 && strncmp(tok, "END_SDK", len) == 0) {
			DEBUG_MSG("End sub SDK")
			break;
		}
		else if(*tok == '@') {
			ustring name(getTok(&text, '='));
			if(name == "IsLib") {
				getMSDK()->sdkLib.add(e);
				nextLine(text);
			}
			else {
				tok = text;
				nextLine(text);
				*text = '\0';
				text++;
				ustring value(tok);
				ElementProperty *prop = e->sysProps.getByName(name);
				if(prop)
					prop->initFromText(value);
				else
					std::cout << "System property not found: " << name.c_str() << std::endl;
			}
		}
		else if(len == 4 && strncmp(tok, "Make", len) == 0) {
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
			getMSDK()->pack = packSet->getPackByName(getTok(&text, ')'));
			if(!getMSDK()->pack) {
				std::cout << "Pack not found" << std::endl;
				return SDK_PERR_PACK_NOT_FOUND;
			}
		}
		else if(len == 3 && strncmp(tok, "ver", len) == 0) {
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
			getTok(&text, ')');
		}
		else if(len == 4 && strncmp(tok, "link", len) == 0) {
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
			_PostConnect pc;
			gchar *p2 = getTok(&text, ',');
			pc.e1 = e;
			pc.p1 = p2;
			pc.e2 = atoi(getTok(&text, ':'));
			pc.p2 = getTok(&text, ',');
			if(*getToken(text, len) != '[') return SDK_PERR_SYNTAX;
			pc.path = getTok(&text, ']');
			text++;

			pcon.push_back(pc);
		}
		else if(len == 5 && strncmp(tok, "elink", len) == 0) {
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
			elink.push_back(_ElementLink(e, atoi(getTok(&text, ')'))));
		}
		else if(len == 5 && strncmp(tok, "Point", len) == 0) {
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
			gchar *p = getTok(&text, ')');
			if(e->points.getByName(p))
				; // if exists skip it
			else if(ConfMethod *m = e->tpl->conf->def.getByName(p)) {
				e->addPoint(m->name, m->info, m->mType);
				e->rePosPoints();
			}
			else if(ElementProperty *prop = e->props.getByPointName(p)) {
				prop->makePoint();	// TODO add wo invalidate
			}
			else {
				DEBUG_MSG("Custom point with name " << p << " not found!")
			}
		}
		else if(len == 7 && strncmp(tok, "AddHint", len) == 0) {
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
            int _x = atoi(getTok(&text,','));
            int _y = atoi(getTok(&text,','));
            int _w = atoi(getTok(&text,','));
            int _h = atoi(getTok(&text,','));

            char *name = getTok(&text,')');
            ElementProperty *prop;
            if(*name == '@')
            	prop = e->sysProps.getByName(name+1);
            else
            	prop = e->props.getByName(name);
        	if(!prop)
        		ERROR_MSG("prop " << name << " not found!")
        	else
        		e->hints.restore(_x, _y, _w, _h, prop);
			nextLine(text);
		}
		else if(len == 7 && strncmp(tok, "MakeExt", len) == 0) {
			// TODO
			nextLine(text);
		}
		else if(e) {
			ustring pname(tok, len);
			ElementProperty *prop = e->props.getByName(pname);
			if(*text++ != '=')
				return SDK_PERR_SYNTAX;
			tok = text;
			nextLine(text);
			*text = '\0';
			text++;

			if(prop) {
				prop->initFromText(ustring(tok));
			}
			else {
				prop = e->sysProps.getByName(pname);
				if(prop)
					prop->initFromText(ustring(tok));
				else
					DEBUG_MSG("Property not found: " << pname.c_str())
			}
		}
//		std::cout << "parse line: " << line << std::endl;
		line++;
	}

	if(first) {
		if(elink.size()) {
			DEBUG_MSG("library restore...")
			for(_ElementLinkList::iterator el = elink.begin(); el != elink.end(); el++) {
				Element *emain = NULL;
				if(!(flag&ELEMENT_LOAD_FILE))
					emain = _getElementById(einf, el->link_id);
				if(!emain)
					emain = getMSDK()->sdkLib.getElementByID(el->link_id);
				if(emain)
					el->el->createELink(emain);
			}
		}

		DEBUG_MSG("post connections...")
		for(_PostConnectList::iterator p = pcon.begin(); p != pcon.end(); p++) {
			Element *e2 = (flag&ELEMENT_LOAD_PASTE) ? _getElementById(einf, (*p).e2) : (*p).e1->parent->getElementById((*p).e2);
			if(e2) {
				ElementPoint *p2 = e2->getPointByName((*p).p2);
				if(p2) {
					ElementPoint *p1 = (*p).e1->getPointByName((*p).p1);
					if(p1) {
						p1->connect(p2);
						gchar *path = new gchar[(*p).path.length()+1], *del_path = path;
						(*p).path.copy(path, (*p).path.length());
						path[(*p).path.length()] = '\0';
						PointPos *pp = p1->pos;
						while(*path == '(') {
							path++;
							int x = atoi(getTok(&path, ','));
							int y = atoi(getTok(&path, ')'));
							pp = p1->addLinePoint(pp, x, y);
						}
						delete del_path;
					}
					else
						DEBUG_MSG("Point not found: " << (*p).p1)
				}
				else
					DEBUG_MSG("Point not found: " << (*p).p2 << " on element " << e2->tpl->name.c_str())
			}
			else
				std::cout << "Element not found: " << (*p).e2 << std::endl;
		}
		DEBUG_MSG("Load complete.")

		einf.clear();
		pcon.clear();
		elink.clear();
	}

	return SDK_PERR_NONE;
}

void SDK::prepareForRun(Element **parent) {
	Element *main = NULL;
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		if((*e)->isParent() && (*e)->isCore()) {
			main = *e;
			break;
		}

	if(main)
		dynamic_cast<ElementCore*>(main)->prepareForRun(parent);

	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		if((*e)->isCore() && *e != main) {
			dynamic_cast<ElementCore*>(*e)->prepareForRun(parent);
		}

	if(main)
		dynamic_cast<ElementTplParent*>(main)->run();
}

void SDK::stopRun() {
	Element *main = NULL;
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		if((*e)->isParent() && (*e)->isCore()) {
			main = *e;
			break;
		}

	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		if((*e)->isCore() && *e != main) {
			dynamic_cast<ElementCore*>(*e)->stopRun();
		}

	if(main)
		dynamic_cast<ElementCore*>(main)->stopRun();
}

ustring getUserId(Element *e) {
	ElementProperty *prop = e->props.getByName("Name");
	if(prop == NULL || prop->isDefault())
		return ustring();
	return prop->readStr();
}

ustring SDK::getListOfManagers(const ustring &name) {
	SDK *sdk = this;
	ustring result, _parent;
	while(sdk) {
		for(ElementsList::iterator e = sdk->elements.begin(); e != sdk->elements.end(); e++)
			if((*e)->checkInterfaceProvide(name)) {
				ustring s2 = getUserId(*e);
				if(!s2.empty()) {
					if(!result.empty())
						result += ",";
					result += _parent + s2;
				}
			}
		_parent += "parent.";
		sdk = sdk->parent ? sdk->parent->parent : NULL;
	}
	return result;
}

Element *SDK::getManagerByName(ustring name) {
	if(name == empty) return NULL;

	SDK *sdk = this;
	while(name.substr(0, 7) == "parent.") {
		if(sdk->parent)
			sdk = sdk->parent->parent;
		else
			return NULL;
		name.erase(0, 7);
	}

	for(ElementsList::iterator e = sdk->elements.begin(); e != sdk->elements.end(); e++) {
		ElementProperty *prop = (*e)->props.getByName("Name");
		if(prop && prop->readStr() == name) {
			return *e;
		}
	}

	return NULL;
}

//-----------------------------------------------------------------------------------------------------------------

MSDK::MSDK():SDK(),
	on_run_state(this, CBT_SDK_RUN_CHANGE)
{
	pack = NULL;
	packProject = NULL;
	nextID = 1;
	running = false;
}

MSDK::~MSDK() {
	clear();
}

bool MSDK::saveToFile(const ustring &file_name) {
	ustring text;

	if(!info.empty())
		text = ustring("*") + info + LINE_END;

	text += ustring("Make(") + pack->name + ")"LINE_END;
	text += ustring("ver(") + HIASM_VERSION() + ")"LINE_END;

	saveToText(text);

	g_file_set_contents(file_name.c_str(), text.c_str(), -1, NULL);

	fileName = file_name;

	return true;
}

bool MSDK::loadFromFile(const ustring &file_name) {
	TRACE_PROC
	if(file_test(file_name, FILE_TEST_EXISTS)) {
		gchar *cont, *scont;
		gsize len;
		g_file_get_contents(file_name.c_str(), &cont, &len, NULL);
		if(len > 0) {
			scont = cont;
			cont[len-1] = '\0';
			int line = 1;
			if(SDKParseError e = loadFromText(cont, line, ELEMENT_LOAD_FILE))
				std::cout << "Error[" << line << "]: code " << e << std::endl;
			else
				DEBUG_MSG("Load " << line << " lines.")
			fileName = file_name;
			g_free(scont);

			for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++) {
				packProject = pack->getProjectByEntry((*e)->tpl->name);
				if(packProject) {
					(*e)->flag |= ELEMENT_FLG_IS_PARENT;
					break;
				}
			}

			return true;
		}
	}

	return false;
}

ustring MSDK::projectDir() {
	return g_path_get_dirname(fileName.c_str());
}

int MSDK::genNextID() {
	nextID++;
	while(getGlobalElementById(nextID))
		nextID++;
	return nextID;
}

void MSDK::run(bool as_app) {
	running = as_app;

	Element *w = NULL;
	prepareForRun(&w);

	on_run_state.run(this);
}

void MSDK::stop() {
	running = false;

	stopRun();

	on_run_state.run(this);
}
