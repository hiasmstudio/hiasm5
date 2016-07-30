/*
 * FormEditor.cpp
 *
 *  Created on: 19.05.2010
 *      Author: dilma
 */

#include "FormEditor.h"
#include "Elements.h"
#include <glibmm.h>

#define PK_CMD_ADD      1
#define PK_CMD_SET_INT  2
#define PK_CMD_SET_STR  3
#define PK_CMD_BEGIN    4
#define PK_CMD_END      5
#define PK_CMD_SELECT   6
#define PK_CMD_DELETE   7

typedef struct {
    char name[65];
    int EID;
    int x;
    int y;
    int w;
    int h;
} TAddPack;

typedef struct {
    int cmd;
    char name[65];
    int EID;
    int value;
} TSetIntPack;

typedef struct {
    int cmd;
    char name[65];
    int EID;
    char value[65];
} TSetStrPack;

typedef struct {
    int cmd;
    int EID;
} TSelectElementPack;

typedef TSelectElementPack TDeleteElementPack;

void FE_ExternalTcp::start(SDK_Editor *sde) {
	FormEditor::start(sde);

	cancel = NULL;
	sock = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM, G_SOCKET_PROTOCOL_TCP, NULL);
	GSocketAddress *addr = g_inet_socket_address_new(g_inet_address_new_from_string("127.0.0.1"), 4588);
	if(!g_socket_bind(sock, addr, true, NULL))
		DEBUG_MSG("Bind failed!")
	g_object_unref(addr);
	if(!g_socket_listen(sock, NULL))
		DEBUG_MSG("Listen failed!")

	serverThread = Glib::Thread::create(sigc::mem_fun(*this, &FE_ExternalTcp::on_server_thread), true);
	feThread = Glib::Thread::create(sigc::mem_fun(*this, &FE_ExternalTcp::on_form_edit_thread), false);

	sde->csdk->on_remove_element += this;
	sde->csdk->selMan->on_selection_change += this;
	sde->csdk->on_add_element += this;
}

void FE_ExternalTcp::stop() {
	sde->csdk->on_remove_element -= this;
	sde->csdk->selMan->on_selection_change -= this;
	sde->csdk->on_add_element -= this;

	on_end_edit.clear();

	g_socket_close(sock, NULL);
	g_object_unref(sock);
	if(cancel) {
		g_cancellable_cancel(cancel);
		serverThread->join();
	}

	for(ElementsList::iterator e = sde->csdk->elements.begin(); e != sde->csdk->elements.end(); e++)
		if((*e)->tpl->conf->isWinElement())
			(*e)->on_property_change -= this;
}

void FE_ExternalTcp::pk_add_element(Element *e) {
	TAddPack ap;
	strcpy(ap.name, e->tpl->name.c_str());
	ap.EID = e->id;
	ap.x = e->props[0]->readInt();
	ap.y = e->props[1]->readInt();
	ap.w = e->props[2]->readInt();
	ap.h = e->props[3]->readInt();

	pk_send(PK_CMD_ADD, &ap, sizeof(TAddPack));

	for(ElementProperties::iterator prop = e->props.begin(); prop != e->props.end(); prop++)
		pk_send_prop(*prop);

	e->on_property_change += this;
}

void FE_ExternalTcp::pk_send(int num, void *data, int size) {
	g_socket_send(client, (const gchar *)&num, sizeof(int), NULL, NULL);
	if(size) {
		g_socket_send(client, (const gchar *)data, size, NULL, NULL);
	}
}

void FE_ExternalTcp::pk_send_prop(ElementProperty *prop) {
	switch(prop->type) {
		case data_list:
		case data_str: {
			TSetStrPack str;
			str.cmd = PK_CMD_SET_STR;
			str.EID = prop->parent->id;
			strcpy(str.name, prop->name.c_str());
			strncpy(str.value, prop->readStr().c_str(), 64);
			g_socket_send(client, (const gchar *)&str, sizeof(TSetStrPack), NULL, NULL);
			break;
		}
		case data_int:
		case data_combo:
		case data_comboEx: {
			TSetIntPack iv;
			iv.cmd = PK_CMD_SET_INT;
			iv.EID = prop->parent->id;
			strcpy(iv.name, prop->name.c_str());
			iv.value = prop->readInt();
			g_socket_send(client, (const gchar *)&iv, sizeof(TSetIntPack), NULL, NULL);
			break;
		}

		default:;
	}
}

void FE_ExternalTcp::pk_send_remove(Element *e) {
	TDeleteElementPack dep;
	dep.cmd = PK_CMD_DELETE;
	dep.EID = e->id;
	g_socket_send(client, (const gchar *)&dep, sizeof(TDeleteElementPack), NULL, NULL);
}

