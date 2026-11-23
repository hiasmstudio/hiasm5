#ifndef HIASM_REPOSITORY_PANEL_H
#define HIASM_REPOSITORY_PANEL_H
#ifdef HIASM_GTK4
#include <gtkmm.h>
#include "GitClient.h"
#include "SvnClient.h"
#include <functional>
#include <string>
#include <vector>

struct RepositorySource {
    std::string id, title, directory;
    bool legacy = false;
};

// The project repository dock contains the current project's files and
// version-control actions. Catalog sources are displayed by ExamplesCatalogPanel.
class RepositoryPanel : public Gtk::Box {
public:
    enum class Protocol { Git, Svn };
    enum class Action { Status, Update, Log, Diff, Init };
    RepositoryPanel();
    ~RepositoryPanel() override;
    void add_source(const RepositorySource &source);
    bool select_source(const std::string &id);
    void set_project(const std::string &file, bool legacy = false);
    void set_directory(const std::string &directory);
    void refresh();
    bool perform(Action action);
    bool clone_repository(const std::string &url);
    bool busy() const { return svn_.busy() || git_.busy(); }
    Protocol protocol() const { return protocol_; }
    const std::string &directory() const { return directory_; }
    std::string output() const { return output_.get_buffer()->get_text(); }
    Gtk::TreeView &tree() { return tree_; }
    std::function<void(const std::string &)> on_open_file;
    std::function<void(const std::string &, bool)> on_directory_changed;
    // A copy of the current in-memory scheme; never changes the editor filename.
    std::function<bool(const std::string &, std::string &)> on_save_copy;
    void set_can_save(bool value);
    bool save_copy(const std::string &filename);
    void save_to_repository() { save_dialog(); }
private:
    struct Columns : Gtk::TreeModelColumnRecord {
        Gtk::TreeModelColumn<Glib::ustring> name;
        Gtk::TreeModelColumn<std::string> path;
        Gtk::TreeModelColumn<bool> folder, loaded;
        Columns() { add(name); add(path); add(folder); add(loaded); }
    } columns_;
    void activate_source();
    void fill(const Gtk::TreeModel::Children &children, const std::string &directory);
    void update_controls();
    void append_output(const std::string &text, bool error = false);
    void completed(bool ok, const std::string &text, const std::string &operationDirectory);
    bool legacy_path(const std::string &path) const;
    std::string selected_directory() const;
    void choose_directory();
    void clone_dialog();
    void save_dialog();
    Protocol protocol_ = Protocol::Git;
    bool legacy_ = false, canSave_ = false;
    std::string directory_, project_, customDirectory_;
    bool projectLegacy_ = false;
    std::vector<RepositorySource> sources_;
    // Source selection is kept as an internal compatibility API. The visible
    // source selector was moved to ExamplesCatalogPanel.
    std::string sourceId_ = "project";
    Gtk::Box sourceRow_{Gtk::Orientation::HORIZONTAL, 2};
    Gtk::Label protocolLabel_;
    Gtk::Box toolbar_{Gtk::Orientation::HORIZONTAL, 1};
    Gtk::Button choose_, refresh_, init_, clone_, update_, log_, diff_, status_, save_;
    Gtk::Label pathLabel_, stateLabel_;
    Gtk::Paned split_{Gtk::Orientation::VERTICAL};
    Gtk::ScrolledWindow filesScroll_, outputScroll_;
    Gtk::TreeView tree_;
    Glib::RefPtr<Gtk::TreeStore> store_;
    Gtk::TextView output_;
    Glib::RefPtr<Gtk::FileChooserNative> chooser_;
    sigc::connection chooserResponse_;
    // Destroy clients first so outstanding callbacks cannot touch deleted widgets.
    SvnClient svn_;
    GitClient git_;
};
#endif
#endif

