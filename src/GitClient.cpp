#include "GitClient.h"
#include <gio/gio.h>

namespace {
gchar *gitExecutable() {
    const gchar *configured = g_getenv("HIASM_GIT");
    return g_find_program_in_path(configured && *configured ? configured : "git");
}
std::string parentPath(const std::string &path) {
    gchar *parent = g_path_get_dirname(path.c_str());
    std::string result(parent); g_free(parent); return result;
}
std::string absolutePath(const std::string &path) {
    gchar *canonical = g_canonicalize_filename(path.c_str(), nullptr);
    std::string result(canonical); g_free(canonical); return result;
}
std::string bytesText(GBytes *bytes) {
    if(!bytes) return {};
    gsize size = 0; const auto data = static_cast<const char *>(g_bytes_get_data(bytes, &size));
    gchar *valid = g_utf8_make_valid(data ? data : "", size);
    std::string result(valid); g_free(valid); return result;
}
}

struct GitClient::State {
    GitClient *owner = nullptr;
    GSubprocess *process = nullptr;
    GCancellable *cancel = nullptr;
    Completion completion;
    guint timeout = 0;
    bool timedOut = false;
    ~State() {
        if(timeout) g_source_remove(timeout);
        g_clear_object(&cancel); g_clear_object(&process);
    }
};

bool GitClient::available() {
    gchar *path = gitExecutable();
    const bool ok = path != nullptr; g_free(path); return ok;
}
std::string GitClient::workingCopyRoot(const std::string &target) {
    if(target.empty()) return {};
    auto path = absolutePath(target);
    if(!g_file_test(path.c_str(), G_FILE_TEST_IS_DIR)) path = parentPath(path);
    for(;;) {
        const auto marker = path + G_DIR_SEPARATOR_S + ".git";
        // Linked worktrees use a .git file instead of a directory.
        if(g_file_test(marker.c_str(), G_FILE_TEST_IS_DIR) || g_file_test(marker.c_str(), G_FILE_TEST_IS_REGULAR)) return path;
        const auto parent = parentPath(path);
        if(parent == path) return {};
        path = parent;
    }
}
bool GitClient::isWorkingCopy(const std::string &target) { return !workingCopyRoot(target).empty(); }
GitClient::~GitClient() {
    if(state) {
        state->owner = nullptr; state->completion = nullptr;
        g_cancellable_cancel(state->cancel); g_subprocess_force_exit(state->process);
    }
}

bool GitClient::start(Action action, const std::string &target, Completion completion) {
    if(state || target.empty()) return false;
    auto root = workingCopyRoot(target);
    if(action == Init) {
        if(!root.empty() || !g_file_test(target.c_str(), G_FILE_TEST_IS_DIR)) return false;
        return launch({"init", "--initial-branch=main", "."}, absolutePath(target), std::move(completion));
    }
    if(root.empty()) return false;
    std::vector<std::string> args;
    if(action == Pull) args = {"pull", "--ff-only", "--no-rebase"};
    else if(action == Log) args = {"log", "-20", "--oneline", "--decorate"};
    else if(action == Status) args = {"status", "--short", "--branch", "--untracked-files=normal"};
    else args = {"diff", "--no-ext-diff", "--no-textconv", "HEAD", "--"};
    // A directory means the whole repository. For a file, keep diff scoped.
    if(action == Diff && !g_file_test(target.c_str(), G_FILE_TEST_IS_DIR)) {
        auto base = g_file_new_for_path(root.c_str());
        auto file = g_file_new_for_path(absolutePath(target).c_str());
        gchar *relative = g_file_get_relative_path(base, file);
        if(relative) { args.push_back(relative); g_free(relative); }
        g_object_unref(base); g_object_unref(file);
    }
    return launch(args, root, std::move(completion));
}
bool GitClient::clone(const std::string &url, const std::string &directory, Completion completion) {
    if(state || url.empty() || directory.empty() || !g_file_test(directory.c_str(), G_FILE_TEST_IS_DIR)) return false;
    GDir *dir = g_dir_open(directory.c_str(), 0, nullptr);
    if(!dir) return false;
    const bool empty = g_dir_read_name(dir) == nullptr; g_dir_close(dir);
    if(!empty) return false;
    return launch({"clone", "--", url, "."}, absolutePath(directory), std::move(completion));
}

bool GitClient::launch(const std::vector<std::string> &arguments, const std::string &cwd, Completion completion) {
    gchar *executable = gitExecutable();
    if(!executable) return false;
    std::vector<const gchar*> args{executable, "--no-pager", "-c", "color.ui=false", "-c", "core.quotepath=false"};
    for(const auto &argument : arguments) args.push_back(argument.c_str());
    args.push_back(nullptr);
    auto launcher = g_subprocess_launcher_new(static_cast<GSubprocessFlags>(G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE));
    g_subprocess_launcher_set_cwd(launcher, cwd.c_str());
    g_subprocess_launcher_setenv(launcher, "LC_ALL", "C.UTF-8", true);
    g_subprocess_launcher_setenv(launcher, "GIT_TERMINAL_PROMPT", "0", true);
    g_subprocess_launcher_setenv(launcher, "GCM_INTERACTIVE", "Never", true);
    GError *error = nullptr;
    GSubprocess *process = g_subprocess_launcher_spawnv(launcher, args.data(), &error);
    g_object_unref(launcher); g_free(executable);
    if(!process) {
        const std::string message = error ? error->message : "Cannot start git";
        g_clear_error(&error); if(completion) completion(false, message); return false;
    }
    state = std::make_shared<State>();
    state->owner = this; state->process = process;
    state->cancel = g_cancellable_new(); state->completion = std::move(completion);
    state->timeout = g_timeout_add_seconds(60, [](gpointer user) -> gboolean {
        auto state = static_cast<State *>(user);
        state->timeout = 0; state->timedOut = true;
        g_cancellable_cancel(state->cancel); g_subprocess_force_exit(state->process);
        return G_SOURCE_REMOVE;
    }, state.get());
    auto holder = new std::shared_ptr<State>(state);
    g_subprocess_communicate_async(process, nullptr, state->cancel, [](GObject *source, GAsyncResult *result, gpointer user) {
        std::unique_ptr<std::shared_ptr<State>> holder(static_cast<std::shared_ptr<State>*>(user));
        const auto state = *holder;
        GBytes *out = nullptr, *err = nullptr; GError *failure = nullptr;
        const bool communicated = g_subprocess_communicate_finish(G_SUBPROCESS(source), result, &out, &err, &failure);
        const bool ok = communicated && g_subprocess_get_successful(state->process);
        std::string message = bytesText(out) + bytesText(err);
        if(failure) { message += failure->message; g_error_free(failure); }
        if(state->timedOut) message += "\nGit operation timed out after 60 seconds.";
        if(out) g_bytes_unref(out);
        if(err) g_bytes_unref(err);
        if(state->timeout) { g_source_remove(state->timeout); state->timeout = 0; }
        auto done = std::move(state->completion);
        if(state->owner) state->owner->state.reset();
        if(done) done(ok, message);
    }, holder);
    return true;
}

