#include "main_gui.h"
#include "ExamplesCatalogPanel.h"
#include <glib/gstdio.h>
#include <iostream>
#include <vector>

#define CHECK(expr) do { if(!(expr)) { std::cerr << __LINE__ << ": " << #expr << std::endl; return 1; } } while(0)

static void settle() {
    auto context = Glib::MainContext::get_default();
    for(int i = 0; i < 50; ++i) {
        while(context->pending()) context->iteration(false);
        g_usleep(3000);
    }
}

template<typename T> static void collect(Gtk::Widget &widget, std::vector<T *> &found) {
    if(auto item = dynamic_cast<T *>(&widget)) found.push_back(item);
    for(auto child = widget.get_first_child(); child; child = child->get_next_sibling())
        collect(*child, found);
}

int main(int argc, char **argv) {
    if(argc != 2) return 2;
    // Exercise real menu callbacks without changing the user's database/tabs.
    gchar *temporary = g_dir_make_tmp("hiasm-main-docks-XXXXXX", nullptr);
    CHECK(temporary);
    const std::string profile(temporary);
    g_free(temporary);
    g_setenv("HOME", profile.c_str(), TRUE);
    g_setenv("USERPROFILE", profile.c_str(), TRUE);
    g_setenv("HIASM_DATA_DIR", argv[1], TRUE);
    g_setenv("GTK_CSD", "0", TRUE);
    CHECK(std::string(g_get_home_dir()) == profile);
    auto app = Gtk::Application::create("com.hiasm.main-docks-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
    {
        MainGUI window;
        app->add_window(window);
        window.present();
        settle();
        std::vector<DockWorkspace *> workspaces;
        std::vector<DockPanel *> panels;
        collect(window, workspaces);
        collect(window, panels);
        // A hidden panel has no parent (dock_workspace_test checks this), so the widget tree holds only
        // the six panels that are visible or collapsed; Errors and Terminal are hidden by default and
        // join the tree when the View commands show them for the first time.
        if(!(workspaces.size() == 1 && panels.size() == 6)) { std::cerr << "workspaces=" << workspaces.size() << " panels=" << panels.size() << std::endl; return 1; }
        auto findPanel = [&](const std::string &title) -> DockPanel * {
            std::vector<DockPanel *> found;
            collect(window, found);
            for(auto *panel : found) if(panel->title() == title) return panel;
            return nullptr;
        };
        auto &workspace = *workspaces[0];
        DockPanel *left = nullptr, *right = nullptr, *debug = nullptr, *repository = nullptr, *examples = nullptr;
        for(auto *panel : panels) {
            if(panel->title() == ui_tr("Elements")) left = panel;
            else if(panel->title() == ui_tr("Element property")) right = panel;
            else if(panel->title() == ui_tr("Debug")) debug = panel;
            else if(panel->title() == ui_tr("Project catalog")) examples = panel;
            else if(panel->title() == ui_tr("Project repository")) repository = panel;
        }
        CHECK(left && right && debug && repository && examples);
        CHECK(!findPanel(ui_tr("Errors")) && !findPanel(ui_tr("Terminal")));
        CHECK(!cmdMan->findByName(CMD_PN_ERRORS)->checked && !cmdMan->findByName(CMD_PN_TERMINAL)->checked);
        std::vector<RepositoryPanel *> repositories;
        collect(window, repositories);
        CHECK(repositories.size() == 1);
        auto oneWindow = [&]() {
            size_t visible = 0;
            for(auto top : Gtk::Window::list_toplevels()) if(top->get_visible()) ++visible;
            for(auto panel : panels) {
                if(!panel->is_panel_visible()) continue;
                if(gtk_widget_get_root(GTK_WIDGET(panel->gobj())) != GTK_ROOT(window.gobj())) return false;
                if(gtk_widget_get_native(GTK_WIDGET(panel->gobj())) != GTK_NATIVE(window.gobj())) return false;
            }
            return visible == 1;
        };
        CHECK(oneWindow() && workspace.get_height() > 300);
        CHECK(left->is_pinned() && left->side() == DockSide::Left);
        CHECK(right->is_pinned() && right->side() == DockSide::Right);
        CHECK(debug->is_pinned() && debug->side() == DockSide::Bottom && debug->is_panel_visible());
        CHECK(examples->is_pinned() && examples->side() == DockSide::Right && examples->is_panel_visible());
        // Only one pinned panel per side is open: the project catalog starts as a tab on the right.
        // Opening it gives the whole dock to the tree; the tree used to keep its natural height (about 80 px).
        CHECK(!examples->is_active());
        g_signal_emit_by_name(examples->collapsed_button().gobj(), "clicked"); settle();
        CHECK(examples->is_active() && examples->get_mapped() && !right->is_active());
        std::vector<ExamplesCatalogPanel *> catalogs;
        collect(*examples, catalogs);
        for(int i = 0; i < 50 && catalogs.size() == 1 && catalogs[0]->get_allocated_height() < 200; ++i) settle();
        CHECK(catalogs.size() == 1 && catalogs[0]->get_mapped() && catalogs[0]->get_allocated_height() > 200);
        CHECK(left->header().get_width() > left->header().get_height());
        CHECK(right->header().get_width() > right->header().get_height());
        CHECK(debug->header().get_width() < debug->header().get_height());
        for(auto command : {CMD_ELEMENTS, CMD_PROPERTY, CMD_PN_REP, CMD_PN_DEBUG, CMD_PN_ERRORS, CMD_PN_TERMINAL, CMD_EXAMPLES, CMD_PANELS, CMD_QPAN_SHOWALL, CMD_QPAN_HIDEALL})
            CHECK(cmdMan->isEnabled(command));
        CHECK(cmdMan->findByName(CMD_PN_ERRORS)->info == ui_tr("Errors"));
        CHECK(cmdMan->findByName(CMD_PN_TERMINAL)->info == ui_tr("Terminal"));
        cmdMan->command(CMD_PN_ERRORS); settle();
        DockPanel *errors = findPanel(ui_tr("Errors"));
        CHECK(errors && errors->is_panel_visible() && errors->is_active());
        panels.push_back(errors);	// from now on oneWindow() checks it as well
        CHECK(oneWindow() && cmdMan->findByName(CMD_PN_ERRORS)->checked);
        cmdMan->command(CMD_PN_ERRORS); settle();
        CHECK(!errors->is_panel_visible() && errors->get_parent() == nullptr && !cmdMan->findByName(CMD_PN_ERRORS)->checked);
        cmdMan->command(CMD_PN_TERMINAL); settle();
        DockPanel *terminal = findPanel(ui_tr("Terminal"));
        CHECK(terminal && terminal->is_panel_visible() && terminal->is_active());
        panels.push_back(terminal);
        CHECK(oneWindow() && cmdMan->findByName(CMD_PN_TERMINAL)->checked);
        std::vector<TerminalPanel *> terminals;
        collect(window, terminals);
        CHECK(terminals.size() == 1);
        terminals[0]->run_command("echo hiasm-terminal-test");
        for(int i = 0; i < 100; ++i) {
            settle();
            const std::string output = terminals[0]->output_view().get_buffer()->get_text().raw();
            if(output.find("hiasm-terminal-test") != std::string::npos) break;
        }
        CHECK(terminals[0]->output_view().get_buffer()->get_text().raw().find("hiasm-terminal-test") != std::string::npos);
        // A second tab with the first shell of the list: the command goes to the selected tab only.
        CHECK(terminals[0]->tab_count() == 1 && !TerminalPanel::profiles().empty());
        terminals[0]->open_tab(TerminalPanel::profiles().front());
        CHECK(terminals[0]->tab_count() == 2);
        terminals[0]->run_command("echo hiasm-second-tab");
        for(int i = 0; i < 100; ++i) {
            settle();
            if(terminals[0]->output_view().get_buffer()->get_text().raw().find("hiasm-second-tab") != std::string::npos) break;
        }
        CHECK(terminals[0]->output_view().get_buffer()->get_text().raw().find("hiasm-second-tab") != std::string::npos);
        CHECK(repository->is_pinned() && repository->is_panel_visible());
        cmdMan->command(CMD_PN_REP); settle();
        CHECK(!repository->is_panel_visible() && !cmdMan->findByName(CMD_PN_REP)->checked);
        g_signal_emit_by_name(repository->collapsed_button().gobj(), "clicked"); settle();
        CHECK(repository->is_panel_visible() && repository->is_active() && !right->is_active());
        cmdMan->command(CMD_PN_REP); settle();
        CHECK(!repository->is_panel_visible());
        cmdMan->command(CMD_PN_REP); settle();
        CHECK(repository->is_panel_visible() && repository->get_mapped() && oneWindow());
        cmdMan->command(CMD_PN_DEBUG); settle();
        CHECK(!debug->is_panel_visible() && debug->get_parent() == nullptr);
        CHECK(debug->collapsed_button().get_parent() != nullptr);
        auto *bottomTabs = dynamic_cast<Gtk::Box *>(debug->collapsed_button().get_parent());
        CHECK(bottomTabs && bottomTabs->get_orientation() == Gtk::Orientation::HORIZONTAL);
        CHECK(debug->collapsed_button().get_width() > debug->collapsed_button().get_height());
        g_signal_emit_by_name(debug->collapsed_button().gobj(), "clicked"); settle();
        CHECK(debug->is_panel_visible() && debug->side() == DockSide::Bottom && debug->get_mapped());
        g_signal_emit_by_name(repository->pin_button().gobj(), "clicked"); settle();
        CHECK(!repository->is_pinned() && repository->get_parent() == &workspace && oneWindow());
        g_signal_emit_by_name(repository->pin_button().gobj(), "clicked"); settle();
        g_signal_emit_by_name(left->close_button().gobj(), "clicked");
        settle();
        CHECK(!left->is_panel_visible() && !cmdMan->findByName(CMD_ELEMENTS)->checked);
        cmdMan->command(CMD_ELEMENTS);
        settle();
        CHECK(left->is_panel_visible() && left->get_mapped() && oneWindow());
        CHECK(cmdMan->findByName(CMD_ELEMENTS)->checked);
        g_signal_emit_by_name(right->pin_button().gobj(), "clicked");
        settle();
        CHECK(!right->is_pinned() && right->get_parent() == &workspace && oneWindow());
        cmdMan->command(CMD_PROPERTY);
        settle();
        CHECK(!right->is_panel_visible());
        cmdMan->command(CMD_PROPERTY);
        settle();
        CHECK(right->is_panel_visible() && !right->is_pinned() && right->get_mapped() && oneWindow());
        cmdMan->command(CMD_QPAN_HIDEALL);
        settle();
        CHECK(!left->is_panel_visible() && !right->is_panel_visible() && !repository->is_panel_visible() && !debug->is_panel_visible() && oneWindow());
        cmdMan->command(CMD_QPAN_SHOWALL);
        settle();
        CHECK(left->is_panel_visible() && right->is_panel_visible() && repository->is_panel_visible() && debug->is_panel_visible() && oneWindow());
        cmdMan->command(CMD_PANELS);
        settle();
        CHECK(!left->is_panel_visible() && !right->is_panel_visible());
        cmdMan->command(CMD_PANELS);
        settle();
        CHECK(left->is_panel_visible() && right->is_panel_visible() && oneWindow());
        // Move a real properties panel and keep View available during updates.
        workspace.begin_drag(*right, 600, 80);
        workspace.update_drag(workspace.get_width() / 2, workspace.get_height() - 4);
        CHECK(workspace.preview_side() == int(DockSide::Bottom));
        workspace.end_drag();
        settle();
        CHECK(right->is_pinned() && right->side() == DockSide::Bottom && oneWindow());
        CHECK(cmdMan->isEnabled(CMD_OPEN) && cmdMan->isEnabled(CMD_ELEMENTS));
        window.hide();
        app->remove_window(window);
    }
    gchar *settings = nullptr;
    CHECK(g_file_get_contents((profile + "/.hiasm/settings.ini").c_str(), &settings, nullptr, nullptr));
    const std::string saved(settings);
    g_free(settings);
    CHECK(saved.find("properties_side=2") != std::string::npos);
    CHECK(saved.find("properties_attached=1") != std::string::npos);
    CHECK(saved.find("repository_visible=1") != std::string::npos);
    CHECK(saved.find("examples_side=1") != std::string::npos);
    std::cout << "OK main docks: real View commands, one visible native window, persisted layout" << std::endl;
    return 0;
}
