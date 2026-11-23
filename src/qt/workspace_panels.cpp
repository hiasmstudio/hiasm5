#include "workspace_panels.h"
#include "package_paths.h"

#include <QAction>
#include <QComboBox>
#include <QCoreApplication>
#include <QContextMenuEvent>
#include <QDir>
#include <QFileDialog>
#include <QFontDatabase>
#include <QInputMethodEvent>
#include <QInputDialog>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QShowEvent>
#include <QStandardPaths>
#include <QStringDecoder>
#include <QStyle>
#include <QTabWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <memory>

namespace {
QString trMain(const char *text) { return QCoreApplication::translate("MainWindow", text); }
enum TreeRole { Path = Qt::UserRole, Directory, Loaded };
constexpr int wslProfileRole = Qt::UserRole + 1;

void appendOutput(QPlainTextEdit *output, const QString &text) {
    auto cursor = output->textCursor();
    cursor.movePosition(QTextCursor::End);
    cursor.insertText(text);
    output->setTextCursor(cursor);
    output->ensureCursorVisible();
}

QString executable(const QString &name) {
    if (QFileInfo(name).isAbsolute()) return QFileInfo(name).isExecutable() ? name : QString();
    return QStandardPaths::findExecutable(name);
}

void streamOutput(QProcess *process, QPlainTextEdit *output) {
    process->setProcessChannelMode(QProcess::MergedChannels);
    const auto decoder = std::make_shared<QStringDecoder>(QStringDecoder::Utf8);
    QObject::connect(process, &QProcess::readyReadStandardOutput, output, [process, output, decoder] {
        appendOutput(output, (*decoder)(process->readAllStandardOutput()));
    });
    QObject::connect(process, &QProcess::started, output, [decoder] { decoder->resetState(); });
    QObject::connect(process, &QProcess::errorOccurred, output, [process, output] {
        output->appendPlainText(process->errorString());
    });
    QObject::connect(process, &QProcess::finished, output, [output](int code, QProcess::ExitStatus) {
        output->appendPlainText(trMain("Process exited with code %1.").arg(code));
    });
}

void stopProcess(QProcess *process) {
    process->disconnect();
    if (process->state() == QProcess::NotRunning) return;
    process->kill();
    process->waitForFinished(1000);
}

QPlainTextEdit *makeOutput(const QString &name, QWidget *parent) {
    auto *output = new QPlainTextEdit(parent);
    output->setObjectName(name);
    output->setReadOnly(true);
    output->setMaximumBlockCount(5000);
    output->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    return output;
}

class ShellSession final : public QPlainTextEdit {
public:
    explicit ShellSession(bool windowsCmd, bool powershell, QWidget *parent)
        : QPlainTextEdit(parent), process_(new QProcess(this)), inputStart_(document()),
          lineEnding_(windowsCmd ? "\r\n" : "\n") {
        setObjectName("terminalOutput");
        setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        setMaximumBlockCount(5000);
        setAcceptDrops(false);
        viewport()->setAcceptDrops(false);
        inputStart_.setKeepPositionOnInsert(true);
        setReadOnly(true);

        process_->setProcessChannelMode(QProcess::MergedChannels);
        connect(process_, &QProcess::readyReadStandardOutput, this, [this] {
            appendShellText(decoder_(process_->readAllStandardOutput()));
        });
        connect(process_, &QProcess::started, this, [this, powershell] {
            if (powershell)
                process_->write("[Console]::InputEncoding = [Console]::OutputEncoding = [Text.UTF8Encoding]::new($false)\n");
            setReadOnly(false);
        });
        connect(process_, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
            appendShellText('\n' + process_->errorString() + '\n');
            if (error == QProcess::FailedToStart) setReadOnly(true);
        });
        connect(process_, &QProcess::finished, this, [this](int code, QProcess::ExitStatus) {
            appendShellText('\n' + trMain("Process exited with code %1.").arg(code) + '\n');
            setReadOnly(true);
        });
    }

    ~ShellSession() override { stopProcess(process_); }

