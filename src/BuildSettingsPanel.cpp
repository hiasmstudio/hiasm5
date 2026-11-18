#include "BuildSettingsPanel.h"
#ifdef HIASM_GTK4
#include "Localization.h"

BuildSettingsPanel::BuildSettingsPanel() : Gtk::Box(Gtk::Orientation::VERTICAL, 6) {
    add_css_class("build-settings-panel");
    set_margin_start(8); set_margin_end(8); set_margin_top(8); set_margin_bottom(8);
    packageLabel_.set_xalign(0); packageLabel_.set_wrap(true);
    compilerLabel_.set_text(ui_tr("Compiler")); compilerLabel_.set_xalign(0);
    compilerBox_.set_hexpand(true);
    buildButton_.set_label(ui_tr("Build"));
    buildButton_.set_icon_name("system-run-symbolic");
    buildButton_.set_tooltip_text(ui_tr("Build current scheme"));
    buildButton_.set_halign(Gtk::Align::START);
    append(packageLabel_); append(compilerLabel_); append(compilerBox_); append(buildButton_);
    compilerBox_.signal_changed().connect([this]() {
        if(updating_ || !pack_) return;
        const auto id = compilerBox_.get_active_id();
        if(!id.empty()) pack_->setActiveCompiler(id);
        refresh();
    });
    buildButton_.signal_clicked().connect([this]() { if(on_build) on_build(); });
    refresh();
}

void BuildSettingsPanel::set_pack(Pack *pack) { pack_ = pack; refresh(); }

void BuildSettingsPanel::refresh() {
    updating_ = true;
    compilerBox_.remove_all();
    if(!pack_) {
        packageLabel_.set_text(ui_tr("No package selected"));
        compilerLabel_.set_sensitive(false); compilerBox_.set_sensitive(false); buildButton_.set_sensitive(false);
        updating_ = false;
        return;
    }
    packageLabel_.set_text(Glib::ustring::compose("%1: %2", ui_tr("Package"), pack_->title));
    compilerLabel_.set_sensitive(true); compilerBox_.set_sensitive(true);
    const Compiler *active = pack_->getActiveCompiler();
    std::string activeId;
    for(const auto &item : pack_->compilers) {
        if(!item.compiler) continue;
        const std::string id = item.compiler->name.raw();
        compilerBox_.append(id, item.compiler->name);
        if(item.compiler == active) activeId = id;
    }
    if(activeId.empty() && !pack_->compilers.empty() && pack_->compilers.front().compiler)
        activeId = pack_->compilers.front().compiler->name.raw();
    if(!activeId.empty()) compilerBox_.set_active_id(activeId);
    buildButton_.set_sensitive(!activeId.empty());
    updating_ = false;
}
#endif
