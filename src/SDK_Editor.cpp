/*
 * SDK_Editor.cpp
 *
 *  Created on: 01.05.2010
 *      Author: dilma
 */

#include <gtkmm-3.0/gtkmm/stylecontext.h>

#include "SDK_Editor.h"

#ifdef OPENGL_SDE
#include "glitz.h"
#endif

CursorList *curList = NULL;

SDK_Editor::SDK_Editor():DrawingArea(),
	on_begin_operation(this, CBT_BEGIN_OPERATION),
	on_end_operation(this, CBT_END_OPERATION),
	on_popup_menu(this, CBT_POPUP_MENU),
	on_key_down(this, CBT_KEY_DOWN),
	on_change_sdk(this, CBT_CHANGE_SDK),
	on_change_scheme(this, CBT_CHANGE_SCHEME)
#ifdef OPENGL_SDE
	//,cairo_surface(new Cairo::Surface::Surface(gl_create_surface(get_window()->gobj())))
#endif
{
	set_events(Gdk::BUTTON_PRESS_MASK|Gdk::POINTER_MOTION_MASK|Gdk::BUTTON_RELEASE_MASK);
	set_size_request(500, 100);
	set_can_focus(true);

#ifdef GL_SDK
	Glib::RefPtr<Gdk::GL::Config> glconfig;

	// Try double-buffered visual
	int major, minor;
	Gdk::GL::query_version(major, minor);
	std::cout << "OpenGL extension version - " << major << "." << minor << std::endl;

	glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB|Gdk::GL::MODE_DEPTH|Gdk::GL::MODE_DOUBLE);
	if (glconfig == NULL) {
		std::cerr << "*** Cannot find the double-buffered visual.\n"
				<< "*** Trying single-buffered visual.\n";

		// Try single-buffered visual
		glconfig = Gdk::GL::Config::create(Gdk::GL::MODE_RGB   |
										 Gdk::GL::MODE_DEPTH);
		if (glconfig == NULL) {
			std::cerr << "*** Cannot find any OpenGL-capable visual.\n";
			exit(1);
		}
	}
	set_gl_capability(glconfig);
#endif

	memset(&mouse, 0, sizeof(mouse));

	zoom = 1.0;
//	cursor = NULL;

	sdk = new MSDK();
	csdk = sdk;
	csdk->on_redraw_rect += this;
	csdk->on_add_element += this;
	csdk->on_remove_element += this;

	oldObj = NULL;

#ifdef OPENGL_SDE
//	cairo_contex = Cairo::Context::create(cairo_surface);
//	if(cairo_contex)
//		DEBUG_MSG("Cairo context created!")
#endif

	memset(&actionMap, 0, sizeof(actionMap));
	ActionMap *a = &actionMap[MS_NO_OPERATION];
	a->downProc = &SDK_Editor::noOperation_DownProc;
	a->moveProc = &SDK_Editor::noOperation_MoveProc;
	a->upProc = &SDK_Editor::noOperation_UpProc;
	a->cursorProc = &SDK_Editor::noOperation_cursorProc;
	a->hintMask = HINT_ALL;

	a = &actionMap[MS_INSERT_ELEMENT];
	a->beginProc = &SDK_Editor::insertElement_BeginProc;
	a->downProc = &SDK_Editor::insertElement_DownProc;
	a->moveProc = &SDK_Editor::insertElement_MoveProc;
	a->upProc = &SDK_Editor::insertElement_UpProc;
	a->drawProc = &SDK_Editor::insertElement_DrawProc;
	a->cursorProc = &SDK_Editor::insertElement_cursorProc;
	a->hintMask = HINT_POINT;

	a = &actionMap[MS_FOCUS_RECT];
	a->downProc = &SDK_Editor::focusRect_DownProc;
	a->moveProc = &SDK_Editor::focusRect_MoveProc;
	a->upProc = &SDK_Editor::focusRect_UpProc;
	a->drawProc = &SDK_Editor::focusRect_DrawProc;
	a->hintMask = HINT_NONE;

	a = &actionMap[MS_MOVE_ELEMENT];
	a->downProc = &SDK_Editor::moveElement_DownProc;
	a->moveProc = &SDK_Editor::moveElement_MoveProc;
	a->upProc = &SDK_Editor::moveElement_UpProc;
	a->hintMask = HINT_NONE;

	a = &actionMap[MS_POINT_LINK];
	a->downProc = &SDK_Editor::pointLink_DownProc;
	a->moveProc = &SDK_Editor::pointLink_MoveProc;
	a->upProc = &SDK_Editor::pointLink_UpProc;
	a->drawProc = &SDK_Editor::pointLink_DrawProc;
	a->cursorProc = &SDK_Editor::pointLink_cursorProc;
	a->hintMask = HINT_POINT|HINT_LINK|HINT_ELEMENT;

	a = &actionMap[MS_MOVE_LINE_POINT];
	a->downProc = &SDK_Editor::moveLinePoint_DownProc;
	a->moveProc = &SDK_Editor::moveLinePoint_MoveProc;
	a->upProc = &SDK_Editor::moveLinePoint_UpProc;
	a->cursorProc = &SDK_Editor::moveLinePoint_cursorProc;
	a->hintMask = HINT_NONE;

	a = &actionMap[MS_LINE];
	a->downProc = &SDK_Editor::line_DownProc;
	a->hintMask = HINT_NONE;

	a = &actionMap[MS_MOUSE_ELEMENT];
	a->moveProc = &SDK_Editor::mouseElement_MoveProc;
	a->upProc = &SDK_Editor::mouseElement_UpProc;
	a->cursorProc = &SDK_Editor::mouseElement_cursorProc;
	a->hintMask = HINT_NONE;

	a = &actionMap[MS_SELECT_BY_EDGE];
	a->moveProc = &SDK_Editor::selectByEdge_MoveProc;
	a->upProc = &SDK_Editor::selectByEdge_UpProc;
	a->downProc = &SDK_Editor::selectByEdge_DownProc;
	a->drawProc = &SDK_Editor::selectByEdge_DrawProc;
	a->cursorProc = &SDK_Editor::selectByEdge_cursorProc;
	a->hintMask = HINT_NONE;

	a = &actionMap[MS_SCROLL];
	a->downProc = &SDK_Editor::scroll_DownProc;
	a->moveProc = &SDK_Editor::scroll_MoveProc;
	a->upProc = &SDK_Editor::scroll_UpProc;
	a->hintMask = HINT_NONE;

	a = &actionMap[MS_MOVE_LINK_HINT];
	a->downProc = &SDK_Editor::linkHint_DownProc;
	a->moveProc = &SDK_Editor::linkHint_MoveProc;
	a->upProc = &SDK_Editor::linkHint_UpProc;
	a->cursorProc = &SDK_Editor::linkHint_cursorProc;
	a->hintMask = HINT_NONE;

	a = &actionMap[MS_ADD_LINK_HINT];
	a->downProc = &SDK_Editor::lh_add_DownProc;
	a->cursorProc = &SDK_Editor::lh_add_cursorProc;
	a->hintMask = HINT_NONE;

	a = &actionMap[MS_REMOVE_LINK_HINT];
	a->downProc = &SDK_Editor::lh_remove_DownProc;
	a->cursorProc = &SDK_Editor::lh_remove_cursorProc;
	a->hintMask = HINT_NONE;
}