    void start(const QString &program, const QStringList &args, const QString &directory) {
        process_->setWorkingDirectory(directory);
        process_->start(program, args);
    }

protected:
    void keyPressEvent(QKeyEvent *event) override {
        if (isReadOnly() || event->matches(QKeySequence::Copy) || event->matches(QKeySequence::SelectAll)) {
            QPlainTextEdit::keyPressEvent(event);
            return;
        }
        const int key = event->key();
        const auto mods = event->modifiers();
        const bool plain = !(mods & (Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier));
        if ((key == Qt::Key_Return || key == Qt::Key_Enter) && plain) {
            submit();
            return;
        }
        if ((key == Qt::Key_Up || key == Qt::Key_Down) && mods == Qt::NoModifier) {
            recallCommand(key == Qt::Key_Up ? 1 : -1);
            return;
        }
        if (key == Qt::Key_Home && plain) {
            auto cursor = textCursor();
            cursor.setPosition(inputStart_.position(), mods.testFlag(Qt::ShiftModifier)
                ? QTextCursor::KeepAnchor : QTextCursor::MoveAnchor);
            setTextCursor(cursor);
            return;
        }
        const auto cursor = textCursor();
        if (key == Qt::Key_Backspace || key == Qt::Key_Delete ||
            event->matches(QKeySequence::Cut) || event->matches(QKeySequence::DeleteStartOfWord)) {
            if (cursor.selectionStart() < inputStart_.position()) return;
            if (key == Qt::Key_Backspace || event->matches(QKeySequence::DeleteStartOfWord)) {
                if (cursor.hasSelection()) {
                    auto selection = cursor;
                    selection.removeSelectedText();
                } else {
                    auto selection = cursor;
                    selection.movePosition(mods.testFlag(Qt::ControlModifier)
                        ? QTextCursor::PreviousWord : QTextCursor::PreviousCharacter);
                    selection.setPosition(qMax(selection.position(), inputStart_.position()));
                    selection.setPosition(cursor.position(), QTextCursor::KeepAnchor);
                    selection.removeSelectedText();
                }
                return;
            }
        }
        if (!event->text().isEmpty() || event->matches(QKeySequence::Paste) ||
            event->matches(QKeySequence::DeleteEndOfWord) || event->matches(QKeySequence::DeleteEndOfLine))
            keepCursorInInput();
        QPlainTextEdit::keyPressEvent(event);
    }

    void insertFromMimeData(const QMimeData *data) override {
        if (isReadOnly() || !data->hasText()) return;
        keepCursorInInput();
        auto input = data->text();
        input.replace(QStringLiteral("\r\n"), QStringLiteral("\n")).replace(u'\r', u' ').replace(u'\n', u' ');
        insertPlainText(input);
    }

    void inputMethodEvent(QInputMethodEvent *event) override {
        keepCursorInInput();
        if (textCursor().position() + event->replacementStart() >= inputStart_.position())
            QPlainTextEdit::inputMethodEvent(event);
    }

    void contextMenuEvent(QContextMenuEvent *event) override {
        QMenu menu(this);
        menu.addAction(trMain("Copy"), this, &QPlainTextEdit::copy)->setEnabled(textCursor().hasSelection());
        menu.addAction(trMain("Paste"), this, &QPlainTextEdit::paste)->setEnabled(canPaste());
        menu.addSeparator();
        menu.addAction(trMain("Clear"), this, [this] {
            const auto command = currentInput().selectedText();
            clear();
            inputStart_.movePosition(QTextCursor::Start);
            if (!isReadOnly()) insertPlainText(command);
        });
        menu.exec(event->globalPos());
    }

private:
    QTextCursor currentInput() const {
        // Qt tracks the marker position and selection anchor independently.
        QTextCursor cursor(document());
        cursor.setPosition(inputStart_.position());
        cursor.movePosition(QTextCursor::End, QTextCursor::KeepAnchor);
        return cursor;
    }

    void keepCursorInInput() {
        if (textCursor().selectionStart() < inputStart_.position()) moveCursor(QTextCursor::End);
    }