void FE_ExternalTcp::pk_send_select(SelectManager *sm) {
	// clear selection
	TSelectElementPack sep;
	sep.cmd = PK_CMD_SELECT;
	sep.EID = 0;
	g_socket_send(client, (const gchar *)&sep, sizeof(TSelectElementPack), NULL, NULL);

	// send selected elements
	for(ElementsList::iterator e = sm->elements.begin(); e != sm->elements.end(); e++) {
		sep.EID = (*e)->id;
		if((*e)->tpl->conf->isWinElement())
			g_socket_send(client, (const gchar *)&sep, sizeof(TSelectElementPack), NULL, NULL);
	}
}

void FE_ExternalTcp::callback(void *owner, CallbackType type, const void *data) {
	switch(type) {
		case CBT_ELEMENT_PROP_CHANGE:
			pk_send_prop((ElementProperty*)data);
			break;
		case CBT_SDK_REMOVE_ELEMENT:
			pk_send_remove((Element*)data);
			break;
		case CBT_SDK_CHANGE_SELECT:
			pk_send_select((SelectManager*)owner);
			break;
		case CBT_SDK_ADD_ELEMENT:
			if(((Element*)data)->tpl->conf->isWinElement())
				pk_add_element((Element*)data);
			break;
		default:;
	}
}

void FE_ExternalTcp::on_form_edit_thread() {
	ustring fe = sde->sdk->pack->pathMake() + "formedit";
	system(fe.c_str());
}

void FE_ExternalTcp::on_server_thread() {
	client = g_socket_accept(sock, NULL, NULL);
	if(client) {
		pk_send(PK_CMD_BEGIN, NULL, 0);
		DEBUG_MSG("Connect from editor!")
		for(ElementsList::iterator e = sde->csdk->elements.begin(); e != sde->csdk->elements.end(); e++)
			if((*e)->isParent()) {
				pk_add_element(*e);
				break;
			}

		for(ElementsList::iterator e = sde->csdk->elements.begin(); e != sde->csdk->elements.end(); e++)
			if(!(*e)->isParent() && (*e)->tpl->conf->isWinElement()) {
				pk_add_element(*e);
			}
		pk_send(PK_CMD_END, NULL, 0);

		int cmd;
		DEBUG_MSG("Read data from client...")
		cancel = g_cancellable_new();
		while(g_socket_receive(client, (gchar *)&cmd, sizeof(cmd), cancel, NULL) > 0) {
			//DEBUG_MSG("Read command " << cmd)
			switch(cmd) {
				case PK_CMD_SET_INT: {
					TSetIntPack ap;
					g_socket_receive(client, (gchar *)&ap.name, sizeof(ap) - sizeof(int), NULL, NULL);
					Element *e = sde->csdk->getElementById(ap.EID);
					if(e) {
						ElementProperty *prop = e->props.getByName(ap.name);
						if(prop) {
							gdk_threads_enter();
							e->on_property_change -= this;
							prop->setValue(&ap.value);
							e->on_property_change += this;
							gdk_threads_leave();
						}
					}
					break;
				}
				case PK_CMD_SELECT: {
					TSelectElementPack sep;
					g_socket_receive(client, (gchar *)&sep.EID, sizeof(sep) - sizeof(int), NULL, NULL);
					Element *e = sde->csdk->getElementById(sep.EID);
					if(e) {
						gdk_threads_enter();
						sde->csdk->selMan->select(e);
						gdk_threads_leave();
					}
					break;
				}
				case PK_CMD_DELETE: {
					TDeleteElementPack dep;
					g_socket_receive(client, (gchar *)&dep.EID, sizeof(dep) - sizeof(int), NULL, NULL);
					Element *e = sde->csdk->getElementById(dep.EID);
					if(e) {
						gdk_threads_enter();
						sde->csdk->selMan->erase();
						gdk_threads_leave();
					}
					break;
				}
			}
		}
		DEBUG_MSG("Client disconnect")
		g_socket_close(client, NULL);
		g_object_unref(client);
	}
	g_object_unref(cancel);
	cancel = NULL;

	if(!on_end_edit.empty()) {
		gdk_threads_enter();
		on_end_edit();
		gdk_threads_leave();
	}
}

//-------------------------------------------------------------------------------------------------------------------

WidgetInfo::WidgetInfo(WidgetEditor *parent, Element *e) {
	wedit = parent;

	tpl = e;

	select = false;
	change = false;

	changeProperty = false;

	tpl->on_property_change += this;

	init();
}

WidgetInfo::~WidgetInfo() {
	TRACE_PROC
	// TODO remove widgets
	tpl->on_property_change -= this;
//	delete ctl;
//	delete window;
}


