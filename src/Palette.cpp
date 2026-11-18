/*
 * Palette.cpp
 *
 *  Created on: 01.05.2010
 *      Author: dilma
 */

#include <gtkmm/stylecontext.h>

#include "Palette.h"
#include "ThemeColors.h"
#include "CommandManager.h"
#include "CodeGen.h"
#include "commands.h"

#define TAB_FONT_SIZE 11
#define GROUP_FONT_SIZE 10

PaletteItem::PaletteItem(Palette *parent, const ustring &name, const ustring &title) {
	this->parent = parent;
	this->name = name;
	this->title = title;
	caption = name;
	opened = false;
	x = 0;
	selected = NULL;
	select = false;
	down = false;
	parentItem = NULL;
	hint = false;
}

PaletteItem *PaletteItem::getItemAtPos(int x, int y) {
	if(opened)
		for(PaletteItems::iterator p = items.begin(); p != items.end(); p++)
			if((*p)->x <= x && (*p)->y <= y && ((*p)->x + (*p)->width) >= x && ((*p)->y + (*p)->height) >= y)
				return *p;
	return NULL;
}

void PaletteItem::mouseDown(int x, int y, int button) {
	PaletteItem *p = getItemAtPos(x, y);
	if(p)
		p->mouseDown(x, y, button);
	else {
		down = true;
		invalidate();
	}
}

void PaletteItem::mouseMove(int x, int y, int button) {
	PaletteItem *p = getItemAtPos(x, y);
	if(p != selected) {
		if(selected)
			selected->mouseLeave();
		selected = p;
		if(selected)
			selected->mouseEnter();
	}
	if(p)
		p->mouseMove(x, y, button);
	else
		down = button;
}

void PaletteItem::mouseUp(int x, int y, int button) {
	PaletteItem *p = getItemAtPos(x, y);
	if(p)
		p->mouseUp(x, y, button);
}

void PaletteItem::mouseEnter() {
	select = true;
	down = false;
	invalidate();
	if(hint) {
#ifdef HIASM_GTK4
		parent->set_tooltip_markup("<b>" + Glib::Markup::escape_text(name) + "</b>\n" + Glib::Markup::escape_text(title));
#else
		parent->hint.setText(name, title);
#endif
	}
}

void PaletteItem::mouseLeave() {
	if(selected) {
		selected->mouseLeave();
		selected = NULL;
	}
	select = false;
	down = false;
	invalidate();
	if(hint) {
#ifdef HIASM_GTK4
		parent->set_has_tooltip(false);
#else
		parent->hint.hide();
#endif
	}
}

void PaletteItem::draw(DrawContext dc)  {
	if(opened)
		for(PaletteItems::iterator p = items.begin(); p != items.end(); p++)
			(*p)->draw(dc);
}

void PaletteItem::reSize() {
	for(PaletteItems::iterator p = items.begin(); p != items.end(); p++)
		(*p)->reSize();

	if(fullWidth)
		width = parent->layoutWidth() - 2;
	height = minHeight;
	if(opened) {
		int _x = x, _y = y + minHeight;
		PaletteItem *item = NULL;
		for(PaletteItems::iterator p = items.begin(); p != items.end(); p++) {
			if(_x + (*p)->width >= parent->layoutWidth() - 2) {
				_x = x;
				if(item)
					_y += item->height + 1;
			}

			(*p)->x = _x;
			(*p)->y = _y;
			_x += (*p)->width;
			height = _y - y + (*p)->height;
			item = *p;
		}
		height ++;
	}
}

void PaletteItem::open() {
	opened = true;
	PaletteItem *pi = parentItem;
	while(pi->parentItem) pi = pi->parentItem;
	pi->reSize();
	invalidate();
}

void PaletteItem::close() {
	opened = false;
	PaletteItem *pi = parentItem;
	while(pi->parentItem) pi = pi->parentItem;
	pi->reSize();
	parent->invalidate();
}

PaletteItem *PaletteItem::add(PaletteItem *item) {
	items.push_back(item);
	item->parentItem = this;
	reSize();
	return item;
}

void PaletteItem::clear() {
	for(PaletteItems::iterator p = items.begin(); p != items.end(); p++)
		delete (*p);
	items.clear();
	select = false;
	selected = NULL;
}

