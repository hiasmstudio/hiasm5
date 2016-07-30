/*
 * ElementPoint.cpp
 *
 *  Created on: 17.05.2010
 *      Author: dilma
 */

#include "ElementPoint.h"
#include "Data.h"
#include "Elements.h"

ElementPoint::ElementPoint(Element *parent, const ustring &name, const ustring &info, int type) {
	this->name = name;
	this->info = info;
	this->type = type;
	this->parent = parent;
	dataType = data_null;

	pos = new PointPos();
	pos->x = 0;
	pos->y = 0;

	point = NULL;
	flag = 0;
}

ElementPoint::~ElementPoint() {
	clear();
	delete pos;
}

ElementPoint *ElementPoint::connect(ElementPoint *link) {
	ElementPoint *p1, *p2;

	if(isPrimary()) {
		p1 = this;
		p2 = link;
	}
	else {
		p1 = link;
		p2 = this;
	}
	p1->point = p2;
	p2->point = p1;
	p1->pos->next = p2->pos;
	p2->pos->prev = p1->pos;

	return this;
}

bool ElementPoint::canConnect(ElementPoint *link) {
	return link && !point && !link->point && abs(type + link->type - 5) == 2;
}

void ElementPoint::clear() {
	if(pos->next) {
		Gdk::Rectangle r = drawRect();
		while(pos->next->next) {
			PointPos *pb = pos->next->next;
			delete pos->next;
			pos->next = pb;
		}
		pos->next = NULL;
		parent->parent->on_redraw_rect.run(&r);
	}
	else pos->prev = NULL;

	if(point) {
		point->point = NULL;
		point->clear();
		point = NULL;
	}
}

typedef struct {
  int x,y;
  int l;
  unsigned char side;
} vector;

const int kx[4] = {1,0,-1,0};
const int ky[4] = {0,1,0,-1};

void moveVector(vector &v, int l) {
	if(l*kx[v.side] > 0) v.x += l;
	if(l*ky[v.side] > 0) v.y += l;
}

bool crossVectors(const vector &v1, const vector &v2, int &cx, int &cy) {
	cx = 0;
	cy = 0;

	if(abs(v1.side - v2.side) % 2 == 0)
		return false;

	// todo
	if(kx[v1.side] == 0) {
		cx = (int)((v1.x - v2.x)/kx[v2.side]);
		if(cx == 0) cx = -POINT_SPACE;
		if(cx < 0) return false;
	}
	else {
		cx = (int)((v2.x - v1.x)/kx[v1.side]);
		if(cx == 0) cx = -POINT_SPACE;
		if(cx < 0) return false;
	}

	if(ky[v1.side] == 0) {
		cy = (int)((v1.y - v2.y)/ky[v2.side]);
		if(cy == 0) cy = -POINT_SPACE;
		if(cy < 0) return false;
	}
	else {
		cy = (int)((v2.y - v1.y)/ky[v1.side]);
		if(cy == 0) cy = -POINT_SPACE;
		if(cy < 0) return false;
	}

	return true;
}

void rotateRight(vector &v) {
	if(v.side == 3)
		v.side = 0;
	else v.side++;
}

void rotateLeft(vector &v) {
	if(v.side == 0)
		v.side = 3;
	else v.side--;
}

int calcIncX(const vector &v1, const vector &v2) {
	if(v1.side == v2.side && v2.y != v1.y)
		return v2.x + kx[v2.side]*v2.l - v1.x;
	else
		return (int)((v2.x - v1.x)/2);
}

int calcIncY(const vector &v1, const vector &v2) {
	if(v1.side == v2.side && v2.x != v1.x)
		return v2.y + ky[v2.side]*v2.l - v1.y;
	else
		return (int)((v2.y - v1.y)/2);
}

void tracePath(PointPos *p, const vector &v1, const vector &v2) {
	if(v1.side != v2.side) {
		if(v1.x == v2.x) {
			if( (ky[v1.side] == 1 && v1.y < v2.y) ||
				(ky[v2.side] == 1 && v2.y < v1.y)) return;
		}
		else if(v1.y == v2.y) {
			if( (kx[v1.side] == 1 && v1.x < v2.x) ||
				(kx[v2.side] == 1 && v2.x < v1.x)) return;
		}
	}

	vector t = v1, old;
	//unsigned char step = 0;
	int cx, cy;

	while(!crossVectors(t, v2, cx, cy)) {
		old = t;
		if(kx[t.side]) moveVector(t, calcIncX(t, v2));
		if(ky[t.side]) moveVector(t, calcIncY(t, v2));

		if(/*step == 0 &&*/ old.x == t.x && old.y == t.y) {
			t.x += t.l*kx[t.side];
			t.y += t.l*ky[t.side];
		}
		p = ElementPoint::addLinePoint(p, t.x, t.y);
		switch(t.side) {
			case 0:	if(v2.y > t.y) rotateRight(t); else rotateLeft(t); break;
			case 1:	if(v2.x > t.x) rotateLeft(t); else rotateRight(t); break;
			case 2:	if(v2.y > t.y) rotateLeft(t); else rotateRight(t); break;
			case 3: if(v2.x > t.x) rotateRight(t); else rotateLeft(t); break;
		}
	}
	t.x += cx*kx[t.side];
	t.y += cy*ky[t.side];
	ElementPoint::addLinePoint(p, t.x, t.y);
}

