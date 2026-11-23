/*
 * SDK.cpp
 *
 *  Created on: 02.05.2010
 *      Author: dilma
 */

#include <cstring>
#include <map>
#include <memory>
#include <algorithm>
#include <set>

#include "SDK.h"
#include "Elements.h"
#include "CoreQtRuntime.h"
#include "SchemeSyntax.h"
#include "LegacyPackCatalog.h"

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
	if(elements.empty()) return;
	selMan->clear();
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		delete *e;
	elements.clear();
}

void SDK::remove(Element *element) {
	getMSDK()->sdkLib.remove(element);
	elements.remove(element);
	on_remove_element.run(element);
	element->remove();
	delete element;
}

ustring SDK::getPath() {
	ustring res;
	SDK *sdk = this;
	while(sdk->parent) {
		res = ustring("\\") + sdk->parent->getName() + res;
		sdk = sdk->parent->parent;
	}
	return ustring("ROOT") + res + "\\";
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
	if(!e)
		return NULL;
	e->storePropPointers();
	e->id = getMSDK()->genNextID();
	elements.push_back(e);

	Gdk::Rectangle r = e->drawRect();
	on_redraw_rect.run(&r);

	on_add_element.run(e);

	return e;
}

bool SDK::replaceElement(int id, const ustring &name, ustring &error) {
    error.clear();
    Element *old = getElementById(id);
    auto fail = [&](const ustring &reason) { error = reason; return false; };
    if(!old || old->isNoDelete() || old->sdk || old->isELink())
        return fail("Select an ordinary, unlinked element; parent elements and containers cannot be replaced.");
    if(old->tpl->name == name) return false;
    if(name.empty() || name.find_first_of("/\\\r\n") != ustring::npos) return fail("Invalid replacement template name.");
    auto *pack = getMSDK()->pack;
    if(!pack || !pack->getElementByName(name)) return fail("Replacement template is unavailable.");
    std::unique_ptr<Element> replacement(createElement(name, this, old->x, old->y));
    if(!replacement || replacement->sdk || replacement->isNoDelete() || pack->getProjectByEntry(name))
        return fail("Replacement must be an ordinary element, not a project entry or container.");
    replacement->id = id;
    replacement->storePropPointers();
    auto copyProperties = [&](ElementProperties &source, ElementProperties &target) {
        for(auto *property : source) {
            auto *dest = target.getByName(property->name);
            if(!dest || property->type != dest->type) {
                if(!property->isDefault()) return fail("Replacement cannot preserve property: " + property->name);
                continue;
            }
            dest->setValue(property->value);
        }
        return true;
    };
    if(!copyProperties(old->props, replacement->props) || !copyProperties(old->sysProps, replacement->sysProps)) return false;
    replacement->init();
    for(auto *point : old->points) {
        auto *dest = replacement->points.getByName(point->name);
        if(!dest) {
            if(auto *method = replacement->tpl->conf->def.getByName(point->name)) {
                dest = replacement->addPoint(method->name, method->info, method->mType);
                dest->dataType = method->dataType;
            } else if(auto *property = replacement->props.getByPointName(point->name)) {
                property->makePoint(); dest = replacement->points.getByName(point->name);
            }
        }
        if(point->point && (!dest || point->type != dest->type ||
            (point->dataType && dest->dataType && point->dataType != dest->dataType)))
            return fail("Replacement cannot preserve connected point: " + point->name);
    }
    replacement->rePosPoints();
    for(auto *hint : old->hints) {
        const bool system = old->sysProps.getByName(hint->prop->name) == hint->prop;
        auto *property = (system ? replacement->sysProps : replacement->props).getByName(hint->prop->name);
        if(!property || property->type != hint->prop->type) return fail("Replacement cannot preserve linked hint.");
        replacement->hints.restore(hint->x, hint->y, hint->width, hint->height, property);
    }
    struct Link { ElementPoint *from; ElementPoint *to; std::vector<std::pair<double,double>> route; };
    std::vector<Link> links;
    for(auto *point : old->points) if(point->point) {
        if(point->point->parent == old && !point->isPrimary()) continue;
        Link link{replacement->points.getByName(point->name), point->point, {}};
        if(link.to->parent == old) link.to = replacement->points.getByName(link.to->name);
        auto *primary = point->isPrimary() ? point : point->point;
        for(auto *pos = primary->pos->next; pos && pos->next; pos = pos->next) link.route.emplace_back(pos->x, pos->y);
        links.push_back(std::move(link));
    }
    const bool selected = old->isSelect();
    replacement->flag |= old->flag & ELEMENT_FLG_IS_FREEZE;
    selMan->clear();
    on_remove_element.run(old);
    old->clearLinks();
    for(auto &link : links) {
        link.from->connect(link.to);
        auto *primary = link.from->isPrimary() ? link.from : link.to;
        auto *position = primary->pos;
        for(const auto &point : link.route) position = ElementPoint::addLinePoint(position, point.first, point.second);
    }
    Element *result = replacement.release();
    for(auto &element : elements) if(element == old) { element = result; break; }
    getMSDK()->sdkLib.remove(old);
    delete old;
    on_add_element.run(result);
    if(selected) selMan->select(result);
    return true;
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
		if(*text == '"')
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
	while(*text && *text != '\r' && *text != '\n')
		text++;
}

