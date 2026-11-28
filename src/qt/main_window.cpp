#include "main_window.h"
#include "element_config.h"
#include "form_canvas.h"
#include "package_paths.h"
#include "legacy_build.h"
#include "workspace_panels.h"
#include <QListWidget>
#include <QStyledItemDelegate>
#include <QToolButton>
#include "project_type.h"
#include "project_manager_dialog.h"
#include "scheme_editor.h"
#include "ui_AboutDialog.h"

#include <QAction>
#include <QActionGroup>
#include <QButtonGroup>
#include <QDialogButtonBox>
#include <QPointer>
#include <cmath>
#include <QSettings>
#include <QShortcut>
#include <QStyleHints>
#include <QStyleFactory>
#include <QCloseEvent>
#include <QColorDialog>
#include <QClipboard>
#include <QTransform>
#include <QtEndian>
#include <QComboBox>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QHeaderView>
#include <QLineEdit>
#include <QImageReader>
#include <QBuffer>
#include <QTimer>
#include <QIntValidator>
#include <QDoubleValidator>
#include <QLocale>
#include <QSignalBlocker>
#include <QScrollBar>
#include <QSyntaxHighlighter>
#include <QFontDatabase>
#include <QMessageBox>
#include <QMimeData>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QTabWidget>
#include <QDockWidget>
#include <QDir>
#include <QSaveFile>
#include <QTemporaryDir>
#include <QStringConverter>
#include <QRegularExpression>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <memory>

namespace {
bool imageProperty(int type) { return type == 6 || type == 11 || type == 17 || type == 23; }

QImage decodePropertyImage(const QString &value) {
    constexpr quint32 limit = 4 * 1024 * 1024;
    if (value.size() > 2 * (limit + 65536)) return {};
    if (!value.startsWith("[ZIP") || !value.endsWith(u']'))
        return QImage::fromData(QByteArray::fromBase64(value.toLatin1()));
    auto bytes = QByteArray::fromHex(value.mid(4, value.size() - 5).toLatin1());
    if (bytes.size() < 5) return {};
    const auto length = qFromLittleEndian<quint32>(bytes.constData());
    if (!length || length > limit) return {};
    qToBigEndian(length, bytes.data());
    const auto decoded = qUncompress(bytes);
    return decoded.size() == length ? QImage::fromData(decoded) : QImage();
}

class StringsHighlighter final : public QSyntaxHighlighter {
public:
    using QSyntaxHighlighter::QSyntaxHighlighter;

    void setLanguage(int language) {
        language_ = language;
        rehighlight();
    }

protected:
    void highlightBlock(const QString &text) override {
        if (!language_) return;
        static const QRegularExpression literals(QStringLiteral(R"rx("(?:\\.|[^"\\])*"|'(?:\\.|[^'\\])*'|\b[0-9]+(?:\.[0-9]+)?\b)rx"));
        static const QRegularExpression keywords(QStringLiteral(
            R"(\b(if|else|elseif|then|for|while|do|end|begin|func|function|procedure|class|return|var|let|const|int|void|auto|true|false|null|select|from|where|insert|update|delete|Make|Add|link|BEGIN_SDK|END_SDK)\b)"),
            QRegularExpression::CaseInsensitiveOption);
        static const QRegularExpression ini(QStringLiteral(R"(^\s*(\[[^\]]+\]|[^=;#]+(?==)))"));
        static const QRegularExpression comments[] = {
            QRegularExpression(), QRegularExpression("//.*"), QRegularExpression("//.*|\\{.*\\}"),
            QRegularExpression("//.*|#.*"), QRegularExpression("'.*"), QRegularExpression("//.*"),
            QRegularExpression(";.*"), QRegularExpression("//.*"), QRegularExpression("^\\s*\\*.*"),
            QRegularExpression("--.*|#.*"), QRegularExpression("^\\s*[;#].*")
        };
        const auto apply = [&](const QRegularExpression &pattern, const QColor &color, bool bold = false) {
            QTextCharFormat format;
            format.setForeground(color);
            if (bold) format.setFontWeight(QFont::Bold);
            auto matches = pattern.globalMatch(text);
            while (matches.hasNext()) {
                const auto match = matches.next();
                setFormat(int(match.capturedStart()), int(match.capturedLength()), format);
            }
        };
        const bool dark = QApplication::palette().color(QPalette::Base).lightness() < 128;
        apply(language_ == 10 ? ini : keywords, dark ? QColor("#8ab4f8") : QColor("#17428a"), true);
        apply(literals, dark ? QColor("#f2ba87") : QColor("#973b20"));
        apply(comments[language_], dark ? QColor("#9ecb99") : QColor("#46733f"));
    }

private:
    int language_ = 0;
};

QColor propertyColor(QString value, const QPalette &palette) {
    value = value.trimmed();
    const bool delphiName = value.startsWith("cl", Qt::CaseInsensitive);
    if (delphiName) value.remove(0, 2);
    if (value.compare("BtnFace", Qt::CaseInsensitive) == 0) return palette.color(QPalette::Button);
    if (value.compare("BtnText", Qt::CaseInsensitive) == 0) return palette.color(QPalette::ButtonText);
    if (value.compare("Window", Qt::CaseInsensitive) == 0) return palette.color(QPalette::Base);
    if (value.compare("WindowText", Qt::CaseInsensitive) == 0) return palette.color(QPalette::Text);
    if (value.compare("Highlight", Qt::CaseInsensitive) == 0) return palette.color(QPalette::Highlight);
    if (value.compare("HighlightText", Qt::CaseInsensitive) == 0) return palette.color(QPalette::HighlightedText);
    if (delphiName && value.compare("Green", Qt::CaseInsensitive) == 0) return QColor(0, 128, 0);
    const auto named = QColor::fromString(value.toLower());
    if (named.isValid()) return named;
    bool ok = false;
    const auto bgr = value.startsWith(u'$') ? value.mid(1).toLongLong(&ok, 16) : value.toLongLong(&ok);
    if (ok && bgr >= 0 && bgr <= 0xffffff) return QColor(bgr & 255, (bgr >> 8) & 255, (bgr >> 16) & 255);
    return palette.color(QPalette::Window);
}

class PaletteIconDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void initStyleOption(QStyleOptionViewItem *option, const QModelIndex &index) const override {
        QStyledItemDelegate::initStyleOption(option, index);
        if (!option->icon.isNull()) option->text.clear();
        else option->text = option->text.left(3);
    }
};

class PaletteGrid final : public QListWidget {
public:
    explicit PaletteGrid(QWidget *parent) : QListWidget(parent) {
        setViewMode(QListView::IconMode);
        setMovement(QListView::Static);
        setResizeMode(QListView::Adjust);
        setIconSize(QSize(24, 24));
        setGridSize(QSize(32, 32));
        setFrameShape(QFrame::NoFrame);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        setItemDelegate(new PaletteIconDelegate(this));
        auto policy = QSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        policy.setHeightForWidth(true);
        setSizePolicy(policy);
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::DragOnly);
        setDefaultDropAction(Qt::CopyAction);
    }
    int heightForWidth(int width) const override {
        const int columns = qMax(1, width / gridSize().width());
        return ((count() + columns - 1) / columns) * gridSize().height();
    }
    QSize minimumSizeHint() const override { return gridSize(); }
    QSize sizeHint() const override { return {160, heightForWidth(160)}; }

protected:
    QMimeData *mimeData(const QList<QListWidgetItem *> &items) const override {
        auto *mime = new QMimeData;
        if (items.size() == 1) mime->setData(FormCanvas::elementMime, items.first()->text().toUtf8());
        return mime;
    }
};

const char *kUntitled = "Untitled.sha";

void applyTheme(const QString &mode) {
    const auto system = QGuiApplication::styleHints()->colorScheme();
    const bool dark = mode == "dark" || (mode == "system" && system == Qt::ColorScheme::Dark);
    if (qApp->style()->objectName() != "fusion") QApplication::setStyle("Fusion");
    QPalette colors = qApp->style()->standardPalette();
    if (dark) {
        const QColor text("#e6e8eb");
        colors.setColor(QPalette::Window, QColor("#282b30"));
        colors.setColor(QPalette::WindowText, text);
        colors.setColor(QPalette::Base, QColor("#1e2024"));
        colors.setColor(QPalette::AlternateBase, QColor("#30343a"));
        colors.setColor(QPalette::Text, text);
        colors.setColor(QPalette::Button, QColor("#373b42"));
        colors.setColor(QPalette::ButtonText, text);
        colors.setColor(QPalette::Mid, QColor("#747d89"));
        colors.setColor(QPalette::Light, QColor("#545b65"));
        colors.setColor(QPalette::Dark, QColor("#191b1e"));
        colors.setColor(QPalette::ToolTipBase, QColor("#31353b"));
        colors.setColor(QPalette::ToolTipText, text);
        colors.setColor(QPalette::Highlight, QColor("#305e96"));
        colors.setColor(QPalette::HighlightedText, Qt::white);
        colors.setColor(QPalette::Link, QColor("#86bbff"));
        colors.setColor(QPalette::PlaceholderText, QColor("#a1a8b3"));
        for (auto role : {QPalette::WindowText, QPalette::ButtonText, QPalette::Text})
            colors.setColor(QPalette::Disabled, role, QColor("#969da6"));
    }
    if (mode == "light") {
        // Fusion's standard palette follows the Windows system theme.
        colors = QPalette(QColor("#242629"), QColor("#efefef"), Qt::white, QColor("#b8bcc2"),
                          QColor("#969ba2"), Qt::black, Qt::black, Qt::white, QColor("#f2f3f5"));
        colors.setColor(QPalette::Highlight, QColor("#305e96"));
        colors.setColor(QPalette::HighlightedText, Qt::white);
        colors.setColor(QPalette::AlternateBase, QColor("#f5f6f8"));
        colors.setColor(QPalette::ToolTipBase, QColor("#fffbdc"));
        colors.setColor(QPalette::ToolTipText, Qt::black);
        for (auto role : {QPalette::WindowText, QPalette::ButtonText, QPalette::Text})
            colors.setColor(QPalette::Disabled, role, QColor("#686d73"));
    }
    QApplication::setPalette(colors);
}

QString trMain(const char *text) {
    return QCoreApplication::translate("MainWindow", text);
}

SchemeFileFormat schemeFileFormat(const QByteArray &contents) {
    SchemeFileFormat format;
    const auto firstBreak = contents.indexOf('\r');
    const auto firstLf = contents.indexOf('\n');
    if (firstBreak >= 0 && (firstLf < 0 || firstBreak < firstLf))
        format.lineEnding = firstLf == firstBreak + 1 ? "\r\n" : "\r";
    format.utf8Bom = contents.startsWith("\xef\xbb\xbf");
    format.windows1251 = !contents.isValidUtf8();
    return format;
}

bool writeSchemeFile(const QString &path, const QByteArray &contents) {
    QSaveFile file(path);
    return file.open(QIODevice::WriteOnly) && file.write(contents) == contents.size() && file.commit();
}
}

