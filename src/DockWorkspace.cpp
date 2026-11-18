#include "DockWorkspace.h"
#ifdef HIASM_GTK4
#include "Localization.h"
#include <algorithm>
#include <cmath>

DockPanel::DockPanel(const Glib::ustring &title, Gtk::Widget &content, DockSide side, int width,
                     const Glib::ustring &icon)
    : Gtk::Box(Gtk::Orientation::HORIZONTAL), title_(title), iconName_(icon), content_(content),
      side_(side), preferredWidth_(width), preferredHeight_(180), floatWidth_(width) {
    add_css_class("dock-panel");
    header_.add_css_class("dock-header");
    header_.set_cursor("grab");
    header_.append(close_);
    header_.append(pin_);
    header_.append(caption_);
    append(header_);
    append(content_);
    collapsed_.set_has_frame(false);
    collapsed_.set_tooltip_text(title_);
    collapsedIcon_.set_from_icon_name(iconName_);
    collapsedIcon_.set_pixel_size(16);
    collapsedContent_.append(collapsedIcon_);
    collapsedContent_.append(collapsedCaption_);
    collapsed_.set_child(collapsedContent_);
    collapsed_.set_size_request(28, 96);
    collapsedCaption_.set_size_request(24, 68);
    collapsedCaption_.set_draw_func([this](const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) {
        auto text = create_pango_layout(title_);
        const bool vertical = side_ != DockSide::Bottom;
        text->set_width(std::max(1, (vertical ? height : width) - 4) * Pango::SCALE);
        text->set_ellipsize(Pango::EllipsizeMode::END);
        text->set_single_paragraph_mode(true);
        int tw = 0, th = 0; text->get_pixel_size(tw, th);
        auto color = get_style_context()->get_color();
        cr->set_source_rgba(color.get_red(), color.get_green(), color.get_blue(), color.get_alpha());
        cr->save();
        if(vertical) {
            cr->translate(0, height); cr->rotate(-1.5707963267948966);
            cr->move_to(2, std::max(0, (width - th) / 2));
        } else {
            cr->move_to(2, std::max(0, (height - th) / 2));
        }
        text->show_in_cairo_context(cr); cr->restore();
    });
    content_.set_hexpand(true);
    content_.set_vexpand(true);
    caption_.set_can_target(false);
    caption_.set_draw_func([this](const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) {
        const bool vertical = pinned_ && side_ == DockSide::Bottom;
        auto text = create_pango_layout(title_);
        text->set_width(std::max(1, (vertical ? height : width) - 12) * Pango::SCALE);
        text->set_ellipsize(Pango::EllipsizeMode::END);
        text->set_single_paragraph_mode(true);
        int tw = 0, th = 0;
        text->get_pixel_size(tw, th);
        auto color = get_style_context()->get_color();
        cr->set_source_rgba(color.get_red(), color.get_green(), color.get_blue(), color.get_alpha());
        cr->save();
        if(vertical) {
            cr->translate(0, height);
            cr->rotate(-1.5707963267948966);
            cr->move_to(6, std::max(0, (width - th) / 2));
        } else {
            cr->move_to(6, std::max(0, (height - th) / 2));
        }
        text->show_in_cairo_context(cr);
        cr->restore();
    });
    for(auto button : {&close_, &pin_}) {
        button->set_has_frame(false);
        button->set_size_request(24, 24);
    }
    close_.set_tooltip_text(ui_tr("Hide panel"));
    refresh_header();
}

