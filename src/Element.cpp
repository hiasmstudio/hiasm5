/*
 * Element.cpp
 *
 *  Created on: 02.05.2010
 *      Author: dilma
 */

#include "Element.h"
#include "StringList.h"

SysColors sysColors;
std::vector<double> dash(1, 4);

Gdk::RGBA lightColor;
Gdk::RGBA darkColor;
Gdk::RGBA hintColor;

TypePixbuf (*stockRender)(const ustring &name) = NULL;

ElementPoint *ElementPoints::getByName(const ustring &name) {
	ustring sp = name.lowercase();
	for(iterator p = begin(); p != end(); p++)
		if((*p)->name.lowercase() == sp)
			return *p;
	return NULL;
}

int ElementPoints::indexOf(ElementPoint *point) {
	int i = 0;
	for(iterator p = begin(); p != end(); p++)
		if(*p == point)
			return i;
		else
			i++;
	return -1;
}

int ElementPoints::indexOfType(ElementPoint *point) {
	int i = 0;
	for(iterator p = begin(); p != end(); p++)
		if((*p)->type == point->type) {
			i++;
			if(*p == point)
				break;
		}
	return i - 1;
}

//----------------------------------------------------------------------------------------

ElementProperty *ElementProperties::getByName(const ustring &name) {
	ustring prop = name.lowercase();
	for(iterator p = begin(); p != end(); p++)
		if((*p)->name.lowercase() == prop)
			return *p;
	return NULL;
}

ElementProperty *ElementProperties::getByPointName(const ustring &name) {
	if(name.length() < 3) return NULL;

	ustring tmp(name.lowercase(), 2, name.length()-2);
	for(iterator p = begin(); p != end(); p++)
		if(((*p)->flag & VF_MAKEPOINT) && (*p)->name.lowercase() == tmp)
			return *p;
	return NULL;
}

void ElementProperties::change() {
	for(iterator p = begin(); p != end(); p++)
		(*p)->parent->on_change_property(*p);
}

void ElementProperties::free() {
	for(iterator p = begin(); p != end(); p++)
		delete (*p);
	clear();
}

void ElementProperties::setParent(Element *parent) {
	for(iterator p = begin(); p != end(); p++)
		(*p)->parent = parent;
}

//----------------------------------------------------------------------------------------

void LinkHints::restore(int x, int y, int width, int height, ElementProperty *prop) {
	LinkHint *h = new LinkHint(prop->parent);
	h->x = x;
	h->y = y;
	h->width = width;
	h->height = height;
	h->prop = prop;
	push_back(h);
	h->updateText();
}

void LinkHints::draw(DrawContext dc) {
	for(iterator h = begin(); h != end(); h++)
		(*h)->draw(dc);
}

void LinkHints::free() {
	for(iterator h = begin(); h != end(); h++)
		delete *h;
	clear();
}

Gdk::Rectangle LinkHints::drawRect() {
	Gdk::Rectangle r;
	for(iterator h = begin(); h != end(); h++)
		r.join((*h)->drawRect());
	return r;
}

void LinkHints::saveToText(ustring &text, const ustring offset) {
	for(iterator h = begin(); h != end(); h++)
		if((*h)->prop) {
			ustring pref = (*h)->parent->sysProps.getByName((*h)->prop->name) ? "@" : "";
			text += offset +  " AddHint(" + int_to_str((*h)->x) + "," +
											int_to_str((*h)->y) + "," +
											int_to_str((*h)->width) + "," +
											int_to_str((*h)->height) + "," + pref + (*h)->prop->name + ")"LINE_END;
		}
}

bool LinkHints::getObjectAtPos(gdouble x, gdouble y, ObjectType *obj) {
	for(iterator h = begin(); h != end(); h++) {
		Gdk::Rectangle r = (*h)->drawRect();
		if(x >= r.get_x() && y >= r.get_y() && x <= r.get_x() + r.get_width() && y <= r.get_y() + r.get_height()) {
			*obj = ObjectType(*h);
			return true;
		}
	}
	return false;
}

