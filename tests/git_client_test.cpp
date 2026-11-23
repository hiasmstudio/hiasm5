#include "GitClient.h"
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <iostream>
#include <vector>
#include <functional>

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)
static bool command(const std::string &git, const std::string &cwd, std::vector<std::string> arguments) {
    std::vector<const char *> args{git.c_str(), "-c", "user.name=Fixture", "-c", "user.email=fixture@localhost", "-c", "commit.gpgsign=false"};
    for(const auto &arg : arguments) args.push_back(arg.c_str());
    args.push_back(nullptr);
    auto launcher = g_subprocess_launcher_new(static_cast<GSubprocessFlags>(G_SUBPROCESS_FLAGS_STDOUT_PIPE | G_SUBPROCESS_FLAGS_STDERR_PIPE));
    g_subprocess_launcher_set_cwd(launcher, cwd.c_str());
    GError *error = nullptr; auto process = g_subprocess_launcher_spawnv(launcher, args.data(), &error); g_object_unref(launcher);
    if(!process) { std::cerr << error->message << std::endl; g_error_free(error); return false; }
    gchar *out = nullptr, *err = nullptr;
    const bool ok = g_subprocess_communicate_utf8(process, nullptr, nullptr, &out, &err, &error) && g_subprocess_get_successful(process);
    if(!ok) std::cerr << (err ? err : "") << (error ? error->message : "") << std::endl;
    g_free(out); g_free(err); g_clear_error(&error); g_object_unref(process); return ok;
}
static bool waitDone(bool &done) {
    const auto end = g_get_monotonic_time() + 15000000;
    while(!done && g_get_monotonic_time() < end) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(1000); }
    return done;
}
int main(int argc, char **argv) {
    if(argc != 2) return 2;
    g_setenv("HIASM_GIT", argv[1], true);
    CHECK(GitClient::available());
    gchar *folder = g_dir_make_tmp("hiasm-git-XXXXXX", nullptr); CHECK(folder);
    const std::string root(folder); g_free(folder);
    const std::string source = root + u8"/source 'каталог 世界", clone = root + "/clone", other = root + "/other", empty = root + "/empty";
    for(const auto &path : {source, clone, other, empty}) CHECK(g_mkdir_with_parents(path.c_str(), 0700) == 0);
    g_setenv("GIT_CONFIG_NOSYSTEM", "1", true);
    g_setenv("GIT_CONFIG_GLOBAL", (root + "/no-global-config").c_str(), true);
    GitClient client; bool done = false, ok = false; std::string output;
    auto complete = [&](bool success, const std::string &text) { ok = success; output = text; done = true; };
    CHECK(!GitClient::isWorkingCopy(source));
    CHECK(!client.start(GitClient::Log, source, complete));
    CHECK(client.start(GitClient::Init, source, complete)); CHECK(waitDone(done) && ok && !client.busy());
    const std::string file = source + u8"/схема @ ' 世界.sha";
    CHECK(g_file_set_contents(file.c_str(), "initial\n", -1, nullptr));
    CHECK(command(argv[1], source, {"add", "--", "."})); CHECK(command(argv[1], source, {"commit", "-m", "initial fixture"}));
    CHECK(GitClient::isWorkingCopy(file));
    done = false; CHECK(client.start(GitClient::Log, file, complete)); CHECK(client.busy() && !client.start(GitClient::Log, file, complete));
    CHECK(waitDone(done) && ok && output.find("initial fixture") != std::string::npos);
    CHECK(g_file_set_contents(file.c_str(), "changed 世界\n", -1, nullptr));
    done = false; CHECK(client.start(GitClient::Diff, file, complete)); CHECK(waitDone(done) && ok && output.find("+changed 世界") != std::string::npos);
    done = false; CHECK(client.start(GitClient::Status, file, complete)); CHECK(waitDone(done) && ok && output.find("схема") != std::string::npos);
    CHECK(command(argv[1], source, {"add", "--", "."})); CHECK(command(argv[1], source, {"commit", "-m", "changed fixture"}));
    done = false; CHECK(client.clone(source, clone, complete)); CHECK(waitDone(done) && ok && GitClient::isWorkingCopy(clone));
    CHECK(!client.clone(source, clone, complete));
    CHECK(g_file_set_contents(file.c_str(), "upstream update\n", -1, nullptr));
    CHECK(command(argv[1], source, {"add", "--", "."})); CHECK(command(argv[1], source, {"commit", "-m", "upstream update"}));
    done = false; CHECK(client.start(GitClient::Pull, clone, complete)); CHECK(waitDone(done) && ok);
    gchar *contents = nullptr; CHECK(g_file_get_contents((clone + u8"/схема @ ' 世界.sha").c_str(), &contents, nullptr, nullptr));
    CHECK(std::string(contents) == "upstream update\n"); g_free(contents);
    CHECK(command(argv[1], clone, {"commit", "--allow-empty", "-m", "local divergence"}));
    CHECK(command(argv[1], source, {"commit", "--allow-empty", "-m", "upstream divergence"}));
    done = false; CHECK(client.start(GitClient::Pull, clone, complete)); CHECK(waitDone(done) && !ok);
    // .git files must be recognized in linked worktrees, including nested files.
    CHECK(command(argv[1], source, {"worktree", "add", "--detach", other}));
    CHECK(GitClient::isWorkingCopy(other + "/nested/nonexistent.sha"));
    bool calledAfterDestruction = false;
    { GitClient cancelled; CHECK(cancelled.start(GitClient::Log, source, [&](bool, const std::string &) { calledAfterDestruction = true; })); }
    const auto end = g_get_monotonic_time() + 250000;
    while(g_get_monotonic_time() < end) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(1000); }
    CHECK(!calledAfterDestruction);
    done = false; CHECK(client.clone("--not-an-option", empty, complete)); CHECK(waitDone(done) && !ok);
    g_setenv("HIASM_GIT", "hiasm-no-such-git", true); CHECK(!GitClient::available()); CHECK(!client.start(GitClient::Status, source, complete));
    std::cout << "OK Git: Unicode, init/clone/status/log/diff/pull, ff-only refusal, worktrees, busy/lifetime, missing client" << std::endl;
    return 0;
}

