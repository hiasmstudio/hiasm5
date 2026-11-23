#pragma once

#include <QDialog>
#include <QJsonArray>
#include <QJsonObject>
#include "ui_ProjectManager.h"

class ProjectManagerDialog final : public QDialog {
public:
    explicit ProjectManagerDialog(const QJsonArray &catalog, QWidget *parent = nullptr);

    static QJsonArray projectCatalog();
    int projectRow() const;
    QString selectedTemplate() const;
    void accept() override;

private:
    void updateProjects(int row);
    void updateSelection(int row);

    Ui::ProjectManager ui_;
    const QJsonArray catalog_;
    QJsonObject pack_;
    QJsonArray projects_;
};
