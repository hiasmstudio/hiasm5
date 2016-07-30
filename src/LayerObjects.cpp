/*
 * LayerObjects.cpp
 *
 *  Created on: 18.05.2010
 *      Author: dilma
 */

#include "LayerObjects.h"

LayerObject::LayerObject(Widget *parent) {
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

gboolean lo_timer_proc(void *data) {
	((LayerObject*)data)->visible = true;
	((LayerObject*)data)->invalidate();
	return false;
}

void LayerObject::show(gdouble x, gdouble y, bool delay) {
	if(delay)
		timerID = gdk_threads_add_timeout(500, lo_timer_proc, this);
	else {
		visible = true;
		invalidate();
	}
}

void LayerObject::invalidate() {
	if(parent) {
		Gdk::Rectangle r(x - 2, y - 2, 400, 400);
		parent->get_window()->invalidate_rect(r, false);
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

void LO_PointHint::drawHintRect(DrawContext dc) {
	static const int r = 5;
	static const double degrees = M_PI / 180.0;

	dc->begin_new_sub_path();
	dc->arc(x + width - r, y + r, r, -90 * degrees, 0 * degrees);
	dc->arc(x + width - r, y + height - r, r, 0 * degrees, 90 * degrees);
	dc->arc(x + r, y + height - r, r, 90 * degrees, 180 * degrees);
	dc->arc(x + r, y + r, r, 180 * degrees, 270 * degrees);
	dc->close_path();
}

void LO_PointHint::draw(DrawContext dc) {

	if(!width) {
		Cairo::TextExtents te;
		for(std::list<ustring>::iterator l = info.begin(); l != info.end(); l++) {
			dc->get_text_extents(*l, te);
			if(te.width + 30 > width)
				width = te.width + 30;
			height += te.height + 1;
		}
		height += (info.size() == 1) ? 9 : 14;
	}

	Cairo::Antialias a = dc->get_antialias();
	dc->set_antialias(Cairo::ANTIALIAS_DEFAULT);

	dc->set_line_width(0.2);
/*
	dc->set_source_rgba(0.0, 0.0, 0.0, 0.2);
	drawHintRect(dc);
	dc->fill();
*/
	/*
	if(parent) {
		Glib::RefPtr<Style> style = parent->get_style();
		style->paint_shadow_gap(parent->get_window(),
								Gtk::STATE_NORMAL,
								Gtk::SHADOW_IN,
								Gdk::Rectangle(x, y, width, height),
								*parent, ustring(), x, y, width, height);
	}
	*/

	dc->set_source_rgba(1.0, 1.0, 1.0, 0.8);
	drawHintRect(dc);
	dc->fill();

	dc->set_source_rgb(0.0, 0.0, 0.0);
	drawHintRect(dc);
	dc->stroke();
	/*
	for(int i = 0; i < 4; i++) {
		x--;
		y--;
		width += 2;
		height += 2;
		dc->set_source_rgba(0, 0, 0, 0.2 - 0.04*i);
		drawHintRect(dc);
		dc->stroke();
	}
	x += 3;
	y += 3;
	width -= 6;
	height -= 6;
	*/

	dc->save();
	dc->translate(x + 1, y + 1);
	Gdk::Cairo::set_source_pixbuf(dc, typeIcon, 0.0, 0.0);
	dc->rectangle (0, 0, ICON_SIZE, ICON_SIZE);
	dc->clip();
	dc->paint();
	dc->restore();

	dc->set_source_rgb(0, 0, 0);
	gdouble _y = y + 12;
	gdouble lh = (height - 14) / info.size();

	dc->set_font_size(11);
	for(std::list<ustring>::iterator l = info.begin(); l != info.end(); l++) {
		if(l == info.begin()) {
			dc->move_to(x + 20, _y);
			dc->select_font_face("arial", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_BOLD);
		}
		else {
			dc->move_to(x + 4, _y + 8);
			dc->select_font_face("arial", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
		}
		dc->show_text(*l);
		_y += lh;
	}

	dc->set_antialias(a);
	dc->set_line_width(1);

	dc->set_source_rgb(0.5, 0.5, 0.5);
	if(info.size() > 1) {
		dc->move_to(x + 1, y + lh + 8);
		dc->line_to(x + width - 1, y + lh + 8);
		dc->stroke();
	}
}

//------------------------------------------------------------------------------------------------

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

void LO_ElementHint::drawHintRect(DrawContext dc) {
	dc->rectangle(x, y, width, height);
	return;
	static const int r = 5;
	static const double degrees = M_PI / 180.0;

	dc->begin_new_sub_path();
	dc->arc(x + width - r, y + r, r, -90 * degrees, 0 * degrees);
	dc->arc(x + width - r, y + height - r, r, 0 * degrees, 90 * degrees);
	dc->arc(x + r, y + height - r, r, 90 * degrees, 180 * degrees);
	dc->arc(x + r, y + r, r, 180 * degrees, 270 * degrees);
	dc->close_path();
}

void LO_ElementHint::draw(DrawContext dc) {

	dc->select_font_face("arial", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_BOLD);

	if(!width) {
		height = 6;
		Cairo::TextExtents te;
		for(std::list<ustring>::iterator l = info.begin(); l != info.end(); l++) {
			dc->get_text_extents(*l, te);
			if(te.width + 12 > width)
				width = te.width + 12;
			height += (*l == "-") ? 4 : (te.height + 3);
		}
	}

	Cairo::Antialias a = dc->get_antialias();
//	dc->set_antialias(Cairo::ANTIALIAS_DEFAULT);
//	dc->set_line_width(0.2);
	dc->set_line_width(1);
	dc->set_antialias(Cairo::ANTIALIAS_NONE);

	dc->set_source_rgb(1.0, 1.0, 1.0);
	drawHintRect(dc);
	dc->fill();

	dc->set_source_rgb(0.7, 0.7, 0.7);
	drawHintRect(dc);
	dc->stroke();

	dc->set_source_rgb(0, 0, 0);
	gdouble _y = y;

	dc->set_line_width(1);
	dc->set_antialias(Cairo::ANTIALIAS_NONE);
	dc->set_source_rgb(0, 0, 0);

	dc->set_font_size(11);
	for(std::list<ustring>::iterator l = info.begin(); l != info.end(); l++) {
		if(l == info.begin()) {
			dc->move_to(x + 4, _y + 12);
			dc->select_font_face("arial", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_BOLD);
		}
		else if(*l == "-") {
			dc->move_to(x + 1, _y + 3);
			dc->line_to(x + width - 1, _y + 3);
			dc->set_source_rgb(0.7, 0.7, 0.7);
			dc->stroke();
			_y += 4;
			continue;
		}
		else {
			dc->move_to(x + 4, _y + 12);
			dc->select_font_face("arial", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);
		}
		dc->set_source_rgb(0, 0, 0);
		dc->show_text(*l);
		Cairo::TextExtents te;
		dc->get_text_extents(*l, te);
		_y += te.height + 3;
	}

	dc->set_antialias(a);
}

void LO_ElementHint::show(gdouble x, gdouble y, bool delay) {
	this->x = x + 16;
	this->y = y;
	LayerObject::show(x, y, delay);
}