HiasmQtMainWindow::HiasmQtMainWindow(QWidget *parent) : QMainWindow(parent) {
    ui_.setupUi(this);
    createActions();
    createDocks();
    connect(ui_.tabs, &QTabWidget::tabCloseRequested, this, &HiasmQtMainWindow::closeEditor);
    connect(ui_.tabs, &QTabWidget::currentChanged, this, [this] {
        updateWindowTitle();
        updateProperties();
        updatePalette();
        updateProjectPanels();
    });

    createMenus();
    statusBar()->showMessage(trMain("Ready"));
    createFromTemplate(QStringLiteral(":/hiasm/templates/cpp/MainForm.sha"));
}

void HiasmQtMainWindow::createActions() {
    ui_.mainToolbar->setIconSize(QSize(20,20));
    for (auto *action : ui_.mainToolbar->actions()) {
        if (action->isSeparator()) continue;
        const auto name = action->objectName().mid(6).toLower();
        action->setIcon(QIcon(":/hiasm/actions/" + name + ".png"));
    }
    connect(ui_.actionFormEdit, &QAction::triggered, this, [this](bool enabled) {
        if (auto *editor = currentEditor()) editor->setFormView(enabled);
    });
    saveAction_ = ui_.actionSave;
    connect(saveAction_, &QAction::triggered, this, &HiasmQtMainWindow::saveScheme);
    connect(ui_.actionSaveAs, &QAction::triggered, this, &HiasmQtMainWindow::saveCurrentAs);
    closeAction_ = ui_.actionClose;
    connect(closeAction_, &QAction::triggered, this, &HiasmQtMainWindow::closeCurrent);
    runAction_ = ui_.actionRun;
    connect(runAction_, &QAction::triggered, this, &HiasmQtMainWindow::runCurrent);
    connect(ui_.actionStop, &QAction::triggered, this, [this] {
        if (process_) process_->kill();
    });
    buildAction_ = ui_.actionBuild;
    connect(buildAction_, &QAction::triggered, this, &HiasmQtMainWindow::buildCurrent);
    connect(ui_.actionNew, &QAction::triggered, this, &HiasmQtMainWindow::newScheme);
    connect(ui_.actionOpen, &QAction::triggered, this, &HiasmQtMainWindow::openSchemeDialog);
    connect(ui_.actionAbout, &QAction::triggered, this, &HiasmQtMainWindow::showAbout);
    connect(ui_.actionExit, &QAction::triggered, this, &QWidget::close);
    connect(ui_.actionProjectManager, &QAction::triggered, this, &HiasmQtMainWindow::newScheme);
}

void HiasmQtMainWindow::createMenus() {
    auto *themes = ui_.viewMenu->addMenu(trMain("Theme"));
    auto *group = new QActionGroup(themes);
    const auto saved = QSettings(QSettings::defaultFormat(), QSettings::UserScope, "HiAsm", "Studio").value("appearance/theme", "system").toString();
    for (const auto &mode : {QStringLiteral("system"), QStringLiteral("light"), QStringLiteral("dark")}) {
        auto *action = themes->addAction(mode == "dark" ? trMain("Dark") :
                                         mode == "light" ? trMain("Light") : trMain("Follow system"));
        action->setObjectName("theme_" + mode);
        action->setData(mode);
        action->setCheckable(true);
        group->addAction(action);
        action->setChecked(mode == saved);
    }
    if (!group->checkedAction()) group->actions().first()->setChecked(true);
    applyTheme(group->checkedAction()->data().toString());
    connect(group, &QActionGroup::triggered, this, [](QAction *action) {
        const QString mode = action->data().toString();
        QSettings(QSettings::defaultFormat(), QSettings::UserScope, "HiAsm", "Studio").setValue("appearance/theme", mode);
        applyTheme(mode);
    });
    connect(qApp->styleHints(), &QStyleHints::colorSchemeChanged, this, [group] {
        if (group->checkedAction()->data() == "system") applyTheme("system");
    });
    ui_.viewMenu->addSeparator();
    for (auto *dock : findChildren<QDockWidget *>()) ui_.viewMenu->addAction(dock->toggleViewAction());
}

