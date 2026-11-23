/*
 * SelectManager.cpp
 *
 *  Created on: 05.05.2010
 *      Author: dilma
 */

#include "SelectManager.h"
#include "SchemeSyntax.h"
#include <set>
#include <climits>

SelectManager::SelectManager(SDK *sdk):
	on_selection_change(this, CBT_SDK_CHANGE_SELECT)
{
	this->sdk = sdk;
	drawEnable = true;
}

void SelectManager::reDraw(const Gdk::Rectangle &r) {
	if(drawEnable)
		sdk->on_redraw_rect.run(&r);
}

void SelectManager::reDrawAll() {
	if(drawEnable)
		sdk->on_redraw_rect.run(NULL);
}

void SelectManager::add(Element *element) {
	if(!element) return;
	bool changed = false;
	auto append = [&](Element *item) {
		if(std::find(elements.begin(), elements.end(), item) != elements.end()) return;
		elements.push_back(item);
		item->flag |= ELEMENT_FLG_IS_SELECT;
		reDraw(item->drawRect());
		changed = true;
	};
	append(element);
	if(element->groupId() > 0 && element->parent == sdk)
		for(Element *item : sdk->elements)
			if(item->groupId() == element->groupId()) append(item);
	if(changed) on_selection_change.run(NULL);
}

int SelectManager::availableGroupId() const {
	std::set<int> used;
	for(Element *element : sdk->elements)
		if(element->groupId() > 0) used.insert(element->groupId());
	int id = 1;
	while(used.count(id)) { if(id == INT_MAX) return 0; ++id; }
	return id;
}

bool SelectManager::group() {
	if(elements.size() < 2) return false;
	const int previous = elements.front()->groupId();
	if(previous > 0 && std::all_of(elements.begin(), elements.end(), [previous](Element *e) { return e->groupId() == previous; })) return false;
	int id = availableGroupId();
	if(!id) return false;
	for(Element *element : elements) element->sysProps.getByName("Group")->setValue(&id);
	on_selection_change.run(NULL);
	return true;
}

bool SelectManager::ungroup() {
	std::set<int> groups;
	for(Element *element : elements)
		if(element->groupId() > 0) groups.insert(element->groupId());
	int zero = 0;
	for(Element *element : sdk->elements)
		if(groups.count(element->groupId())) element->sysProps.getByName("Group")->setValue(&zero);
	if(!groups.empty()) on_selection_change.run(NULL);
	return !groups.empty();
}

void SelectManager::select(Element *element) {
	clear();
	add(element);
}

void SelectManager::selectRect(gdouble x1, gdouble y1, gdouble x2, gdouble y2) {
	gdouble nx1 = min(x1, x2);
	gdouble ny1 = min(y1, y2);
	gdouble nx2 = max(x1, x2);
	gdouble ny2 = max(y1, y2);
	clear();
	bool de = drawEnable;
	drawEnable = false;
	on_selection_change.enabled = false;
	for(ElementsList::iterator e = sdk->elements.begin(); e != sdk->elements.end(); e++) {
		if((*e)->checkColliseRect(nx1, ny1, nx2, ny2))
			add(*e);
	}
	drawEnable = de;
	on_selection_change.enabled = true;

	if(!elements.empty()) {
		on_selection_change.run(NULL);
		reDraw(getSelectionRect());
	}
}

void SelectManager::selectByEdge(SBE_Types mode, gdouble x, gdouble y){
	clear();
	bool de = drawEnable;
	drawEnable = false;
	on_selection_change.enabled = false;
	for (ElementsList::iterator it = sdk->elements.begin(); it != sdk->elements.end(); it++){
		switch(mode){
			case SBE_EDGE_VERTICAL:
				if ((*it)->y > y) add(*it);
				break;
			case SBE_EDGE_HORIZONTAL:
				if ((*it)->x > x) add(*it);
				break;
			case SBE_EDGE_BOTH:
				if (((*it)->x > x) || ((*it)->y > y)) add(*it);
				break;
		}
	}
	drawEnable = de;
	on_selection_change.enabled = true;
	if (!elements.empty()){
		on_selection_change.run(NULL);
		reDraw(getSelectionRect());
	}
}

void SelectManager::clear() {
	if(elements.empty()) return;

	Gdk::Rectangle r = getSelectionRect();

	unsetFlag(ELEMENT_FLG_IS_SELECT);
	elements.clear();

	on_selection_change.run(NULL);

	reDraw(r);
}

void SelectManager::move(gdouble dx, gdouble dy) {
	Gdk::Rectangle r = getSelectionRect();

	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		(*e)->move(dx, dy);

	reDraw(r.join(getSelectionRect()));
}

void SelectManager::erase() {
	if(elements.empty()) return;

	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		sdk->remove(*e);
	elements.clear();

	on_selection_change.run(NULL);

	reDrawAll();
}

Gdk::Rectangle SelectManager::getSelectionRect() {
	Gdk::Rectangle r(0,0,0,0);
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		if(r.has_zero_area())
			r = (*e)->drawRect();
		else
			r = r.join((*e)->drawRect());
	return r;
}

