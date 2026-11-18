/*
 * gtkcompat.h
 *
 * Small compatibility layer for the GTK3 -> GTK4 migration.
 * GTK3 builds keep using the native gtkmm3 API.
 */

#ifndef HIASM_GTKCOMPAT_H_
#define HIASM_GTKCOMPAT_H_

#include <vector>
#include <thread>
#include <set>
#include <map>
#include <memory>
#include "ThemeColors.h"
#include "Localization.h"

#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
#include <windows.h>
#ifdef ERROR
#undef ERROR
#endif
#endif

inline void compat_standard_frame(Gtk::Window &window);

inline int compat_icon_pixels(int size) {
	switch(size) {
		case 1: return 16;
		case 2: return 16;
		case 3: return 24;
		case 4: return 24;
		case 5: return 32;
		case 6: return 48;
		default: return size > 0 ? size : 16;
	}
}
inline bool window_is_fullscreen(Gtk::Window &window) {
#ifdef HIASM_GTK4
	return window.is_fullscreen();
#else
	Glib::RefPtr<Gdk::Window> w = window.get_window();
	return w && ((int)w->get_state() & (int)Gdk::WINDOW_STATE_FULLSCREEN);
#endif
}

#ifdef HIASM_GTK4

namespace Glib {
	constexpr FileTest FILE_TEST_EXISTS = FileTest::EXISTS;
	constexpr FileTest FILE_TEST_IS_DIR = FileTest::IS_DIR;
	constexpr FileTest FILE_TEST_IS_REGULAR = FileTest::IS_REGULAR;
}

struct CompatPolicy {
	constexpr operator int() const { return value; }
	constexpr operator Gtk::PolicyType() const { return (Gtk::PolicyType)value; }
	int value;
};
constexpr CompatPolicy POLICY_ALWAYS{0};
constexpr CompatPolicy POLICY_AUTOMATIC{1};
constexpr CompatPolicy POLICY_NEVER{2};

constexpr Gtk::MessageType MESSAGE_INFO = Gtk::MessageType::INFO;
constexpr Gtk::MessageType MESSAGE_WARNING = Gtk::MessageType::WARNING;
constexpr Gtk::MessageType MESSAGE_QUESTION = Gtk::MessageType::QUESTION;
constexpr Gtk::MessageType MESSAGE_ERROR = Gtk::MessageType::ERROR;

constexpr Gtk::ButtonsType BUTTONS_NONE = Gtk::ButtonsType::NONE;
constexpr Gtk::ButtonsType BUTTONS_OK = Gtk::ButtonsType::OK;
constexpr Gtk::ButtonsType BUTTONS_CLOSE = Gtk::ButtonsType::CLOSE;
constexpr Gtk::ButtonsType BUTTONS_CANCEL = Gtk::ButtonsType::CANCEL;
constexpr Gtk::ButtonsType BUTTONS_YES_NO = Gtk::ButtonsType::YES_NO;
constexpr Gtk::ButtonsType BUTTONS_OK_CANCEL = Gtk::ButtonsType::OK_CANCEL;

constexpr int POS_LEFT = 0;
constexpr int POS_RIGHT = 1;
constexpr int POS_TOP = 2;
constexpr int POS_BOTTOM = 3;

constexpr int JUSTIFY_LEFT = 0;

constexpr bool PACK_SHRINK = false;
constexpr bool PACK_EXPAND_WIDGET = true;

namespace Pango {
	constexpr WrapMode WRAP_WORD = WrapMode::WORD;
	constexpr EllipsizeMode ELLIPSIZE_END = EllipsizeMode::END;
	struct CompatAlignment {
		constexpr operator int() const { return 0; }
		constexpr operator Alignment() const { return Alignment::LEFT; }
	};
	constexpr CompatAlignment ALIGN_LEFT{};
}

namespace Gdk {
	constexpr InterpType INTERP_BILINEAR = InterpType::BILINEAR;
}

namespace Cairo {
	constexpr ToyFontFace::Slant FONT_SLANT_NORMAL = ToyFontFace::Slant::NORMAL;
	constexpr ToyFontFace::Weight FONT_WEIGHT_NORMAL = ToyFontFace::Weight::NORMAL;
	constexpr ToyFontFace::Weight FONT_WEIGHT_BOLD = ToyFontFace::Weight::BOLD;
}

struct CompatOrientation {
	constexpr operator int() const { return value; }
	constexpr operator Gtk::Orientation() const { return (Gtk::Orientation)value; }
	int value;
};
constexpr CompatOrientation ORIENTATION_HORIZONTAL{0};
constexpr CompatOrientation ORIENTATION_VERTICAL{1};

constexpr Gtk::StateFlags STATE_FLAG_NORMAL = Gtk::StateFlags::NORMAL;
constexpr Gtk::StateFlags STATE_FLAG_SELECTED = Gtk::StateFlags::SELECTED;

constexpr Gtk::WrapMode WRAP_WORD = Gtk::WrapMode::WORD;

using EntryIconPosition = Gtk::Entry::IconPosition;
constexpr EntryIconPosition ENTRY_ICON_PRIMARY = Gtk::Entry::IconPosition::PRIMARY;
constexpr EntryIconPosition ENTRY_ICON_SECONDARY = Gtk::Entry::IconPosition::SECONDARY;

constexpr Gtk::FileChooser::Action FILE_CHOOSER_ACTION_OPEN = Gtk::FileChooser::Action::OPEN;
constexpr Gtk::FileChooser::Action FILE_CHOOSER_ACTION_SAVE = Gtk::FileChooser::Action::SAVE;
inline Gtk::FileChooser::Action file_chooser_action(int value) { return (Gtk::FileChooser::Action)value; }

struct GdkEventAny {
	guint type;
};

struct GdkEventButton {
	guint type;
	double x;
	double y;
	guint button;
	guint state;
	guint time;
};

// Values of GdkEventButton::type as GTK 3 named them. GTK 4 reports the number of presses of a click
// gesture instead: the second press of a double click is delivered as COMPAT_2BUTTON_PRESS.
constexpr guint COMPAT_BUTTON_PRESS = 4;
constexpr guint COMPAT_2BUTTON_PRESS = 5;

struct GdkEventMotion {
	guint type;
	double x;
	double y;
	guint state;
	guint time;
};

struct GdkEventScroll {
	guint type;
	double x;
	double y;
	double delta_x;
	double delta_y;
	guint state;
	guint time;
};

struct GdkEventKey {
	guint type;
	guint keyval;
	guint state;
	guint hardware_keycode;
};

struct GdkEventExpose {
	guint type;
	struct {
		int x;
		int y;
		int width;
		int height;
	} area;
};

struct GdkEventCrossing {
	guint type;
	double x;
	double y;
};

namespace Gdk {
	constexpr int KEY_PRESS_MASK = 1 << 0;
	constexpr int BUTTON_PRESS_MASK = 1 << 8;
	constexpr int BUTTON_RELEASE_MASK = 1 << 9;
	constexpr int POINTER_MOTION_MASK = 1 << 12;
	constexpr int LEAVE_NOTIFY_MASK = 1 << 4;
}

template <class WidgetT>
inline void widget_set_events(WidgetT &, int) {
}

template <class WidgetT>
inline void widget_add_events(WidgetT &, int) {
}

enum ButtonBoxStyle {
	BUTTONBOX_SPREAD = 1,
	BUTTONBOX_EDGE,
	BUTTONBOX_START,
	BUTTONBOX_END,
	BUTTONBOX_CENTER
};

inline void compat_pack(Gtk::Box &box, Gtk::Widget &child, bool expand, bool fill, guint padding) {
	box.append(child);
	const bool horizontal = box.get_orientation() == Gtk::Orientation::HORIZONTAL;
	if(horizontal) {
		child.set_hexpand(expand);
		if(!fill)
			child.set_halign(Gtk::Align::START);
	}
	else {
		child.set_vexpand(expand);
		if(!fill)
			child.set_valign(Gtk::Align::START);
	}
	child.set_margin_start(padding);
	child.set_margin_end(padding);
	child.set_margin_top(padding);
	child.set_margin_bottom(padding);
}

class HBox : public Gtk::Box {
	public:
		HBox(bool homogeneous = false, int spacing = 0): Gtk::Box(Gtk::Orientation::HORIZONTAL, spacing) {
			set_homogeneous(homogeneous);
		}
		void add(Gtk::Widget &child) { append(child); }
		void pack_start(Gtk::Widget &child, bool expand = false, bool fill = true, guint padding = 0) {
			compat_pack(*this, child, expand, fill, padding);
		}
		void pack_end(Gtk::Widget &child, bool expand = false, bool fill = true, guint padding = 0) {
			pack_start(child, expand, fill, padding);
			if(!fill)
				child.set_halign(Gtk::Align::END);
		}
};

class VBox : public Gtk::Box {
	public:
		VBox(bool homogeneous = false, int spacing = 0): Gtk::Box(Gtk::Orientation::VERTICAL, spacing) {
			set_homogeneous(homogeneous);
		}
		void add(Gtk::Widget &child) { compat_pack(*this, child, true, true, 0); }
		void pack_start(Gtk::Widget &child, bool expand = false, bool fill = true, guint padding = 0) {
			compat_pack(*this, child, expand, fill, padding);
		}
		void pack_end(Gtk::Widget &child, bool expand = false, bool fill = true, guint padding = 0) {
			pack_start(child, expand, fill, padding);
			if(!fill)
				child.set_valign(Gtk::Align::END);
		}
};