void HiasmQtMainWindow::createDocks() {
    ui_.properties->setHeaderLabels({trMain("Property"), trMain("Value")});
    ui_.properties->header()->setSectionResizeMode(0, QHeaderView::Interactive);
    ui_.properties->header()->resizeSection(0, 145);
    auto *toolbar = new QWidget(ui_.propertyDockContents);
    auto *buttons = new QHBoxLayout(toolbar);
    buttons->setContentsMargins(0,0,0,0);
    buttons->setSpacing(2);
    const auto button = [&](const char *name, const char *icon, const QString &label) {
        auto *tool = new QToolButton(toolbar);
        tool->setObjectName(QString::fromLatin1(name));
        tool->setCheckable(true);
        tool->setIcon(QIcon(QStringLiteral(":/hiasm/actions/%1.png").arg(QString::fromLatin1(icon))));
        tool->setAccessibleName(label);
        tool->setToolTip(label);
        buttons->addWidget(tool);
        return tool;
    };
    auto *values = button("propertyValues", "property", trMain("Properties"));
    propertyPoints_ = button("propertyPoints", "points", trMain("Optional points"));
    propertySystem_ = button("propertySystem", "switch", trMain("Show system properties"));
    auto *modes = new QButtonGroup(toolbar);
    modes->addButton(values);
    modes->addButton(propertyPoints_);
    values->setChecked(true);
    buttons->addStretch();
    ui_.propertyLayout->insertWidget(0, toolbar);
    propertyDescription_ = new QLabel(ui_.propertyDockContents);
    propertyDescription_->setObjectName(QStringLiteral("propertyDescription"));
    propertyDescription_->setWordWrap(true);
    propertyDescription_->setTextFormat(Qt::PlainText);
    propertyDescription_->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    propertyDescription_->setMinimumHeight(42);
    propertyDescription_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
    ui_.propertyLayout->addWidget(propertyDescription_);
    connect(propertyPoints_, &QToolButton::toggled, this, &HiasmQtMainWindow::updateProperties);
    connect(propertySystem_, &QToolButton::toggled, this, &HiasmQtMainWindow::updateProperties);
    connect(ui_.properties, &QTreeWidget::currentItemChanged, this, [this](QTreeWidgetItem *item) {
        propertyDescription_->setText(item ? item->toolTip(0) : QString());
    });
    connect(ui_.properties, &QTreeWidget::itemChanged, this, [this](QTreeWidgetItem *item, int column) {
        const auto point = item->data(0, Qt::UserRole).toString();
        if (column != 0 || point.isEmpty()) return;
        const bool enabled = item->checkState(0) == Qt::Checked;
        const auto revision = propertyRevision_;
        QTimer::singleShot(0, this, [this, revision, point, enabled] {
            if (revision == propertyRevision_ && currentEditor()) currentEditor()->changeSelectedPoint(point, enabled);
        });
    });
    ui_.properties->setAlternatingRowColors(false);
    ui_.debugOutput->setMaximumBlockCount(5000);

    auto *catalog = new ProjectTree(this);
    catalog->setObjectName("projectCatalog");
    const auto openFromPanel = [this](const QString &path) {
        if (!this->openFromPanel(path)) statusBar()->showMessage(trMain("The scheme could not be opened."), 5000);
    };
    catalog->openScheme = openFromPanel;
    auto *catalogLayout = new QVBoxLayout(ui_.catalogDockContents);
    catalogLayout->setContentsMargins(0, 0, 0, 0);
    catalogLayout->addWidget(catalog);
    for (const auto &pack : {"_base", "cpp", "web", "modules"}) {
        for (const auto &root : nativePackRoots()) {
            const auto package = packageDirectory(root, pack);
            const auto examples = packageDirectory(package, "example");
            if (examples.isEmpty()) continue;
            catalog->addDirectory(QString::fromLatin1(pack), examples);
            break;
        }
    }
    for (const auto &pack : legacyPackDirectories())
        catalog->addDirectory(pack.fileName(), packageDirectory(pack.filePath(), "example"));

    repository_ = new RepositoryPanel(ui_.repositoryDockContents);
    repository_->openScheme = openFromPanel;
    repository_->saveCopy = [this](const QString &path) {
        if (auto *editor = currentEditor()) saveEditor(editor, path, true);
    };
    auto *repositoryLayout = new QVBoxLayout(ui_.repositoryDockContents);
    repositoryLayout->setContentsMargins(0, 0, 0, 0);
    repositoryLayout->addWidget(repository_);
    terminal_ = new TerminalPanel(ui_.terminalDockContents);
    auto *terminalLayout = new QVBoxLayout(ui_.terminalDockContents);
    terminalLayout->setContentsMargins(0, 0, 0, 0);
    terminalLayout->addWidget(terminal_);

    auto *buildLayout = new QVBoxLayout(ui_.buildSettingsDockContents);
    buildPackage_ = new QLabel(ui_.buildSettingsDockContents);
    buildPackage_->setWordWrap(true);
    buildLayout->addWidget(buildPackage_);
    auto *compiler = new QLineEdit(ui_.buildSettingsDockContents);
    compiler->setObjectName("buildCompiler");
    compiler->setPlaceholderText(trMain("Qt toolchain (automatic)"));
    compiler->setText(QSettings(QSettings::defaultFormat(), QSettings::UserScope, "HiAsm", "Studio")
        .value("build/compiler").toString());
    auto *pick = new QPushButton(trMain("Browse..."), ui_.buildSettingsDockContents);
    auto *compilerRow = new QHBoxLayout;
    compilerRow->addWidget(compiler, 1);
    compilerRow->addWidget(pick);
    buildLayout->addWidget(new QLabel(trMain("C++ compiler"), ui_.buildSettingsDockContents));
    buildLayout->addLayout(compilerRow);
    const auto saveCompiler = [compiler] {
        QSettings(QSettings::defaultFormat(), QSettings::UserScope, "HiAsm", "Studio")
            .setValue("build/compiler", compiler->text().trimmed());
    };
    connect(compiler, &QLineEdit::editingFinished, this, saveCompiler);
    connect(pick, &QPushButton::clicked, this, [this, compiler, saveCompiler] {
        const auto path = QFileDialog::getOpenFileName(this, trMain("C++ compiler"), compiler->text());
        if (!path.isEmpty()) { compiler->setText(path); saveCompiler(); }
    });
    buildMode_ = new QComboBox(ui_.buildSettingsDockContents);
    buildMode_->setObjectName("buildMode");
    buildMode_->addItems({"Debug", "Release"});
    buildLayout->addWidget(new QLabel(trMain("Build mode"), ui_.buildSettingsDockContents));
    buildLayout->addWidget(buildMode_);
    buildButton_ = new QPushButton(trMain("Build"), ui_.buildSettingsDockContents);
    buildLayout->addWidget(buildButton_);
    buildLayout->addStretch(1);
    connect(buildButton_, &QPushButton::clicked, this, &HiasmQtMainWindow::buildCurrent);
    connect(buildMode_, &QComboBox::activated, this, [this](int mode) {
        if (auto *editor = currentEditor()) editor->setBuildMode(mode);
    });

    ui_.errors->setHeaderLabels({trMain("Scheme"), trMain("Line"), trMain("Message")});
    ui_.errors->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    ui_.errors->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    connect(ui_.errors, &QTreeWidget::itemActivated, this, [this](QTreeWidgetItem *item) {
        const auto line = item->text(1).toInt();
        if (buildErrorEditor_) {
            ui_.tabs->setCurrentWidget(buildErrorEditor_);
            buildErrorEditor_->showSourceLine(line);
            return;
        }
        const auto path = item->data(0, Qt::UserRole).toString();
        if (!path.isEmpty() && this->openFromPanel(path))
            if (auto *editor = currentEditor()) editor->showSourceLine(line);
    });
    for (auto *dock : {ui_.catalogDock, ui_.repositoryDock, ui_.buildSettingsDock})
        tabifyDockWidget(ui_.propertyDock, dock);
    setTabPosition(Qt::RightDockWidgetArea, QTabWidget::East);
    ui_.propertyDock->raise();
    tabifyDockWidget(ui_.debugDock, ui_.errorsDock);
    tabifyDockWidget(ui_.debugDock, ui_.terminalDock);
    for (auto *dock : findChildren<QDockWidget *>(QString(), Qt::FindDirectChildrenOnly)) {
        const auto updateTitle = [dock] {
            const bool vertical = !dock->isFloating() && dock->dockLocation() == Qt::BottomDockWidgetArea;
            auto features = dock->features();
            features.setFlag(QDockWidget::DockWidgetVerticalTitleBar, vertical);
            dock->setFeatures(features);
        };
        connect(dock, &QDockWidget::dockLocationChanged, this, updateTitle);
        connect(dock, &QDockWidget::topLevelChanged, this, updateTitle);
        updateTitle();
    }
    ui_.debugDock->raise();
    resizeDocks({ui_.paletteDock, ui_.propertyDock}, {242, 300}, Qt::Horizontal);
    resizeDocks({ui_.debugDock}, {180}, Qt::Vertical);
}

void HiasmQtMainWindow::updatePalette() {
    const auto *editor = currentEditor();
    const auto pack = editor ? editor->packName() : QString();
    if (palettePack_ == pack) return;
    palettePack_ = pack;
    delete ui_.palette->takeWidget();
    auto *contents = new QWidget;
    auto *groups = new QVBoxLayout(contents);
    groups->setContentsMargins(2,2,2,2);
    groups->setSpacing(1);
    groups->setAlignment(Qt::AlignTop);
    ui_.palette->setWidget(contents);
    static const QRegularExpression identifier(QStringLiteral("^[a-z0-9_]+$"));
    if (!identifier.match(pack).hasMatch()) return;
    const auto root = editorPackRoot(pack);
    QString catalog;
    for (const auto &nativeRoot : nativePackRoots()) {
        catalog = packageDatabase(QDir(nativeRoot).filePath(pack));
        if (!catalog.isEmpty()) break;
    }
    if (catalog.isEmpty()) catalog = packageDatabase(packageDirectory(root, pack));
    if (catalog.isEmpty()) {
        ui_.debugOutput->appendPlainText(trMain("Element catalog not found for %1.").arg(pack));
        return;
    }
    const auto connection = QUuid::createUuid().toString();
    {
        auto db = QSqlDatabase::addDatabase("QSQLITE", connection);
        db.setDatabaseName(catalog);
        db.setConnectOptions("QSQLITE_OPEN_READONLY");
        QSqlQuery query(db);
        if (db.open() && query.exec("SELECT g.id,cast(g.name as blob),cast(e.name as blob),cast(e.info as blob) FROM groups g JOIN elements e ON e.tab=g.id "
                                    "WHERE g.name<>'' ORDER BY g.pos,g.id,e.pos,e.id")) {
            QWidget *group = nullptr;
            QVBoxLayout *sections = nullptr;
            QListWidget *grid = nullptr;
            int lastGroup = -1;
            while (query.next()) {
                const auto id = query.value(0).toInt();
                const auto name = decodePackageText(query.value(2).toByteArray());
                const auto description = decodePackageText(query.value(3).toByteArray());
                if (lastGroup != id) {
                    const bool first = !group;
                    auto *header = new QToolButton(contents);
                    header->setText(decodePackageText(query.value(1).toByteArray()));
                    header->setCheckable(true);
                    header->setChecked(first);
                    header->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                    header->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                    header->setArrowType(first ? Qt::DownArrow : Qt::RightArrow);
                    groups->addWidget(header);
                    group = new QWidget(contents);
                    sections = new QVBoxLayout(group);
                    sections->setContentsMargins(0, 0, 0, 0);
                    sections->setSpacing(2);
                    groups->addWidget(group);
                    group->setVisible(first);
                    connect(header, &QToolButton::toggled, group, [header,group](bool open) {
                        header->setArrowType(open ? Qt::DownArrow : Qt::RightArrow);
                        group->setVisible(open);
                    });
                    grid = nullptr;
                    lastGroup = id;
                }
                if (name.startsWith('*')) {
                    grid = new PaletteGrid(group);
                    auto section = description.isEmpty() ? name.mid(name.indexOf(u'_') + 1) : description;
                    if (section.startsWith('*')) section.remove(0, 1);
                    auto *heading = new QToolButton(group);
                    heading->setObjectName("paletteSection");
                    heading->setText(section);
                    heading->setCheckable(true);
                    heading->setChecked(true);
                    heading->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
                    heading->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                    heading->setArrowType(Qt::DownArrow);
                    sections->addWidget(heading);
                    connect(heading, &QToolButton::toggled, grid, [heading, grid](bool visible) {
                        heading->setArrowType(visible ? Qt::DownArrow : Qt::RightArrow);
                        grid->setVisible(visible && grid->count() > 0);
                    });
                    grid->setAccessibleName(section);
                }
                if (!grid || name.startsWith('*')) {
                    if (!grid) grid = new PaletteGrid(group);
                    sections->addWidget(grid);
                    grid->setVisible(!name.startsWith('*'));
                    connect(grid, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
                        if (auto *editor = currentEditor()) {
                            if (editor->armPlacement(item->text(), item->icon()))
                                statusBar()->showMessage(trMain("Click the canvas to place %1.").arg(item->text()), 5000);
                            else if (editor->isFormView())
                                statusBar()->showMessage(trMain("%1 belongs on the scheme.").arg(item->text()), 5000);
                        }
                    });
                    const auto insert = [this](QListWidgetItem *item) {
                        if (auto *editor = currentEditor()) editor->insertElement(item->text());
                    };
                    connect(grid, &QListWidget::itemDoubleClicked, this, insert);
                    auto *enter = new QShortcut(QKeySequence(Qt::Key_Return), grid);
                    enter->setKeys({QKeySequence(Qt::Key_Return), QKeySequence(Qt::Key_Enter)});
                    enter->setContext(Qt::WidgetWithChildrenShortcut);
                    connect(enter, &QShortcut::activated, grid, [grid, insert] {
                        if (auto *item = grid->currentItem()) insert(item);
                    });
                }
                if (name.startsWith('*')) continue;
                auto *item = new QListWidgetItem(elementIcon(root, pack, name), name, grid);
                item->setToolTip(name + "\n" + description);
                grid->show();
            }
        } else {
            ui_.debugOutput->appendPlainText(catalog + ": " +
                (db.isOpen() ? query.lastError().text() : db.lastError().text()));
        }
    }
    QSqlDatabase::removeDatabase(connection);
}