Gdk::Rectangle SelectManager::getBodyRect() const {
	Gdk::Rectangle bounds;
	bool first = true;
	for(Element *e : elements) {
		Gdk::Rectangle body(e->x, e->y, e->width, e->height);
		bounds = first ? body : bounds.join(body);
		first = false;
	}
	return bounds;
}

bool SelectManager::moveTo(int x, int y) {
	if(elements.empty()) return false;
	const Gdk::Rectangle bounds = getBodyRect();
	const gdouble dx = gdouble(x) - bounds.get_x();
	const gdouble dy = gdouble(y) - bounds.get_y();
	if(dx == 0 && dy == 0) return false;
	bool changed = false;
	for(Element *e : elements) {
		if(e->isFreeze()) continue;
		const gdouble nx = e->x + dx, ny = e->y + dy;
		if(nx < G_MININT || nx > G_MAXINT || ny < G_MININT || ny > G_MAXINT) return false;
		changed = true;
	}
	if(changed) move(dx, dy);
	return changed;
}

bool SelectManager::align(const ustring &mode) {
	if(elements.size() < 2) return false;
	if(mode != "left" && mode != "right" && mode != "center" &&
	   mode != "top" && mode != "bottom" && mode != "middle") return false;
	const Gdk::Rectangle bounds = getBodyRect();
	bool changed = false;
	for(Element *e : elements) {
		if(e->isFreeze()) continue;
		int x = e->x, y = e->y;
		if(mode == "left") x = bounds.get_x();
		else if(mode == "right") x = bounds.get_x() + bounds.get_width() - e->width;
		else if(mode == "center") x = bounds.get_x() + (bounds.get_width() - e->width) / 2;
		else if(mode == "top") y = bounds.get_y();
		else if(mode == "bottom") y = bounds.get_y() + bounds.get_height() - e->height;
		else y = bounds.get_y() + (bounds.get_height() - e->height) / 2;
		if(x != e->x || y != e->y) {
			e->move(gdouble(x) - e->x, gdouble(y) - e->y);
			changed = true;
		}
	}
	if(changed) reDrawAll();
	return changed;
}

bool SelectManager::canDelete() {
	if(elements.empty())
		return false;

	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		if((*e)->isNoDelete())
			return false;

	return true;
}

Element *SelectManager::getFirst() {
	if(elements.empty())
		return NULL;
	ElementsList::iterator t = elements.begin();
	Element *e = *t++;
	if(t == elements.end())
		return e;
	return NULL;
}

bool SelectManager::checkFlag(int flag) {
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		if(!((*e)->flag & flag))
			return false;
	return true;
}

void SelectManager::setFlag(int flag) {
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		(*e)->flag |= flag;
}

void SelectManager::unsetFlag(int flag) {
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		(*e)->flag &= ~flag;
}

void SelectManager::clearLinks() {
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		(*e)->clearLinks();
}

void SelectManager::selectAll() {
	bool de = drawEnable;
	drawEnable = false;
	on_selection_change.enabled = false;

	clear();

	for(ElementsList::iterator e = sdk->elements.begin(); e != sdk->elements.end(); e++)
		add(*e);

	drawEnable = de;
	on_selection_change.enabled = true;
	on_selection_change.run(NULL);

	reDrawAll();
}

void SelectManager::copy() {
	ustring text;
	for(ElementsList::iterator e = elements.begin(); e != elements.end(); e++)
		(*e)->saveToText(text, "", ELEMENT_SAVE_SELECTED | ELEMENT_SAVE_CHANGED);
	clipboard_set_text(text);
}

void SelectManager::copyAsLink() {
	ustring text;
	getFirst()->saveAsLink(text);
	clipboard_set_text(text);
}

void SelectManager::getClipboardText(const ustring &text) {
	pasteText(text);
}

bool SelectManager::pasteText(const ustring &text) {
	int syntaxLine = 1;
	if(text.empty() || !validateSchemeSyntax(text.raw(), syntaxLine)) return false;
	gchar *buf = new gchar[text.bytes()+1], *sbuf;
	strcpy(buf, text.c_str());
	int line = 1;
	sbuf = buf;

	bool de = drawEnable;
	drawEnable = false;
	on_selection_change.enabled = false;
	const ElementsList previous = elements;
	clear();

	SDKParseError e = sdk->loadFromText(buf, line, ELEMENT_LOAD_PASTE);
	if(e) {
		std::cout << "Error parse[" << line << "]: code " << e << std::endl;
		// Only freshly inserted root elements are selected by the paste parser.
		ElementsList inserted = elements;
		elements.clear();
		for(Element *element : inserted) sdk->remove(element);
		for(Element *element : previous) add(element);
	}
	delete[] sbuf;

	drawEnable = de;
	on_selection_change.enabled = true;
	on_selection_change.run(NULL);
	if(e) { reDrawAll(); return false; }

	Gdk::Rectangle r = getSelectionRect();
	gdouble dx = GRID_X(sdk->pasteX - r.get_x());
	gdouble dy = GRID_Y(sdk->pasteY - r.get_y());
	move(dx, dy);
	sdk->pasteX += POINT_SPACE;
	sdk->pasteY += POINT_SPACE;

	reDrawAll();
	return true;
}

void SelectManager::paste() {
	clipboard_request_text(sigc::mem_fun(*this, &SelectManager::getClipboardText));
}