SDKParseError SDK::loadFromText(gchar *&text, int &line, int flag) {
	TRACE_PROC

	gchar *tok = NULL;
	Element *e = NULL;
	int len;

	bool first = line == 1;
	if(first && !validateSchemeSyntax(text, line)) return SDK_PERR_SYNTAX;
	if(first) line = 1;

	static _ElementInfoList einf;
	static _PostConnectList pcon;
	static _ElementLinkList elink;
	static std::map<SDK*, std::map<int, int>> pastedGroups;
	if(first) {
		pastedGroups.clear();
		einf.clear();
		pcon.clear();
		elink.clear();
	}

	while(*text) {
		tok = getToken(text, len);
		if(len == -1) break;
		//std::cout << ustring(tok, len).c_str() << std::endl;

		if(len == 3 && strncmp(tok, "Add", len) == 0) {
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
			ustring name(getTok(&text, ','));
			if(!getMSDK()->pack)
				return SDK_PERR_SYNTAX;
			// A HiAsm 4 installation may lack the element (or its configuration): the read-only legacy scheme keeps
			// a placeholder, as createElement() makes one, instead of failing as a whole.
			if(!getMSDK()->pack->getElementByName(name) && !getMSDK()->pack->isLegacy())
				return SDK_PERR_SYNTAX;
			int id = atoi(getTok(&text, ','));
			gdouble x = atof(getTok(&text, ','));
			gdouble y = atof(getTok(&text, ')'));

			// A container already creates its inner parent. Reuse it before notifying
			// single-child containers about additions to the scheme.
			Element *innerParent = parent ? parent->getParentElement() : nullptr;
			if(!(flag & ELEMENT_LOAD_PASTE) && innerParent && innerParent->tpl->name == name) {
				e = innerParent;
				e->move(x - e->x, y - e->y);
				e->clearLinks();
			}
			else if((flag & ELEMENT_LOAD_PASTE) || e || elements.empty())
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
			if(!e) return SDK_PERR_SYNTAX;

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
			if(!e) return SDK_PERR_SYNTAX;
			e->init();
			//e = NULL;
		}
		else if(len == 9 && strncmp(tok, "BEGIN_SDK", len) == 0) {
			if(!e || !e->sdk) return SDK_PERR_SYNTAX;
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
			if(!e) return SDK_PERR_SYNTAX;
			ustring name(getTok(&text, '='));
			if(name == "IsLib") {
				getMSDK()->sdkLib.add(e);
				nextLine(text);
			}
			else {
				tok = text;
				nextLine(text);
				if(*text) {
					*text = '\0';
					text++;
				}
				ustring value(tok);
				ElementProperty *prop = e->sysProps.getByName(name);
				if(prop) {
					if(!prop->initFromText(value)) return SDK_PERR_SYNTAX;
					if((flag & ELEMENT_LOAD_PASTE) && name.lowercase() == "group" && prop->readInt() > 0) {
						int &id = pastedGroups[this][prop->readInt()];
						if(!id) id = selMan->availableGroupId();
						if(!id) return SDK_PERR_SYNTAX;
						prop->setValue(&id);
					}
				}
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
		else if(len == 3 && strncmp(tok, "Pos", len) == 0) {
			// HiAsm 4 keeps the position of the scheme view here; the view is not part of the build.
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
			getTok(&text, ')');
		}
		else if(len == 6 && strncmp(tok, "PColor", len) == 0) {
			// Colour of a point of HiAsm 4 (dynamic points of child panels): the colour is not part of the build.
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
			getTok(&text, ')');
		}
		else if(len == 4 && strncmp(tok, "link", len) == 0) {
			if(!e) return SDK_PERR_SYNTAX;
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
			_PostConnect pc;
			gchar *p2 = getTok(&text, ',');
			pc.e1 = e;
			pc.p1 = p2;
			pc.e2 = atoi(getTok(&text, ':'));
			pc.p2 = getTok(&text, ',');
			if(*getToken(text, len) != '[') return SDK_PERR_SYNTAX;
			pc.path = getTok(&text, ']');
			if(*text != ')') return SDK_PERR_SYNTAX;
			text++;

			pcon.push_back(pc);
		}
		else if(len == 5 && strncmp(tok, "elink", len) == 0) {
			if(!e) return SDK_PERR_SYNTAX;
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
			elink.push_back(_ElementLink(e, atoi(getTok(&text, ')'))));
		}
		else if(len == 5 && strncmp(tok, "Point", len) == 0) {
			if(!e) return SDK_PERR_SYNTAX;
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
			if(!e) return SDK_PERR_SYNTAX;
			if(*getToken(text, len) != '(') return SDK_PERR_SYNTAX;
            int _x = atoi(getTok(&text,','));
            int _y = atoi(getTok(&text,','));
            int _w = atoi(getTok(&text,','));
            int _h = atoi(getTok(&text,','));

            char *name = getTok(&text,')');
            ElementProperty *prop;
            if(*name == '@') {
                prop = e->sysProps.getByName(name+1);
            }
            else {
                prop = e->props.getByName(name);
            }
            if(!prop) {
                ERROR_MSG("prop " << name << " not found!")
            }
            else {
                e->hints.restore(_x, _y, _w, _h, prop);
            }
			nextLine(text);
		}
		else if(len == 7 && strncmp(tok, "MakeExt", len) == 0) {
			// TODO
			nextLine(text);
		}
		else if(e) {
			ustring pname(tok, len);
			ElementProperty *prop = e->props.getByName(pname);
			if(*text != '=')
				return SDK_PERR_SYNTAX;
			++text;
			tok = text;
			nextLine(text);
			if(*text) {
				*text = '\0';
				text++;
			}

			if(prop) {
				if(!prop->initFromText(ustring(tok))) return SDK_PERR_SYNTAX;
			}
			else {
				prop = e->sysProps.getByName(pname);
				if(prop) {
					if(!prop->initFromText(ustring(tok))) return SDK_PERR_SYNTAX;
				}
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
		// Validate the entire batch before changing any wire. A duplicate input
		// or output used to orphan its first connection and corrupt route memory.
		std::set<ElementPoint*> occupied;
		// A link to an optional point (a method marked with '*' in the configuration) creates the point, as HiAsm 4
		// does: schemes do not always declare it with Point(...) before the link.
		auto optionalPoint = [](Element *element, const ustring &name) -> ElementPoint* {
			if(!element) return nullptr;
			if(ElementPoint *existing = element->getPointByName(name)) return existing;
			ConfMethod *method = element->tpl->conf->def.getByName(name);
			if(!method) return nullptr;
			ElementPoint *created = element->addPoint(method->name, method->info, method->mType);
			created->dataType = method->dataType;
			element->rePosPoints();
			return created;
		};
		// A read-only legacy scheme keeps loading when one of its links cannot be made: the link is reported and skipped.
		const bool tolerantLinks = getMSDK()->pack && getMSDK()->pack->isLegacy();
		for(auto connection = pcon.begin(); connection != pcon.end();) {
			Element *target = (flag & ELEMENT_LOAD_PASTE) ? _getElementById(einf, connection->e2) : connection->e1->parent->getElementById(connection->e2);
			ElementPoint *from = optionalPoint(connection->e1, connection->p1);
			ElementPoint *to = optionalPoint(target, connection->p2);
			if(!from || !to || !from->isPrimary() || !from->canConnect(to) || occupied.count(from) || occupied.count(to)) {
				std::cerr << "Missing, invalid or occupied connection: " << connection->e1->id << ':' << connection->p1
					<< " -> " << connection->e2 << ':' << connection->p2 << std::endl;
				if(!tolerantLinks) {
					einf.clear(); pcon.clear(); elink.clear();
					return SDK_PERR_SYNTAX;
				}
				connection = pcon.erase(connection);
				continue;
			}
			occupied.insert(from); occupied.insert(to);
			++connection;
		}
		for(_PostConnectList::iterator p = pcon.begin(); p != pcon.end(); p++) {
			Element *e2 = (flag&ELEMENT_LOAD_PASTE) ? _getElementById(einf, (*p).e2) : (*p).e1->parent->getElementById((*p).e2);
			if(e2) {
				ElementPoint *p2 = e2->getPointByName((*p).p2);
				if(p2) {
					ElementPoint *p1 = (*p).e1->getPointByName((*p).p1);
					if(p1) {
						p1->connect(p2);
						gchar *path = new gchar[(*p).path.length()+1], *del_path = path;
						memcpy(path, (*p).path.c_str(), (*p).path.length());
						path[(*p).path.length()] = '\0';
						PointPos *pp = p1->pos;
						while(*path == '(') {
							path++;
							int x = atoi(getTok(&path, ','));
							int y = atoi(getTok(&path, ')'));
							pp = p1->addLinePoint(pp, x, y);
						}
						delete[] del_path;
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
		if((*e)->isParent() && ((*e)->isCore() || (CoreQtRuntime::active() && CoreQtRuntime::isWidget(*e)))) {
			main = *e;
			break;
		}

	if(main) {
		if(CoreQtRuntime::prepare(main, *parent)) *parent = main;
		else if(auto *core = dynamic_cast<ElementCore*>(main)) core->prepareForRun(parent);
	}

	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++) {
		if(*e == main) continue;
		if(CoreQtRuntime::prepare(*e, *parent)) continue;
		if((*e)->isCore()) {
			dynamic_cast<ElementCore*>(*e)->prepareForRun(parent);
		}
		else if(CoreQtRuntime::active() && ((*e)->tpl->name == "hcChildForm" || (*e)->tpl->name == "hcChildPanel") && (*e)->sdk) {
			Element *owner = *parent;
			(*e)->sdk->prepareForRun(&owner);
		}
	}

	if(main && !CoreQtRuntime::show(main))
		if(auto *control = dynamic_cast<ElementTplParent*>(main)) control->run();
}

void SDK::stopRun() {
	Element *main = NULL;
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		if((*e)->isParent() && ((*e)->isCore() || CoreQtRuntime::owns(*e))) {
			main = *e;
			break;
		}

	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++) {
		if(*e == main) continue;
		if((*e)->isCore() && !CoreQtRuntime::owns(*e)) {
			dynamic_cast<ElementCore*>(*e)->stopRun();
		}
		else if(CoreQtRuntime::active() && ((*e)->tpl->name == "hcChildForm" || (*e)->tpl->name == "hcChildPanel") && (*e)->sdk)
			(*e)->sdk->stopRun();
	}

	if(main && !CoreQtRuntime::owns(main))
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
	nextID = 0;
	running = false;
}

MSDK::~MSDK() {
	clear();
}

bool MSDK::saveToFile(const ustring &file_name) {
	if(packSet && packSet->isLegacyFile(file_name)) return false;
	if(!pack)
		return false;
	ustring text;

	if(!info.empty())
		text = ustring("*") + info + LINE_END;

	text += ustring("Make(") + pack->name + ")" LINE_END;
	text += ustring("ver(") + HIASM_VERSION() + ")" LINE_END;

	saveToText(text);

	GError *error = NULL;
	if(!g_file_set_contents(file_name.c_str(), text.c_str(), -1, &error)) {
		if(error) {
			DEBUG_MSG("Save failed: " << error->message)
			g_error_free(error);
		}
		return false;
	}

	fileName = file_name;

	return true;
}

// A file inside a HiAsm 4 installation, including the packages that this program provides natively: such a
// package is not in the collection (its legacy copy is shadowed), so the installation roots are checked.
static bool inHiAsm4Installation(const ustring &file) {
	std::vector<std::string> diagnostics;
	for(const auto &location : LegacyPackCatalog::discover(LegacyPackCatalog::configurationPath(dataDir.raw()), diagnostics))
		if(LegacyPackCatalog::containsPath(location.installation, file.raw())) return true;
	return false;
}

// HiAsm 4 tutorials may leave out the Make line: a scheme inside a HiAsm 4 installation is a Delphi scheme.
static Pack *legacyDefaultPack(PackCollection *packs, const ustring &file) {
	if(!packs) return nullptr;
	for(Pack *installed : *packs)
		if(installed->isLegacy() && LegacyPackCatalog::containsPath(installed->legacyRoot().raw(), file.raw()))
			return packs->getPackByName("delphi");
	return nullptr;
}

bool MSDK::loadFromFile(const ustring &file_name) {
	TRACE_PROC
	gchar *contents = NULL;
	gsize length = 0;
	GError *error = NULL;
	if(!g_file_get_contents(file_name.c_str(), &contents, &length, &error)) {
		if(error) {
			DEBUG_MSG("Load failed: " << error->message)
			g_error_free(error);
		}
		return false;
	}
	if(contents && length && !g_utf8_validate(contents, length, NULL)) {
		// Files of a HiAsm 4 installation are CP1251, also the examples of a package that this program provides
		// natively (web). Files of this program stay strictly UTF-8.
		bool legacyText = inHiAsm4Installation(file_name);
		if(!legacyText) {
			const std::string header(contents, std::min<size_t>(length, 256));
			const auto make = header.find("Make(");
			const auto end = make == std::string::npos ? std::string::npos : header.find(')', make);
			Pack *legacy = end != std::string::npos && packSet ? packSet->getPackByName(header.substr(make+5, end-make-5)) : nullptr;
			if(!legacy && make == std::string::npos) legacy = legacyDefaultPack(packSet, file_name);
			legacyText = legacy && legacy->isLegacy();
		}
		if(legacyText) {
			const auto utf8 = LegacyPackCatalog::decodeText(contents, length);
			g_free(contents); contents = g_strndup(utf8.data(), utf8.size()); length = utf8.size();
		}
	}
	if(!contents || length == 0 || !g_utf8_validate(contents, length, NULL)) {
		g_free(contents);
		return false;
	}

	int line = 1;
	// A UTF-8 byte-order mark, which some editors write at the start of a file, is not part of the scheme text.
	gchar *cursor = contents;
	if(length >= 3 && memcmp(contents, "﻿", 3) == 0) cursor += 3;
	// HiAsm 4 tutorials may leave out the Make line: such a scheme of a HiAsm 4 installation is a Delphi scheme.
	if(!getMSDK()->pack && !strstr(cursor, "Make("))
		getMSDK()->pack = legacyDefaultPack(packSet, file_name);
	SDKParseError parse_error = loadFromText(cursor, line, ELEMENT_LOAD_FILE);
	g_free(contents);
	if(parse_error) {
		std::cout << "Error[" << line << "]: code " << parse_error << std::endl;
		return false;
	}
	if(!pack) return false;
	fileName = file_name;
	if(pack) {
		for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++) {
			packProject = pack->getProjectByEntry((*e)->tpl->name);
			if(packProject) {
				(*e)->flag |= ELEMENT_FLG_IS_PARENT;
				break;
			}
		}
	}
	return true;
}

bool MSDK::isCoreDocument() const {
    if(!pack || pack->name != "_base") return false;
    for(auto element : elements)
        if(element->tpl->name == "hcTplForm" || element->tpl->name == "hcTplDialog") return false;
    return true;
}

bool MSDK::isInstalledCoreScheme() const {
    if(fileName.empty()) return false;
    auto file = g_file_new_for_path(fileName.c_str());
    auto directory = g_file_new_for_path((dataDir + "packs/_base").c_str());
    const bool installed = g_file_has_prefix(file, directory);
    g_object_unref(directory);
    g_object_unref(file);
    return installed;
}

ustring MSDK::projectDir() {
	gchar *path = g_path_get_dirname(fileName.c_str());
	ustring result(path);
	g_free(path);
	return result;
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