    void appendShellText(const QString &text) {
        const auto caret = textCursor();
        const bool following = !caret.hasSelection() && caret.position() >= inputStart_.position();
        QTextCursor output(document());
        output.setPosition(inputStart_.position());
        output.insertText(text);
        inputStart_.setPosition(output.position());
        if (following) ensureCursorVisible();
    }

    void submit() {
        if (process_->state() != QProcess::Running) return;
        const auto command = currentInput().selectedText();
        moveCursor(QTextCursor::End);
        insertPlainText("\n");
        inputStart_.setPosition(textCursor().position());
        process_->write(command.toUtf8() + lineEnding_);
        if (!command.isEmpty() && (history_.isEmpty() || history_.first() != command)) {
            history_.prepend(command);
            if (history_.size() > 100) history_.removeLast();
        }
        historyIndex_ = -1;
    }

    void recallCommand(int step) {
        if (history_.isEmpty()) return;
        auto cursor = currentInput();
        if (historyIndex_ < 0) unsubmitted_ = cursor.selectedText();
        historyIndex_ = qBound(-1, historyIndex_ + step, int(history_.size()) - 1);
        cursor.insertText(historyIndex_ < 0 ? unsubmitted_ : history_.at(historyIndex_));
        setTextCursor(cursor);
    }

    QProcess *process_;
    QStringDecoder decoder_{QStringDecoder::Utf8};
    QTextCursor inputStart_;
    const QByteArray lineEnding_;
    QStringList history_;
    QString unsubmitted_;
    int historyIndex_ = -1;
};
}

ProjectTree::ProjectTree(QWidget *parent) : QTreeWidget(parent) {
    setHeaderHidden(true);
    connect(this, &QTreeWidget::itemExpanded, this, [this](QTreeWidgetItem *item) {
        if (item->data(0, Loaded).toBool()) return;
        item->setData(0, Loaded, true);
        fill(item, item->data(0, Path).toString());
    });
    connect(this, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item) {
        if (item->data(0, Directory).toBool()) item->setExpanded(true);
        else if (openScheme) openScheme(item->data(0, Path).toString());
    });
}

void ProjectTree::addDirectory(const QString &name, const QString &path) {
    if (!QFileInfo(path).isDir()) return;
    auto *item = new QTreeWidgetItem(this, {name});
    item->setData(0, Path, path);
    item->setData(0, Directory, true);
    item->setToolTip(0, QDir::toNativeSeparators(path));
    item->setIcon(0, style()->standardIcon(QStyle::SP_DirIcon));
    item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
}

void ProjectTree::fill(QTreeWidgetItem *parent, const QString &directory) {
    const auto entries = QDir(directory).entryInfoList(
        QDir::Dirs | QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks,
        QDir::DirsFirst | QDir::Name | QDir::IgnoreCase);
    parent->setChildIndicatorPolicy(QTreeWidgetItem::DontShowIndicatorWhenChildless);
    for (const auto &entry : entries) {
        if (entry.isJunction() || (!entry.isDir() && entry.suffix().compare("sha", Qt::CaseInsensitive))) continue;
        auto *item = new QTreeWidgetItem(parent, {entry.fileName()});
        item->setData(0, Path, entry.absoluteFilePath());
        item->setData(0, Directory, entry.isDir());
        item->setIcon(0, style()->standardIcon(entry.isDir() ? QStyle::SP_DirIcon : QStyle::SP_FileIcon));
        if (entry.isDir()) item->setChildIndicatorPolicy(QTreeWidgetItem::ShowIndicator);
    }
}