SDK_Editor::~SDK_Editor() {
	TRACE_PROC
	csdk->on_add_element -= this;
	csdk->on_remove_element -= this;
	delete sdk;

#ifdef OPENGL_SDE
	// TODO destroy surface
#endif
}

bool SDK_Editor::on_key_press_event(GdkEventKey *key) {
	on_key_down.run(key);

	return true;
}

void SDK_Editor::beginOperation(MouseOperation op, const void *data) {
	mouse.cur_operation = op;
	mouse.data = data;

	if(actionMap[op].beginProc)
		(this->*actionMap[op].beginProc)(data);

	on_begin_operation.run(&mouse);
}

void SDK_Editor::endOperation() {
	MouseOperation op = mouse.cur_operation;

	memset(&mouse.obj, 0, sizeof(mouse.obj));
	memset(&mouse.select_obj, 0, sizeof(mouse.select_obj));
	mouse.cur_operation = MS_NO_OPERATION;

	if(actionMap[op].endProc)
		(this->*actionMap[op].endProc)();

	on_end_operation.run(&mouse);
}

bool SDK_Editor::on_button_press_event(GdkEventButton* event) {
	if(event->button == BTN_MIDDLE) {
		beginOperation(MS_SCROLL);
		return false;
	}
	grab_focus();

	// ------------------------------------- hints operations
	if(!lobj.empty()) {
		for(LayerObjects::iterator lo = lobj.begin(); lo != lobj.end(); lo++)
			if(!(*lo)->downProc(event->x, event->y, event->button)) {
				// TODO begin object hint operation
				return true;
			}
		Gdk::Rectangle r;
		for(LayerObjects::iterator lo = lobj.begin(); lo != lobj.end(); lo++) {
			r.join((*lo)->getRect());
			delete *lo;
		}
		lobj.clear();
		oldObj = NULL;
		reDrawRect(r);
	}

	// ------------------------------------- MultiElement operations
	if(event->button == 9) {
		Element *e = csdk->selMan->empty() ? NULL : csdk->selMan->getFirst();
		if(e && e->sdk)
			enterSDK(e);
		return true;
	}
	else if(event->button == 8) {
		if(csdk->parent)
			leaveSDK();
		return true;
	}
	// --------------------------------------------------------------

	mouse.curx = event->x / zoom;
	mouse.cury = event->y / zoom;
	mouse.startx = mouse.curx;
	mouse.starty = mouse.cury;

	csdk->getObjectAtPos(mouse.curx, mouse.cury, &mouse.obj);

	mouse.begin_type = mouse.obj.type;
	mouse.begin_operation = mouse.cur_operation;

	if(actionMap[mouse.cur_operation].downProc)
		(this->*actionMap[mouse.cur_operation].downProc)(mouse.curx, mouse.cury, event->button, event->state);

	changeCursor();
	
	return true;
}

bool SDK_Editor::on_motion_notify_event(GdkEventMotion* event) {
	if(mouse.cur_operation == MS_SCROLL)
		return false;

	gdouble x = event->x / zoom;
	gdouble y = event->y / zoom;

	ObjectType old_obj = mouse.select_obj;
	csdk->getObjectAtPos(x,y, &mouse.select_obj);

	if(actionMap[mouse.cur_operation].moveProc)
		(this->*actionMap[mouse.cur_operation].moveProc)(x, y);

	if(old_obj != mouse.select_obj)
		showObjectHint(event->x, event->y);

	changeCursor();

	return true;
}

bool SDK_Editor::on_button_release_event(GdkEventButton* event) {
	if(mouse.cur_operation == MS_SCROLL) {
		endOperation();
		return false;
	}
	gdouble x = event->x / zoom;
	gdouble y = event->y / zoom;

	mouse.time = event->time;

	if(actionMap[mouse.cur_operation].upProc)
		(this->*actionMap[mouse.cur_operation].upProc)(x, y, event->button, event->state);

	csdk->getObjectAtPos(x,y, &mouse.select_obj);

	changeCursor();

	if(event->button == BTN_RIGHT && mouse.begin_type == mouse.select_obj.type && mouse.begin_operation == mouse.cur_operation) {
		int pi[] = {mouse.select_obj.type, (int)event->time };
		on_popup_menu.run(&pi);
	}

	setGeometry();

	return true;
}