void PaletteItem::invalidate() {
	Gdk::Rectangle r(x - 2, y - 2, width + 4, height + 4);
	widget_invalidate_rect(*parent, r);
}

void PaletteItem::save(HStringList &list) {
	for(PaletteItems::iterator p = items.begin(); p != items.end(); p++)
		(*p)->save(list);
}

void PaletteItem::load(HStringList &list) {
	for(PaletteItems::iterator p = items.begin(); p != items.end(); p++)
		(*p)->load(list);
}

PaletteItemTab::PaletteItemTab(Palette *parent, const ustring &name, const ustring &title):PaletteItem(parent, name, title) {
	minHeight = 20;
	height = minHeight;
	timerID = 0;
	fullWidth = true;
}

PaletteItemTab::~PaletteItemTab() {
	stop();
}

void PaletteItemTab::mouseUp(int x, int y, int button) {
	if(y - this->y <= minHeight) {
		if(button == BTN_RIGHT)
			parent->on_tab_menu_popup(this);
		else {
			if(opened)
				close();
			else {
				parent->closeAll();
				open();
			}
		}
	}
	else
		PaletteItem::mouseUp(x, y, button);
}

void tabRoundRect(DrawContext dc, gdouble x, gdouble y, int w, int h) {
	static const int r = 5;
	static const double degrees = G_PI / 180.0;

	dc->begin_new_sub_path();
	dc->arc(x + w - r, y + r, r, -90 * degrees, 0 * degrees);
	dc->arc(x + w - r, y + h - r, r, 0 * degrees, 90 * degrees);
	dc->arc(x + r, y + h - r, r, 90 * degrees, 180 * degrees);
	dc->arc(x + r, y + r, r, 180 * degrees, 270 * degrees);
	dc->close_path();
}

void PaletteItemTab::draw(DrawContext dc) {
	Glib::RefPtr<StyleContext> style = parent->get_style_context();

	Gdk::RGBA dark = themeSelected(style);
	Gdk::RGBA light = themeBackground(style);

	dc->set_line_width(0.5);

	if(select || opened) {
		if(select) {
			Gdk::RGBA norm = style->get_color();
			//TypeColor sel = style->get_base(Gtk::STATE_SELECTED);

			Cairo::RefPtr<Cairo::LinearGradient> lin_grad = Cairo::LinearGradient::create(0, y, width, y + minHeight);
			//Cairo::RefPtr<Cairo::LinearGradient> lin_grad = Cairo::LinearGradient::create(width / 2, y, width / 2, y + minHeight);
			lin_grad->add_color_stop_rgb(0, dark.get_red(), dark.get_green(), dark.get_blue());
			lin_grad->add_color_stop_rgb(1, light.get_red(), light.get_green(), light.get_blue());
			dc->set_source(lin_grad);
		}
		else if(opened) {
			Gdk::RGBA sel = themeSelected(style);
			dc->set_source_rgb(sel.get_red(), sel.get_green(), sel.get_blue());
		}

		dc->rectangle(x, y, width + 2, minHeight);
		dc->fill();

		/*
		tabRoundRect(dc, x + 1, y, width, minHeight);
		dc->fill();
		dc->set_source_rgb(light.get_red_p(), light.get_green_p(), light.get_blue_p());
		tabRoundRect(dc, x + 1, y, width, minHeight);
		dc->stroke();
		*/
	}
	//dc->rectangle(x, y, width, height);
	//dc->stroke();
	/*
	style->paint_box(parent->get_window(),
						opened ? Gtk::STATE_SELECTED : (select ? Gtk::STATE_ACTIVE : Gtk::STATE_NORMAL),
						Gtk::SHADOW_IN,
						Gdk::Rectangle(x + 1, y, width, 20),
						*parent, ustring(), x + 1, y, width, 20);
	*/
	Gdk::Cairo::set_source_rgba(dc, (select || opened) ? themeSelectedText(style) : themeText(style));
	dc->move_to(x + 20,y + 14);
	dc->set_font_size(TAB_FONT_SIZE);
	dc->show_text(caption);

	dc->set_line_width(1);

	if(opened) {
		Cairo::Antialias a = dc->get_antialias();
		dc->set_antialias(Cairo::ANTIALIAS_NONE);
		dc->set_source_rgb(dark.get_red(), dark.get_green(), dark.get_blue());
		dc->move_to(x + 3, y + height);
		dc->rel_line_to(width - 6, 0);
		dc->stroke();
		dc->set_antialias(a);
	}

	Gdk::Cairo::set_source_rgba(dc, (select || opened) ? themeSelectedText(style) : themeText(style));
	dc->arc(x + 10, y + 10, 4, 0, 2*3.1415);
	dc->fill();

	PaletteItem::draw(dc);
}