gboolean damage (GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
  return ((WidgetInfo*)data)->on_expose_event(event);
}


void WidgetInfo::init() {
	Element *e = tpl->sdk ? tpl->getParentElement() : tpl;

	DEBUG_MSG("CLASS:" << e->tpl->conf->editClassName.c_str())

	if(e->tpl->conf->editClassName == "Label") {
		ctl = new Label(e->props[7]->readStr());
		((Label*)ctl)->set_justify((Justification)(e->props[8]->readInt() + JUSTIFY_LEFT));
	}
	else if(e->tpl->conf->editClassName == "Edit") {
		ctl = new Entry();
	}
	else if(e->tpl->conf->editClassName == "Button") {
		ctl = new Button(tpl->props[7]->readStr());
		if(!e->props[9]->isDefault()) {
			if(!e->props[11]->isDefault())
				((Button*)ctl)->set_image_position((PositionType)(e->props[11]->readInt() + POS_LEFT));

			BuiltinStockID sid = {e->props[9]->readStr().c_str()};
			Image *img = new Image(sid, (BuiltinIconSize)(e->props[10]->readInt() + ICON_SIZE_MENU));
			((Button*)ctl)->set_image(*img);
			img->show();
		}
	}
	else if(e->tpl->conf->editClassName == "CheckBox") {
		ctl = new CheckButton(e->props[8]->readStr());
		((CheckButton*)ctl)->set_active(e->props[7]->readInt() == 0);
	}
	else if(e->tpl->conf->editClassName == "ScrollBar") {
		if(e->props[10]->readInt())
			ctl = new VScrollbar();
		else
			ctl = new HScrollbar();
	}
	else if(e->tpl->conf->editClassName == "ListBox") {
		Glib::RefPtr<ListStore> list = Gtk::ListStore::create(listColumns);
		ctl = new TreeView(list);
	}
	else if(e->tpl->conf->editClassName == "GroupBox") {
		ctl = new Frame(e->props[7]->readStr());
	}
	else {
		ctl = new Label();
	}

	ctl->set_sensitive(e->props[5]->readInt() == 0);
	ctl->show();

	x = e->props[0]->readInt();
	y = e->props[1]->readInt();
	width = e->props[2]->readInt();
	height = e->props[3]->readInt();

//	window = gtk_offscreen_window_new();
//	gtk_window_set_default_size(GTK_WINDOW(window), width, height);
//	gtk_container_add(GTK_CONTAINER(window), ctl->gobj());

//	gtk_widget_show_all(window);
//	g_signal_connect (window, "damage-event", G_CALLBACK (damage), this);
}

void WidgetInfo::on_realize() {
	TRACE_PROC
}

void WidgetInfo::snap() {
//	bmp = window->get_pixmap();
	nb = false;
}

bool WidgetInfo::checkAtPos(gdouble x, gdouble y) {
	return (this->x <= x) && (this->x + width >= x) && (this->y <= y) && (this->y + height >= y);
}

void WidgetInfo::move(int dx, int dy) {
	x += dx;
	y += dy;
	changeProperty = true;
	Element *e = tpl->sdk ? tpl->getParentElement() : tpl;
	e->props[0]->setValue(&x);
	e->props[1]->setValue(&y);
	changeProperty = false;
}

void WidgetInfo::size(int dx, int dy) {
	width += dx;
	height += dy;

//	window->set_default_size(width, height);
	gtk_window_set_default_size(GTK_WINDOW(window), width, height);

	changeProperty = true;
	Element *e = tpl->sdk ? tpl->getParentElement() : tpl;
	e->props[2]->setValue(&width);
	e->props[3]->setValue(&height);
	changeProperty = false;
}

bool WidgetInfo::on_expose_event(GdkEventExpose* event) {
	Gdk::Rectangle r(x, y, width, height);
	wedit->get_window()->invalidate_rect(r, false);
	return false;
}

void WidgetInfo::on_size_allocate(Allocation &alloc) {
	DEBUG_MSG("Alloc size")
	//snap();
}

