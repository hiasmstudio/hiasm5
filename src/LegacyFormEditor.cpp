/*
 * LegacyFormEditor.cpp
 *
 * Form editor of external HiAsm 4 packages (FE_Legacy).
 */

#include "FormEditor.h"
#include "ElementProperty.h"
#include "CodeGen.h"
#include <climits>
#include <cmath>
#include <cstdlib>
#include <vector>

FE_Legacy *FE_Legacy::activeInstance = nullptr;

namespace {
const int gridStep = 10;	// grid dots, as in WidgetEditorEx
const int moveStep = 5;		// the mouse moves controls by 5 pixels
const int minimumSize = 5;
const double gripX[8] = {0, 0.5, 1, 1, 1, 0.5, 0, 0};
const double gripY[8] = {0, 0, 0, 0.5, 1, 1, 1, 0.5};

// Delphi system colors used to paint the controls.
const int tcBtnFace = -16777201, tcBtnHighlight = -16777196, tcBtnShadow = -16777200;
const int tc3DDkShadow = -16777195, tc3DLight = -16777194, tcWindow = -16777211;
const int tcWindowText = -16777208, tcGrayText = -16777199;
enum { AlignLeft, AlignCenter, AlignRight };

// Element property behind an [Edit] name (Caption=Caption, Alignment=HAlign).
ElementProperty *mapped(Element *element, const char *name) {
	if(!element || !element->tpl || !element->tpl->conf) return nullptr;
	const auto &props = element->tpl->conf->editProps;
	const auto it = props.find(name);
	return it == props.end() ? nullptr : element->props.getByName(it->second);
}
int intProp(Element *element, const char *name, int fallback = 0) {
	ElementProperty *p = element ? element->props.getByName(name) : nullptr;
	return p ? p->readInt() : fallback;
}
ustring strProp(ElementProperty *p) { return p ? p->readStr() : ustring(); }
int colorOf(ElementProperty *p, int fallback) {
	if(!p || p->type != data_color) return fallback;
	const int color = ((TypeColor*)p->value)->toDelphi();
	// The C++ Button has an empty colour default: black there means "not set", so the face colour is drawn.
	const Pack *pack = p->parent && p->parent->tpl && p->parent->tpl->conf ? p->parent->tpl->conf->pack : nullptr;
	if(color == 0 && pack && !pack->isLegacy() && p->isDefault()) return fallback;
	return color;
}
void setSource(const Cairo::RefPtr<Cairo::Context> &cr, int color) {
	const TypeColor c = TypeColor::fromDelphi(color);
	cr->set_source_rgb(c.get_red(), c.get_green(), c.get_blue());
}
// ClientSize (True,False): index 0 means Width/Height are the client area.
bool clientSizeMode(Element *form) {
	ElementProperty *p = mapped(form, "ClientSize");
	return p && p->readInt() == 0;
}
int borderStyle(Element *form) {
	ElementProperty *p = mapped(form, "BorderStyle");
	const int value = p ? p->readInt() : 2;
	return value >= 0 && value <= 9 ? value : 2;
}

#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
// THIMainForm.SetBorderStyle (code/hiMainForm.pas): bsNone, bsSingle,
// bsSizeable, bsDialog, bsToolWindow, bsSizeToolWin, bsBox, bsSizeBox,
// bsBoxEdge, bsSizeBoxEdge.
const DWORD styleSet[10] = {
	0, WS_CAPTION | WS_MINIMIZEBOX, WS_CAPTION | WS_THICKFRAME | WS_MAXIMIZEBOX | WS_MINIMIZEBOX,
	WS_CAPTION, WS_CAPTION, WS_CAPTION | WS_THICKFRAME, 0, WS_THICKFRAME, 0, WS_THICKFRAME,
};
const DWORD exSet[10] = {
	0, 0, 0, WS_EX_WINDOWEDGE, WS_EX_TOOLWINDOW, WS_EX_TOOLWINDOW, WS_EX_DLGMODALFRAME,
	WS_EX_TOOLWINDOW, WS_EX_CLIENTEDGE | WS_EX_DLGMODALFRAME, WS_EX_CLIENTEDGE,
};
const wchar_t *windowClass = L"HiAsmLegacyForm";

template<typename T> T userProc(const char *name) {
	HMODULE user32 = GetModuleHandleW(L"user32.dll");
	return user32 ? reinterpret_cast<T>(reinterpret_cast<void(*)()>(GetProcAddress(user32, name))) : nullptr;
}
UINT windowDpi(HWND hwnd) {
	typedef UINT (WINAPI *GetDpi)(HWND);
	static const GetDpi getDpi = userProc<GetDpi>("GetDpiForWindow");
	const UINT dpi = getDpi && hwnd ? getDpi(hwnd) : 0;
	return dpi ? dpi : 96;
}
// Size the frame adds to the client area at the given DPI.
void frameExtra(DWORD style, DWORD exStyle, UINT dpi, int &width, int &height) {
	typedef BOOL (WINAPI *AdjustForDpi)(LPRECT, DWORD, BOOL, DWORD, UINT);
	static const AdjustForDpi adjust = userProc<AdjustForDpi>("AdjustWindowRectExForDpi");
	RECT r = {0, 0, 100, 100};
	if(!(adjust && adjust(&r, style, FALSE, exStyle, dpi))) AdjustWindowRectEx(&r, style, FALSE, exStyle);
	width = r.right - r.left - 100;
	height = r.bottom - r.top - 100;
}
void kolStyle(int border, DWORD &style, DWORD &exStyle) {
	style = WS_SYSMENU | WS_CLIPCHILDREN | styleSet[border];
	exStyle = exSet[border];
}
LRESULT CALLBACK legacyWndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
	if(msg == WM_NCCREATE)
		SetWindowLongPtrW(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(reinterpret_cast<CREATESTRUCTW*>(lParam)->lpCreateParams));
	if(auto editor = reinterpret_cast<FE_Legacy*>(GetWindowLongPtrW(hwnd, GWLP_USERDATA)))
		return editor->nativeMessage(hwnd, msg, wParam, lParam);
	return DefWindowProcW(hwnd, msg, wParam, lParam);
}
HCURSOR gripCursor(int grip) {
	// Grips clockwise from the top-left corner.
	static const LPCTSTR shapes[8] = {IDC_SIZENWSE, IDC_SIZENS, IDC_SIZENESW, IDC_SIZEWE,
		IDC_SIZENWSE, IDC_SIZENS, IDC_SIZENESW, IDC_SIZEWE};
	return LoadCursor(nullptr, grip >= 0 && grip < 8 ? shapes[grip] : IDC_ARROW);
}
#endif