SchemeEditor *HiasmQtMainWindow::currentEditor() const {
    return qobject_cast<SchemeEditor *>(ui_.tabs->currentWidget());
}

void HiasmQtMainWindow::editProperty(const QString &name) {
    const QPointer<SchemeEditor> editor(currentEditor());
    if (!editor) return;
    const auto rows = editor->selectedProperties();
    if (rows.isEmpty()) return;
    const auto pack = editor->packName();
    const auto config = elementConfig(editorPackRoot(pack), pack, rows.first().second);
    const auto property = std::find_if(config.properties.cbegin(), config.properties.cend(), [&](const auto &p) {
        return (p.type == 2 || p.type == 3 || p.type == 5 || p.type == 19 || imageProperty(p.type)) &&
            (name.isEmpty() ? p.primary : QString::fromUtf8(p.name).compare(name, Qt::CaseInsensitive) == 0);
    });
    const bool systemHint = name == "@Hint";
    if (!systemHint && property == config.properties.cend()) return;
    const auto key = systemHint ? name : QString::fromUtf8(property->name);
    const int propertyType = systemHint ? 5 : property->type;
    const auto row = std::find_if(rows.cbegin() + SchemeEditor::firstPropertyRow, rows.cend(), [&](const auto &p) {
        return p.first.compare(key, Qt::CaseInsensitive) == 0;
    });
    const auto original = row == rows.cend() ? (systemHint ? QString() : QString::fromUtf8(property->initial)) : row->second;
    const auto revision = propertyRevision_;
    if (imageProperty(propertyType)) {
        const bool icon = propertyType == 6 || key.compare("Icon", Qt::CaseInsensitive) == 0;
        QDialog dialog(this);
        dialog.setObjectName(icon ? "iconEditor" : "pictureEditor");
        dialog.setWindowTitle(icon ? trMain("Icon") : trMain("Picture editor"));
        dialog.resize(icon ? QSize(340, 260) : QSize(560, 400));
        auto *layout = new QVBoxLayout(&dialog);
        auto *tools = new QHBoxLayout;
        layout->addLayout(tools);
        auto *preview = new QLabel(&dialog);
        preview->setObjectName("imagePreview");
        preview->setAlignment(Qt::AlignCenter);
        preview->setBackgroundRole(QPalette::Base);
        preview->setAutoFillBackground(true);
        preview->setMinimumSize(160, 140);
        layout->addWidget(preview, 1);
        auto *size = new QLabel(&dialog);
        layout->addWidget(size);
        QImage image = decodePropertyImage(original);
        bool changed = false;
        const auto refresh = [&] {
            if (image.isNull()) preview->setText(trMain("No image"));
            else preview->setPixmap(QPixmap::fromImage(image.scaled(320, 220, Qt::KeepAspectRatio, Qt::SmoothTransformation)));
            size->setText(trMain("Size: %1 × %2").arg(image.width()).arg(image.height()));
        };
        const auto setImage = [&](QImage next) {
            if (!next.isNull() && (next.width() > 1024 || next.height() > 1024)) {
                QMessageBox::warning(&dialog, dialog.windowTitle(), trMain("Maximum image size is 1024 × 1024."));
                return;
            }
            image = std::move(next);
            changed = true;
            refresh();
        };
        const auto tool = [&](const char *id, const QString &caption, auto operation) {
            auto *button = new QToolButton(&dialog);
            button->setObjectName(QString::fromLatin1(id));
            button->setText(caption);
            tools->addWidget(button);
            connect(button, &QToolButton::clicked, &dialog, operation);
        };
        tool("imageOpen", trMain("Open image"), [&] {
            const auto path = QFileDialog::getOpenFileName(&dialog, dialog.windowTitle(), {},
                trMain("Images (*.ico *.png *.bmp *.jpg *.jpeg *.gif *.webp);;All files (*)"));
            if (path.isEmpty()) return;
            QImageReader reader(path);
            reader.setAutoTransform(true);
            const auto dimensions = reader.size();
            if (dimensions.width() > 1024 || dimensions.height() > 1024) {
                QMessageBox::warning(&dialog, dialog.windowTitle(), trMain("Maximum image size is 1024 × 1024."));
                return;
            }
            auto loaded = reader.read();
            if (loaded.isNull()) QMessageBox::warning(&dialog, dialog.windowTitle(), reader.errorString());
            else setImage(std::move(loaded));
        });
        tool("imageClear", trMain("Clear image"), [&] { setImage({}); });
        tool("imagePaste", trMain("Paste image"), [&] {
            auto copied = QApplication::clipboard()->image();
            if (!copied.isNull()) setImage(std::move(copied));
        });
        if (!icon) {
            tool("imageRotate", trMain("Rotate right"), [&] {
                if (!image.isNull()) setImage(image.transformed(QTransform().rotate(90)));
            });
            tool("imageSave", trMain("Save image"), [&] {
                if (image.isNull()) return;
                const auto path = QFileDialog::getSaveFileName(&dialog, trMain("Save image"), {}, trMain("PNG image (*.png)"));
                if (path.isEmpty()) return;
                QSaveFile file(path);
                if (!file.open(QIODevice::WriteOnly) || !image.save(&file, "PNG") || !file.commit())
                    QMessageBox::warning(&dialog, dialog.windowTitle(), trMain("Could not save the image."));
            });
        }
        tools->addStretch();
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        layout->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        refresh();
        if (dialog.exec() != QDialog::Accepted || !changed || !editor || currentEditor() != editor || revision != propertyRevision_) return;
        QString serialized;
        if (!image.isNull()) {
            QByteArray bytes;
            QBuffer buffer(&bytes);
            buffer.open(QIODevice::WriteOnly);
            const auto *format = propertyType == 6 ? "ICO" : propertyType == 11 ? "BMP" : propertyType == 17 ? "JPEG" : "PNG";
            if (!image.save(&buffer, format) || bytes.size() > 4 * 1024 * 1024) {
                QMessageBox::warning(this, dialog.windowTitle(), trMain("Could not encode the image for this property."));
                return;
            }
            if (propertyType == 23) serialized = QString::fromLatin1(bytes.toBase64());
            else {
                auto compressed = qCompress(bytes);
                qToLittleEndian(quint32(bytes.size()), compressed.data());
                serialized = QStringLiteral("[ZIP") + QString::fromLatin1(compressed.toHex().toUpper()) + u']';
            }
        }
        if (serialized != original) editor->changeSelectedProperty(key, serialized);
        return;
    }
    if (propertyType != 3) {
        QDialog dialog(this);
        dialog.setObjectName("stringsEditor");
        dialog.setWindowTitle(trMain("Strings editor"));
        dialog.resize(570, 390);
        auto *layout = new QVBoxLayout(&dialog);
        auto *tools = new QHBoxLayout;
        layout->addLayout(tools);
        auto *input = new QPlainTextEdit(&dialog);
        input->setObjectName("stringsValue");
        input->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
        input->setLineWrapMode(QPlainTextEdit::NoWrap);
        QStringList lines;
        const bool encoded = decodeSchemeLines(original, lines);
        auto text = encoded ? lines.join(u'\n') : original;
        if (!encoded && propertyType == 2 && text.size() >= 2 && text.startsWith(u'"') && text.endsWith(u'"'))
            text = text.mid(1, text.size() - 2);
        else if (!encoded && !nativePackRoots().contains(editorPackRoot(pack))) text.replace("#13#10", "\n");
        input->setPlainText(text);
        const auto initialText = input->toPlainText();

        const auto tool = [&](const QString &label, QStyle::StandardPixmap icon) {
            auto *button = new QToolButton(&dialog);
            button->setIcon(dialog.style()->standardIcon(icon));
            button->setToolTip(label);
            button->setAccessibleName(label);
            tools->addWidget(button);
            return button;
        };
        auto *clear = tool(trMain("Clear text"), QStyle::SP_FileIcon);
        auto *open = tool(trMain("Open text file"), QStyle::SP_DialogOpenButton);
        auto *save = tool(trMain("Save text file"), QStyle::SP_DialogSaveButton);
        connect(clear, &QToolButton::clicked, input, [input] {
            auto cursor = input->textCursor();
            cursor.select(QTextCursor::Document);
            cursor.removeSelectedText();
        });
        connect(open, &QToolButton::clicked, &dialog, [&] {
            const auto path = QFileDialog::getOpenFileName(&dialog, trMain("Open text file"));
            if (path.isEmpty()) return;
            QFile file(path);
            if (!file.open(QIODevice::ReadOnly)) {
                QMessageBox::warning(&dialog, trMain("Open text file"), file.errorString());
                return;
            }
            auto loaded = decodePackageText(file.readAll());
            loaded.replace("\r\n", "\n").replace(u'\r', u'\n');
            auto cursor = input->textCursor();
            cursor.select(QTextCursor::Document);
            cursor.insertText(loaded);
        });
        connect(save, &QToolButton::clicked, &dialog, [&] {
            const auto path = QFileDialog::getSaveFileName(&dialog, trMain("Save text file"));
            if (path.isEmpty()) return;
            QSaveFile file(path);
            const auto bytes = input->toPlainText().toUtf8();
            if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
                QMessageBox::warning(&dialog, trMain("Save text file"), file.errorString());
        });
        auto *syntax = new QComboBox(&dialog);
        syntax->setObjectName("stringsSyntax");
        syntax->setAccessibleName(trMain("Syntax highlighting"));
        syntax->addItems({trMain("None"), "C++", "Delphi", "PHP", "Basic", "JavaScript", "ASM", "HWS", "SHA", "MySQL", "INI"});
        tools->addWidget(syntax);
        tools->addStretch();
        auto *highlighter = new StringsHighlighter(input->document());
        connect(syntax, &QComboBox::currentIndexChanged, &dialog, [highlighter](int i) { highlighter->setLanguage(i); });
        layout->addWidget(input, 1);

        auto *bottom = new QHBoxLayout;
        layout->addLayout(bottom);
        auto *status = new QLabel(&dialog);
        status->setTextFormat(Qt::PlainText);
        bottom->addWidget(status, 1);
        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
        bottom->addWidget(buttons);
        connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
        const auto updateStatus = [&] {
            const auto cursor = input->textCursor();
            status->setText(trMain("Lines: %1   Cursor: %2:%3   Editing: %4")
                .arg(input->document()->isEmpty() ? 0 : input->blockCount())
                .arg(cursor.blockNumber() + 1).arg(cursor.positionInBlock() + 1).arg(key));
        };
        connect(input, &QPlainTextEdit::textChanged, &dialog, updateStatus);
        connect(input, &QPlainTextEdit::cursorPositionChanged, &dialog, updateStatus);
        updateStatus();
        input->setFocus();
        if (dialog.exec() != QDialog::Accepted || !editor || currentEditor() != editor || revision != propertyRevision_) return;
        const auto edited = input->toPlainText();
        if (edited == initialText) return;
        const auto editedLines = edited.split(u'\n');
        QString serialized;
        if (propertyType == 2 && editedLines.size() == 1) {
            serialized = u'"' + edited + u'"';
        } else {
            serialized = QStringLiteral("#");
            if (!edited.isEmpty()) for (const auto &line : editedLines)
                serialized += QString::number(line.toUcs4().size()) + u':' + line + u'|';
        }
        editor->changeSelectedProperty(key, serialized);
        return;
    }
    const QStringList formats{"Null", "Integer", "String", "Real"};
    const auto opening = original.indexOf(u'(');
    int initial = original.isEmpty() ? 0 : -1;
    if (opening > 0 && original.endsWith(u')')) initial = formats.indexOf(original.left(opening));
    const auto payload = initial > 0 ? original.mid(opening + 1, original.size() - opening - 2) :
        initial < 0 ? original : QString();

    QDialog dialog(this);
    dialog.setObjectName("dataEditor");
    dialog.setWindowTitle(trMain("Data editor"));
    auto *layout = new QVBoxLayout(&dialog);
    auto *types = new QHBoxLayout;
    layout->addLayout(types);
    auto *choices = new QButtonGroup(&dialog);
    const QStringList labels{QStringLiteral("NULL"), trMain("Integer"), trMain("String"), trMain("Real"), trMain("Raw value")};
    const bool showRaw = initial < 0 || (pack != "cpp" && pack != "delphi" && pack != "_base");
    for (int i = 0; i < (showRaw ? labels.size() : formats.size()); ++i) {
        auto *choice = new QToolButton(&dialog);
        choice->setText(labels.at(i));
        choice->setCheckable(true);
        choices->addButton(choice, i);
        types->addWidget(choice);
    }
    choices->button(initial < 0 ? formats.size() : initial)->setChecked(true);
    auto *input = new QLineEdit(payload, &dialog);
    input->setObjectName("dataValue");
    layout->addWidget(input);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    const auto validate = [&] {
        const int type = choices->checkedId();
        bool valid = true;
        if (type == 1) input->text().toInt(&valid);
        if (type == 3) {
            auto locale = QLocale::c();
            locale.setNumberOptions(QLocale::RejectGroupSeparator);
            const auto value = locale.toDouble(input->text(), &valid);
            valid = valid && std::isfinite(value);
        }
        input->setEnabled(type != 0);
        buttons->button(QDialogButtonBox::Ok)->setEnabled(valid);
    };
    connect(choices, &QButtonGroup::idClicked, &dialog, validate);
    connect(input, &QLineEdit::textChanged, &dialog, validate);
    validate();
    if (dialog.exec() != QDialog::Accepted || !editor || currentEditor() != editor || revision != propertyRevision_) return;
    const int type = choices->checkedId();
    auto text = input->text();
    if (type == 0) text = original == "Null()" ? original : QString();
    else if (type < formats.size()) text = formats.at(type) + u'(' + text + u')';
    if (text != original) editor->changeSelectedProperty(key, text);
}

