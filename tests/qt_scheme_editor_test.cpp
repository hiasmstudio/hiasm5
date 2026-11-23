#include "main_window.h"
#include "scheme_editor.h"

#include <QApplication>
#include <QEnterEvent>
#include <QFile>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QLineEdit>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QDialog>
#include <QComboBox>
#include <QColorDialog>
#include <QClipboard>
#include <QKeyEvent>
#include <QMessageBox>
#include <QListWidget>
#include <QMouseEvent>
#include <QPushButton>
#include <QTemporaryDir>
#include <QTimer>
#include <QTextBlock>
#include <QTextLayout>
#include <QTranslator>
#include <QSettings>
#include <algorithm>
#include <cstdio>

#define CHECK(condition) do { \
    if (!(condition)) { \
        std::fprintf(stderr, "Line %d: %s\n", __LINE__, #condition); \
        return 1; \
    } \
} while (false)

namespace {
QGraphicsRectItem *element(QGraphicsScene *scene, int index) {
    for (auto *item : scene->items()) {
        auto *rect = qgraphicsitem_cast<QGraphicsRectItem *>(item);
        if (rect && rect->data(0).toInt() == index)
            return rect;
    }
    return nullptr;
}

QGraphicsPathItem *wire(QGraphicsScene *scene) {
    for (auto *item : scene->items()) {
        if (auto *path = qgraphicsitem_cast<QGraphicsPathItem *>(item))
            return path;
    }
    return nullptr;
}

void mouse(QWidget *viewport, QEvent::Type type, const QPoint &position, Qt::MouseButton button,
           Qt::MouseButtons buttons) {
    QMouseEvent event(type, position, viewport->mapToGlobal(position), button, buttons, Qt::NoModifier);
    QApplication::sendEvent(viewport, &event);
}
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    CHECK(argc == 2 || argc == 4);
    QTemporaryDir settings;
    CHECK(settings.isValid());
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, settings.path());
    QTranslator translator;
    if (argc == 4 && translator.load(app.applicationDirPath() + "/src/qt/hiasm_ru.qm"))
        app.installTranslator(&translator);
    HiasmQtMainWindow window;
    window.show();
    auto *tabs = window.findChild<QTabWidget *>("tabs");
    auto *properties = window.findChild<QTreeWidget *>("properties");
    CHECK(tabs && properties && !properties->alternatingRowColors());

    CHECK(window.openScheme(app.arguments().at(1)));
    auto *exampleCanvas = tabs->currentWidget()->findChild<QGraphicsView *>("canvas");
    CHECK(exampleCanvas && element(exampleCanvas->scene(), 2) && wire(exampleCanvas->scene()));

    const QString scheme = QStringLiteral(
        "Make(cpp)\n* Схема\nAdd(MainForm,1,24,30)\n{\n"
        " Caption=Привет\u00a0мир {=)}\n link(onCreate,2:doWork,[(150,36)(150,105)])\n}\n"
        "Add(MultiElement,2, 196 , 100)\n{\n}\nBEGIN_SDK\n"
        "  Add(EditMulti,3,-12,8)\n  {\n  }\nEND_SDK\n");
    QTemporaryDir folder;
    CHECK(folder.isValid());
    const auto fileName = folder.filePath("editor.sha");
    QFile file(fileName);
    const QByteArray original = QByteArray("\xef\xbb\xbf") + scheme.toUtf8().replace("\n", "\r\n");
    CHECK(file.open(QIODevice::WriteOnly) && file.write(original) == original.size());
    file.close();
    CHECK(window.openScheme(fileName));
    auto *editor = qobject_cast<SchemeEditor *>(tabs->currentWidget());
    CHECK(editor && editor->source() == scheme);
    auto *canvas = editor->findChild<QGraphicsView *>("canvas");
    auto *views = editor->findChild<QTabWidget *>("editorViews");
    const auto root = [editor] { return editor->findChild<QToolButton *>("schemeLevel0"); };
    auto *source = editor->findChild<QPlainTextEdit *>("sourceEditor");
    auto *error = editor->findChild<QLabel *>("diagramError");
    CHECK(canvas && views && !root() && source && error);
    app.processEvents();
    auto *scene = canvas->scene();
    auto *form = element(scene, 0);
    CHECK(form && element(scene, 1) && !element(scene, 2) && wire(scene));
    const auto originalWire = wire(scene)->path();
    CHECK(originalWire.elementCount() == 4 && originalWire.elementAt(1).x == 150);
    QGraphicsItem *occupied = nullptr;
    for (auto *child : form->childItems())
        if (child->data(2).toString() == "onCreate") occupied = child;
    CHECK(occupied);
    const auto contact = canvas->mapFromScene(occupied->scenePos());
    const auto blankPoint = canvas->mapFromScene(QPointF(140, 145));
    CHECK(!canvas->itemAt(blankPoint));
    mouse(canvas->viewport(), QEvent::MouseButtonPress, contact, Qt::LeftButton, Qt::LeftButton);
    mouse(canvas->viewport(), QEvent::MouseMove, blankPoint, Qt::NoButton, Qt::LeftButton);
    mouse(canvas->viewport(), QEvent::MouseButtonRelease, blankPoint, Qt::LeftButton, Qt::NoButton);
    CHECK(!wire(scene) && !editor->source().contains("link("));
    editor->document()->undo();
    CHECK(editor->source() == scheme && wire(scene) && wire(scene)->path() == originalWire);
    form = element(scene, 0);

    const auto from = canvas->mapFromScene(form->sceneBoundingRect().center());
    const QPoint delta(42, 51);
    mouse(canvas->viewport(), QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
    mouse(canvas->viewport(), QEvent::MouseMove, from + delta, Qt::NoButton, Qt::LeftButton);
    mouse(canvas->viewport(), QEvent::MouseButtonRelease, from + delta, Qt::LeftButton, Qt::NoButton);
    app.processEvents();
    QString moved = scheme;
    moved.replace("Add(MainForm,1,24,30)", "Add(MainForm,1,66,81)");
    CHECK(editor->source() == moved && editor->document()->isModified());
    CHECK(element(scene, 0)->pos() == QPointF(66, 81));
    CHECK(wire(scene)->path() != originalWire && wire(scene)->path().elementAt(1).x == 150);
    const auto captions = properties->findItems("Caption", Qt::MatchExactly | Qt::MatchRecursive);
    CHECK(captions.size() == 1);
    auto *caption = qobject_cast<QLineEdit *>(properties->itemWidget(captions.first(), 1));
    CHECK(caption);
    caption->setText(QStringLiteral("Новая подпись"));
    caption->setModified(true);
    caption->editingFinished();
    app.processEvents();
    CHECK(editor->source().contains(QStringLiteral(" Caption=\"Новая подпись\"\n link(")));
    editor->document()->undo();
    CHECK(editor->source() == moved);

    const auto modal = properties->findItems("Modal", Qt::MatchExactly | Qt::MatchRecursive);
    CHECK(modal.size() == 1);
    auto *modalEdit = qobject_cast<QComboBox *>(properties->itemWidget(modal.first(), 1));
    CHECK(modalEdit && modalEdit->currentText() == "False" && modalEdit->itemText(0) == "True");
    modalEdit->setCurrentIndex(0);
    modalEdit->activated(0);
    app.processEvents();
    CHECK(editor->source().contains(" Modal=0\n"));
    editor->document()->undo();
    const auto borders = properties->findItems("BorderStyle", Qt::MatchExactly | Qt::MatchRecursive);
    CHECK(borders.size() == 1);
    auto *borderEdit = qobject_cast<QComboBox *>(properties->itemWidget(borders.first(), 1));
    CHECK(borderEdit && borderEdit->currentIndex() == 2);
    borderEdit->setCurrentIndex(0);
    borderEdit->activated(0);
    app.processEvents();
    CHECK(editor->source().contains(" BorderStyle=0\n"));
    editor->document()->undo();
    CHECK(editor->source() == moved);

    editor->document()->undo();
    CHECK(editor->source() == scheme && element(scene, 0)->pos() == QPointF(24, 30));
    editor->document()->redo();
    CHECK(editor->source() == moved && element(scene, 0)->pos() == QPointF(66, 81));
    CHECK(window.saveCurrent());
    CHECK(file.open(QIODevice::ReadOnly));
    CHECK(file.readAll() == QByteArray("\xef\xbb\xbf") + moved.toUtf8().replace("\n", "\r\n"));
    file.close();

    const auto nested = canvas->mapFromScene(element(scene, 1)->sceneBoundingRect().center());
    mouse(canvas->viewport(), QEvent::MouseButtonDblClick, nested, Qt::LeftButton, Qt::LeftButton);
    app.processEvents();
    CHECK(root() && root()->isVisible() && element(scene, 0)->pos() == QPointF(-12, 8));
    CHECK(!element(scene, 1) && !wire(scene));
    auto *palette = window.findChild<QScrollArea *>("palette");
    CHECK(palette);
    QListWidgetItem *button = nullptr;
    for (auto *group : palette->findChildren<QListWidget *>()) {
        const auto matches = group->findItems("Button", Qt::MatchExactly);
        if (!matches.isEmpty()) button = matches.first();
    }
    CHECK(button && !button->icon().isNull());
    button->listWidget()->itemDoubleClicked(button);
    CHECK(element(scene, 1) && editor->source().contains("Add(Button,4,"));
    CHECK(editor->changeSelectedProperty(4, "37"));
    editor->document()->undo();
    CHECK(editor->source().indexOf("Add(Button,4,") < editor->source().indexOf("END_SDK"));
    editor->document()->undo();
    CHECK(editor->source() == moved && !element(scene, 1));

    auto *viewport = canvas->viewport();
    auto *zoom = editor->findChild<QSlider *>("zoomSlider");
    CHECK(zoom);
    zoom->setValue(145);
    app.processEvents();
    const QPoint destination(160, 105);
    const auto expectedPosition = canvas->mapToScene(destination).toPoint();
    button->listWidget()->itemClicked(button);
    mouse(viewport, QEvent::MouseMove, destination, Qt::NoButton, Qt::NoButton);
    CHECK(editor->source() == moved && !viewport->cursor().pixmap().isNull() &&
          viewport->cursor().hotSpot() == QPoint(3, 3));
    mouse(viewport, QEvent::MouseButtonPress, destination, Qt::LeftButton, Qt::LeftButton);
    mouse(viewport, QEvent::MouseButtonRelease, destination, Qt::LeftButton, Qt::NoButton);
    CHECK(element(scene, 1) && element(scene, 1)->pos() == expectedPosition && viewport->cursor().pixmap().isNull());
    editor->document()->undo();
    CHECK(editor->source() == moved && !element(scene, 1));
    button->listWidget()->itemClicked(button);
    QKeyEvent escape(QEvent::KeyPress, Qt::Key_Escape, Qt::NoModifier);
    QApplication::sendEvent(canvas, &escape);
    CHECK(viewport->cursor().pixmap().isNull() && editor->source() == moved);
    zoom->setValue(100);
    root()->click();
    CHECK(!root() && element(scene, 1) && wire(scene));

    CHECK(editor->connectPorts(1, "onClose", 1, "doClose"));
    CHECK(!editor->connectPorts(1, "onClose", 1, "doClose"));
    CHECK(editor->source().contains("link(onClose,1:doClose,[])"));
    editor->document()->undo();
    element(scene,1)->setSelected(true);
    editor->deleteSelection();
    CHECK(!editor->source().contains("BEGIN_SDK") && !editor->source().contains("link("));
    editor->document()->undo();
    CHECK(editor->source() == moved);

    views->setCurrentIndex(1);
    source->setPlainText("Make(cpp)\nAdd(Bad,7,10)\n{\n}\n");
    views->setCurrentIndex(0);
    CHECK(!error->isHidden() && !element(scene, 0));
    views->setCurrentIndex(1);
    source->setPlainText(moved);
    source->document()->setModified(true);
    views->setCurrentIndex(0);
    CHECK(error->isHidden() && element(scene, 0) && wire(scene));

    // The document still needs confirmation when closing a background tab or the main window.
    const int editorIndex = tabs->indexOf(editor);
    tabs->setCurrentIndex(0);
    bool asked = false;
    auto cancel = [&] {
        if (auto *box = qobject_cast<QMessageBox *>(QApplication::activeModalWidget())) {
            asked = box->text().contains("editor.sha");
            box->button(QMessageBox::Cancel)->click();
        }
    };
    QTimer::singleShot(0, &window, cancel);
    tabs->tabCloseRequested(editorIndex);
    CHECK(asked && tabs->indexOf(editor) == editorIndex);
    asked = false;
    QTimer::singleShot(0, &window, cancel);
    CHECK(!window.close() && asked);

    // Reproduce the ChildPanel workflow using the same palette and mouse gestures as the IDE.
    HiasmQtMainWindow nestedWindow;
    nestedWindow.findChild<QAction *>("theme_dark")->trigger();
    nestedWindow.show();
    app.processEvents();
    auto *childEditor = qobject_cast<SchemeEditor *>(nestedWindow.findChild<QTabWidget *>("tabs")->currentWidget());
    auto *view = childEditor->findChild<QGraphicsView *>("canvas");
    auto *childScene = view->scene();
    auto addFromPalette = [&](const QString &name) {
        const auto groups = nestedWindow.findChild<QScrollArea *>("palette")->findChildren<QListWidget *>();
        for (auto *group : groups) {
            const auto items = group->findItems(name, Qt::MatchExactly);
            if (!items.isEmpty()) { group->itemDoubleClicked(items.first()); return true; }
        }
        return false;
    };
    auto drag = [&](QPointF start, QPointF end) {
        const auto a = view->mapFromScene(start), b = view->mapFromScene(end);
        mouse(view->viewport(), QEvent::MouseButtonPress, a, Qt::LeftButton, Qt::LeftButton);
        mouse(view->viewport(), QEvent::MouseMove, b, Qt::NoButton, Qt::LeftButton);
        mouse(view->viewport(), QEvent::MouseButtonRelease, b, Qt::LeftButton, Qt::NoButton);
        app.processEvents();
    };
    auto port = [&](int node, const QString &name) -> QGraphicsItem * {
        if (auto *owner = element(childScene, node))
            for (auto *item : owner->childItems())
                if (item->data(2).toString() == name) return item;
        return nullptr;
    };
    CHECK(addFromPalette("Edit") && addFromPalette("ChildPanel"));
    auto *owner = element(childScene, 2);
    CHECK(owner && childEditor->source().contains("Add(EditMulti,3,14,14)") &&
          childEditor->source().contains("Add(Panel,4,56,56)"));
    const auto rootSource = childEditor->source();
    childEditor->document()->undo();
    CHECK(!childEditor->source().contains("BEGIN_SDK"));
    childEditor->document()->redo();
    CHECK(rootSource == childEditor->source());
    auto enter = [&] {
        auto *node = element(childScene, 2);
        mouse(view->viewport(), QEvent::MouseButtonDblClick, view->mapFromScene(node->mapToScene(node->rect().center())),
              Qt::LeftButton, Qt::LeftButton);
    };
    enter();
    const auto currentLevel = [childEditor] {
        return childEditor->findChild<QLabel *>("currentSchemeLevel")->text();
    };
    CHECK(currentLevel() == "ChildPanel");
    auto *frame = element(childScene, 0);
    CHECK(frame && frame->rect().size() == QSize(300,200));
    CHECK(!childScene->itemAt(QPointF(110,110), QTransform()));
    frame->setSelected(true);
    childEditor->deleteSelection();
    CHECK(rootSource == childEditor->source());
    drag(frame->mapToScene(frame->rect().bottomRight()), QPointF(378,258));
    CHECK(element(childScene,0)->rect().size() == QSize(364,244));
    childEditor->document()->undo();
    CHECK(element(childScene,0)->rect().size() == QSize(300,200));
    childEditor->document()->redo();
    CHECK(addFromPalette("Label") && addFromPalette("Button"));
    CHECK(port(2,"onClick") && port(2,"Caption") && port(3,"onClick"));
    const auto buttonCenter = element(childScene,3)->sceneBoundingRect().center();
    drag(buttonCenter, buttonCenter + QPointF(28,42));
    const auto beforeLinks = childEditor->source();
    drag(port(2,"onClick")->scenePos(), QPointF(378,85));
    CHECK(port(0,"onEvent1") && port(0,"onEvent1")->pos() == QPointF(364,6));
    CHECK(childEditor->source().contains("link(onClick,3:onEvent1,[])"));
    childEditor->document()->undo();
    CHECK(childEditor->source() == beforeLinks);
    childEditor->document()->redo();
    drag(port(3,"onClick")->scenePos(), QPointF(378,95));
    drag(port(2,"Caption")->scenePos(), QPointF(110,258));
    CHECK(port(0,"onEvent2") && port(0,"Var1") && port(0,"Var1")->pos() == QPointF(6,244));
    CHECK(childEditor->source().contains("link(Var1,5:Caption,[])"));
    const auto connected = childEditor->source();
    const QPointF blank(323,215);
    CHECK(!childScene->itemAt(blank, QTransform()));
    auto *labelClick = port(2,"onClick");
    drag(labelClick->scenePos(), labelClick->scenePos());
    CHECK(childEditor->source() == connected);
    drag(labelClick->scenePos(), blank);
    CHECK(!childEditor->source().contains("link(onClick,3:onEvent1,[])") &&
          childEditor->source().contains("link(onClick,3:onEvent2,[])"));
    childEditor->document()->undo();
    CHECK(childEditor->source() == connected);
    CHECK(port(1,"doCaption"));
    drag(port(0,"onEvent1")->scenePos(), port(1,"doCaption")->scenePos());
    CHECK(childEditor->source().contains("link(onClick,4:doCaption,[])"));
    childEditor->document()->undo();
    CHECK(childEditor->source() == connected);
    drag(port(2,"onClick")->scenePos(), port(1,"onClick")->scenePos());
    const auto reconnect = readSchemeDiagram(childEditor->source());
    CHECK(reconnect.layers.at(1).connections.size() == 3);
    const auto &rewired = reconnect.layers.at(1);
    CHECK(std::any_of(rewired.connections.cbegin(), rewired.connections.cend(), [&](const auto &link) {
        return rewired.elements.at(link.source).id == 4 && link.sourcePoint == "onClick" && link.targetPoint == "onEvent1";
    }));
    childEditor->document()->undo();
    CHECK(childEditor->source() == connected);
    drag(port(2,"onClick")->scenePos(), port(2,"Caption")->scenePos());
    CHECK(childEditor->source() == connected);

    QTimer::singleShot(0, &nestedWindow, [] {
        if (auto *menu = qobject_cast<QMenu *>(QApplication::activePopupWidget())) {
            const auto remove = QCoreApplication::translate("SchemeEditor", "Remove connection");
            for (auto *action : menu->actions())
                if (action->text() == remove) { action->trigger(); break; }
            menu->close();
        }
    });
    view->customContextMenuRequested(view->mapFromScene(port(2,"Caption")->scenePos()));
    CHECK(!childEditor->source().contains("link(Var1,5:Caption,[])"));
    childEditor->document()->undo();
    CHECK(childEditor->source() == connected);
    if (argc == 4) CHECK(nestedWindow.grab().save(app.arguments().at(2)));
    childEditor->findChild<QToolButton *>("schemeLevel0")->click();
    CHECK(currentLevel() == "ROOT" && element(childScene,2)->isSelected());
    CHECK(port(2,"onEvent1") && port(2,"onEvent1")->pos() == QPointF(32,6));
    CHECK(port(2,"Var1") && port(2,"Var1")->pos() == QPointF(6,32));
    CHECK(childEditor->connectPorts(2, "onEvent2", 1, "doText"));
    CHECK(childEditor->connectPorts(1, "Str", 2, "Var1"));
    if (argc == 4) CHECK(nestedWindow.grab().save(app.arguments().at(3)));
    const auto wired = childEditor->source();
    enter();
    element(childScene,0)->setSelected(true);
    const auto rows = childEditor->selectedProperties();
    int eventRow = -1;
    for (int i = 0; i < rows.size(); ++i) if (rows[i].first == "EventCount") eventRow = i;
    CHECK(eventRow >= 0 && childEditor->changeSelectedProperty(eventRow, "1"));
    CHECK(!port(0,"onEvent2") && !childEditor->source().contains("onEvent2") && port(0,"onEvent1"));
    childEditor->document()->undo();
    CHECK(childEditor->source() == wired && port(0,"onEvent2"));

    CHECK(addFromPalette("ChildPanel"));
    auto *childViews = childEditor->findChild<QTabWidget *>("editorViews");
    auto *childSource = childEditor->findChild<QPlainTextEdit *>("sourceEditor");
    auto *chooseChild = childEditor->findChild<QToolButton *>("childLevels");
    CHECK(childViews && childSource && chooseChild);
    childViews->setCurrentWidget(childEditor->findChild<QWidget *>("sourcePage"));
    const auto withNestedPanel = childEditor->source();
    const auto outerSdkOffset = withNestedPanel.indexOf("BEGIN_SDK");
    const auto innerSdkOffset = withNestedPanel.indexOf("BEGIN_SDK", outerSdkOffset + 1);
    CHECK(childSource->textCursor().position() == outerSdkOffset && innerSdkOffset > outerSdkOffset &&
          childEditor->findChild<QWidget *>("schemePath")->isVisible() &&
          childEditor->findChild<QSlider *>("zoomSlider")->isHidden());
    const auto enterNestedPanel = [&] {
        chooseChild->menu()->aboutToShow();
        auto *action = chooseChild->menu()->actions().first();
        if (!action->text().startsWith("ChildPanel ") || !action->isEnabled()) return false;
        action->trigger();
        return childSource->textCursor().position() == innerSdkOffset;
    };
    CHECK(enterNestedPanel());
    app.processEvents();
    auto *middle = childEditor->findChild<QToolButton *>("schemeLevel1");
    CHECK(middle && middle->isVisible() && middle->text() == "ChildPanel");
    QEnterEvent hover(QPointF(1,1), QPointF(1,1), middle->mapToGlobal(QPoint(1,1)));
    QApplication::sendEvent(middle, &hover);
    CHECK(middle->font().underline());
    QEvent leave(QEvent::Leave);
    QApplication::sendEvent(middle, &leave);
    CHECK(!middle->font().underline());
    middle->click();
    CHECK(childSource->textCursor().position() == outerSdkOffset && !childEditor->findChild<QToolButton *>("schemeLevel1"));
    CHECK(enterNestedPanel());
    childEditor->findChild<QToolButton *>("schemeLevel0")->click();
    CHECK(currentLevel() == "ROOT" && childSource->textCursor().position() == 0 &&
          childViews->currentWidget() == childSource->parentWidget() && childEditor->source() == withNestedPanel);
    childViews->setCurrentWidget(childEditor->findChild<QWidget *>("diagramPage"));
    enter();
    childEditor->document()->undo();
    CHECK(childEditor->source() == wired && currentLevel() == "ChildPanel");

    const auto nestedFile = folder.filePath("nested.sha");
    QFile saved(nestedFile);
    CHECK(saved.open(QIODevice::WriteOnly));
    CHECK(saved.write(childEditor->encodedSource()) == childEditor->encodedSource().size());
    saved.close();
    CHECK(nestedWindow.openScheme(nestedFile));
    auto *reopened = qobject_cast<SchemeEditor *>(nestedWindow.findChild<QTabWidget *>("tabs")->currentWidget());
    CHECK(reopened->source() == childEditor->source() && !readSchemeDiagram(reopened->source()).errorLine);
    const auto legacyFile = folder.filePath("legacy.sha");
    QFile legacy(legacyFile);
    const QByteArray cp1251 = "* " + QByteArray::fromHex("cff0e8e2e5f2") + "\r\n" + wired.toUtf8().replace("\n", "\r\n");
    CHECK(legacy.open(QIODevice::WriteOnly) && legacy.write(cp1251) == cp1251.size());
    legacy.close();
    CHECK(nestedWindow.openScheme(legacyFile));
    auto *oldEditor = qobject_cast<SchemeEditor *>(nestedWindow.findChild<QTabWidget *>("tabs")->currentWidget());
    CHECK(oldEditor->source().startsWith(QStringLiteral("* Привет\n")) && nestedWindow.saveCurrent());
    CHECK(legacy.open(QIODevice::ReadOnly) && legacy.readAll() == cp1251);
    legacy.close();
    QTextCursor comment(oldEditor->document());
    comment.setPosition(2);
    comment.insertText(QStringLiteral("Ω"));
    CHECK(nestedWindow.saveCurrent() && legacy.open(QIODevice::ReadOnly));
    const auto upgraded = legacy.readAll();
    CHECK(upgraded.startsWith("\xef\xbb\xbf* ") && upgraded.isValidUtf8() && upgraded.contains("\r\n"));

    SchemeEditor pages(QStringLiteral("Make(cpp)\nAdd(MainForm,1,0,0)\n{\n Backend=1\n}\n"), {}, {});
    CHECK(pages.insertElement(QStringLiteral("PageControl")));
    const auto pageDiagram = readSchemeDiagram(pages.source());
    CHECK(!pageDiagram.errorLine && pageDiagram.layers.size() == 3);
    const auto &pageControl = pageDiagram.layers.first().elements.last();
    CHECK(pageControl.childLayers == QList<int>({1, 2}));
    CHECK(pageDiagram.layers[1].elements.size() == 1 && pageDiagram.layers[2].elements.last().name == "PagePanel");
    auto *pageMenu = pages.findChild<QToolButton *>("childLevels")->menu();
    pageMenu->aboutToShow();
    auto *classes = pageMenu->actions().first()->menu();
    CHECK(classes && classes->actions().size() == 2);
    pages.findChild<QTabWidget *>("editorViews")->setCurrentWidget(pages.findChild<QWidget *>("sourcePage"));
    classes->actions().first()->trigger();
    auto *pageSource = pages.findChild<QPlainTextEdit *>("sourceEditor");
    CHECK(pageSource->textCursor().position() == pageDiagram.layers[1].startOffset);
    pages.findChild<QToolButton *>("schemeLevel0")->click();
    pageMenu->aboutToShow();
    classes = pageMenu->actions().first()->menu();
    CHECK(classes && classes->actions().last()->text() == "Page");
    classes->actions().last()->trigger();
    CHECK(pageSource->textCursor().position() == pageDiagram.layers[2].startOffset);

    QFile dataScheme(folder.filePath("typed-data.sha"));
    CHECK(dataScheme.open(QIODevice::WriteOnly));
    CHECK(dataScheme.write("Make(cpp)\nAdd(MainForm,1,0,0)\n{\n}\nAdd(Button,2,120,55)\n{\n Data=String(original)\n Color=clBtnFace\n Hint=\"Existing hint\"\n}\n"
                           "Add(Memo,3,210,100)\n{\n Left=12\n Top=80\n Strings=#5:First|0:|6:Second|\n}\n") > 0);
    dataScheme.close();
    CHECK(window.openScheme(dataScheme.fileName()));
    auto *dataEditor = qobject_cast<SchemeEditor *>(tabs->currentWidget());
    auto *dataCanvas = dataEditor->findChild<QGraphicsView *>("canvas");
    auto *buttonItem = element(dataCanvas->scene(), 1);
    CHECK(buttonItem);
    buttonItem->setSelected(true);
    const auto dataRows = properties->findItems("Data", Qt::MatchExactly | Qt::MatchRecursive);
    CHECK(dataRows.size() == 1 && dataRows.first()->font(0).bold());
    auto *dataCell = properties->itemWidget(dataRows.first(), 1);
    CHECK(dataCell);
    auto *openData = dataCell->findChild<QToolButton *>("editDataProperty");
    CHECK(openData);
    bool validated = false;
    QTimer::singleShot(0, &window, [&] {
        auto *dialog = window.findChild<QDialog *>("dataEditor");
        if (!dialog) return;
        auto *value = dialog->findChild<QLineEdit *>("dataValue");
        auto *types = dialog->findChild<QButtonGroup *>();
        const bool preserved = value->text() == "original" && types->checkedId() == 2;
        types->button(1)->click();
        auto *ok = dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok);
        const bool rejectsText = !ok->isEnabled();
        value->setText("42");
        validated = preserved && rejectsText && ok->isEnabled();
        if (validated) ok->click();
        else dialog->reject();
    });
    openData->click();
    CHECK(validated && dataEditor->source().contains(" Data=Integer(42)\n"));
    bool dataDialogReopened = false;
    QTimer::singleShot(0, &window, [&] {
        if (auto *dialog = window.findChild<QDialog *>("dataEditor")) {
            dataDialogReopened = dialog->findChild<QButtonGroup *>()->checkedId() == 1;
            dialog->reject();
        }
    });
    buttonItem = element(dataCanvas->scene(), 1);
    CHECK(buttonItem);
    mouse(dataCanvas->viewport(), QEvent::MouseButtonDblClick,
          dataCanvas->mapFromScene(buttonItem->sceneBoundingRect().center()), Qt::LeftButton, Qt::LeftButton);
    CHECK(dataDialogReopened && dataEditor->source().contains(" Data=Integer(42)\n"));

    dataEditor->setFormView(true);
    app.processEvents();
    auto *designer = dataEditor->findChild<QWidget *>("formDesigner");
    CHECK(designer);
    auto *buttonPreview = designer->findChild<QWidget *>("control_2");
    CHECK(buttonPreview);
    bool openedOnForm = false;
    QTimer::singleShot(0, &window, [&] {
        if (auto *dialog = window.findChild<QDialog *>("dataEditor")) {
            openedOnForm = dialog->findChild<QLineEdit *>("dataValue")->text() == "42";
            dialog->findChild<QButtonGroup *>()->button(0)->click();
            dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
        }
    });
    mouse(designer, QEvent::MouseButtonDblClick, buttonPreview->mapTo(designer, buttonPreview->rect().center()),
          Qt::LeftButton, Qt::LeftButton);
    CHECK(openedOnForm && dataEditor->source().contains(" Data=\n"));
    dataEditor->document()->undo();
    CHECK(dataEditor->source().contains(" Data=Integer(42)\n"));

    const auto colorButton = [&]() -> QToolButton * {
        const auto rows = properties->findItems("Color", Qt::MatchExactly | Qt::MatchRecursive);
        if (rows.size() != 1) return nullptr;
        auto *field = properties->itemWidget(rows.first(), 1);
        if (!field) return nullptr;
        for (auto *button : field->findChildren<QToolButton *>())
            if (button->defaultAction() && button->defaultAction()->objectName() == "choosePropertyColor") return button;
        return nullptr;
    };
    auto *swatch = colorButton();
    CHECK(swatch);
    const auto beforeColor = dataEditor->source();
    bool loadedColor = false;
    QTimer::singleShot(0, &window, [&] {
        if (auto *dialog = window.findChild<QColorDialog *>()) {
            loadedColor = dialog->currentColor() == swatch->palette().color(QPalette::Button);
            dialog->setCurrentColor(QColor("#123456"));
            dialog->accept();
        }
    });
    swatch->click();
    app.processEvents();
    CHECK(loadedColor);
    CHECK(dataEditor->source().contains(" Color=#123456\n"));
    const auto afterColor = dataEditor->source();
    swatch = colorButton();
    CHECK(swatch);
    QTimer::singleShot(0, &window, [&] {
        if (auto *dialog = window.findChild<QColorDialog *>()) {
            dialog->setCurrentColor(Qt::red);
            dialog->reject();
        }
    });
    swatch->click();
    app.processEvents();
    CHECK(dataEditor->source() == afterColor);
    dataEditor->document()->undo();
    CHECK(dataEditor->source() == beforeColor);

    dataEditor->setFormView(false);
    auto *memoItem = element(dataCanvas->scene(), 2);
    CHECK(memoItem);
    bool stringsOpened = false;
    QTimer::singleShot(0, &window, [&] {
        auto *dialog = window.findChild<QDialog *>("stringsEditor");
        if (!dialog) return;
        auto *text = dialog->findChild<QPlainTextEdit *>("stringsValue");
        auto *syntax = dialog->findChild<QComboBox *>("stringsSyntax");
        stringsOpened = text && syntax && text->toPlainText() == "First\n\nSecond" && syntax->findText("Delphi") >= 0;
        if (!stringsOpened) { dialog->reject(); return; }
        text->setPlainText("begin");
        syntax->setCurrentIndex(syntax->findText("Delphi"));
        stringsOpened = !text->document()->firstBlock().layout()->formats().isEmpty();
        text->setPlainText(QStringLiteral("\u0421\u0442\u0440\u043E\u043A\u0430 | \U0001F600\n\n"));
        dialog->findChild<QDialogButtonBox *>()->button(QDialogButtonBox::Ok)->click();
    });
    mouse(dataCanvas->viewport(), QEvent::MouseButtonDblClick,
          dataCanvas->mapFromScene(memoItem->sceneBoundingRect().center()), Qt::LeftButton, Qt::LeftButton);
    const auto editedStrings = dataEditor->source();
    CHECK(stringsOpened && editedStrings.contains(QStringLiteral(" Strings=#10:\u0421\u0442\u0440\u043E\u043A\u0430 | \U0001F600|0:|0:|\n")));

    const auto stringRows = properties->findItems("Strings", Qt::MatchExactly | Qt::MatchRecursive);
    CHECK(stringRows.size() == 1 && stringRows.first()->font(0).bold());
    auto *stringsCell = properties->itemWidget(stringRows.first(), 1);
    CHECK(stringsCell);
    auto *openStrings = stringsCell->findChild<QToolButton *>("editStringsProperty");
    CHECK(openStrings && openStrings->width() == openStrings->height());
    bool stringsReopened = false;
    QTimer::singleShot(0, &window, [&] {
        if (auto *dialog = window.findChild<QDialog *>("stringsEditor")) {
            auto *text = dialog->findChild<QPlainTextEdit *>("stringsValue");
            stringsReopened = text && text->toPlainText() == QStringLiteral("\u0421\u0442\u0440\u043E\u043A\u0430 | \U0001F600\n\n");
            if (text) text->setPlainText("discarded");
            dialog->reject();
        }
    });
    openStrings->click();
    CHECK(stringsReopened && dataEditor->source() == editedStrings);
    dataEditor->document()->undo();
    CHECK(dataEditor->source() == beforeColor);

    dataCanvas->scene()->clearSelection();
    element(dataCanvas->scene(), 1)->setSelected(true);
    const auto hintEditor = [&]() -> QLineEdit * {
        const auto rows = properties->findItems("Hint", Qt::MatchExactly | Qt::MatchRecursive);
        return rows.size() == 1 ? qobject_cast<QLineEdit *>(properties->itemWidget(rows.first(), 1)) : nullptr;
    };
    auto *hint = hintEditor();
    CHECK(hint && !hint->isReadOnly() && hint->text() == "Existing hint");
    auto *hintAction = hint->findChild<QAction *>("editTextProperty");
    CHECK(hintAction);
    bool hintOpened = false;
    QTimer::singleShot(0, &window, [&] {
        if (auto *dialog = window.findChild<QDialog *>("stringsEditor")) {
            auto *text = dialog->findChild<QPlainTextEdit *>("stringsValue");
            hintOpened = text && text->toPlainText() == "Existing hint";
            if (text) text->setPlainText("First\nSecond");
            dialog->accept();
        }
    });
    hintAction->trigger();
    CHECK(hintOpened && dataEditor->source().contains(" Hint=#5:First|6:Second|\n"));
    hint = hintEditor();
    CHECK(hint && hint->isReadOnly() && hint->text() == "First" && hint->toolTip() == "First\nSecond");
    hintAction = hint->findChild<QAction *>("editTextProperty");
    CHECK(hintAction);
    bool hintReopened = false;
    QTimer::singleShot(0, &window, [&] {
        if (auto *dialog = window.findChild<QDialog *>("stringsEditor")) {
            auto *text = dialog->findChild<QPlainTextEdit *>("stringsValue");
            hintReopened = text && text->toPlainText() == "First\nSecond";
            dialog->reject();
        }
    });
    hintAction->trigger();
    CHECK(hintReopened && dataEditor->source().contains(" Hint=#5:First|6:Second|\n"));
    dataEditor->document()->undo();
    CHECK(dataEditor->source() == beforeColor);

    const auto imageButton = [&](const QString &name) -> QToolButton * {
        const auto rows = properties->findItems(name, Qt::MatchExactly | Qt::MatchRecursive);
        if (rows.size() != 1) return nullptr;
        auto *cell = properties->itemWidget(rows.first(), 1);
        return cell ? cell->findChild<QToolButton *>("editImageProperty") : nullptr;
    };
    dataCanvas->scene()->clearSelection();
    element(dataCanvas->scene(), 0)->setSelected(true);
    auto *iconButton = imageButton("Icon");
    CHECK(iconButton);
    QImage sample(8, 5, QImage::Format_ARGB32);
    sample.fill(QColor(20, 80, 160, 200));
    QApplication::clipboard()->setImage(sample);
    bool iconLoaded = false;
    QTimer::singleShot(0, &window, [&] {
        if (auto *dialog = window.findChild<QDialog *>("iconEditor")) {
            dialog->findChild<QToolButton *>("imagePaste")->click();
            iconLoaded = !dialog->findChild<QLabel *>("imagePreview")->pixmap().isNull();
            dialog->accept();
        }
    });
    iconButton->click();
    CHECK(iconLoaded && dataEditor->source().contains(" Icon=iVBOR"));
    const auto withIcon = dataEditor->source();
    auto *pictureButton = imageButton("Picture");
    CHECK(pictureButton);
    bool pictureEdited = false;
    QTimer::singleShot(0, &window, [&] {
        if (auto *dialog = window.findChild<QDialog *>("pictureEditor")) {
            dialog->findChild<QToolButton *>("imagePaste")->click();
            dialog->findChild<QToolButton *>("imageRotate")->click();
            const auto preview = dialog->findChild<QLabel *>("imagePreview")->pixmap();
            pictureEdited = !preview.isNull() && preview.height() > preview.width();
            dialog->accept();
        }
    });
    pictureButton->click();
    CHECK(pictureEdited && dataEditor->source().contains(" Picture=iVBOR"));
    pictureButton = imageButton("Picture");
    CHECK(pictureButton);
    bool pictureRestored = false;
    const auto withPicture = dataEditor->source();
    QTimer::singleShot(0, &window, [&] {
        if (auto *dialog = window.findChild<QDialog *>("pictureEditor")) {
            pictureRestored = !dialog->findChild<QLabel *>("imagePreview")->pixmap().isNull();
            dialog->findChild<QToolButton *>("imageClear")->click();
            dialog->reject();
        }
    });
    pictureButton->click();
    CHECK(pictureRestored && dataEditor->source() == withPicture);
    dataEditor->document()->undo();
    CHECK(dataEditor->source() == withIcon);
}