void LinkHints::change(ElementProperty *prop) {
	bool ch = false;
	Gdk::Rectangle r = prop->parent->drawRect();
	for(iterator h = begin(); h != end(); h++)
		if((*h)->prop == prop) {
			(*h)->updateText();
			ch = true;
		}
	if(ch) {
		r.join(prop->parent->drawRect());
		prop->parent->parent->on_redraw_rect.run(&r);
	}
}

//----------------------------------------------------------------------------------------

void roundRect(DrawContext dc, gdouble x, gdouble y, int w, int h) {
	static const int r = 5;
	static const double degrees = M_PI / 180.0;

	dc->begin_new_sub_path();
	dc->arc(x + w - r, y + r, r, -90 * degrees, 0 * degrees);
	dc->arc(x + w - r, y + h - r, r, 0 * degrees, 90 * degrees);
	dc->arc(x + r, y + h - r, r, 90 * degrees, 180 * degrees);
	dc->arc(x + r, y + r, r, 180 * degrees, 270 * degrees);
	dc->close_path();
}

//----------------------------------------------------------------------------------------

Gdk::Rectangle LinkHint::drawRect() {
	return Gdk::Rectangle(parent->x + x - 3, parent->y + y - 3, width + 6, height + 6);
}

void LinkHint::draw(DrawContext dc) {
	if(!textLayout) {
		textLayout = Pango::Layout::create(dc);
		textLayout->set_text(text);
		textLayout->set_wrap(Pango::WRAP_WORD);
		//textLayout->set_ellipsize(Pango::ELLIPSIZE_END);
		Pango::FontDescription f("arial 8");
		textLayout->set_font_description(f);
		textLayout->set_indent(10);
		textLayout->set_alignment(Pango::ALIGN_LEFT);
		textLayout->set_width(width*Pango::SCALE);
		textLayout->set_height(-1);
		textLayout->get_pixel_size(width, height);
	}

	Gdk::Rectangle r = drawRect();

	Gdk::Cairo::set_source_rgba(dc, darkColor);
	int px = parent->x + parent->width / 2;
	int py = parent->y + parent->height / 2;
	int mx = parent->x + x + width / 2;
	int my = parent->y + y + height / 2;

	dc->set_dash(dash, 1);
	dc->move_to(px, py);
	if(!(x + width < 0 || x > parent->width))
		dc->line_to((px + mx) / 2, my);
	else if(x < 0)
		dc->line_to((px + parent->x + x + width) / 2, my);
	else
		dc->line_to((px + parent->x + x) / 2, my);
	dc->line_to(mx, my);
	dc->stroke();
	dc->unset_dash();

	//Cairo::Antialias old_a = dc->get_antialias();
	//if(old_a == Cairo::ANTIALIAS_NONE)
	//	dc->set_antialias(Cairo::ANTIALIAS_DEFAULT);

	Gdk::Cairo::set_source_rgba(dc, sysColors.btnLight);
	dc->rectangle(r.get_x(), r.get_y(), r.get_width(), r.get_height());
	//roundRect(dc, r.get_x(), r.get_y(), r.get_width(), r.get_height());
	dc->fill();

	Gdk::Cairo::set_source_rgba(dc, sysColors.btnDark);
	dc->rectangle(r.get_x(), r.get_y(), r.get_width(), r.get_height());
	//roundRect(dc, r.get_x(), r.get_y(), r.get_width(), r.get_height());
	dc->stroke();

	//dc->set_antialias(old_a);

	//Gdk::Cairo::set_source_color(dc, darkColor);
	//dc->rectangle(r.get_x(), r.get_y(), r.get_width() - 1, r.get_height() - 1);
	//roundRect(dc, r.get_x(), r.get_y(), r.get_width(), r.get_height());
	//dc->stroke();

	//dc->set_font_size(11);
	//dc->select_font_face("arial", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
	//dc->move_to(r.get_x() + 4, r.get_y() + 12);
	//dc->show_text(text);

	//dc->rectangle(r.get_x(), r.get_y(), r.get_width(), r.get_height());
	//dc->clip();

	dc->set_source_rgb(0, 0, 0);
	dc->move_to(r.get_x() + 3, r.get_y() + 3);
	textLayout->show_in_cairo_context(dc);

	//dc->reset_clip();
}

