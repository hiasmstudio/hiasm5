#include "TerminalPanel.h"

#ifdef HIASM_GTK4
#include <gdk/gdkkeysyms.h>
#include <algorithm>
#ifdef G_OS_WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace {

// Text shown in a tab when a shell cannot be started or written to.
std::string failureText(const GError *error) {
    return std::string(ui_tr("Command failed")) + ": " +
           (error ? error->message : ui_tr("Unknown error")) + "\n";
}

// Distributions of WSL as wsl.exe lists them (UTF-16 text, one name per line).
std::vector<std::string> wslDistributions() {
    std::vector<std::string> names;
#ifdef G_OS_WIN32
    GError *error = nullptr;
    GSubprocess *process = g_subprocess_new(
        static_cast<GSubprocessFlags>(G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_SILENCE),
        &error, "wsl.exe", "--list", "--quiet", nullptr);
    if(!process) {
        g_clear_error(&error);
        return names;
    }
    GBytes *out = nullptr;
    const gboolean done = g_subprocess_communicate(process, nullptr, nullptr, &out, nullptr, &error);
    const bool ok = done && out && g_subprocess_get_exit_status(process) == 0;
    g_clear_error(&error);
    g_object_unref(process);
    if(!ok) {
        if(out) g_bytes_unref(out);
        return names;
    }
    gsize size = 0;
    const char *data = static_cast<const char *>(g_bytes_get_data(out, &size));
    gchar *text = g_convert(data, static_cast<gssize>(size), "UTF-8", "UTF-16LE", nullptr, nullptr, nullptr);
    g_bytes_unref(out);
    if(!text) return names;
    gchar **lines = g_strsplit_set(text, "\r\n", -1);
    for(gchar **line = lines; line && *line; ++line) {
        gchar *stripped = g_strdup(*line);
        g_strstrip(stripped);
        std::string name(stripped);
        g_free(stripped);
        // The text starts with a byte order mark (U+FEFF).
        if(name.compare(0, 3, "\xEF\xBB\xBF") == 0) name.erase(0, 3);
        if(!name.empty()) names.push_back(name);
    }
    g_strfreev(lines);
    g_free(text);
#endif
    return names;
}

}  // namespace

struct TerminalPanel::Tab {
    TerminalPanel *owner = nullptr;
    int id = 0;
    TerminalProfile profile;
    std::string pending;                // bytes of a character split between two reads
    GSubprocess *process = nullptr;
    GInputStream *output = nullptr;     // owned by the process
    GOutputStream *input = nullptr;     // owned by the process
    bool exited = false;
    Gtk::Box tabLabel{Gtk::Orientation::HORIZONTAL, 4};
    Gtk::Label title;
    Gtk::Button close;
    Gtk::ScrolledWindow scroll;
    Gtk::TextView view;
    Glib::RefPtr<Gtk::TextBuffer::Mark> inputMark;

    ~Tab() {
        if(process) {
            if(!exited) g_subprocess_force_exit(process);
            g_object_unref(process);
        }
    }
};

std::vector<TerminalProfile> TerminalPanel::profiles() {
    std::vector<TerminalProfile> list;
#ifdef G_OS_WIN32
    const std::string oem = "CP" + std::to_string(GetOEMCP());
    list.push_back({"cmd", {"cmd.exe", "/Q"}, oem});
    list.push_back({"Windows PowerShell", {"powershell.exe", "-NoLogo", "-NoProfile", "-NoExit", "-Command", "-"}, oem});
    if(gchar *pwsh = g_find_program_in_path("pwsh.exe")) {
        g_free(pwsh);
        list.push_back({"PowerShell 7", {"pwsh.exe", "-NoLogo", "-NoProfile", "-NoExit", "-Command", "-"}, oem});
    }
    for(const auto &distro : wslDistributions())
        list.push_back({"WSL: " + distro, {"wsl.exe", "-d", distro}, "UTF-8"});
#else
    const gchar *shell = g_getenv("SHELL");
    const std::string program = shell && *shell ? shell : "/bin/sh";
    gchar *base = g_path_get_basename(program.c_str());
    list.push_back({base, {program}, "UTF-8"});
    g_free(base);
#endif
    return list;
}

