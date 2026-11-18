/*
 * FormEditor.cpp
 *
 *  Created on: 19.05.2010
 *      Author: dilma
 */

#include "FormEditor.h"
#include "Elements.h"
#include "SettingsManager.h"
#include <glibmm.h>
#include <cmath>

//-------------------------------------------------------------------------------------------------------------------


WidgetEditorEx *WidgetEditorEx::activeInstance = NULL;

WidgetEditorEx::WidgetEditorEx(SDK *sdk, SDK_Editor *sde) {
	activeInstance = this;
	this->sde = sde;
	widget_set_events(*this, Gdk::BUTTON_PRESS_MASK|Gdk::POINTER_MOTION_MASK|Gdk::BUTTON_RELEASE_MASK);
	set_above_child(true);
	this->sdk = sdk;
	sdk->on_remove_element += this;
	sdk->on_add_element += this;
	sdk->selMan->on_selection_change += this;
	state = WE_MS_NONE;
	start_x = start_y = cur_x = cur_y = 0;
	selSizer = -1;

	ElementTplWinControl *main = NULL;
	for(ElementsList::iterator e = sdk->elements.begin(); e != sdk->elements.end(); e++)
		if((*e)->isParent() && (*e)->isCore()) {
			main = dynamic_cast<ElementTplWinControl*>(*e);
			break;
		}

	Element *parent = NULL;
	asParent = false;
	parentForm = NULL;
	formSurface = NULL;
	formFrame = NULL;
	panelField = NULL;
	if(main) {
		parentForm = main;
		if(main->tpl->conf->editClassName == "Form") {
			parent = main;
#ifdef HIASM_GTK4
			if(settingsMan && settingsMan->readInt(IDS_FORM_EDITOR) == 1) {
				// "Plain field": the form is a free canvas without a frame, as the designer was before.
				Fixed *surface = new Fixed();
				surface->set_hexpand(true);
				surface->set_vexpand(true);
				main->ctl = surface;
			}
			else {
			// The form is drawn like the runtime window: a frame (title from Caption, border from
			// BorderStyle) around a client area of Width x Height, where the controls are placed.
			formSurface = new Fixed();
			formSurface->set_hexpand(true);
			formSurface->set_vexpand(true);
			formSurface->add_css_class("hiasm-form-surface");
			formCss = Gtk::CssProvider::create();
			buildFormFrame();
			formSurface->get_style_context()->add_provider(formCss, G_MAXUINT);
			main->ctl = formSurface;
			main->on_property_change += this;
			applyFormStyle(main);
			}
#else
			main->ctl = this;
			asParent = true;
#endif
			//ctrls.insert(std::pair<Widget*, Element*>(this, main));
		}
		else if(main->tpl->conf->editClassName == "Panel") {
			// The scheme of a panel (ChildPanel): its field is the panel's size in the parent form, and the
			// controls of the scheme are placed on it. The frame of the panel carries its Caption.
			main->prepareForRun(&parent);
			if(Gtk::Frame *frame = dynamic_cast<Gtk::Frame*>(main->ctl)) {
				panelField = new Gtk::Fixed();
				// The field has the designer's dot grid and a border, as the field of a form.
				panelField->add_css_class("hiasm-panel-field");
				panelCss = Gtk::CssProvider::create();
				panelCss->load_from_data(".hiasm-panel-field { border: 1px solid #8a8a8a; "
					"background-image: radial-gradient(circle, #9a9a9a 1px, transparent 1px); background-size: 8px 8px; }\n");
				panelField->get_style_context()->add_provider(panelCss, G_MAXUINT);
				frame->set_child(*panelField);
				main->on_property_change += this;
				applyPanelSize(main);
			}
		}
		else if(main->tpl->conf->editClassName == "Dialog") {
			parent = main;
			VBox *vb = new VBox();
			main->ctl = vb;
			//ctrls.insert(std::pair<Widget*, Element*>(this, main));
		}
		else {
			main->prepareForRun(&parent);
			//ctrls.insert(std::pair<Widget*, Element*>(main->ctl, main));
		}
	}

	for(ElementsList::iterator e = sdk->elements.begin(); e != sdk->elements.end(); e++) {
		ElementTplWinControl *ew;
		if((*e)->sdk && (*e)->getParentElement())
			ew = dynamic_cast<ElementTplWinControl *>((*e)->getParentElement());
		else
			ew = dynamic_cast<ElementTplWinControl *>(*e);
		if(*e != main && ew) {
			prepareControl(*e, ew, &parent);
			if(!ew->ctl) continue;
			placeControl(*e, ew);
			ctrls.insert(std::pair<Widget*, Element*>(ew->ctl, *e));
			ew->on_property_change += this;
		}
	}

	if(main && !asParent)
		add(formFrame ? static_cast<Gtk::Widget&>(*formFrame) : *main->ctl);

	unBuffer(this);
}

