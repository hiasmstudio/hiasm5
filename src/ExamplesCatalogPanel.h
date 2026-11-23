#ifndef HIASM_EXAMPLES_CATALOG_PANEL_H
#define HIASM_EXAMPLES_CATALOG_PANEL_H
#ifdef HIASM_GTK4
#include <gtkmm.h>
#include <functional>
#include <string>
#include <vector>

struct ExamplesCatalogSource {
    std::string title;
    std::string directory;
    std::string id;
};

// A read-only, HiAsm 4 style tree of package examples and tutorials.
class ExamplesCatalogPanel : public Gtk::Box {
public:
    ExamplesCatalogPanel();
    void add_source(const ExamplesCatalogSource &source);
    void set_source(const std::string &id, const std::string &title, const std::string &directory);
    void refresh();
    std::function<void(const std::string &)> on_open_file;
    Gtk::TreeView &tree() { return tree_; }
private:
    struct Columns : Gtk::TreeModelColumnRecord {
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<std::string> path;
        Gtk::TreeModelColumn<bool> folder, loaded;
        Columns() { add(name); add(path); add(folder); add(loaded); }
    } columns_;
    void fill(const Gtk::TreeModel::Children &children, const std::string &directory, int depth);
    std::vector<ExamplesCatalogSource> sources_;
    Gtk::ScrolledWindow scroll_;
    Gtk::TreeView tree_;
    Glib::RefPtr<Gtk::TreeStore> store_;
};
#endif
#endif
