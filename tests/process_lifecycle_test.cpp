#include "ProjectBuilder.h"
#include "MainDataBase.h"
#include <glib/gstdio.h>
#ifndef G_OS_WIN32
#include <sys/stat.h>
#endif

static bool waitStopped(ProjectBuilder &builder) {
    const gint64 deadline = g_get_monotonic_time() + 5*G_TIME_SPAN_SECOND;
    while(builder.isRunning() && g_get_monotonic_time() < deadline) {
        while(g_main_context_iteration(NULL, false)) {}
        g_usleep(1000);
    }
    return !builder.isRunning();
}
int main(int argc, char **argv) {
    if(argc != 3) return 2;
    Glib::init(); Gio::init();
    dataDir = ustring(argv[2]) + G_DIR_SEPARATOR_S;
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open();
    gchar *tmp = g_dir_make_tmp("hiasm-process-test-XXXXXX", NULL);
    if(!tmp) return 3;
    const ustring dir(tmp); g_free(tmp);
    // An empty Core SDK is now intentionally a non-executable document.
    // Use the runnable C++ profile for this process-management fixture.
    MSDK sdk;
    Pack pack("cpp", "test");
    sdk.pack = &pack;
    sdk.fileName = dir + "/lifecycle.sha";
    ProjectBuilder builder;
    const ustring executable = builder.getOutputProjectName(&sdk);
    GFile *from = g_file_new_for_path(argv[1]), *to = g_file_new_for_path(executable.c_str());
    bool copied = g_file_copy(from, to, G_FILE_COPY_NONE, NULL, NULL, NULL, NULL);
    g_object_unref(from); g_object_unref(to);
    if(!copied) return 4;
#ifndef G_OS_WIN32
    chmod(executable.c_str(), 0700);
#endif
    {
        builder.run(&sdk);
        if(!builder.isRunning() || !waitStopped(builder)) return 5;
        for(int i = 0; i < 3; ++i) {
            builder.run(&sdk);
            if(!builder.isRunning()) return 6;
            builder.run(&sdk); // Must not overwrite the existing child handle.
            builder.stop();
            if(!waitStopped(builder)) return 7;
        }
        // Closing the tab while a program runs must not leave a callback to it.
        { ProjectBuilder detached; detached.run(&sdk); }
        const gint64 deadline = g_get_monotonic_time() + G_TIME_SPAN_SECOND;
        while(g_get_monotonic_time() < deadline) {
            while(g_main_context_iteration(NULL, false)) {}
            g_usleep(1000);
        }
    }
    g_unlink(executable.c_str());
    gchar *profile = g_path_get_dirname(executable.c_str());
    g_rmdir(profile); g_free(profile);
    g_rmdir((dir + "/build").c_str()); g_rmdir(dir.c_str());
    return 0;
}