WidgetEditorEx::~WidgetEditorEx() {
	if(activeInstance == this)
		activeInstance = NULL;

	sdk->on_remove_element -= this;
	sdk->on_add_element -= this;
	sdk->selMan->on_selection_change -= this;

	for(WEItems::iterator w = ctrls.begin(); w != ctrls.end(); w++)
		if(w->second->sdk)
			w->second->getParentElement()->on_property_change -= this;
		else
			w->second->on_property_change -= this;
	for(const auto &item : ctrls) {
		Element *element = item.second->sdk ? item.second->getParentElement() : item.second;
		delete item.first;
		if(auto control = dynamic_cast<ElementTplWinControl*>(element)) control->ctl = nullptr;
	}
	if(!asParent) {
		if(auto control = dynamic_cast<ElementTplWinControl*>(parentForm)) {
			if(panelField) {
				// The frame of the panel owns its field and the controls on it.
				control->on_property_change -= this;
				panelField->get_style_context()->remove_provider(panelCss);
				panelField = NULL;
			}
			if(formFrame) {
				// The frame owns the client area (control->ctl).
				control->on_property_change -= this;
				formSurface->get_style_context()->remove_provider(formCss);
				formFrame->get_style_context()->remove_provider(formCss);
				delete formFrame;
				formFrame = NULL;
				formSurface = NULL;
			}
			else
				delete control->ctl;
			control->ctl = nullptr;
		}
	}
	else if(auto control = dynamic_cast<ElementTplWinControl*>(parentForm)) control->ctl = nullptr;
}

void WidgetEditorEx::beginInsert(const ustring &name) {
	pendingInsertName = name;
	if(curList)
		widget_set_cursor(*this, curList->cursor(CURSOR_INSERT_ELEMENT));
}

// Prepares the widget of an element of the scheme for the editor. A container of the C++ package (ChildPanel,
// ChildForm) is not an ElementCore: its window is the inner parent element ew, and a core container (hcChildPanel)
// prepares its whole scheme itself. A child window (ChildForm, hcChildForm) is not a control of the form: it lives
// in its own scheme, so drawing its frame here would put a window frame on the main form, a panel or another child
// form. Only the form editor of its own scheme shows that window.
void WidgetEditorEx::prepareControl(Element *e, ElementTplWinControl *ew, Element **parent) {
	if(ElementCore *core = dynamic_cast<ElementCore *>(e))
		core->prepareForRun(parent);
	else if(dynamic_cast<ElementTplForm *>(ew))
		return;
	else
		ew->prepareForRun(parent);
}

// Puts the widget into the field of its panel, or into the client area of its form, and moves it to its place.
// A container is drawn where it stands in the scheme: its inner window was created at a fixed point of its own
// scheme, so the positions of several containers would coincide.
void WidgetEditorEx::placeControl(Element *e, ElementTplWinControl *ew) {
#ifdef HIASM_GTK4
	if(!ew->ctl) return;
	ElementTplWinControl *form = dynamic_cast<ElementTplWinControl*>(parentForm);
	Gtk::Widget *area = panelField ? static_cast<Gtk::Widget*>(panelField) : (form ? form->ctl : nullptr);
	if(!area) return;
	if(panelField && !ew->ctl->get_parent())
		panelField->put(*ew->ctl, 0, 0);
	if(ew->ctl->get_parent() != area) return;
	if(Gtk::Fixed *surface = dynamic_cast<Gtk::Fixed*>(area)) {
		if(ew != e)
			surface->move(*ew->ctl, GRID_FRM_X(e->x), GRID_FRM_Y(e->y));
		else
			surface->move(*ew->ctl, ew->props[0]->readInt(), ew->props[1]->readInt());
	}
#endif
}

// The frame of a panel is the panel's Width x Height, as in the parent form, with its Caption; the field of the
// controls fills the frame inside it.
void WidgetEditorEx::applyPanelSize(Element *panel) {
	if(!panelField) return;
	ElementProperty *width = panel->props.getByName("Width");
	ElementProperty *height = panel->props.getByName("Height");
	ElementTplWinControl *control = dynamic_cast<ElementTplWinControl*>(panel);
	if(Gtk::Frame *frame = control ? dynamic_cast<Gtk::Frame*>(control->ctl) : nullptr) {
		ElementProperty *caption = panel->props.getByName("Caption");
		// An empty Caption leaves no title row, so the field gets the whole panel.
		ustring text = caption ? caption->readStr() : ustring();
		if(text.empty())
			frame->unset_label();
		else
			frame->set_label(text);
		frame->set_size_request(width ? width->readInt() : 60, height ? height->readInt() : 50);
		frame->set_halign(Gtk::Align::START);
		frame->set_valign(Gtk::Align::START);
		frame->set_hexpand(false);
		frame->set_vexpand(false);
	}
	panelField->set_hexpand(true);
	panelField->set_vexpand(true);
}