void LinkHint::updateText() {
	if(!prop) {
		text = "(empty)";
		return;
	}

	switch(prop->type) {
		case data_int:
			text = int_to_str(prop->readInt());
			break;
		case data_str:
		case data_list:
		case data_stock:
		case data_element:
			text = prop->readStr();
			break;
		case data_combo:
		case data_comboEx: {
			StringList list;
			list.split(prop->list.c_str(), ',');
			text = list[prop->readInt()];
			break;
		}
		case data_data: {
			TData *data = prop->readData();
			if(data->type == data_null)
				text = "NULL";
			else
				text = data->toStr();
			break;
		}
		default:
			text = "(no support)";
	}
	if(textLayout) {
		textLayout->set_text(text);
		textLayout->get_pixel_size(width, height);
	}
}

void LinkHint::setWidth(int value) {
	width = value;
	if(textLayout) {
		textLayout->set_width(width*Pango::SCALE);
		textLayout->get_pixel_size(width, height);
	}
}

//----------------------------------------------------------------------------------------

Element::Element(PackElement *pe, SDK *sdk, gdouble x, gdouble y):
		on_property_change(this, CBT_ELEMENT_PROP_CHANGE)
{
	TRACE_PROC
	this->x = x;
	this->y = y;
	parent = sdk;
	minWidth = 32;
	minHeight = 32;
	width = minWidth;
	height = minHeight;
	tpl = pe;
	this->sdk = NULL;
	flag = 0;
	cIndex = 0;
	elinkList = NULL;
	userData = NULL;

	buildFromTemplate();

	sysProps.push_back(new ElementProperty(this, "Hint", "Element description", data_list));
	ElementProperty *c = new ElementProperty(this, "Color", "Element color", data_color);
	sysProps.push_back(c);
	color = (Gdk::RGBA*)c->value;
	*color = sysColors.btnFace;
	colorLight = sysColors.btnLight;
	colorDark = sysColors.btnDark;
	c->setAsDefault();
}

Element::~Element() {
	// points
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		delete (*p);
	points.clear();

	// elinks
	if(elinkList) {
		if(isMainELink()) {
			props.setParent(*(++elinkList->begin()));
		}
		elinkList->remove(this);
		if(elinkList->size() == 1) {
			(*elinkList->begin())->elinkList = NULL;
			delete elinkList;
		}
		elinkList = NULL;
	}
	else {
		// properties
		props.free();
	}
	sysProps.free();

	hints.free();
}

void Element::initView() {
	viewProp = tpl->conf->view.empty() ? NULL : props.getByName(tpl->conf->view);
	if(viewProp) {
		switch(viewProp->type) {
			case data_int:	viewText = int_to_str(viewProp->readInt()); break;
			case data_str:	viewText = viewProp->readStr(); break;
			case data_real:	viewText = double_to_str(viewProp->readInt()); break;
			case data_combo:
			case data_comboEx: {
				StringList l;
				l.split(viewProp->list.c_str(), ',');
				viewText = l[viewProp->readInt()]; break;
				break;
			}
			default:
				viewText.clear();
		}
	}
}

void Element::draw(DrawContext dc, double zoom) {
	dc->set_line_width(1);

//	GTimer *t = g_timer_new();
	drawBody(dc, zoom);
//	gulong m;
//	g_timer_elapsed(t, &m);
//	DEBUG_MSG("Body: " << m)
	if(zoom > 0.6)
		drawIcon(dc);
//	g_timer_elapsed(t, &m);
//	DEBUG_MSG("Icon: " << m)
	drawPoints(dc);
//	g_timer_elapsed(t, &m);
//	DEBUG_MSG("Points: " << m)
//	g_timer_destroy(t);
}

