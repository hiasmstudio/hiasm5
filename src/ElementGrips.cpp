/*
 * ElementGrips.cpp
 *
 *  Created on: Jun 20, 2010
 *      Author: dilma
 */

#include "ElementGrips.h"

ElementGrip::~ElementGrip() {
	clear();
}

void ElementGrip::clear() {
	for(GripList::iterator g = grips.begin(); g != grips.end(); g++)
		delete *g;
	grips.clear();
}

void ElementGrip::drawGrips(DrawContext dc) {
	for(GripList::iterator g = grips.begin(); g != grips.end(); g++)
		on_grip_draw(dc, *g);
}

GripInfo *ElementGrip::getGripAtPos(gdouble x, gdouble y) {
	for(GripList::iterator g = grips.begin(); g != grips.end(); g++)
		if((*g)->x <= x && (*g)->y <= y && ((*g)->x + (*g)->width) >= x && ((*g)->y + (*g)->height) >= y)
			return *g;
	return NULL;
}

GripInfo *ElementGrip::gripDown(gdouble x, gdouble y) {
	active = getGripAtPos(x, y);
	startx = x;
	starty = y;
	return active;
}

void ElementGrip::gripMove(gdouble x, gdouble y) {
	if(active) {
		int dx = (int)(x - startx);
		int dy = (int)(y - starty);

		int v = active->stepx;
		if(v > 1)
			dx = (abs(dx) >= v) ? (int)(dx / v)*v : 0;

		v = active->stepy;
		if(v > 1)
			dy = (abs(dy) >= v) ? (int)(dy / v)*v : 0;

		if((dx || dy) && on_grip_can_change(active, dx, dy)) {
			for(GripValueList::iterator v = active->values.begin(); v != active->values.end(); v++)
				*v->value += dx*v->dirx + dy*v->diry;
			active->x += dx;
			active->y += dy;
			on_grip_change(active);
			startx += dx;
			starty += dy;
		}
	}
}

void ElementGrip::gripUp(gdouble x, gdouble y) {
	active = NULL;
}

GripInfo *ElementGrip::addGrip(gdouble x, gdouble y, int width, int height, int sx, int sy, int cursor) {
	GripInfo *g = new GripInfo();
	g->x = x;
	g->y = y;
	g->width = width;
	g->height = height;
	g->stepx = sx;
	g->stepy = sy;
	g->tag = 0;
	g->cursor = cursor;

	grips.push_back(g);
	return g;
}
