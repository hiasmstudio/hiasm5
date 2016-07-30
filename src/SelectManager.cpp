/*
 * SelectManager.cpp
 *
 *  Created on: 05.05.2010
 *      Author: dilma
 */

#include "SelectManager.h"

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
	elements.push_back(element);
	element->flag |= ELEMENT_FLG_IS_SELECT;
	reDraw(element->drawRect());
	on_selection_change.run(NULL);
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
			r.join((*e)->drawRect());
	return r;
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
		(*e)->flag ^= flag;
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
	Clipboard::get()->set_text(text);
}

void SelectManager::copyAsLink() {
	ustring text;
	getFirst()->saveAsLink(text);
	Clipboard::get()->set_text(text);
}

void SelectManager::getClipboardText(const ustring &text) {
	gchar *buf = new gchar[text.bytes()+1], *sbuf;
	strcpy(buf, text.c_str());
	int line = 1;
	sbuf = buf;

	bool de = drawEnable;
	drawEnable = false;
	on_selection_change.enabled = false;
	clear();

	if(SDKParseError e = sdk->loadFromText(buf, line, ELEMENT_LOAD_PASTE))
		std::cout << "Error parse[" << line << "]: code " << e << std::endl;
	delete sbuf;

	drawEnable = de;
	on_selection_change.enabled = true;
	on_selection_change.run(NULL);

	Gdk::Rectangle r = getSelectionRect();
	gdouble dx = GRID_X(sdk->pasteX - r.get_x());
	gdouble dy = GRID_Y(sdk->pasteY - r.get_y());
	move(dx, dy);
	sdk->pasteX += POINT_SPACE;
	sdk->pasteY += POINT_SPACE;

	reDrawAll();
}

void SelectManager::paste() {
	Clipboard::get()->request_text(sigc::mem_fun(*this, &SelectManager::getClipboardText));
}
