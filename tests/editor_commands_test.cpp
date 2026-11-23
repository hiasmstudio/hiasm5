#include "WindowManager.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "commands.h"
#include <glib/gstdio.h>
#include <iostream>

#define CHECK(expr) do { if(!(expr)) { std::cerr << __LINE__ << ": " << #expr << std::endl; return 1; } } while(0)
struct KeyCommand : CallBack {
    ustring command;
    void callback(void *, CallbackType, const void *data) override { command = *(const ustring*)data; }
};
struct DialogReplies {
    std::vector<int> responses;
    size_t sent = 0;
    sigc::connection timer;
    DialogReplies(std::initializer_list<int> values):responses(values) {
        timer = Glib::signal_timeout().connect([this]() {
            for(auto window : Gtk::Window::list_toplevels()) {
                if(auto dialog = dynamic_cast<Gtk::Dialog*>(window)) {
                    if(dialog->get_visible() && sent < responses.size()) {
                        dialog->response(responses[sent++]);
                        break;
                    }
                }
            }
            return sent < responses.size();
        },10);
    }
    ~DialogReplies() { timer.disconnect(); }
    bool complete() const { return sent == responses.size(); }
};
int main(int argc, char **argv) {
    if(argc != 2) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.editor-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs();
    dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    cmdMan = new CommandManager();
    {
        // Palette menus use their own target/availability without changing
        // the selected scheme's command state.
        bool available = false, called = false;
        auto *command = cmdMan->findByName(CMD_SHA_INI);
        CHECK(command);
        auto *item = new CM_ImageMenuItem(cmdMan, CMD_SHA_INI, "test",
            [&](const ustring&) { called = true; }, [&](const ustring&) { return available; });
        command->widgets.push_back(item);
        command->enabled = true; cmdMan->endUpdate();
        CHECK(!item->get_sensitive());
        item->on_activate(); CHECK(!called);
        available = true; command->enabled = false; cmdMan->endUpdate();
        CHECK(item->get_sensitive());
        item->on_activate(); CHECK(called);
        const auto count = command->widgets.size(); delete item;
        CHECK(command->widgets.size()+1 == count); cmdMan->endUpdate();
    }
    compilerSet = new CompilerCollection(); packSet = new PackCollection();
    gchar *tmp = g_dir_make_tmp("hiasm-editor-test-XXXXXX", nullptr);
    CHECK(tmp);
    const ustring dir(tmp); g_free(tmp);
    const ustring input = dir + "/example.cpp";
    CHECK(g_file_set_contents(input.c_str(), u8"世界: first; 世界: second", -1, nullptr));
    WindowManager manager;
    {
        cmdMan->on_command -= &manager;
        KeyCommand received;
        cmdMan->on_command += &received;
        cmdMan->enable(CMD_OPEN);
        GdkEventKey key = {}; key.keyval = 'o'; key.state = GDK_CONTROL_MASK;
        CHECK(manager.translateKey(&key,true) && received.command == CMD_OPEN);
        received.command.clear(); key.keyval = 'O';
        CHECK(manager.translateKey(&key,true) && received.command == CMD_OPEN);
        received.command.clear(); key.state |= (1 << 3); // Alt in GTK3 and GTK4.
        CHECK(!manager.translateKey(&key,true) && received.command.empty());
        key.state = GDK_CONTROL_MASK; key.keyval = 'c'; cmdMan->enable(CMD_COPY);
        CHECK(!manager.translateKey(&key,true) && received.command.empty());
        cmdMan->on_command -= &received;
        cmdMan->on_command += &manager;
    }
    auto editor = dynamic_cast<WM_Text_Editor*>(manager.open(input));
    CHECK(editor);
    auto buffer = editor->memo->get_buffer();
    buffer->place_cursor(buffer->begin());
    CHECK(editor->findNext(u8"世界"));
    auto start = buffer->begin(), end = buffer->end();
    CHECK(buffer->get_selection_bounds(start, end) && start.get_offset() == 0);
    CHECK(editor->processCommand(CMD_SEARCHNEXT, ""));
    CHECK(buffer->get_selection_bounds(start, end) && start.get_offset() == 11);
    CHECK(editor->findNext(u8"世界"));
    CHECK(buffer->get_selection_bounds(start, end) && start.get_offset() == 0);
    CHECK(!editor->findNext("absent"));
    CHECK(editor->processCommand(CMD_SELECTALL, ""));
    CHECK(buffer->get_selection_bounds(start, end) && buffer->get_text(start, end) == buffer->get_text());
    buffer->place_cursor(buffer->end());
    buffer->begin_user_action();
    buffer->insert_at_cursor(u8"; Привет");
    buffer->end_user_action();
    CHECK(editor->changed);
#ifdef HIASM_GTK4
    CHECK(buffer->get_can_undo());
    CHECK(editor->processCommand(CMD_UNDO, ""));
    CHECK(buffer->get_text() == u8"世界: first; 世界: second");
    CHECK(editor->processCommand(CMD_REDO, ""));
    CHECK(buffer->get_text() == u8"世界: first; 世界: second; Привет");
#endif
    const ustring before = editor->fileName();
    CHECK(!editor->saveToFile(dir + "/missing/file.cpp"));
    CHECK(editor->changed && editor->fileName() == before);
    const ustring output = dir + u8"/Сохранено 世界.cpp";
    CHECK(editor->saveToFile(output));
    CHECK(!editor->changed && editor->fileName() == output);
    gchar *bytes = nullptr; gsize size = 0;
    CHECK(g_file_get_contents(output.c_str(), &bytes, &size, nullptr));
    CHECK(std::string(bytes, size) == buffer->get_text().raw());
    g_free(bytes);
    CHECK(editor->processCommand(CMD_SAVE, ""));
    const ustring scheme = dir + "/selection.sha";
    CHECK(g_file_set_contents(scheme.c_str(), "Make(_base)\nAdd(hcDoData,1,10,20)\n{\n}\nAdd(hcDoData,2,100,150)\n{\n}", -1, nullptr));
    {
        // New projects are copied to an isolated cache directory before they
        // are saved, so the template remains untouched and builds have a real
        // project directory for relative paths.
        auto fresh = dynamic_cast<WM_SDK_Editor*>(manager.open(scheme, true));
        CHECK(fresh && fresh->isTemporaryProject());
        const ustring tempDir = fresh->temporaryProjectPath();
        CHECK(!tempDir.empty() && tempDir != dir);
        CHECK(fresh->fileName().find(tempDir) == 0);
        CHECK(file_test(fresh->fileName(), FILE_TEST_EXISTS));
        ProjectBuilder builder;
        CHECK(builder.getOutputProjectName(fresh->sde->sdk).find(tempDir) == 0);
        fresh->setChanged(false);
        manager.setCurrent(fresh);
        CHECK(manager.closeCurrent());
        CHECK(!file_test(tempDir, FILE_TEST_EXISTS));
    }
    auto diagram = dynamic_cast<WM_SDK_Editor*>(manager.open(scheme));
    CHECK(diagram);
    auto selection = diagram->sde->csdk->selMan;
    ustring original;
    diagram->sde->sdk->saveToText(original);
    CHECK(diagram->sde->sdk->showLinkHints);
    CHECK(diagram->processCommand(CMD_SHOW_LH, ""));
    CHECK(!diagram->sde->sdk->showLinkHints && !diagram->changed);
    ustring hidden;
    diagram->sde->sdk->saveToText(hidden);
    CHECK(hidden == original);
    CHECK(diagram->processCommand(CMD_SHOW_LH, ""));
    CHECK(diagram->sde->sdk->showLinkHints && !diagram->changed);
    auto unsupported = dynamic_cast<WM_SDK_Editor*>(manager.open(dataDir + "packs/cpp/new/Entry.sha"));
    CHECK(unsupported);
    manager.update();
    CHECK(!cmdMan->isEnabled(CMD_FORMEDIT));
    CHECK(unsupported->processCommand(CMD_FORMEDIT, ""));
    CHECK(!WidgetEditorEx::getActive(unsupported->sde));
    manager.setCurrent(diagram);
    auto it = diagram->sde->csdk->elements.begin();
    Element *a = *it++, *b = *it;
    a->width = 20; a->height = 30; b->width = 40; b->height = 50;
    selection->selectAll();
    const auto drawn = a->drawRect().join(b->drawRect());
    CHECK(selection->getSelectionRect() == drawn);
    const char *modes[] = {"left", "right", "center", "top", "bottom", "middle"};
    const int expected[][4] = {{10,20,10,150}, {120,20,100,150}, {65,20,55,150},
                              {10,20,100,20}, {10,170,100,150}, {10,95,100,85}};
    for(int i = 0; i < 6; ++i) {
        a->move(10-a->x, 20-a->y); b->move(100-b->x, 150-b->y);
        diagram->setChanged(false);
        CHECK(diagram->processCommand(CMD_ALIGN, modes[i]));
        CHECK(a->x == expected[i][0] && a->y == expected[i][1] &&
              b->x == expected[i][2] && b->y == expected[i][3]);
        CHECK(diagram->changed);
    }
    diagram->setChanged(false);
    CHECK(diagram->processCommand(CMD_ALIGN, "invalid"));
    CHECK(!diagram->changed);
    CHECK(diagram->processCommand(CMD_MOVETO, "200, 300"));
    CHECK(selection->getBodyRect().get_x() == 200 && selection->getBodyRect().get_y() == 300);
    CHECK(diagram->changed);
    for(const char *invalid : {"1,2junk", "1,2,3", "NaN,1", "1.5,2", "999999999999,2"}) {
        diagram->setChanged(false);
        CHECK(diagram->processCommand(CMD_MOVETO, invalid));
        CHECK(!diagram->changed);
    }
    selection->setFlag(ELEMENT_FLG_IS_FREEZE);
    CHECK(diagram->processCommand(CMD_MOVETO, "0 0"));
    CHECK(!diagram->changed && selection->getBodyRect().get_x() == 200);
    selection->unsetFlag(ELEMENT_FLG_IS_FREEZE);
    CHECK(diagram->processCommand(CMD_MOVETO, "-20;-30"));
    CHECK(diagram->changed && selection->getBodyRect().get_x() == -20 && selection->getBodyRect().get_y() == -30);
    {
        WindowManager closing;
        auto document = dynamic_cast<WM_Text_Editor*>(closing.open(output)); CHECK(document);
        document->memo->get_buffer()->set_text(u8"Не терять изменения");
        CHECK(document->changed && closing.tabs.size()==1);
        for(int cancel : std::initializer_list<int>{0,GTK_RESPONSE_CANCEL,GTK_RESPONSE_DELETE_EVENT,GTK_RESPONSE_NONE}) {
            DialogReplies replies{cancel};
            CHECK(!closing.closeCurrent() && replies.complete());
            CHECK(closing.getCurrent()==document && document->changed && closing.get_n_pages()==1);
        }
        {
            DialogReplies replies{GTK_RESPONSE_YES};
            CHECK(closing.closeCurrent() && replies.complete());
        }
        CHECK(closing.tabs.size()==1 && closing.get_n_pages()==1 && closing.getCurrent()->type==WMT_NEW_PROJECT);
        CHECK(closing.page_num(*closing.getCurrent()->widget())==0);
        CHECK(g_file_get_contents(output.c_str(), &bytes, &size, nullptr));
        CHECK(std::string(bytes,size)==u8"Не терять изменения"); g_free(bytes);
        auto unsavable=dynamic_cast<WM_Text_Editor*>(closing.open(dir+"/missing/unsaved.cpp")); CHECK(unsavable);
        unsavable->memo->get_buffer()->set_text("keep");
        {
            DialogReplies replies{GTK_RESPONSE_YES,GTK_RESPONSE_CLOSE};
            CHECK(!closing.closeCurrent() && replies.complete());
            CHECK(unsavable->changed && closing.getCurrent()==unsavable);
        }
        {
            DialogReplies replies{GTK_RESPONSE_NO};
            CHECK(closing.closeCurrent() && replies.complete());
        }
        auto first=dynamic_cast<WM_Text_Editor*>(closing.open(input)); CHECK(first);
        auto second=dynamic_cast<WM_Text_Editor*>(closing.open(output)); CHECK(second);
        first->memo->get_buffer()->set_text("first edited");
        second->memo->get_buffer()->set_text("second edited");
        const auto pages=closing.get_n_pages();
        {
            DialogReplies replies{GTK_RESPONSE_NO,GTK_RESPONSE_CANCEL};
            CHECK(!closing.closeAll() && replies.complete());
            CHECK(closing.get_n_pages()==pages && first->changed && second->changed);
        }
        {
            DialogReplies replies{GTK_RESPONSE_YES,GTK_RESPONSE_NO};
            CHECK(closing.closeAll() && replies.complete());
        }
        CHECK(closing.tabs.size()==1 && closing.get_n_pages()==1 && closing.getCurrent()->type==WMT_NEW_PROJECT);
        CHECK(!closing.closeCurrent());
        CHECK(closing.canCloseAll());
        closing.open(input);
        CHECK(closing.get_n_pages()==2);
        closing.processCommand(CMD_CLOSE,"");
        CHECK(closing.get_n_pages()==1);
        closing.open(input);
        closing.processCommand(CMD_CLOSEALL,"");
        CHECK(closing.get_n_pages()==1 && closing.getCurrent()->type==WMT_NEW_PROJECT);
    }
    g_unlink(scheme.c_str());
    g_unlink(input.c_str()); g_unlink(output.c_str()); g_rmdir(dir.c_str());
    std::cout << "OK editor commands: text search/undo/save; scheme alignment/move/freeze/dirty state" << std::endl;
    return 0;
}