bool PaletteItemTab::on_timer() {
	if(!items.size())
		return false;

	int cols = parent->layoutWidth() / PaletteItemButton::BTN_SIZE;
	int rows = (items.size() - 1) / cols;
	if(opened) {
		if(height < minHeight + rows*PaletteItemButton::BTN_SIZE)
			height++;
		else {
			open();
			return false;
		}
		parent->invalidate();
	}
	else {
		parent->invalidate();
		if(height > 20)
			height--;
		else {
			close();
			return false;
		}
	}

	return true;
}

gboolean tab_timer_proc(void *data) {
	return ((PaletteItemTab*)data)->on_timer();
}

void PaletteItemTab::start() {
	if(!timerID)
		timerID = g_timeout_add(20, tab_timer_proc, this);
}

void PaletteItemTab::stop() {
	if(timerID) {
		g_source_remove(timerID);
		timerID = 0;
	}
}

//--------------------------------------------------------------------------------------

PaletteItemGroup::PaletteItemGroup(Palette *parent, const ustring &name):PaletteItem(parent, name, "") {
	int p = name.find('_');
	if(p != -1)
		caption = name.substr(p+1, -1);
	minHeight = 14;
	height = minHeight;
	fullWidth = true;
}

PaletteItemGroup::~PaletteItemGroup() {

}

void PaletteItemGroup::mouseUp(int x, int y, int button) {
	if(y - this->y <= 12) {
		if(opened)
			close();
		else
			open();
	}
	else
		PaletteItem::mouseUp(x, y, button);
}

void PaletteItemGroup::draw(DrawContext dc) {
	Glib::RefPtr<StyleContext> style = parent->get_style_context();

	Gdk::RGBA dark = themeSelected(style);
	Gdk::RGBA light = themeBackground(style);

	dc->set_line_width(0.5);
	Gdk::Cairo::set_source_rgba(dc, themeMix(themeBackground(style), themeText(style), 0.08));
	dc->rectangle(x + 1, y, width, minHeight);
	dc->fill();
	dc->set_source_rgb(light.get_red(), light.get_green(), light.get_blue());
	dc->rectangle(x + 1, y, width, minHeight);
	dc->stroke();

//	if(select)
//		Gdk::Cairo::set_source_color(dc, style->get_text(Gtk::STATE_NORMAL));
//	else
//		Gdk::Cairo::set_source_color(dc, style->get_text(Gtk::STATE_SELECTED));
	Gdk::Cairo::set_source_rgba(dc, themeText(style));
	dc->move_to(x + 10,y + 11);
	dc->set_font_size(GROUP_FONT_SIZE);
	dc->show_text(caption);

	PaletteItem::draw(dc);
}

//----------------------------------------------------------------------------------------

PaletteItemButton::PaletteItemButton(Palette *parent, const ustring &name, const ustring &title, Glib::RefPtr<Gdk::Pixbuf> icon):
		PaletteItem(parent, name, title)
{
	minHeight = BTN_SIZE;
	height = BTN_SIZE;
	width = BTN_SIZE;
	this->icon = icon;
	fullWidth = false;
	hint = true;
}