TerminalPanel::TerminalPanel(): Gtk::Box(Gtk::Orientation::VERTICAL, 4), profiles_(profiles()) {
    gchar *cwd = g_get_current_dir();
    workingDirectory_ = cwd ? cwd : g_get_home_dir();
    g_free(cwd);

    // The tabs along the bottom edge; the selected tab's terminal fills the rest of the panel.
    notebook_.set_tab_pos(Gtk::PositionType::BOTTOM);
    notebook_.set_scrollable(true);
    notebook_.set_hexpand(true);
    notebook_.set_vexpand(true);
    notebook_.signal_switch_page().connect([this](Gtk::Widget *page, guint) {
        for(auto &tab : tabs_)
            if(&tab->scroll == page) {
                selected_ = tab;
                tab->view.grab_focus();
                return;
            }
    });

    // The shell list and "+" sit at the right end of the same bar.
    for(const auto &profile : profiles_) shell_.append(profile.name);
    if(!profiles_.empty()) shell_.set_active(0);
    shell_.set_tooltip_text(ui_tr("Shell"));
    shell_.set_visible(profiles_.size() > 1);
    add_.set_icon_name("list-add-symbolic");
    add_.set_tooltip_text(ui_tr("New tab"));
    add_.signal_clicked().connect([this]() { open_selected_shell(); });
    actions_.append(shell_);
    actions_.append(add_);
    notebook_.set_action_widget(&actions_, Gtk::PackType::END);
    append(notebook_);
    set_hexpand(true);
    set_vexpand(true);
    set_margin_start(6);
    set_margin_end(6);
    set_margin_top(6);
    set_margin_bottom(6);

    // The first shell starts when the panel is shown, not when the window is built.
    signal_map().connect([this]() {
        if(tabs_.empty()) open_selected_shell();
    });
}

TerminalPanel::~TerminalPanel() {
    for(auto &tab : tabs_) {
        const int page = notebook_.page_num(tab->scroll);
        if(page >= 0) notebook_.remove_page(page);
    }
    tabs_.clear();
}

void TerminalPanel::set_working_directory(const std::string &directory) {
    if(!directory.empty() && g_file_test(directory.c_str(), G_FILE_TEST_IS_DIR))
        workingDirectory_ = directory;
}

void TerminalPanel::open_selected_shell() {
    if(profiles_.empty()) return;
    const int index = shell_.get_active_row_number();
    open_tab(profiles_[index >= 0 && index < static_cast<int>(profiles_.size()) ? index : 0]);
}

void TerminalPanel::open_tab(const TerminalProfile &profile) {
    auto tab = std::make_shared<Tab>();
    tab->owner = this;
    tab->id = ++nextTab_;
    tab->profile = profile;

    // Tab label: the shell name and a close button.
    tab->title.set_text(profile.name);
    tab->close.set_icon_name("window-close-symbolic");
    tab->close.add_css_class("flat");
    tab->close.set_tooltip_text(ui_tr("Close tab"));
    std::weak_ptr<Tab> weak = tab;
    tab->close.signal_clicked().connect([this, weak]() {
        // The button is still handling its click; the tab is closed on the next idle step.
        Glib::signal_idle().connect([this, weak]() {
            if(auto closing = weak.lock()) close_tab(closing);
            return false;
        });
    });
    tab->tabLabel.append(tab->title);
    tab->tabLabel.append(tab->close);

    // The terminal: typing goes straight into the tab, Enter sends the line to the shell.
    tab->view.set_editable(true);
    tab->view.set_cursor_visible(true);
    tab->view.set_monospace(true);
    tab->view.set_wrap_mode(Gtk::WrapMode::CHAR);
    tab->view.set_left_margin(4);
    tab->scroll.set_policy(Gtk::PolicyType::AUTOMATIC, Gtk::PolicyType::AUTOMATIC);
    tab->scroll.set_child(tab->view);
    tab->scroll.set_hexpand(true);
    tab->scroll.set_vexpand(true);
    auto keys = Gtk::EventControllerKey::create();
    keys->set_propagation_phase(Gtk::PropagationPhase::CAPTURE);
    Tab *raw = tab.get();
    keys->signal_key_pressed().connect([this, raw](guint key, guint, Gdk::ModifierType state) {
        return on_key(*raw, key, state);
    }, false);
    tab->view.add_controller(keys);
    auto buffer = tab->view.get_buffer();
    // Left gravity: the mark stays in front of the text typed after it, so the typed line is
    // everything from the mark to the end. Output is inserted by append_text, which moves the mark.
    auto end = buffer->end();
    tab->inputMark = buffer->create_mark(end, true);

    notebook_.append_page(tab->scroll, tab->tabLabel);
    tabs_.push_back(tab);
    if(spawn(*tab)) {
        read_output(tab);
        g_subprocess_wait_async(tab->process, nullptr, &TerminalPanel::on_exit, new std::weak_ptr<Tab>(tab));
    }
    select_tab(*tab);
}

