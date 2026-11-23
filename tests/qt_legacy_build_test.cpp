#include "main_window.h"

#include <QApplication>
#include <QDir>
#include <QDirIterator>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QTemporaryDir>
#include <QThread>
#include <QTimer>
#include <QFileDialog>
#include <cstdio>
#include <windows.h>

#define CHECK(condition) do { if (!(condition)) { std::fprintf(stderr, "%d: %s\n", __LINE__, #condition); return 1; } } while (false)

namespace {
template <typename Condition>
bool waitFor(Condition condition, int timeout = 15000) {
    QElapsedTimer timer;
    timer.start();
    while (!condition() && timer.elapsed() < timeout) {
        QApplication::processEvents();
        QThread::msleep(10);
    }
    return condition();
}

QByteArray contents(const QString &path) {
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

QString windowText(HWND window) {
    wchar_t text[256]{};
    GetWindowTextW(window, text, 256);
    return QString::fromWCharArray(text);
}

struct WindowSearch {
    DWORD pid;
    QString title;
    HWND result = nullptr;
};

BOOL CALLBACK findWindow(HWND window, LPARAM data) {
    auto &search = *reinterpret_cast<WindowSearch *>(data);
    DWORD pid = 0;
    GetWindowThreadProcessId(window, &pid);
    if (pid != search.pid || !IsWindowVisible(window) || windowText(window) != search.title) return TRUE;
    wchar_t cls[32]{};
    GetClassNameW(window, cls, 32);
    if (QString::fromWCharArray(cls) == "#32770") return TRUE;
    search.result = window;
    return FALSE;
}

bool ideRun(HiasmQtMainWindow &ide, const QString &scheme, const QString &title, const QString &expectedOutput = {}) {
    if (!ide.openScheme(scheme)) return false;
    auto *run = ide.findChild<QAction *>("actionRun");
    auto *log = ide.findChild<QPlainTextEdit *>("debugOutput");
    if (!run || !run->isEnabled()) return false;
    bool prompted = false;
    QTimer rejectSave;
    QObject::connect(&rejectSave, &QTimer::timeout, &ide, [&] {
        if (auto *dialog = qobject_cast<QFileDialog *>(QApplication::activeModalWidget())) {
            prompted = true;
            dialog->reject();
        }
    });
    rejectSave.start(10);
    run->trigger();
    const auto find = [&ide, &run](const QString &caption) -> HWND {
        if (run->isEnabled()) return nullptr;
        for (auto *process : ide.findChildren<QProcess *>()) {
            if (process->state() != QProcess::Running || process->arguments().contains("--build")) continue;
            WindowSearch search{DWORD(process->processId()), caption};
            EnumWindows(findWindow, reinterpret_cast<LPARAM>(&search));
            if (search.result) return search.result;
        }
        return nullptr;
    };
    HWND main = nullptr;
    waitFor([&] { return (main = find(title)) || run->isEnabled(); }, 30000);
    const auto fail = [&](const char *stage) {
        std::fprintf(stderr, "%s\n%s\n", stage, qPrintable(log->toPlainText()));
        for (auto *process : ide.findChildren<QProcess *>())
            if (process->state() != QProcess::NotRunning) process->kill();
        waitFor([&] { return run->isEnabled(); }, 5000);
        return false;
    };
    if (!main || prompted) return fail("Main window was not found or a save dialog opened");
    PostMessageW(main, WM_CLOSE, 0, 0);
    if (!waitFor([&] { return run->isEnabled(); })) return fail("Application did not close");
    if (!expectedOutput.isEmpty() && !contents(expectedOutput).startsWith("MZ")) return false;
    const auto *terminal = ide.findChild<QPlainTextEdit *>("programOutput");
    return terminal && terminal->toPlainText().contains("Process exited with code 0.");
}

QStringList installationFiles(const QString &root) {
    QStringList result;
    QDirIterator paths(root, QDir::Files | QDir::Hidden, QDirIterator::Subdirectories);
    while (paths.hasNext()) {
        const auto path = paths.next();
        const auto info = paths.fileInfo();
        result.append(path + ':' + QString::number(info.size()) + ':' + QString::number(info.lastModified().toMSecsSinceEpoch()));
    }
    result.sort();
    return result;
}
}

int main(int argc, char **argv) {
    QApplication::setAttribute(Qt::AA_DontUseNativeDialogs);
    QApplication app(argc, argv);
    const auto args = app.arguments();
    if (args.size() < 3 || args.at(2).isEmpty()) return 77;
    const QDir root(args.at(2));
    const auto info = root.filePath("Elements/Delphi/Example/Info.sha");
    const auto speed = root.filePath("Elements/Delphi/Example/FTCG/SpeedTest.sha");
    CHECK(QFileInfo::exists(info) && QFileInfo::exists(speed));
    qputenv("HIASM4_ROOTS", root.absolutePath().toUtf8());
    qunsetenv("HIASM4_CGBRIDGE");
    QTemporaryDir temp;
    CHECK(temp.isValid());
    const auto before = installationFiles(root.absolutePath());
    const auto cli = [&](const QStringList &command, const char *bridgeOverride = nullptr) {
        QProcess process;
        auto env = QProcessEnvironment::systemEnvironment();
        if (bridgeOverride) env.insert("HIASM4_CGBRIDGE", QString::fromUtf8(bridgeOverride));
        process.setProcessEnvironment(env);
        process.start(args.at(1), command);
        if (!process.waitForStarted() || !process.waitForFinished(45000)) return qMakePair(-1, QByteArray());
        const auto diagnostic = process.readAllStandardError();
        if (!diagnostic.isEmpty()) std::fprintf(stderr, "%s", diagnostic.constData());
        return qMakePair(process.exitCode(), diagnostic);
    };
    const auto output = temp.filePath(QStringLiteral("CLI \u041f\u0440\u043e\u0435\u043a\u0442.exe"));
    CHECK(cli({"--build", info, output}).first == 0);
    const auto binary = contents(output);
    CHECK(binary.startsWith("MZ"));
    const auto missing = cli({"--build", info, output}, "missing-hiasm4-bridge.exe");
    CHECK(missing.first == 1 && missing.second.contains("x86 HiAsm 4 CodeGen bridge is missing"));
    CHECK(contents(output) == binary);
    const auto rejected = cli({"--build", info, root.filePath("Int/qt-forbidden-build-output.exe")});
    CHECK(rejected.first == 1 && rejected.second.contains("inside a HiAsm 4 installation"));

    HiasmQtMainWindow ide;
    ide.show();
    CHECK(ideRun(ide, info, "Eorm"));
    const auto speedCopy = temp.filePath("Speed Test.sha");
    CHECK(QFile::copy(speed, speedCopy));
    CHECK(ideRun(ide, speedCopy, ":(", temp.filePath("build/Speed Test.exe")));
    for (const auto *example : {"ReplaceChars", "TechSpeedReport"}) {
        const auto fileName = QString::fromLatin1(example) + ".sha";
        const auto scheme = temp.filePath(fileName);
        CHECK(QFile::copy(root.filePath("Elements/Delphi/Example/FTCG/" + fileName), scheme));
        CHECK(ideRun(ide, scheme, "Form", temp.filePath("build/" + QString::fromLatin1(example) + ".exe")));
    }
    CHECK(installationFiles(root.absolutePath()) == before);
    return 0;
}