void DockPanel::refresh_header() {
    const bool vertical = pinned_ && side_ == DockSide::Bottom;
    set_orientation(vertical ? Gtk::Orientation::HORIZONTAL : Gtk::Orientation::VERTICAL);
    header_.set_orientation(vertical ? Gtk::Orientation::VERTICAL : Gtk::Orientation::HORIZONTAL);
    collapsedContent_.set_orientation(side_ == DockSide::Bottom ? Gtk::Orientation::HORIZONTAL : Gtk::Orientation::VERTICAL);
    const bool collapsedVertical = side_ != DockSide::Bottom;
    collapsed_.set_size_request(collapsedVertical ? 28 : 110, collapsedVertical ? 96 : 30);
    collapsedCaption_.set_size_request(collapsedVertical ? 24 : 80, collapsedVertical ? 68 : 24);
    header_.set_size_request(vertical ? 28 : -1, vertical ? -1 : 28);
    header_.set_hexpand(!vertical);
    header_.set_vexpand(vertical);
    caption_.set_size_request(vertical ? 24 : 40, vertical ? 40 : 24);
    caption_.set_hexpand(!vertical);
    caption_.set_vexpand(vertical);
    if(vertical) {
        header_.reorder_child_at_start(close_);
        header_.reorder_child_after(pin_, close_);
        header_.reorder_child_after(caption_, pin_);
        header_.add_css_class("dock-header-vertical");
    } else {
        header_.reorder_child_at_start(caption_);
        header_.reorder_child_after(pin_, caption_);
        header_.reorder_child_after(close_, pin_);
        header_.remove_css_class("dock-header-vertical");
    }
    pin_.set_label(pinned_ ? "»" : "«");
    pin_.set_tooltip_text(ui_tr(pinned_ ? "Detach panel" : "Attach panel"));
    if(pinned_) remove_css_class("dock-floating"); else add_css_class("dock-floating");
    caption_.queue_draw();
    collapsedCaption_.queue_draw();
}

DockWorkspace::DockWorkspace(Gtk::Widget &editor, Gtk::Widget &debug) {
    set_hexpand(true);
    set_vexpand(true);
    set_overflow(Gtk::Overflow::HIDDEN);
    layout_.set_hexpand(true);
    layout_.set_vexpand(true);
    mainRow_.set_hexpand(true);
    mainRow_.set_vexpand(true);
    editor_.set_start_child(editor);
    // The debug widget is managed by a DockPanel below. Keeping it out of the
    // editor Paned avoids assigning the same GTK widget to two parents.
    (void)debug;
    editor_.set_resize_start_child(true);
    editor_.set_resize_end_child(false);
    editor_.set_shrink_start_child(false);
    editor_.set_shrink_end_child(false);
    editor_.set_hexpand(true);
    editor_.set_vexpand(true);
    left_.set_visible(false);
    right_.set_visible(false);
    bottom_.set_visible(false);
    bottomArea_.set_visible(false);
    left_.set_hexpand(false);
    right_.set_hexpand(false);
    bottom_.set_vexpand(false);
    bottomArea_.set_hexpand(true);
    bottomArea_.set_vexpand(false);
    mainRow_.append(left_);
    mainRow_.append(editor_);
    mainRow_.append(right_);
    layout_.append(mainRow_);
    bottomArea_.append(bottom_);
    layout_.append(bottomArea_);
    parkedPanels_.set_visible(false);
    layout_.append(parkedPanels_);
    set_child(layout_);
    preview_.set_can_target(false);
    preview_.set_draw_func(sigc::mem_fun(*this, &DockWorkspace::draw_preview));
    add_overlay(preview_);
    set_measure_overlay(preview_, false);

    auto gesture = Gtk::GestureDrag::create();
    auto gesturePtr = gesture.get();
    gesture->set_button(1);
    gesture->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    gesture->signal_drag_begin().connect([this, gesturePtr](double x, double y) {
        if(auto panel = resize_panel_at(x, y)) {
            resizePanel_ = panel;
            resizeActive_ = false;
            resizeStartX_ = x;
            resizeStartY_ = y;
            resizeStartSize_ = panel->side_ == DockSide::Bottom ? panel->preferredHeight_ : panel->preferredWidth_;
            panel->set_cursor(panel->side_ == DockSide::Bottom ? "ns-resize" : "ew-resize");
            gesturePtr->set_state(Gtk::EventSequenceState::CLAIMED);
            return;
        }
        auto panel = header_at(x, y);
        if(!panel) { gesturePtr->set_state(Gtk::EventSequenceState::DENIED); return; }
        begin_drag(*panel, x, y);
        gesturePtr->set_state(Gtk::EventSequenceState::CLAIMED);
    });
    gesture->signal_drag_update().connect([this](double dx, double dy) {
        if(resizePanel_) {
            auto *panel = resizePanel_;
            if(panel->side_ == DockSide::Bottom) {
                const int maxSize = std::max(100, get_height() - 100);
                panel->preferredHeight_ = std::clamp(resizeStartSize_ - int(std::lround(dy)), 100, maxSize);
            } else {
                const int direction = panel->side_ == DockSide::Left ? 1 : -1;
                const int maxSize = std::max(120, get_width() / 2);
                panel->preferredWidth_ = std::clamp(resizeStartSize_ + direction * int(std::lround(dx)), 120, maxSize);
            }
            resizeActive_ = true;
            if(panel->side_ == DockSide::Bottom)
                panel->set_size_request(160, panel->preferredHeight_);
            else
                panel->set_size_request(panel->preferredWidth_, 100);
            panel->queue_resize();
            return;
        }
        if(drag_) update_drag(startX_ + dx, startY_ + dy);
    });
    gesture->signal_drag_end().connect([this](double dx, double dy) {
        if(resizePanel_) {
            auto *panel = resizePanel_;
            panel->set_cursor("default");
            resizePanel_ = nullptr;
            const bool changed = resizeActive_;
            resizeActive_ = false;
            if(changed) changed_.emit();
            return;
        }
        if(drag_) { update_drag(startX_ + dx, startY_ + dy); end_drag(); }
    });
    gesture->signal_cancel().connect([this](Gdk::EventSequence *) {
        if(resizePanel_) { resizePanel_->set_cursor("default"); resizePanel_ = nullptr; resizeActive_ = false; }
        else end_drag(true);
    });
    add_controller(gesture);
    auto motion = Gtk::EventControllerMotion::create();
    motion->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    motion->signal_motion().connect([this](double x, double y) {
        if(resizePanel_) return;
        if(auto panel = resize_panel_at(x, y))
            set_cursor(panel->side_ == DockSide::Bottom ? "ns-resize" : "ew-resize");
        else
            set_cursor("default");
    });
    motion->signal_leave().connect([this]() { if(!resizePanel_) set_cursor("default"); });
    add_controller(motion);
    auto keys = Gtk::EventControllerKey::create();
    keys->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    keys->signal_key_pressed().connect([this](guint key, guint, Gdk::ModifierType) {
        if(key != GDK_KEY_Escape || !drag_) return false;
        end_drag(true);
        return true;
    }, false);
    add_controller(keys);
    preview_.signal_resize().connect([this](int width, int height) {
        if(width == lastWidth_ && height == lastHeight_) return;
        lastWidth_ = width;
        lastHeight_ = height;
        if(!drag_) {
            for(auto &panel : panels_) if(!panel->pinned_) place_float(*panel);
        }
        if(editor_.get_position() <= 0) editor_.set_position(std::max(100, height - 110));
    });
}