void PaletteItemButton::draw(DrawContext dc) {
	Glib::RefPtr<StyleContext> style = parent->get_style_context();

	Gdk::RGBA dark = themeSelected(style);
	Gdk::RGBA light = themeBackground(style);

	dc->set_antialias(Cairo::ANTIALIAS_NONE);
	if(select) {
		dc->set_source_rgb(dark.get_red(), dark.get_green(), dark.get_blue());
	}
	else if(opened) {
		Gdk::RGBA sel = themeSelected(style);
		dc->set_source_rgb(sel.get_red(), sel.get_green(), sel.get_blue());
	}

	if(select || opened) {
		dc->rectangle(x, y, width, height);
		dc->fill();
		dc->set_line_width(1.0);
		dc->set_source_rgb(light.get_red(), light.get_green(), light.get_blue());
		dc->rectangle(x, y, width, height);
		dc->stroke();
	}

	dc->set_antialias(Cairo::ANTIALIAS_DEFAULT);
	/*
	if(select)
	style->paint_box(parent->get_window(),
						Gtk::STATE_ACTIVE,
						down ? Gtk::SHADOW_IN : Gtk::SHADOW_OUT,
						Gdk::Rectangle(x, y, width, height),
						*parent, ustring(), x, y, width, height);
	else if(opened)
		style->paint_box(parent->get_window(),
							Gtk::STATE_SELECTED,
							Gtk::SHADOW_IN,
							Gdk::Rectangle(x, y, width, height),
							*parent, ustring(), x, y, width, height);
	*/

	dc->save();
	dc->translate(x + 3, y + 3);
	Gdk::Cairo::set_source_pixbuf(dc, icon, 0.0, 0.0);
	dc->rectangle (0, 0, 24, 24);
	dc->clip();
	dc->paint();
	dc->restore();
}

void PaletteItemButton::mouseUp(int x, int y, int button) {
	if(button == BTN_RIGHT) parent->on_element_menu_popup(this);
	else if(button == BTN_LEFT) open();
}

void PaletteItemButton::open() {
	parent->select(this);
	opened = true;
	invalidate();
}

void PaletteItemButton::close() {
	opened = false;
	invalidate();
}

PaletteItemRoot::PaletteItemRoot(Palette *parent, const ustring &name):PaletteItem(parent, name, "") {
	opened = true;
}

void PaletteItemRoot::reSize() {
	int y = 0;
	for(PaletteItems::iterator p = items.begin(); p != items.end(); p++) {
		(*p)->y = y;
		(*p)->reSize();
		y += (*p)->height;
	}
	parent->set_size_request(80, y);
}

//---------------------------------------------------------------------------------------------------------------

PaletteHint::PaletteHint(): Window() {
	set_default_size(200, 20);
	set_name("gtk-tooltip");
	vb.add(caption);
	vb.add(info);
	vb.set_spacing(5);
#ifdef HIASM_GTK4
	set_child(vb);
#else
	add(vb);
#endif
	vb.show();
	Glib::RefPtr<CssProvider> css = Gtk::CssProvider::create();
	css->load_from_data("* {"
		"border: 1px solid black;"
	"}");
	get_style_context()->add_provider(css, G_MAXUINT);
}

void PaletteHint::setText(const ustring &name, const ustring &info) {
	if(is_visible()) return;

	caption.set_markup("<b>" + name + "</b>");
	this->info.set_text(" " + info + " ");
	set_size_request(100, 20);
	show();
}

//---------------------------------------------------------------------------------------------------------------

Palette::Palette():
	on_element_select(this, CBT_SELECT_ELEMENT_PALETTE)
{
#ifdef HIASM_GTK4
	get_style_context()->add_class("element-palette");
#endif
	widget_set_events(*this, Gdk::BUTTON_PRESS_MASK|Gdk::POINTER_MOTION_MASK|Gdk::BUTTON_RELEASE_MASK|Gdk::LEAVE_NOTIFY_MASK);
#ifdef HIASM_GTK4

	connect_button_press(*this, sigc::mem_fun(*this, &Palette::on_button_press_event));
	connect_button_release(*this, sigc::mem_fun(*this, &Palette::on_button_release_event));
	connect_motion_notify(*this, sigc::mem_fun(*this, &Palette::on_motion_notify_event));
	connect_leave_notify(*this, sigc::mem_fun(*this, &Palette::on_leave_notify_event));
#endif
	pack = NULL;
	currentTab = NULL;
	selected = NULL;
	layout_width = 0;
	items = new PaletteItemRoot(this, "root");
	downButton = 0;

	palTabMenu = cmdMan->createMenu(MENUNAME_PAL_TAB);
	palElementMenu = cmdMan->createMenu("pal_el_menu", [this](const ustring &command) {
		if(pack && !menuElement.empty() && on_open_file && (command == CMD_SHA_INI || command == CMD_SHA_PAS)) {
			const auto path = command == CMD_SHA_INI ? pack->pathElementConf(menuElement) : pack->pathElementCode(menuElement);
			if(file_test(path, FILE_TEST_IS_REGULAR)) on_open_file(path);
		} else processCommand(command);
	}, [this](const ustring &command) { return commandAvailable(command); });
}