void WidgetEditorEx::addElement(Element *e) {
	ElementTplWinControl *ew = NULL;
	if(e->sdk && e->getParentElement())
		ew = dynamic_cast<ElementTplWinControl *>((Element*)e->getParentElement());
	else
		ew = dynamic_cast<ElementTplWinControl *>(e);
	if(ew) {
		Element *parent = parentForm;
		prepareControl(e, ew, &parent);
		if(ew->ctl) {
			placeControl(e, ew);
			ctrls.insert(std::pair<Widget*, Element*>(ew->ctl, e));
			ew->on_property_change += this;
			ew->ctl->show();
		}
		invalidate();
	}
}

void WidgetEditorEx::unBuffer(Widget *ctl) {
	if(!dynamic_cast<TextView*>(ctl) && !dynamic_cast<TreeView*>(ctl))
		(void)ctl;
	Container *c = dynamic_cast<Container*>(ctl);
	if(!c) return;

	Glib::ListHandle<Widget*> ch = c->get_children();
	for(Glib::ListHandle<Widget*>::iterator c = ch.begin(); c != ch.end(); c++)
		unBuffer(*c); //->set_double_buffered(false);
}

Gdk::Rectangle WidgetEditorEx::widgetRectangle(Widget *widget) {
#ifdef HIASM_GTK4
	graphene_rect_t bounds;
	if(gtk_widget_compute_bounds(widget->gobj(), GTK_WIDGET(gobj()), &bounds))
		return Gdk::Rectangle(bounds.origin.x, bounds.origin.y, bounds.size.width, bounds.size.height);
	return Gdk::Rectangle();
#else
	int x = 0, y = 0;
	widget->translate_coordinates(*this, 0, 0, x, y);
	return Gdk::Rectangle(x, y, widget->get_width(), widget->get_height());
#endif
}

Element *WidgetEditorEx::getElementAtPos(gdouble x, gdouble y) {
	Element *hit = nullptr;
	// Use scheme order (also the widget stacking order), never pointer order.
	for(Element *element : sdk->elements)
		for(const auto &item : ctrls) {
			if(item.second != element) continue;
			const auto a = widgetRectangle(item.first);
			if(x >= a.get_x() && y >= a.get_y() && x < a.get_x()+a.get_width() && y < a.get_y()+a.get_height())
				hit = element;
		}
	return hit;
}

#include "main_gui.h"

// Anchors of the 8 resize grips on the selection rectangle, clockwise from the
// top-left corner (0 = top-left, 2 = top-right, 4 = bottom-right, 6 = bottom-left,
// 1/3/5/7 = edge midpoints). Same layout as ElementDynGeometry::rePosGrids().
static const double kx[8] = {0, 0.5, 1, 1, 1, 0.5, 0, 0};
static const double ky[8] = {0, 0, 0, 0.5, 1, 1, 1, 0.5};

int WidgetEditorEx::getSizer(int x, int y) {
	Gdk::Rectangle r(0,0,0,0);
	for(WEItems::iterator w = ctrls.begin(); w != ctrls.end(); w++)
		if(w->second->isSelect()) {
			if(r.has_zero_area())
				r = widgetRectangle(w->first);
			else
				r = r.join(widgetRectangle(w->first));
		}
	if(!r.has_zero_area())
		for(int i = 0; i < 8; i++) {
			gdouble _x = r.get_x() + kx[i]*r.get_width();
			gdouble _y = r.get_y() + ky[i]*r.get_height();
			if(abs(_x - x) <= 3 && abs(_y - y) <= 3)
				return i;
		}
	return -1;
}