void HiasmQtMainWindow::updateProperties() {
    ++propertyRevision_;
    const QSignalBlocker updates(ui_.properties);
    const auto selectedName = ui_.properties->currentItem() ? ui_.properties->currentItem()->text(0) : QString();
    const auto scroll = ui_.properties->verticalScrollBar()->value();
    ui_.actionFormEdit->setEnabled(currentEditor() != nullptr);
    ui_.actionFormEdit->setChecked(currentEditor() && currentEditor()->isFormView());
    ui_.properties->clear();
    propertyDescription_->clear();
    if (const auto *editor = currentEditor()) {
        const auto rows = editor->selectedProperties();
        if (rows.isEmpty()) return;
        const auto pack = editor->packName();
        const auto packRoot = editorPackRoot(pack);
        const auto config = elementConfig(packRoot, pack, rows.first().second);
        const bool legacyColors = !nativePackRoots().contains(packRoot);
        QHash<QString,ConfigProperty> properties;
        for (const auto &p : config.properties) properties.insert(QString::fromUtf8(p.name).toCaseFolded(), p);
        const bool points = propertyPoints_->isChecked();
        ui_.properties->setColumnHidden(1, points);
        const auto checkbox = [&](QTreeWidgetItem *item, const QString &point) {
            item->setFlags(item->flags() | Qt::ItemIsUserCheckable);
            item->setCheckState(0, editor->selectedPointEnabled(point) ? Qt::Checked : Qt::Unchecked);
            item->setData(0, Qt::UserRole, point);
        };
        if (points) {
            static const char *icons[] = {"work", "event", "variable", "data"};
            for (const auto &port : config.ports) {
                if (!port.optional) continue;
                const auto name = QString::fromUtf8(port.name);
                auto *item = new QTreeWidgetItem(ui_.properties, {name});
                if (port.kind >= 1 && port.kind <= 4)
                    item->setIcon(0, QIcon(QStringLiteral(":/hiasm/points/%1.png").arg(QString::fromLatin1(icons[port.kind - 1]))));
                item->setToolTip(0, QString::fromUtf8(port.description));
                checkbox(item, name);
            }
        } else {
            const auto revision = propertyRevision_;
            const auto commit = [this, revision](const QString &name, const QString &value) {
                QTimer::singleShot(0, this, [this, revision, name, value] {
                    if (revision == propertyRevision_ && currentEditor()) currentEditor()->changeSelectedProperty(name, value);
                });
            };
            QHash<QString, QTreeWidgetItem *> groups;
            const auto group = [&](const QString &label, const QString &description = {}) {
                if (auto *existing = groups.value(label)) return existing;
                auto *item = new QTreeWidgetItem(ui_.properties, {label});
                item->setToolTip(0, description);
                item->setExpanded(true);
                groups.insert(label, item);
                return item;
            };
            const auto addProperty = [&](const QString &name, const QString &value, const ConfigProperty &p,
                                         QTreeWidgetItem *parent = nullptr) {
                const auto label = name.startsWith('@') ? name.mid(1) : name;
                auto *item = parent ? new QTreeWidgetItem(parent, {label, value}) : new QTreeWidgetItem(ui_.properties, {label, value});
                item->setToolTip(0, QString::fromUtf8(p.description));
                if (p.primary) {
                    auto font = item->font(0);
                    font.setBold(true);
                    item->setFont(0, font);
                }
                if (p.setter) checkbox(item, QStringLiteral("do") + name);
                if (p.type == 3 || p.type == 5 || p.type == 19 || imageProperty(p.type)) {
                    const bool data = p.type == 3;
                    const bool image = imageProperty(p.type);
                    const bool icon = p.type == 6 || name.compare("Icon", Qt::CaseInsensitive) == 0;
                    item->setText(1, data ? trMain("[Data]") : image ? (icon ? trMain("[Icon]") : trMain("[Image]")) : trMain("[List]"));
                    if (data) item->setToolTip(1, value);
                    auto *cell = new QWidget(ui_.properties);
                    auto *layout = new QHBoxLayout(cell);
                    layout->setContentsMargins(2, 0, 0, 0);
                    layout->addWidget(new QLabel(item->text(1), cell), 1);
                    auto *button = new QToolButton(cell);
                    button->setObjectName(data ? "editDataProperty" : image ? "editImageProperty" : "editStringsProperty");
                    button->setText(QStringLiteral("..."));
                    button->setFixedSize(button->sizeHint().height(), button->sizeHint().height());
                    button->setToolTip(data ? trMain("Data editor") : image ? (icon ? trMain("Icon") : trMain("Picture editor")) : trMain("Strings editor"));
                    layout->addWidget(button);
                    connect(button, &QToolButton::clicked, this, [this, name] { editProperty(name); });
                    ui_.properties->setItemWidget(item, 1, cell);
                    return;
                }
                if (p.type == 4 || p.type == 14) {
                    if (!p.choices.isEmpty()) {
                        auto *combo = new QComboBox(ui_.properties);
                        combo->setSizeAdjustPolicy(QComboBox::AdjustToMinimumContentsLengthWithIcon);
                        combo->setAccessibleName(label);
                        for (const auto &choice : p.choices) combo->addItem(QString::fromUtf8(choice));
                        bool number = false;
                        const int index = value.toInt(&number);
                        combo->setCurrentIndex(number ? index : combo->findText(value, Qt::MatchFixedString));
                        if (combo->currentIndex() >= 0) item->setText(1, combo->currentText());
                        connect(combo, &QComboBox::activated, this, [commit, name](int i) { commit(name, QString::number(i)); });
                        ui_.properties->setItemWidget(item, 1, combo);
                        return;
                    }
                }
                auto *input = new QLineEdit(ui_.properties);
                input->setAccessibleName(label);
                input->setFrame(false);
                if (p.type == 1) input->setValidator(new QIntValidator(input));
                if (p.type == 7) {
                    auto *validator = new QDoubleValidator(input);
                    validator->setLocale(QLocale::c());
                    input->setValidator(validator);
                }
                if ((p.type == 2 || p.type == 20) && value.size() >= 2 && value.startsWith(u'"') && value.endsWith(u'"')) {
                    input->setText(value.mid(1, value.size() - 2));
                } else input->setText(value);
                if (p.type == 2) {
                    QStringList lines;
                    if (decodeSchemeLines(value, lines)) {
                        input->setText(lines.value(0));
                        input->setToolTip(lines.join(u'\n'));
                        input->setReadOnly(lines.size() > 1);
                    }
                    auto *action = input->addAction(style()->standardIcon(QStyle::SP_FileDialogDetailedView), QLineEdit::TrailingPosition);
                    action->setObjectName("editTextProperty");
                    action->setToolTip(trMain("Strings editor"));
                    connect(action, &QAction::triggered, this, [this, name] { editProperty(name); });
                }
                if (p.type == 8) {
                    QPixmap swatch(12, 12);
                    swatch.fill(propertyColor(value, input->palette()));
                    auto *action = input->addAction(QIcon(swatch), QLineEdit::LeadingPosition);
                    action->setObjectName("choosePropertyColor");
                    action->setToolTip(trMain("Choose color"));
                    connect(action, &QAction::triggered, this, [this, commit, input, name, value, legacyColors] {
                        const QPointer<QLineEdit> field(input);
                        const auto current = propertyColor(input->text(), input->palette());
                        input->setProperty("choosingColor", true);
                        QColorDialog dialog(current, this);
                        dialog.setWindowTitle(trMain("Choose color"));
                        dialog.setOption(QColorDialog::DontUseNativeDialog);
                        const bool accepted = dialog.exec() == QDialog::Accepted;
                        if (!field) return;
                        field->setProperty("choosingColor", false);
                        if (!accepted || !dialog.selectedColor().isValid()) return;
                        const auto color = dialog.selectedColor();
                        if (color == current && !field->isModified()) return;
                        const auto serialized = legacyColors
                            ? QString::number(color.red() | (color.green() << 8) | (color.blue() << 16))
                            : color.name();
                        field->setModified(false);
                        if (serialized != value) commit(name, serialized);
                    });
                }
                connect(input, &QLineEdit::editingFinished, this, [commit, name, value, input, type = p.type] {
                    if (input->property("choosingColor").toBool() || !input->isModified() || !input->hasAcceptableInput()) return;
                    auto text = input->text();
                    if (type == 2 || type == 20) {
                        text = u'"' + text + u'"';
                    }
                    if (text != value) commit(name, text);
                });
                ui_.properties->setItemWidget(item, 1, input);
            };
            for (int i = 0; i < SchemeEditor::firstPropertyRow; ++i) {
                const auto &[name, value] = rows.at(i);
                auto *item = new QTreeWidgetItem(ui_.properties, {name, value});
                item->setHidden(editor->isFormView() && i >= 2);
            }
            auto saved = rows.mid(SchemeEditor::firstPropertyRow);
            for (const auto &p : config.properties) {
                const auto name = QString::fromUtf8(p.name);
                const auto found = std::find_if(saved.cbegin(), saved.cend(), [&](const auto &row) {
                    return row.first.compare(name, Qt::CaseInsensitive) == 0;
                });
                const auto value = found == saved.cend() ? QString::fromUtf8(p.initial) : found->second;
                const auto separator = p.group.indexOf('=');
                auto *parent = p.group.isEmpty() ? nullptr : group(QString::fromUtf8(p.group.left(separator)),
                    separator < 0 ? QString() : QString::fromUtf8(p.group.mid(separator + 1)));
                addProperty(name, value, p, parent);
            }
            for (const auto &[name, value] : saved)
                if (!name.startsWith('@') && !properties.contains(name.toCaseFolded())) addProperty(name, value, {});
            if (propertySystem_->isChecked()) {
                auto *system = group(trMain("System properties"), trMain("Internal element properties"));
                const auto systemProperty = [&](const QString &name, int type, const QString &fallback, const QString &description) {
                    const auto found = std::find_if(saved.cbegin(), saved.cend(), [&](const auto &row) { return row.first.compare(name, Qt::CaseInsensitive) == 0; });
                    ConfigProperty p;
                    p.type = type;
                    p.description = description.toUtf8();
                    addProperty(name, found == saved.cend() ? fallback : found->second, p, system);
                };
                systemProperty("@Hint", 5, {}, trMain("Element description"));
                systemProperty("@Group", 1, "0", trMain("Selection group identifier"));
                systemProperty("@Color", 8, {}, trMain("Element color"));
                for (const auto &[name, value] : saved)
                    if (name.startsWith('@') && name != "@Hint" && name != "@Group" && name != "@Color") addProperty(name, value, {}, system);
            }
        }
        const auto matches = ui_.properties->findItems(selectedName, Qt::MatchExactly | Qt::MatchRecursive);
        if (!matches.isEmpty()) {
            ui_.properties->setCurrentItem(matches.first());
            propertyDescription_->setText(matches.first()->toolTip(0));
        }
        ui_.properties->verticalScrollBar()->setValue(scroll);
    }
}