bool SDK_Editor::on_draw (const ::Cairo::RefPtr< ::Cairo::Context >& cr) {
//	GTimer *t = g_timer_new();

#ifdef GL_SDK
	  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();

	  // *** OpenGL BEGIN ***
	  if (!glwindow->gl_begin(get_gl_context()))
	    return false;

	  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	  glwindow->draw_cube(true, 0.5);

		glColor3f(0.5, 0.5, 0.5);
		glBegin(GL_LINE_LOOP);
			glVertex2d(0, 0);
			glVertex2d(0.5, 0);
			glVertex2d(0.5, 0.5);
		glEnd();
#else

#ifdef OPENGL_SDE
	DrawContext &cr = cairo_contex;
	cr->set_source_rgb(1, 1, 1);
	cr->rectangle(0, 0, 800, 600);
	cr->fill();
#else
	Gdk::Cairo::set_source_rgba(cr, get_style_context()->get_background_color(STATE_FLAG_NORMAL));
	cr->rectangle(0, 0, get_width(), get_height());
	cr->fill();
#endif

#endif

	// zoom scale
	if(zoom != 1.0) {
		cr->save();
		cr->scale(zoom, zoom);
	}

	if(zoom > 0.7) {
		cr->set_source_rgb(0.0, 0.0, 0.0);
		gdouble zx = 10 * (int)(0/zoom/10);
		gdouble zy = 10 * (int)(0/zoom/10);
		for(int j = 0; j < get_height()/zoom + 10; j += 10)
			for(int i = 0; i < get_width()/zoom + 10; i += 10)
				cr->rectangle(zx + i, zy + j, 1, 1);
		cr->fill();
	}

	// antialias
	cr->set_antialias((zoom == 1.0) ? Cairo::ANTIALIAS_NONE : Cairo::ANTIALIAS_DEFAULT);

	csdk->draw(cr, zoom);

	if(zoom != 1.0)
		cr->restore();

	if(actionMap[mouse.cur_operation].drawProc)
		(this->*actionMap[mouse.cur_operation].drawProc)(cr);

	for(LayerObjects::iterator lo = lobj.begin(); lo != lobj.end(); lo++)
		if((*lo)->visible)
			(*lo)->draw(cr);

#ifdef GL_SDK
	  // Swap buffers.
	  if (glwindow->is_double_buffered())
	    glwindow->swap_buffers();
	  else {
	    glFlush();
	  }

	  glwindow->gl_end();
	  // *** OpenGL END ***
#endif


#ifdef OPENGL_SDE
	//DrawContext dc = get_window()->create_cairo_context();
	//dc->set_source(cairo_surface, 0, 0);
	//dc->paint();
	gl_swap();
#endif

	return false;
}

bool SDK_Editor::on_expose_event(GdkEventExpose *event) {
//	GTimer *t = g_timer_new();

#ifdef GL_SDK
	  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();

	  // *** OpenGL BEGIN ***
	  if (!glwindow->gl_begin(get_gl_context()))
	    return false;

	  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	  glwindow->draw_cube(true, 0.5);

		glColor3f(0.5, 0.5, 0.5);
		glBegin(GL_LINE_LOOP);
			glVertex2d(0, 0);
			glVertex2d(0.5, 0);
			glVertex2d(0.5, 0.5);
		glEnd();
#else

#ifdef OPENGL_SDE
	DrawContext &cr = cairo_contex;
	cr->set_source_rgb(1, 1, 1);
	cr->rectangle(0, 0, 800, 600);
	cr->fill();
#else
	DrawContext cr = get_window()->create_cairo_context();
//	Gdk::Cairo::set_source_rgba(cr, get_style()->get_base(Gtk::STATE_NORMAL));
	cr->rectangle(event->area.x, event->area.y, event->area.width, event->area.height);
	cr->fill();
#endif

#endif

	// zoom scale
	if(zoom != 1.0) {
		cr->save();
		cr->scale(zoom, zoom);
	}

	if(zoom > 0.7) {
		cr->set_source_rgb(0.0, 0.0, 0.0);
		gdouble zx = 10 * (int)(event->area.x/zoom/10);
		gdouble zy = 10 * (int)(event->area.y/zoom/10);
		for(int j = 0; j < event->area.height/zoom + 10; j += 10)
			for(int i = 0; i < event->area.width/zoom + 10; i += 10)
				cr->rectangle(zx + i, zy + j, 1, 1);
		cr->fill();
	}

	// antialias
	cr->set_antialias((zoom == 1.0) ? Cairo::ANTIALIAS_NONE : Cairo::ANTIALIAS_DEFAULT);

	csdk->draw(cr, zoom);

	if(zoom != 1.0)
		cr->restore();

	if(actionMap[mouse.cur_operation].drawProc)
		(this->*actionMap[mouse.cur_operation].drawProc)(cr);

	for(LayerObjects::iterator lo = lobj.begin(); lo != lobj.end(); lo++)
		if((*lo)->visible)
			(*lo)->draw(cr);

#ifdef GL_SDK
	  // Swap buffers.
	  if (glwindow->is_double_buffered())
	    glwindow->swap_buffers();
	  else {
	    glFlush();
	  }

	  glwindow->gl_end();
	  // *** OpenGL END ***
#endif


#ifdef OPENGL_SDE
	//DrawContext dc = get_window()->create_cairo_context();
	//dc->set_source(cairo_surface, 0, 0);
	//dc->paint();
	gl_swap();
#endif

	return false;
 }

void SDK_Editor::on_realize() {
	DrawingArea::on_realize();

#ifdef OPENGL_SDE
	set_double_buffered(false);

	Cairo::RefPtr<Cairo::Surface> cs(new Cairo::Surface(gl_create_surface((GtkWidget*)gobj())));
#endif
//	Cairo::RefPtr<Cairo::Surface> cs(new Cairo::Surface(xlib_create_surface((GtkWidget*)gobj())));
	//cairo_surface = cs;

//	cairo_contex = Cairo::Context::create(cs);
//	if(cairo_contex)
//		DEBUG_MSG("Cairo context created!")
}

#ifdef GL_SDK
  // We need to call the base on_realize()
  Gtk::GL::DrawingArea::on_realize();

  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();

  // *** OpenGL BEGIN ***
  if (!glwindow->gl_begin(get_gl_context()))
    return;

  glClearColor(1, 1, 1, 0);
  glEnable(GL_DEPTH_TEST);
  glEnable(GL_NORMALIZE);
  glEnable(GL_DEPTH_TEST);
  glDepthFunc(GL_LESS);

  glwindow->gl_end();
  // *** OpenGL END ***
}

void SDK_Editor::on_idle() {
	DEBUG_MSG("idle...")
}