void ElementPoint::createPath() {
	if(!point->point) return;

	ElementPoint *point1 = isPrimary() ? this : point;
	ElementPoint *point2 = point1->point;

	vector v1;
	v1.x = point1->pos->x;
	v1.y = point1->pos->y;

	if(point1->parent->cIndex == CI_HubEx || point1->parent->cIndex == CI_GetDataEx) {
		v1.side = dynamic_cast<ElementHubEx*>(point1->parent)->calcSide(point1);
		v1.l = 2*POINT_OFF;
		int i = v1.side;
		DEBUG_MSG("Side is " << i)
	}
//	else if TElement(Point1.Parent.Element) is TGetDataEx then
//	begin
//	  v1.side := TGetDataEx(Point1.Parent.Element).CalcSide(Point1);
//	  v1.l := point_off;
//	end
	else {
		v1.l = POINT_SPACE*2;
		switch(point1->type) {
			case pt_event: v1.side = 0; break;
			case pt_data: v1.side = 3; break;
			case pt_work: v1.side = 2; break;
			case pt_var: v1.side = 1; break;
		}
	}

	vector v2;
	v2.x = point2->pos->x;
	v2.y = point2->pos->y;

	if(point2->parent->cIndex == CI_HubEx || point2->parent->cIndex == CI_GetDataEx) {
		v2.side = dynamic_cast<ElementHubEx*>(point2->parent)->calcSide(point2);
		v2.l = 2*POINT_OFF;
		int i = v2.side;
		DEBUG_MSG("Side is " << i)
	}
//	else if TElement(Point2.Parent.Element) is TGetDataEx then
//	begin
//	  v2.side := TGetDataEx(Point2.Parent.Element).CalcSide(Point2);
//	  v2.l := point_off;
//	end
	else {
		v2.l = POINT_SPACE*2;
		switch(point2->type) {
			case pt_event: v2.side = 0; break;
			case pt_data: v2.side = 3; break;
			case pt_work: v2.side = 2; break;
			case pt_var: v2.side = 1; break;
		}
	}

	tracePath(point1->pos, v1, v2);

	Gdk::Rectangle r = drawRect();
	parent->parent->on_redraw_rect.run(&r);

}

void ElementPoint::movePoints(PointPos *pb, int dx, int dy) {
	while(pb->next) {
		pb->x += dx;
		pb->y += dy;
		pb = pb->next;
	}
}

void ElementPoint::movePos(PointPos *pb, int x, int y) {
	if(pb->next->next && pb->x == pb->next->x && pb->y != pb->next->y) {
		pb->x = x;
		pb->next->x = x;
	}
	else if(pb->prev->prev && pb->x == pb->prev->x && pb->y != pb->prev->y) {
		pb->x = x;
		pb->prev->x = x;
	}
	else if(abs(pb->next->x - x) < 5)
		pb->x = pb->next->x;
	else if(abs(pb->prev->x - x) < 5)
		pb->x = pb->prev->x;
	else
		pb->x = x;

	if(pb->next->next && pb->y == pb->next->y && pb->x != pb->next->x) {
		pb->y = y;
		pb->next->y = y;
	}
	else if(pb->prev->prev && pb->y == pb->prev->y && pb->x != pb->prev->x) {
		pb->y = y;
		pb->prev->y = y;
	}
	else if(abs(pb->next->y - y) < 5)
		pb->y = pb->next->y;
	else if(abs(pb->prev->y - y) < 5)
		pb->y = pb->prev->y;
	else
		pb->y = y;
}