DockWorkspace::~DockWorkspace() {
    // Unparent before deleting the C++ wrappers (including hidden panels).
    for(auto &panel : panels_) detach(*panel);
}

DockPanel &DockWorkspace::add_panel(const Glib::ustring &title, Gtk::Widget &content, DockSide side, int width,
                                    const Glib::ustring &icon) {
    auto panel = std::make_unique<DockPanel>(title, content, side, width, icon);
    auto ptr = panel.get();
    for(const auto &other : panels_)
        if(other->visible_ && other->pinned_ && other->side_ == side) {
            ptr->active_ = false;
            break;
        }
    ptr->close_.signal_clicked().connect([this, ptr]() { set_visible(*ptr, false); });
    ptr->pin_.signal_clicked().connect([this, ptr]() { set_pinned(*ptr, !ptr->pinned_); });
    ptr->collapsed_.signal_clicked().connect([this, ptr]() { activate_panel(*ptr); });
    panels_.push_back(std::move(panel));
    update_layout();
    return *ptr;
}

void DockWorkspace::detach(DockPanel &panel) {
    if(auto box = dynamic_cast<Gtk::Box *>(panel.get_parent())) box->remove(panel);
    else if(panel.get_parent() == this) remove_overlay(panel);
    if(auto box = dynamic_cast<Gtk::Box *>(panel.collapsed_.get_parent())) box->remove(panel.collapsed_);
}

void DockWorkspace::set_state(DockPanel &panel, bool visible, bool pinned, DockSide side) {
    const bool wasVisible = panel.visible_;
    panel.visible_ = visible;
    panel.pinned_ = pinned;
    panel.side_ = side;
    if(!visible) panel.active_ = false;
    else if(!wasVisible || !pinned) panel.active_ = true;
}

