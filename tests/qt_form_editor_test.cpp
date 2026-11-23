#include "main_window.h"
#include "scheme_editor.h"
#include "form_canvas.h"
#include <QApplication>
#include <QDragEnterEvent>
#include <QGraphicsView>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QGroupBox>
#include <QTableWidget>
#include <QLineEdit>
#include <QKeyEvent>
#include <QMenu>
#include <QMouseEvent>
#include <QMimeData>
#include <QSettings>
#include <QPushButton>
#include <QToolBar>
#include <QFile>
#include <QListWidget>
#include <QTemporaryDir>
#include <QTextBlock>
#include <QTimer>
#include <QTranslator>
#include <cstdio>
#include <memory>

#define CHECK(c) do { if (!(c)) { std::fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (false)

void drag(QWidget *widget, QPoint from, QPoint to) {
    auto send = [&](QEvent::Type type, QPoint p, Qt::MouseButton button, Qt::MouseButtons buttons) {
        QMouseEvent event(type, p, widget->mapToGlobal(p), button, buttons, Qt::NoModifier);
        QApplication::sendEvent(widget, &event);
    };
    send(QEvent::MouseButtonPress, from, Qt::LeftButton, Qt::LeftButton);
    send(QEvent::MouseMove, to, Qt::NoButton, Qt::LeftButton);
    send(QEvent::MouseButtonRelease, to, Qt::LeftButton, Qt::NoButton);
}