void ElementPoint::move(gdouble dx, gdouble dy) {
	PointPos *pb = pos;
	if(point && (point->parent->flag & ELEMENT_FLG_IS_SELECT)) {
		if(pb->next && isPrimary()) movePoints(pb->next, dx, dy);
	}
	else {
		if(pb->next && pb->next->next) {
			if(pb->next->x == pb->x) {
				pb->next->x += dx;
				if(pb->y + dy - pb->next->y < 3)
					movePos(pb->next, pb->next->x, pb->next->y + dy);
			}
			else if(pb->next->y == pb->y) {
				pb->next->y += dy;
				if(pb->next->x - (pb->x + dx) < 3)
					movePos(pb->next, pb->next->x + dx, pb->next->y);
			}
		}

		if(pb->prev && pb->prev->prev) {
			if(pb->prev->y == pb->y) {
				pb->prev->y += dy;
				if(abs(pb->x + dx - pb->prev->x) < 3)
					movePos(pb->prev, pb->prev->x + dx, pb->prev->y);
			}
			else if(pb->prev->x == pb->x) {
				pb->prev->x += dx;
				if(abs(pb->prev->y - (pb->y + dy)) < 3 )
					movePos(pb->prev, pb->prev->x, pb->prev->y + dy);
			}
		}
	}
	pb->y += dy;
	pb->x += dx;
}

void ElementPoint::removeLinePoint(PointPos *lp) {
	Gdk::Rectangle old = drawRect();

	lp->prev->next = lp->next;
	lp->next->prev = lp->prev;
	delete lp;

	Gdk::Rectangle r = drawRect();
	r.join(old);
	parent->parent->on_redraw_rect.run(&r);
}

void ElementPoint::moveLinePoint(PointPos *lp, gdouble x, gdouble y) {
	Gdk::Rectangle old = drawRect();
	if(lp->next->next && lp->x == lp->next->x && lp->y != lp->next->y) {
		lp->x = x;
		lp->next->x = x;
	}
	else if(lp->prev->prev && lp->x == lp->prev->x && lp->y != lp->prev->y) {
		lp->x = x;
		lp->prev->x = x;
	}
	else if(abs(lp->next->x - x) < 5)
		lp->x = lp->next->x;
	else if(abs(lp->prev->x - x) < 5)
		lp->x = lp->prev->x;
	else lp->x = x;

	if(lp->next->next && lp->y == lp->next->y && lp->x != lp->next->x) {
		lp->y = y;
		lp->next->y = y;
	}
	else if(lp->prev->prev && lp->y == lp->prev->y && lp->x != lp->prev->x) {
		lp->y = y;
		lp->prev->y = y;
	}
	else if(abs(lp->next->y - y) < 5)
		lp->y = lp->next->y;
	else if(abs(lp->prev->y - y) < 5)
		lp->y = lp->prev->y;
	else lp->y = y;

	Gdk::Rectangle r = drawRect();
	r.join(old);
	parent->parent->on_redraw_rect.run(&r);
}

PointPos *ElementPoint::addLinePoint(PointPos *lp, gdouble x, gdouble y) {
	PointPos *np = new PointPos();

	if(lp) {
		np->next = lp->next;
		np->prev = lp;
		lp->next = np;
		if(np->next) np->next->prev = np;
	}
	else {
		np->next = NULL;
		np->prev = NULL;
	}

	np->x = x;
	np->y = y;

	return np;
}

Gdk::Rectangle ElementPoint::drawRect() {
	PointPos *p = (type == pt_event || type == pt_data || !point) ? pos : point->pos;
	gdouble x1 = p->x;
	gdouble y1 = p->y;
	gdouble x2 = p->x;
	gdouble y2 = p->y;

	while((p = p->next)) {
		if(p->x < x1) x1 = p->x;
		if(p->y < y1) y1 = p->y;
		if(p->x > x2) x2 = p->x;
		if(p->y > y2) y2 = p->y;
	}
	return Gdk::Rectangle(x1 - POINT_OFF, y1 - POINT_OFF, x2 - x1 + POINT_SPACE, y2 - y1 + POINT_SPACE);
}

void ElementPoint::invalidate() {
	Gdk::Rectangle r = drawRect();
	parent->parent->on_redraw_rect.run(&r);
}

ustring ElementPoint::serializePath() {
	ustring path;
	PointPos *p = pos->next;
	while(p->next) {
		path += ustring("(") + int_to_str((int)p->x) + "," + int_to_str((int)p->y) + ")";
		p = p->next;
	}
	return path;
}

ElementPoint *ElementPoint::getRealPointWithPath() {
	ElementPoint *p, *root = this, *Result = this;
	do {
		p = Result;
		Result = p->parent->getRealPoint(p);
	} while( p != Result && Result && root != Result );

	if(root == Result)
		Result = p;
	return Result;
}

void ElementPoint::on_event(TData *data) {
	if(point) {
		Element *e = point->parent;
		if(e->isCore()) {
			dynamic_cast<ElementCore*>(e)->do_work(point, data);
		}
	}
}

void ElementPoint::get_data(TData *data) {
	if(point) {
		Element *e = point->parent;
		if(e->isCore()) {
			dynamic_cast<ElementCore*>(e)->read_var(point, data);
		}
	}
}
