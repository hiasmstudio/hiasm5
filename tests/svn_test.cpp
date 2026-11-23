#include "SvnClient.h"
#include <gio/gio.h>
#include <glib/gstdio.h>
#include <iostream>
#include <vector>
#include <functional>
#ifdef G_OS_WIN32
#include <windows.h>
#endif

#define CHECK(x) do { if(!(x)) { std::cerr << __LINE__ << ": " << #x << std::endl; return 1; } } while(0)
static bool command(const std::vector<std::string> &args) {
    std::vector<gchar*> argv; for(const auto &arg : args) argv.push_back(const_cast<gchar*>(arg.c_str())); argv.push_back(nullptr);
#ifdef G_OS_WIN32
    if(!args.empty() && (args[0].find("/usr/bin/") != std::string::npos || args[0].find("\\usr\\bin\\") != std::string::npos)) {
        for(size_t i = 1; i < args.size(); ++i) {
            if(args[i].size() < 3 || !g_ascii_isalpha(args[i][0]) || args[i][1] != ':' || (args[i][2] != '\\' && args[i][2] != '/')) continue;
            gchar *converted = g_strdup_printf("/%c%s", g_ascii_tolower(args[i][0]), args[i].c_str()+2);
            for(char *p = converted; *p; ++p) if(*p == '\\') *p = '/';
            argv[i] = converted;
        }
    }
#endif
    gint status = -1; gchar *out = nullptr, *err = nullptr; GError *error = nullptr;
    bool ok = g_spawn_sync(nullptr, argv.data(), nullptr, G_SPAWN_SEARCH_PATH, nullptr, nullptr, &out, &err, &status, &error) && g_spawn_check_wait_status(status, nullptr);
    if(!ok) std::cerr << (err ? err : "") << (error ? error->message : "") << std::endl;
    g_free(out); g_free(err); g_clear_error(&error);
#ifdef G_OS_WIN32
    for(size_t i=1;i<args.size();++i) if(argv[i] != args[i].c_str()) g_free(argv[i]);
#endif
    return ok;
}
static bool waitDone(bool &done) {
    const auto end = g_get_monotonic_time() + 20000000; while(!done && g_get_monotonic_time() < end) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(1000); } return done;
}
int main(int argc, char **argv) {
    if(argc != 3) return 2;
    g_setenv("HIASM_SVN", argv[1], true);
    g_setenv("LC_ALL", "C.UTF-8", true);
    CHECK(SvnClient::available());
    gchar *folder = g_dir_make_tmp("hiasm-svn-XXXXXX", nullptr); CHECK(folder); const std::string root(folder); g_free(folder);
    const auto repo = root + "/repo", wc = root + "/wc", other = root + "/other";
    CHECK(g_mkdir_with_parents(root.c_str(), 0700) == 0);
    CHECK(command({argv[2], "create", repo})); gchar *uri = g_filename_to_uri(repo.c_str(), nullptr, nullptr); CHECK(uri); std::string url(uri); g_free(uri);
#ifdef G_OS_WIN32
    // The optional MSYS client uses /c/... file URLs rather than Win32 C:/....
    std::string executable(argv[1]);
    if(executable.find("/usr/bin/") != std::string::npos || executable.find("\\usr\\bin\\") != std::string::npos) {
        if(url.size() > 10 && url.compare(0, 8, "file:///") == 0 && url[9] == ':') {
            url[8] = g_ascii_tolower(url[8]); url.erase(9, 1);
        }
    }
#endif
    CHECK(command({argv[1], "checkout", "--non-interactive", url, wc}));
#ifdef G_OS_WIN32
    // SVN 1.14 Windows clients still narrow argv through the system code page.
    // Exercise peg escaping here; the Linux fixture additionally covers Unicode.
    const std::string filename = "/scheme @ name.ini";
#else
    const std::string filename = u8"/Схема @ 世界.ini";
#endif
    const std::string file = wc + filename;
    CHECK(g_file_set_contents(file.c_str(), "initial\n", -1, nullptr)); CHECK(command({argv[1], "add", "--", file + "@"})); CHECK(command({argv[1], "commit", "-m", "initial", wc})); CHECK(command({argv[1], "checkout", "--non-interactive", url, other}));
    CHECK(SvnClient::isWorkingCopy(file)); CHECK(!SvnClient::isWorkingCopy(repo)); SvnClient client; bool done = false, ok = false; std::string output;
    auto complete = [&](bool success, const std::string &text) { ok = success; output = text; done = true; if(!ok) std::cerr << text << std::endl; };
    CHECK(client.start(SvnClient::Log, file, complete)); CHECK(client.busy() && !client.start(SvnClient::Log, file, complete)); CHECK(waitDone(done) && ok && !client.busy() && output.find("initial") != std::string::npos);
    CHECK(g_file_set_contents(file.c_str(), "changed\n", -1, nullptr)); done = false; CHECK(client.start(SvnClient::Diff, file, complete)); CHECK(waitDone(done) && ok && output.find("+changed") != std::string::npos);
    CHECK(command({argv[1], "revert", "--", file + "@"})); const std::string otherFile = other + filename; CHECK(g_file_set_contents(otherFile.c_str(), "from other checkout\n", -1, nullptr)); CHECK(command({argv[1], "commit", "-m", "update fixture", other}));
    done = false; CHECK(client.start(SvnClient::Update, file, complete)); CHECK(waitDone(done) && ok); gchar *contents = nullptr; CHECK(g_file_get_contents(file.c_str(), &contents, nullptr, nullptr)); CHECK(std::string(contents) == "from other checkout\n"); g_free(contents);
    bool calledAfterDestruction = false; { SvnClient cancelled; CHECK(cancelled.start(SvnClient::Log, file, [&](bool, const std::string&) { calledAfterDestruction = true; })); } const auto end = g_get_monotonic_time() + 500000; while(g_get_monotonic_time() < end) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(1000); } CHECK(!calledAfterDestruction);
    std::function<void(GFile*)> remove = [&](GFile *entry) {
        auto children = g_file_enumerate_children(entry, G_FILE_ATTRIBUTE_STANDARD_NAME, G_FILE_QUERY_INFO_NOFOLLOW_SYMLINKS, nullptr, nullptr);
        if(children) {
            while(auto info = g_file_enumerator_next_file(children, nullptr, nullptr)) {
                auto child = g_file_get_child(entry, g_file_info_get_name(info)); remove(child); g_object_unref(child); g_object_unref(info);
            }
            g_object_unref(children);
        }
#ifdef G_OS_WIN32
        gchar *filename = g_file_get_path(entry);
        auto wide = g_utf8_to_utf16(filename, -1, nullptr, nullptr, nullptr);
        if(wide) { SetFileAttributesW(reinterpret_cast<LPCWSTR>(wide), FILE_ATTRIBUTE_NORMAL); g_free(wide); }
        g_free(filename);
#endif
        g_file_delete(entry, nullptr, nullptr);
    };
    auto tempRoot = g_file_new_for_path(root.c_str()); remove(tempRoot); g_object_unref(tempRoot);
    CHECK(!g_file_test(root.c_str(), G_FILE_TEST_EXISTS));
    std::cout << "OK SVN: peg paths, log/diff/update, busy state and destruction" << std::endl;
}
