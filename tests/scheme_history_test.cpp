#include "WindowManager.h"
#include "MainDataBase.h"
#include "SettingsManager.h"
#include "commands.h"
#include <glib/gstdio.h>
#include <iostream>

#define CHECK(expr) do { if(!(expr)) { std::cerr << __LINE__ << ": " << #expr << std::endl; return 1; } } while(0)
static void drain() { while(g_main_context_iteration(nullptr, false)) {} }
int main(int argc, char **argv) {
    if(argc != 2) return 2;
#ifdef HIASM_GTK4
    auto app = Gtk::Application::create("com.hiasm.history-test", Gio::Application::Flags::NON_UNIQUE);
    app->register_application();
#else
    Gtk::Main kit(argc, argv);
#endif
    initDirs(); dataDir = ustring(argv[1]) + G_DIR_SEPARATOR_S;
    databaseFile = dataDir + INT_PATH + DATABASE_FILE;
    mdb.open(); SettingsManager::init();
    cmdMan = new CommandManager(); compilerSet = new CompilerCollection(); packSet = new PackCollection();
    gchar *tmp = g_dir_make_tmp("hiasm-history-XXXXXX", nullptr); CHECK(tmp);
    const ustring dir(tmp); g_free(tmp);
    const ustring input = dir + "/history.sha";
    std::string text = "Make(_base)";
    auto line = [&](const char *s) { text += char(10); text += s; };
    line("Add(hcDoData,1,10,20)"); line("{"); line("}");
    line("Add(hcDoData,2,100,150)"); line("{"); line("}");
    CHECK(g_file_set_contents(input.c_str(), text.c_str(), text.size(), nullptr));
    WindowManager manager;
    auto editor = dynamic_cast<WM_SDK_Editor*>(manager.open(input)); CHECK(editor);
    auto root = [&]() { return editor->sde->sdk; };
    auto element = [&](int id) { return root()->getGlobalElementById(id); };
    auto command = [&](const char *c, const char *a = "") { return editor->processCommand(c, a); };
    root()->selMan->select(element(1));
    CHECK(command(CMD_MOVETO, "60,70")); CHECK(element(1)->x == 60 && editor->changed);
    CHECK(command(CMD_UNDO)); CHECK(element(1)->x == 10 && !editor->changed);
    CHECK(root()->selMan->getFirst() == element(1));
    CHECK(command(CMD_REDO)); CHECK(element(1)->x == 60 && editor->changed);
    CHECK(command(CMD_SAVE)); CHECK(!editor->changed);
    CHECK(command(CMD_UNDO)); CHECK(editor->changed && element(1)->x == 10);
    CHECK(command(CMD_REDO)); CHECK(!editor->changed && element(1)->x == 60);
    CHECK(command(CMD_FREEZ)); CHECK(element(1)->flag & ELEMENT_FLG_IS_FREEZE);
    CHECK(command(CMD_UNDO)); CHECK(!(element(1)->flag & ELEMENT_FLG_IS_FREEZE) && !editor->changed);
    CHECK(command(CMD_REDO)); CHECK(element(1)->flag & ELEMENT_FLG_IS_FREEZE);
    CHECK(command(CMD_UNDO));
    // A new edit after undo drops the redo branch.
    CHECK(command(CMD_MOVETO, "80,90")); CHECK(command(CMD_REDO)); CHECK(element(1)->x == 80);
    CHECK(command(CMD_UNDO)); CHECK(element(1)->x == 60);
    CHECK(command(CMD_REDO));
    element(1)->points[1]->connect(element(2)->points[0])->createPath();
    ustring hint = u8"Привет 世界"; element(1)->sysProps.getByName("Hint")->setValue(&hint);
    editor->sde->changeScheme(); drain();
    CHECK(command(CMD_DELETE)); CHECK(!element(1) && root()->elements.size() == 1);
    CHECK(command(CMD_UNDO)); CHECK(element(1) && root()->elements.size() == 2);
    CHECK(element(1)->sysProps.getByName("Hint")->readStr() == hint);
    CHECK(element(1)->points[1]->point == element(2)->points[0]);
    CHECK(command(CMD_REDO)); CHECK(!element(1)); CHECK(command(CMD_UNDO));
    // Replacement must preserve user data and routed links or leave the model intact.
    const ustring replacementFile = dataDir + "packs/_base/conf/QADoData.ini";
    CHECK(!g_file_test(replacementFile.c_str(), G_FILE_TEST_EXISTS));
    CHECK(g_file_set_contents(replacementFile.c_str(), "[Type]\nClass=HCDoData\n\n[Property]\n+Data=Default data|3|\n\n[Methods]\ndoData=Input|1|0\nonEventData=Output|2|0\nData=Input data|4|0\n", -1, nullptr));
    root()->selMan->select(element(1));
    auto *wire = element(1)->points[1];
    const auto pathBefore = wire->serializePath();
    Element *unchanged = element(1);
    CHECK(command(CMD_REPLACETO, "hcConvertor")); CHECK(element(1) == unchanged);
    CHECK(command(CMD_REPLACETO, "QADoData"));
    CHECK(element(1)->tpl->name == "QADoData" && element(1)->id == 1);
    CHECK(element(1)->sysProps.getByName("Hint")->readStr() == hint);
    CHECK(element(1)->points[1]->point == element(2)->points[0]);
    CHECK(element(1)->points[1]->serializePath() == pathBefore);
    CHECK(command(CMD_UNDO)); CHECK(element(1)->tpl->name == "hcDoData");
    CHECK(command(CMD_REDO)); CHECK(element(1)->tpl->name == "QADoData");
    CHECK(command(CMD_UNDO));
    g_unlink(replacementFile.c_str());
    // Paste arrives asynchronously from the clipboard; it must be one history step.
    ustring fragment = "Add(hcDoData,99,0,0)" + ustring(1, char(10)) + "{" + ustring(1, char(10)) + "}";
    CHECK(root()->selMan->pasteText(fragment)); editor->sde->changeScheme(); drain();
    CHECK(root()->elements.size() == 3); CHECK(command(CMD_UNDO)); CHECK(root()->elements.size() == 2);
    CHECK(command(CMD_REDO)); CHECK(root()->elements.size() == 3);
    // Several mouse updates are coalesced at the completed operation.
    root()->selMan->select(element(1)); const int before = element(1)->x;
    editor->sde->beginOperation(MS_SCROLL);
    for(int i=0; i<4; ++i) { element(1)->move(7,0); editor->sde->changeScheme(); drain(); }
    editor->sde->endOperation();
    CHECK(command(CMD_UNDO)); CHECK(element(1)->x == before);
    CHECK(command(CMD_REDO)); CHECK(element(1)->x == before + 28);
    // Nested SDK navigation and selected IDs survive replacing the root model.
    Element *container = root()->add("hcMultiElement", 300, 300); CHECK(container && container->sdk);
    const int containerId = container->id;
    Element *child = container->sdk->add("hcDoData", 14, 21); CHECK(child); const int childId = child->id;
    editor->sde->enterSDK(container); editor->sde->csdk->selMan->select(child);
    editor->sde->changeScheme(); drain();
    CHECK(command(CMD_MOVETO, "70,84")); CHECK(command(CMD_UNDO));
    CHECK(editor->sde->csdk == element(containerId)->sdk);
    CHECK(element(childId)->x == 14 && editor->sde->csdk->selMan->getFirst() == element(childId));
    CHECK(command(CMD_REDO)); CHECK(element(childId)->x == 70);
    CHECK(command(CMD_BACK)); CHECK(editor->sde->csdk == root());
    CHECK(command(CMD_UNDO)); CHECK(editor->sde->csdk == element(containerId)->sdk);
    CHECK(element(childId)->x == 14);
    CHECK(command(CMD_BACK));
    // Groups are persisted in the v4-compatible @Group property and participate in history.
    root()->selMan->select(element(1)); root()->selMan->add(element(2));
    CHECK(command(CMD_GROUP));
    const int group = element(1)->groupId();
    CHECK(group > 0 && element(2)->groupId() == group);
    root()->selMan->select(element(1)); root()->selMan->add(element(1));
    CHECK(root()->selMan->elements.size() == 2);
    CHECK(command(CMD_UNDO)); CHECK(element(1)->groupId() == 0 && element(2)->groupId() == 0);
    CHECK(command(CMD_REDO)); CHECK(element(1)->groupId() == group && element(2)->groupId() == group);
    extern int elGetGroup(Element*); CHECK(elGetGroup(element(1)) == group);
    ustring grouped;
    for(Element *item : root()->selMan->elements) item->saveToText(grouped, "", ELEMENT_SAVE_SELECTED | ELEMENT_SAVE_CHANGED);
    const size_t count = root()->elements.size();
    CHECK(root()->selMan->pasteText(grouped)); editor->sde->changeScheme(); drain();
    CHECK(root()->elements.size() == count + 2 && root()->selMan->elements.size() == 2);
    const int pastedGroup = root()->selMan->elements.front()->groupId();
    CHECK(pastedGroup > 0 && pastedGroup != group);
    for(Element *item : root()->selMan->elements) CHECK(item->groupId() == pastedGroup);
    root()->selMan->select(element(1)); CHECK(root()->selMan->elements.size() == 2);
    CHECK(command(CMD_UNGROUP)); CHECK(element(1)->groupId() == 0 && element(2)->groupId() == 0);
    CHECK(command(CMD_UNDO)); CHECK(element(1)->groupId() == group);
    CHECK(command(CMD_SAVE));
    MSDK reload; CHECK(reload.loadFromFile(input));
    CHECK(reload.getGlobalElementById(1)->groupId() == group);
    g_unlink(input.c_str()); g_rmdir(dir.c_str());
    std::cout << "OK scheme history: move/save/delete/links/Unicode/freeze/paste/branch/mouse/nested SDK" << std::endl;
}
