#include "main_window.h"
#include "project_type.h"
#include "scheme_editor.h"
#include <QApplication>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileDialog>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <cstdio>

int main(int argc, char **argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    if (argc != 3) return 1;
    QTemporaryDir workspace;
    const auto scheme = workspace.filePath(QStringLiteral("Проект Qt.sha"));
    QFile original(QString::fromLocal8Bit(argv[1]));
    if (!original.open(QIODevice::ReadOnly)) return 2;
    auto contents = original.readAll();
    const bool gui = QByteArray(argv[2]) == "gui";
    const bool core = QByteArray(argv[2]) == "core";
    const bool draft = QByteArray(argv[2]) == "draft";
    if (core) {
        contents.replace("\n{", "\n{\n link(onCreate,2:doData,[])");
        contents += "\nAdd(hcDoData,2,56,28)\n{\n Data=String(Core Qt)\n link(onEventData,3:doPrint,[])\n}\n"
                    "Add(hcConsole,3,84,28)\n{\n link(onPrint,2953706:doClose,[])\n}\n";
    }
    if (gui) {
        // Close through a real generated Qt event after the window is created.
        contents.replace(" Backend=1", " link(onCreate,1:doClose,[])\n Backend=1");
    }
    HiasmQtMainWindow window;
    if (!draft) {
        QFile copy(scheme);
        if (!copy.open(QIODevice::WriteOnly) || copy.write(contents) != contents.size()) return 3;
        copy.close();
        if (!window.openScheme(scheme)) return 4;
    }
    auto *run = window.findChild<QAction *>("actionRun");
    auto *terminal = window.findChild<QPlainTextEdit *>("programOutput");
    auto *log = window.findChild<QPlainTextEdit *>("debugOutput");
    const auto trigger = [&](QAction *action) {
        bool prompted = false;
        QTimer rejectSave;
        rejectSave.setSingleShot(true);
        QObject::connect(&rejectSave, &QTimer::timeout, &window, [&] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
                prompted = true;
                dialog->reject();
            }
        });
        rejectSave.start(0);
        action->trigger();
        QElapsedTimer timer;
        timer.start();
        while (!action->isEnabled() && timer.elapsed() < 90000) {
            app.processEvents();
            QThread::msleep(10);
        }
        app.processEvents();
        return !prompted && action->isEnabled();
    };
    const auto suffix = artifactSuffix(ProjectType::Console);
    QString draftProgram;
    if (draft) {
        auto *tabs = window.findChild<QTabWidget *>("tabs");
        auto *editor = qobject_cast<SchemeEditor *>(tabs->currentWidget());
        auto *build = window.findChild<QAction *>("actionBuild");
        auto *errors = window.findChild<QTreeWidget *>("errors");
        const auto replaceDraft = [&](const QString &source) {
            QTextCursor cursor(editor->document());
            cursor.select(QTextCursor::Document);
            cursor.insertText(source);
        };
        replaceDraft("Make(cpp)\nAdd(Bad,1,0)\n");
        if (!trigger(build) || errors->topLevelItemCount() != 1) return 6;
        errors->itemActivated(errors->topLevelItem(0), 0);
        if (tabs->currentWidget() != editor || tabs->count() != 1 ||
            editor->findChild<QPlainTextEdit *>("sourceEditor")->textCursor().blockNumber() != 1) return 7;
        replaceDraft(QString::fromUtf8(contents));
        if (!trigger(build) || !editor->fileName().isEmpty() || !editor->document()->isModified()) return 8;
        for (const auto &line : log->toPlainText().split(u'\n')) {
            if (line.endsWith("/build/qt-native/Project" + suffix) && QFileInfo::exists(line))
                draftProgram = line;
        }
        const auto work = QFileInfo(draftProgram).dir().absoluteFilePath("../..");
        if (draftProgram.isEmpty() || !QFileInfo(QDir(work).filePath("Project.sha")).isFile() ||
            QFileInfo(work).canonicalFilePath().isEmpty() ||
            QFileInfo(QFileInfo(work).canonicalFilePath()).absoluteDir() != QDir(QDir::temp().canonicalPath()))
            return 9;
        auto changed = editor->source();
        changed.replace("Hello from C++17", "Hello from draft");
        replaceDraft(changed);
        if (!trigger(run) || !terminal->toPlainText().contains("Hello from draft") ||
            !log->toPlainText().contains(draftProgram) || !editor->fileName().isEmpty() ||
            !editor->document()->isModified() || QFileInfo::exists(scheme)) return 10;

        QTimer::singleShot(0, &window, [&] {
            if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
                dialog->selectFile(scheme);
                static_cast<QDialog *>(dialog)->accept();
            }
        });
        if (!window.saveCurrentAs() || editor->fileName() != scheme || editor->document()->isModified()) return 11;
        QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
        if (QFileInfo::exists(work)) return 12;
    }
    if (!trigger(run)) return 13;
    const auto output = terminal->toPlainText();
    const auto expected = core ? "Core Qt" : draft ? "Hello from draft" : "Hello from C++17";
    const auto artifact = workspace.filePath("build/qt-native/" + QFileInfo(scheme).completeBaseName() + suffix);
    if (!output.contains("Process exited with code 0.") || (!gui && !output.contains(expected)) ||
        !QFileInfo::exists(artifact)) {
        std::fprintf(stderr, "%s\n%s\n", qPrintable(log->toPlainText()), qPrintable(output));
        return 5;
    }
    return 0;
}