TerminalPanel::Tab &TerminalPanel::selected_tab() {
    if(auto tab = selected_.lock()) return *tab;
    if(tabs_.empty()) open_selected_shell();
    select_tab(*tabs_.back());
    return *tabs_.back();
}

void TerminalPanel::select_tab(Tab &tab) {
    for(auto &item : tabs_)
        if(item.get() == &tab) {
            selected_ = item;
            break;
        }
    notebook_.set_current_page(notebook_.page_num(tab.scroll));
    tab.view.grab_focus();
}

void TerminalPanel::close_tab(const std::shared_ptr<Tab> &tab) {
    auto found = std::find(tabs_.begin(), tabs_.end(), tab);
    if(found == tabs_.end()) return;
    // Out of the list first: the page switch that removing the page causes must not select this tab again.
    tabs_.erase(found);
    if(selected_.lock() == tab) selected_.reset();
    const int page = notebook_.page_num(tab->scroll);
    if(page >= 0) notebook_.remove_page(page);
}

bool TerminalPanel::spawn(Tab &tab) {
    std::vector<std::string> args = tab.profile.argv;
    if(args.empty()) return false;
#ifdef G_OS_WIN32
    // WSL starts in the working directory of the panel.
    if(args[0] == "wsl.exe" && !workingDirectory_.empty()) {
        args.push_back("--cd");
        args.push_back(workingDirectory_);
    }
#endif
    if(gchar *program = g_find_program_in_path(args[0].c_str())) {
        args[0] = program;
        g_free(program);
    }
    std::vector<const char *> argv;
    for(const auto &arg : args) argv.push_back(arg.c_str());
    argv.push_back(nullptr);

    GSubprocessLauncher *launcher = g_subprocess_launcher_new(static_cast<GSubprocessFlags>(
        G_SUBPROCESS_FLAGS_STDIN_PIPE | G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_MERGE));
    if(!workingDirectory_.empty()) g_subprocess_launcher_set_cwd(launcher, workingDirectory_.c_str());
    GError *error = nullptr;
    tab.process = g_subprocess_launcher_spawnv(launcher, argv.data(), &error);
    g_object_unref(launcher);
    if(!tab.process) {
        append_text(tab, failureText(error));
        g_clear_error(&error);
        tab.exited = true;
        return false;
    }
    tab.input = g_subprocess_get_stdin_pipe(tab.process);
    tab.output = g_subprocess_get_stdout_pipe(tab.process);
    return true;
}

void TerminalPanel::read_output(const std::shared_ptr<Tab> &tab) {
    if(!tab->output) return;
    auto *holder = new std::weak_ptr<Tab>(tab);
    g_input_stream_read_bytes_async(tab->output, 4096, G_PRIORITY_DEFAULT, nullptr, &TerminalPanel::on_output, holder);
}

void TerminalPanel::on_output(GObject *source, GAsyncResult *result, gpointer data) {
    std::unique_ptr<std::weak_ptr<Tab>> holder(static_cast<std::weak_ptr<Tab> *>(data));
    GError *error = nullptr;
    GBytes *bytes = g_input_stream_read_bytes_finish(G_INPUT_STREAM(source), result, &error);
    g_clear_error(&error);
    if(!bytes) return;
    gsize size = 0;
    const char *chunk = static_cast<const char *>(g_bytes_get_data(bytes, &size));
    if(size > 0) {
        if(auto tab = holder->lock()) {
            tab->owner->append_text(*tab, tab->owner->decode(*tab, std::string(chunk, size)));
            tab->owner->read_output(tab);
        }
    }
    g_bytes_unref(bytes);
}

void TerminalPanel::on_exit(GObject *source, GAsyncResult *result, gpointer data) {
    std::unique_ptr<std::weak_ptr<Tab>> holder(static_cast<std::weak_ptr<Tab> *>(data));
    g_subprocess_wait_finish(G_SUBPROCESS(source), result, nullptr);
    auto tab = holder->lock();
    if(!tab) return;
    tab->exited = true;
    const int status = g_subprocess_get_exit_status(G_SUBPROCESS(source));
    tab->owner->append_text(*tab, "\n[" + std::string(ui_tr("Exit code")) + " " + std::to_string(status) + "]\n");
}

