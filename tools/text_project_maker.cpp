// UTF-8 source writer shared by the C++ and web packages.
#include <gio/gio.h>
#include <gmodule.h>
#include <cstring>

struct MakeRecord { const char *result; const char *filename; const char *compiler; };
struct RunRecord { const char *filename; int mode, serverPort, clientPort; void *data; };

extern "C" G_MODULE_EXPORT int buildMakePrj(MakeRecord *record) {
    if(!record || !record->result || !record->filename) return 10;
    GError *error = nullptr;
    const bool ok = g_file_set_contents(record->filename, record->result, std::strlen(record->result), &error);
    if(error) { g_printerr("Project write failed: %s\n", error->message); g_error_free(error); }
    return ok ? 0 : 10;
}

extern "C" G_MODULE_EXPORT int buildRunProc(RunRecord *record) {
    if(!record || !record->filename || !g_file_test(record->filename, G_FILE_TEST_IS_REGULAR)) return 20;
    GError *error = nullptr;
    gchar *uri = g_filename_to_uri(record->filename, nullptr, &error);
    const bool ok = uri && g_app_info_launch_default_for_uri(uri, nullptr, &error);
    g_free(uri);
    if(error) { g_printerr("Project launch failed: %s\n", error->message); g_error_free(error); }
    return ok ? 0 : 20;
}