void DockWorkspace::set_visible(DockPanel &panel, bool visible) {
    if(drag_ == &panel) end_drag(true);
    panel.visible_ = visible;
    if(visible && panel.pinned_) {
        for(auto &other : panels_)
            if(other.get() != &panel && other->visible_ && other->pinned_ && other->side_ == panel.side_)
                other->active_ = false;
        panel.active_ = true;
    } else if(!visible) {
        panel.active_ = false;
    }
    update_layout();
    changed_.emit();
}

void DockWorkspace::activate_panel(DockPanel &panel) {
    for(auto &other : panels_)
        if(other->visible_ && other->pinned_ && other->side_ == panel.side_)
            other->active_ = other.get() == &panel;
    panel.visible_ = true;
    panel.active_ = true;
    update_layout();
    changed_.emit();
}

void DockWorkspace::set_pinned(DockPanel &panel, bool pinned) {
    if(drag_ == &panel) end_drag(true);
    if(panel.pinned_ && !pinned) {
        graphene_rect_t bounds{};
        if(gtk_widget_compute_bounds(GTK_WIDGET(panel.gobj()), GTK_WIDGET(gobj()), &bounds)) {
            panel.floatX_ = bounds.origin.x + 16;
            panel.floatY_ = bounds.origin.y + 16;
        }
    }
    panel.pinned_ = pinned;
    if(!pinned) panel.active_ = true;
    else if(panel.visible_) {
        for(auto &other : panels_)
            if(other.get() != &panel && other->visible_ && other->pinned_ && other->side_ == panel.side_)
                other->active_ = false;
        panel.active_ = true;
    }
    update_layout();
    changed_.emit();
}

void DockWorkspace::update_layout() {
    for(auto &panel : panels_) detach(*panel);
    if(leftTabs_.get_parent()) static_cast<Gtk::Box *>(leftTabs_.get_parent())->remove(leftTabs_);
    if(rightTabs_.get_parent()) static_cast<Gtk::Box *>(rightTabs_.get_parent())->remove(rightTabs_);
    if(bottomTabs_.get_parent()) static_cast<Gtk::Box *>(bottomTabs_.get_parent())->remove(bottomTabs_);
    while(auto child = leftTabs_.get_first_child()) leftTabs_.remove(*child);
    while(auto child = rightTabs_.get_first_child()) rightTabs_.remove(*child);
    while(auto child = bottomTabs_.get_first_child()) bottomTabs_.remove(*child);
    left_.set_orientation(Gtk::Orientation::HORIZONTAL);
    right_.set_orientation(Gtk::Orientation::HORIZONTAL);
    // The bottom dock is a vertical stack: the active panel occupies the
    // upper area and collapsed panel buttons form a horizontal strip below
    // it.  Keeping the tabs inside this stack prevents them from taking a
    // vertical column on the right side of the editor.
    bottom_.set_orientation(Gtk::Orientation::HORIZONTAL);
    leftTabs_.add_css_class("dock-tabs");
    rightTabs_.add_css_class("dock-tabs");
    bottomTabs_.add_css_class("dock-tabs");
    bottomTabs_.add_css_class("dock-tabs-bottom");
    bottomTabs_.set_orientation(Gtk::Orientation::HORIZONTAL);
    bool hasLeft = false, hasRight = false, hasBottom = false;
    for(auto &owned : panels_) {
        auto &panel = *owned;
        panel.refresh_header();
        panel.set_margin_start(0);
        panel.set_margin_top(0);
        panel.Gtk::Widget::set_visible(panel.visible_ && (!panel.pinned_ || panel.active_));
        if(!panel.visible_ || (panel.pinned_ && !panel.active_)) {
            if(panel.visible_ && panel.pinned_ && !panel.active_) {
                parkedPanels_.append(panel);
                panel.Gtk::Widget::set_visible(false);
            }
            if(panel.pinned_) {
                if(panel.side_ == DockSide::Left) { leftTabs_.append(panel.collapsed_); panel.collapsed_.add_css_class("dock-tab"); }
                else if(panel.side_ == DockSide::Right) { rightTabs_.append(panel.collapsed_); panel.collapsed_.add_css_class("dock-tab"); }
                else { bottomTabs_.append(panel.collapsed_); panel.collapsed_.add_css_class("dock-tab"); }
            }
            continue;
        }
        if(!panel.pinned_) {
            add_overlay(panel);
            set_measure_overlay(panel, false);
            place_float(panel);
            continue;
        }
        panel.set_halign(Gtk::Align::FILL);
        panel.set_valign(Gtk::Align::FILL);
        if(panel.side_ == DockSide::Bottom) {
            panel.set_hexpand(true);
            panel.set_vexpand(true);
            panel.set_size_request(160, panel.preferredHeight_);
            bottom_.append(panel);
            hasBottom = true;
        } else {
            panel.set_hexpand(false);
            panel.set_vexpand(true);
            panel.set_size_request(panel.preferredWidth_, 100);
            if(panel.side_ == DockSide::Left) { left_.append(panel); hasLeft = true; }
            else { right_.append(panel); hasRight = true; }
        }
    }
    if(leftTabs_.get_first_child()) {
        // Keep collapsed left-dock tabs on the outer edge, like HiAsm 4.
        if(leftTabs_.get_parent()) static_cast<Gtk::Box *>(leftTabs_.get_parent())->remove(leftTabs_);
        left_.prepend(leftTabs_);
    }
    if(rightTabs_.get_first_child()) right_.append(rightTabs_);
    if(bottomTabs_.get_first_child()) {
        bottomTabs_.set_hexpand(true);
        bottomTabs_.set_vexpand(false);
        bottom_.append(bottomTabs_);
    }
    left_.set_visible(hasLeft || leftTabs_.get_first_child());
    right_.set_visible(hasRight || rightTabs_.get_first_child());
    bottom_.set_hexpand(true);
    bottom_.set_vexpand(false);
    bottom_.set_visible(hasBottom);
    bottomArea_.set_hexpand(true);
    bottomArea_.set_vexpand(false);
    bottomArea_.set_visible(hasBottom || bottomTabs_.get_first_child());
    if(bottomTabs_.get_parent()) static_cast<Gtk::Box *>(bottomTabs_.get_parent())->remove(bottomTabs_);
    if(bottomTabs_.get_first_child()) {
        bottomTabs_.set_hexpand(true);
        bottomTabs_.set_vexpand(false);
        bottomArea_.append(bottomTabs_);
    }
    // Keep the non-interactive preview above floating panels.
    remove_overlay(preview_);
    add_overlay(preview_);
    set_measure_overlay(preview_, false);
    preview_.queue_draw();
}