RepositoryPanel::RepositoryPanel(QWidget *parent) : QWidget(parent), tree_(new ProjectTree(this)),
    path_(new QLabel(this)), copy_(new QPushButton(trMain("Save a copy..."), this)),
    actions_(new QPushButton(trMain("Repository actions"), this)),
    output_(makeOutput("repositoryOutput", this)), process_(new QProcess(this)) {
    tree_->setObjectName("repositoryTree");
    actions_->setObjectName("repositoryActions");
    tree_->openScheme = [this](const QString &path) { if (openScheme) openScheme(path); };
    auto *row = new QHBoxLayout;
    auto *sources = new QMenu(this);
    auto *folder = new QPushButton(trMain("Directory"), this);
    folder->setMenu(sources);
    sources->addAction(trMain("Current project"), this, [this] { customDirectory_.clear(); refresh(); });
    sources->addAction(trMain("Choose directory..."), this, [this] {
        const auto path = QFileDialog::getExistingDirectory(this, trMain("Choose project directory"), directory());
        if (!path.isEmpty()) { customDirectory_ = path; refresh(); }
    });
    sources->addAction(trMain("Refresh"), this, [this] { refresh(); });
    row->addWidget(folder);
    row->addWidget(actions_);
    auto *menu = new QMenu(this);
    actions_->setMenu(menu);
    menu->addAction(trMain("Status"), this, [this] { run(client_ == "svn" ? QStringList{"status"} : QStringList{"status", "--short", "--branch"}); });
    menu->addAction(trMain("History"), this, [this] { run(client_ == "svn" ? QStringList{"log", "-l", "30"} : QStringList{"log", "-30", "--oneline"}); });
    menu->addAction(trMain("Changes"), this, [this] { run(client_ == "svn" ? QStringList{"diff"} : QStringList{"diff", "--no-ext-diff", "HEAD"}); });
    menu->addAction(trMain("Update repository"), this, [this] { run(client_ == "svn" ? QStringList{"update"} : QStringList{"pull", "--ff-only"}); });
    auto *init = menu->addAction(trMain("Create Git repository"), this, [this] { run({"init"}); });
    auto *clone = menu->addAction(trMain("Clone Git repository..."), this, [this] {
        const auto url = QInputDialog::getText(this, trMain("Clone Git repository..."), trMain("Repository URL")).trimmed();
        if (url.isEmpty()) return;
        const QDir target(directory());
        if (!target.isEmpty(QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot)) {
            output_->appendPlainText(trMain("Choose an empty directory for cloning."));
            return;
        }
        run({"clone", "--", url, "."});
    });
    connect(menu, &QMenu::aboutToShow, this, [this, init, clone] {
        init->setEnabled(client_.isEmpty());
        clone->setEnabled(client_.isEmpty());
    });
    connect(copy_, &QPushButton::clicked, this, [this] {
        const auto path = QFileDialog::getSaveFileName(this, trMain("Save a copy..."),
            QDir(directory()).filePath("Project.sha"), trMain("HiAsm schemes (*.sha);;All files (*)"));
        if (!path.isEmpty() && saveCopy) { saveCopy(path); refresh(); }
    });
    auto *layout = new QVBoxLayout(this);
    layout->addLayout(row);
    path_->setWordWrap(true);
    path_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    layout->addWidget(path_);
    layout->addWidget(tree_, 3);
    layout->addWidget(output_, 1);
    layout->addWidget(copy_);
    streamOutput(process_, output_);
    connect(process_, &QProcess::stateChanged, this, [this](QProcess::ProcessState state) {
        actions_->setEnabled(state == QProcess::NotRunning && !directory().isEmpty() &&
                             !isInLegacyInstallation(directory()));
    });
    connect(process_, &QProcess::finished, this, [this] { refresh(); });
    refresh();
}

RepositoryPanel::~RepositoryPanel() { stopProcess(process_); }

void RepositoryPanel::setProject(const QString &schemePath, bool canSave) {
    const auto path = schemePath.isEmpty() ? QString() : QFileInfo(schemePath).absolutePath();
    copy_->setEnabled(canSave && !directory().isEmpty());
    canSave_ = canSave;
    if (path == projectDirectory_) return;
    projectDirectory_ = path;
    refresh();
}

QString RepositoryPanel::directory() const {
    return customDirectory_.isEmpty() ? projectDirectory_ : customDirectory_;
}