void Element::drawBody(DrawContext dc, double zoom) {
	hints.draw(dc);

	if(flag & ELEMENT_FLG_IS_SELECT)
		Gdk::Cairo::set_source_rgba(dc, colorDark);
	else
		Gdk::Cairo::set_source_rgba(dc, *color);

/*
	//----------
	Cairo::RefPtr<Cairo::LinearGradient> ptr = Cairo::LinearGradient::create(x, y, x, y + height);
	dc->set_source(ptr);
	ptr->add_color_stop_rgb(0, 1.0, 1.0, 1.0);
	if(flag & ELEMENT_FLG_IS_SELECT)
		ptr->add_color_stop_rgb(1, colorDark.get_red_p(), colorDark.get_green_p(), colorDark.get_blue_p());
	else
		ptr->add_color_stop_rgb(1, color.get_red_p(), color.get_green_p(), color.get_blue_p());
	//----------
*/
	if(zoom == 1)
		dc->rectangle(x, y, width, height);
	else
		roundRect(dc, x, y, width, height);
	dc->fill();

	if(flag & ELEMENT_FLG_IS_FREEZE)
		dc->set_source_rgb(0.0, 0.0, 1.0);
	else
		Gdk::Cairo::set_source_rgba(dc, colorDark);
	if(zoom == 1) {
		//dc->rectangle(x, y, width, height);

		dc->move_to(x + width, y);
		dc->line_to(x + width, y + height);
		dc->line_to(x, y + height);
		dc->stroke();

		Gdk::Cairo::set_source_rgba(dc, colorLight);
		dc->move_to(x, y + height);
		dc->line_to(x, y);
		dc->line_to(x + width, y);
		dc->stroke();
	}
	else
		roundRect(dc, x, y, width, height);
	dc->stroke();

	if(isELink()) {
		if(isMainELink())
			Gdk::Cairo::set_source_rgba(dc, *color);
		else
			Gdk::Cairo::set_source_rgba(dc, colorLight);
		dc->rectangle(x + width - POINT_OFF, y + height - POINT_OFF, POINT_OFF*2, POINT_OFF*2);
		dc->fill();
		Gdk::Cairo::set_source_rgba(dc, colorDark);
		dc->rectangle(x + width - POINT_OFF, y + height - POINT_OFF, POINT_OFF*2, POINT_OFF*2);
		dc->stroke();
	}
}

void Element::drawIcon(DrawContext dc) {
	if(viewProp) {
		dc->set_font_size(tpl->conf->viewFontHeight);
		//if(!width) {
			Cairo::TextExtents te;
			dc->get_text_extents(viewText, te);
		//}
		dc->set_source_rgb(0, 0, 0);
		dc->move_to(x - 2 + width / 2 - te.width / 2, y + 10 + height / 2 - te.height / 2);
		dc->show_text(viewText);
	}
	else {
		dc->save();
		dc->translate(x + width / 2 - ICON_SIZE / 2, y + height / 2 - ICON_SIZE / 2);
		Gdk::Cairo::set_source_pixbuf(dc, tpl->image, 0.0, 0.0);
		dc->rectangle (0, 0, ICON_SIZE, ICON_SIZE);
		dc->clip();
		dc->paint();
		dc->restore();
	}
}

void Element::drawPoints(DrawContext dc) {
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++) {
		if((*p)->isSelect())
			dc->set_source_rgb(0.0, 0.6, 0.0);
		else
			dc->set_source_rgb(0.0, 1.0, 0.0);
		dc->arc((*p)->pos->x, (*p)->pos->y, POINT_OFF, 0, 2*3.1415);
		//dc->rectangle((*p)->pos->x, (*p)->pos->y, POINT_SPACE, POINT_SPACE);
		dc->fill();
		dc->set_source_rgb(0.4, 0.4, 0.4);
		dc->arc((*p)->pos->x, (*p)->pos->y, POINT_OFF, 0, 2*3.1415);
		//dc->rectangle((*p)->pos->x, (*p)->pos->y, POINT_SPACE, POINT_SPACE);
		dc->stroke();
	}
}