int HiasmQtMainWindow::editorCount() const { return ui_.tabs->count(); }
int HiasmQtMainWindow::dockCount() const { return findChildren<QDockWidget *>().size(); }

void HiasmQtMainWindow::newScheme() {
    ProjectManagerDialog dialog(ProjectManagerDialog::projectCatalog(), this);
    if (dialog.exec() == QDialog::Accepted)
        createFromTemplate(dialog.selectedTemplate());
}

void HiasmQtMainWindow::createFromTemplate(const QString &templatePath) {
    QFile file(templatePath);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const auto contents = file.readAll();
        if (file.error() == QFileDevice::NoError && !contents.isEmpty()) {
            addEditor(new SchemeEditor(decodePackageText(contents), {}, {}, this));
            return;
        }
    }
    QMessageBox::warning(this, trMain("New"), trMain("The project template could not be opened."));
}

bool HiasmQtMainWindow::openFromPanel(const QString &path) {
    for (int i = 0; i < ui_.tabs->count(); ++i) {
        const auto *editor = qobject_cast<SchemeEditor *>(ui_.tabs->widget(i));
        if (editor && !editor->fileName().isEmpty() && QFileInfo(editor->fileName()) == QFileInfo(path)) {
            ui_.tabs->setCurrentIndex(i);
            return true;
        }
    }
    return openScheme(path);
}