int checkFormHierarchy(QApplication &app, const QString &path, const QStringList &snapshots) {
    QTemporaryDir temp;
    const auto copy = temp.filePath("Notepad.sha");
    CHECK(QFile::copy(path, copy));
    HiasmQtMainWindow window;
    window.findChild<QAction *>("theme_dark")->trigger();
    CHECK(window.openScheme(copy));
    window.show();
    auto *tabs = window.findChild<QTabWidget *>("tabs");
    auto *editor = qobject_cast<SchemeEditor *>(tabs->currentWidget());
    auto *toggle = window.findChild<QAction *>("actionFormEdit");
    CHECK(toggle && !toggle->isChecked());
    toggle->trigger();
    app.processEvents();
    auto *canvas = editor->findChild<QWidget *>("formDesigner");
    auto *levels = editor->findChild<QToolButton *>("childLevels");
    auto *pathBar = editor->findChild<QWidget *>("schemePath");
    const auto currentLevel = [](SchemeEditor *view) {
        const auto *label = view->findChild<QLabel *>("currentSchemeLevel");
        return label ? label->text() : QString();
    };
    auto bar = [&] { return canvas->findChild<QToolBar *>("control_93842"); };
    auto memo = [&] { return canvas->findChild<QPlainTextEdit *>("control_66999"); };
    CHECK(toggle->isChecked() && bar() && memo() && memo()->isVisible() &&
          pathBar && pathBar->isVisible() && levels && levels->isEnabled() && currentLevel(editor) == "ROOT");
    CHECK(bar()->actions().size() == 5 && bar()->actions().first()->text() == "New" &&
          !bar()->actions().first()->icon().pixmap(16,16).isNull() && bar()->actions().last()->isSeparator());
    const auto client = bar()->parentWidget()->rect();
    CHECK(bar()->geometry() == QRect(0,0,client.width(),25));
    CHECK(memo()->geometry() == client.adjusted(0,25,0,0));
    const auto original = editor->source();
    CHECK(!canvas->findChild<QWidget *>("control_15904"));
    auto center = bar()->mapTo(canvas, bar()->rect().center());
    drag(canvas, center, center);
    auto *inspector = window.findChild<QTreeWidget *>("properties");
    const auto align = inspector->findItems("Align",Qt::MatchExactly);
    CHECK(align.size() == 1 && align.first()->text(1) == "caTop");
    if (!snapshots.isEmpty()) CHECK(window.grab().save(snapshots.at(0)));
    const auto edge = bar()->mapTo(canvas, QPoint(bar()->rect().center().x(),24));
    drag(canvas, edge, edge + QPoint(0,12));
    CHECK(bar()->height() == 37 && memo()->geometry() == client.adjusted(0,37,0,0));
    auto expected = original;
    expected.replace(" Height=25\n", " Height=37\n");
    CHECK(editor->source() == expected);
    editor->document()->undo();
    CHECK(editor->source() == original && bar()->height() == 25);
    bool openedChild = false;
    QTimer::singleShot(0, levels, [&] {
        auto *menu = levels->menu();
        const auto actions = menu->actions();
        if (menu->isVisible() && actions.size() == 1 && actions.first()->text().startsWith("ChildForm ")) {
            actions.first()->trigger();
            openedChild = true;
        }
        menu->close();
    });
    levels->showMenu();
    CHECK(openedChild);
    app.processEvents();
    auto *label = canvas->findChild<QLabel *>("control_29715");
    CHECK(label && label->isVisible() && label->text().contains('\n') && currentLevel(editor) == "ChildForm");
    CHECK(!bar() && !memo() && label->geometry() == QRect(100,15,175,50));
    center = label->mapTo(canvas, label->rect().center());
    drag(canvas, center, center + QPoint(7,5));
    expected = original;
    expected.replace(" Left=100\n", " Left=107\n").replace(" Top=15\n", " Top=20\n");
    CHECK(editor->source() == expected && window.saveCurrent());
    if (snapshots.size() >= 2) CHECK(window.grab().save(snapshots.at(1)));
    toggle->trigger();
    CHECK(!toggle->isChecked() && pathBar->isVisible() && currentLevel(editor) == "ChildForm");
    auto *editorViews = editor->findChild<QTabWidget *>("editorViews");
    auto *source = editor->findChild<QPlainTextEdit *>("sourceEditor");
    editorViews->setCurrentWidget(source->parentWidget());
    CHECK(pathBar->isVisible() && currentLevel(editor) == "ChildForm" &&
          source->textCursor().block().text().trimmed() == "BEGIN_SDK");
    if (snapshots.size() >= 3) CHECK(window.grab().save(snapshots.at(2)));
    toggle->trigger();
    auto *root = pathBar->findChild<QToolButton *>("schemeLevel0");
    CHECK(root && root->text() == "ROOT" && root->isVisible());
    root->click();
    CHECK(bar() && memo() && currentLevel(editor) == "ROOT");
    CHECK(window.openScheme(copy));
    auto *reopened = qobject_cast<SchemeEditor *>(tabs->currentWidget());
    CHECK(reopened->source() == expected && !toggle->isChecked());

    // A child panel is embedded on its parent form. Geometry belongs to its inner Panel.
    const auto panelFile = temp.filePath("panel.sha");
    QFile nested(panelFile);
    CHECK(nested.open(QIODevice::WriteOnly));
    nested.write("Make(cpp)\nAdd(MainForm,1,0,0)\n{\n}\nAdd(ChildPanel,2,80,70)\n{\n}\nBEGIN_SDK\n"
                 "Add(EditMulti,3,14,14)\n{\n}\nAdd(Panel,4,56,56)\n{\n Left=20\n Top=30\n Width=180\n Height=90\n}\n"
                 "Add(Button,5,100,90)\n{\n Left=12\n Top=10\n Caption=\"Child\"\n}\nEND_SDK\n"
                 "Add(GroupBox,6,200,80)\n{\n Left=210\n Top=30\n Caption=\"Options\"\n Flat=0\n}\n"
                 "Add(StringTable,7,200,140)\n{\n Left=210\n Top=140\n Width=125\n Height=90\n Grid=0\n Columns=#7:Name=70|8:Value=40|\n}\n"
                 "Add(StringTableMT,8,200,190)\n{\n Left=12\n Top=145\n Width=180\n Height=90\n ColumnWidth=58\n Columns=#12:Name=70=-1=2|5:Value|\n}\n");
    nested.close();
    CHECK(window.openScheme(panelFile));
    toggle->trigger();
    auto *panelEditor = qobject_cast<SchemeEditor *>(tabs->currentWidget());
    auto *designer = panelEditor->findChild<QWidget *>("formDesigner");
    auto panel = [&] { return designer->findChild<QWidget *>("control_2"); };
    CHECK(panel() && panel()->geometry() == QRect(20,30,180,90) && panel()->findChild<QPushButton *>("control_5"));
    CHECK(!qobject_cast<QGroupBox *>(panel()));
    auto *group = designer->findChild<QGroupBox *>("control_6");
    CHECK(group && group->title() == "Options" && group->isFlat());
    auto *table = designer->findChild<QTableWidget *>("control_7");
    CHECK(table && table->columnCount() == 2 && table->showGrid());
    CHECK(table->horizontalHeaderItem(0)->text() == "Name" && table->columnWidth(0) == 70);
    CHECK(table->horizontalHeaderItem(1)->text() == "Value" && table->columnWidth(1) == 40);
    auto *mtTable = designer->findChild<QTableWidget *>("control_8");
    CHECK(mtTable && mtTable->columnCount() == 2);
    CHECK(mtTable->horizontalHeaderItem(0)->text() == "Name" && mtTable->columnWidth(0) == 70);
    CHECK(mtTable->horizontalHeaderItem(1)->text() == "Value" && mtTable->columnWidth(1) == 58);
    auto origin = panel()->mapTo(designer, QPoint(100,50));
    const auto before = panelEditor->source();
    drag(designer, origin, origin + QPoint(8,4));
    expected = before;
    expected.replace(" Left=20\n Top=30\n", " Left=28\n Top=34\n");
    CHECK(panelEditor->source() == expected);
    panelEditor->document()->undo();
    CHECK(panelEditor->source() == before);
    origin = panel()->mapTo(designer, QPoint(100,50));
    QMouseEvent enter(QEvent::MouseButtonDblClick, origin, designer->mapToGlobal(origin), Qt::LeftButton, Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(designer,&enter);
    CHECK(currentLevel(panelEditor) == "ChildPanel");
    CHECK(!panel() && designer->findChild<QPushButton *>("control_5")->isVisible());
    auto *childButton = designer->findChild<QPushButton *>("control_5");
    QWidget *surface = childButton->parentWidget();
    const auto placement = surface->mapTo(designer, QPoint(60,40));
    auto *palette = window.findChild<QDockWidget *>("paletteDock");
    QListWidgetItem *item = nullptr;
    QListWidget *grid = nullptr;
    for (auto *list : palette->findChildren<QListWidget *>()) {
        const auto buttons = list->findItems("Button", Qt::MatchExactly);
        if (!buttons.isEmpty()) { item = buttons.first(); grid = list; break; }
    }
    CHECK(grid && item);
    grid->itemClicked(item);
    CHECK(panelEditor->source() == before && !designer->cursor().pixmap().isNull() &&
          designer->cursor().hotSpot() == QPoint(3, 3));
    drag(designer, placement, placement);
    const auto inserted = panelEditor->source();
    CHECK(inserted.contains("Left=60\n Top=40") && inserted.lastIndexOf("Add(Button,") < inserted.indexOf("END_SDK") &&
          designer->cursor().pixmap().isNull());
    CHECK(inspector->topLevelItem(0)->text(1) == "Button");
    const auto positions = inspector->findItems("Left", Qt::MatchExactly | Qt::MatchRecursive);
    CHECK(positions.size() == 1 && positions.first()->text(1) == "60");
    const auto caption = inspector->findItems("Caption", Qt::MatchExactly).first();
    auto *captionEdit = qobject_cast<QLineEdit *>(inspector->itemWidget(caption, 1));
    CHECK(captionEdit);
    captionEdit->setText("Nested button");
    captionEdit->setModified(true);
    captionEdit->editingFinished();
    app.processEvents();
    CHECK(panelEditor->source().contains("Caption=\"Nested button\""));
    const auto left = inspector->findItems("Left", Qt::MatchExactly | Qt::MatchRecursive);
    CHECK(left.size() == 1 && left.first()->checkState(0) == Qt::Unchecked);
    left.first()->setCheckState(0, Qt::Checked);
    app.processEvents();
    CHECK(panelEditor->source().contains("Point(doLeft)"));
    const int buttonId = panelEditor->selectedProperties().at(1).second.toInt();
    CHECK(panelEditor->connectPorts(buttonId, "onClick", buttonId, "doLeft"));
    const auto withLink = panelEditor->source();
    const auto enabledLeft = inspector->findItems("Left", Qt::MatchExactly | Qt::MatchRecursive);
    CHECK(enabledLeft.size() == 1 && enabledLeft.first()->checkState(0) == Qt::Checked);
    enabledLeft.first()->setCheckState(0, Qt::Unchecked);
    app.processEvents();
    CHECK(!panelEditor->source().contains("Point(doLeft)") && !panelEditor->source().contains(":doLeft,"));
    panelEditor->document()->undo();
    CHECK(panelEditor->source() == withLink);
    panelEditor->document()->undo();
    panelEditor->document()->undo();
    CHECK(!panelEditor->source().contains("Point(doLeft)"));
    auto *points = window.findChild<QToolButton *>("propertyPoints");
    auto *values = window.findChild<QToolButton *>("propertyValues");
    auto *system = window.findChild<QToolButton *>("propertySystem");
    CHECK(points && values && system && !points->icon().isNull() && !values->icon().isNull() && !system->icon().isNull());
    points->click();
    const auto focus = inspector->findItems("doSetFocus", Qt::MatchExactly);
    CHECK(focus.size() == 1 && inspector->isColumnHidden(1));
    focus.first()->setCheckState(0, Qt::Checked);
    app.processEvents();
    CHECK(panelEditor->source().contains("Point(doSetFocus)"));
    panelEditor->document()->undo();
    values->click();
    system->click();
    CHECK(!inspector->isColumnHidden(1) && inspector->findItems("Group", Qt::MatchExactly | Qt::MatchRecursive).size() == 1);
    system->click();
    panelEditor->deleteSelection();
    CHECK(panelEditor->source() == before);
    const std::unique_ptr<QMimeData> mime(grid->model()->mimeData({grid->indexFromItem(item)}));
    CHECK(mime && mime->hasFormat(FormCanvas::elementMime));
    QDragEnterEvent dragEnter(placement, Qt::CopyAction, mime.get(), Qt::LeftButton, Qt::NoModifier);
    QApplication::sendEvent(designer, &dragEnter);
    CHECK(dragEnter.isAccepted());
    QDropEvent drop(placement, Qt::CopyAction, mime.get(), Qt::NoButton, Qt::NoModifier);
    QApplication::sendEvent(designer, &drop);
    CHECK(drop.isAccepted() && panelEditor->source() == inserted);
    return 0;
}

int main(int argc,char **argv) {
    QApplication app(argc,argv);
    QTranslator translator;
    if (argc >= 3 && translator.load(app.applicationDirPath() + "/src/qt/hiasm_ru.qm"))
        app.installTranslator(&translator);
    QTemporaryDir settings;
    CHECK(settings.isValid() && argc >= 2);
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat,QSettings::UserScope,settings.path());
    if (app.arguments().at(1) == "--hierarchy" && argc >= 3)
        return checkFormHierarchy(app, app.arguments().at(2), app.arguments().mid(3));
    HiasmQtMainWindow window;
    CHECK(window.openScheme(app.arguments().at(1)));
    auto *viewMenu = window.findChild<QMenu *>("viewMenu");
    CHECK(viewMenu && window.menuBar()->actions().contains(viewMenu->menuAction()));
    window.findChild<QAction *>("theme_dark")->trigger();
    window.show();
    app.processEvents();
    auto *editor = qobject_cast<SchemeEditor *>(window.findChild<QTabWidget *>("tabs")->currentWidget());
    auto *views = editor->findChild<QTabWidget *>("editorViews");
    auto *diagram = editor->findChild<QGraphicsView *>("canvas");
    auto *zoom = editor->findChild<QSlider *>("zoomSlider");
    CHECK(zoom && zoom->value() == 100);
    zoom->setValue(150);
    CHECK(qFuzzyCompare(diagram->transform().m11(), 1.5));
    CHECK(editor->findChild<QLabel *>("zoomValue")->text() == "150%");
    zoom->setValue(100);
    const auto port = [&](int elementIndex, const char *name) -> QGraphicsItem * {
        for (auto *item : diagram->scene()->items())
            if (item->parentItem() && item->data(2).toString() == QString::fromLatin1(name) &&
                item->parentItem()->data(0).toInt() == elementIndex) return item;
        return nullptr;
    };
    auto *output = port(0,"onCreate");
    auto *input = port(0,"doClose");
    CHECK(output && input);
    CHECK(output->pos().x() == qgraphicsitem_cast<QGraphicsRectItem *>(output->parentItem())->rect().width());
    CHECK(input->pos().x() == 0);
    drag(diagram->viewport(), diagram->mapFromScene(output->scenePos()), diagram->mapFromScene(input->scenePos()));
    CHECK(editor->source().contains("link(onCreate,1:doClose,[])"));
    editor->document()->undo();
    CHECK(!editor->source().contains("link(onCreate,1:doClose,[])"));
    if (argc == 4) CHECK(window.grab().save(app.arguments().at(3)));
    views->setCurrentWidget(editor->findChild<QWidget *>("formPage"));
    app.processEvents();
    auto *canvas = editor->findChild<QWidget *>("formDesigner");
    auto button = [&] { return canvas->findChild<QPushButton *>("control_4"); };
    CHECK(button() && button()->isVisible() && button()->geometry() == QRect(268,16,96,32));
    CHECK(button()->palette().color(QPalette::Button).lightness() < 100);
    CHECK(button()->palette().color(QPalette::ButtonText).lightness() > 150);
    if (argc >= 3) CHECK(window.grab().save(app.arguments().at(2)));
    const QString original = editor->source();
    auto center = button()->mapTo(canvas, button()->rect().center());
    drag(canvas, center, center + QPoint(-12,7));
    CHECK(button()->pos() == QPoint(256,23) && editor->source().contains(" Left=256"));
    auto edge = button()->mapTo(canvas,button()->rect().bottomRight() - QPoint(1,1));
    drag(canvas,edge,edge + QPoint(15,8));
    CHECK(button()->size() == QSize(111,40));
    editor->document()->undo();
    CHECK(button()->size() == QSize(96,32) && button()->pos() == QPoint(256,23));
    editor->document()->undo();
    CHECK(editor->source() == original);
    auto *core = new SchemeEditor(QStringLiteral("Make(_base)\nAdd(hcTplForm,1,0,0)\n{\n}\n"
        "Add(hcTplButton,2,20,20)\n{\n Caption=\"Core\"\n}\n"), {}, {}, &window);
    core->findChild<QTabWidget *>("editorViews")->setCurrentWidget(core->findChild<QWidget *>("formPage"));
    auto *coreButton = core->findChild<QPushButton *>("control_2");
    CHECK(coreButton && coreButton->text() == "Core" && coreButton->size() == QSize(55,25));
    FormCanvas framePreview;
    framePreview.show();
    const auto preview = [&](int border, int clientSize) {
        const auto scheme = readSchemeDiagram(QStringLiteral("Make(cpp)\nAdd(MainForm,1,0,0)\n"
            "{\n Width=320\n Height=180\n BorderStyle=%1\n ClientSize=%2\n Color=#123456\n AlphaBlendValue=204\n}\n")
            .arg(border).arg(clientSize));
        return framePreview.setLayer(scheme, 0, QStringLiteral("cpp"), {});
    };
    CHECK(preview(2, 0));
    auto *frame = framePreview.findChild<QWidget *>("formFrame");
    auto *title = framePreview.findChild<QWidget *>("formTitle");
    auto *clientArea = framePreview.findChild<QWidget *>("formClient");
    CHECK(frame && title && clientArea && title->isVisible() && clientArea->size() == QSize(320,180));
    CHECK(frame->height() > clientArea->height() && clientArea->palette().color(QPalette::Window) == QColor("#123456"));
    CHECK(preview(2, 1) && frame->size() == QSize(320,180) && title->isVisible());
    CHECK(preview(0, 1) && title->isHidden() && clientArea->geometry() == frame->rect());
    CHECK(preview(4, 0) && title->isVisible() && clientArea->size() == QSize(320,180));
    for (const int border : {0, 4}) {
        CHECK(preview(border, 0));
        QTimer::singleShot(0, &framePreview, [&] {
            if (auto *menu = framePreview.findChild<QMenu *>()) {
                QKeyEvent select(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
                QApplication::sendEvent(menu, &select);
                QKeyEvent activate(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
                QApplication::sendEvent(menu, &activate);
                menu->close();
            }
        });
        framePreview.customContextMenuRequested(QPoint(20,20));
        auto *native = framePreview.findChild<QWidget *>("nativeFormPreview");
        CHECK(native && native->isWindow() && native->size() == QSize(320,180));
        CHECK(native->windowFlags().testFlag(Qt::FramelessWindowHint) == (border == 0));
        CHECK(qAbs(native->windowOpacity() - 0.8) < 0.01);
        native->close();
        QApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    }
    HiasmQtMainWindow reopened;
    CHECK(reopened.findChild<QAction *>("theme_dark")->isChecked());
    reopened.findChild<QAction *>("theme_light")->trigger();
    app.processEvents();
    CHECK(button()->palette().color(QPalette::Button).lightness() > 150);
}