bool SDK_Editor::on_configure_event(GdkEventConfigure* event) {
  Glib::RefPtr<Gdk::GL::Window> glwindow = get_gl_window();

  // *** OpenGL BEGIN ***
  if (!glwindow->gl_begin(get_gl_context()))
    return false;

  glViewport(0, 0, get_width(), get_height());

  glwindow->gl_end();
  // *** OpenGL END ***

  return true;
}
#endif

void SDK_Editor::showObjectHint(gdouble x, gdouble y) {
	if(oldObj) {
		lobj.remove(oldObj);
		delete oldObj;
		oldObj = NULL;
		reDraw();
	}
	unselectPoints();
	switch(mouse.select_obj.type) {
		case SDK_OBJ_POINT:
			if(actionMap[mouse.cur_operation].hintMask & HINT_POINT) {
				lobj.push_back(oldObj = new LO_PointHint(mouse.select_obj.point, zoom, this));
				oldObj->show(x, y, true);
				selectPoint(mouse.select_obj.point);
			}
			break;
		case SDK_OBJ_ELEMENT:
			if(actionMap[mouse.cur_operation].hintMask & HINT_ELEMENT) {
				lobj.push_back(oldObj = new LO_ElementHint(mouse.select_obj.element, zoom, this));
				oldObj->show(x, y, true);
			}
			break;
		default:;
	}
}

void SDK_Editor::invalidateRect(gdouble x1, gdouble y1, gdouble x2, gdouble y2) {
	if(x1 > x2) {
		int t = x1;
		x1 = x2;
		x2 = t;
	}
	if(y1 > y2) {
		int t = y1;
		y1 = y2;
		y2 = t;
	}
	Gdk::Rectangle r(x1 - 3, y1 - 3, x2 - x1 + 6, y2 - y1 + 6);
	get_window()->invalidate_rect(r, false);
}

void SDK_Editor::invalidateRealRect(gdouble x1, gdouble y1, gdouble x2, gdouble y2) {
	invalidateRect((int)(x1*zoom), (int)(y1*zoom), (int)(x2*zoom), (int)(y2*zoom));
}

void SDK_Editor::reDraw() {
	if(Glib::RefPtr<Gdk::Window> w = get_window()) w->invalidate(false);
}

void SDK_Editor::reDrawRect(const Gdk::Rectangle &r) {
	if(Glib::RefPtr<Gdk::Window> w = get_window()) w->invalidate_rect(r, false);
}

void SDK_Editor::callback(void *owner, CallbackType type, const void *data) {
	switch(type) {
		case CBT_SDK_REDRAW_RECT:
			if(data) {
				if(zoom == 1.0)
					reDrawRect(*(Gdk::Rectangle *)data);
				else {
					Gdk::Rectangle r(*(Gdk::Rectangle *)data);
					r.set_x((int)(r.get_x() * zoom) - 2);
					r.set_y((int)(r.get_y() * zoom) - 2);
					r.set_width((int)(r.get_width() * zoom) + 4);
					r.set_height((int)(r.get_height() * zoom) + 4);
					reDrawRect(r);
				}
			}
			else
				reDraw();
			break;
		case CBT_SDK_ADD_ELEMENT:
		case CBT_SDK_REMOVE_ELEMENT:
			changeScheme();
			break;
		default: ;
	}
}

bool SDK_Editor::load(const ustring& file_name) {
	bool res = sdk->loadFromFile(file_name);
	setGeometry();
	return res;
}

void SDK_Editor::setGeometry() {
	gdouble min_x, min_y, max_x, max_y;

	csdk->getSchemeGeometry(min_x, min_y, max_x, max_y);
	set_size_request((int)(max_x*zoom + 100), (int)(max_y*zoom + 100));
}

bool SDK_Editor::saveToFile(const ustring &file_name) {
	return sdk->saveToFile(file_name);
}

void SDK_Editor::enterSDK(Element *element) {
	csdk->on_redraw_rect -= this;
	csdk->on_add_element -= this;
	csdk->on_remove_element -= this;

	ChangeSDKInfo csi(csdk);
	on_change_sdk.run(&csi);

	csi.setSDK(csdk = element->sdk);

	csdk->on_redraw_rect += this;
	csdk->on_add_element += this;
	csdk->on_remove_element += this;

	reDraw();
	setGeometry();
	on_change_sdk.run(&csi);
}

void SDK_Editor::leaveSDK() {
	csdk->on_redraw_rect -= this;
	csdk->on_add_element -= this;
	csdk->on_remove_element -= this;

	ChangeSDKInfo csi(csdk);
	on_change_sdk.run(&csi);

	csi.setSDK(csdk = csdk->parent->parent);

	csdk->on_redraw_rect += this;
	csdk->on_add_element += this;
	csdk->on_remove_element += this;

	reDraw();
	setGeometry();
	on_change_sdk.run(&csi);
}

void SDK_Editor::selectPoint(ElementPoint *point) {
	if(point->isSelect()) return;

	selPoints.push_back(point);
	point->flag |= POINT_FLG_IS_SELECT;
	point->invalidate();
}

void SDK_Editor::unselectPoints() {
	if(selPoints.empty()) return;

	for(ElementPoints::iterator p = selPoints.begin(); p != selPoints.end(); p++)
		for(ElementsList::iterator e = csdk->elements.begin(); e != csdk->elements.end(); e++)
			if((*e)->points.indexOf(*p) != -1) {
				(*p)->flag ^= POINT_FLG_IS_SELECT;
				(*p)->invalidate();
			}
	selPoints.clear();
}