bool HiasmQtMainWindow::openScheme(const QString &fileName) {
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly)) return false;
    const auto contents = file.readAll();
    if (file.error() != QFileDevice::NoError) return false;
    const auto format = schemeFileFormat(contents);
    if ((format.utf8Bom && format.windows1251) || contents.contains('\0')) return false;
    addEditor(new SchemeEditor(decodePackageText(contents), QFileInfo(fileName).absoluteFilePath(), format, this));
    return true;
}

void HiasmQtMainWindow::addEditor(SchemeEditor *editor) {
    const auto fileName = editor->fileName();
    connect(editor, &SchemeEditor::selectionChanged, this, &HiasmQtMainWindow::updateProperties);
    connect(editor, &SchemeEditor::primaryPropertyRequested, this, [this] { editProperty(); });
    connect(editor->document(), &QTextDocument::modificationChanged, this, &HiasmQtMainWindow::updateWindowTitle);
    connect(editor->document(), &QTextDocument::contentsChanged, this, &HiasmQtMainWindow::updatePalette);
    connect(editor->document(), &QTextDocument::contentsChanged, this, &HiasmQtMainWindow::updateBuildSettings);
    const auto name = fileName.isEmpty() ? trMain(kUntitled) : QFileInfo(fileName).fileName();
    const int index = ui_.tabs->addTab(editor, name);
    ui_.tabs->setCurrentIndex(index);
    updateWindowTitle();
}

void HiasmQtMainWindow::openSchemeDialog() {
    const auto fileName = QFileDialog::getOpenFileName(this, trMain("Open scheme"), QString(),
        trMain("HiAsm schemes (*.sha);;All files (*)"));
    if (!fileName.isEmpty() && !openScheme(fileName))
        QMessageBox::warning(this, trMain("Open scheme"), trMain("The scheme could not be opened."));
}

bool HiasmQtMainWindow::saveCurrent() {
    auto *editor = currentEditor();
    if (!editor) return false;
    return editor->fileName().isEmpty() || isInLegacyInstallation(editor->fileName())
        ? saveEditorAs(editor) : saveEditor(editor, editor->fileName());
}

bool HiasmQtMainWindow::saveEditor(SchemeEditor *editor, const QString &fileName, bool copy) {
    if (isInLegacyInstallation(fileName)) {
        QMessageBox::warning(this, trMain("Save scheme"), trMain("Save the scheme outside the HiAsm 4 installation."));
        return false;
    }
    const auto contents = editor->encodedSource();
    if (!writeSchemeFile(fileName, contents)) {
        QMessageBox::warning(this, trMain("Save scheme"), trMain("The scheme could not be saved."));
        return false;
    }
    // An explicitly saved file must survive even if the user chose a draft directory.
    const auto savedPath = QFileInfo(fileName).canonicalFilePath();
    for (auto work = draftWorkspaces_.begin(); work != draftWorkspaces_.end();) {
        const auto root = QDir(work.value()->path()).canonicalPath();
        const auto relative = QDir(root).relativeFilePath(savedPath);
        if (!root.isEmpty() && !savedPath.isEmpty() && !QDir::isAbsolutePath(relative) &&
            relative != ".." && !relative.startsWith("../")) {
            work.value()->setAutoRemove(false);
            work = draftWorkspaces_.erase(work);
        } else {
            ++work;
        }
    }
    if (copy) {
        statusBar()->showMessage(trMain("Scheme saved"), 3000);
        return true;
    }
    editor->setSavedFile(fileName, schemeFileFormat(contents));
    draftWorkspaces_.remove(editor);
    ui_.tabs->setTabText(ui_.tabs->indexOf(editor), QFileInfo(fileName).fileName());
    editor->document()->setModified(false);
    updateWindowTitle();
    updateProjectPanels();
    statusBar()->showMessage(trMain("Scheme saved"), 3000);
    return true;
}

bool HiasmQtMainWindow::saveCurrentAs() {
    auto *editor = currentEditor();
    return editor && saveEditorAs(editor);
}

bool HiasmQtMainWindow::saveEditorAs(SchemeEditor *editor) {
    const auto seed = editor->fileName().isEmpty() ? QStringLiteral("Project.sha") : QFileInfo(editor->fileName()).fileName();
    const auto directory = isInLegacyInstallation(editor->fileName()) ? QDir::home() : QDir::current();
    const auto fileName = QFileDialog::getSaveFileName(this, trMain("Save scheme"), directory.filePath(seed),
        trMain("HiAsm schemes (*.sha);;All files (*)"));
    return !fileName.isEmpty() && saveEditor(editor, fileName);
}

void HiasmQtMainWindow::saveScheme() { saveCurrent(); }
void HiasmQtMainWindow::closeCurrent() { closeEditor(ui_.tabs->currentIndex()); }

bool HiasmQtMainWindow::confirmClose(SchemeEditor *editor) {
    if (!editor->document()->isModified())
        return true;
    const auto name = ui_.tabs->tabText(ui_.tabs->indexOf(editor));
    const auto answer = QMessageBox::question(this, trMain("Close scheme"),
        trMain("Save changes to %1 before closing?").arg(name),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);
    if (answer == QMessageBox::Save) {
        return editor->fileName().isEmpty() || isInLegacyInstallation(editor->fileName())
            ? saveEditorAs(editor) : saveEditor(editor, editor->fileName());
    }
    return answer == QMessageBox::Discard;
}

void HiasmQtMainWindow::closeEditor(int index) {
    auto *editor = qobject_cast<SchemeEditor *>(ui_.tabs->widget(index));
    if (editor && confirmClose(editor)) {
        draftWorkspaces_.remove(editor);
        ui_.tabs->removeTab(index);
        delete editor;
    }
}

void HiasmQtMainWindow::closeEvent(QCloseEvent *event) {
    if (process_ && !ui_.actionStop->isEnabled()) {
        // Let the compiler helper reap CMake and its compiler children first.
        closeAfterBuild_ = true;
        event->ignore();
        return;
    }
    closeAfterBuild_ = false;
    for (int i = 0; i < ui_.tabs->count(); ++i) {
        auto *editor = qobject_cast<SchemeEditor *>(ui_.tabs->widget(i));
        if (editor && !confirmClose(editor)) {
            event->ignore();
            return;
        }
    }
    if (process_) {
        process_->disconnect(this);
        process_->kill();
        process_->waitForFinished(3000);
    }
    QMainWindow::closeEvent(event);
}

void HiasmQtMainWindow::runCurrent() {
    startBuild(true);
}

void HiasmQtMainWindow::buildCurrent() {
    startBuild(false);
}

void HiasmQtMainWindow::updateProjectPanels() {
    auto *editor = currentEditor();
    const auto path = editor ? editor->fileName() : QString();
    repository_->setProject(path, editor != nullptr);
    terminal_->setProjectDirectory(path.isEmpty() ? QString() : QFileInfo(path).absolutePath());
    updateBuildSettings();
}

void HiasmQtMainWindow::updateBuildSettings() {
    const auto *editor = currentEditor();
    const auto pack = editor ? editor->packName() : QString();
    const auto mode = editor ? editor->buildMode() : std::nullopt;
    const bool native = pack == "cpp" || pack == "_base" || pack == "web" || pack == "modules";
    buildPackage_->setText(pack.isEmpty() ? trMain("No package selected") :
        trMain("Package: %1").arg(pack) + (native ? QString() :
        '\n' + trMain("Building %1 requires the HiAsm 4 compiler bridge.").arg(pack)));
    buildMode_->setEnabled(mode.has_value());
    buildMode_->setCurrentIndex(mode ? qBound(0, *mode, 1) : -1);
    buildButton_->setEnabled(editor && !process_ && !pack.isEmpty());
}

void HiasmQtMainWindow::showBuildError(const QString &scheme, const QString &message, int line) {
    const auto name = scheme.isEmpty() ? trMain(kUntitled) : QFileInfo(scheme).fileName();
    auto *item = new QTreeWidgetItem(ui_.errors, {name,
        line > 0 ? QString::number(line) : QString(), message});
    item->setData(0, Qt::UserRole, scheme);
    item->setToolTip(2, message);
    ui_.errorsDock->show();
    ui_.errorsDock->raise();
}