// Client area of the form in form pixels (96 DPI).
void clientSize(Element *form, int &width, int &height) {
	width = intProp(form, "Width", 400);
	height = intProp(form, "Height", 300);
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
	if(!clientSizeMode(form)) {
		DWORD style, exStyle; kolStyle(borderStyle(form), style, exStyle);
		int extraW, extraH; frameExtra(style, exStyle, 96, extraW, extraH);
		width -= extraW; height -= extraH;
	}
#endif
	width = max(width, 1);
	height = max(height, 1);
}
bool isControl(Element *element) {
	if(!element || !element->tpl || !element->tpl->conf) return false;
	ElementConfig *conf = element->tpl->conf;
	if(conf->editClassName.empty() || conf->editClassName == "Form") return false;
	for(const char *name : {"Left", "Top", "Width", "Height"})
		if(!element->props.getByName(name)) return false;
	return true;
}
// Font of a control: its own, or the form's with ParentFont=True (index 0).
FontRecord fontOf(Element *element, Element *form) {
	ElementProperty *own = mapped(element, "Font");
	if(!own) own = element ? element->props.getByName("Font") : nullptr;
	ElementProperty *parent = element ? element->props.getByName("ParentFont") : nullptr;
	if((!own || (parent && parent->readInt() == 0)) && form && element != form)
		own = form->props.getByName("Font");
	// A font that is not set (C++ widgets keep an empty default) falls back to the IDE default font.
	if(own && own->type == data_font && static_cast<FontRecord*>(own->value)->size > 0) return *(FontRecord*)own->value;
	FontRecord font;
	font.name = "MS Sans Serif"; font.size = 8; font.charset = 1;
	return font;
}
void drawText(const Cairo::RefPtr<Cairo::Context> &cr, const FontRecord &font, const ustring &text,
		const Gdk::Rectangle &r, int halign, bool middle, int color = INT_MIN) {
	if(text.empty() || r.get_width() <= 0 || r.get_height() <= 0) return;
	auto layout = Pango::Layout::create(cr);
	Pango::FontDescription description;
	// MS Sans Serif is a bitmap font; Windows substitutes the TrueType version.
	description.set_family(font.name.empty() || font.name == "MS Sans Serif" ? ustring("Microsoft Sans Serif") : font.name);
	// Delphi: Height = -Size * 96 / 72 pixels.
	description.set_absolute_size(max(1, font.size) * 96.0 / 72.0 * PANGO_SCALE);
	pango_font_description_set_weight(description.gobj(), font.style & 1 ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
	pango_font_description_set_style(description.gobj(), font.style & 2 ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
	layout->set_font_description(description);
	PangoAttrList *attrs = pango_attr_list_new();
	if(font.style & 4) pango_attr_list_insert(attrs, pango_attr_underline_new(PANGO_UNDERLINE_SINGLE));
	if(font.style & 8) pango_attr_list_insert(attrs, pango_attr_strikethrough_new(TRUE));
	pango_layout_set_attributes(layout->gobj(), attrs);
	pango_attr_list_unref(attrs);
	layout->set_text(text);
	int width = 0, height = 0;
	layout->get_pixel_size(width, height);
	double x = r.get_x(), y = r.get_y();
	if(halign == AlignCenter) x += (r.get_width() - width) / 2.0;
	else if(halign == AlignRight) x += r.get_width() - width;
	if(middle) y += (r.get_height() - height) / 2.0;
	cr->save();
	cr->rectangle(r.get_x(), r.get_y(), r.get_width(), r.get_height());
	cr->clip();
	setSource(cr, color == INT_MIN ? font.color : color);
	cr->move_to(x, y);
	layout->show_in_cairo_context(cr);
	cr->restore();
}
void fill(const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &r, int color) {
	setSource(cr, color);
	cr->rectangle(r.get_x(), r.get_y(), r.get_width(), r.get_height());
	cr->fill();
}
// One-pixel frame: top/left in one color, bottom/right in another.
void frame(const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &r, int topLeft, int bottomRight) {
	const int x = r.get_x(), y = r.get_y(), w = r.get_width(), h = r.get_height();
	if(w <= 0 || h <= 0) return;
	setSource(cr, topLeft);
	cr->rectangle(x, y, w, 1); cr->rectangle(x, y, 1, h); cr->fill();
	setSource(cr, bottomRight);
	cr->rectangle(x, y + h - 1, w, 1); cr->rectangle(x + w - 1, y, 1, h); cr->fill();
}
Gdk::Rectangle inset(const Gdk::Rectangle &r, int d) {
	return Gdk::Rectangle(r.get_x() + d, r.get_y() + d, max(0, r.get_width() - 2*d), max(0, r.get_height() - 2*d));
}
void raised(const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &r) {
	frame(cr, r, tcBtnHighlight, tc3DDkShadow);
	frame(cr, inset(r, 1), tc3DLight, tcBtnShadow);
}
void sunken(const Cairo::RefPtr<Cairo::Context> &cr, const Gdk::Rectangle &r) {
	frame(cr, r, tcBtnShadow, tcBtnHighlight);
	frame(cr, inset(r, 1), tc3DDkShadow, tc3DLight);
}
}

Element *FE_Legacy::findForm(SDK *sdk) {
	if(!sdk) return nullptr;
	Element *first = nullptr;
	for(Element *element : sdk->elements) {
		if(!element->tpl || !element->tpl->conf || element->tpl->conf->editClassName != "Form") continue;
		if(element->isParent()) return element;
		if(!first) first = element;
	}
	return first;
}

// Controls in scheme order with their rectangles; Align is applied in the
// VCL/KOL order top, bottom, left, right, client.
std::vector<FE_Legacy::Item> FE_Legacy::computeLayout() {
	std::vector<Item> items;
	if(!form || !sdk) return items;
	for(Element *element : sdk->elements) {
		if(element == form || !isControl(element)) continue;
		items.push_back({element, Gdk::Rectangle(intProp(element, "Left"), intProp(element, "Top"),
			max(intProp(element, "Width"), 1), max(intProp(element, "Height"), 1))});
	}
	int cw, ch; clientSize(form, cw, ch);
	int left = 0, top = 0, right = cw, bottom = ch;
	// caNone, caLeft, caTop, caRight, caBottom, caClient
	for(int align : {2, 4, 1, 3, 5})
		for(auto &item : items) {
			ElementProperty *p = item.element->props.getByName("Align");
			if(!p || p->type != data_comboEx || p->readInt() != align) continue;
			Gdk::Rectangle &r = item.rect;
			const int w = r.get_width(), h = r.get_height();
			switch(align) {
				case 2: r = Gdk::Rectangle(left, top, right - left, h); top += h; break;
				case 4: r = Gdk::Rectangle(left, bottom - h, right - left, h); bottom -= h; break;
				case 1: r = Gdk::Rectangle(left, top, w, bottom - top); left += w; break;
				case 3: r = Gdk::Rectangle(right - w, top, w, bottom - top); right -= w; break;
				case 5: r = Gdk::Rectangle(left, top, max(right - left, 0), max(bottom - top, 0)); break;
			}
		}
	return items;
}

Element *FE_Legacy::elementAt(double x, double y) {
	Element *hit = nullptr;	// the last one drawn is on top
	for(const auto &item : computeLayout()) {
		const auto &r = item.rect;
		if(x >= r.get_x() && y >= r.get_y() && x < r.get_x() + r.get_width() && y < r.get_y() + r.get_height())
			hit = item.element;
	}
	return hit;
}

Gdk::Rectangle FE_Legacy::selectionRect() {
	Gdk::Rectangle result(0, 0, 0, 0);
	for(const auto &item : computeLayout())
		if(item.element->isSelect()) {
			if(result.has_zero_area()) result = item.rect;
			else result.join(item.rect);
		}
	return result;
}

int FE_Legacy::gripAt(double x, double y) {
	const auto r = selectionRect();
	if(r.has_zero_area()) return -1;
	for(int i = 0; i < 8; ++i)
		if(std::abs(r.get_x() + gripX[i]*r.get_width() - x) <= 3 && std::abs(r.get_y() + gripY[i]*r.get_height() - y) <= 3)
			return i;
	return -1;
}

void FE_Legacy::shiftSelection(const char *xName, const char *yName, int dx, int dy, int minimum) {
	if(!sdk) return;
	for(Element *element : sdk->elements) {
		if(!element->isSelect() || element->isFreeze() || !isControl(element)) continue;
		const struct { const char *name; int delta; } changes[] = {{xName, dx}, {yName, dy}};
		for(const auto &change : changes) {
			ElementProperty *p = element->props.getByName(change.name);
			if(!change.delta || !p) continue;
			int value = max(minimum, p->readInt() + change.delta);
			if(value != p->readInt()) p->setValue(&value);
		}
	}
}

bool FE_Legacy::on_button_press_event(GdkEventButton *event) {
	if(!form || !sdk || event->button != BTN_LEFT) return false;
	const double x = event->x / zoom, y = event->y / zoom;
	if(sde) sde->beginOperation(MS_SCROLL);
	const int sizer = gripAt(x, y);
	Element *element = sizer < 0 ? elementAt(x, y) : nullptr;
	if(sizer >= 0) {
		grip = sizer;
		drag = DragSize;
	}
	else if(element) {
		if(!element->isSelect()) {
			if(event->state & GDK_SHIFT_MASK) sdk->selMan->add(element);
			else sdk->selMan->select(element);
		}
		drag = DragMove;
	}
	else {
		sdk->selMan->clear();
		drag = DragBand;
	}
	startX = curX = static_cast<int>(x);
	startY = curY = static_cast<int>(y);
	invalidate();
	return true;
}

bool FE_Legacy::on_motion_notify_event(GdkEventMotion *event) {
	const double x = event->x / zoom, y = event->y / zoom;
	if(drag == DragNone) {
		hoverGrip = gripAt(x, y);
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
		if(hwnd) SetCursor(gripCursor(hoverGrip));
#else
		static const int cursors[8] = {CURSOR_TL_CORNER, CURSOR_V_DOUBLE, CURSOR_TR_CORNER, CURSOR_H_DOUBLE,
			CURSOR_BR_CORNER, CURSOR_V_DOUBLE, CURSOR_BL_CORNER, CURSOR_H_DOUBLE};
		if(curList)
			widget_set_cursor(*this, curList->cursor(static_cast<CursorListType>(hoverGrip < 0 ? CURSOR_DEFAULT : cursors[hoverGrip])));
#endif
		return true;
	}
	if(drag == DragBand) {
		curX = static_cast<int>(x); curY = static_cast<int>(y);
		invalidate();
		return true;
	}
	const int dx = static_cast<int>((x - startX) / moveStep) * moveStep;
	const int dy = static_cast<int>((y - startY) / moveStep) * moveStep;
	if(!dx && !dy) return true;
	if(drag == DragMove)
		shiftSelection("Left", "Top", dx, dy, INT_MIN);
	else {
		// Grips clockwise from the top-left corner, as in WidgetEditorEx.
		const bool movesLeft = grip == 0 || grip == 6 || grip == 7, movesTop = grip == 0 || grip == 1 || grip == 2;
		const bool sizesX = grip != 1 && grip != 5, sizesY = grip != 3 && grip != 7;
		const int mx = movesLeft ? dx : 0, my = movesTop ? dy : 0;
		shiftSelection("Left", "Top", mx, my, INT_MIN);
		shiftSelection("Width", "Height", sizesX ? (movesLeft ? -dx : dx) : 0, sizesY ? (movesTop ? -dy : dy) : 0, minimumSize);
	}
	startX += dx; startY += dy;
	if(sde) sde->changeScheme();
	invalidate();
	return true;
}

bool FE_Legacy::on_button_release_event(GdkEventButton *event) {
	if(event->button != BTN_LEFT || drag == DragNone) return false;
	if(drag == DragBand && sdk) {
		curX = static_cast<int>(event->x / zoom); curY = static_cast<int>(event->y / zoom);
		const int x1 = min(startX, curX), y1 = min(startY, curY), x2 = max(startX, curX), y2 = max(startY, curY);
		for(const auto &item : computeLayout()) {
			const auto &r = item.rect;
			if(r.get_x() <= x2 && r.get_y() <= y2 && r.get_x() + r.get_width() >= x1 && r.get_y() + r.get_height() >= y1)
				sdk->selMan->add(item.element);
		}
	}
	drag = DragNone;
	grip = -1;
	if(sde) sde->endOperation();
	invalidate();
	return true;
}

bool FE_Legacy::on_draw(const Cairo::RefPtr<Cairo::Context> &cr) {
	paint(cr, get_width(), get_height());
	return true;
}

// Paints the client area; width/height are device pixels of the target.
void FE_Legacy::paint(const Cairo::RefPtr<Cairo::Context> &cr, int deviceWidth, int deviceHeight) {
	if(!form) return;
	cr->save();
	cr->scale(zoom, zoom);
	int cw, ch; clientSize(form, cw, ch);
	const double width = max(static_cast<double>(cw), deviceWidth / zoom);
	const double height = max(static_cast<double>(ch), deviceHeight / zoom);
	setSource(cr, colorOf(mapped(form, "Color"), tcBtnFace));
	cr->rectangle(0, 0, width, height);
	cr->fill();
	cr->set_source_rgb(0, 0, 0);
	for(int y = 0; y < height; y += gridStep)
		for(int x = 0; x < width; x += gridStep)
			cr->rectangle(x, y, 1, 1);
	cr->fill();
	const auto items = computeLayout();
	for(const auto &item : items) drawControl(cr, item);
	drawSelection(cr, items);
	cr->restore();
}

void FE_Legacy::drawControl(const Cairo::RefPtr<Cairo::Context> &cr, const Item &item) {
	Element *e = item.element;
	const Gdk::Rectangle &r = item.rect;
	const ustring &cls = e->tpl->conf->editClassName;
	ElementProperty *enabledProp = e->props.getByName("Enabled");
	const bool enabled = !enabledProp || enabledProp->readInt() == 0;	// True,False
	FontRecord font = fontOf(e, form);
	if(!enabled) font.color = tcGrayText;
	const ustring caption = strProp(mapped(e, "Caption"));
	cr->save();
	cr->rectangle(r.get_x(), r.get_y(), r.get_width(), r.get_height());
	cr->clip();
	if(cls == "Button") {
		fill(cr, r, colorOf(mapped(e, "Color"), tcBtnFace));
		raised(cr, r);
		drawText(cr, font, caption, inset(r, 2), AlignCenter, true);
	}
	else if(cls == "Label") {
		fill(cr, r, colorOf(mapped(e, "Color"), tcBtnFace));
		ElementProperty *alignment = mapped(e, "Alignment");	// taLeftJustify, taRightJustify, taCenter
		const int a = alignment ? alignment->readInt() : 0;
		drawText(cr, font, caption, r, a == 1 ? AlignRight : a == 2 ? AlignCenter : AlignLeft, false);
	}
	else if(cls == "Edit" || cls == "Memo" || cls == "ListBox" || cls == "ComboBox") {
		fill(cr, r, colorOf(mapped(e, "Color"), tcWindow));
		sunken(cr, r);
		Gdk::Rectangle text = inset(r, 3);
		if(cls == "ComboBox") {
			const int button = min(r.get_height() - 4, 17);
			const Gdk::Rectangle arrow(r.get_x() + r.get_width() - button - 2, r.get_y() + 2, button, r.get_height() - 4);
			fill(cr, arrow, tcBtnFace);
			raised(cr, arrow);
			setSource(cr, tcWindowText);
			const double cx = arrow.get_x() + arrow.get_width() / 2.0, cy = arrow.get_y() + arrow.get_height() / 2.0;
			cr->move_to(cx - 4, cy - 2); cr->line_to(cx + 4, cy - 2); cr->line_to(cx, cy + 2); cr->close_path(); cr->fill();
			text = Gdk::Rectangle(text.get_x(), text.get_y(), max(0, text.get_width() - button), text.get_height());
		}
		const bool single = cls == "Edit" || cls == "ComboBox";
		const ustring value = strProp(mapped(e, single ? "Text" : "Strings"));
		drawText(cr, font, value, text, AlignLeft, single);
	}
	else if(cls == "CheckBox" || cls == "RadioButton") {
		fill(cr, r, colorOf(mapped(e, "Color"), tcBtnFace));
		const int box = 13, top = r.get_y() + (r.get_height() - box) / 2;
		if(cls == "CheckBox") {
			const Gdk::Rectangle mark(r.get_x(), top, box, box);
			fill(cr, mark, tcWindow);
			sunken(cr, mark);
		}
		else {
			setSource(cr, tcWindow);
			cr->arc(r.get_x() + box / 2.0, top + box / 2.0, box / 2.0 - 0.5, 0, 2 * G_PI); cr->fill_preserve();
			setSource(cr, tcBtnShadow); cr->set_line_width(1); cr->stroke();
		}
		drawText(cr, font, caption, Gdk::Rectangle(r.get_x() + box + 4, r.get_y(), max(0, r.get_width() - box - 4), r.get_height()), AlignLeft, true);
	}
	else if(cls == "GroupBox") {
		fill(cr, r, colorOf(mapped(e, "Color"), tcBtnFace));
		const int line = 7;	// half the caption height of an 8 pt font
		const Gdk::Rectangle edge(r.get_x(), r.get_y() + line, r.get_width(), max(0, r.get_height() - line));
		frame(cr, edge, tcBtnShadow, tcBtnHighlight);
		frame(cr, inset(edge, 1), tcBtnHighlight, tcBtnShadow);
		if(!caption.empty()) {
			fill(cr, Gdk::Rectangle(r.get_x() + 6, r.get_y(), min(r.get_width() - 12, static_cast<int>(caption.size()) * 6 + 4), line * 2), colorOf(mapped(e, "Color"), tcBtnFace));
			drawText(cr, font, caption, Gdk::Rectangle(r.get_x() + 8, r.get_y(), max(0, r.get_width() - 16), line * 2), AlignLeft, true);
		}
	}
	else if(cls == "Panel") {
		fill(cr, r, colorOf(mapped(e, "Color"), tcBtnFace));
		frame(cr, r, tcBtnHighlight, tcBtnShadow);
		drawText(cr, font, caption, inset(r, 2), AlignCenter, true);
	}
	else {
		fill(cr, r, colorOf(mapped(e, "Color"), tcBtnFace));
		if(cls == "Progress" || cls == "ScrollBar" || cls == "TrackBar" || cls == "Tab" || cls == "TreeView" || cls == "ScrollBox")
			sunken(cr, r);
		else {
			setSource(cr, tcBtnShadow);
			cr->set_line_width(1);
			const std::vector<double> dashes = {3, 2};
			cr->set_dash(dashes, 0);
			cr->rectangle(r.get_x() + 0.5, r.get_y() + 0.5, r.get_width() - 1, r.get_height() - 1);
			cr->stroke();
		}
		drawText(cr, font, e->tpl->name, inset(r, 2), AlignCenter, true, tcGrayText);
	}
	cr->restore();
}

void FE_Legacy::drawSelection(const Cairo::RefPtr<Cairo::Context> &cr, const std::vector<Item> &items) {
	for(const auto &item : items)
		if(item.element->isSelect()) {
			cr->set_source_rgba(0.5, 0.7, 0.7, 0.4);
			cr->rectangle(item.rect.get_x(), item.rect.get_y(), item.rect.get_width(), item.rect.get_height());
			cr->fill();
		}
	const auto r = selectionRect();
	if(!r.has_zero_area()) {
		cr->set_source_rgb(0, 0, 0);
		for(int i = 0; i < 8; ++i)
			cr->rectangle(r.get_x() + gripX[i]*r.get_width() - 3, r.get_y() + gripY[i]*r.get_height() - 3, 6, 6);
		cr->fill();
	}
	if(drag == DragBand) {
		const int x1 = min(startX, curX), y1 = min(startY, curY);
		const int w = std::abs(curX - startX), h = std::abs(curY - startY);
		cr->set_source_rgba(0.5, 0.7, 0.7, 0.4);
		cr->rectangle(x1, y1, w, h); cr->fill();
		cr->set_line_width(1);
		cr->set_source_rgba(0.2, 0.2, 0.2, 0.4);
		cr->rectangle(x1 + 0.5, y1 + 0.5, w, h); cr->stroke();
	}
}

// Window title, frame (BorderStyle), size and position from the form.
void FE_Legacy::applyFrame() {
	if(!form) return;
	const ustring caption = strProp(mapped(form, "Caption"));
	int cw, ch; clientSize(form, cw, ch);
	const int border = borderStyle(form);
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
	HWND handle = static_cast<HWND>(hwnd);
	if(!handle || !IsWindow(handle)) return;
	if(gunichar2 *title = g_utf8_to_utf16(caption.c_str(), -1, nullptr, nullptr, nullptr)) {
		SetWindowTextW(handle, reinterpret_cast<LPCWSTR>(title));
		g_free(title);
	}
	// Exactly the bits of THIMainForm.SetBorderStyle; nobody else restyles this window.
	DWORD style, exStyle; kolStyle(border, style, exStyle);
	style |= static_cast<DWORD>(GetWindowLongPtrW(handle, GWL_STYLE)) & WS_VISIBLE;
	SetWindowLongPtrW(handle, GWL_STYLE, style);
	SetWindowLongPtrW(handle, GWL_EXSTYLE, exStyle);
	const UINT dpi = windowDpi(handle);
	int extraW, extraH; frameExtra(style, exStyle, dpi, extraW, extraH);
	const int width = MulDiv(cw, dpi, 96) + extraW, height = MulDiv(ch, dpi, 96) + extraH;
	RECT target = {MulDiv(intProp(form, "Left"), dpi, 96), MulDiv(intProp(form, "Top"), dpi, 96), 0, 0};
	target.right = target.left + width; target.bottom = target.top + height;
	// Keep the form reachable when Left/Top lie outside every monitor.
	const UINT move = MonitorFromRect(&target, MONITOR_DEFAULTTONULL) ? 0 : SWP_NOMOVE;
	SetWindowPos(handle, nullptr, target.left, target.top, width, height,
		move | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#else
	if(!window) return;
	// Without the Win32 frame: decorations and resizing of the BorderStyle.
	static const bool decorated[10] = {false, true, true, true, true, true, false, false, false, false};
	static const bool resizable[10] = {false, false, true, false, false, true, false, true, false, true};
	window->set_title(caption);
	window->set_decorated(decorated[border]);
	window->set_resizable(resizable[border]);
	window->set_default_size(static_cast<int>(cw * zoom), static_cast<int>(ch * zoom));
	if(!resizable[border]) set_size_request(static_cast<int>(cw * zoom), static_cast<int>(ch * zoom));
	else set_size_request(-1, -1);
#endif
}

void FE_Legacy::updateZoom() {
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
	zoom = windowDpi(static_cast<HWND>(hwnd)) / 96.0;	// device pixels of the Win32 window
#else
	zoom = 1;
#endif
}

// Win32: an owned top-level window created with the KOL styles of the form.
void FE_Legacy::createNative(void *owner) {
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
	HINSTANCE instance = GetModuleHandleW(nullptr);
	static bool registered = false;
	if(!registered) {
		WNDCLASSEXW wc = {};
		wc.cbSize = sizeof(wc);
		wc.style = CS_HREDRAW | CS_VREDRAW;
		wc.lpfnWndProc = legacyWndProc;
		wc.hInstance = instance;
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);
		wc.lpszClassName = windowClass;
		registered = RegisterClassExW(&wc) != 0 || GetLastError() == ERROR_CLASS_ALREADY_EXISTS;
	}
	DWORD style, exStyle; kolStyle(borderStyle(form), style, exStyle);
	hwnd = CreateWindowExW(exStyle, windowClass, L"", style, CW_USEDEFAULT, CW_USEDEFAULT, 320, 240,
		static_cast<HWND>(owner), nullptr, instance, this);
	if(!hwnd) return;
	updateZoom();
	applyFrame();
	ShowWindow(static_cast<HWND>(hwnd), SW_SHOW);
	UpdateWindow(static_cast<HWND>(hwnd));
#else
	(void)owner;
#endif
}

ustring FE_Legacy::windowTitle() const {
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
	wchar_t text[512] = {};
	if(!hwnd) return ustring();
	GetWindowTextW(static_cast<HWND>(hwnd), text, G_N_ELEMENTS(text));
	gchar *utf8 = g_utf16_to_utf8(reinterpret_cast<const gunichar2*>(text), -1, nullptr, nullptr, nullptr);
	const ustring result = utf8 ? utf8 : "";
	g_free(utf8);
	return result;
#else
	return window ? ustring(window->get_title()) : ustring();
#endif
}

void FE_Legacy::invalidate() {
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
	if(hwnd) InvalidateRect(static_cast<HWND>(hwnd), nullptr, FALSE);
#else
	widget_invalidate(*this);
#endif
}

// Window procedure of the Win32 form: Cairo painting, mouse to the editor.
intptr_t FE_Legacy::nativeMessage(void *native, unsigned msg, uintptr_t wParam, intptr_t lParam) {
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
	HWND handle = static_cast<HWND>(native);
	auto mouse = [&]() {
		GdkEventButton event = {};
		event.button = BTN_LEFT;
		event.x = static_cast<short>(LOWORD(lParam));
		event.y = static_cast<short>(HIWORD(lParam));
		event.state = (wParam & MK_SHIFT) ? GDK_SHIFT_MASK : 0;
		return event;
	};
	// The client area painted by Cairo, copied to a device context.
	auto blit = [&](HDC dc) {
		RECT client; GetClientRect(handle, &client);
		const int w = client.right - client.left, h = client.bottom - client.top;
		if(w <= 0 || h <= 0) return;
		auto surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, w, h);
		paint(Cairo::Context::create(surface), w, h);
		surface->flush();
		BITMAPINFO info = {};
		info.bmiHeader.biSize = sizeof(info.bmiHeader);
		info.bmiHeader.biWidth = surface->get_stride() / 4;
		info.bmiHeader.biHeight = -h;	// top-down rows, as Cairo stores them
		info.bmiHeader.biPlanes = 1;
		info.bmiHeader.biBitCount = 32;
		info.bmiHeader.biCompression = BI_RGB;
		SetDIBitsToDevice(dc, 0, 0, w, h, 0, 0, 0, h, surface->get_data(), &info, DIB_RGB_COLORS);
	};
	switch(msg) {
		case WM_PAINT: {
			PAINTSTRUCT ps;
			HDC dc = BeginPaint(handle, &ps);
			blit(dc);
			EndPaint(handle, &ps);
			return 0;
		}
		case WM_PRINTCLIENT:	// PrintWindow, thumbnails
			blit(reinterpret_cast<HDC>(wParam));
			return 0;
		case WM_ERASEBKGND:
			return 1;
		case WM_LBUTTONDOWN: {
			SetCapture(handle);
			GdkEventButton event = mouse();
			on_button_press_event(&event);
			return 0;
		}
		case WM_LBUTTONUP: {
			GdkEventButton event = mouse();
			on_button_release_event(&event);
			if(GetCapture() == handle) ReleaseCapture();
			return 0;
		}
		case WM_MOUSEMOVE: {
			const GdkEventButton button = mouse();
			GdkEventMotion event = {};
			event.x = button.x; event.y = button.y; event.state = button.state;
			on_motion_notify_event(&event);
			return 0;
		}
		case WM_CAPTURECHANGED:
			if(drag != DragNone) {	// the drag was interrupted (Alt+Tab, another capture)
				GdkEventButton event = {};
				event.button = BTN_LEFT; event.x = curX * zoom; event.y = curY * zoom;
				on_button_release_event(&event);
			}
			return 0;
		case WM_SETCURSOR:
			if(LOWORD(lParam) == HTCLIENT) {
				SetCursor(gripCursor(drag == DragSize ? grip : hoverGrip));
				return TRUE;
			}
			break;
		case WM_SIZE:
			InvalidateRect(handle, nullptr, FALSE);
			return 0;
		case 0x02E0: {	// WM_DPICHANGED: same form pixels on the new monitor
			zoom = HIWORD(wParam) / 96.0;
			const RECT *r = reinterpret_cast<const RECT*>(lParam);
			SetWindowPos(handle, nullptr, r->left, r->top, r->right - r->left, r->bottom - r->top, SWP_NOZORDER | SWP_NOACTIVATE);
			InvalidateRect(handle, nullptr, FALSE);
			return 0;
		}
		case WM_EXITSIZEMOVE:
			queueSync();
			return 0;
		case WM_CLOSE:
			scheduleClose();	// like the toolbar button; the window is destroyed in stop()
			return 0;
		case WM_NCDESTROY:
			SetWindowLongPtrW(handle, GWLP_USERDATA, 0);
			if(hwnd == handle) hwnd = nullptr;
			break;
	}
	return DefWindowProcW(handle, msg, wParam, lParam);
#else
	(void)native; (void)msg; (void)wParam; (void)lParam;
	return 0;
#endif
}

