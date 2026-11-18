// Cases for FileSearch/Execute, GUI-only elements and the wider form widget set.
// Included by cpp_hc_test.cpp after cpp_hc_forms.inc.

// The scheme must be rejected and the build log must mention `needle`.
static void expectFailure(const char *name, const std::function<void(Scheme&)> &make, const char *needle,
                          const char *project = "Entry") {
    if(onlyCase && strcmp(onlyCase, name)) return;
    ++executed;
    Scheme scheme(cppPack, project);
    make(scheme);
    if(scheme.failed) { ++failures; std::cerr << "FAIL " << name << ": scheme setup" << std::endl; return; }
    scheme.sdk.fileName = workDir + "/" + name + ".sha";
    debug.log.clear();
    cgt_on_debug += &debug;
    const bool built = builder->build(&scheme.sdk);
    cgt_on_debug -= &debug;
    if(built || debug.log.find(needle) == std::string::npos) {
        ++failures;
        std::cerr << "FAIL " << name << ": expected a build error mentioning \"" << needle << "\", built=" << built << "\n"
                  << debug.log << std::endl;
    }
}

// Runs a form scheme under the probe; `body` sees the window and may pump the main loop.
static void formCase(const char *name, const std::function<void(Scheme&)> &make, const std::string &body,
                     const std::string &expect, std::initializer_list<const char*> needles = {}) {
    if(onlyCase && strcmp(onlyCase, name)) return;
    ++executed;
    ustring directory;
    const auto source = buildSource(name, make, directory);
    if(source.empty()) { ++failures; return; }
    for(const char *needle : needles)
        if(source.find(needle) == std::string::npos) { ++failures; std::cerr << "FAIL " << name << ": missing " << needle << std::endl; return; }
    if(!probeForm(name, directory, body, expect)) { ++failures; std::cerr << "FAIL " << name << ": probe" << std::endl; }
}

static const char *pump =
    "  for(int i = 0; i < 40; ++i) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(10000); }\n";