void HiasmQtMainWindow::startBuild(bool run) {
    auto *editor = currentEditor();
    if (process_ || !editor) return;
    ui_.errors->clear();
    buildErrorEditor_ = editor;
    const auto pack = editor->packName();
    const bool externalSource = isInLegacyInstallation(editor->fileName());
    if (!editor->fileName().isEmpty() && !externalSource && editor->document()->isModified() && !saveCurrent()) return;
    const auto fileName = editor->fileName();
    const auto diagram = readSchemeDiagram(editor->source());
    if (diagram.errorLine) {
        showBuildError(fileName, trMain("Invalid scheme."), diagram.errorLine);
        return;
    }
    LegacyProject legacy;
    try {
        legacy = legacyProject(pack, diagram);
    } catch (const std::exception &e) {
        showBuildError(fileName, QString::fromUtf8(e.what()));
        return;
    }
    const bool legacyBuild = !legacy.directory.isEmpty();
    std::shared_ptr<QTemporaryDir> workspace;
    QString input = fileName;
    if (input.isEmpty() || externalSource) {
        workspace = draftWorkspaces_.value(editor);
        if (!workspace) {
            workspace = std::make_shared<QTemporaryDir>(QDir::temp().filePath("hiasm-project-XXXXXX"));
            if (!workspace->isValid()) {
                showBuildError(fileName, trMain("Could not create the temporary project directory."));
                return;
            }
            draftWorkspaces_.insert(editor, workspace);
        }
        input = workspace->filePath(externalSource ? QFileInfo(fileName).completeBaseName() + ".sha" : QStringLiteral("Project.sha"));
        if (!writeSchemeFile(input, editor->encodedSource())) {
            showBuildError(fileName, trMain("Could not write the temporary scheme."));
            return;
        }
    }
    const QFileInfo scheme(input);
    QDir directory = scheme.absoluteDir();
    const bool webWidgets = pack == "cpp" && schemeWebWidgets(diagram).needsWebEngine();
#ifdef Q_OS_WIN
    const QString webBuildDir = QStringLiteral("build/qt-webview2");
#else
    const QString webBuildDir = QStringLiteral("build/qt-webengine");
#endif
    const QString buildDir = legacyBuild ? QStringLiteral("build")
        : webWidgets ? webBuildDir : QStringLiteral("build/qt-native");
    if (isInLegacyInstallation(directory.filePath(buildDir)) || !directory.mkpath(buildDir) || !directory.cd(buildDir)) {
        showBuildError(fileName, trMain("Could not create the build directory."));
        return;
    }
    ProjectType type = ProjectType::Unknown;
    if (!diagram.layers.empty())
        for (const auto &e : diagram.layers.first().elements)
            if ((type = projectType(e.name, pack)) != ProjectType::Unknown) break;
    const auto output = directory.absoluteFilePath(scheme.completeBaseName() + (legacyBuild ? '.' + legacy.suffix : artifactSuffix(type)));
    const auto builder = QCoreApplication::applicationDirPath() + "/hiasm" + artifactSuffix(ProjectType::Console);
    const bool standalone = legacyBuild ? legacy.suffix.compare("exe", Qt::CaseInsensitive) == 0 &&
        legacy.entry != "Service" && legacy.entry != "NTService" :
        !textArtifact(type) && !sharedArtifact(type) && type != ProjectType::WindowsService;
    ui_.debugDock->show();
    ui_.debugDock->raise();
    ui_.debugOutput->clear();
    if (run && !standalone) ui_.debugOutput->appendPlainText(trMain("This project builds an artifact for an external host."));
    startProcess(builder, {"--build", scheme.absoluteFilePath(), output}, legacyBuild ? directory.absolutePath() : scheme.absolutePath(),
                 fileName, run && standalone ? output : QString(), workspace);
}

void HiasmQtMainWindow::startProcess(const QString &program, const QStringList &args,
                                     const QString &directory, std::optional<QString> scheme, const QString &runAfter,
                                     const std::shared_ptr<QTemporaryDir> &workspace) {
    auto *p = new QProcess(this);
    // Keep the working directory alive until the process stops, even if the draft tab closes.
    if (workspace)
        connect(p, &QObject::destroyed, [keep = workspace]() mutable { keep.reset(); });
    process_ = p;
    const bool building = scheme.has_value();
    ui_.actionStop->setEnabled(!building);
    updateWindowTitle();
    updateBuildSettings();
    p->setWorkingDirectory(directory);
    auto env = QProcessEnvironment::systemEnvironment();
    const QString appDir = QCoreApplication::applicationDirPath();
    env.insert("PATH", appDir + QDir::listSeparator() + env.value("PATH"));
    const auto programDir = QFileInfo(program).absolutePath();
    if (!building && QDir(programDir + "/platforms").exists()) env.insert("QT_PLUGIN_PATH", programDir);
    else if (QDir(appDir + "/platforms").exists()) env.insert("QT_PLUGIN_PATH", appDir);
    const auto compiler = findChild<QLineEdit *>("buildCompiler")->text().trimmed();
    if (building && !compiler.isEmpty()) env.insert("CXX", compiler);
    p->setProcessEnvironment(env);
    auto *log = building ? ui_.debugOutput : terminal_->showRunOutput();
    if (!building) {
        ui_.terminalDock->show();
        ui_.terminalDock->raise();
    }
    const auto diagnostics = std::make_shared<QString>();
    const auto append = [log](const QString &text) {
        auto cursor = log->textCursor();
        cursor.movePosition(QTextCursor::End);
        cursor.insertText(text);
        log->setTextCursor(cursor);
        log->ensureCursorVisible();
    };
    const auto stdoutDecoder = std::make_shared<QStringDecoder>(QStringDecoder::Utf8);
    const auto stderrDecoder = std::make_shared<QStringDecoder>(QStringDecoder::Utf8);
    const auto buildOutput = std::make_shared<QString>();
    connect(p, &QProcess::readyReadStandardOutput, this, [p, append, stdoutDecoder, buildOutput, building] {
        const QString output = (*stdoutDecoder)(p->readAllStandardOutput());
        append(output);
        if (building) *buildOutput = (*buildOutput + output).right(65536);
    });
    connect(p, &QProcess::readyReadStandardError, this, [p, append, stderrDecoder, diagnostics, building] {
        const QString output = (*stderrDecoder)(p->readAllStandardError());
        append(output);
        if (building) *diagnostics = (*diagnostics + output).right(65536);
    });
    auto clearProcess = [this, p] {
        process_ = nullptr;
        ui_.actionStop->setEnabled(false);
        p->deleteLater();
        updateWindowTitle();
        updateBuildSettings();
        if (closeAfterBuild_) QMetaObject::invokeMethod(this, &QWidget::close, Qt::QueuedConnection);
    };
    connect(p, &QProcess::errorOccurred, this, [this, p, log, scheme, clearProcess](QProcess::ProcessError error) {
        log->appendPlainText(p->errorString());
        if (error != QProcess::FailedToStart) return;
        if (scheme) showBuildError(*scheme, p->errorString());
        clearProcess();
    });
    connect(p, &QProcess::finished, this, [this, log, directory, scheme, runAfter, workspace, diagnostics, buildOutput, clearProcess](int code, QProcess::ExitStatus status) {
        clearProcess();
        const auto exitMessage = trMain("Process exited with code %1.").arg(code);
        log->appendPlainText(exitMessage);
        if (code != 0 || status != QProcess::NormalExit) {
            if (!scheme) return;
            auto messages = diagnostics->trimmed().split(u'\n', Qt::SkipEmptyParts);
            static const QRegularExpression diagnostic(QStringLiteral(R"((?:\berror:|^CMake Error\b|^FAILED:))"));
            for (const auto &line : buildOutput->split(u'\n'))
                if (diagnostic.match(line).hasMatch() && !messages.contains(line.trimmed())) messages.append(line.trimmed());
            if (messages.isEmpty()) showBuildError(*scheme, exitMessage);
            for (const auto &message : messages) {
                const auto trimmed = message.trimmed();
                if (!trimmed.isEmpty()) showBuildError(*scheme, trimmed);
            }
        } else if (!closeAfterBuild_ && !runAfter.isEmpty()) {
            startProcess(runAfter, {}, directory, std::nullopt, {}, workspace);
        }
    });
    p->start(program, args);
}

void HiasmQtMainWindow::showAbout() {
    QDialog dialog(this);
    Ui::AboutDialog about;
    about.setupUi(&dialog);
    about.programName->setText(QStringLiteral("HiAsm " HIASM_QT_VERSION));
    about.buttons->button(QDialogButtonBox::Close)->setText(trMain("Close"));
    QFile license(QStringLiteral(":/hiasm/license"));
    if (license.open(QIODevice::ReadOnly | QIODevice::Text))
        about.licenseText->setPlainText(QString::fromUtf8(license.readAll()));
    dialog.exec();
}

void HiasmQtMainWindow::updateWindowTitle() {
    const auto *editor = currentEditor();
    const auto fileName = editor ? editor->fileName() : QString();
    const auto name = fileName.isEmpty() ? trMain(kUntitled) : QFileInfo(fileName).fileName();
    setWindowTitle(name + (editor && editor->document()->isModified() ? QStringLiteral(" *") : QString()) + QStringLiteral(" - HiAsm"));
    if (saveAction_) saveAction_->setEnabled(editor != nullptr);
    ui_.actionSaveAs->setEnabled(editor != nullptr);
    if (closeAction_) closeAction_->setEnabled(editor != nullptr);
    if (runAction_) runAction_->setEnabled(editor && !process_);
    if (buildAction_) buildAction_->setEnabled(editor && !process_);
}
