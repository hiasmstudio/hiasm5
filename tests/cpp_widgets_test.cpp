// Delphi WinControl properties of the C++ Widgets palette: every property the
// palette declares must reach the generated GTK code, and a form built from
// them must still compile. The test uses no example schemes of its own.
#include "SDK.h"
#include "CodeGen.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "ProjectBuilder.h"
#include <glib/gstdio.h>
#include <iostream>
#include <string>

struct DebugLog : CallBack {
    std::string log;
    void callback(void *, CallbackType, const void *data) override {
        if(data) { log += (const char*)data; log += '\n'; }
    }
};

int main(int argc, char **argv) {
    if(argc != 2) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.cpp-widgets-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs();
    dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    compilerSet = new CompilerCollection(); packSet = new PackCollection();
    MSDK widgets;
    widgets.pack = packSet->getPackByName("cpp");
    if(!widgets.pack) return 12;
    widgets.packProject = widgets.pack->getProjectByEntry("MainForm");
    if(!widgets.packProject) return 13;
    gchar *temp = g_dir_make_tmp("hiasm-widgets-test-XXXXXX", nullptr);
    if(!temp) return 14;
    const ustring dir(temp); g_free(temp);
    widgets.fileName = dir + "/widgets.sha";
    auto form = widgets.add("MainForm", 0, 0);
    auto edit = widgets.add("Edit", 10, 10), button = widgets.add("Button", 10, 40), label = widgets.add("Label", 10, 70);
    auto list = widgets.add("ListBox", 10, 100), combo = widgets.add("ComboBox", 120, 100), memo = widgets.add("Memo", 10, 160);
    auto track = widgets.add("TrackBar", 10, 260), progress = widgets.add("ProgressBar", 10, 290);
    if(!form || !edit || !button || !label || !list || !combo || !memo || !track || !progress) return 110;
    form->flag |= ELEMENT_FLG_IS_PARENT;
    // Combos are indexes; Delphi "True,False" switches are 0 for True.
    int yes = 0, right = 1, integer = 2, five = 5, scrollBoth = 3, tickCount = 5, maximum = 200, vertical = 1;
    edit->props.getByName("Alignment")->setValue(&right);
    edit->props.getByName("DataType")->setValue(&integer);
    edit->props.getByName("MaxLength")->setValue(&five);
    edit->props.getByName("ClearAfterEnter")->setValue(&yes);
    button->props.getByName("Flat")->setValue(&yes);
    button->props.getByName("DefaultBtn")->setValue(&yes);
    label->props.getByName("Alignment")->setValue(&right);
    list->props.getByName("MultiSelect")->setValue(&yes);
    list->props.getByName("AddType")->setValue(&right);
    list->props.getByName("SelectAdd")->setValue(&yes);
    combo->props.getByName("AddType")->setValue(&right);
    ustring comboText = u8"Pick";
    combo->props.getByName("Text")->setValue(&comboText);
    memo->props.getByName("ScrollBars")->setValue(&scrollBoth);
    track->props.getByName("TickCount")->setValue(&tickCount);
    progress->props.getByName("Max")->setValue(&maximum);
    progress->props.getByName("Kind")->setValue(&vertical);
    // Event-driven points are generated only when wired: one wire per source point.
    button->points.getByName("onClick")->connect(combo->points.getByName("doAdd"));
    edit->points.getByName("onEnter")->connect(list->points.getByName("doSelectAll"));
    memo->points.getByName("onChange")->connect(list->points.getByName("doAdd"));
    track->points.getByName("onPosition")->connect(progress->points.getByName("doValue"));

    ProjectBuilder builder;
    DebugLog debug;
    debug.log.clear(); cgt_on_debug += &debug;
    const bool built = builder.build(&widgets);
    cgt_on_debug -= &debug;
    if(!built) { std::cerr << debug.log << std::endl; return 111; }
    const ustring artifact = builder.getOutputProjectName(&widgets);
    gchar *folder = g_path_get_dirname(artifact.c_str());
    const ustring source = ustring(folder) + "/widgets.cpp";
    gchar *text = nullptr; gsize size = 0;
    const bool readSource = g_file_get_contents(source.c_str(), &text, &size, nullptr);
    const std::string generated = readSource ? std::string(text, size) : std::string();
    g_free(text);
    const char *expected[] = {
        "gtk_entry_set_alignment(", "gtk_entry_set_max_length(", "GTK_INPUT_PURPOSE_DIGITS", "gtk_button_set_has_frame(",
        "hiasm_set_default(\"", "gtk_list_box_set_selection_mode(", "GTK_SELECTION_MULTIPLE", "gtk_list_box_select_all(",
        "hiasm_list_select(", "gtk_combo_box_text_prepend_text(", "gtk_combo_box_get_child(", "gtk_scrolled_window_set_policy(",
        "GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC", "gtk_scale_add_mark(", "hiasm_progress_fraction_of(", "GTK_ORIENTATION_VERTICAL",
        "gtk_editable_set_text(GTK_EDITABLE(source), \"\");"};
    const char *missing = nullptr;
    for(const char *fragment : expected)
        if(!missing && generated.find(fragment) == std::string::npos) missing = fragment;
    g_unlink(artifact.c_str()); g_unlink(source.c_str()); g_rmdir(folder);
    g_free(folder);
    g_rmdir((dir + "/build").c_str()); g_rmdir(dir.c_str());
    if(!readSource) { std::cerr << "Generated form source is missing" << std::endl; return 112; }
    if(missing) { std::cerr << "Widget codegen lacks: " << missing << std::endl; return 113; }
    std::cout << "OK cpp widgets: Delphi properties reach the generated GTK form and compile" << std::endl;
    return 0;
}
