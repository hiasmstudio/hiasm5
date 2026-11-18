#include "RepositoryPanel.h"
#ifdef HIASM_GTK4
#include "LegacyPackCatalog.h"
#include "Localization.h"
#include "share.h"
#include <algorithm>

namespace {
std::string absolutePath(const std::string &path) {
    gchar *value = g_canonicalize_filename(path.c_str(), nullptr);
    std::string result(value); g_free(value); return result;
}
std::string parentPath(const std::string &path) {
    gchar *value = g_path_get_dirname(path.c_str());
    std::string result(value); g_free(value); return result;
}
}

RepositoryPanel::RepositoryPanel() : Gtk::Box(Gtk::Orientation::VERTICAL, 2) {
    add_css_class("repository-panel");
    // The source selector belongs to the examples catalog. Keep only the
    // protocol indicator in this panel so its top row remains compact.
    sourceRow_.append(protocolLabel_);
    append(sourceRow_);
    auto button = [this](Gtk::Button &widget, const char *icon, const char *tip) {
        widget.set_icon_name(icon); widget.set_tooltip_text(ui_tr(tip));
        widget.set_has_frame(false); widget.add_css_class("flat"); toolbar_.append(widget);
    };
    button(choose_, "folder-open-symbolic", "Choose project directory");
    button(refresh_, "view-refresh-symbolic", "Refresh repository");
    button(init_, "document-new-symbolic", "Create Git repository");
    button(clone_, "edit-copy-symbolic", "Clone Git repository");
    button(update_, "go-down-symbolic", "Update repository");
    button(log_, "document-open-recent-symbolic", "Repository history");
    button(diff_, "document-properties-symbolic", "Repository changes");
    button(status_, "dialog-information-symbolic", "Repository status");
    button(save_, "document-save-symbolic", "Save to repository");
    toolbar_.add_css_class("repository-toolbar"); append(toolbar_);
    pathLabel_.set_ellipsize(Pango::EllipsizeMode::MIDDLE);
    pathLabel_.set_xalign(0); append(pathLabel_);
    store_ = Gtk::TreeStore::create(columns_);
    tree_.set_model(store_); tree_.set_headers_visible(false);
    tree_.append_column(ui_tr("Projects"), columns_.name);
    tree_.signal_row_expanded().connect([this](const Gtk::TreeModel::iterator &iter, const Gtk::TreeModel::Path &) {
        auto row = *iter;
        if(!row[columns_.folder] || row[columns_.loaded]) return;
        row[columns_.loaded] = true;
        while(!row.children().empty()) store_->erase(row.children().begin());
        const std::string path = row[columns_.path]; fill(row.children(), path);
    });
    tree_.signal_row_activated().connect([this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *) {
        auto iter = store_->get_iter(path); if(!iter) return;
        auto row = *iter;
        if(row[columns_.folder]) {
            if(tree_.row_expanded(path)) tree_.collapse_row(path); else tree_.expand_row(path, false);
        } else {
            const std::string file = row[columns_.path];
            if(on_open_file && g_file_test(file.c_str(), G_FILE_TEST_IS_REGULAR)) on_open_file(file);
        }
    });
    filesScroll_.set_child(tree_); filesScroll_.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    output_.set_editable(false); output_.set_monospace(true); output_.set_wrap_mode(Gtk::WrapMode::WORD_CHAR);
    outputScroll_.set_child(output_); outputScroll_.set_min_content_height(60);
    split_.set_start_child(filesScroll_); split_.set_end_child(outputScroll_);
    split_.set_resize_end_child(false); split_.set_position(300); split_.set_vexpand(true);
    append(split_); stateLabel_.set_wrap(true); stateLabel_.set_xalign(0); append(stateLabel_);
    choose_.signal_clicked().connect(sigc::mem_fun(*this, &RepositoryPanel::choose_directory));
    refresh_.signal_clicked().connect(sigc::mem_fun(*this, &RepositoryPanel::refresh));
    init_.signal_clicked().connect([this]() { perform(Action::Init); });
    clone_.signal_clicked().connect(sigc::mem_fun(*this, &RepositoryPanel::clone_dialog));
    update_.signal_clicked().connect([this]() { perform(Action::Update); });
    log_.signal_clicked().connect([this]() { perform(Action::Log); });
    diff_.signal_clicked().connect([this]() { perform(Action::Diff); });
    status_.signal_clicked().connect([this]() { perform(Action::Status); });
    save_.signal_clicked().connect(sigc::mem_fun(*this, &RepositoryPanel::save_dialog));
    refresh();
}
RepositoryPanel::~RepositoryPanel() {
    chooserResponse_.disconnect(); if(chooser_) chooser_->hide();
}
void RepositoryPanel::add_source(const RepositorySource &source) {
    for(const auto &item : sources_) if(item.id == source.id) return;
    sources_.push_back(source);
}
bool RepositoryPanel::select_source(const std::string &id) {
    if(busy()) return false;
    if(id != "project" && id != "folder") {
        bool found = false;
        for(const auto &source : sources_) if(source.id == id) { found = true; break; }
        if(!found) return false;
    }
    sourceId_ = id;
    activate_source();
    return true;
}
void RepositoryPanel::set_project(const std::string &file, bool legacy) {
    if(project_ == file && projectLegacy_ == legacy) return;
    project_ = file; projectLegacy_ = legacy;
    if(sourceId_ == "project" && !busy()) activate_source();
}
void RepositoryPanel::set_directory(const std::string &directory) {
    if(busy() || !g_file_test(directory.c_str(), G_FILE_TEST_IS_DIR)) return;
    customDirectory_ = absolutePath(directory);
    if(sourceId_ == "folder") activate_source(); else { sourceId_ = "folder"; activate_source(); }
}
bool RepositoryPanel::legacy_path(const std::string &path) const {
    if(path.empty()) return false;
    std::vector<std::string> diagnostics;
    for(const auto &location : LegacyPackCatalog::discover(LegacyPackCatalog::configurationPath(dataDir.raw()), diagnostics))
        if(LegacyPackCatalog::containsPath(location.installation, path)) return true;
    return false;
}
void RepositoryPanel::activate_source() {
    if(busy()) return;
    const auto &id = sourceId_;
    legacy_ = false; directory_.clear();
    if(id == "project" && !project_.empty()) {
        legacy_ = projectLegacy_ || legacy_path(project_);
        directory_ = legacy_ ? std::string() : GitClient::workingCopyRoot(project_);
        if(directory_.empty()) directory_ = g_file_test(project_.c_str(), G_FILE_TEST_IS_DIR) ? project_ : parentPath(project_);
    } else if(id == "folder") directory_ = customDirectory_;
    else for(const auto &source : sources_) if(source.id == id) { directory_ = source.directory; legacy_ = source.legacy; break; }
    if(!directory_.empty()) directory_ = absolutePath(directory_);
    legacy_ = legacy_ || legacy_path(directory_);
    protocol_ = legacy_ ? Protocol::Svn : Protocol::Git;
    refresh();
    if(on_directory_changed) on_directory_changed(directory_, legacy_);
}
void RepositoryPanel::fill(const Gtk::TreeModel::Children &children, const std::string &directory) {
    GError *error = nullptr; GDir *dir = g_dir_open(directory.c_str(), 0, &error);
    if(!dir) { append_output(error ? error->message : ui_tr("Cannot read repository directory."), true); g_clear_error(&error); return; }
    struct Entry { std::string name, path; bool folder; };
    std::vector<Entry> entries;
    while(const gchar *name = g_dir_read_name(dir)) {
        if(name[0] == '.') continue;
        const std::string path = directory + "/" + name;
        // Do not follow directory symlinks/junctions outside the selected catalog.
        if(g_file_test(path.c_str(), G_FILE_TEST_IS_SYMLINK)) continue;
        const bool folder = g_file_test(path.c_str(), G_FILE_TEST_IS_DIR);
        gchar *lower = g_ascii_strdown(name, -1);
        const bool scheme = g_str_has_suffix(lower, ".sha"); g_free(lower);
        if(folder || scheme) entries.push_back({name, path, folder});
        if(entries.size() >= 2000) { append_output(ui_tr("Repository listing limited to 2000 entries.")); break; }
    }
    g_dir_close(dir);
    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
        return a.folder != b.folder ? a.folder > b.folder : g_utf8_collate(a.name.c_str(), b.name.c_str()) < 0;
    });
    for(const auto &entry : entries) {
        auto row = *store_->append(children);
        row[columns_.name] = Glib::filename_display_name(entry.name);
        row[columns_.path] = entry.path; row[columns_.folder] = entry.folder; row[columns_.loaded] = !entry.folder;
        if(entry.folder) (*store_->append(row.children()))[columns_.name] = "…";
    }
}
void RepositoryPanel::refresh() {
    if(busy()) return;
    store_->clear();
    if(g_file_test(directory_.c_str(), G_FILE_TEST_IS_DIR)) fill(store_->children(), directory_);
    pathLabel_.set_text(directory_.empty() ? ui_tr("No project selected") : directory_);
    pathLabel_.set_tooltip_text(directory_); update_controls();
}
void RepositoryPanel::update_controls() {
    const bool running = busy();
    const bool exists = !directory_.empty() && g_file_test(directory_.c_str(), G_FILE_TEST_IS_DIR);
    const bool svn = protocol_ == Protocol::Svn;
    const bool available = svn ? SvnClient::available() : GitClient::available();
    const bool working = exists && (svn ? SvnClient::isWorkingCopy(directory_) : GitClient::isWorkingCopy(directory_));
    choose_.set_sensitive(!running); tree_.set_sensitive(!running);
    refresh_.set_sensitive(!running); init_.set_visible(!svn); clone_.set_visible(!svn); status_.set_visible(!svn);
    init_.set_sensitive(!running && exists && available && !working);
    clone_.set_sensitive(!running && exists && available && !working);
    for(auto button : {&update_, &log_, &diff_, &status_}) button->set_sensitive(!running && working && available);
    save_.set_sensitive(!running && exists && !legacy_ && canSave_ && bool(on_save_copy));
    protocolLabel_.set_text(svn ? "SVN" : "Git");
    stateLabel_.set_text(running ? ui_tr("Repository operation in progress…") : !exists ? ui_tr("Choose a project or repository folder.") :
        !available ? ui_tr(svn ? "SVN client not found. Install svn or set HIASM_SVN." : "Git client not found. Install Git or set HIASM_GIT.") :
        working ? ui_tr(svn ? "HiAsm 4 catalog (SVN). Open schemes by double-clicking." : "Git repository. Open schemes by double-clicking.") :
        ui_tr(svn ? "HiAsm 4 catalog: no SVN working copy found. Files can be browsed read-only." : "Local folder. Create or clone a Git repository to enable version control."));
}
void RepositoryPanel::append_output(const std::string &text, bool error) {
    auto buffer = output_.get_buffer();
    if(buffer->get_char_count() > 100000) buffer->set_text("");
    if(buffer->get_char_count()) buffer->insert(buffer->end(), "\n");
    buffer->insert(buffer->end(), (error ? "! " : "") + text);
}
void RepositoryPanel::completed(bool ok, const std::string &text, const std::string &operationDirectory) {
    append_output(operationDirectory + "\n" + (text.empty() ? ui_tr(ok ? "Operation completed." : "Repository operation failed.") : text), !ok);
    if(ok) append_output(ui_tr("Reload open schemes and restart HiAsm after updating package definitions."));
    activate_source();
}
bool RepositoryPanel::perform(Action action) {
    if(busy() || directory_.empty()) return false;
    const auto directory = directory_;
    auto completion = [this, directory](bool ok, const std::string &text) { completed(ok, text, directory); };
    bool started = false;
    if(protocol_ == Protocol::Svn) {
        if(action == Action::Init || action == Action::Status) return false;
        started = svn_.start(action == Action::Update ? SvnClient::Update : action == Action::Log ? SvnClient::Log : SvnClient::Diff, directory, completion);
    } else {
        const auto command = action == Action::Update ? GitClient::Pull : action == Action::Log ? GitClient::Log :
            action == Action::Diff ? GitClient::Diff : action == Action::Init ? GitClient::Init : GitClient::Status;
        started = git_.start(command, directory, completion);
    }
    if(!started) append_output(ui_tr("Cannot start repository operation. Check the client and working copy."), true);
    update_controls(); return started;
}
bool RepositoryPanel::clone_repository(const std::string &url) {
    if(busy() || protocol_ != Protocol::Git || directory_.empty()) return false;
    const auto directory = directory_;
    const bool started = git_.clone(url, directory, [this, directory](bool ok, const std::string &text) { completed(ok, text, directory); });
    if(!started) append_output(ui_tr("Choose an empty destination folder before cloning."), true);
    update_controls(); return started;
}
std::string RepositoryPanel::selected_directory() const {
    auto selection = const_cast<Gtk::TreeView &>(tree_).get_selection();
    if(auto iter = selection->get_selected()) {
        const std::string path = (*iter)[columns_.path];
        return (*iter)[columns_.folder] ? path : parentPath(path);
    }
    return directory_;
}
void RepositoryPanel::set_can_save(bool value) { canSave_ = value; update_controls(); }
bool RepositoryPanel::save_copy(const std::string &filename) {
    if(busy() || legacy_ || !canSave_ || !on_save_copy || !LegacyPackCatalog::containsPath(directory_, filename) || legacy_path(filename)) return false;
    if(filename.size() < 4 || g_ascii_strcasecmp(filename.c_str() + filename.size() - 4, ".sha")) {
        append_output(ui_tr("Use a .sha filename for the repository copy."), true); return false;
    }
    if(g_file_test(filename.c_str(), G_FILE_TEST_EXISTS)) {
        append_output(ui_tr("A file with this name already exists. Choose another name."), true); return false;
    }
    std::string error;
    if(!on_save_copy(filename, error)) { append_output(error, true); return false; }
    refresh(); append_output(ui_tr("Scheme copied to repository.") + std::string("\n") + filename); return true;
}
void RepositoryPanel::choose_directory() {
    if(chooser_) chooser_->hide();
    chooserResponse_.disconnect();
    chooser_ = Gtk::FileChooserNative::create(ui_tr("Choose project directory"), Gtk::FileChooser::Action::SELECT_FOLDER, ui_tr("Open"), ui_tr("Cancel"));
    if(auto window = dynamic_cast<Gtk::Window *>(get_root())) chooser_->set_transient_for(*window);
    if(!directory_.empty()) chooser_->set_current_folder(Gio::File::create_for_path(directory_));
    chooserResponse_ = chooser_->signal_response().connect([this](int response) {
        if(response == int(Gtk::ResponseType::ACCEPT)) if(auto file = chooser_->get_file()) set_directory(file->get_path());
        chooser_->hide();
    });
    chooser_->show();
}
void RepositoryPanel::clone_dialog() {
    Gtk::Dialog dialog(ui_tr("Clone Git repository"));
    if(auto window = dynamic_cast<Gtk::Window *>(get_root())) dialog.set_transient_for(*window);
    dialog.set_modal(true);
    Gtk::Label label(ui_tr("Repository URL or local path")); Gtk::Entry url; url.set_hexpand(true);
    dialog.get_content_area()->append(label); dialog.get_content_area()->append(url);
    dialog.add_button(ui_tr("Cancel"), int(Gtk::ResponseType::CANCEL));
    dialog.add_button(ui_tr("Clone"), int(Gtk::ResponseType::OK));
    if(dialog_run(dialog) == int(Gtk::ResponseType::OK) && !url.get_text().empty()) clone_repository(url.get_text().raw());
}
void RepositoryPanel::save_dialog() {
    if(legacy_ || !canSave_) return;
    if(chooser_) chooser_->hide();
    chooserResponse_.disconnect();
    chooser_ = Gtk::FileChooserNative::create(ui_tr("Save to repository"), Gtk::FileChooser::Action::SAVE, ui_tr("Save"), ui_tr("Cancel"));
    if(auto window = dynamic_cast<Gtk::Window *>(get_root())) chooser_->set_transient_for(*window);
    chooser_->set_current_folder(Gio::File::create_for_path(selected_directory()));
    chooser_->set_current_name("Project.sha");
    auto filter = Gtk::FileFilter::create(); filter->set_name("HiAsm (*.sha)"); filter->add_pattern("*.sha"); chooser_->add_filter(filter);
    chooserResponse_ = chooser_->signal_response().connect([this](int response) {
        if(response == int(Gtk::ResponseType::ACCEPT)) if(auto file = chooser_->get_file()) save_copy(file->get_path());
        chooser_->hide();
    });
    chooser_->show();
}
#endif