void WidgetInfo::callback(void *owner, CallbackType type, const void *data) {
	if(changeProperty) return;

	ElementProperty *p = (ElementProperty *)data;
	if(p == tpl->props[0]) {
		move(p->readInt() - x, 0);
		wedit->invalidate();
	}
	else if(p == tpl->props[1]) {
		move(0, p->readInt() - y);
		wedit->invalidate();
	}
	else if(p == tpl->props[2]) {
		size(p->readInt() - width, 0);
		wedit->invalidate();
	}
	else if(p == tpl->props[3]) {
		size(0, p->readInt() - height);
		wedit->invalidate();
	}
	else if(p == tpl->props[5]) {
		ctl->set_sensitive(p->readInt() == 0);
	}
	else if(tpl->tpl->conf->editClassName == "Label") {
		if(p == tpl->props[7])
			((Label *)ctl)->set_text(p->readStr());
		else if(p == tpl->props[8])
			((Label *)ctl)->set_justify((Justification)(p->readInt() + JUSTIFY_LEFT));
	}
	else if(tpl->tpl->conf->editClassName == "Edit") {

	}
	else if(tpl->tpl->conf->editClassName == "Button") {
		((Button *)ctl)->set_label(tpl->props[7]->readStr());

		((Button*)ctl)->set_image_position((PositionType)(tpl->props[11]->readInt() + POS_LEFT));

		if(tpl->props[9]->isDefault()) {
			// remove image
		}
		else {
			BuiltinStockID sid = {tpl->props[9]->readStr().c_str()};
			Image *img = new Image(sid, (BuiltinIconSize)(tpl->props[10]->readInt() + ICON_SIZE_MENU));
			((Button*)ctl)->set_image(*img);
			img->show();
		}
	}
	else if(tpl->tpl->conf->editClassName == "CheckBox") {
		if(p == tpl->props[7])
			((CheckButton *)ctl)->set_active(p->readInt() == 0);
		else if(p == tpl->props[8])
			((CheckButton *)ctl)->set_label(p->readStr());
	}
	else if(tpl->tpl->conf->editClassName == "ScrollBar") {
		if(p == tpl->props[10]) {
			gtk_container_remove(GTK_CONTAINER(window), ctl->gobj());
			delete ctl;
			if(p->readInt())
				ctl = new VScrollbar();
			else
				ctl = new HScrollbar();
			gtk_container_add(GTK_CONTAINER(window), ctl->gobj());
			ctl->set_sensitive(tpl->props[5]->readInt() == 0);
			ctl->show();
		}
	}
}

//-----------------------------------------------------------------------------------------------

static const double kx[] = {0, 0.5, 1, 1, 1, 0.5, 0, 0};
static const double ky[] = {0, 0, 0, 0.5, 1, 1, 1, 0.5};

WidgetEditor::WidgetEditor(SDK *parent) {
//	set_events(Gdk::BUTTON_PRESS_MASK|Gdk::POINTER_MOTION_MASK|Gdk::BUTTON_RELEASE_MASK);
//	set_size_request(200, 200);
	set_above_child(true);

	sdk = parent;
	sdk->selMan->on_selection_change += this;
	sdk->on_remove_element += this;

	state = WE_MS_NONE;
}

WidgetEditor::~WidgetEditor() {
	for(WidgetsList::iterator w = wlist.begin(); w != wlist.end(); w++)
		delete *w;
	wlist.clear();
	sdk->selMan->on_selection_change -= this;
	sdk->on_remove_element -= this;
}

void WidgetEditor::addWidget(Element *e) {
	wlist.push_back(new WidgetInfo(this, e));
}

bool WidgetEditor::on_draw (const ::Cairo::RefPtr< ::Cairo::Context >& cr) {
	EventBox::on_draw(cr);
	
	Gdk::RGBA black;
	black.set_rgba(0,0,0);
	Gdk::Cairo::set_source_rgba(cr, black);
	cr->rectangle(0, 0, 200, 100);
	cr->fill();
	
	return true;
}