void WidgetEditorEx::snapMovement(int &dx, int &dy, bool resizing) {
	const bool edges = SettingsManager::formBinding(SettingsManager::BindEdges);
	const bool centers = SettingsManager::formBinding(SettingsManager::BindCenter);
	const bool padding = SettingsManager::formBinding(SettingsManager::BindPadding);
	if(!edges && !centers && !padding) return;
	const auto selected = getSelectionRect();
	if(selected.has_zero_area()) return;
	for(const auto &item : ctrls)
		if(item.second->isSelect() && item.second->isFreeze()) return;
	int correctionX = 0, correctionY = 0, distanceX = 6, distanceY = 6;
	auto candidate = [](double moving, double target, int &correction, int &distance, int factor = 1) {
		const int delta = static_cast<int>(std::lround(target - moving));
		if(std::abs(delta) < distance) { distance = std::abs(delta); correction = factor*delta; }
	};
	const bool moveLeft = !resizing || selSizer == 0 || selSizer == 6 || selSizer == 7;
	const bool moveRight = !resizing || selSizer == 2 || selSizer == 3 || selSizer == 4;
	const bool moveTop = !resizing || selSizer == 0 || selSizer == 1 || selSizer == 2;
	const bool moveBottom = !resizing || selSizer == 4 || selSizer == 5 || selSizer == 6;
	const double left = selected.get_x() + (moveLeft ? dx : 0), top = selected.get_y() + (moveTop ? dy : 0);
	const double right = selected.get_x() + selected.get_width() + (moveRight ? dx : 0);
	const double bottom = selected.get_y() + selected.get_height() + (moveBottom ? dy : 0);
	auto compare = [&](const Gdk::Rectangle &target, bool parent) {
		const double l = target.get_x(), t = target.get_y();
		const double r = l + target.get_width(), b = t + target.get_height();
		if(edges) {
			for(double guide : {l, r}) {
				if(moveLeft) candidate(left, guide, correctionX, distanceX);
				if(moveRight) candidate(right, guide, correctionX, distanceX);
			}
			for(double guide : {t, b}) {
				if(moveTop) candidate(top, guide, correctionY, distanceY);
				if(moveBottom) candidate(bottom, guide, correctionY, distanceY);
			}
		}
		if(centers) {
			if(moveLeft || moveRight) candidate((left+right)/2, (l+r)/2, correctionX, distanceX, resizing ? 2 : 1);
			if(moveTop || moveBottom) candidate((top+bottom)/2, (t+b)/2, correctionY, distanceY, resizing ? 2 : 1);
		}
		if(padding) {
			if(parent) {
				if(moveLeft) candidate(left, l+5, correctionX, distanceX);
				if(moveRight) candidate(right, r-5, correctionX, distanceX);
				if(moveTop) candidate(top, t+5, correctionY, distanceY);
				if(moveBottom) candidate(bottom, b-5, correctionY, distanceY);
			} else {
				if(moveLeft) candidate(left, r+5, correctionX, distanceX);
				if(moveRight) candidate(right, l-5, correctionX, distanceX);
				if(moveTop) candidate(top, b+5, correctionY, distanceY);
				if(moveBottom) candidate(bottom, t-5, correctionY, distanceY);
			}
		}
	};
	for(Element *element : sdk->elements) {
		if(element->isSelect()) continue;
		for(const auto &item : ctrls) {
			if(item.second != element) continue;
			bool sameParent = true;
			for(const auto &moving : ctrls) if(moving.second->isSelect() && moving.first->get_parent() != item.first->get_parent()) sameParent = false;
			if(sameParent) compare(widgetRectangle(item.first), false);
		}
	}
	if(auto control = dynamic_cast<ElementTplWinControl*>(parentForm))
		if(control->ctl) compare(widgetRectangle(control->ctl), true);
	dx += correctionX; dy += correctionY;
}

void  WidgetEditorEx::moveSelection(int dx, int dy) {
	for(WEItems::iterator w = ctrls.begin(); w != ctrls.end(); w++)
		if(w->second->isSelect() && !w->second->isFreeze()) {
			//changeProperty = true;
			Element *e = w->second->sdk ? w->second->getParentElement() : w->second;
			int v = e->props[0]->readInt() + dx;
			e->props[0]->setValue(&v);
			v = e->props[1]->readInt() + dy;
			e->props[1]->setValue(&v);
			//changeProperty = false;
		}
}

void  WidgetEditorEx::sizeSelection(int dx, int dy) {
	for(WEItems::iterator w = ctrls.begin(); w != ctrls.end(); w++)
		if(w->second->isSelect() && !w->second->isFreeze()) {
			//changeProperty = true;
			Element *e = w->second->sdk ? w->second->getParentElement() : w->second;
			int v = max(5, e->props[2]->readInt() + dx);
			e->props[2]->setValue(&v);
			v = max(5, e->props[3]->readInt() + dy);
			e->props[3]->setValue(&v);
			//changeProperty = false;
		}
}

Gdk::Rectangle WidgetEditorEx::getSelectionRect() {
	Gdk::Rectangle r(0,0,0,0);
	for(WEItems::iterator w = ctrls.begin(); w != ctrls.end(); w++)
		if(w->second->isSelect()) {
			if(r.has_zero_area())
				r = widgetRectangle(w->first);
			else
				r = r.join(widgetRectangle(w->first));
		}

	return r;
}

Gdk::Rectangle WidgetEditorEx::calcRectangle(bool forDraw) {
	int x1 = min(start_x, cur_x);
	int y1 = min(start_y, cur_y);
	int x2 = max(start_x, cur_x);
	int y2 = max(start_y, cur_y);
	if(forDraw) {
		x1 -= 3;
		y1 -= 3;
		x2 += 3;
		y2 += 3;
	}
	return Gdk::Rectangle(x1, y1, x2 - x1, y2 - y1);
}