void RepositoryPanel::refresh() {
    const auto path = directory();
    tree_->clear();
    client_.clear();
    if (!path.isEmpty()) {
        tree_->addDirectory(QFileInfo(path).fileName().isEmpty() ? path : QFileInfo(path).fileName(), path);
        if (auto *root = tree_->topLevelItem(0)) root->setExpanded(true);
        QDir dir(path);
        do {
            if (dir.exists(".git")) { client_ = "git"; break; }
            if (dir.exists(".svn")) { client_ = "svn"; break; }
        } while (!dir.isRoot() && dir.cdUp());
    }
    path_->setText(path.isEmpty() ? trMain("Open or save a project, or choose a directory.") :
                   QDir::toNativeSeparators(path) + (client_.isEmpty() ? QString() : " (" + client_.toUpper() + ')'));
    copy_->setEnabled(canSave_ && !path.isEmpty());
    actions_->setEnabled(!path.isEmpty() && !isInLegacyInstallation(path) && process_->state() == QProcess::NotRunning);
}

void RepositoryPanel::run(const QStringList &args) {
    const auto path = directory();
    if (path.isEmpty() || isInLegacyInstallation(path) || process_->state() != QProcess::NotRunning) return;
    const auto client = client_.isEmpty() ? QStringLiteral("git") : client_;
    const auto program = executable(client);
    if (program.isEmpty()) { output_->appendPlainText(trMain("Could not find %1.").arg(client)); return; }
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("GIT_TERMINAL_PROMPT", "0");
    env.insert("GIT_PAGER", "cat");
    if (client == "svn") {
        auto options = args;
        options.prepend("--non-interactive");
        process_->setArguments(options);
    } else {
        process_->setArguments(args);
    }
    output_->clear();
    appendOutput(output_, client + " " + args.join(' ') + '\n');
    process_->setProcessEnvironment(env);
    process_->setWorkingDirectory(path);
    process_->setProgram(program);
    process_->start();
}

TerminalPanel::TerminalPanel(QWidget *parent) : QWidget(parent), shells_(new QComboBox(this)),
    tabs_(new QTabWidget(this)), runOutput_(makeOutput("programOutput", tabs_)) {
    shells_->setObjectName("terminalShell");
    shells_->setAccessibleName(trMain("Shell"));
    shells_->setToolTip(trMain("Shell"));
    shells_->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    shells_->setMaximumWidth(260);
#ifdef Q_OS_WIN
    if (const auto cmd = executable(qEnvironmentVariable("ComSpec", "cmd.exe")); !cmd.isEmpty())
        shells_->addItem("cmd", QStringList{cmd, "/D", "/Q", "/K", "chcp 65001 >nul"});
    if (const auto powershell = executable("powershell.exe"); !powershell.isEmpty())
        shells_->addItem("Windows PowerShell", QStringList{powershell, "-NoLogo", "-NoProfile", "-NonInteractive", "-Command", "-"});
    if (const auto pwsh = executable("pwsh.exe"); !pwsh.isEmpty())
        shells_->addItem("PowerShell 7", QStringList{pwsh, "-NoLogo", "-NoProfile", "-NonInteractive", "-Command", "-"});
#else
    if (const auto shell = executable(qEnvironmentVariable("SHELL", "/bin/sh")); !shell.isEmpty())
        shells_->addItem(QFileInfo(shell).fileName(), QStringList{shell});
    if (const auto shell = executable("/bin/sh"); !shell.isEmpty() && shells_->findData(QStringList{shell}) < 0)
        shells_->addItem("sh", QStringList{shell});
#endif
    tabs_->setObjectName("terminalTabs");
    tabs_->setTabPosition(QTabWidget::South);
    tabs_->setDocumentMode(true);
    tabs_->setTabsClosable(true);
    tabs_->setMovable(true);
    runOutput_->hide();

    auto *actions = new QWidget(tabs_);
    auto *bar = new QHBoxLayout(actions);
    bar->setContentsMargins(6, 0, 3, 3);
    bar->setSpacing(4);
    auto *add = new QToolButton(actions);
    add->setObjectName("addTerminalTab");
    add->setText(QStringLiteral("+"));
    add->setAccessibleName(trMain("New terminal tab"));
    add->setToolTip(trMain("New terminal tab"));
    add->setEnabled(shells_->count() != 0);
    bar->addWidget(shells_);
    bar->addWidget(add);
    tabs_->setCornerWidget(actions, Qt::BottomRightCorner);

    auto *layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(tabs_);
    connect(shells_, &QComboBox::currentIndexChanged, add, [add](int index) { add->setEnabled(index >= 0); });
    connect(add, &QToolButton::clicked, this, &TerminalPanel::startShell);
    connect(tabs_, &QTabWidget::tabCloseRequested, this, &TerminalPanel::closeTab);
    connect(tabs_, &QTabWidget::currentChanged, this, [this] {
        if (isVisible() && tabs_->currentWidget()) tabs_->currentWidget()->setFocus();
    });
    setProjectDirectory({});
}