Palette::~Palette() {
	delete items;
	delete palTabMenu;
	delete palElementMenu;
}

void Palette::loadPack(Pack *pack) {
	TRACE_PROC
	if(this->pack == pack) return;

	clear();
	if(!pack) return;

	this->pack = pack;
	pack->ensureLoaded();

	for(std::list<PackGroup*>::iterator pg = pack->groups.begin(); pg != pack->groups.end(); pg++) {
		if((*pg)->name.empty() || (*pg)->elements.empty()) continue;
		// A tab and a section header appear only when at least one element can be offered,
		// so groups whose elements are all planned leave no empty headers behind.
		bool offered = false;
		for(PackElement *candidate : (*pg)->elements)
			if(candidate->name.c_str()[0] != '*' && pack->compiles(candidate->name)) { offered = true; break; }
		if(!offered) continue;

		PaletteItem *tab = items->add(new PaletteItemTab(this, (*pg)->name, (*pg)->name));
		PaletteItem *group = tab;
		PackElement *separator = nullptr;
		for(std::list<PackElement*>::iterator pk = (*pg)->elements.begin(); pk != (*pg)->elements.end(); pk++) {
			if((*pk)->name.c_str()[0] == '*') { separator = *pk; continue; }
			if(!pack->compiles((*pk)->name)) continue;
			if(separator) {
				group = tab->add(new PaletteItemGroup(this, separator->name));
				if(!separator->info.empty()) group->caption = separator->info;
				separator = nullptr;
			}
			group->add(new PaletteItemButton(this, (*pk)->name, (*pk)->info, (*pk)->image));
		}
	}
	items->reSize();
	invalidate();
	load();
}

bool Palette::on_expose_event(GdkEventExpose* event) {
	return false;
}

bool Palette::on_draw (const ::Cairo::RefPtr< ::Cairo::Context >& cr) {
//	DrawContext cr = get_window()->create_cairo_context();

//	cr->set_antialias(Cairo::ANTIALIAS_NONE);
	cr->set_line_width(1);

		Glib::RefPtr<StyleContext> style = get_style_context();
Gdk::Cairo::set_source_rgba(cr, themeBackground(style));

	cr->paint();


	cr->select_font_face("arial", Cairo::FONT_SLANT_NORMAL, Cairo::FONT_WEIGHT_NORMAL);

	items->draw(cr);

	return true;
}

bool Palette::on_button_press_event(GdkEventButton* event) {
	downButton = event->button;
	items->mouseDown(event->x, event->y, event->button);
	return true;
}

bool Palette::on_button_release_event(GdkEventButton* event) {
	downButton = 0;
	eventTime = event->time;
	items->mouseUp(event->x, event->y, event->button);
	return true;
}

bool Palette::on_motion_notify_event(GdkEventMotion* event) {
#ifndef HIASM_GTK4
	int x, y;
	Gdk::ModifierType mt;
	get_root_window()->get_pointer(x, y, mt);
	hint.move(x + 16, y + 16);
#endif
	items->mouseMove(event->x, event->y, downButton);
	return true;
}

bool Palette::on_leave_notify_event(GdkEventCrossing* event) {
	items->mouseMove(-1, -1, 0);
	return true;
}

#ifdef HIASM_GTK4
void Palette::on_map() {
	Gtk::Widget::on_map();
	items->reSize();
}

void Palette::size_allocate_vfunc(int width, int height, int baseline) {
	Gtk::DrawingArea::size_allocate_vfunc(width, height, baseline);
	layout_width = width;
	items->reSize();
}
#else
void Palette::on_size_allocate(Allocation &alloc) {
	Widget::on_size_allocate(alloc);
	items->reSize();
}
#endif