void WidgetEditorEx::selectWigdetsFromRect() {
	int x1 = min(start_x, cur_x);
	int y1 = min(start_y, cur_y);
	int x2 = max(start_x, cur_x);
	int y2 = max(start_y, cur_y);
	for(WEItems::iterator w = ctrls.begin(); w != ctrls.end(); w++) {
		Allocation a = widgetRectangle(w->first);
		if(a.get_x() > x2 || a.get_y() > y2 || (a.get_x() + a.get_width()) < x1 || (a.get_y() + a.get_height()) < y1)
			; // no
		else
			sdk->selMan->add(w->second);
	}
}

bool WidgetEditorEx::on_button_press_event(GdkEventButton* event)  {
	if(event->button != BTN_LEFT && pendingInsertName.empty()) return false;
	if(!pendingInsertName.empty()) {
		if(event->button != BTN_LEFT) {
			pendingInsertName.clear();
			if(sde) sde->on_end_operation.run(NULL);
			return true;
		}
		ustring name = pendingInsertName;
		pendingInsertName.clear();
		if(curList)
			widget_set_cursor(*this, curList->cursor(CURSOR_DEFAULT));
		if(sde) sde->beginOperation(MS_SCROLL);
		Element *e = sdk->add(name, GRID_FRM_X(event->x), GRID_FRM_Y(event->y));
		if(e) {
			sdk->selMan->select(e);
		}
		if(sde) {
			sde->changeScheme();
			sde->endOperation();
		}
		invalidate();
		return true;
	}

//	sdk->selMan->clear();
	Element *e = getElementAtPos(event->x, event->y);
//	if(e)
//		sdk->selMan->add(e);

	int sz = getSizer(event->x, event->y);
	if(sde) sde->beginOperation(MS_SCROLL);
	if(sz != -1) {
		selSizer = sz;
		state = WE_MS_SIZER;
	}
	else if(e) {
		if(e->isSelect()) {
			//setSelectionChange(true);
		}
		else {
			if(event->state & GDK_SHIFT_MASK) sdk->selMan->add(e);
			else sdk->selMan->select(e);
		}
		state = WE_MS_MOVE;
	}
	else {
		sdk->selMan->clear();
		state = WE_MS_RECT;
		cur_x = event->x;
		cur_y = event->y;
	}

	start_x = event->x;
	start_y = event->y;

	invalidate();

	return true;
}

bool WidgetEditorEx::on_button_release_event(GdkEventButton* event) {
	if(event->button != BTN_LEFT) return false;
	switch(state) {
		case WE_MS_MOVE:
			//setSelectionChange(false);
			break;
		case WE_MS_RECT:
			selectWigdetsFromRect();
			break;
		default:;
	}
	state = WE_MS_NONE;
	if(sde) sde->endOperation();

	invalidate();

	return true;
}

bool WidgetEditorEx::on_motion_notify_event(GdkEventMotion* event) {
	switch(state) {
		case WE_MS_NONE: {
			if(!curList) break;
			int sz = getSizer(event->x, event->y);
			switch(sz) {
				case 0: widget_set_cursor(*this, curList->cursor(CURSOR_TL_CORNER)); break;
				case 5:
				case 1: widget_set_cursor(*this, curList->cursor(CURSOR_V_DOUBLE)); break;
				case 2: widget_set_cursor(*this, curList->cursor(CURSOR_TR_CORNER)); break;
				case 7:
				case 3: widget_set_cursor(*this, curList->cursor(CURSOR_H_DOUBLE)); break;
				case 4: widget_set_cursor(*this, curList->cursor(CURSOR_BR_CORNER)); break;
				case 6: widget_set_cursor(*this, curList->cursor(CURSOR_BL_CORNER)); break;
				default: widget_set_cursor(*this, curList->cursor(CURSOR_DEFAULT)); break;
			}
			break;
		}
		case WE_MS_MOVE: {
			int dx = (int)((event->x - start_x) / 5)*5;
			int dy = (int)((event->y - start_y) / 5)*5;
			if(abs(dx) >= 5 || abs(dy) >= 5) {
				snapMovement(dx, dy);
				moveSelection(dx, dy);
				if(sde) sde->changeScheme();
				start_x += dx;
				start_y += dy;
			}
			break;
		}
		case WE_MS_RECT: {
			Gdk::Rectangle r = calcRectangle();
			cur_x = event->x;
			cur_y = event->y;
			r = r.join(calcRectangle());
			widget_invalidate_rect(*this, r);
			break;
		}
		case WE_MS_SIZER: {
			Gdk::Rectangle r = getSelectionRect();
			int dx = (int)((event->x - start_x) / 5)*5;
			int dy = (int)((event->y - start_y) / 5)*5;
			if(dx || dy) snapMovement(dx, dy, true);
			switch(selSizer) {
				case 0:
					moveSelection(dx, dy);
					sizeSelection(-dx, -dy);
					break;
				case 1:
					moveSelection(0, dy);
					sizeSelection(0, -dy);
					break;
				case 2:
					moveSelection(0, dy);
					sizeSelection(dx, -dy);
					break;
				case 3:
					sizeSelection(dx, 0);
					break;
				case 4:
					sizeSelection(dx, dy);
					break;
				case 5:
					sizeSelection(0, dy);
					break;
				case 6:
					moveSelection(dx, 0);
					sizeSelection(-dx, dy);
					break;
				case 7:
					moveSelection(dx, 0);
					sizeSelection(-dx, 0);
					break;
			}
			start_x += dx;
			start_y += dy;
			if((dx || dy) && sde) sde->changeScheme();
			r = r.join(getSelectionRect());
			r = Gdk::Rectangle(r.get_x() - 4, r.get_y() - 4, r.get_width() + 8, r.get_height() + 8);
			widget_invalidate_rect(*this, r);
			break;
		}
		default: ;
	}

	return true;
}