bool WidgetEditor::on_expose_event(GdkEventExpose* event) {
//	EventBox::on_expose_event(event);
//
//	GdkGC *gc = gdk_gc_new(event->window);
//	gdk_gc_set_subwindow(gc, GDK_INCLUDE_INFERIORS);
//	gdk_draw_line(event->window, gc, 0, 0, 400, 400);
//
//
////	DrawContext cr = get_window()->create_cairo_context();
//
////	cr->set_line_width(0.5);
////	cr->set_antialias(Cairo::ANTIALIAS_SUBPIXEL);
//
////	Gdk::Cairo::set_source_color(cr, get_style()->get_bg(Gtk::STATE_NORMAL));
////	cr->rectangle(event->area.x, event->area.y, event->area.width, event->area.height);
////	cr->fill();
//
////	cr->set_source_rgb(0, 0, 0);
////	for(int j = 0; j < get_height(); j += 10)
////		for(int i = 0; i < get_width(); i += 10)
////			cr->rectangle(i, j, 1, 1);
////	cr->fill();
//
//	int c = 0;
//	Gdk::Rectangle r(0,0,0,0);
//	for(WidgetsList::iterator wl = wlist.begin(); wl != wlist.end(); wl++) {
////		GdkPixmap *pixmap = gtk_offscreen_window_get_pixmap(GTK_OFFSCREEN_WINDOW((*wl)->window));
////		if(!pixmap)
////			DEBUG_MSG("Failed pixmap")
///*
//		cr->save();
//		cr->translate((*wl)->x, (*wl)->y);
//		gdk_cairo_set_source_pixmap(cr->cobj(), pixmap, 0, 0);
//		cr->rectangle (0, 0, (*wl)->width, (*wl)->height);
//		cr->clip();
//		cr->paint();
//		cr->restore();
//*/
//		if((*wl)->select) {
//			if((*wl)->change) {
//				gdouble _x = (*wl)->x + (*wl)->width/2;
//				gdouble _y = (*wl)->y + (*wl)->height/2;
////				cr->set_source_rgba(0.0, 1.0, 1.0, 0.6);
////				cr->arc(_x, _y, 5, 0, 2*3.1415);
////				cr->fill();
////				cr->set_source_rgba(0.0, 0.0, 0.0, 0.4);
////				cr->arc(_x, _y, 5, 0, 2*3.1415);
////				cr->stroke();
//				gdk_draw_rectangle(event->window, gc, true, _x - 4, _y - 4, 8, 8);
//			}
//			else {
//				c++;
//				if(r.has_zero_area())
//					r = (*wl)->getRect();
//				else
//					r.join((*wl)->getRect());
//			}
//		}
//	}
//
//	for(int i = 0; c && i < 8; i++) {
//		gdouble _x = r.get_x() + kx[i]*r.get_width();
//		gdouble _y = r.get_y() + ky[i]*r.get_height();
////		cr->set_source_rgba(1, 1, 1, 0.7);
////		cr->arc(_x, _y, 3, 0, 2*3.1415);
////		cr->fill();
////		cr->set_source_rgba(0, 0, 0, 0.8);
////		cr->arc(_x, _y, 3, 0, 2*3.1415);
////		cr->stroke();
//		gdk_draw_rectangle(event->window, gc, true, _x - 3, _y - 3, 6, 6);
//	}
//
//	switch(state) {
//		case WE_MS_RECT: {
//			//cr->set_antialias(Cairo::ANTIALIAS_NONE);
//			Gdk::Rectangle r = calcRectangle(false);
//			//cr->set_source_rgba(0.5, 0.7, 0.7, 0.4);
//			//cr->rectangle(r.get_x(), r.get_y(), r.get_width(), r.get_height());
//			//cr->fill();
//			//cr->set_line_width(1);
//			//cr->set_source_rgba(0.2, 0.2, 0.2, 0.4);
//			//cr->rectangle(r.get_x(), r.get_y(), r.get_width(), r.get_height());
//			//cr->stroke();
//			gdk_draw_rectangle(event->window, gc, true, r.get_x(), r.get_y(), r.get_width(), r.get_height());
//			break;
//		}
//		default:;
//	}
//
//	gdk_flush();
//	gdk_gc_unref(gc);

	return true;
}

void WidgetEditor::invalidate() {
	get_window()->invalidate(false);
}

void WidgetEditor::setSelectionChange(bool value) {
	for(WidgetsList::iterator w = wlist.begin(); w != wlist.end(); w++)
		if((*w)->select)
			(*w)->change = value;
}

void  WidgetEditor::moveSelection(int dx, int dy) {
	for(WidgetsList::iterator w = wlist.begin(); w != wlist.end(); w++)
		if((*w)->select) {
			(*w)->move(dx, dy);
		}
}

void  WidgetEditor::sizeSelection(int dx, int dy) {
	for(WidgetsList::iterator w = wlist.begin(); w != wlist.end(); w++)
		if((*w)->select) {
			(*w)->size(dx, dy);
		}
}

WidgetInfo *WidgetEditor::getWidgetAtPos(gdouble x, gdouble y) {
	for(WidgetsList::reverse_iterator w = wlist.rbegin(); w != wlist.rend(); w++)
		if((*w)->checkAtPos(x, y))
			return *w;
	return NULL;
}

void WidgetEditor::clearSelection() {
	for(WidgetsList::iterator w = wlist.begin(); w != wlist.end(); w++)
		(*w)->select = false;
}

Gdk::Rectangle WidgetEditor::getSelectionRect() {
	Gdk::Rectangle r(0,0,0,0);
	for(WidgetsList::iterator w = wlist.begin(); w != wlist.end(); w++)
		if((*w)->select) {
			if(r.has_zero_area())
				r = Gdk::Rectangle((*w)->x - 4, (*w)->y - 4, (*w)->width + 8, (*w)->height + 8);
			else
				r.join(Gdk::Rectangle((*w)->x - 4, (*w)->y - 4, (*w)->width + 8, (*w)->height + 8));
		}

	return r;
}

