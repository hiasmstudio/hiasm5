#ifndef HIASM_DOCK_WORKSPACE_H
#define HIASM_DOCK_WORKSPACE_H

#ifdef HIASM_GTK4
#include <gtkmm.h>
#include <memory>
#include <vector>

enum class DockSide { Left = 0, Right = 1, Bottom = 2 };
class DockWorkspace;

// A panel is a child widget, including when it floats above the editor.
class DockPanel : public Gtk::Box {
public:
    DockPanel(const Glib::ustring &title, Gtk::Widget &content, DockSide side, int width,
              const Glib::ustring &icon = "view-list-symbolic");
    bool is_pinned() const { return pinned_; }
    bool is_panel_visible() const { return visible_; }
    bool is_active() const { return active_; }
    DockSide side() const { return side_; }
    int preferred_width() const { return preferredWidth_; }
    int preferred_height() const { return preferredHeight_; }
    void set_preferred_width(int width) { preferredWidth_ = width; }
    void set_preferred_height(int height) { preferredHeight_ = height; }
    double float_x() const { return floatX_; }
    double float_y() const { return floatY_; }
    int float_width() const { return floatWidth_; }
    int float_height() const { return floatHeight_; }
    void set_float_geometry(double x, double y, int width, int height) {
        floatX_ = x; floatY_ = y; floatWidth_ = width; floatHeight_ = height;
    }
    const Glib::ustring &title() const { return title_; }
    Gtk::Widget &header() { return header_; }
    Gtk::Button &close_button() { return close_; }
    Gtk::Button &pin_button() { return pin_; }
    Gtk::Button &collapsed_button() { return collapsed_; }
private:
    friend class DockWorkspace;
    void refresh_header();
    Glib::ustring title_;
    Glib::ustring iconName_;
    Gtk::Box header_{Gtk::Orientation::HORIZONTAL};
    Gtk::DrawingArea caption_;
    Gtk::DrawingArea collapsedCaption_;
    Gtk::Image collapsedIcon_;
    Gtk::Box collapsedContent_{Gtk::Orientation::VERTICAL};
    Gtk::Button collapsed_;
    Gtk::Button close_{"×"};
    Gtk::Button pin_;
    Gtk::Widget &content_;
    bool pinned_ = true;
    bool visible_ = true;
    bool active_ = true;
    DockSide side_;
    int preferredWidth_;
    int preferredHeight_;
    double floatX_ = 40, floatY_ = 40;
    int floatWidth_, floatHeight_ = 340;
};

class DockWorkspace : public Gtk::Overlay {
public:
    DockWorkspace(Gtk::Widget &editor, Gtk::Widget &debug);
    ~DockWorkspace() override;
    DockPanel &add_panel(const Glib::ustring &title, Gtk::Widget &content, DockSide side, int width,
                         const Glib::ustring &icon = "view-list-symbolic");
    void set_state(DockPanel &panel, bool visible, bool pinned, DockSide side);
    void set_visible(DockPanel &panel, bool visible);
    void activate_panel(DockPanel &panel);
    void set_pinned(DockPanel &panel, bool pinned);
    void update_layout();
    sigc::signal<void()> &signal_changed() { return changed_; }

    // The workspace owns the gesture, so reparenting a panel cannot cancel it.
    void begin_drag(DockPanel &panel, double x, double y);
    void update_drag(double x, double y);
    void end_drag(bool cancel = false);
    int preview_side() const { return previewSide_; }
private:
    void detach(DockPanel &panel);
    void place_float(DockPanel &panel);
    void draw_preview(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height);
    DockPanel *header_at(double x, double y);
    DockPanel *resize_panel_at(double x, double y);
    Gtk::Box layout_{Gtk::Orientation::VERTICAL};
    Gtk::Box mainRow_{Gtk::Orientation::HORIZONTAL};
    Gtk::Box left_{Gtk::Orientation::VERTICAL};
    Gtk::Box right_{Gtk::Orientation::VERTICAL};
    Gtk::Box bottom_{Gtk::Orientation::HORIZONTAL};
    Gtk::Box bottomArea_{Gtk::Orientation::VERTICAL};
    Gtk::Box leftTabs_{Gtk::Orientation::VERTICAL};
    Gtk::Box rightTabs_{Gtk::Orientation::VERTICAL};
    Gtk::Box bottomTabs_{Gtk::Orientation::HORIZONTAL};
    Gtk::Box parkedPanels_{Gtk::Orientation::VERTICAL};
    Gtk::Paned editor_{Gtk::Orientation::VERTICAL};
    Gtk::DrawingArea preview_;
    std::vector<std::unique_ptr<DockPanel>> panels_;
    sigc::signal<void()> changed_;
    DockPanel *drag_ = nullptr;
    DockPanel *resizePanel_ = nullptr;
    bool resizeActive_ = false;
    double resizeStartX_ = 0, resizeStartY_ = 0;
    int resizeStartSize_ = 0;
    bool dragActive_ = false, originalPinned_ = true;
    DockSide originalSide_ = DockSide::Left;
    double startX_ = 0, startY_ = 0, grabX_ = 0, grabY_ = 0;
    double originalX_ = 0, originalY_ = 0;
    int previewSide_ = -1, lastWidth_ = 0, lastHeight_ = 0;
};
#endif
#endif