bool WidgetEditorEx::on_draw (const ::Cairo::RefPtr< ::Cairo::Context >& cr) {
	cr->set_source_rgb(0.0, 0.0, 0.0);
	gdouble zx = 0;
	gdouble zy = 0;
	for(int j = 0; j < get_height() + 10; j += 10)
		for(int i = 0; i < get_width() + 10; i += 10)
			cr->rectangle(zx + i, zy + j, 1, 1);
	cr->fill();
		
	EventBox::on_draw(cr);
#ifdef HIASM_GTK4
	return true;
#else
	return drawSelection(cr);
#endif
}

bool WidgetEditorEx::drawSelection(const Cairo::RefPtr<Cairo::Context> &cr) {
	for(WEItems::iterator e = ctrls.begin(); e != ctrls.end(); e++) {
		if(e->second->isSelect()) {
			Allocation a = widgetRectangle(e->first);
			cr->set_source_rgba(0.5, 0.7, 0.7, 0.4);
			cr->rectangle(a.get_x(), a.get_y(), a.get_width(), a.get_height());
			cr->fill();
		}
	}

	Gdk::Rectangle r = getSelectionRect();
	if(!r.has_zero_area())
		for(int i = 0; i < 8; i++) {
			gdouble _x = r.get_x() + kx[i]*r.get_width();
			gdouble _y = r.get_y() + ky[i]*r.get_height();
			cr->set_source_rgb(0, 0, 0);
			cr->rectangle(_x - 3, _y - 3, 6, 6);
			cr->fill();
		}

	switch(state) {
		case WE_MS_RECT: {
			Gdk::Rectangle r = calcRectangle(false);
			cr->set_source_rgba(0.5, 0.7, 0.7, 0.4);
			cr->rectangle(r.get_x(), r.get_y(), r.get_width(), r.get_height());
			cr->fill();
			cr->set_line_width(1);
			cr->set_source_rgba(0.2, 0.2, 0.2, 0.4);
			cr->rectangle(r.get_x(), r.get_y(), r.get_width(), r.get_height());
			cr->stroke();

			break;
		}
		default:;
	}
	
	return true;
}

// Size from Width and Height, title from Caption, border from BorderStyle and background from Color,
// the same properties the runtime window uses (ElementTplForm::applyFrame, createWidget).
// The form is drawn like the window of HiAsm 4: a title bar (icon, Caption and the window buttons) above the
// client area. Which parts show follows BorderStyle (applyFormStyle).
void WidgetEditorEx::buildFormFrame() {
	formFrame = new Gtk::Box(Gtk::Orientation::VERTICAL);
	formFrame->add_css_class("hiasm-form-frame");
	formTitle = new Gtk::Box(Gtk::Orientation::HORIZONTAL, 4);
	formTitle->add_css_class("hiasm-form-title");
	// The HiAsm icon, as the window of HiAsm 4 shows it in the title bar.
	auto icon = new Gtk::Picture();
	const ustring logo = dataDir + INT_PATH"hiasm.png";
	if(g_file_test(logo.c_str(), G_FILE_TEST_IS_REGULAR)) icon->set_filename(logo.raw());
	icon->set_size_request(16, 16);
	icon->set_can_shrink(false);
	formTitle->append(*icon);
	formTitleText = new Gtk::Label();
	formTitleText->set_hexpand(true);
	formTitleText->set_xalign(0);
	formTitle->append(*formTitleText);
	auto windowButton = [this](const char *iconName) {
		auto button = new Gtk::Button();
		button->set_icon_name(iconName);
		button->add_css_class("flat");
		button->add_css_class("hiasm-window-button");
		button->get_style_context()->add_provider(formCss, G_MAXUINT);
		formTitle->append(*button);
		return button;
	};
	formMinimize = windowButton("window-minimize-symbolic");
	formMaximize = windowButton("window-maximize-symbolic");
	windowButton("window-close-symbolic");
	formTitle->get_style_context()->add_provider(formCss, G_MAXUINT);
	formFrame->get_style_context()->add_provider(formCss, G_MAXUINT);
	formFrame->append(*formTitle);
	formFrame->append(*formSurface);
}

