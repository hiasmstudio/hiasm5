// Cases for the form-based project (MainForm). Included by cpp_hc_test.cpp.

// Builds the scheme and returns the generated C++ source ("" on failure).
static std::string buildSource(const char *name, const std::function<void(Scheme&)> &make, ustring &directory) {
    Scheme scheme(cppPack, "MainForm");
    make(scheme);
    if(scheme.failed) return {};
    scheme.sdk.fileName = workDir + "/" + name + ".sha";
    debug.log.clear();
    cgt_on_debug += &debug;
    const bool built = builder->build(&scheme.sdk);
    cgt_on_debug -= &debug;
    if(!built) { std::cerr << "FAIL " << name << ": build failed\n" << debug.log << std::endl; return {}; }
    const ustring artifact = builder->getOutputProjectName(&scheme.sdk);
    gchar *parent = g_path_get_dirname(artifact.c_str());
    directory = parent; g_free(parent);
    gchar *code = nullptr;
    if(!g_file_get_contents((directory + "/" + name + ".cpp").c_str(), &code, nullptr, nullptr)) return {};
    std::string text = code; g_free(code);
    return text;
}

// Compiles the generated program with its main() renamed and runs `body`
// (which sees activate()) against a registered but never run application.
static bool probeForm(const char *name, const ustring &directory, const std::string &body, const std::string &expect) {
    const std::string probe = "#define main hiasm_generated_main\n#include \"" + std::string(name) + ".cpp\"\n#undef main\n"
        "int main() {\n  auto app = gtk_application_new(\"com.hiasm.form-probe\", G_APPLICATION_NON_UNIQUE);\n"
        "  if(!g_application_register(G_APPLICATION(app), nullptr, nullptr)) return 1;\n"
        "  activate(app, nullptr);\n  auto windows = gtk_application_get_windows(app);\n"
        "  if(!windows || g_list_length(windows) != 1) return 2;\n  auto window = GTK_WINDOW(windows->data);\n"
        "  auto surface = gtk_window_get_child(window);\n  if(!GTK_IS_FIXED(surface)) return 3;\n" + body +
        "  gtk_window_destroy(window); g_object_unref(app);\n  std::cout << \"form probe OK\" << std::endl;\n  return 0;\n}\n";
    const ustring source = directory + "/probe.cpp";
    if(!g_file_set_contents(source.c_str(), probe.c_str(), probe.size(), nullptr)) return false;
    ustring pkg = "pkg-config";
    gchar *pkgArgs[] = {const_cast<gchar*>(pkg.c_str()), const_cast<gchar*>("--cflags"), const_cast<gchar*>("--libs"), const_cast<gchar*>("gtk4"), nullptr};
    gchar *flags = nullptr; gint status = -1;
    if(!g_spawn_sync(nullptr, pkgArgs, nullptr, G_SPAWN_SEARCH_PATH, nullptr, nullptr, &flags, nullptr, &status, nullptr) ||
       !g_spawn_check_wait_status(status, nullptr)) return false;
    // Windows GTK4 forms call GDI and USER (regions, icon, taskbar), as ProjectBuilder's systemLibraries adds.
#ifdef G_OS_WIN32
    const ustring windowsLibraries = " -lgdi32 -luser32";
#else
    const ustring windowsLibraries;
#endif
    const ustring command = ustring("g++ -std=c++17 -g probe.cpp -o probe ") + flags + windowsLibraries;
    g_free(flags);
    gchar **args = nullptr;
    if(!g_shell_parse_argv(command.c_str(), nullptr, &args, nullptr)) return false;
    gchar *errors = nullptr;
    bool ok = g_spawn_sync(directory.c_str(), args, nullptr, G_SPAWN_SEARCH_PATH, nullptr, nullptr, nullptr, &errors, &status, nullptr) &&
              g_spawn_check_wait_status(status, nullptr);
    g_strfreev(args);
    if(!ok) { std::cerr << "probe compile failed\n" << (errors ? errors : "") << std::endl; g_free(errors); return false; }
    g_free(errors);
    const std::string output = execute(directory + "/probe.exe");
    if(output.find(expect) == std::string::npos) { std::cerr << "probe output: " << output << std::endl; return false; }
    return true;
}

static void checkForms() {
    if(onlyCase && strcmp(onlyCase, "form_controls")) return;
    ++executed;
    ustring directory;
    const auto source = buildSource("form_controls", [](Scheme &s) {
        auto form = s.entry;
        s.set(form, "Caption", "Форма 世界");
        auto button = s.add("hcTplButton"), edit = s.add("hcTplEdit"), label = s.add("hcTplLabel");
        auto check = s.add("hcTplCheckBox"), progress = s.add("hcTplProgressBar");
        s.set(button, "Left", "20"); s.set(button, "Top", "30"); s.set(button, "Caption", "Нажми");
        s.set(edit, "Left", "20"); s.set(edit, "Top", "70"); s.set(edit, "Text", "abc");
        s.set(label, "Left", "20"); s.set(label, "Top", "110"); s.set(label, "Caption", "Надпись");
        s.set(check, "Left", "20"); s.set(check, "Top", "140"); s.set(check, "Checked", "0");
        s.set(progress, "Left", "20"); s.set(progress, "Top", "170");
        auto out = s.add("hcConsole");
        s.set(out, "Count", "1");
        s.link(button, "onClick", out, "doPrint");
        s.link(edit, "Text", out, "Text1");
    }, directory);
    if(source.empty()) { ++failures; return; }
    for(const char *needle : {"gtk_button_new_with_label", "gtk_entry_new", "gtk_label_new", "gtk_check_button_new_with_label",
                              "gtk_progress_bar_new", "hiasm_place(", "gtk_fixed_put", "\"clicked\""})
        if(source.find(needle) == std::string::npos) { ++failures; std::cerr << "FAIL form_controls: missing " << needle << std::endl; return; }
    const bool ok = probeForm("form_controls", directory,
        "  if(std::string(gtk_window_get_title(window)) != u8\"Форма 世界\") return 4;\n"
        "  int buttons = 0, entries = 0, labels = 0, checks = 0, bars = 0;\n"
        "  for(auto w = gtk_widget_get_first_child(surface); w; w = gtk_widget_get_next_sibling(w)) {\n"
        "    buttons += GTK_IS_BUTTON(w) && !GTK_IS_CHECK_BUTTON(w); entries += GTK_IS_ENTRY(w); labels += GTK_IS_LABEL(w);\n"
        "    checks += GTK_IS_CHECK_BUTTON(w); bars += GTK_IS_PROGRESS_BAR(w);\n  }\n"
        "  if(buttons != 1 || entries != 1 || labels != 1 || checks != 1 || bars != 1) return 5;\n"
        "  auto button = hiasm_widget(\"hcTplButton2\");\n  if(!button) return 6;\n"
        "  double x = 0, y = 0; gtk_widget_translate_coordinates(button.get(), surface, 0, 0, &x, &y);\n"
        "  g_signal_emit_by_name(button.get(), \"clicked\");\n", "form probe OK");
    if(!ok) { ++failures; std::cerr << "FAIL form_controls: probe" << std::endl; }
}