bool SDK_Editor::pasteElementOnLine(const ustring &name) {
	Element *e = csdk->add(name, GRID_X(mouse.startx), GRID_Y(mouse.starty));
	e->place(mouse.startx, mouse.starty);
	if(e) csdk->selMan->select(e);

	if(mouse.obj.type == SDK_OBJ_LINE) {
		e->insertInLine(mouse.obj.point, mouse.obj.pos);
		return true;
	}

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
//                                         mouse operation
///////////////////////////////////////////////////////////////////////////////////////////////////////

void SDK_Editor::changeCursor() {
	Glib::RefPtr<Gdk::Cursor> cur;
	if(actionMap[mouse.cur_operation].cursorProc)
		cur = (this->*actionMap[mouse.cur_operation].cursorProc)();

	if(!cur)
		cur = curList->cursor(CURSOR_DEFAULT);

	if(cur != cursor) {
		get_window()->set_cursor(cur);
		cursor = cur;
	}
}

// MS_NO_OPERATION --------------------------------------------------------------------
void SDK_Editor::noOperation_DownProc(gdouble x, gdouble y, int button, int state) {
	if(button == BTN_MIDDLE) {
		beginOperation(MS_SCROLL);
		scroll_DownProc(x, y, button, state);
	}

	switch(mouse.obj.type) {
		case SDK_OBJ_NONE:
			csdk->pasteX = GRID_X(x);
			csdk->pasteY = GRID_Y(y);

			csdk->selMan->clear();
			if(button == BTN_LEFT) {
				beginOperation(MS_FOCUS_RECT);
				focusRect_DownProc(x, y, button, state);
			}
			/*
			Widget *w = this;
			do {
				w = w->get_parent();
			} while(!GTK_IS_SCROLLED_WINDOW(w->gobj()));
			ScrolledWindow *sc = (ScrolledWindow *)w;
			sc->get_vadjustment()->set_value(sc->get_vadjustment()->get_value() + 10);
			*/
			break;
		case SDK_OBJ_ELEMENT:
			if(mouse.obj.element->checkCollise(x,y) && mouse.obj.element->mouseDown(x, y, button)) {
				beginOperation(MS_MOUSE_ELEMENT);
				moveElement_DownProc(x, y, button, state);
			}
			else {
				beginOperation(MS_MOVE_ELEMENT);
				moveElement_DownProc(x, y, button, state);
			}
			break;
		case SDK_OBJ_POINT:
			beginOperation(MS_POINT_LINK);
			pointLink_DownProc(x, y, button, state);
			break;
		case SDK_OBJ_LINKPOINT:
			beginOperation(MS_MOVE_LINE_POINT);
			moveLinePoint_DownProc(x, y, button, state);
			break;
		case SDK_OBJ_LINE:
			beginOperation(MS_LINE);
			line_DownProc(x, y, button, state);
			break;
		case SDK_OBJ_LHINT:
			beginOperation(MS_MOVE_LINK_HINT);
			linkHint_DownProc(x, y, button, state);
			break;
		default:;
	}
}

void SDK_Editor::noOperation_MoveProc(gdouble x, gdouble y) {
	switch(mouse.select_obj.type) {
		case SDK_OBJ_ELEMENT:
			mouse.curx = x;
			mouse.cury = y;
			break;
		default: ;
	}
}

void SDK_Editor::noOperation_UpProc(gdouble x, gdouble y, int button, int state) {

}

static const CursorListType elementCursors[] = {CURSOR_DEFAULT, CURSOR_MOVE, CURSOR_H_DOUBLE, CURSOR_V_DOUBLE, CURSOR_BR_CORNER};

Glib::RefPtr<Gdk::Cursor> SDK_Editor::noOperation_cursorProc() {
	Glib::RefPtr<Gdk::Cursor> cur;
	switch(mouse.select_obj.type) {
		case SDK_OBJ_POINT: return curList->cursor(CURSOR_HAND);
		case SDK_OBJ_LINE: return curList->cursor(CURSOR_ADD_LINE_POINT);
		case SDK_OBJ_LINKPOINT: return curList->cursor(CURSOR_MOVE_LINE_POINT);
		case SDK_OBJ_ELEMENT: return curList->cursor(elementCursors[mouse.select_obj.element->mouseCursor(mouse.curx, mouse.cury)]);
		case SDK_OBJ_LHINT: return curList->cursor(CURSOR_MOVE);
		//selHint: Result := crSize;
		default: return cur;
	}
}

// MS_INSERT_ELEMENT --------------------------------------------------------------------

Cairo::ErrorStatus SDK_Editor::savePNG(const unsigned char* data, unsigned int length) {
	cursorLoader->write(data, length);
	return CAIRO_STATUS_SUCCESS;
}

void SDK_Editor::createInsertCursor(TypePixbuf img, const char *maskName, CursorListType cursor) {
	Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create_from_png(dataDir + CURSORS_PATH + maskName + ".png");
	Cairo::RefPtr<Cairo::Context> dc = Cairo::Context::create(surface);

	dc->translate(6, 6);
	Gdk::Cairo::set_source_pixbuf(dc, img, 0.0, 0.0);
	dc->rectangle (0, 0, ICON_SIZE, ICON_SIZE);
	dc->clip();
	dc->paint();

	cursorLoader = Gdk::PixbufLoader::create();
	surface->write_to_png_stream(sigc::mem_fun(*this, &SDK_Editor::savePNG));
	cursorLoader->close();

	curList->setCustom(cursor, cursorLoader->get_pixbuf());
}

void SDK_Editor::insertElement_BeginProc(const void *data) {
	PackElement *pe = sdk->pack->getElementByName((char*)data);

	for(int i = CURSOR_INSERT_ELEMENT; i <= CURSOR_INSERT_ELEMENT_POINT; i++)
		createInsertCursor(pe->image, curInfo[i - CURSOR_MOVE_LINE_POINT].name, (CursorListType)i);
}

void SDK_Editor::insertElement_DownProc(gdouble x, gdouble y, int button, int state) {
	if(button == BTN_LEFT) {
		if(mouse.obj.type == SDK_OBJ_POINT) {
			mouse.curx = x;
			mouse.cury = y;
			return;
		}
		else {
			if(pasteElementOnLine((char*)mouse.data)) {
				endOperation();
				beginOperation(MS_MOVE_ELEMENT, NULL);
			}
		}
	}
	endOperation();
}

void SDK_Editor::insertElement_MoveProc(gdouble x, gdouble y) {
	if(mouse.obj.type == SDK_OBJ_POINT) {
		pointLink_MoveProc(x, y);
	}
}

void SDK_Editor::insertElement_UpProc(gdouble x, gdouble y, int button, int state) {
	Element *e = csdk->add((char*)mouse.data, GRID_X(x), GRID_Y(y));
	if(e) {
		csdk->selMan->select(e);
		ElementPoint *p = e->getFirstFreePoint(mouse.obj.point->getPair());
		if(p) {
			mouse.obj.point->clear();
			mouse.obj.point->connect(p);
		}
		switch(mouse.select_obj.type) {
			case SDK_OBJ_POINT:
				p = e->getFirstFreePoint(mouse.select_obj.point->getPair());
				if(p) {
					mouse.select_obj.point->clear();
					Element *e1 = mouse.obj.point->parent;
					Element *e2 = mouse.select_obj.point->parent;
					if(mouse.obj.point->getDirection() != mouse.select_obj.point->getDirection()) {
						if(p->getDirection())
							e->move(GRID_X(e1->x - e->x), GRID_Y(e2->y - e->y));
						else
							e->move(GRID_X(e2->x - e->x), GRID_Y(e1->y - e->y));
					}
					else
						e->move(GRID_X(e1->x + (e2->x - e1->x) / 2) - e->x,
								GRID_Y(e1->y + (e2->y - e1->y) / 2) - e->y);
					mouse.select_obj.point->connect(p)->createPath();
					reDraw();
				}
				break;
			default:
				;
		}
		if(mouse.obj.point->point)
			mouse.obj.point->createPath();
	}
	endOperation();
}

void SDK_Editor::insertElement_DrawProc(DrawContext cr) {
	if(mouse.obj.type == SDK_OBJ_POINT) {
		pointLink_DrawProc(cr);
	}
}

Glib::RefPtr<Gdk::Cursor> SDK_Editor::insertElement_cursorProc() {
	switch(mouse.select_obj.type) {
		case SDK_OBJ_POINT:	return curList->cursor(CURSOR_INSERT_ELEMENT_POINT);
		case SDK_OBJ_LINE:	return curList->cursor(CURSOR_INSERT_ELEMENT_LINE);
		default:			return curList->cursor(CURSOR_INSERT_ELEMENT);
	}
}

// MS_FOCUS_RECT --------------------------------------------------------------------
void SDK_Editor::focusRect_DownProc(gdouble x, gdouble y, int button, int state) {
	csdk->selMan->clear();
}

void SDK_Editor::focusRect_MoveProc(gdouble x, gdouble y)  {
	gdouble _x, _y;

	if(mouse.curx < mouse.startx)
		_x = min(mouse.curx, x) ;
	else
		_x = max(mouse.curx, x);
	if(mouse.cury < mouse.starty)
		_y = min(mouse.cury, y);
	else
		_y = max(mouse.cury, y);

	if(x != mouse.curx)
		invalidateRealRect(x, _y, mouse.curx, mouse.starty);
	if(y != mouse.cury)
		invalidateRealRect(_x, y, mouse.startx, mouse.cury);

	mouse.curx = x;
	mouse.cury = y;
}

void SDK_Editor::focusRect_UpProc(gdouble x, gdouble y, int button, int state) {
	csdk->selMan->selectRect(mouse.startx, mouse.starty, mouse.curx, mouse.cury);

	endOperation();
	reDraw();
}

void SDK_Editor::focusRect_DrawProc(DrawContext cr) {
	int x = (int)(mouse.startx*zoom);
	int y = (int)(mouse.starty*zoom);
	int w = (int)((mouse.curx - mouse.startx)*zoom);
	int h = (int)((mouse.cury - mouse.starty)*zoom);
	Cairo::Antialias old = cr->get_antialias();
	cr->set_antialias(Cairo::ANTIALIAS_NONE);
	cr->set_source_rgba(0.5, 0.7, 0.7, 0.4);
	cr->rectangle(x, y, w, h);
	cr->fill();
	cr->set_line_width(1);
	cr->set_source_rgba(0.2, 0.2, 0.2, 0.4);
	cr->rectangle(x, y, w, h);
	cr->stroke();
	cr->set_antialias(old);
}

// MS_MOVE_ELEMENT --------------------------------------------------------------------
void SDK_Editor::moveElement_DownProc(gdouble x, gdouble y, int button, int state) {
	if(!mouse.obj.element->isSelect()) {
		csdk->selMan->select(mouse.obj.element);
	}

	if(button != BTN_LEFT)
		endOperation();
	else if(state & KEY_CONTROL) {
		for(ElementsList::iterator e = csdk->selMan->elements.begin(); e != csdk->selMan->elements.end(); e++)
			for(ElementPoints::iterator p = (*e)->points.begin(); p != (*e)->points.end(); p++)
				if((*p)->point)
					selectPoint(*p);
	}
}

void SDK_Editor::moveElement_MoveProc(gdouble x, gdouble y) {
	gdouble dx = GRID_X(x - mouse.curx);
	gdouble dy = GRID_Y(y - mouse.cury);
	if(dx || dy) {
		csdk->selMan->move(dx, dy);
		changeScheme();
		mouse.curx += dx;
		mouse.cury += dy;
	}
}

void SDK_Editor::moveElement_UpProc(gdouble x, gdouble y, int button, int state) {
	unselectPoints();
	endOperation();
}

// MS_POINT_LINK --------------------------------------------------------------------
void SDK_Editor::pointLink_DownProc(gdouble x, gdouble y, int button, int state) {
	switch(button) {
		case BTN_RIGHT:
			if(mouse.obj.type == SDK_OBJ_POINT) {
				mouse.obj.point->clear();
				changeScheme();
			}
			endOperation();
			break;
		case BTN_LEFT: {
			ElementPoint *p = mouse.obj.point->point;
			if(p) {
				mouse.obj.point = p;
				p->clear();
			}
			mouse.startx = mouse.obj.point->pos->x;
			mouse.starty = mouse.obj.point->pos->y;
			break;
		}
	}
}

void SDK_Editor::pointLink_MoveProc(gdouble x, gdouble y) {
	gdouble _x, _y;

	if(mouse.curx < mouse.startx)
		_x = min(mouse.curx, x) ;
	else
		_x = max(mouse.curx, x);
	if(mouse.cury < mouse.starty)
		_y = min(mouse.cury, y);
	else
		_y = max(mouse.cury, y);

	mouse.curx = x;
	mouse.cury = y;

	invalidateRealRect(mouse.startx, mouse.starty, _x, _y);

	if(mouse.select_obj.type == SDK_OBJ_LINE)
		selectPoint(mouse.select_obj.point);
}

void SDK_Editor::pointLink_UpProc(gdouble x, gdouble y, int button, int state) {
	ObjectType obj;
	if(csdk->getObjectAtPos(x, y, &obj))
		switch(obj.type) {
			case SDK_OBJ_POINT:
				if(mouse.obj.point->canConnect(obj.point)) {
					mouse.obj.point->connect(obj.point)->createPath();
					changeScheme();
				}
				break;
			case SDK_OBJ_ELEMENT: {
				obj.element->connectToPoint(mouse.obj.point);
				if(mouse.obj.point->point)
					changeScheme();
				break;
			}
			case SDK_OBJ_LINE: {
				Element *e = csdk->add(obj.point->type == pt_event ? "HubEx" : "GetDataEx", GRID_X(x), GRID_Y(y));
				e->insertInLine(obj.point, obj.pos);
				e->connectToPoint(mouse.obj.point);
				changeScheme();
			}
			default: ;
		}

	reDraw();
	endOperation();
}

void SDK_Editor::pointLink_DrawProc(DrawContext cr) {
	cr->set_line_width(0.5*zoom);
	Cairo::Antialias a = cr->get_antialias();
	cr->set_antialias(Cairo::ANTIALIAS_DEFAULT);
	if(mouse.obj.point->type <= pt_event)
		cr->set_source_rgb(0.0, 0.0, 1.0);
	else
		cr->set_source_rgb(1.0, 0.0, 0.0);
	cr->move_to(mouse.obj.point->pos->x*zoom, mouse.obj.point->pos->y*zoom);
	cr->line_to(mouse.curx*zoom, mouse.cury*zoom);
	cr->stroke();
	cr->set_antialias(a);
	cr->set_line_width(1);
}

Glib::RefPtr<Gdk::Cursor> SDK_Editor::pointLink_cursorProc() {
	switch(mouse.select_obj.type) {
		case SDK_OBJ_POINT:	return curList->cursor(mouse.obj.point->canConnect(mouse.select_obj.point) ? CURSOR_POINT_LINK_POINT : CURSOR_POINT_LINK_NONE);
		case SDK_OBJ_LINE:	return curList->cursor(CURSOR_POINT_LINK_LINE);
		default:			return curList->cursor(CURSOR_POINT_LINK_NONE);
	}
}

// MS_MOVE_LINE_POINT --------------------------------------------------------------------
void SDK_Editor::moveLinePoint_DownProc(gdouble x, gdouble y, int button, int state) {
	if(button == BTN_RIGHT) {
		mouse.obj.point->removeLinePoint(mouse.obj.pos);
		changeScheme();
		endOperation();
	}
}

void SDK_Editor::moveLinePoint_MoveProc(gdouble x, gdouble y) {
	mouse.obj.point->moveLinePoint(mouse.obj.pos, x, y);
	changeScheme();
}

void SDK_Editor::moveLinePoint_UpProc(gdouble x, gdouble y, int button, int state) {
	endOperation();
}

Glib::RefPtr<Gdk::Cursor> SDK_Editor::moveLinePoint_cursorProc() {
	return curList->cursor(CURSOR_MOVE_LINE_POINT);
}

// MS_LINE --------------------------------------------------------------------
void SDK_Editor::line_DownProc(gdouble x, gdouble y, int button, int state) {
	if(button == BTN_LEFT) {
		mouse.obj.pos = ElementPoint::addLinePoint(mouse.obj.pos, x, y);
		beginOperation(MS_MOVE_LINE_POINT, NULL);
	}
	else {
		ObjectType popupData = mouse.obj;
		endOperation();
		mouse.obj = popupData;
	}
}

// MS_MOUSE_ELEMENT --------------------------------------------------------------------
void SDK_Editor::mouseElement_MoveProc(gdouble x, gdouble y) {
	mouse.curx = x;
	mouse.cury = y;
	mouse.obj.element->mouseMove(x, y);
}

void SDK_Editor::mouseElement_UpProc(gdouble x, gdouble y, int button, int state) {
	mouse.obj.element->mouseUp(x, y, button);
	endOperation();
}

Glib::RefPtr<Gdk::Cursor> SDK_Editor::mouseElement_cursorProc() {
	return curList->cursor(elementCursors[mouse.obj.element->mouseCursor(mouse.curx, mouse.cury)]);
}

// MS_SELECT_BY_EDGE --------------------------------------------------------------------
void SDK_Editor::selectByEdge_DownProc(gdouble x, gdouble y, int button, int state) {
	switch(button) {
		case BTN_RIGHT:
			endOperation();
			break;
		case BTN_LEFT: {
			sdk->selMan->selectByEdge((SelectManager::SBE_Types) (long) mouse.data,mouse.curx,mouse.cury);
			endOperation();
			reDraw();
			beginOperation(MS_MOVE_ELEMENT, NULL);
			break;
		}
	}
}

void SDK_Editor::selectByEdge_MoveProc(gdouble x, gdouble y) {
	switch((SelectManager::SBE_Types) (long) mouse.data){
		case SelectManager::SBE_EDGE_VERTICAL:
			invalidateRealRect(0, y, get_width()/zoom, mouse.cury);
			break;
		case SelectManager::SBE_EDGE_HORIZONTAL:
			invalidateRealRect(x, 0, mouse.curx, get_height()/zoom);
			break;
		case SelectManager::SBE_EDGE_BOTH:
			invalidateRealRect(0, y, max(mouse.curx, x), mouse.cury);
			invalidateRealRect(x, 0, mouse.curx, max(mouse.cury, y));
			break;
	}
	mouse.curx = x;
	mouse.cury = y;
}

void SDK_Editor::selectByEdge_UpProc(gdouble x, gdouble y, int button, int state) {
	endOperation();
}

void SDK_Editor::selectByEdge_DrawProc(DrawContext cr) {
	cr->set_line_width(1);
	Cairo::Antialias a = cr->get_antialias();
	cr->set_antialias(Cairo::ANTIALIAS_NONE);
	cr->set_source_rgb(0.0, 0.0, 1.0);
	switch((SelectManager::SBE_Types) (long) mouse.data){
		case SelectManager::SBE_EDGE_VERTICAL:
			cr->move_to(0, mouse.cury*zoom);
			cr->line_to(get_width(), mouse.cury*zoom);
			break;
		case SelectManager::SBE_EDGE_HORIZONTAL:
			cr->move_to(mouse.curx*zoom, 0);
			cr->line_to(mouse.curx*zoom, get_height());
			break;
		case SelectManager::SBE_EDGE_BOTH:
			cr->move_to(0, mouse.cury*zoom);
			cr->line_to(mouse.curx*zoom, mouse.cury*zoom);
			cr->line_to(mouse.curx*zoom, 0);
			break;
	}
	cr->stroke();
	cr->set_antialias(a);
}

Glib::RefPtr<Gdk::Cursor> SDK_Editor::selectByEdge_cursorProc() {
	return curList->cursor((CursorListType)(CURSOR_EDGE_HORZ + (long) mouse.data));
}

// MS_SCROLL --------------------------------------------------------------------
void SDK_Editor::scroll_DownProc(gdouble x, gdouble y, int button, int state) {
	mouse.startx = x;
	mouse.starty = y;
}

void SDK_Editor::scroll_MoveProc(gdouble x, gdouble y) {
	int dx = (int)(mouse.startx - x);
	int dy = (int)(mouse.starty - y);

	//printf("%d : %d\n",dx, dy);
	Widget *w = this;
	do {
		w = w->get_parent();
	} while(!GTK_IS_SCROLLED_WINDOW(w->gobj()));
	ScrolledWindow *sc = (ScrolledWindow *)w;
	sc->get_vadjustment()->set_value(sc->get_vadjustment()->get_value() + dy);
	sc->get_hadjustment()->set_value(sc->get_hadjustment()->get_value() + dx);

	mouse.startx = x + dx;
	mouse.starty = y + dy;
}

void SDK_Editor::scroll_UpProc(gdouble x, gdouble y, int button, int state) {
	endOperation();
}

// MS_MOVE_LINK_HINT --------------------------------------------------------------------
void SDK_Editor::linkHint_DownProc(gdouble x, gdouble y, int button, int state) {
	if(button == BTN_LEFT) {
		mouse.startx = mouse.obj.hint->x;
		mouse.starty = mouse.obj.hint->y;
		mouse.data = NULL;
	}
	else {
		mouse.startx = mouse.obj.hint->width;
		mouse.data = &mouse;
	}
}

void SDK_Editor::linkHint_MoveProc(gdouble x, gdouble y) {
	changeScheme();
	Gdk::Rectangle r(mouse.obj.hint->parent->drawRect());
	if(!mouse.data) {
		mouse.obj.hint->x = mouse.startx + x - mouse.curx;
		mouse.obj.hint->y = mouse.starty + y - mouse.cury;
	}
	else {
		mouse.obj.hint->setWidth(mouse.startx + x - mouse.curx);
	}
	r.join(mouse.obj.hint->parent->drawRect());
	invalidateRealRect(r.get_x(), r.get_y(), r.get_x() + r.get_width(), r.get_y() + r.get_height());
}

class LH_MenuItem : public MenuItem {
	private:
		ElementProperty *prop;
		LinkHint *hint;
	public:
		LH_MenuItem(LinkHint *hint, ElementProperty *prop):MenuItem(prop->name) {
			this->prop = prop;
			this->hint = hint;
		}
		virtual void on_activate() {
			hint->prop = prop;
			hint->updateText();
		}
};

void SDK_Editor::linkHint_UpProc(gdouble x, gdouble y, int button, int state) {
	if(!mouse.obj.hint->prop) {
//		hintMenu.items().clear();

		Element *e = mouse.obj.hint->parent;
		LH_MenuItem *item;
		for(ElementProperties::iterator prop = e->props.begin(); prop != e->props.end(); prop++) {
			item = new LH_MenuItem(mouse.obj.hint, *prop);
			hintMenu.append(*item);
		}
		hintMenu.append(*(new SeparatorMenuItem()));
		for(ElementProperties::iterator prop = e->sysProps.begin(); prop != e->sysProps.end(); prop++) {
			item = new LH_MenuItem(mouse.obj.hint, *prop);
			hintMenu.append(*item);
		}
		hintMenu.show_all_children();
		hintMenu.popup(button, mouse.time);
	}
	endOperation();
}

Glib::RefPtr<Gdk::Cursor> SDK_Editor::linkHint_cursorProc() {
	return curList->cursor(CURSOR_MOVE);
}

// MS_ADD_LINK_HINT --------------------------------------------------------------------
void SDK_Editor::lh_add_DownProc(gdouble x, gdouble y, int button, int state) {
	ObjectType obj;
	if(csdk->getObjectAtPos(x, y, &obj) && obj.type == SDK_OBJ_ELEMENT) {
		LinkHint *lh = new LinkHint(obj.element);
		obj.element->hints.push_back(lh);
		lh->updateText();
		mouse.obj.type = SDK_OBJ_LHINT;
		mouse.obj.hint = lh;
		beginOperation(MS_MOVE_LINK_HINT);
		linkHint_DownProc(x, y, BTN_LEFT, state);
	}
	else
		endOperation();
}

Glib::RefPtr<Gdk::Cursor> SDK_Editor::lh_add_cursorProc() {
	return curList->cursor(mouse.select_obj.type == SDK_OBJ_ELEMENT ? CURSOR_LH_ADD_OK : CURSOR_LH_ADD_FAIL);
}

// MS_REMOVE_LINK_HINT --------------------------------------------------------------------
void SDK_Editor::lh_remove_DownProc(gdouble x, gdouble y, int button, int state) {
	ObjectType obj;
	if(csdk->getObjectAtPos(x, y, &obj) && obj.type == SDK_OBJ_LHINT) {
		obj.hint->parent->hints.remove(obj.hint);
		delete obj.hint;
		reDraw();
	}
	endOperation();
}

Glib::RefPtr<Gdk::Cursor> SDK_Editor::lh_remove_cursorProc() {
	return curList->cursor(CURSOR_LH_REMOVE);
}