int WidgetEditor::getSizer(int x, int y) {
	Gdk::Rectangle r(0,0,0,0);
	for(WidgetsList::iterator w = wlist.begin(); w != wlist.end(); w++)
		if((*w)->select) {
			if(r.has_zero_area())
				r = (*w)->getRect();
			else
				r.join((*w)->getRect());
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

void WidgetEditor::callback(void *owner, CallbackType type, const void *data) {
	switch(type) {
		case CBT_SDK_CHANGE_SELECT:
			clearSelection();
			for(ElementsList::iterator e = sdk->selMan->elements.begin(); e != sdk->selMan->elements.end(); e++) {
				for(WidgetsList::iterator w = wlist.begin(); w != wlist.end(); w++) {
					if((*w)->tpl == *e) {
						(*w)->select = true;
						break;
					}
				}
			}
			invalidate();
			break;
		case CBT_SDK_REMOVE_ELEMENT:
			for(WidgetsList::iterator w = wlist.begin(); w != wlist.end(); w++) {
				if((*w)->tpl == data) {
					delete *w;
					wlist.remove(*w);
					invalidate();
					break;
				}
			}
			break;
		default: ;
	}
}

Gdk::Rectangle WidgetEditor::calcRectangle(bool forDraw) {
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

void WidgetEditor::selectWigdetsFromRect() {
	int x1 = min(start_x, cur_x);
	int y1 = min(start_y, cur_y);
	int x2 = max(start_x, cur_x);
	int y2 = max(start_y, cur_y);
	for(WidgetsList::iterator w = wlist.begin(); w != wlist.end(); w++) {
		if((*w)->x > x2 || (*w)->y > y2 || ((*w)->x + (*w)->width) < x1 || ((*w)->y + (*w)->height) < y1)
			; // no
		else
			sdk->selMan->add((*w)->tpl);
	}
}

bool WidgetEditor::on_button_press_event(GdkEventButton* event) {
	int sz = getSizer(event->x, event->y);
	WidgetInfo *w = getWidgetAtPos(event->x, event->y);
	if(sz != -1) {
		selSizer = sz;
		state = WE_MS_SIZER;
	}
	else if(w) {
		if(w->select) {
			setSelectionChange(true);
		}
		else {
			w->change = true;
			sdk->selMan->select(w->tpl);
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

	return true;
}

bool WidgetEditor::on_button_release_event(GdkEventButton* event) {
	switch(state) {
		case WE_MS_MOVE:
			setSelectionChange(false);
			break;
		case WE_MS_RECT:
			selectWigdetsFromRect();
			break;
		default:;
	}
	invalidate();
	state = WE_MS_NONE;
	return true;
}

bool WidgetEditor::on_motion_notify_event(GdkEventMotion* event) {
	switch(state) {
		case WE_MS_NONE: {
			int sz = getSizer(event->x, event->y);
			switch(sz) {
				case 0: get_window()->set_cursor(curList->cursor(CURSOR_TL_CORNER)); break;
				case 5:
				case 1: get_window()->set_cursor(curList->cursor(CURSOR_V_DOUBLE)); break;
				case 2: get_window()->set_cursor(curList->cursor(CURSOR_TR_CORNER)); break;
				case 7:
				case 3: get_window()->set_cursor(curList->cursor(CURSOR_H_DOUBLE)); break;
				case 4: get_window()->set_cursor(curList->cursor(CURSOR_BR_CORNER)); break;
				case 6: get_window()->set_cursor(curList->cursor(CURSOR_BL_CORNER)); break;
				default: get_window()->set_cursor(curList->cursor(CURSOR_DEFAULT)); break;
			}
			break;
		}
		case WE_MS_MOVE: {
			int dx = (int)((event->x - start_x) / 5)*5;
			int dy = (int)((event->y - start_y) / 5)*5;
			if(abs(dx) >= 5 || abs(dy) >= 5) {
				Gdk::Rectangle r = getSelectionRect();
				moveSelection(dx, dy);
				start_x += dx;
				start_y += dy;
				r.join(getSelectionRect());
				get_window()->invalidate_rect(r, false);
			}
			break;
		}
		case WE_MS_RECT: {
			Gdk::Rectangle r = calcRectangle();
			cur_x = event->x;
			cur_y = event->y;
			r.join(calcRectangle());
			get_window()->invalidate_rect(r, false);
			break;
		}
		case WE_MS_SIZER: {
			Gdk::Rectangle r = getSelectionRect();
			int dx = (int)((event->x - start_x) / 5)*5;
			int dy = (int)((event->y - start_y) / 5)*5;
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
			r.join(getSelectionRect());
			get_window()->invalidate_rect(r, false);
			break;
		}
		default: ;
	}
	return false;
}

//-----------------------------------------------------------------------------------------------

WidgetEditorEx::WidgetEditorEx(SDK *sdk) {
	set_events(Gdk::BUTTON_PRESS_MASK|Gdk::POINTER_MOTION_MASK|Gdk::BUTTON_RELEASE_MASK);
	set_above_child(true);
	this->sdk = sdk;
	sdk->on_remove_element += this;
	state = WE_MS_NONE;

	ElementTplWinControl *main = NULL;
	for(ElementsList::iterator e = sdk->elements.begin(); e != sdk->elements.end(); e++)
		if((*e)->isParent() && (*e)->isCore()) {
			main = dynamic_cast<ElementTplWinControl*>(*e);
			break;
		}

	Element *parent = NULL;
	asParent = false;
	if(main) {
		if(main->tpl->conf->editClassName == "Form") {
			parent = main;
			main->ctl = this;
			asParent = true;
			//ctrls.insert(std::pair<Widget*, Element*>(this, main));
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
			dynamic_cast<ElementCore *>(*e)->prepareForRun(&parent);
			ctrls.insert(std::pair<Widget*, Element*>(ew->ctl, *e));
			ew->on_property_change += this;
		}
	}

	if(!asParent)
		add(*main->ctl);

	unBuffer(this);
}

WidgetEditorEx::~WidgetEditorEx() {
	sdk->on_remove_element -= this;

	for(WEItems::iterator w = ctrls.begin(); w != ctrls.end(); w++)
		if(w->second->sdk)
			w->second->getParentElement()->on_property_change -= this;
		else
			w->second->on_property_change -= this;
}

void WidgetEditorEx::unBuffer(Widget *ctl) {
	if(!dynamic_cast<TextView*>(ctl) && !dynamic_cast<TreeView*>(ctl))
		ctl->set_double_buffered(false);
	Container *c = dynamic_cast<Container*>(ctl);
	if(!c) return;

	Glib::ListHandle<Widget*> ch = c->get_children();
	for(Glib::ListHandle<Widget*>::iterator c = ch.begin(); c != ch.end(); c++)
		unBuffer(*c); //->set_double_buffered(false);
}

Element *WidgetEditorEx::getElementAtPos(gdouble x, gdouble y) {
	Element *er = NULL;
	Container *prn = asParent ? this : (Container*)(*get_children().begin());
	Glib::ListHandle<Widget*> ch = prn->get_children();
	for(Glib::ListHandle<Widget*>::iterator c = ch.begin(); c != ch.end(); c++) {
		Allocation a = (*c)->get_allocation();
		if(a.get_x() <= x && a.get_y() <= y && (a.get_x() + a.get_width()) >= x && (a.get_y() + a.get_height()) >= y) {
			er = ctrls[*c];
		}
	}
	return er;
}

#include "main_gui.h"

int WidgetEditorEx::getSizer(int x, int y) {
	Gdk::Rectangle r(0,0,0,0);
	for(WEItems::iterator w = ctrls.begin(); w != ctrls.end(); w++)
		if(w->second->isSelect()) {
			if(r.has_zero_area())
				r = w->first->get_allocation();
			else
				r.join(w->first->get_allocation());
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

void  WidgetEditorEx::moveSelection(int dx, int dy) {
	for(WEItems::iterator w = ctrls.begin(); w != ctrls.end(); w++)
		if(w->second->isSelect()) {
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
		if(w->second->isSelect()) {
			//changeProperty = true;
			Element *e = w->second->sdk ? w->second->getParentElement() : w->second;
			int v = e->props[2]->readInt() + dx;
			e->props[2]->setValue(&v);
			v = e->props[3]->readInt() + dy;
			e->props[3]->setValue(&v);
			//changeProperty = false;
		}
}

Gdk::Rectangle WidgetEditorEx::getSelectionRect() {
	Gdk::Rectangle r(0,0,0,0);
	for(WEItems::iterator w = ctrls.begin(); w != ctrls.end(); w++)
		if(w->second->isSelect()) {
			if(r.has_zero_area())
				r = w->first->get_allocation();
			else
				r.join(w->first->get_allocation());
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
		Allocation a = w->first->get_allocation();
		if(a.get_x() > x2 || a.get_y() > y2 || (a.get_x() + a.get_width()) < x1 || (a.get_y() + a.get_height()) < y1)
			; // no
		else
			sdk->selMan->add(w->second);
	}
}

bool WidgetEditorEx::on_button_press_event(GdkEventButton* event)  {
//	sdk->selMan->clear();
	Element *e = getElementAtPos(event->x, event->y);
//	if(e)
//		sdk->selMan->add(e);

	int sz = getSizer(event->x, event->y);
	if(sz != -1) {
		selSizer = sz;
		state = WE_MS_SIZER;
	}
	else if(e) {
		if(e->isSelect()) {
			//setSelectionChange(true);
		}
		else {
			sdk->selMan->select(e);
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

	invalidate();

	return true;
}

bool WidgetEditorEx::on_motion_notify_event(GdkEventMotion* event) {
	switch(state) {
		case WE_MS_NONE: {
			int sz = getSizer(event->x, event->y);
			switch(sz) {
				case 0: get_window()->set_cursor(curList->cursor(CURSOR_TL_CORNER)); break;
				case 5:
				case 1: get_window()->set_cursor(curList->cursor(CURSOR_V_DOUBLE)); break;
				case 2: get_window()->set_cursor(curList->cursor(CURSOR_TR_CORNER)); break;
				case 7:
				case 3: get_window()->set_cursor(curList->cursor(CURSOR_H_DOUBLE)); break;
				case 4: get_window()->set_cursor(curList->cursor(CURSOR_BR_CORNER)); break;
				case 6: get_window()->set_cursor(curList->cursor(CURSOR_BL_CORNER)); break;
				default: get_window()->set_cursor(curList->cursor(CURSOR_DEFAULT)); break;
			}
			break;
		}
		case WE_MS_MOVE: {
			int dx = (int)((event->x - start_x) / 5)*5;
			int dy = (int)((event->y - start_y) / 5)*5;
			if(abs(dx) >= 5 || abs(dy) >= 5) {
				moveSelection(dx, dy);
				start_x += dx;
				start_y += dy;
			}
			break;
		}
		case WE_MS_RECT: {
			Gdk::Rectangle r = calcRectangle();
			cur_x = event->x;
			cur_y = event->y;
			r.join(calcRectangle());
			get_window()->invalidate_rect(r, true);
			break;
		}
		case WE_MS_SIZER: {
			Gdk::Rectangle r = getSelectionRect();
			int dx = (int)((event->x - start_x) / 5)*5;
			int dy = (int)((event->y - start_y) / 5)*5;
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
			r.join(getSelectionRect());
			r = Gdk::Rectangle(r.get_x() - 4, r.get_y() - 4, r.get_width() + 8, r.get_height() + 8);
			get_window()->invalidate_rect(r, true);
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

	for(WEItems::iterator e = ctrls.begin(); e != ctrls.end(); e++) {
		if(e->second->isSelect()) {
			Allocation a = e->first->get_allocation();
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

void WidgetEditorEx::changeProperty(ElementProperty *prop) {
	Element *e = prop->parent;
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
			//get_window()->invalidate_rect(r, true);
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
	else if(e->tpl->conf->editClassName == "FileChooserButton") {
		if(prop->name == "Action")
			((FileChooserButton *)w)->set_action((FileChooserAction)(prop->readInt()+FILE_CHOOSER_ACTION_OPEN));
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

		((Button*)w)->set_image_position((PositionType)(e->props[11]->readInt() + POS_LEFT));

		if(e->props[9]->isDefault()) {
			Image *img = new Image();
			((Button*)w)->set_image(*img);
		}
		else {
			BuiltinStockID sid = {e->props[9]->readStr().c_str()};
			Image *img = new Image(sid, (BuiltinIconSize)(e->props[10]->readInt() + ICON_SIZE_MENU));
			((Button*)w)->set_image(*img);
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
			img->set(sid, (BuiltinIconSize)(e->props[11]->readInt() + ICON_SIZE_MENU));
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
					delete e->first;
					ctrls.erase(e);
					invalidate();
					break;
				}
			}
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

	we = new WidgetEditorEx(sde->csdk);
	sde->get_parent()->add(*we);
	we->show_all();

	sde->hide();

	return;

	weditor = new WidgetEditor(sde->csdk);

	for(ElementsList::iterator e = sde->csdk->elements.begin(); e != sde->csdk->elements.end(); e++)
		if((*e)->isParent()) {
			// do nothing
			break;
		}
	for(ElementsList::iterator e = sde->csdk->elements.begin(); e != sde->csdk->elements.end(); e++)
		if(!(*e)->isParent() && !(*e)->tpl->conf->editClassName.empty() && ((*e)->tpl->conf->editClassName != "EditMulti" || !(*e)->tpl->conf->sub.empty())) {
			weditor->addWidget(*e);
		}
	sde->get_parent()->add(*weditor);
	weditor->show();

	weditor->callback(sde, CBT_SDK_CHANGE_SELECT, sde->csdk->selMan);

	sde->hide();
}

void FE_Internal::stop() {
	//delete weditor;
	delete we;
	sde->show();
}
