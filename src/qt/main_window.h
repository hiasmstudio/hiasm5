#pragma once

#include <QMainWindow>
#include <QHash>
#include <QPointer>
#include <QProcess>
#include <memory>
#include <optional>
#include "ui_MainWindow.h"

class SchemeEditor;
class TerminalPanel;
class RepositoryPanel;
class QLabel;
class QComboBox;
class QPushButton;
class QTemporaryDir;
class QToolButton;

class HiasmQtMainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit HiasmQtMainWindow(QWidget *parent = nullptr);
    bool openScheme(const QString &fileName);
    bool saveCurrent();
    bool saveCurrentAs();
    int editorCount() const;
    int dockCount() const;

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void newScheme();
    void openSchemeDialog();
    void saveScheme();
    void closeCurrent();
    void showAbout();
    void runCurrent();
    void buildCurrent();
    void updateWindowTitle();
    void updateProperties();
    void editProperty(const QString &name = {});
    void updatePalette();
    void updateBuildSettings();

private:
    void createActions();
    void createMenus();
    void createDocks();
    void addEditor(SchemeEditor *editor);
    void createFromTemplate(const QString &templatePath);
    SchemeEditor *currentEditor() const;
    bool saveEditor(SchemeEditor *editor, const QString &fileName, bool copy = false);
    bool saveEditorAs(SchemeEditor *editor);
    bool confirmClose(SchemeEditor *editor);
    void closeEditor(int index);
    void startBuild(bool run);
    void startProcess(const QString &program, const QStringList &args, const QString &directory,
                      std::optional<QString> scheme = {}, const QString &runAfter = {},
                      const std::shared_ptr<QTemporaryDir> &workspace = {});
    void showBuildError(const QString &scheme, const QString &message, int line = 0);
    void updateProjectPanels();
    bool openFromPanel(const QString &path);

    Ui::MainWindow ui_;
    QAction *saveAction_ = nullptr;
    QAction *closeAction_ = nullptr;
    QAction *runAction_ = nullptr;
    QAction *buildAction_ = nullptr;
    QProcess *process_ = nullptr;
    QHash<SchemeEditor *, std::shared_ptr<QTemporaryDir>> draftWorkspaces_;
    QPointer<SchemeEditor> buildErrorEditor_;
    bool closeAfterBuild_ = false;
    QString palettePack_;
    TerminalPanel *terminal_ = nullptr;
    RepositoryPanel *repository_ = nullptr;
    QLabel *buildPackage_ = nullptr;
    QComboBox *buildMode_ = nullptr;
    QPushButton *buildButton_ = nullptr;
    QToolButton *propertyPoints_ = nullptr;
    QToolButton *propertySystem_ = nullptr;
    QLabel *propertyDescription_ = nullptr;
    quint64 propertyRevision_ = 0;
};