void Element::drawLinks(DrawContext dc) {
	PointPos *pb;

	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		if((*p)->pos->next) {
			if((*p)->type == pt_event)
				dc->set_source_rgb(0.0, 0.0, 0.5);
			else
				dc->set_source_rgb(1.0, 0.0, 0.0);

			if((*p)->flag & POINT_FLG_IS_SELECT || (*p)->point->flag & POINT_FLG_IS_SELECT)
				dc->set_line_width(3);
			else
				dc->set_line_width(1);

			//---------bezie---------
			/**/
			if(!(*p)->pos->next->next) {
				dc->move_to((*p)->pos->x, (*p)->pos->y);
				gdouble x = (*p)->pos->next->x, y = (*p)->pos->next->y;
				if((*p)->type == pt_event)
					dc->curve_to((*p)->pos->x + (x - (*p)->pos->x) / 2, (*p)->pos->y,
								 (*p)->pos->x + (x - (*p)->pos->x) / 2, y, x, y);
				else
					dc->curve_to((*p)->pos->x, (*p)->pos->y + (y - (*p)->pos->y) / 2,
								 x, (*p)->pos->y + (y - (*p)->pos->y) / 2, x, y);

				dc->stroke();
				continue;
			}
			//*/
			//------------------

			dc->move_to((*p)->pos->x, (*p)->pos->y);
			pb = (*p)->pos;
			do {
				dc->line_to(pb->next->x, pb->next->y);
				pb = pb->next;
			} while(pb->next);
			dc->stroke();
		}
}

void Element::changeColor() {
	colorLight.set_rgba(min(1.0, color->get_red() + ECL_LIGHT),
						 min(1.0, color->get_green() + ECL_LIGHT),
						 min(1.0, color->get_blue() + ECL_LIGHT), 1.0);
	colorDark.set_rgba(max(0.0, color->get_red() + ECL_DARK),
						max(0.0, color->get_green() + ECL_DARK),
						max(0.0, color->get_blue() + ECL_DARK), 1.0);
}

void Element::clearLinks() {
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		(*p)->clear();
}

ElementPoint *Element::addPoint(const ustring &name, const ustring &info, int type) {
	ElementPoint *p = new ElementPoint(this, name, info, type);
	points.push_back(p);
	return p;
}

void Element::removePoint(ElementPoint *point) {
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		if(*p == point) {
			points.erase(p);
			break;
		}
	delete point;
}

void Element::rePosPoints() {
	reSize();
	rePosPointsWOsize();
}

void Element::rePosPointsWOsize() {
	int count[4] = {0};
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++) {
		int *c = &count[(*p)->type-1];
		(*p)->pos->x = x;
		(*p)->pos->y = y;
		switch((*p)->type) {
			case pt_event:
				(*p)->pos->x += width;
			case pt_work:
				(*p)->pos->y += POINT_SPACE + *c*POINT_SPACE - 1;
				break;
			case pt_var:
				(*p)->pos->y += height;
			case pt_data:
				(*p)->pos->x += POINT_SPACE + *c*POINT_SPACE - 1;
			break;
		}
		(*c)++;
	}
}

void Element::reSize() {
	int count[4] = {0};
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		count[(*p)->type-1] ++;

	width = max(minWidth, max(count[2], count[3])*POINT_SPACE + POINT_OFF);
	height = max(minHeight, max(count[0], count[1])*POINT_SPACE + POINT_OFF);
}

void Element::buildFromTemplate() {
	// points ----------------------------------------------------
	for(ConfMethods::iterator mt = tpl->conf->mtd.begin(); mt != tpl->conf->mtd.end(); mt++) {
		ElementPoint *p = addPoint((*mt)->name, (*mt)->info, (*mt)->mType);
		p->dataType = (*mt)->dataType;
	}
	rePosPoints();

	// propertys -------------------------------------------------
	for(ConfPropertys::iterator pt = tpl->conf->prop.begin(); pt != tpl->conf->prop.end(); pt++) {
		ElementProperty *p = new ElementProperty(this, *pt);
		props.push_back(p);
	}
}

