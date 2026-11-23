#ifndef HIASM_BUILD_SETTINGS_PANEL_H
#define HIASM_BUILD_SETTINGS_PANEL_H

#ifdef HIASM_GTK4
#include <gtkmm.h>
#include "Pack.h"
#include <functional>

class BuildSettingsPanel : public Gtk::Box {
public:
    BuildSettingsPanel();
    void set_pack(Pack *pack);
    std::function<void()> on_build;
private:
    void refresh();
    Gtk::Label packageLabel_;
    Gtk::Label compilerLabel_;
    Gtk::ComboBoxText compilerBox_;
    Gtk::Button buildButton_;
    Pack *pack_ = nullptr;
    bool updating_ = false;
};
#endif
#endif
