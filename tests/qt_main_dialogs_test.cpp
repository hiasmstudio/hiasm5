#include "main_window.h"
#include "package_paths.h"
#include "scheme_editor.h"

#include <QApplication>
#include <QComboBox>
#include <QDialog>
#include <QDialogButtonBox>
#include <QFile>
#include <QElapsedTimer>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QPointer>
#include <QSplitter>
#include <QTimer>
#include <QSettings>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QThread>
#include <QToolButton>
#include <QTranslator>
#include <algorithm>
#include <cstdio>

namespace {
bool currentEditorUsesTemplate(QTabWidget *tabs, const QString &path) {
    auto *editor = tabs->currentWidget()->findChild<QPlainTextEdit *>("sourceEditor");
    QFile file(path);
    return editor && !editor->document()->isModified() &&
        file.open(QIODevice::ReadOnly | QIODevice::Text) &&
        editor->toPlainText() == QString::fromUtf8(file.readAll());
}

int fail(const char *message) {
    std::fprintf(stderr, "%s\n", message);
    return 1;
}
}

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QTemporaryDir temp;
    if (!temp.isValid()) return fail("Temporary directory unavailable");
    QSettings::setDefaultFormat(QSettings::IniFormat);
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope, temp.path());
    QTranslator translator;
    const bool capture = argc >= 2;
    if (capture && translator.load(app.applicationDirPath() + "/src/qt/hiasm_ru.qm"))
        app.installTranslator(&translator);
    if (!capture) {
        const auto install = temp.filePath("installation");
        qputenv("HIASM4_ROOTS", install.toUtf8() + ";" + install.toUtf8());
        const auto pack = install + "/Elements/CNET";
        for (const auto &dir : {"conf", "new", "icon", "Example/Examples"})
            if (!QDir().mkpath(pack + "/" + dir)) return fail("Creating fixture package");
        const auto db = app.applicationDirPath() + "/packs/cpp/elements.db";
        if (!QFile::copy(db, pack + "/Elements.db") || !QFile::copy(":/hiasm/logo.png", pack + "/icon/Console.png"))
            return fail("Creating fixture catalog");
        QFile file(pack + "/new/Console.sha");
        if (!file.open(QIODevice::WriteOnly)) return fail("Creating fixture template");
        file.write("Make(CNET)\nAdd(Console,1,0,0)\n{\n Caption=" + QByteArray::fromHex("cff0e8e2e5f2") + "\n}\n");
        file.close();
        if (!QFile::copy(file.fileName(), pack + "/Example/Examples/Sample.sha")) return fail("Creating fixture example");
        {
            auto catalog = QSqlDatabase::addDatabase("QSQLITE", "palette-fixture");
            catalog.setDatabaseName(pack + "/Elements.db");
            if (!catalog.open()) return fail(qPrintable(catalog.lastError().text()));
            QSqlQuery query(catalog);
            if (!query.exec("INSERT INTO elements(name,info,tab,pos) SELECT '*separator','Example subsection',id,99998 FROM groups WHERE name='Widgets'") ||
                !query.exec("INSERT INTO elements(name,info,tab,pos) SELECT 'SampleElement','',id,99999 FROM groups WHERE name='Widgets'"))
                return fail(qPrintable(query.lastError().text()));
        }
        QSqlDatabase::removeDatabase("palette-fixture");
        // The same legacy installation can also contain a native package.
        if (!QDir().mkpath(install + "/Elements/Web/conf") ||
            !QFile::copy(db, install + "/Elements/Web/Elements.db")) return fail("Creating shadow package");
    }
    HiasmQtMainWindow window;
    if (capture) window.findChild<QAction *>("theme_dark")->trigger();
    window.show();
    auto *tabs = window.findChild<QTabWidget *>("tabs");
    auto *create = window.findChild<QAction *>("actionNew");
    auto *manager = window.findChild<QAction *>("actionProjectManager");
    auto *about = window.findChild<QAction *>("actionAbout");
    if (!tabs || !create || !manager || !about)
        return fail("Main window actions or tabs are missing");
    if (!currentEditorUsesTemplate(tabs, ":/hiasm/templates/cpp/MainForm.sha"))
        return fail("The initial tab must use the default package template");

    auto *catalog = window.findChild<QTreeWidget *>("projectCatalog");
    if (window.dockCount() != 8 || !catalog || catalog->topLevelItemCount() < 4)
        return fail("Workspace docks and package examples must be available");
    auto *buildMode = window.findChild<QComboBox *>("buildMode");
    if (!buildMode || !buildMode->isEnabled() || buildMode->currentIndex() != 0)
        return fail("Default C++ build mode");
    buildMode->activated(1);
    const auto *initial = qobject_cast<SchemeEditor *>(tabs->currentWidget());
    if (!initial || !initial->source().contains("BuildMode=1")) return fail("Build setting must update the scheme");
    initial->document()->undo();

    bool cancelled = false;
    QTimer::singleShot(0, &window, [&] {
        if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
            auto *packs = dialog->findChild<QListWidget *>("packs");
            auto *projects = dialog->findChild<QListWidget *>("projects");
            auto *splitter = dialog->findChild<QSplitter *>("splitter");
            if (dialog->parentWidget() == &window && dialog->objectName() == "ProjectManager" &&
                packs && packs->currentItem() && packs->currentItem()->data(Qt::UserRole) == "cpp" && projects && projects->currentRow() == 1 &&
                splitter && splitter->orientation() == Qt::Vertical) {
                const auto height = projects->height();
                dialog->resize(dialog->width(), dialog->height() + 120);
                app.processEvents();
                cancelled = projects->height() > height;
            }
            dialog->reject();
        }
    });
    manager->trigger();
    if (!cancelled || tabs->count() != 1)
        return fail("Cancelling the project manager must preserve the tabs");

    const struct {
        const char *packId;
        int projectRow;
        const char *templatePath;
    } projectCases[] = {
        {"cpp", 0, ":/hiasm/templates/cpp/Entry.sha"},
        {"cpp", 1, ":/hiasm/templates/cpp/MainForm.sha"},
        {"_base", 0, ":/hiasm/templates/_base/hcTplForm.sha"},
        {"_base", 1, ":/hiasm/templates/_base/hcTplDialog.sha"}
    };
    for (const auto &project : projectCases) {
        const int previousCount = tabs->count();
        QTimer::singleShot(0, &window, [&] {
            if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
                auto *packs = dialog->findChild<QListWidget *>("packs");
                auto *projects = dialog->findChild<QListWidget *>("projects");
                auto *buttons = dialog->findChild<QDialogButtonBox *>("buttons");
                if (packs && projects && buttons) {
                    for (int i = 0; i < packs->count(); ++i)
                        if (packs->item(i)->data(Qt::UserRole) == project.packId) packs->setCurrentRow(i);
                    projects->setCurrentRow(project.projectRow);
                    if (auto *ok = buttons->button(QDialogButtonBox::Ok); ok && ok->isEnabled()) {
                        ok->click();
                        return;
                    }
                }
                dialog->reject();
            }
        });
        create->trigger();
        if (tabs->count() != previousCount + 1 ||
            !currentEditorUsesTemplate(tabs, QString::fromLatin1(project.templatePath)))
            return fail(project.templatePath);
    }

    bool external = false;
    QTimer::singleShot(0, &window, [&] {
        auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget());
        if (!dialog) return;
        auto *packs = dialog->findChild<QListWidget *>("packs");
        auto *projects = dialog->findChild<QListWidget *>("projects");
        auto *ok = dialog->findChild<QDialogButtonBox *>("buttons")->button(QDialogButtonBox::Ok);
        const auto cnet = packs->findItems("HiAsm 4: cnet", Qt::MatchExactly);
        if (cnet.size() == 1 && packs->findItems("HiAsm 4: web", Qt::MatchExactly).isEmpty()) {
            packs->setCurrentItem(cnet.first());
            const auto console = projects->findItems("Console", Qt::MatchExactly);
            external = !ok->isEnabled() && projects->currentRow() < 0 && console.size() == 1 &&
                !console.first()->icon().pixmap(20).isNull() &&
                !dialog->findChild<QPlainTextEdit *>("description")->toPlainText().isEmpty();
            if (external && capture) {
                app.processEvents();
                external = dialog->grab().save(app.arguments().at(1));
                std::printf("%d packages, %d cnet templates\n", packs->count(), projects->count());
            }
            if (external) {
                projects->setCurrentItem(console.first());
                ok->click();
                return;
            }
        }
        dialog->reject();
    });
    create->trigger();
    auto *externalEditor = qobject_cast<SchemeEditor *>(tabs->currentWidget());
    if (!external || !externalEditor || externalEditor->packName() != "cnet" ||
        !externalEditor->fileName().isEmpty() || (!capture && !externalEditor->source().contains(QStringLiteral("Привет"))))
        return fail("Opening an external template as a new scheme");
    auto *palette = window.findChild<QDockWidget *>("paletteDock")->findChild<QListWidget *>();
    if (!palette || !palette->count()) return fail("Loading the external package palette");
    if (!capture) {
        auto *paletteDock = window.findChild<QDockWidget *>("paletteDock");
        const auto sections = paletteDock->findChildren<QToolButton *>("paletteSection");
        const auto subsection = std::find_if(sections.cbegin(), sections.cend(),
            [](const auto *button) { return button->text() == "Example subsection"; });
        if (subsection == sections.cend()) return fail("Legacy palette must render a subsection");
        QListWidget *sectionGrid = nullptr;
        for (auto *grid : paletteDock->findChildren<QListWidget *>()) {
            if (!grid->findItems("*separator", Qt::MatchExactly).isEmpty()) return fail("Subsection offered as an element");
            if (!grid->findItems("SampleElement", Qt::MatchExactly).isEmpty()) sectionGrid = grid;
        }
        if (!sectionGrid || sectionGrid->isHidden()) return fail("Subsection must contain elements");
        (*subsection)->click();
        if (!sectionGrid->isHidden()) return fail("Subsection must collapse");
    }

    window.findChild<QAction *>("actionBuild")->trigger();
    const auto *errors = window.findChild<QTreeWidget *>("errors");
    if (errors->topLevelItemCount() != 1 || !errors->topLevelItem(0)->text(2).contains("cnet"))
        return fail("Missing build configuration diagnostic for the external project");

    QFile malformed(temp.filePath("malformed.sha"));
    if (!malformed.open(QIODevice::WriteOnly)) return fail("Creating invalid scheme");
    malformed.write("Make(cpp)\nAdd(MainForm,1,0,0)\n{\nAdd(Label,2,0,0)\n");
    malformed.close();
    if (!window.openScheme(malformed.fileName())) return fail("Opening invalid scheme for diagnosis");
    window.findChild<QAction *>("actionBuild")->trigger();
    if (errors->topLevelItemCount() != 1 || errors->topLevelItem(0)->text(1).isEmpty())
        return fail("Syntax error must identify a line");
    auto *currentErrors = window.findChild<QTreeWidget *>("errors");
    currentErrors->itemActivated(currentErrors->topLevelItem(0), 0);
    auto *source = tabs->currentWidget()->findChild<QPlainTextEdit *>("sourceEditor");
    if (!source || !source->isVisible()) return fail("Activating a diagnostic must open the scheme source");
    auto *repo = window.findChild<QTreeWidget *>("repositoryTree");
    if (!repo || repo->findItems("malformed.sha", Qt::MatchExactly | Qt::MatchRecursive).size() != 1)
        return fail("The repository tree must follow the current project");

    if (!capture && !QStandardPaths::findExecutable("git").isEmpty()) {
        // The test repository is independent of command-scoped Git configuration.
        qputenv("GIT_CONFIG_COUNT", "0");
        auto *actions = window.findChild<QPushButton *>("repositoryActions");
        auto *log = window.findChild<QPlainTextEdit *>("repositoryOutput");
        const auto trigger = [&](const QString &label) {
            for (auto *action : actions->menu()->actions()) {
                if (action->text() != label) continue;
                action->trigger();
                QElapsedTimer timer;
                timer.start();
                while (!actions->isEnabled() && timer.elapsed() < 5000) {
                    app.processEvents();
                    QThread::msleep(10);
                }
                return actions->isEnabled() && log->toPlainText().contains("Process exited with code 0.");
            }
            return false;
        };
        if (!trigger("Create Git repository") || !QFileInfo(temp.filePath(".git")).isDir() ||
            !trigger("Status") || !log->toPlainText().contains("?? malformed.sha"))
            return fail(qPrintable(log->toPlainText()));
    }

    const auto dockState = window.saveState();
    for (auto *dock : window.findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
        const auto vertical = [dock] { return dock->features().testFlag(QDockWidget::DockWidgetVerticalTitleBar); };
        if (vertical() != (window.dockWidgetArea(dock) == Qt::BottomDockWidgetArea))
            return fail("Initial dock title orientation");
        window.addDockWidget(Qt::BottomDockWidgetArea, dock);
        if (!vertical()) return fail("Bottom dock title must be vertical");
        dock->setFloating(true);
        if (vertical()) return fail("Floating dock title must be horizontal");
        dock->setFloating(false);
        if (!vertical()) return fail("Dock title must become vertical when docked again");
        for (auto area : {Qt::LeftDockWidgetArea, Qt::RightDockWidgetArea}) {
            window.addDockWidget(area, dock);
            if (vertical()) return fail("Side dock title must be horizontal");
        }
    }
    if (!window.restoreState(dockState)) return fail("Restoring original dock layout");
    for (auto *dock : window.findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly))
        if (dock->features().testFlag(QDockWidget::DockWidgetVerticalTitleBar) !=
            (window.dockWidgetArea(dock) == Qt::BottomDockWidgetArea))
            return fail("Restored dock title orientation");
    window.findChild<QDockWidget *>("terminalDock")->raise();
    app.processEvents();
    auto *terminalTabs = window.findChild<QTabWidget *>("terminalTabs");
    auto *shells = window.findChild<QComboBox *>("terminalShell");
    auto *add = window.findChild<QToolButton *>("addTerminalTab");
    if (!terminalTabs || terminalTabs->tabPosition() != QTabWidget::South || terminalTabs->count() != 1 ||
        !shells || !add || !terminalTabs->cornerWidget(Qt::BottomRightCorner)->isAncestorOf(add))
        return fail("Missing terminal tabs or bottom profile selector");
    const auto send = [](QPlainTextEdit *session, const QString &command) {
        session->moveCursor(QTextCursor::End);
        QKeyEvent type(QEvent::KeyPress, Qt::Key_unknown, Qt::NoModifier, command);
        QKeyEvent enter(QEvent::KeyPress, Qt::Key_Return, Qt::NoModifier);
        QApplication::sendEvent(session, &type);
        QApplication::sendEvent(session, &enter);
    };
    const auto waitFor = [&app](auto ready) {
        QElapsedTimer timer;
        timer.start();
        while (!ready() && timer.elapsed() < 3500) {
            app.processEvents();
            QThread::msleep(10);
        }
        return ready();
    };
    QStringList profiles{"cmd", "Windows PowerShell", "PowerShell 7", "sh"};
    if (!profiles.contains(terminalTabs->tabText(0))) profiles.prepend(terminalTabs->tabText(0));
    const auto distro = qEnvironmentVariable("HIASM_TEST_WSL");
    if (!distro.isEmpty()) {
        const auto label = "WSL: " + distro;
        if (!waitFor([&] { return shells->findText(label) >= 0; })) return fail("Missing installed WSL profile");
        profiles.append(label);
    }
    for (const auto &name : profiles) {
        const auto profile = shells->findText(name);
        if (profile < 0) continue;
        if (terminalTabs->tabText(0) == name) {
            terminalTabs->setCurrentIndex(0);
        } else {
            const int count = terminalTabs->count();
            shells->setCurrentIndex(profile);
            add->click();
            if (terminalTabs->count() != count + 1) return fail("New shell must open a separate tab");
        }
        auto *session = qobject_cast<QPlainTextEdit *>(terminalTabs->currentWidget());
        if (!session || terminalTabs->tabText(terminalTabs->currentIndex()) != name)
            return fail("The shell needs its own console");
        if (!waitFor([&] { return !session->isReadOnly(); })) return fail("The shell must start");
        if (name == "cmd") {
            send(session, "set HIASM_TEST=terminal");
            send(session, "echo %HIASM_TEST%-works");
        } else if (name != "Windows PowerShell" && name != "PowerShell 7") {
            send(session, "HIASM_TEST=terminal");
            send(session, "echo \"$HIASM_TEST-works\"");
        } else {
            send(session, "$hiasmTest = 'terminal-'");
            send(session, "$hiasmTest + 'works'");
        }
        if (!waitFor([&] { return session->toPlainText().contains("terminal-works\n"); })) {
            std::fprintf(stderr, "%s: %s\n", qPrintable(name), qPrintable(session->toPlainText()));
            return fail("Shell must execute commands and retain session state");
        }
        if (name.startsWith("WSL: ")) {
            send(session, "pwd");
            if (!waitFor([&] { return session->toPlainText().contains(QFileInfo(temp.path()).fileName()); }))
                return fail("WSL shell must start in the project directory");
        }
    }
    if (terminalTabs->count() == 1) add->click();
    if (terminalTabs->count() < 2) return fail("Multiple shell sessions must coexist");
    if (argc >= 3) {
        terminalTabs->setCurrentIndex(0);
        app.processEvents();
        if (!window.grab().save(app.arguments().at(2))) return fail("Could not save terminal snapshot");
    }
    auto *first = qobject_cast<QPlainTextEdit *>(terminalTabs->widget(0));
    const auto transcript = first->toPlainText();
    first->selectAll();
    QKeyEvent erase(QEvent::KeyPress, Qt::Key_Backspace, Qt::ControlModifier);
    QApplication::sendEvent(first, &erase);
    if (first->toPlainText() != transcript) return fail("Shell input must protect the output");
    first->moveCursor(QTextCursor::End);
    QKeyEvent previous(QEvent::KeyPress, Qt::Key_Up, Qt::NoModifier);
    QApplication::sendEvent(first, &previous);
    if (first->toPlainText() == transcript || !first->toPlainText().startsWith(transcript))
        return fail("Shell history must recall a command in the input area");
    QKeyEvent next(QEvent::KeyPress, Qt::Key_Down, Qt::NoModifier);
    QApplication::sendEvent(first, &next);
    if (first->toPlainText() != transcript) return fail("Shell history must restore the draft");
    QPointer<QWidget> closing(terminalTabs->widget(1));
    terminalTabs->tabCloseRequested(1);
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
    if (closing || first->findChild<QProcess *>()->state() == QProcess::NotRunning)
        return fail("Closing a shell tab must leave other sessions running");
    send(first, "exit 0");
    if (!waitFor([&] { return first->isReadOnly() && first->findChild<QProcess *>()->state() == QProcess::NotRunning; }))
        return fail("An exited shell must stop accepting input");

    if (!capture) {
        const auto packs = catalog->findItems("CNET", Qt::MatchFixedString);
        if (packs.size() != 1) return fail("Legacy package examples must appear in the catalog");
        packs.first()->setExpanded(true);
        auto *folder = packs.first()->child(0);
        if (!folder || folder->text(0) != "Examples") return fail("Catalog directory tree");
        folder->setExpanded(true);
        auto *example = folder->child(0);
        if (!example) return fail("Catalog example is missing");
        catalog->itemActivated(example, 0);
        const auto *selected = qobject_cast<SchemeEditor *>(tabs->currentWidget());
        if (!selected || !selected->fileName().endsWith("/Examples/Sample.sha") ||
            !isInLegacyInstallation(selected->fileName())) return fail("Opening a legacy example from the catalog");
    }

    bool aboutLoaded = false;
    QTimer::singleShot(0, &window, [&] {
        if (auto *dialog = qobject_cast<QDialog *>(QApplication::activeModalWidget())) {
            auto *logo = dialog->findChild<QLabel *>("logo");
            auto *license = dialog->findChild<QPlainTextEdit *>("licenseText");
            aboutLoaded = dialog->objectName() == "AboutDialog" && logo && !logo->pixmap().isNull() &&
                license && license->isReadOnly() && !license->toPlainText().isEmpty();
            dialog->reject();
        }
    });
    about->trigger();
    if (!aboutLoaded)
        return fail("The About dialog must show the bundled logo and license");
}