void DockWorkspace::place_float(DockPanel &panel) {
    // START-aligned overlays use the child's natural size, not just its
    // size request. Include the content and CSS in the clamping calculation.
    panel.set_margin_start(0);
    panel.set_margin_top(0);
    panel.set_size_request(-1, -1);
    int minimum = 0, natural = 0;
    gtk_widget_measure(GTK_WIDGET(panel.gobj()), GTK_ORIENTATION_HORIZONTAL, -1,
        &minimum, &natural, nullptr, nullptr);
    const int width = std::min(std::max(panel.floatWidth_, natural), std::max(1, get_width()));
    gtk_widget_measure(GTK_WIDGET(panel.gobj()), GTK_ORIENTATION_VERTICAL, width,
        &minimum, &natural, nullptr, nullptr);
    const int height = std::min(std::max(panel.floatHeight_, natural), std::max(1, get_height()));
    panel.floatX_ = std::clamp(panel.floatX_, 0.0, double(std::max(0, get_width() - width)));
    panel.floatY_ = std::clamp(panel.floatY_, 0.0, double(std::max(0, get_height() - height)));
    panel.set_halign(Gtk::Align::START);
    panel.set_valign(Gtk::Align::START);
    panel.set_hexpand(false);
    panel.set_vexpand(false);
    panel.set_size_request(width, height);
    panel.set_margin_start(int(panel.floatX_));
    panel.set_margin_top(int(panel.floatY_));
}

DockPanel *DockWorkspace::header_at(double x, double y) {
    GtkWidget *picked = gtk_widget_pick(GTK_WIDGET(gobj()), x, y, GTK_PICK_DEFAULT);
    for(GtkWidget *p = picked; p && p != GTK_WIDGET(gobj()); p = gtk_widget_get_parent(p))
        if(GTK_IS_BUTTON(p)) return nullptr;
    for(auto &panel : panels_) {
        auto header = GTK_WIDGET(panel->header_.gobj());
        if(picked && panel->visible_ && (picked == header || gtk_widget_is_ancestor(picked, header)))
            return panel.get();
    }
    return nullptr;
}