static void checkMore() {
    // FileSearch: names that match the mask, then the others, then the count.
    if(!onlyCase || !strcmp(onlyCase, "file_search") || !strcmp(onlyCase, "file_search_flat") || !strcmp(onlyCase, "file_search_full")) {
        const ustring folder = workDir + "/search";
        g_mkdir_with_parents(folder.c_str(), 0755);
        for(const char *file : {"a.txt", "b.txt", "c.log"}) g_file_set_contents((folder + "/" + file).c_str(), "x", 1, nullptr);
        std::string path = folder.raw();
        std::replace(path.begin(), path.end(), '\\', '/');
        check("file_search", [&](Scheme &s) {
            auto find = s.add("hcFileSearch"), hit = printer(s), other = printer(s), done = printer(s);
            s.set(find, "Ext", "*.txt");
            s.set(find, "Dir", path);
            s.set(find, "FullName", "1");
            s.set(find, "SubDir", "1");
            s.reveal(find, "onOtherFiles");
            s.start(find, "doSearch");
            s.link(find, "onSearch", hit, "doPrint");
            s.link(find, "onOtherFiles", other, "doPrint");
            s.link(find, "onEndSearch", done, "doPrint");
            s.link(done, "Text1", find, "Count");
        }, "a.txt\nb.txt\nc.log\n2\n");
        const ustring one = workDir + "/search_one";
        g_mkdir_with_parents(one.c_str(), 0755);
        g_file_set_contents((one + "/only.txt").c_str(), "x", 1, nullptr);
        std::string pathOne = one.raw();
        std::replace(pathOne.begin(), pathOne.end(), '\\', '/');
        std::string expectedFull = pathOne;
        expectedFull += G_DIR_SEPARATOR_S;
        expectedFull += "only.txt\n";
        check("file_search_flat", [&](Scheme &s) {
            auto find = s.add("hcFileSearch"), hit = printer(s);
            s.set(find, "Ext", "*.txt");
            s.set(find, "Dir", path);
            s.set(find, "SubDir", "1");
            s.set(find, "FullName", "1");
            s.start(find, "doSearch");
            s.link(find, "onSearch", hit, "doPrint");
        }, "a.txt\nb.txt\n");
        check("file_search_full", [&](Scheme &s) {
            auto find = s.add("hcFileSearch"), hit = printer(s);
            s.set(find, "Ext", "*.txt");
            s.set(find, "Dir", pathOne);
            s.set(find, "SubDir", "1");
            s.set(find, "FullName", "0");
            s.start(find, "doSearch");
            s.link(find, "onSearch", hit, "doPrint");
        }, expectedFull);
    }
    check("execute", [](Scheme &s) {
        auto run = s.add("hcExecute"), out = printer(s), mark = constant(s, "done");
        s.set(run, "Application", "echo");
        s.set(run, "Args", "hello");
        s.start(run, "doExec");
        s.link(run, "onExec", mark, "doData");
        s.link(mark, "onEventData", out, "doPrint");
    }, "hello\ndone\n");
    // Elements that need a window fail the build of a console project instead of vanishing.
    expectFailure("timer_console", [](Scheme &s) {
        auto timer = s.add("hcTimer");
        s.start(timer, "doTimer");
    }, "hcTimer needs a GUI project");
    expectFailure("message_console", [](Scheme &s) {
        auto message = s.add("hcMessage");
        s.start(message, "doMessage");
    }, "hcMessage needs a GUI project");
    expectFailure("clipboard_console", [](Scheme &s) {
        auto clip = s.add("hcClipboard");
        s.set(clip, "Text", "x");
        s.start(clip, "doPutText");
    }, "hcClipboard needs a GUI project");

    formCase("form_clipboard_message", [](Scheme &s) {
        auto clip = s.add("hcClipboard"), message = s.add("hcMessage");
        auto put = s.add("Button"), show = s.add("Button"), read = s.add("Button"), out = printer(s);
        s.set(clip, "Text", "clipboard text");
        s.set(message, "Message", "Hi");
        s.set(message, "Caption", "Title");
        s.link(put, "onClick", clip, "doPutText");
        s.link(show, "onClick", message, "doMessage");
        s.link(read, "onClick", out, "doPrint");
        s.link(out, "Text1", clip, "CurrentText");
    }, "", "form probe OK", {"hiasm_message(", "hiasm_clipboard_text()", "gdk_clipboard_set_text"});

    formCase("form_props", [](Scheme &s) {
        auto edit = s.add("Edit"), check = s.add("CheckBox"), track = s.add("TrackBar");
        auto progress = s.add("ProgressBar"), out = printer(s);
        s.set(check, "Checked", "0");
        s.set(track, "Max", "100");
        s.link(edit, "onChange", out, "doPrint");
        s.link(out, "Text1", edit, "Text");
        s.link(track, "onPosition", progress, "doPosition");
    },
        "  auto edit = hiasm_widget(\"Edit2\"); if(!edit) return 20;\n"
        "  gtk_editable_set_text(GTK_EDITABLE(edit.get()), \"abc\");\n"
        "  auto check = hiasm_widget(\"CheckBox3\"); if(!check || !gtk_check_button_get_active(GTK_CHECK_BUTTON(check.get()))) return 21;\n"
        "  auto track = hiasm_range(\"TrackBar4\"); if(!track) return 22;\n"
        "  gtk_adjustment_set_value(track, 40);\n"
        "  auto progress = GTK_PROGRESS_BAR(hiasm_widget(\"ProgressBar5\").get()); if(!progress) return 23;\n"
        "  if(hiasm_progress_fraction_of(progress) < 0.39 || hiasm_progress_fraction_of(progress) > 0.41) return 24;\n",
        "abc\nform probe OK", {"gtk_editable_get_text", "gtk_check_button_set_active", "gtk_adjustment_get_value", "gtk_progress_bar_set_fraction"});

    formCase("form_lists", [](Scheme &s) {
        auto list = s.add("ListBox"), combo = s.add("ComboBox"), memo = s.add("Memo");
        auto out1 = printer(s), out2 = printer(s), out3 = printer(s);
        s.set(list, "Strings", "x\ny");
        s.set(combo, "Strings", "m\nn");
        s.set(memo, "Strings", "p\nq");
        s.link(list, "onClick", out1, "doPrint");
        s.link(combo, "onClick", out2, "doPrint");
        s.link(memo, "onChange", out3, "doPrint");
    },
        "  auto list = hiasm_list_box(\"ListBox2\"); if(!list) return 30;\n"
        "  if(hiasm_list_lines(list).size() != 2) return 31;\n"
        "  hiasm_list_select(list, 1);\n"
        "  auto combo = hiasm_combo(\"ComboBox3\"); if(!combo) return 32;\n"
        "  if(hiasm_combo_lines(combo).size() != 2) return 33;\n"
        "  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 1);\n"
        "  auto memo = hiasm_text_view(\"Memo4\"); if(!memo) return 34;\n"
        "  if(hiasm_view_text(memo) != \"p\\nq\") return 35;\n"
        "  gtk_text_buffer_set_text(gtk_text_view_get_buffer(memo), \"z\", -1);\n",
        "1\n1\nz\nform probe OK", {"gtk_list_box_append", "gtk_combo_box_text_append_text", "hiasm_view_text"});

    formCase("form_widgets", [](Scheme &s) {        auto memo = s.add("hcTplMemo"), list = s.add("hcTplListBox"), combo = s.add("hcTplComboBox"), bar = s.add("hcTplScrollBar");
        auto out1 = printer(s), out2 = printer(s), out3 = printer(s);
        s.set(memo, "Strings", "p\nq");
        s.set(list, "Strings", "x\ny");
        s.set(combo, "Strings", "m\nn");
        s.set(bar, "Max", "100");
        s.set(bar, "Position", "10");
        s.link(list, "onClick", out1, "doPrint");
        s.link(combo, "onClick", out2, "doPrint");
        s.link(bar, "onPosition", out3, "doPrint");
    },
        "  auto memo = hiasm_text_view(\"hcTplMemo2\"); if(!memo) return 10;\n"
        "  if(hiasm_view_text(memo) != \"p\\nq\") return 11;\n"
        "  hiasm_view_add(memo, \"r\"); if(hiasm_view_lines(memo) != 3) return 12;\n"
        "  auto list = hiasm_list_box(\"hcTplListBox3\"); if(!list) return 13;\n"
        "  if(hiasm_list_lines(list).size() != 2) return 14;\n"
        "  hiasm_list_select(list, 1);\n"
        "  auto combo = hiasm_combo(\"hcTplComboBox4\"); if(!combo) return 15;\n"
        "  gtk_combo_box_set_active(GTK_COMBO_BOX(combo), 1);\n"
        "  auto bar = hiasm_range(\"hcTplScrollBar5\"); if(!bar) return 16;\n"
        "  if(hiasm_range_value(\"hcTplScrollBar5\") != 10) return 17;\n"
        "  gtk_adjustment_set_value(bar, 40);\n",
        "1\n1\n40\nform probe OK", {"gtk_scrolled_window_new", "gtk_scrollbar_new", "gtk_combo_box_text_new"});

    // The Delphi-named set ("Windows" group) compiles to the same widgets as the Core templates.
    formCase("form_delphi", [](Scheme &s) {
        auto button = s.add("Button"), label = s.add("Label"), edit = s.add("Edit"), check = s.add("CheckBox");
        auto radio1 = s.add("RadioButton"), radio2 = s.add("RadioButton"), track = s.add("TrackBar"), group = s.add("GroupBox");
        auto panel = s.add("Panel"), progress = s.add("ProgressBar"), memo = s.add("Memo"), list = s.add("ListBox");
        auto combo = s.add("ComboBox"), message = s.add("Message"), timer = s.add("Timer"), out = printer(s);
        (void)label; (void)edit; (void)check; (void)group; (void)progress; (void)memo; (void)list; (void)combo; (void)timer;
        s.set(radio1, "Checked", "0");
        s.set(track, "Max", "100");
        s.link(panel, "onClick", out, "doPrint");
        s.link(button, "onClick", message, "doMessage");
        s.set(message, "Message", "Hi");
        (void)radio2;
    },
        "  auto first = hiasm_widget(\"RadioButton6\"), second = hiasm_widget(\"RadioButton7\"); if(!first || !second) return 10;\n"
        "  if(!gtk_check_button_get_active(GTK_CHECK_BUTTON(first.get()))) return 11;\n"
        "  gtk_check_button_set_active(GTK_CHECK_BUTTON(second.get()), TRUE);\n"
        "  if(gtk_check_button_get_active(GTK_CHECK_BUTTON(first.get()))) return 12;\n"
        "  if(!gtk_check_button_get_active(GTK_CHECK_BUTTON(second.get()))) return 13;\n"
        "  auto track = hiasm_range(\"TrackBar8\"); if(!track) return 14;\n"
        "  gtk_adjustment_set_value(track, 30); if(hiasm_range_value(\"TrackBar8\") != 30) return 15;\n"
        "  auto group = hiasm_widget(\"GroupBox9\"); if(!group || !GTK_IS_FRAME(group.get()) || gtk_widget_get_can_target(group.get())) return 16;\n"
        "  auto panel = hiasm_widget(\"Panel10\"); if(!panel || !GTK_IS_FRAME(panel.get()) || !gtk_widget_get_can_target(panel.get())) return 17;\n"
        "  if(!hiasm_text_view(\"Memo12\") || !hiasm_list_box(\"ListBox13\") || !hiasm_combo(\"ComboBox14\")) return 18;\n",
        "form probe OK", {"gtk_scale_new", "gtk_frame_new", "hiasm_radio_join", "hiasm_message("});

    // ChildPanel and ChildForm (HiAsm 4 "Panel" and "Child form"): the elements of the container's scheme are created
    // inside its Panel or its window, and events cross the frame both ways (sys.inner_parent, rtcgRealLink).
    formCase("form_child_panel", [](Scheme &s) {
        auto container = s.add("ChildPanel"), inward = constant(s, "Hello"), outward = constant(s, "inner"), out = printer(s);
        Element *panel = container ? container->getParentElement() : nullptr, *frame = nullptr;
        for(Element *e : container ? container->sdk->elements : ElementsList()) if(e->cIndex == CI_EditMulti) frame = e;
        Element *button = container ? container->sdk->add("Button", 40, 40) : nullptr;
        if(!panel || !frame || !button) { s.failed = true; return; }
        s.set(panel, "Left", "10"); s.set(panel, "Top", "20"); s.set(panel, "Width", "200"); s.set(panel, "Height", "120");
        // Inside: the button's click goes out through the frame; the frame's event sets the panel caption.
        frame->connectToPoint(button->points.getByName("onClick"));
        frame->connectToPoint(panel->points.getByName("doCaption"));
        s.link(container, container->getLastPoint(pt_event)->name.c_str(), outward, "doData");
        s.link(outward, "onEventData", out, "doPrint");
        s.link(s.entry, "onCreate", inward, "doData");
        s.link(inward, "onEventData", container, container->getLastPoint(pt_work)->name.c_str());
    },
        "  GtkWidget *panel = nullptr, *button = nullptr;\n"
        "  for(auto &item : hiasm_widgets) {\n"
        "    if(item.first.rfind(\"Panel\", 0) == 0) panel = item.second;\n"
        "    if(item.first.rfind(\"Button\", 0) == 0) button = item.second;\n"
        "  }\n"
        "  if(!panel || !GTK_IS_FRAME(panel) || gtk_widget_get_parent(panel) != surface) return 10;\n"
        "  GtkWidget *client = gtk_frame_get_child(GTK_FRAME(panel)); if(!GTK_IS_FIXED(client)) return 11;\n"
        "  if(!button || gtk_widget_get_parent(button) != client) return 12;\n"
        "  int width = 0, height = 0; gtk_widget_get_size_request(panel, &width, &height);\n"
        "  if(width != 200 || height != 120) return 13;\n"
        "  const char *caption = gtk_frame_get_label(GTK_FRAME(panel)); if(!caption || std::string(caption) != \"Hello\") return 14;\n"
        "  g_signal_emit_by_name(button, \"clicked\");\n",
        "inner\nform probe OK", {"hiasm_parents.push_back(panel_client)", "gtk_frame_set_child"});

    formCase("form_child_form", [](Scheme &s) {
        auto container = s.add("ChildForm"), outward = constant(s, "child"), out = printer(s);
        Element *form = container ? container->getParentElement() : nullptr, *frame = nullptr;
        for(Element *e : container ? container->sdk->elements : ElementsList()) if(e->cIndex == CI_EditMulti) frame = e;
        Element *button = container ? container->sdk->add("Button", 40, 40) : nullptr;
        if(!form || !frame || !button) { s.failed = true; return; }
        s.set(form, "Caption", "Second");
        frame->connectToPoint(button->points.getByName("onClick"));
        s.link(container, container->getLastPoint(pt_event)->name.c_str(), outward, "doData");
        s.link(outward, "onEventData", out, "doPrint");
    },
        "  GtkWidget *child = nullptr, *button = nullptr;\n"
        "  for(auto &item : hiasm_widgets) {\n"
        "    if(item.first.rfind(\"MainForm\", 0) == 0 && item.second != GTK_WIDGET(window)) child = item.second;\n"
        "    if(item.first.rfind(\"Button\", 0) == 0) button = item.second;\n"
        "  }\n"
        "  if(!child || !GTK_IS_WINDOW(child)) return 20;\n"
        "  if(gtk_window_get_transient_for(GTK_WINDOW(child)) != window) return 21;\n"
        "  if(std::string(gtk_window_get_title(GTK_WINDOW(child))) != \"Second\") return 22;\n"
        "  GtkWidget *client = gtk_window_get_child(GTK_WINDOW(child)); if(!GTK_IS_FIXED(client)) return 23;\n"
        "  if(!button || gtk_widget_get_parent(button) != client) return 24;\n"
        "  if(!gtk_widget_get_visible(child)) return 25;\n"
        "  g_signal_emit_by_name(button, \"clicked\");\n"
        "  gtk_window_destroy(GTK_WINDOW(child));\n",
        "child\nform probe OK", {"gtk_window_set_transient_for(GTK_WINDOW(child), hiasm_main_window)", "hiasm_parents.push_back(client)"});

    // Width of a text field: the C++ Edit and the Core hcTplEdit with the same Width get that width in the program.
    // The allocation is the element's Width; the widget's content is smaller by the theme's entry margins.
    formCase("form_edit_width", [](Scheme &s) {
        auto edit = s.add("Edit"), core = s.add("hcTplEdit");
        s.set(edit, "Width", "200");
        s.set(core, "Width", "200");
    },
        "  gtk_window_present(window);\n"
        "  for(int i = 0; i < 60; ++i) { while(g_main_context_iteration(nullptr, false)) {} g_usleep(10000); }\n"
        "  GtkWidget *edit = nullptr, *core = nullptr;\n"
        "  for(auto &item : hiasm_widgets) {\n"
        "    if(item.first.rfind(\"Edit\", 0) == 0) edit = item.second;\n"
        "    if(item.first.rfind(\"hcTplEdit\", 0) == 0) core = item.second;\n"
        "  }\n"
        "  std::cout << \"WIDTH edit=\" << gtk_widget_get_allocated_width(edit) << \" core=\" << gtk_widget_get_allocated_width(core) << std::endl;\n",
        "WIDTH edit=200 core=200");

    // The timer starts on onCreate and stops itself after AutoStop ticks.
    formCase("form_timer", [](Scheme &s) {
        auto timer = s.add("hcTimer"), tick = constant(s, "t"), stop = constant(s, "s"), out1 = printer(s), out2 = printer(s);
        s.set(timer, "Interval", "10");
        s.set(timer, "AutoStop", "3");
        s.link(s.entry, "onCreate", timer, "doTimer");
        s.link(timer, "onTimer", tick, "doData");
        s.link(timer, "onStop", stop, "doData");
        s.link(tick, "onEventData", out1, "doPrint");
        s.link(stop, "onEventData", out2, "doPrint");
    }, std::string(pump), "t\nt\nt\ns\nform probe OK", {"hiasm_timer(\"hcTimer"});
}