inline Cairo::RefPtr<Cairo::Context> compat_snapshot_begin(Gtk::Widget &widget, int &width, int &height) {
	width = widget.get_width();
	height = widget.get_height();
	if(width <= 0 || height <= 0 || width > 8192 || height > 8192)
		return Cairo::RefPtr<Cairo::Context>();
	try {
		Cairo::RefPtr<Cairo::ImageSurface> surface =
				Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, width, height);
		return Cairo::Context::create(surface);
	} catch(...) {
		return Cairo::RefPtr<Cairo::Context>();
	}
}

inline void compat_snapshot_end(const Glib::RefPtr<Gtk::Snapshot> &snapshot,
		const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) {
	Cairo::RefPtr<Cairo::Surface> surface = cr->get_target();
	if(!surface)
		return;
	Glib::RefPtr<Gdk::Pixbuf> pixbuf = Gdk::Pixbuf::create(surface, 0, 0, width, height);
	if(pixbuf)
		snapshot->append_texture(Gdk::Texture::create_for_pixbuf(pixbuf),
				Gdk::Graphene::Rect(0, 0, width, height));
}

class HDrawingArea : public Gtk::DrawingArea {
	protected:
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &) { return false; }
		void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot> &snapshot) override {
			int width = 0, height = 0;
			Cairo::RefPtr<Cairo::Context> cr = compat_snapshot_begin(*this, width, height);
			if(cr) {
				try {
					on_draw(cr);
					compat_snapshot_end(snapshot, cr, width, height);
				} catch(...) {
				}
			}
			Gtk::DrawingArea::snapshot_vfunc(snapshot);
		}
};

class HFixed : public Gtk::Fixed {
	protected:
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &) { return false; }
		void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot> &snapshot) override {
			int width = 0, height = 0;
			Cairo::RefPtr<Cairo::Context> cr = compat_snapshot_begin(*this, width, height);
			if(cr) {
				try {
					on_draw(cr);
					compat_snapshot_end(snapshot, cr, width, height);
				} catch(...) {
				}
			}
			Gtk::Fixed::snapshot_vfunc(snapshot);
		}
};

class EventBox : public Gtk::Box {
	private:
		sigc::signal<bool(GdkEventButton *)> button_press_signal_;
		sigc::signal<bool(GdkEventButton *)> button_release_signal_;
		sigc::signal<bool(GdkEventMotion *)> motion_signal_;
		sigc::signal<void(Gtk::Allocation &)> size_allocate_signal_;
		int notified_width_ = -1;
		int notified_height_ = -1;
		Glib::RefPtr<Gtk::GestureClick> click_;
		Glib::RefPtr<Gtk::EventControllerMotion> motion_;
		void notify_size(int width, int height) {
			if(width == notified_width_ && height == notified_height_)
				return;
			notified_width_ = width;
			notified_height_ = height;
			Gtk::Allocation allocation(0, 0, width, height);
			size_allocate_signal_.emit(allocation);
		}
	public:
		EventBox(): Gtk::Box(Gtk::Orientation::VERTICAL, 0) {
			Glib::RefPtr<Gtk::GestureClick> click = Gtk::GestureClick::create();
			click_ = click;
			Gtk::GestureClick *click_ptr = click.get();
			click->set_button(0);
			click->signal_pressed().connect([this, click_ptr](int, double x, double y) {
				GdkEventButton event = {};
				event.x = x;
				event.y = y;
				event.button = click_ptr->get_current_button();
				event.state = (guint)click_ptr->get_current_event_state();
				event.time = click_ptr->get_current_event_time();
				bool handled = button_press_signal_.emit(&event);
				if(!handled)
					handled = on_button_press_event(&event);
				if(handled)
					click_ptr->set_state(Gtk::EventSequenceState::CLAIMED);
			}, false);
			click->signal_released().connect([this, click_ptr](int, double x, double y) {
				GdkEventButton event = {};
				event.x = x;
				event.y = y;
				event.button = click_ptr->get_current_button();
				event.state = (guint)click_ptr->get_current_event_state();
				event.time = click_ptr->get_current_event_time();
				bool handled = button_release_signal_.emit(&event);
				if(!handled)
					handled = on_button_release_event(&event);
				if(handled)
					click_ptr->set_state(Gtk::EventSequenceState::CLAIMED);
			}, false);
			add_controller(click);
			Glib::RefPtr<Gtk::EventControllerMotion> motion = Gtk::EventControllerMotion::create();
			motion_ = motion;
			Gtk::EventControllerMotion *motion_ptr = motion.get();
			motion->signal_motion().connect([this, motion_ptr](double x, double y) {
				GdkEventMotion event = {};
				event.x = x;
				event.y = y;
				event.state = (guint)motion_ptr->get_current_event_state();
				event.time = motion_ptr->get_current_event_time();
				bool handled = motion_signal_.emit(&event);
				if(!handled)
					on_motion_notify_event(&event);
			}, false);
			add_controller(motion);
		}
		void add(Gtk::Widget &child) { compat_pack(*this, child, true, true, 0); }
		void remove(Gtk::Widget &child) { Gtk::Box::remove(child); }
		void set_above_child(bool above) {
			const auto phase = above ? Gtk::PropagationPhase::CAPTURE : Gtk::PropagationPhase::BUBBLE;
			click_->set_propagation_phase(phase);
			motion_->set_propagation_phase(phase);
		}
		void set_visible_window(bool) {}
		virtual bool on_button_press_event(GdkEventButton *) { return false; }
		virtual bool on_button_release_event(GdkEventButton *) { return false; }
		virtual bool on_motion_notify_event(GdkEventMotion *) { return false; }
		virtual bool on_draw(const Cairo::RefPtr<Cairo::Context> &) { return false; }
		virtual void on_draw_overlay(const Cairo::RefPtr<Cairo::Context> &) {}
		virtual bool has_draw_overlay() const { return false; }
		void size_allocate_vfunc(int width, int height, int baseline) override {
			Gtk::Box::size_allocate_vfunc(width, height, baseline);
			notify_size(width, height);
		}
		void snapshot_vfunc(const Glib::RefPtr<Gtk::Snapshot> &snapshot) override {
			int width = 0, height = 0;
			Cairo::RefPtr<Cairo::Context> cr = compat_snapshot_begin(*this, width, height);
			if(cr) {
				try {
					on_draw(cr);
					compat_snapshot_end(snapshot, cr, width, height);
				} catch(...) {
				}
			}
			Gtk::Box::snapshot_vfunc(snapshot);
			if(has_draw_overlay()) {
				Cairo::RefPtr<Cairo::Context> overlay = compat_snapshot_begin(*this, width, height);
				if(overlay) {
					on_draw_overlay(overlay);
					compat_snapshot_end(snapshot, overlay, width, height);
				}
			}
			// GtkBox uses GtkBoxLayout; snapshot is the reliable post-layout
			// notification point for containers whose size_allocate vfunc is
			// not dispatched by the layout manager. Emit after child snapshot
			// so resize handlers may safely change child visibility.
			notify_size(get_width(), get_height());
		}
		sigc::signal<void(Gtk::Allocation &)> &signal_size_allocate() { return size_allocate_signal_; }
		sigc::signal<bool(GdkEventButton *)> &signal_button_press_event() { return button_press_signal_; }
		sigc::signal<bool(GdkEventButton *)> &signal_button_release_event() { return button_release_signal_; }
		sigc::signal<bool(GdkEventMotion *)> &signal_motion_notify_event() { return motion_signal_; }
};

class ButtonBox : public Gtk::Box {
	protected:
		ButtonBoxStyle layout;
		ButtonBox(Gtk::Orientation orientation, ButtonBoxStyle layout_, int spacing):
			Gtk::Box(orientation, spacing), layout(layout_) {}
	public:
		void set_layout(ButtonBoxStyle value) { layout = value; }
		ButtonBoxStyle get_layout() const { return layout; }
		void pack_start(Gtk::Widget &child, bool expand = false, bool fill = true, guint padding = 0) {
			compat_pack(*this, child, expand, fill, padding);
		}
		void add(Gtk::Widget &child) { compat_pack(*this, child, true, true, 0); }
		void set_child_secondary(Gtk::Widget &child, bool secondary) {
			if(secondary) {
				if(get_orientation() == Gtk::Orientation::HORIZONTAL)
					child.set_halign(Gtk::Align::END);
				else
					child.set_valign(Gtk::Align::END);
			}
		}
};

class HButtonBox : public ButtonBox {
	public:
		HButtonBox(ButtonBoxStyle layout = BUTTONBOX_SPREAD, int spacing = 0):
			ButtonBox(Gtk::Orientation::HORIZONTAL, layout, spacing) {}
};

class VButtonBox : public ButtonBox {
	public:
		VButtonBox(ButtonBoxStyle layout = BUTTONBOX_SPREAD, int spacing = 0):
			ButtonBox(Gtk::Orientation::VERTICAL, layout, spacing) {}
};

class HandleBox : public Gtk::Frame {
	public:
		HandleBox(): Gtk::Frame() {}
		void add(Gtk::Widget &child) { set_child(child); }
		void remove() { unset_child(); }
		void set_handle_position(int) {}
		void set_snap_edge(int) {}
		void set_shadow_type(int) {}
};

class HPaned : public Gtk::Paned {
	public:
		HPaned(): Gtk::Paned(Gtk::Orientation::HORIZONTAL) {}
		void pack1(Gtk::Widget &child, bool resize = false, bool shrink = true) {
			set_start_child(child);
			set_resize_start_child(resize);
			set_shrink_start_child(shrink);
		}
		void pack2(Gtk::Widget &child, bool resize = true, bool shrink = true) {
			set_end_child(child);
			set_resize_end_child(resize);
			set_shrink_end_child(shrink);
		}
		// GTK3's add1/add2 used the expanding pane defaults. Gtk::Paned in
		// GTK4 only installs the child, so preserve those semantics explicitly.
		void add1(Gtk::Widget &child) {
			set_start_child(child);
			set_resize_start_child(true);
			set_shrink_start_child(true);
		}
		void add2(Gtk::Widget &child) {
			set_end_child(child);
			set_resize_end_child(true);
			set_shrink_end_child(true);
		}
		Gtk::Widget *get_child1() { return get_start_child(); }
		Gtk::Widget *get_child2() { return get_end_child(); }
};