void Palette::on_tab_menu_popup(PaletteItemTab *tab) {
	currentTab = tab;
	palTabMenu->popup(BTN_RIGHT, eventTime);
	//currentTab = NULL;
}

void Palette::on_element_menu_popup(PaletteItemButton *item) {
	menuElement = item->name;
	updateCommands();
	cmdMan->endUpdate();
	palElementMenu->popup(BTN_RIGHT, eventTime);
}

void Palette::updateCommands() {
	for(const char *command : {CMD_SVN, CMD_SVN_UPDATE, CMD_SVN_LOG, CMD_SVN_SHOW})
		if(auto *item = cmdMan->findByName(command)) item->enabled = commandAvailable(command);
}

bool Palette::commandAvailable(const ustring &command) const {
	if(!pack || menuElement.empty()) return false;
	if(command == CMD_SHA_INI || command == CMD_SHA_PAS) {
		const auto file = command == CMD_SHA_INI ? pack->pathElementConf(menuElement) : pack->pathElementCode(menuElement);
		return on_open_file && file_test(file, FILE_TEST_IS_REGULAR);
	}
	if(command == CMD_SVN || command == CMD_SVN_UPDATE || command == CMD_SVN_LOG || command == CMD_SVN_SHOW)
		return !svn.busy() && SvnClient::available() &&
			SvnClient::isWorkingCopy(pack->pathElementConf(menuElement).raw());
	return false;
}

bool Palette::processCommand(const ustring &command) {
	if(command != CMD_SVN_UPDATE && command != CMD_SVN_LOG && command != CMD_SVN_SHOW) return false;
	if(!commandAvailable(command)) return true;
	const auto action = command == CMD_SVN_UPDATE ? SvnClient::Update :
		command == CMD_SVN_LOG ? SvnClient::Log : SvnClient::Diff;
	const ustring target = pack->pathElementConf(menuElement);
	if(!svn.start(action, target.raw(), [this, action](bool ok, const std::string &text) {
		const std::string result = (ok ? "@" : "!") + (text.empty() ? "SVN: no changes" : text);
		cgt_on_debug.run(result.c_str());
		if(ok && action == SvnClient::Update)
			cgt_on_debug.run("@SVN update finished. Restart HiAsm to reload changed element definitions; existing schemes retain their loaded configuration.");
		updateCommands(); cmdMan->endUpdate();
	})) cgt_on_debug.run("!SVN requires an installed svn client and an element config inside a working copy");
	updateCommands(); cmdMan->endUpdate();
	return true;
}

void Palette::addGroup(const ustring &name) {
	if(currentTab) {
		PaletteItem *group = currentTab->add(new PaletteItemGroup(this, "*" + currentTab->caption + "_" + name));
		group->caption = name;
		if(currentTab->opened) {
			currentTab->close();
			currentTab->open();
		}
	}
}

void Palette::unSelect() {
	if(selected)
		selected->close();
	selected = NULL;
}

void Palette::select(PaletteItemButton *item) {
	if(selected)
		selected->close();

	selected = item;
	on_element_select.run(item->name.c_str());
}

void Palette::closeAll() {
	for(PaletteItems::iterator item = items->items.begin(); item != items->items.end(); item++) {
		if((*item)->opened)
			(*item)->close();
	}
}

void Palette::clear() {
	save();
	currentTab = NULL;
	selected = NULL;
	menuElement.clear();
	pack = NULL;
	items->clear();
	invalidate();
}

void Palette::invalidate() {
	widget_invalidate(*this);
}

void Palette::save() {
	if(!pack) return;

	ustring tabs = homeDir + TABS_STATE_PATH;
	if(!file_test(tabs, FILE_TEST_IS_DIR))
		g_mkdir_with_parents(tabs.c_str(), 0700);

	HStringList lst;
	items->save(lst);
	lst.saveToFile(tabs + pack->name);
}

void Palette::load() {
	if(!pack) return;

	ustring tabs = homeDir + TABS_STATE_PATH + pack->name;
	if(!file_test(tabs, FILE_TEST_EXISTS))
		return;

	HStringList lst(tabs);
	items->load(lst);
}