void FE_Legacy::scheduleClose() {
	if(closeIdle) return;
	// Close outside of the window and SDK callbacks that requested it.
	closeIdle = g_idle_add([](gpointer data) -> gboolean {
		auto self = static_cast<FE_Legacy*>(data);
		self->closeIdle = 0;
		ObjectEvent ended = self->on_end_edit;	// may delete self
		if(ended.empty()) self->stop(); else ended();
		return G_SOURCE_REMOVE;
	}, this);
}

void FE_Legacy::queueSync() {
	if(syncIdle) return;
	syncIdle = g_idle_add([](gpointer data) -> gboolean {
		auto self = static_cast<FE_Legacy*>(data);
		self->syncIdle = 0;
		self->syncFromWindow();
		return G_SOURCE_REMOVE;
	}, this);
}

// The user moved or resized the window: Left/Top/Width/Height of the form.
void FE_Legacy::syncFromWindow() {
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
	HWND handle = static_cast<HWND>(hwnd);
	if(!form || !handle || !IsWindow(handle) || IsIconic(handle) || IsZoomed(handle)) return;
	RECT outer, client;
	if(!GetWindowRect(handle, &outer) || !GetClientRect(handle, &client)) return;
	const UINT dpi = windowDpi(handle);
	int width = MulDiv(client.right - client.left, 96, dpi), height = MulDiv(client.bottom - client.top, 96, dpi);
	if(!clientSizeMode(form)) {
		DWORD style, exStyle; kolStyle(borderStyle(form), style, exStyle);
		int extraW, extraH; frameExtra(style, exStyle, 96, extraW, extraH);
		width += extraW; height += extraH;
	}
	const struct { const char *name; int value; } values[] = {
		{"Left", MulDiv(outer.left, 96, dpi)}, {"Top", MulDiv(outer.top, 96, dpi)}, {"Width", width}, {"Height", height},
	};
	bool changed = false;
	syncing = true;
	for(const auto &item : values) {
		ElementProperty *p = form->props.getByName(item.name);
		if(!p || p->readInt() == item.value) continue;
		if(!changed && sde) sde->beginOperation(MS_SCROLL);
		int value = item.value;
		p->setValue(&value);
		changed = true;
	}
	syncing = false;
	if(changed && sde) {
		sde->changeScheme();
		sde->endOperation();
	}
	invalidate();
#endif
}

