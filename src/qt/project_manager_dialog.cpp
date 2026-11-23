#include "project_manager_dialog.h"

#include "package_paths.h"
#include <QCoreApplication>
#include <QDir>
#include <QJsonObject>
#include <QPainter>
#include <QPushButton>
#include <QStyledItemDelegate>

namespace {
QString trManager(const char *text) {
    return QCoreApplication::translate("ProjectManager", text);
}

class ProjectTileDelegate final : public QStyledItemDelegate {
public:
    using QStyledItemDelegate::QStyledItemDelegate;
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override {
        const auto rect = option.rect.adjusted(2, 2, -2, -2);
        const bool selected = option.state.testFlag(QStyle::State_Selected);
        painter->save();
        if (selected) {
            painter->setPen(Qt::NoPen);
            painter->setBrush(option.palette.highlight());
            painter->drawRoundedRect(rect, 3, 3);
        }
        const auto icon = qvariant_cast<QIcon>(index.data(Qt::DecorationRole));
        const int iconHeight = option.decorationSize.height();
        const int textHeight = option.fontMetrics.height();
        const int gap = 6;
        const int top = rect.center().y() - (iconHeight + gap + textHeight) / 2;
        icon.paint(painter, QRect(rect.left(), top, rect.width(), iconHeight), Qt::AlignCenter);
        const auto label = option.fontMetrics.elidedText(index.data().toString(), Qt::ElideRight, rect.width() - 4);
        painter->setFont(option.font);
        painter->setPen(option.palette.color(selected ? QPalette::HighlightedText : QPalette::Text));
        painter->drawText(QRect(rect.left(), top + iconHeight + gap, rect.width(), textHeight), Qt::AlignCenter, label);
        painter->restore();
    }
};

QString projectIcon(const QString &directory, const QString &entry, const QString &fallback) {
    if (!directory.isEmpty()) {
        for (const auto *suffix : {".ico", ".png"}) {
            const auto file = directory + "/icon/" + entry + suffix;
            if (QFileInfo(file).isFile()) return file;
        }
    }
    return fallback;
}

void addItem(QListWidget *list, const QJsonObject &item, const QString &label) {
    auto *tile = new QListWidgetItem(QIcon(item.value("icon").toString()), label, list);
    tile->setData(Qt::UserRole, item.value("id").toString());
    tile->setToolTip(label);
    tile->setSizeHint(list->gridSize() - QSize(4, 4));
}
}