void WidgetEditorEx::applyFormStyle(Element *form) {
	if(!formSurface || !formFrame) return;
	ElementProperty *width = form->props.getByName("Width");
	ElementProperty *height = form->props.getByName("Height");
	formSurface->set_size_request(width ? width->readInt() : 400, height ? height->readInt() : 300);

	// BorderStyle (Delphi list, default bsSizeable): 0 bsNone has no frame; 1..5 have a title bar.
	ElementProperty *border = form->props.getByName("BorderStyle");
	const int style = border ? border->readInt() : 2;
	const bool titled = style >= 1 && style <= 5;
	ElementProperty *caption = form->props.getByName("Caption");
	formTitleText->set_text(titled && caption ? caption->readStr() : ustring());
	formTitle->set_visible(titled);
	// Minimize and maximize belong to the sizable borders (bsSizeable, bsSizeToolWin, bsSizeBox, bsSizeBoxEdge).
	const bool sizable = style == 2 || style == 5 || style == 7 || style == 9;
	formMinimize->set_visible(sizable);
	formMaximize->set_visible(sizable);
	if(style == 0)
		formFrame->add_css_class("hiasm-noframe");
	else
		formFrame->remove_css_class("hiasm-noframe");

	ElementProperty *color = form->props.getByName("Color");
	std::string css = ".hiasm-noframe { border-width: 0; }\n"
		".hiasm-form-frame { border: 1px solid #8a8a8a; }\n"
		".hiasm-form-title { background-color: #f3f3f3; padding: 2px 4px; }\n"
		".hiasm-window-button { min-width: 28px; min-height: 22px; margin: 0; padding: 0; border-radius: 0; }\n";
	if(color && color->type == data_color)
		css += ".hiasm-form-surface { background-color: " + ((TypeColor*)color->value)->to_string() + "; }\n";
	// The designer's dot grid stays visible on the client area, as in HiAsm 4.
	css += ".hiasm-form-surface { background-image: radial-gradient(circle, #9a9a9a 1px, transparent 1px); background-size: 8px 8px; }\n";
	formCss->load_from_data(css);
}

