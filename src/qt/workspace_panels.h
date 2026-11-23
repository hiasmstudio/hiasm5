#pragma once

#include <QProcess>
#include <QTreeWidget>
#include <QWidget>
#include <functional>

class QComboBox;
class QLineEdit;
class QPlainTextEdit;
class QLabel;
class QPushButton;
class QTabWidget;
class QShowEvent;

class ProjectTree final : public QTreeWidget {
public:
    explicit ProjectTree(QWidget *parent = nullptr);
    void addDirectory(const QString &name, const QString &path);
    std::function<void(const QString &)> openScheme;

private:
    void fill(QTreeWidgetItem *parent, const QString &directory);
};

class RepositoryPanel final : public QWidget {
public:
    explicit RepositoryPanel(QWidget *parent = nullptr);
    ~RepositoryPanel() override;
    void setProject(const QString &schemePath, bool canSave);
    std::function<void(const QString &)> openScheme;
    std::function<void(const QString &)> saveCopy;

private:
    void refresh();
    void run(const QStringList &args);
    QString directory() const;

    QString projectDirectory_;
    QString customDirectory_;
    bool canSave_ = false;
    ProjectTree *tree_;
    QLabel *path_;
    QPushButton *copy_;
    QPushButton *actions_;
    QPlainTextEdit *output_;
    QProcess *process_;
    QString client_;
};

class TerminalPanel final : public QWidget {
public:
    explicit TerminalPanel(QWidget *parent = nullptr);
    void setProjectDirectory(const QString &path);
    QPlainTextEdit *showRunOutput();

protected:
    void showEvent(QShowEvent *event) override;

private:
    void startShell();
    void closeTab(int index);
#ifdef Q_OS_WIN
    void addWslProfiles();
#endif

    QComboBox *shells_;
    QTabWidget *tabs_;
    QPlainTextEdit *runOutput_;
    QString projectDirectory_;
    bool shown_ = false;
};
