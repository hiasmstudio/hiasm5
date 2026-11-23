#include "ExamplesCatalogPanel.h"
#ifdef HIASM_GTK4
#include "Localization.h"
#include <algorithm>

namespace {
struct Entry { std::string name, path; bool folder; };
}

ExamplesCatalogPanel::ExamplesCatalogPanel() : Gtk::Box(Gtk::Orientation::VERTICAL, 2) {
    add_css_class("examples-catalog-panel");
    store_ = Gtk::TreeStore::create(columns_);
    tree_.set_model(store_);
    tree_.set_headers_visible(false);
    tree_.append_column(ui_tr("Examples"), columns_.name);
    tree_.set_enable_search(true);
    tree_.signal_row_activated().connect([this](const Gtk::TreeModel::Path &path, Gtk::TreeViewColumn *) {
        auto iter = store_->get_iter(path);
        if(!iter) return;
        auto row = *iter;
        if(row[columns_.folder]) {
            if(tree_.row_expanded(path)) tree_.collapse_row(path); else tree_.expand_row(path, false);
            return;
        }
        const std::string file = row[columns_.path];
        if(on_open_file && g_file_test(file.c_str(), G_FILE_TEST_IS_REGULAR)) on_open_file(file);
    });
    scroll_.set_child(tree_);
    scroll_.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    // The tree takes the whole height the dock gives the panel; without expand flags the
    // scroll window keeps its natural height (about 80 px).
    set_hexpand(true);
    set_vexpand(true);
    scroll_.set_hexpand(true);
    scroll_.set_vexpand(true);
    append(scroll_);
}

void ExamplesCatalogPanel::add_source(const ExamplesCatalogSource &source) {
    for(const auto &item : sources_)
        if((!source.id.empty() && item.id == source.id) ||
           (source.id.empty() && item.directory == source.directory)) return;
    sources_.push_back(source);
}

void ExamplesCatalogPanel::set_source(const std::string &id, const std::string &title,
                                      const std::string &directory) {
    for(auto &item : sources_) {
        if(item.id != id) continue;
        item.title = title;
        item.directory = directory;
        refresh();
        return;
    }
    add_source({title, directory, id});
    refresh();
}

void ExamplesCatalogPanel::fill(const Gtk::TreeModel::Children &children, const std::string &directory, int depth) {
    if(depth > 8) return;
    GError *error = nullptr;
    GDir *dir = g_dir_open(directory.c_str(), 0, &error);
    if(!dir) { g_clear_error(&error); return; }
    std::vector<Entry> entries;
    while(const gchar *name = g_dir_read_name(dir)) {
        if(name[0] == '.') continue;
        const std::string path = directory + "/" + name;
        if(g_file_test(path.c_str(), G_FILE_TEST_IS_SYMLINK)) continue;
        const bool folder = g_file_test(path.c_str(), G_FILE_TEST_IS_DIR);
        gchar *lower = g_ascii_strdown(name, -1);
        // A catalog contains directories and scheme files only.  A directory
        // whose name happens to end in .sha is still a directory.
        const bool scheme = !folder && g_file_test(path.c_str(), G_FILE_TEST_IS_REGULAR) &&
                            g_str_has_suffix(lower, ".sha");
        g_free(lower);
        if(folder || scheme) entries.push_back({name, path, folder});
    }
    g_dir_close(dir);
    std::sort(entries.begin(), entries.end(), [](const Entry &a, const Entry &b) {
        return a.folder != b.folder ? a.folder > b.folder : g_utf8_collate(a.name.c_str(), b.name.c_str()) < 0;
    });
    for(const auto &entry : entries) {
        auto row = *store_->append(children);
        row[columns_.name] = Glib::filename_display_name(entry.name);
        row[columns_.path] = entry.path;
        row[columns_.folder] = entry.folder;
        // Populate the complete tree up front. The catalog is intentionally
        // a clean hierarchy of packages, folders and .sha files; placeholder
        // rows make directories look like files and are therefore avoided.
        row[columns_.loaded] = true;
        if(entry.folder) fill(row.children(), entry.path, depth + 1);
    }
}

void ExamplesCatalogPanel::refresh() {
    store_->clear();
    for(const auto &source : sources_) {
        if(!g_file_test(source.directory.c_str(), G_FILE_TEST_IS_DIR)) continue;
        auto rootIter = store_->append();
        auto root = *rootIter;
        root[columns_.name] = Glib::filename_display_name(source.title);
        root[columns_.path] = source.directory;
        root[columns_.folder] = true;
        root[columns_.loaded] = true;
        fill(root.children(), source.directory, 1);
        tree_.expand_row(store_->get_path(rootIter), false);
    }
}
#endif