void TerminalPanel::showEvent(QShowEvent *event) {
    QWidget::showEvent(event);
    if (shown_) return;
    shown_ = true;
    if (tabs_->count() == 0) startShell();
#ifdef Q_OS_WIN
    addWslProfiles();
#endif
}

#ifdef Q_OS_WIN
void TerminalPanel::addWslProfiles() {
    auto wsl = executable(QDir(qEnvironmentVariable("SystemRoot")).filePath("System32/wsl.exe"));
    if (wsl.isEmpty()) wsl = executable("wsl.exe");
    if (wsl.isEmpty()) return;
    auto *query = new QProcess(this);
    connect(query, &QProcess::errorOccurred, this, [query](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) query->deleteLater();
    });
    connect(query, &QProcess::finished, this, [this, query, wsl](int code, QProcess::ExitStatus status) {
        if (code == 0 && status == QProcess::NormalExit) {
            const auto data = query->readAllStandardOutput();
            QStringDecoder decoder(data.contains('\0') ? QStringDecoder::Utf16LE : QStringDecoder::Utf8);
            const auto output = QString(decoder(data)).remove(QChar::Null);
            for (const auto &line : output.split(u'\n', Qt::SkipEmptyParts)) {
                const auto name = line.trimmed();
                if (name.isEmpty()) continue;
                shells_->addItem("WSL: " + name, QStringList{wsl, "--distribution", name});
                shells_->setItemData(shells_->count() - 1, true, wslProfileRole);
            }
        }
        query->deleteLater();
    });
    QTimer::singleShot(5000, query, [query] { query->kill(); });
    query->start(wsl, {"--list", "--quiet"});
}
#endif

QPlainTextEdit *TerminalPanel::showRunOutput() {
    runOutput_->clear();
    if (tabs_->indexOf(runOutput_) < 0) tabs_->addTab(runOutput_, trMain("Program output"));
    tabs_->setCurrentWidget(runOutput_);
    return runOutput_;
}

void TerminalPanel::setProjectDirectory(const QString &path) {
    projectDirectory_ = path.isEmpty() ? QDir::homePath() : path;
}

void TerminalPanel::startShell() {
    auto profile = shells_->currentData().toStringList();
    if (profile.isEmpty()) return;
    const auto program = profile.takeFirst();
    if (shells_->currentData(wslProfileRole).toBool())
        profile << "--cd" << QDir::toNativeSeparators(projectDirectory_);

    const bool windowsCmd = QFileInfo(program).fileName().compare("cmd.exe", Qt::CaseInsensitive) == 0;
    auto *session = new ShellSession(windowsCmd, profile.contains("-NonInteractive"), tabs_);
    const auto index = tabs_->addTab(session, shells_->currentText());
    tabs_->setTabToolTip(index, QDir::toNativeSeparators(projectDirectory_));
    tabs_->setCurrentIndex(index);
    session->start(program, profile, projectDirectory_);
    session->setFocus();
}

void TerminalPanel::closeTab(int index) {
    auto *page = tabs_->widget(index);
    if (!page) return;
    tabs_->removeTab(index);
    if (page != runOutput_) page->deleteLater();
}
