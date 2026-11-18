#include "SvnClient.h"
#include <gio/gio.h>
#include <vector>

namespace {
gchar *svnExecutable() {
    const gchar *configured = g_getenv("HIASM_SVN");
    return g_find_program_in_path(configured && *configured ? configured : "svn");
}
std::string parentPath(const std::string &path) {
    gchar *parent = g_path_get_dirname(path.c_str());
    std::string result(parent);
    g_free(parent);
    return result;
}
std::string svnPath(const std::string &path, const gchar *executable) {
#ifdef G_OS_WIN32
    const std::string program(executable ? executable : "");
    if(program.find("/usr/bin/") != std::string::npos || program.find("\\usr\\bin\\") != std::string::npos) {
        if(path.size() >= 3 && g_ascii_isalpha(path[0]) && path[1] == ':' &&
           (path[2] == '\\' || path[2] == '/')) {
            std::string converted("/");
            converted += static_cast<char>(g_ascii_tolower(path[0]));
            converted += path.substr(2);
            for(char &ch : converted) if(ch == '\\') ch = '/';
            return converted;
        }
    }
#else
    (void)executable;
#endif
    return path;
}
std::string validUtf8(const char *text) {
    gchar *valid = g_utf8_make_valid(text ? text : "", -1);
    std::string result(valid);
    g_free(valid);
    return result;
}
}

struct SvnClient::State {
    SvnClient *owner;
    GSubprocess *process;
    GCancellable *cancel;
    Completion completion;
    ~State() { g_object_unref(cancel); g_object_unref(process); }
};

bool SvnClient::available() {
    gchar *path = svnExecutable();
    const bool result = path != nullptr;
    g_free(path);
    return result;
}

bool SvnClient::isWorkingCopy(const std::string &target) {
    if(target.empty()) return false;
    gchar *canonical = g_canonicalize_filename(target.c_str(), nullptr);
    std::string path(canonical); g_free(canonical);
    if(!g_file_test(path.c_str(), G_FILE_TEST_IS_DIR)) path = parentPath(path);
    for(;;) {
        if(g_file_test((path + G_DIR_SEPARATOR_S ".svn").c_str(), G_FILE_TEST_IS_DIR)) return true;
        const auto parent = parentPath(path);
        if(parent == path) return false;
        path = parent;
    }
}

SvnClient::~SvnClient() {
    if(state) {
        state->owner = nullptr;
        state->completion = nullptr;
        g_cancellable_cancel(state->cancel);
        g_subprocess_force_exit(state->process);
    }
}

bool SvnClient::start(Action action, const std::string &target, Completion completion) {
    if(state || !isWorkingCopy(target)) return false;
    gchar *executable = svnExecutable();
    if(!executable) return false;
    // log/update parse peg revisions; local diff accepts the filename literally.
    gchar *canonical = g_canonicalize_filename(target.c_str(), nullptr);
    std::string file(canonical); g_free(canonical);
    const std::string argumentTarget = svnPath(file, executable);
    std::string pegTarget = action == Diff ? argumentTarget : argumentTarget + "@";
    std::vector<const gchar*> args{executable};
    if(action == Update) args.insert(args.end(), {"update", "--accept", "postpone"});
    else if(action == Log) args.insert(args.end(), {"log", "--limit", "20"});
    else args.push_back("diff");
    args.insert(args.end(), {"--non-interactive", "--no-auth-cache", "--", pegTarget.c_str(), nullptr});
    auto launcher = g_subprocess_launcher_new(static_cast<GSubprocessFlags>(G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE));
    const std::string workingDir = g_file_test(file.c_str(), G_FILE_TEST_IS_DIR) ? file : parentPath(file);
    g_subprocess_launcher_set_cwd(launcher, workingDir.c_str());
    g_subprocess_launcher_setenv(launcher, "LC_ALL", "C.UTF-8", true);
    GError *error = nullptr;
    GSubprocess *process = g_subprocess_launcher_spawnv(launcher, args.data(), &error);
    g_object_unref(launcher); g_free(executable);
    if(!process) {
        const std::string message = error ? error->message : "Cannot start svn";
        g_clear_error(&error);
        if(completion) completion(false, message);
        return false;
    }
    state = std::make_shared<State>();
    state->owner = this; state->process = process;
    state->cancel = g_cancellable_new(); state->completion = std::move(completion);
    auto holder = new std::shared_ptr<State>(state);
    g_subprocess_communicate_utf8_async(process, nullptr, state->cancel,
        [](GObject *source, GAsyncResult *result, gpointer user) {
            std::unique_ptr<std::shared_ptr<State>> holder(static_cast<std::shared_ptr<State>*>(user));
            const auto state = *holder;
            gchar *out = nullptr, *err = nullptr;
            GError *failure = nullptr;
            const bool communicated = g_subprocess_communicate_utf8_finish(G_SUBPROCESS(source), result, &out, &err, &failure);
            const bool ok = communicated && g_subprocess_get_successful(state->process);
            std::string message = validUtf8(out) + validUtf8(err);
            if(failure) { message += validUtf8(failure->message); g_error_free(failure); }
            g_free(out); g_free(err);
            auto completion = std::move(state->completion);
            if(state->owner) state->owner->state.reset();
            if(completion) completion(ok, message);
        }, holder);
    return true;
}