void WidgetEditorEx::changeProperty(ElementProperty *prop) {
	Element *e = prop->parent;
	if(formFrame && e == parentForm) {
		applyFormStyle(e);
		return;
	}
	if(panelField && e == parentForm) {
		applyPanelSize(e);
		return;
	}
	Widget *w = NULL, *fw;

	WEItems::iterator we;
	for(we = ctrls.begin(); we != ctrls.end(); we++)
		if(we->second == e || we->second == e->parent->parent) {
			w = we->first;
			break;
		}
	if(!w) return;
	fw = w;
	if(ScrolledWindow *c = dynamic_cast<ScrolledWindow*>(w)) {
		w = *c->get_children().begin();
	}

	// update widget params
	if(prop->name == "Width" || prop->name == "Height") {
		fw->set_size_request(e->props[2]->readInt(), e->props[3]->readInt());
	}
	else if(prop->name == "Left" || prop->name == "Top") {
		Fixed *f = dynamic_cast<Fixed*>(fw->get_parent());
		if(f) {
			Gdk::Rectangle r = fw->get_allocation();
			f->move(*fw, e->props[0]->readInt(), e->props[1]->readInt());
			r.join(fw->get_allocation());
			//widget_invalidate_rect(*this, r);
			invalidate();
		}
	}
	else if(prop->name == "Enabled") {
		w->set_sensitive(prop->readInt() == 0);
	}
	else if(e->tpl->conf->editClassName == "Label") {
		if(prop->name == "Caption")
			((Label *)w)->set_text(prop->readStr());
	}
	else if(prop->name == "Caption") {
		// Caption of the other controls: a button, a check box or a group box frame
		if(auto button = dynamic_cast<Gtk::Button*>(w)) button->set_label(prop->readStr());
		else if(auto check = dynamic_cast<Gtk::CheckButton*>(w)) check->set_label(prop->readStr());
		else if(auto frame = dynamic_cast<Gtk::Frame*>(w)) frame->set_label(prop->readStr());
	}
	else if(e->tpl->conf->editClassName == "FileChooserButton") {
		if(prop->name == "Action")
			((FileChooserButton *)w)->set_action(file_chooser_action(prop->readInt()));
	}
	else if(e->tpl->conf->editClassName == "Edit") {
		if(prop->name == "Text")
			((Entry *)w)->set_text(prop->readStr());
	}
	else if(e->tpl->conf->editClassName == "Memo") {
		if(prop->name == "Strings")
			((TextView *)w)->get_buffer()->set_text(prop->readStr());
	}
	else if(e->tpl->conf->editClassName == "Button") {
		((Button *)w)->set_label(e->props[7]->readStr());

		button_set_image_position(*((Gtk::Button*)w), (PositionType)(e->props[11]->readInt() + POS_LEFT));

		if(e->props[9]->isDefault()) {
			Image *img = new Image();
			button_set_image(*((Gtk::Button*)w), *img);
		}
		else {
			BuiltinStockID sid = {e->props[9]->readStr().c_str()};
			Image *img = stock_image(sid, (BuiltinIconSize)(e->props[10]->readInt() + ICON_SIZE_MENU));
			button_set_image(*((Gtk::Button*)w), *img);
		}
	}
	else if(e->tpl->conf->editClassName == "Image") {
		Container *c = dynamic_cast<Container*>(w);
		Image *img = (Image*)(c ? *c->get_children().begin() : w);
		if(e->props[10]->isDefault()) {
			if(e->props[12]->isDefault())
				img->clear();
			else
				img->set(*(TypePixbuf*)e->props[12]->value);
		}
		else {
			BuiltinStockID sid = {e->props[10]->readStr().c_str()};
			image_set_stock(*img, sid, (BuiltinIconSize)(e->props[11]->readInt() + ICON_SIZE_MENU));
		}
	}
	else if(e->tpl->conf->editClassName == "CheckBox") {
		if(prop->name == "Checked")
			((CheckButton *)w)->set_active(prop->readInt() == 0);
		else if(prop->name == "Caption")
			((CheckButton *)w)->set_label(prop->readStr());
	}
	/*
	else if(e->tpl->conf->editClassName == "ScrollBar") {
		if(prop->name == "Kind") {
			Container *p = w->get_parent();
			p->remove(*w);
			delete w;
			if(prop->readInt())
				w = new VScrollbar();
			else
				w = new HScrollbar();
			p->add(*w);
			ctrls.erase(we);
			ctrls.insert(std::pair<Widget*, Element*>(w, e));
			for(ElementProperties::iterator prop = e->props.begin(); prop != e->props.end(); prop++)
				changeProperty(*prop);
			w->show();
		}
	}
	*/
}

void WidgetEditorEx::callback(void *owner, CallbackType type, const void *data) {
	switch(type) {
		case CBT_SDK_CHANGE_SELECT:
			invalidate();
			/*
			for(ElementsList::iterator e = sdk->selMan->elements.begin(); e != sdk->selMan->elements.end(); e++) {
				for(WidgetsList::iterator w = wlist.begin(); w != wlist.end(); w++) {
					if((*w)->tpl == *e) {
						(*w)->select = true;
						break;
					}
				}
			}
			invalidate();
			*/
			break;
		case CBT_SDK_REMOVE_ELEMENT:
			for(WEItems::iterator e = ctrls.begin(); e != ctrls.end(); e++) {
				if(e->second == data) {
					Element *control = e->second->sdk ? e->second->getParentElement() : e->second;
					if(control) control->on_property_change -= this;
					delete e->first;
					if(auto widget = dynamic_cast<ElementTplWinControl*>(control)) widget->ctl = nullptr;
					ctrls.erase(e);
					invalidate();
					break;
				}
			}
			break;
		case CBT_SDK_ADD_ELEMENT:
			addElement((Element*)data);
			break;
		case CBT_ELEMENT_PROP_CHANGE:
			changeProperty((ElementProperty*)data);
			break;
		default: ;
	}
}

void WidgetEditorEx::edit(Container *w) {
	add(*w);
//	w->signal_expose_event().connect(sigc::mem_fun(*this, &WidgetEditorEx::on_child_expose_event));
//	for(Glib::ListHandle<Widget*>::iterator c = w->get_children().begin(); c != w->get_children().end(); c++)
//		(*c)->signal_expose_event().connect(sigc::mem_fun(*this, &WidgetEditorEx::on_child_expose_event));
		//DEBUG_MSG("ok")
}

//-----------------------------------------------------------------------------------------------

void FE_Internal::start(SDK_Editor *sde) {
	FormEditor::start(sde);

	we = new WidgetEditorEx(sde->csdk, sde);
	widget_add_child(*sde->get_parent(), *we);
	we->set_hexpand(true);
	we->set_vexpand(true);
	compat_show_all(*we);

	sde->hide();
}

void FE_Internal::stop() {
	if(we) {
		widget_remove_child(*sde->get_parent(), *we);
		delete we;
		we = NULL;
	}
	sde->show();
}