std::string TerminalPanel::decode(Tab &tab, const std::string &bytes) {
    tab.pending += bytes;
    if(tab.profile.encoding.empty() || tab.profile.encoding == "UTF-8") {
        std::string text;
        size_t pos = 0;
        while(pos < tab.pending.size()) {
            const gchar *p = tab.pending.data() + pos;
            const gssize rest = static_cast<gssize>(tab.pending.size() - pos);
            const gunichar c = g_utf8_get_char_validated(p, rest);
            // A character split between two reads waits for the next read.
            if(c == static_cast<gunichar>(-2)) break;
            // A byte that is not UTF-8 becomes a replacement character.
            if(c == static_cast<gunichar>(-1)) {
                text += "\xEF\xBF\xBD";
                pos += 1;
                continue;
            }
            const gchar *next = g_utf8_next_char(p);
            text.append(p, static_cast<size_t>(next - p));
            pos += static_cast<size_t>(next - p);
        }
        tab.pending.erase(0, pos);
        return text;
    }
    gsize written = 0;
    gchar *converted = g_convert_with_fallback(tab.pending.data(), static_cast<gssize>(tab.pending.size()),
        "UTF-8", tab.profile.encoding.c_str(), "?", nullptr, &written, nullptr);
    tab.pending.clear();
    if(!converted) return std::string();
    std::string text(converted, written);
    g_free(converted);
    return text;
}

void TerminalPanel::append_text(Tab &tab, const std::string &text) {
    if(text.empty()) return;
    auto buffer = tab.view.get_buffer();
    // The output goes in front of the line being typed; the input mark moves behind the new text.
    const int at = buffer->get_iter_at_mark(tab.inputMark).get_offset();
    const Glib::ustring chunk(text);
    auto where = buffer->get_iter_at_offset(at);
    buffer->insert(where, chunk);
    auto after = buffer->get_iter_at_offset(at + static_cast<int>(chunk.length()));
    buffer->move_mark(tab.inputMark, after);
    tab.view.scroll_to(buffer->get_insert());
}

void TerminalPanel::submit(Tab &tab) {
    auto buffer = tab.view.get_buffer();
    auto start = buffer->get_iter_at_mark(tab.inputMark);
    auto stop = buffer->end();
    const std::string line = buffer->get_text(start, stop, false).raw();
    auto last = buffer->end();
    buffer->insert(last, "\n");
    auto next = buffer->end();
    buffer->move_mark(tab.inputMark, next);
    send(tab, line);
}

void TerminalPanel::send(Tab &tab, const std::string &line) {
    if(tab.exited || !tab.input) return;
    std::string data = line + "\n";
    if(!tab.profile.encoding.empty() && tab.profile.encoding != "UTF-8") {
        gsize written = 0;
        gchar *converted = g_convert(data.data(), static_cast<gssize>(data.size()),
            tab.profile.encoding.c_str(), "UTF-8", nullptr, &written, nullptr);
        if(converted) {
            data.assign(converted, written);
            g_free(converted);
        }
    }
    GError *error = nullptr;
    if(!g_output_stream_write_all(tab.input, data.data(), data.size(), nullptr, nullptr, &error)) {
        append_text(tab, failureText(error));
        g_clear_error(&error);
    }
}

bool TerminalPanel::on_key(Tab &tab, guint key, Gdk::ModifierType state) {
    auto buffer = tab.view.get_buffer();
    const int modifiers = static_cast<int>(state);
    const bool control = (modifiers & GDK_CONTROL_MASK) != 0;
    const bool alt = (modifiers & GDK_ALT_MASK) != 0;
    if(!control && !alt && (key == GDK_KEY_Return || key == GDK_KEY_KP_Enter)) {
        submit(tab);
        return true;
    }
    // Ctrl+C copies the selected output as usual.
    if(control && (key == GDK_KEY_c || key == GDK_KEY_C)) return false;
    // Editing starts at the line being typed; the output above it stays read-only.
    auto input = buffer->get_iter_at_mark(tab.inputMark);
    const int start = input.get_offset();
    if(buffer->get_iter_at_mark(buffer->get_insert()).get_offset() < start) {
        auto end = buffer->end();
        buffer->place_cursor(end);
    }
    const int cursor = buffer->get_iter_at_mark(buffer->get_insert()).get_offset();
    if(cursor <= start && (key == GDK_KEY_BackSpace || key == GDK_KEY_Left)) return true;
    if(key == GDK_KEY_Home && !control && !alt) {
        buffer->place_cursor(input);
        return true;
    }
    return false;
}

void TerminalPanel::run_command(const std::string &command) {
    if(command.empty()) return;
    Tab &tab = selected_tab();
    auto buffer = tab.view.get_buffer();
    auto end = buffer->end();
    buffer->insert(end, command);
    submit(tab);
}

Gtk::TextView &TerminalPanel::output_view() {
    return selected_tab().view;
}
#endif