void Element::move(gdouble dx, gdouble dy) {
	if(isFreeze()) return;

	x += dx;
	y += dy;

	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		(*p)->move(dx, dy);
}

void Element::injectElementAtLine(ElementPoint *pl1, ElementPoint *pl2, ElementPoint *pl3, PointPos *p1) {
	ElementPoint *pl4 = pl1->point;
	PointPos *p2 = p1->next;

	PointPos *op;
	if(pl1->pos->next == pl4->pos)
		op = NULL;
	else
		op = pl1->pos->next;

	pl1->connect(pl2);

	if(op) {
		pl1->pos->next = op;
		p1->next = pl2->pos;
		pl2->pos->prev = p1;
	}

	if(pl4->pos->prev == pl1->pos)
		op = NULL;
	else
		op = pl4->pos->prev;

	pl4->connect(pl3);

	if(op) {
		pl4->pos->prev = op;
		p2->prev = pl3->pos;
		pl3->pos->next = p2;
		/*
		if EMouse.SelectState = selPointLine then
		 if P1.X = P1.Prev.X then
		   SDK.Last.Move(P1.Prev.X - pl2.Pos.X, 0, 0)
		 else if P1.Y = P1.Prev.Y then
		   SDK.Last.Move(0, P1.Prev.Y - pl2.Pos.Y, 0);
		*/
	}
}

void Element::insertInLine(ElementPoint *point, PointPos *pos) {
	TRACE_PROC
	if(point->type == pt_event) {
		ElementPoint *work = getFirstFreePoint(pt_work);
		ElementPoint *event = getFirstFreePoint(pt_event);
		if(!work || !event) {
			ElementPoint *sp = point->point;
			point->clear();
			if(work)  point->connect(work)->createPath();
			if(event) event->connect(sp)->createPath();
		}
		else {
			PointPos *p2 = pos->next;
			injectElementAtLine(point, work, event, pos);
			if(pos->y == p2->y)
				move(0, pos->y - event->pos->y);
		}
	}
	invalidate();
}

void Element::connectToPoint(ElementPoint *point) {
	ElementPoint *fp = getFirstFreePoint(point->getPair());
	if(fp && point->canConnect(fp))
		point->connect(fp)->createPath();
}

void Element::remove() {
	// do nothing
}

void Element::removeFromLine(bool we, bool vd) {
	for(int i = 0; i <2; i++)
		if( (i == 0 && we) || (i == 1 && vd)) {
			int k = i*2 + 1;
			ElementPoint *p1 = NULL, *p2 = NULL;

			for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
				if((*p)->point) {
					if((*p)->type == k && p1 == NULL)
						p1 = *p;
					else if((*p)->type == (k+1) && p2 == NULL)
						p2 = *p;
				}

			if(p1 && p2) {
				PointPos *pp = ElementPoint::addLinePoint(NULL, 0, 0);
				PointPos *fp = pp;
				PointPos *pt = p1->point->pos;
				while(pt->next->next) {
					fp = ElementPoint::addLinePoint(fp, pt->next->x, pt->next->y);
					pt = pt->next;
				}
				pt = p2->pos;
				while(pt->next->next) {
					fp = ElementPoint::addLinePoint(fp, pt->next->x, pt->next->y);
					pt = pt->next;
				}
				ElementPoint *np1 = p1->point;
				ElementPoint *np2 = p2->point;
				p1->clear();
				p2->clear();
				np1->connect(np2);
				if(pp->next) {
					np1->pos->next = pp->next;
					pp->next->prev = np1->pos;
					fp->next = np2->pos;
					np2->pos->prev = fp;
				}
				delete pp;
			}
		}
}

