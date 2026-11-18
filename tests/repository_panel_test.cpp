#include "RepositoryPanel.h"
#include "LegacyPackCatalog.h"
#include "share.h"
#include <glib/gstdio.h>
#include <iostream>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)
static bool waitDone(RepositoryPanel &panel) {
    const auto end = g_get_monotonic_time() + 15000000;
    while(panel.busy() && g_get_monotonic_time() < end) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(1000); }
    return !panel.busy();
}
static bool samePath(const std::string &a, const std::string &b) {
    auto first = g_file_new_for_path(a.c_str()); auto second = g_file_new_for_path(b.c_str());
    bool equal = g_file_equal(first, second); g_object_unref(first); g_object_unref(second); return equal;
}
int main(int argc, char **argv) {
    if(argc != 2) return 2;
    auto app = Gtk::Application::create("com.hiasm.repository-test", Gio::Application::Flags::NON_UNIQUE); app->register_application();
    gchar *tmp = g_dir_make_tmp("hiasm-repository-XXXXXX", nullptr); CHECK(tmp);
    const std::string root(tmp); g_free(tmp); dataDir = root + "/";
    const std::string native = root + u8"/native ' 世界", legacy = root + "/legacy", example = legacy + "/Elements/delphi/Example";
    for(const auto &path : {native + "/nested", example, legacy + "/Elements/delphi/conf", root + "/packs/_hiasm4"}) CHECK(g_mkdir_with_parents(path.c_str(), 0700) == 0);
    CHECK(g_file_set_contents((legacy + "/Elements/delphi/Elements.db").c_str(), "", 0, nullptr));
    CHECK(g_file_set_contents((root + "/packs/_hiasm4/paths.ini").c_str(), legacy.c_str(), -1, nullptr));
    g_unsetenv("HIASM4_ROOTS");
    CHECK(g_file_set_contents((native + "/first.sha").c_str(), "Make(cpp)\n", -1, nullptr));
    CHECK(g_file_set_contents((native + "/nested/second.sha").c_str(), "Make(cpp)\n", -1, nullptr));
    CHECK(g_file_set_contents((native + "/not-a-scheme.txt").c_str(), "hidden", -1, nullptr));
    CHECK(g_file_set_contents((example + "/old.sha").c_str(), "Make(delphi)\n", -1, nullptr));
    RepositoryPanel panel;
    panel.add_source({"native", "Native", native, false}); panel.add_source({"legacy", "HiAsm 4", example, true});
    CHECK(panel.select_source("native") && panel.protocol() == RepositoryPanel::Protocol::Git);
    const auto model = panel.tree().get_model(); CHECK(model->children().size() == 2);
    std::string opened;
    panel.on_open_file = [&](const std::string &file) { opened = file; };
    panel.tree().row_activated(Gtk::TreeModel::Path("1"), *panel.tree().get_column(0)); CHECK(samePath(opened, native + "/first.sha"));
    panel.tree().expand_row(Gtk::TreeModel::Path("0"), false); CHECK(model->children().begin()->children().size() == 1);
    panel.tree().row_activated(Gtk::TreeModel::Path("0:0"), *panel.tree().get_column(0)); CHECK(samePath(opened, native + "/nested/second.sha"));
    CHECK(panel.select_source("legacy") && panel.protocol() == RepositoryPanel::Protocol::Svn);
    CHECK(!panel.perform(RepositoryPanel::Action::Init)); CHECK(!panel.clone_repository(native));
    bool saved = false;
    panel.on_save_copy = [&](const std::string &file, std::string &) { saved = true; return g_file_set_contents(file.c_str(), "copy", -1, nullptr); };
    panel.set_can_save(true); CHECK(!panel.save_copy(example + "/forbidden.sha") && !saved);
    panel.set_directory(example); CHECK(panel.protocol() == RepositoryPanel::Protocol::Svn); // Config-based protocol outside a pack tab.
    panel.set_directory(native); CHECK(panel.protocol() == RepositoryPanel::Protocol::Git);
    CHECK(!panel.save_copy(native + "/../escaped.sha") && !saved);
    CHECK(panel.save_copy(native + "/copied.sha") && saved);
    if(GitClient::available()) {
        CHECK(panel.perform(RepositoryPanel::Action::Init)); CHECK(!panel.perform(RepositoryPanel::Action::Init)); CHECK(waitDone(panel));
        CHECK(GitClient::isWorkingCopy(native));
        CHECK(panel.perform(RepositoryPanel::Action::Status)); CHECK(waitDone(panel)); CHECK(panel.output().find("first.sha") != std::string::npos);
    }
    panel.set_project(example + "/old.sha", true); CHECK(panel.select_source("project") && panel.protocol() == RepositoryPanel::Protocol::Svn);
    panel.set_project(native + "/first.sha"); CHECK(panel.protocol() == RepositoryPanel::Protocol::Git);
    // A similarly named neighboring directory is not a HiAsm 4 installation.
    CHECK(g_mkdir_with_parents((legacy + "-other").c_str(), 0700) == 0); panel.set_directory(legacy + "-other");
    CHECK(panel.protocol() == RepositoryPanel::Protocol::Git);
    std::cout << "OK Repository: catalog tree, open, save-copy isolation, native Git / configured HiAsm4 SVN, async actions" << std::endl;
    return 0;
}