ProjectManagerDialog::ProjectManagerDialog(const QJsonArray &catalog, QWidget *parent)
    : QDialog(parent), catalog_(catalog) {
    ui_.setupUi(this);
    ui_.packs->setItemDelegate(new ProjectTileDelegate(ui_.packs));
    ui_.projects->setItemDelegate(new ProjectTileDelegate(ui_.projects));
    setWindowIcon(QIcon(":/hiasm/logo.png"));
    ui_.splitter->setSizes({380, 190});
    ui_.splitter->setStretchFactor(0, 0);
    ui_.splitter->setStretchFactor(1, 1);

    auto *ok = ui_.buttons->button(QDialogButtonBox::Ok);
    ok->setText(trManager("OK"));
    ok->setEnabled(false);
    ui_.buttons->button(QDialogButtonBox::Cancel)->setText(trManager("Cancel"));
    connect(ui_.buttons, &QDialogButtonBox::accepted, this, &ProjectManagerDialog::accept);
    connect(ui_.buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(ui_.projects, &QListWidget::itemDoubleClicked, this, &ProjectManagerDialog::accept);
    connect(ui_.projects, &QListWidget::currentRowChanged, this, &ProjectManagerDialog::updateSelection);
    connect(ui_.packs, &QListWidget::currentRowChanged, this, &ProjectManagerDialog::updateProjects);

    int defaultPack = 0;
    for (int i = 0; i < catalog_.size(); ++i) {
        const auto pack = catalog_.at(i).toObject();
        const bool cpp = pack.value("id") == QStringLiteral("cpp");
        addItem(ui_.packs, pack, cpp ? trManager("C++ package") : pack.value("title").toString());
        if (cpp) defaultPack = i;
    }
    ui_.packs->setCurrentRow(defaultPack);
}

QJsonArray ProjectManagerDialog::projectCatalog() {
    const QJsonArray coreProjects{
        QJsonObject{{"id", "hcTplForm"}, {"title", trManager("Core form")},
                    {"template", ":/hiasm/templates/_base/hcTplForm.sha"}},
        QJsonObject{{"id", "hcTplDialog"}, {"title", trManager("Core dialog")},
                    {"template", ":/hiasm/templates/_base/hcTplDialog.sha"}}
    };
    QJsonArray cppProjects{
        QJsonObject{{"id", "Entry"}, {"title", trManager("Console application")},
                    {"template", ":/hiasm/templates/cpp/Entry.sha"}},
        QJsonObject{{"id", "MainForm"}, {"title", trManager("Form application")},
                    {"info", trManager("Qt 6.9 form application")},
                    {"template", ":/hiasm/templates/cpp/MainForm.sha"}}
    };
    const auto add = [&](const char *entry,const char *title) {
        cppProjects.append(QJsonObject{{"id",QString::fromLatin1(entry)},{"title",trManager(title)},
            {"template",QStringLiteral(":/hiasm/templates/cpp/%1.sha").arg(QString::fromLatin1(entry))}});
    };
    add("Library","Shared library");
    add("Daemon","Background process");
    add("Plugin","HiAsm plugin");
#ifdef Q_OS_WIN
    add("WinDll","Windows DLL");
    add("WinService","Windows service");
    add("CplApp","Control Panel applet");
    add("ComExt","COM object");
#endif
    QJsonArray native{
        QJsonObject{{"id", "_base"}, {"title", "Core"}, {"projects", coreProjects}},
        QJsonObject{{"id","web"},{"title","Web"},{"projects",QJsonArray{
            QJsonObject{{"title","HTML"},{"template",":/hiasm/templates/web/HtmlEntryPoint.sha"}},
            QJsonObject{{"title","CSS"},{"template",":/hiasm/templates/web/CSSEntryPoint.sha"}},
            QJsonObject{{"title","JavaScript"},{"template",":/hiasm/templates/web/JavaEntryPoint.sha"}},
            QJsonObject{{"title","PHP"},{"template",":/hiasm/templates/web/EntryPoint.sha"}}}}},
        QJsonObject{{"id","modules"},{"title",trManager("Modules")},{"projects",QJsonArray{
            QJsonObject{{"title","HIS"},{"template",":/hiasm/templates/modules/HISEntryPoint.sha"}},
            QJsonObject{{"title","C++"},{"template",":/hiasm/templates/modules/MakeEntryPoint.sha"}}}}},
        QJsonObject{{"id", "cpp"}, {"projects", cppProjects}}
    };
    QJsonArray catalog;
    for (const auto &value : native) {
        auto pack = value.toObject();
        const auto id = pack.value("id").toString();
        QString directory;
        for (const auto &root : nativePackRoots()) {
            const auto dir = QDir(root).filePath(id);
            if (QFileInfo(dir + "/icon.png").isFile()) { directory = dir; break; }
        }
        pack["icon"] = directory.isEmpty() ? QStringLiteral(":/hiasm/logo.png") : directory + "/icon.png";
        auto projects = pack.value("projects").toArray();
        for (qsizetype i = 0; i < projects.size(); ++i) {
            auto project = projects.at(i).toObject();
            const auto name = QFileInfo(project.value("template").toString()).completeBaseName();
            project["id"] = name;
            project["icon"] = projectIcon(directory, name, pack.value("icon").toString());
            projects.replace(i, project);
        }
        pack["projects"] = projects;
        catalog.append(pack);
    }
    for (const auto &dir : legacyPackDirectories()) {
        const auto id = dir.fileName().toLower();
        const auto icons = QDir(dir.filePath()).entryList({dir.fileName() + ".ico"}, QDir::Files);
        const auto icon = icons.isEmpty() ? QStringLiteral(":/hiasm/logo.png") : dir.filePath() + '/' + icons.first();
        const auto info = trManager("External HiAsm 4 package (read-only; building requires the legacy compiler bridge).");
        QJsonArray projects;
        const QDir templates(dir.filePath() + "/new");
        for (const auto &file : templates.entryInfoList({"*.sha"}, QDir::Files | QDir::Readable, QDir::Name)) {
            const auto name = file.completeBaseName();
            projects.append(QJsonObject{{"id",name},{"title",name},{"info",info},
                {"template",file.absoluteFilePath()},{"icon",projectIcon(dir.filePath(),name,icon)}});
        }
        catalog.append(QJsonObject{{"id",id},{"title","HiAsm 4: " + id},{"info",info},{"icon",icon},{"projects",projects}});
    }
    return catalog;
}

void ProjectManagerDialog::updateProjects(int row) {
    ui_.projects->clear();
    pack_ = row >= 0 ? catalog_.at(row).toObject() : QJsonObject{};
    projects_ = pack_.value("projects").toArray();

    int defaultProject = -1;
    for (int i = 0; i < projects_.size(); ++i) {
        const auto project = projects_.at(i).toObject();
        addItem(ui_.projects, project, project.value("title").toString());
        if (pack_.value("id") == QStringLiteral("cpp") && project.value("id") == QStringLiteral("MainForm"))
            defaultProject = i;
    }
    ui_.projects->setCurrentRow(defaultProject);
    updateSelection(ui_.projects->currentRow());
}

void ProjectManagerDialog::updateSelection(int row) {
    ui_.buttons->button(QDialogButtonBox::Ok)->setEnabled(row >= 0);
    const auto selected = row >= 0 ? projects_.at(row).toObject() : pack_;
    ui_.description->setPlainText(selected.value("info").toString());
}

int ProjectManagerDialog::projectRow() const { return ui_.projects->currentRow(); }

QString ProjectManagerDialog::selectedTemplate() const {
    const int row = projectRow();
    return row >= 0 ? projects_.at(row).toObject().value("template").toString() : QString{};
}

void ProjectManagerDialog::accept() {
    if (projectRow() >= 0)
        QDialog::accept();
}