ustring Element::getHint() {
	ElementProperty *hint = sysProps.getByName("Hint");
	return hint ? hint->readStr() : ustring();
}

bool Test(gdouble v1, gdouble v2, gdouble tp) {
	if( v2 < v1 )
		return( (v2-4 < tp)&&(tp < v1+4) );
	else
		return( (v1-4 < tp)&&(tp < v2+4) );
}

int isLine(gdouble _x, gdouble _y, gdouble lx1, gdouble ly1, gdouble lx2, gdouble ly2) {
	int p,k,C;

	if( Test(lx1,lx2,_x) && Test(ly1,ly2,_y) ) {
		p = ly2 - ly1;
		k = lx2 - lx1;
		C = ly1*k - lx1*p;
		return abs(p*_x - k*_y + C);
	}
	else return 500;
}

bool Element::findLine(gdouble x, gdouble y, ObjectType *obj) {
	PointPos *pb;
	void *tmp;

	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		if((*p)->pos->next) {
			pb = (*p)->pos;
			do {
				tmp = pb;
				if(isLine(x, y, pb->x, pb->y, pb->next->x, pb->next->y) < 200) {
					*obj = ObjectType(pb, *p);
					obj->type = SDK_OBJ_LINE;
					return true;
				}
				pb = pb->next;
			} while(pb->next);
		}
	return false;
}

bool Element::getObjectAtPos(gdouble x, gdouble y, ObjectType *obj) {
	if(flag & ELEMENT_FLG_IS_NOMOUSE)
		return false;

	PointPos *pb;
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		if((*p)->pos->next) {
			pb = (*p)->pos->next;
			while(pb->next)
				if((abs(pb->x - x) < POINT_OFF+1) && ( abs(pb->y - y) < POINT_OFF+1)) {
					*obj = ObjectType(pb, *p);
					return true;
				}
				else pb = pb->next;
		}

	if(hints.getObjectAtPos(x, y, obj))
		return true;

	if( !(y < this->y-3 || y > this->y + height+3 || x < this->x-3 || x > this->x + width + 3) ) {
		for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
			if((abs((*p)->pos->x - x) <= POINT_OFF + 1) && (abs((*p)->pos->y - y) <= POINT_OFF + 1)) {
				*obj = ObjectType(*p);
				return true;
			}

		if(checkCollise(x, y)) {
			*obj = ObjectType(this);
			return true;
		}
	}
	return false;
}

bool Element::checkCollise(gdouble x, gdouble y) {
	return (y >= this->y && y <= this->y + height && x >= this->x && x <= this->x + width);
}

bool Element::checkColliseRect(gdouble x1, gdouble y1, gdouble x2, gdouble y2) {
	return !((x > x2)||(y > y2)||(x + width < x1)||(y + height < y1));
}

Gdk::Rectangle Element::drawRect() {
	Gdk::Rectangle r(x - POINT_OFF, y - POINT_OFF, width + POINT_SPACE, height + POINT_SPACE);
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		r.join((*p)->drawRect());

	if(hints.size())
		r.join(hints.drawRect());

	return r;
}

void Element::invalidate() {
	Gdk::Rectangle r = drawRect();
	parent->on_redraw_rect.run(&r);
}

ElementPoint *Element::getFirstFreePoint(int type) {
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		if((*p)->type == type && !(*p)->point)
			return *p;
	return NULL;
}

ElementPoint *Element::getLastFreePoint(int type) {
	for(ElementPoints::reverse_iterator p = points.rbegin(); p != points.rend(); p++)
		if((*p)->type == type)
			return (*p)->point ? NULL : *p;
	return NULL;
}

ElementPoint *Element::getLastPoint(int type) {
	for(ElementPoints::reverse_iterator p = points.rbegin(); p != points.rend(); p++)
		if((*p)->type == type)
			return *p;
	return NULL;
}

int Element::getPointsCount(int type) {
	int c = 0;
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++)
		if((*p)->type == type)
			c++;
	return c;
}