void FE_Legacy::callback(void *owner, CallbackType type, const void *data) {
	switch(type) {
		case CBT_ELEMENT_PROP_CHANGE: {
			auto prop = static_cast<const ElementProperty*>(data);
			if(owner == form && !syncing && prop) {
				const ustring &name = prop->name;
				if(name == "Left" || name == "Top" || name == "Width" || name == "Height" ||
						prop == mapped(form, "BorderStyle") || prop == mapped(form, "ClientSize") || prop == mapped(form, "Caption"))
					applyFrame();
			}
			invalidate();
			break;
		}
		case CBT_SDK_ADD_ELEMENT:
			subscribe((Element*)data);
			invalidate();
			break;
		case CBT_SDK_REMOVE_ELEMENT: {
			Element *element = (Element*)data;
			element->on_property_change -= this;
			if(element == form) { form = nullptr; scheduleClose(); }
			invalidate();
			break;
		}
		case CBT_SDK_CHANGE_SELECT:
			invalidate();
			break;
		default: ;
	}
}

void FE_Legacy::start(SDK_Editor *editor) {
	stop();
	FormEditor::start(editor);
	Element *found = findForm(editor->csdk);
	if(!found) found = findForm(editor->sdk);
	if(!found) { cgt_on_debug.run("!The scheme has no HiAsm 4 form (Class=Form)"); return; }
	form = found;
	sdk = found->parent;
	activeInstance = this;
	sdk->on_add_element += this;
	sdk->on_remove_element += this;
	sdk->selMan->on_selection_change += this;
	for(Element *element : sdk->elements) subscribe(element);
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
	// Owned by the IDE window: stays above it and minimizes with it, as in HiAsm 4.
	void *owner = nullptr;
	if(auto host = dynamic_cast<Gtk::Native*>(editor->get_root()))
		if(auto surface = host->get_surface()) owner = gdk_win32_surface_get_handle(surface->gobj());
	createNative(owner);
	if(!hwnd) { cgt_on_debug.run("!Cannot create the form window"); stop(); }
#else
	window = new Gtk::Window();
#ifdef HIASM_GTK4
	window->set_child(*this);
	gtk_window_set_titlebar(window->gobj(), nullptr);
	if(auto host = dynamic_cast<Gtk::Window*>(editor->get_root())) window->set_transient_for(*host);
	window->signal_close_request().connect([this]() { scheduleClose(); return true; }, false);
#else
	window->add(*this);
	if(auto host = dynamic_cast<Gtk::Window*>(editor->get_toplevel())) window->set_transient_for(*host);
	window->signal_delete_event().connect([this](GdkEventAny*) { scheduleClose(); return true; }, false);
#endif
	set_hexpand(true);
	set_vexpand(true);
	applyFrame();
	compat_show_all(*window);
	window->present();
#endif
}

void FE_Legacy::stop() {
	for(guint *source : {&closeIdle, &syncIdle})
		if(*source) { g_source_remove(*source); *source = 0; }
	if(drag != DragNone && sde) sde->endOperation();
	drag = DragNone;
	if(sdk) {
		sdk->on_add_element -= this;
		sdk->on_remove_element -= this;
		sdk->selMan->on_selection_change -= this;
		for(Element *element : sdk->elements) element->on_property_change -= this;
	}
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
	if(HWND handle = static_cast<HWND>(hwnd)) {
		hwnd = nullptr;
		SetWindowLongPtrW(handle, GWLP_USERDATA, 0);	// no more messages to this object
		if(IsWindow(handle)) DestroyWindow(handle);
	}
#endif
	if(window) {
		window->set_visible(false);
#ifdef HIASM_GTK4
		window->unset_child();
#else
		window->remove();
#endif
		delete window;
		window = nullptr;
	}
	if(activeInstance == this) activeInstance = nullptr;
	form = nullptr;
	sdk = nullptr;
}
