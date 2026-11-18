/*
 * LayerObjects.cpp
 *
 *  Created on: 18.05.2010
 *      Author: dilma
 */

#include "LayerObjects.h"
#include "HintLayout.h"

LayerObject::LayerObject(Widget *parent) {
    x = y = 0; width = height = 0; dead = false;
	this->parent = parent;
	visible = false;
	timerID = 0;
}

LayerObject::~LayerObject() {
	if(timerID)
		g_source_remove(timerID);
}

bool LayerObject::inRect(gdouble mx, gdouble my) {
	return mx >= x && mx <= x + width && my >= y && my <= y + height;
}

gboolean LayerObject::showTimeout(void *data) {
    auto *object = static_cast<LayerObject*>(data);
    object->timerID = 0;
    if(!object->dead) { object->visible = true; object->invalidate(); }
    return G_SOURCE_REMOVE;
}

void LayerObject::show(gdouble x, gdouble y, bool delay) {
    if(timerID) { g_source_remove(timerID); timerID = 0; }
    if(dead) return;
    if(visible) invalidate();
    visible = false;
	if(delay)
		timerID = g_timeout_add(500, showTimeout, this);
	else {
		visible = true;
		invalidate();
	}
}

void LayerObject::invalidate() {
	if(parent) {
        // First layout may be wider than the previous hint or the old 400px area.
        widget_invalidate(*parent);
	}
}

//------------------------------------------------------------------------------------------------

LO_PointHint::LO_PointHint(ElementPoint *point, double zoom, Widget *parent):LayerObject(parent) {
	info.push_back(point->name);

	unsigned int i = 0, s = 0;
	while(i < point->info.length()) {
		if(i - s > 30 && point->info[i] == ' ') {
			info.push_back(ustring(point->info, s, i - s));
			s = i + 1;
		}
		i++;
	}
	if(i - s)
		info.push_back(ustring(point->info, s, i - s));

	width = 0;
	height = 0;

	x = (point->pos->x)*zoom + 16;
	y = point->pos->y*zoom;
	type = point->type;

	typeIcon = getPointIcon(point->type);
}

LO_PointHint::~LO_PointHint() {

}

void LO_PointHint::draw(DrawContext dc) { drawLayerHint(dc, info, x, y, width, height, parent, typeIcon); }

LO_ElementHint::LO_ElementHint(Element *e, double zoom, Widget *parent):LayerObject(parent) {
	info.push_back(e->tpl->name);
	info.push_back("-");

	unsigned int i = 0, s = 0;
	ustring si = e->tpl->conf->info;
	while(i < si.length()) {
		if(i - s > 30 && si[i] == ' ') {
			info.push_back(ustring(si, s, i - s));
			s = i + 1;
		}
		i++;
	}
	if(i - s)
		info.push_back(ustring(si, s, i - s));
	info.push_back("-");
	unsigned int sz = info.size();
	for(ElementProperties::iterator prop = e->props.begin(); prop != e->props.end(); prop++)
		if(!(*prop)->isDefault()) {
			ustring text = (*prop)->serialize();
			if(text.length() > 30)
				text = ustring(text, 0, 30) + "...";
			info.push_back((*prop)->name + " = " + text);
		}
	if(sz == info.size()) {
		info.push_back("(default)");
	}

	width = 0;
	height = 0;

	x = (e->x + e->width + 4)*zoom;
	y = e->y*zoom;
}

void LO_ElementHint::draw(DrawContext dc) { drawLayerHint(dc, info, x, y, width, height, parent); }

void LO_ElementHint::show(gdouble x, gdouble y, bool delay) {
	this->x = x + 16;
	this->y = y;
	LayerObject::show(x, y, delay);
}