void Element::saveToText(ustring &text, const ustring offset, int flags) {
	text += offset + "Add(" + tpl->name + "," + int_to_str(id) + "," + int_to_str((int)x) + "," + int_to_str((int)y) + ")"LINE_END;
	text += offset + "{"LINE_END;
	if(isELink()) {
		if(isMainELink()) {
			if(elinkList->size() > 1)
				text += offset + "  @IsLib=true"LINE_END;
		}
		else
			text += offset + "  elink(" + int_to_str(getMainELink()->id) + ")"LINE_END;
	}
	saveSysParamsToText(text, offset, flags);
	saveParamsToText(text, offset, flags);
	savePointsToText(text, offset, flags);
	hints.saveToText(text, offset);
	text += offset + "}"LINE_END;
}

void Element::saveAsLink(ustring &text) {
	text += "Add(" + tpl->name + "," + int_to_str(parent->getMSDK()->genNextID()) + "," + int_to_str((int)x) + "," + int_to_str((int)y) + ")"LINE_END;
	text += "{"LINE_END;
	text += "  elink("+ int_to_str(isELink() ? getMainELink()->id : id) + ")"LINE_END;
	saveSysParamsToText(text, "  ", ELEMENT_SAVE_CHANGED);
	saveParamsToText(text, "  ", ELEMENT_SAVE_CHANGED);
	text += "}"LINE_END;

	parent->getMSDK()->sdkLib.add(this);
}

void Element::saveParamsToText(ustring &text, const ustring offset, int flags) {
	for(ElementProperties::iterator p = props.begin(); p != props.end(); p++)
		if(!(flags & ELEMENT_SAVE_CHANGED) || !(*p)->isDefault())
			text += offset + " " + (*p)->name + "=" + (*p)->serialize() + LINE_END;
}

void Element::saveSysParamsToText(ustring &text, const ustring offset, int flags) {
	for(ElementProperties::iterator p = sysProps.begin(); p != sysProps.end(); p++)
		if(!(flags & ELEMENT_SAVE_CHANGED) || !(*p)->isDefault())
			text += offset + " @" + (*p)->name + "=" + (*p)->serialize() + LINE_END;
}

void Element::savePointsToText(ustring &text, const ustring offset, int flags) {
	for(ElementPoints::iterator p = points.begin(); p != points.end(); p++) {
		if(props.getByPointName((*p)->name) || tpl->conf->def.getByName((*p)->name))
			text += offset + " Point(" + (*p)->name + ")"LINE_END;

		if((*p)->point && (*p)->isPrimary() && (!(flags&ELEMENT_SAVE_SELECTED) || (*p)->point->parent->isSelect()))
			text += offset + " link(" + (*p)->name + "," +
								int_to_str((*p)->point->parent->id) + ":" + (*p)->point->name + ",[" +
								(*p)->serializePath() + "])"LINE_END;
	}
}

void Element::on_change_property(ElementProperty *prop) {
	if(prop->value == color) {
		changeColor();
		invalidate();
	}
	else if(prop == viewProp) {
		initView();
		invalidate();
	}

	if(isMainELink()) {

	}

	hints.change(prop);

	on_property_change.run(prop);
}

Element *Element::getParentElement() {
	if(sdk && sdk->elements.size() > 1) {
		ElementsList::iterator it = sdk->elements.begin();
		it++;
		return (*it)->isParent() ? *it : NULL;
	}

	return NULL;
}

bool Element::checkInterfaceProvide(const ustring &name) {
	//TODO correct check...
	return (tpl->conf->interfaces + ",").find(name + ",") != (std::string::size_type)-1;
}

void Element::storePropPointers() {
	initView();
}

void Element::createMainELink() {
	if(elinkList) return;

	elinkList = new ElementsList();
	elinkList->push_back(this);
}

void Element::createELink(Element *owner) {
	owner->createMainELink();
	owner->elinkList->push_back(this);
	elinkList = owner->elinkList;
	props.free();
	props.assign(owner->props.begin(), owner->props.end());
	storePropPointers();
}