class VPaned : public Gtk::Paned {
	public:
		VPaned(): Gtk::Paned(Gtk::Orientation::VERTICAL) {}
		void pack1(Gtk::Widget &child, bool resize = false, bool shrink = true) {
			set_start_child(child);
			set_resize_start_child(resize);
			set_shrink_start_child(shrink);
		}
		void pack2(Gtk::Widget &child, bool resize = true, bool shrink = true) {
			set_end_child(child);
			set_resize_end_child(resize);
			set_shrink_end_child(shrink);
		}
		// Keep the GTK3 add1/add2 expanding-pane behavior on GTK4.
		void add1(Gtk::Widget &child) {
			set_start_child(child);
			set_resize_start_child(true);
			set_shrink_start_child(true);
		}
		void add2(Gtk::Widget &child) {
			set_end_child(child);
			set_resize_end_child(true);
			set_shrink_end_child(true);
		}
		Gtk::Widget *get_child1() { return get_start_child(); }
		Gtk::Widget *get_child2() { return get_end_child(); }
};

class HScale : public Gtk::Scale {
	public:
		using Gtk::Scale::set_value_pos;
		HScale(double min = 0, double max = 100, double step = 1):
			Gtk::Scale(Gtk::Adjustment::create(min, min, max, step, step * 10), Gtk::Orientation::HORIZONTAL) {}
		void set_value_pos(int pos) { Gtk::Scale::set_value_pos((Gtk::PositionType)pos); }
};

class HScrollbar : public Gtk::Scrollbar {
	public:
		HScrollbar() { set_orientation(Gtk::Orientation::HORIZONTAL); }
		explicit HScrollbar(const Glib::RefPtr<Gtk::Adjustment> &adjustment):
			Gtk::Scrollbar(adjustment, Gtk::Orientation::HORIZONTAL) {}
};

class VScrollbar : public Gtk::Scrollbar {
	public:
		VScrollbar() { set_orientation(Gtk::Orientation::VERTICAL); }
		explicit VScrollbar(const Glib::RefPtr<Gtk::Adjustment> &adjustment):
			Gtk::Scrollbar(adjustment, Gtk::Orientation::VERTICAL) {}
};

class BuiltinStockID {
	public:
		const char *id;
		operator Glib::ustring() const;
};

enum BuiltinIconSize {
	ICON_SIZE_INVALID = 0,
	ICON_SIZE_MENU,
	ICON_SIZE_SMALL_TOOLBAR,
	ICON_SIZE_LARGE_TOOLBAR,
	ICON_SIZE_BUTTON,
	ICON_SIZE_DND,
	ICON_SIZE_DIALOG
};


namespace Gtk {

class StockID {
	private:
		Glib::ustring id_;
	public:
		StockID() {}
		StockID(const char *id): id_(id ? id : "") {}
		StockID(const Glib::ustring &id): id_(id) {}
		const Glib::ustring &get_string() const { return id_; }
		operator Glib::ustring() const { return id_; }
};

class StockIconMap {
	public:
		static Glib::ustring icon_name(const Glib::ustring &id) {
			if(id == "gtk-add") return "list-add";
			if(id == "gtk-cancel") return "process-stop";
			if(id == "gtk-clear") return "edit-clear";
			if(id == "gtk-edit") return "document-edit";
			if(id == "gtk-execute") return "system-run";
			if(id == "gtk-find") return "edit-find";
			if(id == "gtk-fullscreen") return "view-fullscreen";
			if(id == "gtk-ok") return "object-select";
			if(id == "gtk-open") return "document-open";
			if(id == "gtk-save") return "document-save";
			if(id == "gtk-select-all") return "edit-select-all";
			if(id == "gtk-zoom-in") return "zoom-in";
			if(id == "gtk-zoom-out") return "zoom-out";
			if(id.size() > 4 && id.substr(0, 4) == "gtk-")
				return id.substr(4);
			return id;
		}
		static Glib::ustring icon_name(const ::BuiltinStockID &id) {
			return icon_name(Glib::ustring(id.id));
		}
		static Glib::ustring label(const Glib::ustring &id) {
			if(id == "gtk-add") return ui_tr("Add");
			if(id == "gtk-cancel") return ui_tr("Cancel");
			if(id == "gtk-clear") return ui_tr("Clear");
			if(id == "gtk-edit") return ui_tr("Edit");
			if(id == "gtk-execute") return ui_tr("Execute");
			if(id == "gtk-find") return ui_tr("Find");
			if(id == "gtk-fullscreen") return ui_tr("Fullscreen");
			if(id == "gtk-ok") return ui_tr("OK");
			if(id == "gtk-open") return ui_tr("Open");
			if(id == "gtk-save") return ui_tr("Save");
			if(id == "gtk-select-all") return ui_tr("Select All");
			if(id == "gtk-zoom-in") return ui_tr("Zoom In");
			if(id == "gtk-zoom-out") return ui_tr("Zoom Out");
			if(id.size() > 4 && id.substr(0, 4) == "gtk-")
				return id.substr(4);
			return id;
		}
		static Glib::ustring label(const ::BuiltinStockID &id) {
			return label(Glib::ustring(id.id));
		}
};

namespace Stock {
	inline constexpr ::BuiltinStockID ADD{"gtk-add"};
	inline constexpr ::BuiltinStockID CANCEL{"gtk-cancel"};
	inline constexpr ::BuiltinStockID CLEAR{"gtk-clear"};
	inline constexpr ::BuiltinStockID EDIT{"gtk-edit"};
	inline constexpr ::BuiltinStockID EXECUTE{"gtk-execute"};
	inline constexpr ::BuiltinStockID FIND{"gtk-find"};
	inline constexpr ::BuiltinStockID FULLSCREEN{"gtk-fullscreen"};
	inline constexpr ::BuiltinStockID OK{"gtk-ok"};
	inline constexpr ::BuiltinStockID OPEN{"gtk-open"};
	inline constexpr ::BuiltinStockID SAVE{"gtk-save"};
	inline constexpr ::BuiltinStockID SELECT_ALL{"gtk-select-all"};
	inline constexpr ::BuiltinStockID ZOOM_IN{"gtk-zoom-in"};
	inline constexpr ::BuiltinStockID ZOOM_OUT{"gtk-zoom-out"};
	inline std::vector<StockID> get_ids() {
		return {
			StockID("gtk-add"), StockID("gtk-cancel"), StockID("gtk-clear"),
			StockID("gtk-edit"), StockID("gtk-execute"), StockID("gtk-find"),
			StockID("gtk-fullscreen"), StockID("gtk-ok"), StockID("gtk-open"),
			StockID("gtk-save"), StockID("gtk-select-all"),
			StockID("gtk-zoom-in"), StockID("gtk-zoom-out")
		};
	}
}

}

inline BuiltinStockID::operator Glib::ustring() const {
	return Gtk::StockIconMap::label(id);
}

inline int icon_pixel_size(int size) {
	switch(size) {
		case ICON_SIZE_MENU:
		case ICON_SIZE_SMALL_TOOLBAR:
			return 16;
		case ICON_SIZE_LARGE_TOOLBAR:
		case ICON_SIZE_BUTTON:
			return 24;
		case ICON_SIZE_DND:
			return 32;
		case ICON_SIZE_DIALOG:
			return 48;
		default:
			return 16;
	}
}

inline Gtk::Image *stock_image(const Gtk::StockID &id, int size) {
	Gtk::Image *image = new Gtk::Image();
	image->set_from_icon_name(Gtk::StockIconMap::icon_name(id.get_string()));
	image->set_pixel_size(icon_pixel_size(size));
	return image;
}

inline Gtk::Image *stock_image(const BuiltinStockID &id, int size) {
	return stock_image(Gtk::StockID(id.id), size);
}

inline void image_set_stock(Gtk::Image &image, const Gtk::StockID &id, int size) {
	image.set_from_icon_name(Gtk::StockIconMap::icon_name(id.get_string()));
	image.set_pixel_size(icon_pixel_size(size));
}

inline void button_set_image(Gtk::Button &button, Gtk::Widget &image) {
	button.set_child(image);
}

inline void button_set_image_position(Gtk::Button &, Gtk::PositionType) {
}

class RadioButtonGroup {
	public:
		struct State { Gtk::Widget *active = NULL; };
		std::shared_ptr<State> state;
		RadioButtonGroup(): state(new State()) {}
};

inline void radio_group_set_active(const std::shared_ptr<RadioButtonGroup::State> &state, Gtk::Widget *widget, bool active) {
	if(!state)
		return;
	if(!active) {
		if(state->active == widget) {
			widget->remove_css_class("active");
			state->active = NULL;
		}
		return;
	}
	if(state->active && state->active != widget)
		state->active->remove_css_class("active");
	state->active = widget;
	widget->add_css_class("active");
}

class HMenuShell : public Gtk::Box {
	protected:
		std::vector<Gtk::Widget *> children_;
		virtual Gtk::Box &container() { return *this; }
		virtual void child_appended(Gtk::Widget &) {}
	public:
		HMenuShell(Gtk::Orientation orientation): Gtk::Box(orientation, 0) {}
		void append(Gtk::Widget &child) {
			container().append(child);
			children_.push_back(&child);
			child.show();
			child_appended(child);
		}
		void remove(Gtk::Widget &child) {
			container().remove(child);
			for(std::vector<Gtk::Widget *>::iterator i = children_.begin(); i != children_.end(); ++i)
				if(*i == &child) {
					children_.erase(i);
					break;
				}
		}
		std::vector<Gtk::Widget *> get_children() const { return children_; }
};