DockPanel *DockWorkspace::resize_panel_at(double x, double y) {
    constexpr double edge = 7.0;
    for(auto &owned : panels_) {
        auto &panel = *owned;
        if(!panel.visible_ || !panel.pinned_ || !panel.active_) continue;
        graphene_rect_t bounds{};
        if(!gtk_widget_compute_bounds(GTK_WIDGET(panel.gobj()), GTK_WIDGET(gobj()), &bounds)) continue;
        const double right = bounds.origin.x + bounds.size.width;
        const double bottom = bounds.origin.y + bounds.size.height;
        if(panel.side_ == DockSide::Left && std::abs(x - right) <= edge && y >= bounds.origin.y && y <= bottom) return &panel;
        if(panel.side_ == DockSide::Right && std::abs(x - bounds.origin.x) <= edge && y >= bounds.origin.y && y <= bottom) return &panel;
        if(panel.side_ == DockSide::Bottom && std::abs(y - bounds.origin.y) <= edge && x >= bounds.origin.x && x <= right) return &panel;
    }
    return nullptr;
}

void DockWorkspace::begin_drag(DockPanel &panel, double x, double y) {
    if(drag_ || !panel.visible_) return;
    drag_ = &panel;
    dragActive_ = false;
    originalPinned_ = panel.pinned_;
    originalSide_ = panel.side_;
    originalX_ = panel.floatX_;
    originalY_ = panel.floatY_;
    startX_ = x; startY_ = y;
    graphene_rect_t bounds{};
    if(gtk_widget_compute_bounds(GTK_WIDGET(panel.gobj()), GTK_WIDGET(gobj()), &bounds)) {
        grabX_ = std::clamp(x - bounds.origin.x, 6.0, double(panel.floatWidth_ - 12));
        grabY_ = std::clamp(y - bounds.origin.y, 6.0, 22.0);
    } else { grabX_ = 16; grabY_ = 14; }
    previewSide_ = -1;
}

void DockWorkspace::update_drag(double x, double y) {
    if(!drag_) return;
    if(!dragActive_) {
        if(std::hypot(x - startX_, y - startY_) < 6) return;
        dragActive_ = true;
        drag_->pinned_ = false;
        update_layout();
        drag_->header_.set_cursor("grabbing");
    }
    drag_->floatX_ = x - grabX_;
    drag_->floatY_ = y - grabY_;
    place_float(*drag_);
    const double edge = 48, width = get_width(), height = get_height();
    previewSide_ = -1;
    if(x >= -edge && x <= width + edge && y >= -edge && y <= height + edge) {
        if(y >= height - edge) previewSide_ = int(DockSide::Bottom);
        else if(x <= edge) previewSide_ = int(DockSide::Left);
        else if(x >= width - edge) previewSide_ = int(DockSide::Right);
    }
    preview_.queue_draw();
}

void DockWorkspace::end_drag(bool cancel) {
    if(!drag_) return;
    auto panel = drag_;
    const bool moved = dragActive_;
    if(cancel) {
        panel->pinned_ = originalPinned_;
        panel->side_ = originalSide_;
        panel->floatX_ = originalX_;
        panel->floatY_ = originalY_;
    } else if(moved && previewSide_ >= 0) {
        panel->pinned_ = true;
        panel->side_ = DockSide(previewSide_);
    }
    panel->header_.set_cursor("grab");
    drag_ = nullptr;
    dragActive_ = false;
    previewSide_ = -1;
    if(moved) update_layout();
    preview_.queue_draw();
    if(moved && !cancel) changed_.emit();
}

void DockWorkspace::draw_preview(const Cairo::RefPtr<Cairo::Context> &cr, int width, int height) {
    if(!drag_ || previewSide_ < 0) return;
    double x = 1, y = 1, w = width - 2, h = height - 2;
    if(previewSide_ == int(DockSide::Bottom)) { h = std::min(drag_->preferredHeight_, height / 2); y = height - h - 1; }
    else {
        w = std::min(drag_->preferredWidth_, width / 2);
        if(previewSide_ == int(DockSide::Right)) x = width - w - 1;
    }
    cr->rectangle(x, y, w, h);
    cr->set_source_rgba(0.22, 0.50, 0.90, 0.18);
    cr->fill_preserve();
    cr->set_source_rgba(0.22, 0.50, 0.90, 0.95);
    cr->set_line_width(2);
    cr->stroke();
}
#endif
