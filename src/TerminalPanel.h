#ifndef HIASM_TERMINAL_PANEL_H
#define HIASM_TERMINAL_PANEL_H

#include "share.h"
#include "Localization.h"
#include <memory>
#include <string>
#include <vector>

#ifdef HIASM_GTK4
// A shell that a tab of the terminal panel runs.
struct TerminalProfile {
    std::string name;               // title in the shell list and on the tab
    std::vector<std::string> argv;  // program and its arguments; the working directory is the current one
    std::string encoding;           // code page of the shell's output and input: "UTF-8", "CP866", ...
};

// Terminal panel: the terminal of the selected tab fills the panel. The tabs and the shell list
// (cmd, Windows PowerShell and WSL distributions on Windows) with the "+" button form one bar at the
// bottom. The text is typed straight into the tab; there is no command line.
class TerminalPanel : public Gtk::Box {
public:
    TerminalPanel();
    ~TerminalPanel() override;
    void set_working_directory(const std::string &directory);
    const std::string &working_directory() const { return workingDirectory_; }
    // Shells of the list: cmd, PowerShell and WSL distributions on Windows; the login shell elsewhere.
    static std::vector<TerminalProfile> profiles();
    // Opens a tab with the shell and selects it.
    void open_tab(const TerminalProfile &profile);
    // Sends a command line to the selected tab; the default shell is opened when there is no tab.
    void run_command(const std::string &command);
    // Text of the selected tab (opens the default shell when there is no tab).
    Gtk::TextView &output_view();
    int tab_count() const { return static_cast<int>(tabs_.size()); }
private:
    struct Tab;
    static void on_output(GObject *source, GAsyncResult *result, gpointer data);
    static void on_exit(GObject *source, GAsyncResult *result, gpointer data);
    Tab &selected_tab();
    void select_tab(Tab &tab);
    void close_tab(const std::shared_ptr<Tab> &tab);
    void open_selected_shell();
    bool spawn(Tab &tab);
    void read_output(const std::shared_ptr<Tab> &tab);
    std::string decode(Tab &tab, const std::string &bytes);
    void append_text(Tab &tab, const std::string &text);
    void submit(Tab &tab);
    void send(Tab &tab, const std::string &line);
    bool on_key(Tab &tab, guint key, Gdk::ModifierType state);

    std::vector<TerminalProfile> profiles_;
    std::vector<std::shared_ptr<Tab>> tabs_;
    std::weak_ptr<Tab> selected_;
    int nextTab_ = 0;
    std::string workingDirectory_;

    Gtk::Notebook notebook_;         // tabs along the bottom edge; the bar also holds the shell list
    Gtk::Box actions_{Gtk::Orientation::HORIZONTAL, 4};
    Gtk::ComboBoxText shell_;
    Gtk::Button add_;
};
#endif

#endif