class HMenu : public HMenuShell {
	private:
		Gtk::Box content_;
		Gtk::Popover popover_;
		Gtk::Widget *anchor_;
		bool attached_;
		HMenu *parent_menu_;
		bool submenu_mode_;
	protected:
		Gtk::Box &container() override { return content_; }
		void child_appended(Gtk::Widget &child) override;
	private:
		void attach_to(Gtk::Widget &anchor) {
			Gtk::Window *window = dynamic_cast<Gtk::Window*>(anchor.get_root());
			if(!window) return;
			if(attached_ && anchor_ != window) {
				popover_.unparent();
				attached_ = false;
			}
			if(!attached_) {
				popover_.set_child(content_);
				content_.show();
				popover_.set_has_arrow(false);
				popover_.set_position(submenu_mode_ ? Gtk::PositionType::RIGHT : Gtk::PositionType::BOTTOM);
				popover_.set_parent(*window);
				attached_ = true;
			}
			anchor_ = &anchor;
			double x = 0, y = 0;
			if(anchor.translate_coordinates(*window, 0, 0, x, y)) {
				if(submenu_mode_) {
					// nested submenu: place to the right of the parent item
					popover_.set_pointing_to(Gdk::Rectangle((int)x, (int)y,
						anchor.get_width(), anchor.get_height()));
				}
				else {
					int min_width = 0, natural_width = 0, min_baseline = 0, natural_baseline = 0;
					content_.measure(Gtk::Orientation::HORIZONTAL, -1, min_width, natural_width,
						min_baseline, natural_baseline);
					int menu_width = natural_width > 0 ? natural_width : anchor.get_width();
					int pointing_x = (int)x + (menu_width - anchor.get_width()) / 2;
					popover_.set_pointing_to(Gdk::Rectangle(pointing_x, (int)y,
						anchor.get_width(), anchor.get_height()));
				}
			}
		}
	public:
		HMenu(): HMenuShell(Gtk::Orientation::VERTICAL), content_(Gtk::Orientation::VERTICAL, 0), anchor_(NULL), attached_(false), parent_menu_(NULL), submenu_mode_(false) {
			static bool css_loaded = false;
			if(!css_loaded) {
				css_loaded = true;
				Glib::RefPtr<Gtk::CssProvider> css = Gtk::CssProvider::create();
				css->load_from_string(
					".hmenu-content { padding: 4px 0; }"
					".hmenu-item { padding: 3px 10px; border-radius: 0; }"
					".hmenu-item:hover { background: alpha(currentColor, 0.08); }"
				".hmenu-item:disabled { color: alpha(currentColor, 0.5); }"
				".hmenu-item.active { background: alpha(currentColor, 0.16); }");
				if(Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default())
					Gtk::StyleProvider::add_provider_for_display(display, css,
						GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
			}
			content_.get_style_context()->add_class("hmenu-content");
		}
		void set_parent_menu(HMenu *menu) { parent_menu_ = menu; }
		void set_submenu_mode(bool value) {
			if(submenu_mode_ == value)
				return;
			submenu_mode_ = value;
			if(attached_) {
				// re-attach so the popover position is recalculated
				popover_.unparent();
				attached_ = false;
			}
		}
		void popdown() {
			popover_.popdown();
			if(parent_menu_)
				parent_menu_->popdown();
		}
		void popup_at(Gtk::Widget &anchor) {
			attach_to(anchor);
			popover_.popup();
		}
		void popup(int, guint32) {
			Gtk::Window *window = NULL;
			for(Gtk::Window *w : Gtk::Window::list_toplevels())
				if(w->get_visible()) {
					window = w;
					break;
				}
			if(!window) return;
			attach_to(*window);
			double x = 0, y = 0;
			if(Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default())
				if(Glib::RefPtr<Gdk::Seat> seat = display->get_default_seat())
					if(Glib::RefPtr<Gdk::Device> pointer = seat->get_pointer())
						pointer->get_surface_at_position(x, y);
			popover_.set_pointing_to(Gdk::Rectangle((int)x - 2, (int)y - 2, 4, 4));
			popover_.popup();
		}
		void popup() {
			if(anchor_)
				popover_.popup();
		}
};

class HMenuBar : public HMenuShell {
	public:
		HMenuBar(): HMenuShell(Gtk::Orientation::HORIZONTAL) {}
		void child_appended(Gtk::Widget &child) override;
};

class HMenuItem : public Gtk::Button {
	private:
		Glib::ustring label_;
		Gtk::Box *content_;
		Gtk::Label *label_widget_;
		Gtk::Label *arrow_;
		HMenu *submenu_;
		HMenu *parent_menu_;
		bool menu_bar_;
	public:
		explicit HMenuItem(const Glib::ustring &label):
			Gtk::Button(), label_(label), content_(NULL), label_widget_(NULL), arrow_(NULL), submenu_(NULL), parent_menu_(NULL), menu_bar_(false) {
			set_has_frame(false);
			get_style_context()->add_class("hmenu-item");
			content_ = new Gtk::Box(Gtk::Orientation::HORIZONTAL, 6);
			label_widget_ = new Gtk::Label(label_);
			label_widget_->set_halign(Gtk::Align::START);
			label_widget_->set_hexpand(true);
			content_->append(*label_widget_);
			label_widget_->show();
			set_child(*content_);
			content_->show();
			signal_clicked().connect([this]() {
				if(submenu_)
					submenu_->popup_at(*this);
				else {
					if(parent_menu_)
						parent_menu_->popdown();
					on_activate();
				}
			}, false);
		}
		virtual void on_activate() {}
		void set_parent_menu(HMenu *menu) {
			parent_menu_ = menu;
			if(submenu_)
				submenu_->set_parent_menu(menu);
		}
		void set_in_menu_bar(bool value) {
			menu_bar_ = value;
			if(value && arrow_)
				arrow_->hide();
			// top-level menus of a menu bar drop down below their item, not beside it
			if(submenu_)
				submenu_->set_submenu_mode(!menu_bar_);
		}
		void set_submenu(HMenu &submenu) {
			submenu_ = &submenu;
			submenu.set_parent_menu(parent_menu_);
			submenu.set_submenu_mode(!menu_bar_);
			if(!menu_bar_ && !arrow_) {
				arrow_ = new Gtk::Label("\xE2\x96\xB8");
				arrow_->set_halign(Gtk::Align::END);
				content_->append(*arrow_);
				arrow_->show();
			}
		}
		void set_image(Gtk::Widget &image) {
			content_->prepend(image);
			image.show();
		}
};

inline void HMenu::child_appended(Gtk::Widget &child) {
	if(HMenuItem *item = dynamic_cast<HMenuItem*>(&child))
		item->set_parent_menu(this);
}

inline void HMenuBar::child_appended(Gtk::Widget &child) {
	if(HMenuItem *item = dynamic_cast<HMenuItem*>(&child))
		item->set_in_menu_bar(true);
}

class HImageMenuItem : public HMenuItem {
	public:
		explicit HImageMenuItem(const Glib::ustring &label): HMenuItem(label) {}
		void set_image(Gtk::Widget &image) { HMenuItem::set_image(image); }
};

class HRadioMenuItem : public HMenuItem {
	private:
		RadioButtonGroup group_;
	public:
		HRadioMenuItem(const RadioButtonGroup &group, const Glib::ustring &label):
			HMenuItem(label), group_(group) {
			signal_clicked().connect([this]() { set_active(true); }, false);
		}
		~HRadioMenuItem() override {
			if(group_.state && group_.state->active == this)
				group_.state->active = NULL;
		}
		RadioButtonGroup get_group() const { return group_; }
		void set_active(bool value) { radio_group_set_active(group_.state, this, value); }
		bool get_active() const { return group_.state->active == this; }
};

class HSeparatorMenuItem : public Gtk::Separator {
	public:
		HSeparatorMenuItem(): Gtk::Separator(Gtk::Orientation::HORIZONTAL) {}
};

class HToolButton : public Gtk::Button {
	public:
		explicit HToolButton(const Glib::ustring &info = Glib::ustring()): Gtk::Button(info) {
			// gtkmm already calls the virtual on_clicked() from the default "clicked" handler;
			// connecting it again here made every toolbar command run twice.
		}
		virtual void on_clicked() {}
		void set_icon_widget(Gtk::Widget &image) {
			set_tooltip_text(get_label());
			set_child(image);
		}
};

class HToggleToolButton : public Gtk::ToggleButton {
	public:
		explicit HToggleToolButton(const Glib::ustring &info = Glib::ustring()): Gtk::ToggleButton(info) {}
		void set_icon_widget(Gtk::Widget &image) {
			set_tooltip_text(get_label());
			set_child(image);
		}
};


class HMenuToolButton : public Gtk::Box {
	private:
		HMenu *menu_;
		Gtk::Button main_button_;
		Gtk::Button arrow_button_;
	public:
		explicit HMenuToolButton(const Glib::ustring &info = Glib::ustring()):
			Gtk::Box(Gtk::Orientation::HORIZONTAL, 0), menu_(NULL),
			arrow_button_(Glib::ustring("\xE2\x96\xBE")) {
			main_button_.set_hexpand(true);
			main_button_.set_tooltip_text(info);
			arrow_button_.set_tooltip_text(info);
			arrow_button_.set_has_frame(false);
			append(main_button_);
			append(arrow_button_);
			main_button_.show();
			arrow_button_.show();
			main_button_.signal_clicked().connect([this]() { on_clicked(); }, false);
			arrow_button_.signal_clicked().connect([this]() {
				if(menu_)
					menu_->popup_at(arrow_button_);
			}, false);
		}
		virtual void on_clicked() {}
		void set_icon_widget(Gtk::Widget &image) {
			main_button_.set_child(image);
			image.show();
		}
		void set_menu(HMenu &menu) { menu_ = &menu; }
};

class HRadioToolButton : public HToolButton {
	private:
		RadioButtonGroup group_;
	public:
		HRadioToolButton(): HToolButton() {
			signal_clicked().connect([this]() { set_active(true); }, false);
		}
		explicit HRadioToolButton(const RadioButtonGroup &group): HToolButton(), group_(group) {
			signal_clicked().connect([this]() { set_active(true); }, false);
		}
		~HRadioToolButton() override {
			if(group_.state && group_.state->active == this)
				group_.state->active = NULL;
		}
		RadioButtonGroup get_group() const { return group_; }
		void set_active(bool value) { radio_group_set_active(group_.state, this, value); }
		bool get_active() const { return group_.state->active == this; }
};

class HSeparatorToolItem : public Gtk::Separator {
	public:
		HSeparatorToolItem(): Gtk::Separator(Gtk::Orientation::VERTICAL) {}
};

class HToolbar : public Gtk::Box {
	public:
		HToolbar(): Gtk::Box(Gtk::Orientation::HORIZONTAL, 0) {}
		void set_toolbar_style(int) {}
		void add(Gtk::Widget &child) { append(child); child.show(); }
};

constexpr int TOOLBAR_ICONS = 0;
#define Menu HMenu
#define MenuBar HMenuBar
#define MenuShell HMenuShell
#define MenuItem HMenuItem
#define ImageMenuItem HImageMenuItem
#define RadioMenuItem HRadioMenuItem
#define SeparatorMenuItem HSeparatorMenuItem
#define Toolbar HToolbar
#define ToolButton HToolButton
#define MenuToolButton HMenuToolButton
#define ToggleToolButton HToggleToolButton
#define RadioToolButton HRadioToolButton
#define SeparatorToolItem HSeparatorToolItem
#define Container Gtk::Widget

namespace Glib {
template<typename T>
using ListHandle = std::vector<T>;

class Thread {
	private:
		std::thread thread_;
	public:
		template<typename Functor>
		Thread(Functor f): thread_(f) {}
		template<typename Functor>
		static Thread *create(Functor f, bool = false) { return new Thread(f); }
		void join() { if(thread_.joinable()) thread_.join(); }
};
}

inline void gdk_threads_init() {}
inline void gdk_threads_enter() {}
inline void gdk_threads_leave() {}

inline void compat_show_uri(const Glib::ustring &uri) {
	Gio::AppInfo::launch_default_for_uri(uri);
}

inline Glib::RefPtr<Gdk::Cursor> compat_cursor_create(const char *name) {
	return Gdk::Cursor::create(name);
}

inline Glib::RefPtr<Gdk::Cursor> compat_cursor_create_from_pixbuf(const Glib::RefPtr<Gdk::Display> &, const Glib::RefPtr<Gdk::Pixbuf> &pix, int x, int y) {
	return Gdk::Cursor::create(Gdk::Texture::create_for_pixbuf(pix), x, y);
}

inline void widget_add_child(Gtk::Widget &parent, Gtk::Widget &child) {
	if(Gtk::Box *box = dynamic_cast<Gtk::Box*>(&parent)) {
		box->append(child);
	}
	else if(Gtk::Fixed *fixed = dynamic_cast<Gtk::Fixed*>(&parent)) {
		fixed->put(child, 0, 0);
	}
	else {
		child.set_parent(parent);
	}
}

inline void widget_remove_child(Gtk::Widget &parent, Gtk::Widget &child) {
	if(Gtk::Box *box = dynamic_cast<Gtk::Box*>(&parent)) {
		box->remove(child);
	}
	else if(Gtk::Fixed *fixed = dynamic_cast<Gtk::Fixed*>(&parent)) {
		fixed->remove(child);
	}
	else {
		child.unparent();
	}
}

inline void widget_set_cursor(Gtk::Widget &widget, const Glib::RefPtr<Gdk::Cursor> &cursor) {
	widget.set_cursor(cursor);
}

inline Glib::RefPtr<Gtk::GestureClick> compat_click_controller(Gtk::Widget &widget) {
	static std::map<Gtk::Widget*, Glib::RefPtr<Gtk::GestureClick> > controllers;
	std::map<Gtk::Widget*, Glib::RefPtr<Gtk::GestureClick> >::iterator it = controllers.find(&widget);
	if(it != controllers.end())
		return it->second;
	Glib::RefPtr<Gtk::GestureClick> gesture = Gtk::GestureClick::create();
	Gtk::GestureClick *gesture_ptr = gesture.get();
	gesture->set_button(0);
	widget.add_controller(gesture);
	Gtk::Widget *widget_ptr = &widget;
	// The controller remains attached across unrealize/realize cycles.
	// Remove the cache entry only when the widget itself is destroyed;
	// otherwise a later realize would attach a duplicate gesture controller.
	widget.signal_destroy().connect([widget_ptr]() { controllers.erase(widget_ptr); }, false);
	controllers[&widget] = gesture;
	return gesture;
}

template<class Slot>
inline void connect_button_press(Gtk::Widget &widget, Slot slot) {
	Glib::RefPtr<Gtk::GestureClick> gesture = compat_click_controller(widget);
	Gtk::GestureClick *gesture_ptr = gesture.get();
	gesture->signal_pressed().connect([slot, gesture_ptr](int n_press, double x, double y) mutable {
		GdkEventButton event = {};
		event.type = n_press == 2 ? COMPAT_2BUTTON_PRESS : COMPAT_BUTTON_PRESS;
		event.x = x;
		event.y = y;
		event.button = gesture_ptr->get_current_button();
		event.state = (guint)gesture_ptr->get_current_event_state();
		event.time = gesture_ptr->get_current_event_time();
		if(slot(&event))
			gesture_ptr->set_state(Gtk::EventSequenceState::CLAIMED);
	}, false);
}

template<class Slot>
inline void connect_motion_notify(Gtk::Widget &widget, Slot slot) {
	Glib::RefPtr<Gtk::EventControllerMotion> controller = Gtk::EventControllerMotion::create();
	Gtk::EventControllerMotion *controller_ptr = controller.get();
	controller->signal_motion().connect([slot, controller_ptr](double x, double y) mutable {
		GdkEventMotion event = {};
		event.x = x;
		event.y = y;
		event.state = (guint)controller_ptr->get_current_event_state();
		event.time = controller_ptr->get_current_event_time();
		slot(&event);
	}, false);
	widget.add_controller(controller);
}

template<class Slot>
inline void connect_button_release(Gtk::Widget &widget, Slot slot) {
	Glib::RefPtr<Gtk::GestureClick> gesture = compat_click_controller(widget);
	Gtk::GestureClick *gesture_ptr = gesture.get();
	gesture->signal_released().connect([slot, gesture_ptr](int, double x, double y) mutable {
		GdkEventButton event = {};
		event.x = x;
		event.y = y;
		event.button = gesture_ptr->get_current_button();
		event.state = (guint)gesture_ptr->get_current_event_state();
		event.time = gesture_ptr->get_current_event_time();
		if(slot(&event))
			gesture_ptr->set_state(Gtk::EventSequenceState::CLAIMED);
	}, false);
}

template<class Slot>
inline void connect_leave_notify(Gtk::Widget &widget, Slot slot) {
	Glib::RefPtr<Gtk::EventControllerMotion> controller = Gtk::EventControllerMotion::create();
	controller->signal_leave().connect([slot]() mutable {
		GdkEventCrossing event = {};
		slot(&event);
	}, false);
	widget.add_controller(controller);
}

template<class Slot>
inline void connect_key_press(Gtk::Widget &widget, Slot slot) {
	Glib::RefPtr<Gtk::EventControllerKey> controller = Gtk::EventControllerKey::create();
	controller->signal_key_pressed().connect([slot](guint keyval, guint keycode, Gdk::ModifierType state) mutable {
		GdkEventKey event = {};
		event.keyval = keyval;
		event.hardware_keycode = keycode;
		event.state = (guint)state;
		return slot(&event);
	}, false);
	widget.add_controller(controller);
}

template<class Slot>
inline void connect_scroll(Gtk::Widget &widget, Slot slot) {
	Glib::RefPtr<Gtk::EventControllerScroll> controller =
		Gtk::EventControllerScroll::create();
	controller->set_flags(Gtk::EventControllerScroll::Flags::BOTH_AXES);
	Gtk::EventControllerScroll *controller_ptr = controller.get();
	controller->signal_scroll().connect([slot, controller_ptr](double dx, double dy) mutable {
		GdkEventScroll event = {};
		event.delta_x = dx;
		event.delta_y = dy;
		event.state = (guint)controller_ptr->get_current_event_state();
		event.time = controller_ptr->get_current_event_time();
		return slot(&event);
	}, false);
	widget.add_controller(controller);
}

template<class Slot>
inline void connect_size_allocate(Gtk::Widget &widget, Slot slot) {
	if(EventBox *event_box = dynamic_cast<EventBox*>(&widget)) {
		event_box->signal_size_allocate().connect(slot);
		return;
	}
	widget.signal_map().connect([&widget, slot]() mutable {
		Gtk::Allocation allocation = widget.get_allocation();
		slot(allocation);
	}, false);
}

inline Glib::RefPtr<Gdk::Pixbuf> stock_pixbuf(const Glib::ustring &name, int size) {
	Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
	if(!display) return Glib::RefPtr<Gdk::Pixbuf>();
	Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_for_display(display);
	if(!theme) return Glib::RefPtr<Gdk::Pixbuf>();
	Glib::RefPtr<Gtk::IconPaintable> icon = theme->lookup_icon(Gtk::StockIconMap::icon_name(name), compat_icon_pixels(size));
	if(!icon) return Glib::RefPtr<Gdk::Pixbuf>();
	Glib::RefPtr<Gio::File> file = icon->get_file();
	if(!file) return Glib::RefPtr<Gdk::Pixbuf>();
	try {
		return Gdk::Pixbuf::create_from_file(file->get_path());
	} catch(...) {
		return Glib::RefPtr<Gdk::Pixbuf>();
	}
}

inline Gtk::Widget *widget_get_child(Gtk::Widget &widget) { return widget.get_first_child(); }

inline void fixed_add(Gtk::Fixed &fixed, Gtk::Widget &child) { fixed.put(child, 0, 0); }

inline Gtk::TreePath tree_path_at(Gtk::TreeModel &model, int index) {
	Gtk::TreeModel::Children children = model.children();
	Gtk::TreeModel::Children::iterator it = children.begin();
	for(int i = 0; i < index && it != children.end(); ++i)
		++it;
	return model.get_path(it);
}

inline void clipboard_request_text(const sigc::slot<void(const Glib::ustring&)> &slot) {
	Glib::RefPtr<Gdk::Clipboard> clipboard = Gdk::Display::get_default()->get_clipboard();
	clipboard->read_text_async([clipboard, slot](const Glib::RefPtr<Gio::AsyncResult> &result) {
		slot(clipboard->read_text_finish(result));
	});
}

#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
// src/win32_file_dialog.cpp: standard Windows dialogs (kind: 0 open, 1 save, 2 folder)
bool win32_file_dialog(int kind, const std::string &title, const std::string &initial_name,
	const std::string &initial_folder, const std::vector<std::string> &places,
	void *parent, std::string &result);
// GDK Win32 backend: HWND behind a surface
extern "C" void *gdk_win32_surface_get_handle(GdkSurface *surface);

// shortcut folders of each file chooser; the native Windows dialog shows them as places
inline std::map<Gtk::FileChooser*, std::vector<std::string>>& compat_file_places() {
	static std::map<Gtk::FileChooser*, std::vector<std::string>> places;
	return places;
}
#endif

inline void file_chooser_add_shortcut_folder(Gtk::FileChooser &chooser, const Glib::ustring &path) {
	chooser.add_shortcut_folder(Gio::File::create_for_path(path));
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
	compat_file_places()[&chooser].push_back(path.raw());
#endif
}

inline void entry_set_icon_from_stock(Gtk::Entry &entry, const Gtk::StockID &id, EntryIconPosition pos) {
	entry.set_icon_from_icon_name(Gtk::StockIconMap::icon_name(id.get_string()), pos);
}

inline void entry_set_icon_from_stock(Gtk::Entry &entry, const BuiltinStockID &id, EntryIconPosition pos) {
	entry_set_icon_from_stock(entry, Gtk::StockID(id.id), pos);
}

inline Cairo::RefPtr<Cairo::Context> widget_create_cairo_context(Gtk::Widget &) {
	Cairo::RefPtr<Cairo::ImageSurface> surface = Cairo::ImageSurface::create(Cairo::Surface::Format::ARGB32, 1, 1);
	return Cairo::Context::create(surface);
}

inline void image_set_stock(Gtk::Image &image, const BuiltinStockID &id, int size) {
	image_set_stock(image, Gtk::StockID(id.id), size);
}

class ColorSelectionDialog : public Gtk::ColorChooserDialog {
	public:
		explicit ColorSelectionDialog(const Glib::ustring &title): Gtk::ColorChooserDialog(title) {
			add_button("_Cancel", Gtk::ResponseType::CANCEL);
			add_button("_Select", Gtk::ResponseType::OK);
		}
		ColorSelectionDialog *get_color_selection() { return this; }
		void set_previous_rgba(const Gdk::RGBA &color) { set_rgba(color); }
		void set_current_rgba(const Gdk::RGBA &color) { set_rgba(color); }
		Gdk::RGBA get_current_rgba() const { return get_rgba(); }
		void set_has_palette(bool) {}
};

inline std::set<Gtk::Widget*>& compat_no_show_all_set() {
	static std::set<Gtk::Widget*> widgets;
	return widgets;
}

inline void compat_hide_no_show_all(Gtk::Widget &widget) {
	compat_no_show_all_set().insert(&widget);
	widget.hide();
}

inline void compat_show_all(Gtk::Widget &widget) {
	if(dynamic_cast<Gtk::Popover*>(&widget) || compat_no_show_all_set().count(&widget))
		return;
	widget.set_visible(true);

	if(Gtk::ScrolledWindow *sw = dynamic_cast<Gtk::ScrolledWindow*>(&widget)) {
		if(Gtk::Widget *child = sw->get_child())
			compat_show_all(*child);
		return;
	}
	if(Gtk::Frame *frame = dynamic_cast<Gtk::Frame*>(&widget)) {
		if(Gtk::Widget *child = frame->get_child())
			compat_show_all(*child);
		return;
	}
	if(Gtk::Window *window = dynamic_cast<Gtk::Window*>(&widget)) {
		if(Gtk::Widget *child = window->get_child())
			compat_show_all(*child);
		return;
	}
	if(Gtk::Viewport *viewport = dynamic_cast<Gtk::Viewport*>(&widget)) {
		if(Gtk::Widget *child = viewport->get_child())
			compat_show_all(*child);
		return;
	}
	if(Gtk::Expander *expander = dynamic_cast<Gtk::Expander*>(&widget)) {
		if(Gtk::Widget *child = expander->get_child())
			compat_show_all(*child);
		return;
	}
	if(Gtk::Notebook *notebook = dynamic_cast<Gtk::Notebook*>(&widget)) {
		for(int page = 0; page < notebook->get_n_pages(); ++page)
			if(Gtk::Widget *child = notebook->get_nth_page(page))
				compat_show_all(*child);
		return;
	}

	const bool container = dynamic_cast<Gtk::Box*>(&widget) || dynamic_cast<Gtk::Paned*>(&widget) ||
			dynamic_cast<Gtk::Fixed*>(&widget) ||
			dynamic_cast<Gtk::Stack*>(&widget) || dynamic_cast<Gtk::ListBox*>(&widget) ||
			dynamic_cast<Gtk::Grid*>(&widget);
	if(!container)
		return;
	for(Gtk::Widget *child = widget.get_first_child(); child; child = child->get_next_sibling())
		compat_show_all(*child);
}

class Main {
	private:
		static Glib::RefPtr<Gtk::Application>& app_ref() {
			// Each editor process owns its windows. There is no activate/open
			// forwarding handler for another process (including portable builds).
			static Glib::RefPtr<Gtk::Application> app = Gtk::Application::create(
				"com.hiasm.studio", Gio::Application::Flags::NON_UNIQUE);
			return app;
		}
	public:
		Main(int, char **) {
			if(!app_ref()->is_registered())
				app_ref()->register_application();
		}
		static void run(Gtk::Window &window) {
			Glib::RefPtr<Gtk::Application> app = app_ref();
			if(!app->is_registered())
				app->register_application();
			app->add_window(window);
			compat_standard_frame(window);
			window.show();
			Glib::signal_timeout().connect_once([&window]() {
				compat_show_all(window);
				window.queue_resize();
				window.queue_draw();
				window.present();
			}, 50);
			app->run();
		}
		static bool events_pending() { return Glib::MainContext::get_default()->pending(); }
		static bool iteration() { return Glib::MainContext::get_default()->iteration(false); }
};

inline int dialog_run(Gtk::Dialog &dialog);
inline Glib::ustring file_chooser_get_filename(Gtk::FileChooser &chooser);

class FileChooserButton : public Gtk::Button {
	private:
		Gtk::FileChooser::Action action_;
		Glib::ustring filename_;
		sigc::signal<void()> signal_file_set_;
	public:
		FileChooserButton(const Glib::ustring &title, Gtk::FileChooser::Action action):
			Gtk::Button(title), action_(action) {
			signal_clicked().connect(sigc::mem_fun(*this, &FileChooserButton::choose));
		}
		sigc::signal<void()>& signal_file_set() { return signal_file_set_; }
		void set_action(Gtk::FileChooser::Action action) { action_ = action; }
		Glib::ustring get_filename() const { return filename_; }
		void choose() {
			Gtk::FileChooserDialog dialog("Select file", action_);
			dialog.add_button("_Cancel", Gtk::ResponseType::CANCEL);
			dialog.add_button("_Open", Gtk::ResponseType::ACCEPT);
			int response = dialog_run(dialog);
			if(response == 1 || response == Gtk::ResponseType::ACCEPT) {	// 1: native Windows dialog
				filename_ = file_chooser_get_filename(dialog);
				signal_file_set_();
			}
		}
};

inline void tool_button_set_icon(Gtk::Widget *button, Gtk::Widget &image) {
	if(HToolButton *b = dynamic_cast<HToolButton*>(button))
		b->set_icon_widget(image);
	else if(HToggleToolButton *t = dynamic_cast<HToggleToolButton*>(button))
		t->set_icon_widget(image);
	else if(HMenuToolButton *m = dynamic_cast<HMenuToolButton*>(button))
		m->set_icon_widget(image);
}

#endif // HIASM_GTK4

#ifndef HIASM_GTK4

using EntryIconPosition = Gtk::EntryIconPosition;

template <class WidgetT>
inline void widget_set_events(WidgetT &widget, int events) { widget.set_events((Gdk::EventMask)events); }

template <class WidgetT>
inline void widget_add_events(WidgetT &widget, int events) { widget.add_events((Gdk::EventMask)events); }

inline void compat_pack(Gtk::Box &box, Gtk::Widget &child, bool expand, bool fill, guint padding) {
	box.pack_start(child, expand, fill, padding);
}

inline void compat_hide_no_show_all(Gtk::Widget &widget) {
	widget.set_no_show_all(true);
	widget.hide();
}

inline void compat_show_all(Gtk::Widget &widget) {
	widget.show_all();
}

inline void compat_show_uri(const Glib::ustring &uri) {
	gtk_show_uri(Gdk::Screen::get_default()->gobj(), uri.c_str(), GDK_CURRENT_TIME, NULL);
}

inline Gtk::FileChooserAction file_chooser_action(int value) { return (Gtk::FileChooserAction)value; }
inline Glib::RefPtr<Gdk::Pixbuf> stock_pixbuf(const Glib::ustring &name, int size) {
	Glib::RefPtr<Gtk::IconTheme> theme = Gtk::IconTheme::get_default();
	if(!theme) return Glib::RefPtr<Gdk::Pixbuf>();
	try {
		return theme->load_icon(name, compat_icon_pixels(size));
	} catch(...) {
		return Glib::RefPtr<Gdk::Pixbuf>();
	}
}

inline Gtk::Widget *widget_get_child(Gtk::Widget &widget) {
	Gtk::Container *container = dynamic_cast<Gtk::Container*>(&widget);
	if(!container) return NULL;
	std::vector<Gtk::Widget*> children = container->get_children();
	return children.empty() ? NULL : children.front();
}

inline void fixed_add(Gtk::Fixed &fixed, Gtk::Widget &child) { fixed.put(child, 0, 0); }

inline Gtk::TreePath tree_path_at(Gtk::TreeModel &model, int index) {
	Gtk::TreeModel::Children children = model.children();
	Gtk::TreeModel::Children::iterator it = children.begin();
	for(int i = 0; i < index && it != children.end(); ++i)
		++it;
	return model.get_path(it);
}

inline void clipboard_request_text(const sigc::slot<void(const Glib::ustring&)> &slot) {
	Gtk::Clipboard::get()->request_text(slot);
}

inline void file_chooser_add_shortcut_folder(Gtk::FileChooser &chooser, const Glib::ustring &path) {
	chooser.add_shortcut_folder(path);
}

inline void entry_set_icon_from_stock(Gtk::Entry &entry, const Gtk::StockID &id, EntryIconPosition pos) {
	entry.set_icon_from_stock(id, pos);
}

inline Cairo::RefPtr<Cairo::Context> widget_create_cairo_context(Gtk::Widget &widget) {
	return widget.get_window()->create_cairo_context();
}

inline Glib::RefPtr<Gdk::Cursor> compat_cursor_create_from_pixbuf(const Glib::RefPtr<Gdk::Display> &display, const Glib::RefPtr<Gdk::Pixbuf> &pix, int x, int y) {
	return Gdk::Cursor::create(display, pix, x, y);
}

inline void widget_add_child(Gtk::Widget &parent, Gtk::Widget &child) {
	if(Gtk::Container *container = dynamic_cast<Gtk::Container*>(&parent))
		container->add(child);
}

inline void widget_remove_child(Gtk::Widget &parent, Gtk::Widget &child) {
	if(Gtk::Container *container = dynamic_cast<Gtk::Container*>(&parent))
		container->remove(child);
}

inline void widget_set_cursor(Gtk::Widget &widget, const Glib::RefPtr<Gdk::Cursor> &cursor) {
	if(widget.get_window())
		widget.get_window()->set_cursor(cursor);
}

template<class Slot>
inline void connect_button_press(Gtk::Widget &widget, Slot slot) {
	widget.signal_button_press_event().connect(slot);
}

template<class Slot>
inline void connect_motion_notify(Gtk::Widget &widget, Slot slot) {
	widget.signal_motion_notify_event().connect(slot);
}

template<class Slot>
inline void connect_key_press(Gtk::Widget &widget, Slot slot) {
	widget.signal_key_press_event().connect(slot);
}

template<class Slot>
inline void connect_scroll(Gtk::Widget &widget, Slot slot) {
	(void)widget;
	(void)slot;
}

template<class Slot>
inline void connect_size_allocate(Gtk::Widget &widget, Slot slot) {
	widget.signal_size_allocate().connect(slot);
}

inline void tool_button_set_icon(Gtk::Widget *button, Gtk::Widget &image) {
	if(Gtk::ToolButton *b = dynamic_cast<Gtk::ToolButton*>(button))
		b->set_icon_widget(image);
}

#endif // !HIASM_GTK4

inline void scrolled_set_child(Gtk::ScrolledWindow &scroll, Gtk::Widget &child) {
#ifdef HIASM_GTK4
	scroll.set_child(child);
#else
	scroll.add(child);
#endif
}

// ScrolledWindow scrollbar widgets were removed from GTK4. Keep callers
// working with the underlying adjustments on both GTK generations.
inline double scrolled_get_hscroll(Gtk::ScrolledWindow &scroll) {
	Glib::RefPtr<Gtk::Adjustment> adjustment = scroll.get_hadjustment();
	return adjustment ? adjustment->get_value() : 0.0;
}

inline double scrolled_get_vscroll(Gtk::ScrolledWindow &scroll) {
	Glib::RefPtr<Gtk::Adjustment> adjustment = scroll.get_vadjustment();
	return adjustment ? adjustment->get_value() : 0.0;
}

inline void scrolled_set_hscroll(Gtk::ScrolledWindow &scroll, double value) {
	if(Glib::RefPtr<Gtk::Adjustment> adjustment = scroll.get_hadjustment())
		adjustment->set_value(value);
}

inline void scrolled_set_vscroll(Gtk::ScrolledWindow &scroll, double value) {
	if(Glib::RefPtr<Gtk::Adjustment> adjustment = scroll.get_vadjustment())
		adjustment->set_value(value);
}

inline void frame_set_child(Gtk::Frame &frame, Gtk::Widget &child) {
#ifdef HIASM_GTK4
	frame.set_child(child);
#else
	frame.add(child);
#endif
}

inline void expander_set_child(Gtk::Expander &expander, Gtk::Widget &child) {
#ifdef HIASM_GTK4
	expander.set_child(child);
#else
	expander.add(child);
#endif
}

inline void expander_set_spacing(Gtk::Expander &, int) {
}

inline void paned_pack1(Gtk::Paned &paned, Gtk::Widget &child, bool resize, bool shrink) {
#ifdef HIASM_GTK4
	paned.set_start_child(child);
	paned.set_resize_start_child(resize);
	paned.set_shrink_start_child(shrink);
#else
	paned.pack1(child, resize, shrink);
#endif
}

inline void paned_pack2(Gtk::Paned &paned, Gtk::Widget &child, bool resize, bool shrink) {
#ifdef HIASM_GTK4
	paned.set_end_child(child);
	paned.set_resize_end_child(resize);
	paned.set_shrink_end_child(shrink);
#else
	paned.pack2(child, resize, shrink);
#endif
}

inline void clipboard_set_text(const Glib::ustring &text) {
#ifdef HIASM_GTK4
	Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
	if(display)
		display->get_clipboard()->set_text(text);
#else
	Gtk::Clipboard::get()->set_text(text);
#endif
}

inline Glib::ustring clipboard_wait_for_text() {
#ifdef HIASM_GTK4
	Glib::RefPtr<Gdk::Display> display = Gdk::Display::get_default();
	if(!display) return Glib::ustring();
	Glib::RefPtr<Gdk::Clipboard> clipboard = display->get_clipboard();
	struct ClipboardReadState {
		Glib::ustring text;
		bool done = false;
	};
	std::shared_ptr<ClipboardReadState> state(new ClipboardReadState());
	clipboard->read_text_async([clipboard, state](const Glib::RefPtr<Gio::AsyncResult> &result) {
		try {
			state->text = clipboard->read_text_finish(result);
		} catch(...) {
		}
		state->done = true;
	});
	const gint64 deadline = g_get_monotonic_time() + 2 * G_TIME_SPAN_SECOND;
	while(!state->done && g_get_monotonic_time() < deadline) {
		while(Glib::MainContext::get_default()->iteration(false)) {}
		g_usleep(1000);
	}
	return state->text;
#else
	return Gtk::Clipboard::get()->wait_for_text();
#endif
}

template<class Slot>
inline void compat_invoke_ui(Slot slot) {
#ifdef HIASM_GTK4
	Glib::MainContext::get_default()->invoke([slot]() mutable {
		slot();
		return false;
	});
#else
	slot();
#endif
}

inline void box_pack_start(Gtk::Box &box, Gtk::Widget &child, bool expand, bool fill, guint padding) {
#ifdef HIASM_GTK4
	compat_pack(box, child, expand, fill, padding);
#else
	box.pack_start(child, expand, fill, padding);
#endif
}

inline void box_pack_end(Gtk::Box &box, Gtk::Widget &child, bool expand, bool fill, guint padding) {
#ifdef HIASM_GTK4
	compat_pack(box, child, expand, fill, padding);
	const bool horizontal = box.get_orientation() == Gtk::Orientation::HORIZONTAL;
	if(horizontal)
		child.set_halign(Gtk::Align::END);
	else
		child.set_valign(Gtk::Align::END);
#else
	box.pack_end(child, expand, fill, padding);
#endif
}

inline void widget_invalidate(Gtk::Widget &widget, bool = false) {
#ifdef HIASM_GTK4
	widget.queue_draw();
#else
	if(widget.get_window())
		widget.get_window()->invalidate(false);
#endif
}

inline void widget_invalidate_rect(Gtk::Widget &widget, const Gdk::Rectangle &rect, bool = false) {
#ifdef HIASM_GTK4
	widget.queue_draw();
#else
	if(widget.get_window())
		widget.get_window()->invalidate_rect(rect, false);
#endif
}

inline void file_chooser_set_current_folder(Gtk::FileChooser &chooser, const Glib::ustring &path) {
#ifdef HIASM_GTK4
	chooser.set_current_folder(Gio::File::create_for_path(path));
#else
	chooser.set_current_folder(path);
#endif
}

inline void file_chooser_set_filename(Gtk::FileChooser &chooser, const Glib::ustring &path) {
#ifdef HIASM_GTK4
	chooser.set_file(Gio::File::create_for_path(path));
#else
	chooser.set_filename(path);
#endif
}

inline Glib::ustring file_chooser_get_filename(Gtk::FileChooser &chooser) {
#ifdef HIASM_GTK4
	Glib::RefPtr<Gio::File> file = chooser.get_file();
	if(!file)
		return Glib::ustring();
	return file->get_path();
#else
	return chooser.get_filename();
#endif
}

inline void message_dialog_set_image(Gtk::MessageDialog &dialog, Gtk::Widget &image) {
#ifdef HIASM_GTK4
	if(Gtk::Box *area = dialog.get_message_area())
		area->prepend(image);
#else
	dialog.set_image(image);
#endif
}

// BorderStyle indexes of the Delphi list: 0 bsNone, 1 bsSingle, 2 bsSizeable, 3 bsDialog,
// 4 bsToolWindow, 5 bsSizeToolWin, 6 bsBox, 7 bsSizeBox, 8 bsBoxEdge, 9 bsSizeBoxEdge.
inline bool compat_border_has_caption(int border) { return border >= 1 && border <= 5; }
inline bool compat_border_is_sizeable(int border) { return border == 2 || border == 5 || border == 7 || border == 9; }

// Windows: the Win32 style of a GTK4 window is rewritten by GTK on layout, so the native frame
// (the border index kept in "hiasm-border") is applied again after realize/map and on theme changes.
inline void compat_apply_native_frame(Gtk::Window &window) {
#if defined(G_OS_WIN32) && defined(HIASM_GTK4)
	// Without a caption GTK's own undecorated style is kept.
	if(!compat_border_has_caption(GPOINTER_TO_INT(g_object_get_data(G_OBJECT(window.gobj()), "hiasm-border"))))
		return;
	Glib::RefPtr<Gdk::Surface> surface = window.get_surface();
	if(!surface)
		return;
	HWND hwnd = static_cast<HWND>(gdk_win32_surface_get_handle(surface->gobj()));
	if(!hwnd)
		return;
	// GTK may initially use a popup/child surface for client-side
	// decorations. Replace those bits with a regular overlapped form.
	LONG_PTR style = GetWindowLongPtrW(hwnd, GWL_STYLE);
	style |= WS_OVERLAPPEDWINDOW;
	style &= ~static_cast<LONG_PTR>(WS_CHILD | WS_POPUP);
	if(!window.get_resizable())
		style &= ~static_cast<LONG_PTR>(WS_THICKFRAME | WS_MAXIMIZEBOX);
	SetWindowLongPtrW(hwnd, GWL_STYLE, style);
	LONG_PTR ex_style = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
	ex_style &= ~static_cast<LONG_PTR>(WS_EX_TOOLWINDOW);
	// Owned dialogs stay associated with their parent and do not create a
	// second taskbar button.
	if(!window.get_transient_for())
		ex_style |= WS_EX_APPWINDOW;
	SetWindowLongPtrW(hwnd, GWL_EXSTYLE, ex_style);
	// Keep the Win32 caption consistent with the selected GTK theme.
	if(HMODULE dwm = LoadLibraryW(L"dwmapi.dll")) {
		typedef HRESULT (WINAPI *SetAttribute)(HWND, DWORD, LPCVOID, DWORD);
		auto setAttribute = reinterpret_cast<SetAttribute>(GetProcAddress(dwm, "DwmSetWindowAttribute"));
		BOOL dark = themePrefersDark();
		if(setAttribute) setAttribute(hwnd, 20, &dark, sizeof(dark));
		FreeLibrary(dwm);
	}
	SetWindowPos(hwnd, NULL, 0, 0, 0, 0,
		SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
#else
	(void)window;
#endif
}

// The window gets the frame of the Delphi BorderStyle index (0..9); GTK4 otherwise draws its own
// client-side header bar. Sizing is left to the caller.
inline void compat_native_frame(Gtk::Window &window, int border) {
	g_object_set_data(G_OBJECT(window.gobj()), "hiasm-border", GINT_TO_POINTER(border));
	gtk_window_set_decorated(window.gobj(), compat_border_has_caption(border));
#ifdef G_OS_WIN32
#ifdef HIASM_GTK4
	g_setenv("GTK_CSD", "0", TRUE);
#endif
	// A null titlebar disables GTK's client-side decoration.
	gtk_window_set_titlebar(window.gobj(), NULL);
	if(!g_object_get_data(G_OBJECT(window.gobj()), "hiasm-native-frame")) {
		g_object_set_data(G_OBJECT(window.gobj()), "hiasm-native-frame", GINT_TO_POINTER(1));
#if defined(HIASM_GTK4)
		// GTK4 creates the Win32 surface lazily, so the style is enforced again after realize/map.
		Gtk::Window *w = &window;
		window.signal_realize().connect([w]() { compat_apply_native_frame(*w); }, false);
		window.signal_map().connect([w]() { compat_apply_native_frame(*w); }, false);
		Gtk::Settings::get_default()->property_gtk_application_prefer_dark_theme().signal_changed().connect(
			sigc::track_obj([w]() { compat_apply_native_frame(*w); }, window));
		if(g_object_class_find_property(G_OBJECT_GET_CLASS(gtk_settings_get_default()), "gtk-interface-color-scheme"))
			Gtk::Settings::get_default()->connect_property_changed("gtk-interface-color-scheme",
				sigc::track_obj([w]() { compat_apply_native_frame(*w); }, window));
#endif
	}
	compat_apply_native_frame(window);
#else
	(void)window;
#endif
}

// Windows: the standard window frame and title bar (dialogs, IDE windows).
inline void compat_standard_frame(Gtk::Window &window) {
	compat_native_frame(window, 2);
}

inline int dialog_run(Gtk::Dialog &dialog) {
#if defined(HIASM_GTK4) && defined(G_OS_WIN32)
	// file choosers: the standard Windows dialog; the picked path goes back into the chooser
	if(Gtk::FileChooserDialog *chooser = dynamic_cast<Gtk::FileChooserDialog*>(&dialog)) {
		Gtk::FileChooser::Action action = chooser->get_action();
		int kind = action == Gtk::FileChooser::Action::SAVE ? 1
			: (action == Gtk::FileChooser::Action::SELECT_FOLDER ? 2 : 0);
		std::string initial_name, initial_folder, path;
		if(Glib::RefPtr<Gio::File> file = chooser->get_file())
			initial_name = file->get_basename();
		if(Glib::RefPtr<Gio::File> folder = chooser->get_current_folder())
			initial_folder = folder->get_path();
		void *parent = NULL;
		for(Gtk::Window *w : Gtk::Window::list_toplevels())
			if(w != &dialog && w->get_visible()) {
				if(Glib::RefPtr<Gdk::Surface> surface = w->get_surface())
					parent = gdk_win32_surface_get_handle(surface->gobj());
				break;
			}
		std::vector<std::string> places;
		std::map<Gtk::FileChooser*, std::vector<std::string>>::iterator found = compat_file_places().find(chooser);
		if(found != compat_file_places().end())
			places = found->second;
		if(!win32_file_dialog(kind, chooser->get_title().raw(), initial_name, initial_folder, places, parent, path))
			return 0;
		chooser->set_file(Gio::File::create_for_path(path));
		return 1;
	}
#endif
	int result = 0;
	Glib::RefPtr<Glib::MainLoop> loop = Glib::MainLoop::create();
	sigc::connection connection = dialog.signal_response().connect([&](int response) {
		result = response;
		loop->quit();
	}, false);
	dialog.set_modal(true);
	compat_standard_frame(dialog);
	if(!dialog.get_transient_for())
		for(Gtk::Window *w : Gtk::Window::list_toplevels())
			if(w != &dialog && w->get_visible()) {
				dialog.set_transient_for(*w);
				break;
			}
	dialog.show();
	loop->run();
	connection.disconnect();
	dialog.hide();
	return result;
}

inline Gtk::Widget *paned_get_child1(Gtk::Paned &paned) {
#ifdef HIASM_GTK4
	return paned.get_start_child();
#else
	return paned.get_child1();
#endif
}

#ifndef HIASM_GTK4

inline Gtk::Image *stock_image(const Gtk::StockID &id, int size) {
	return new Gtk::Image(id, (Gtk::IconSize)size);
}

inline Gtk::Image *stock_image(const Gtk::BuiltinStockID &id, int size) {
	return new Gtk::Image(Gtk::StockID(id.id), (Gtk::IconSize)size);
}

inline void image_set_stock(Gtk::Image &image, const Gtk::StockID &id, int size) {
	image.set(id, (Gtk::IconSize)size);
}

inline void image_set_stock(Gtk::Image &image, const Gtk::BuiltinStockID &id, int size) {
	image.set(Gtk::StockID(id.id), (Gtk::IconSize)size);
}

inline void button_set_image(Gtk::Button &button, Gtk::Widget &image) {
	button.set_image(image);
}

inline void button_set_image_position(Gtk::Button &button, Gtk::PositionType pos) {
	button.set_image_position(pos);
}

#endif // !HIASM_GTK4

#endif // HIASM_GTKCOMPAT_H_
