#pragma once

#include <QWidget>
#include <QCursor>
#include "scheme_diagram.h"
#include "ui_SchemeEditor.h"

class QGraphicsRectItem;
class QGraphicsScene;
class QGraphicsPathItem;
class FormCanvas;

struct SchemeFileFormat {
    QByteArray lineEnding = "\n";
    bool utf8Bom = false;
    bool windows1251 = false;
};

class SchemeEditor final : public QWidget {
    Q_OBJECT
public:
    explicit SchemeEditor(const QString &source, const QString &fileName, const SchemeFileFormat &format,
                          QWidget *parent = nullptr);

    ~SchemeEditor() override;
    QTextDocument *document() const;
    QString source() const;
    QByteArray encodedSource() const;
    QString packName() const;
    bool isFormView() const { return ui_.editorViews->currentWidget() == ui_.formPage; }
    void setFormView(bool enabled) { ui_.editorViews->setCurrentWidget(enabled ? ui_.formPage : ui_.diagramPage); }
    bool insertElement(const QString &name);
    bool armPlacement(const QString &name, const QIcon &icon);
    void showSourceLine(int line);
    std::optional<int> buildMode() const;
    void setBuildMode(int mode);
    QString fileName() const { return fileName_; }
    void setSavedFile(const QString &fileName, const SchemeFileFormat &format) { fileName_ = fileName; format_ = format; }
    QList<QPair<QString, QString>> selectedProperties() const;
    void deleteSelection();
    bool connectPorts(int fromId, const QString &fromPoint, int toId, const QString &toPoint);
    bool changeSelectedProperty(int row, const QString &value);
    bool changeSelectedProperty(const QString &name, const QString &value);
    bool selectedPointEnabled(const QString &name) const;
    bool changeSelectedPoint(const QString &name, bool enabled);
    static constexpr int firstPropertyRow = 4; // Element, ID, X and Y precede serialized properties.

signals:
    void selectionChanged();
    void primaryPropertyRequested();

protected:
    void changeEvent(QEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

private:
    void refreshDiagram(bool resetNavigation);
    void drawLayer(bool retainView = false);
    void updateNavigation();
    void updatePathHeight();
    void populateChildLevels();
    void showCurrentLevel();
    void revealSourceLevel();
    void commitPositions();
    void refreshForm();
    bool insertElement(const QString &name, std::optional<QPoint> position);
    void cancelPlacement();
    const SchemeElement *inspectedElement() const;
    const SchemeLayer &inspectedLayer() const;
    void enterChild(int index, int childIndex = -1);
    void goToAncestor(int level);
    void commitGeometry(int index, const QRect &rect, bool form, int layer = -1, const QRect &before = {});
    struct WireDrag {
        QGraphicsLineItem *preview;
        int anchorId;
        QString anchorPort;
        int anchorKind;
        int connection;
        QPoint start;
    };
    bool connectFrameEdge(QGraphicsRectItem *frame, const QPointF &at, const WireDrag &drag);
    bool connectPorts(int fromId, const QString &fromPoint, int toId, const QString &toPoint, int replaced);
    void beginWireDrag(QGraphicsItem *point, const QPoint &start);
    void finishWireDrag(const QPoint &position);
    void cancelWireDrag();
    void removeConnection(int index);
    void canvasMenu(const QPoint &point);
    bool portUsed(int id, const QString &name) const;
    QGraphicsRectItem *resizePreview_ = nullptr;
    Qt::Edges resizeEdges_;
    int resizeIndex_ = -1;
    std::optional<WireDrag> wireDrag_;
    QList<QGraphicsPathItem *> wires_;
    int pendingSource_ = 0;
    QString pendingPort_;
    QString placement_;
    QCursor placementCursor_;

    Ui::SchemeEditor ui_;
    QGraphicsScene *scene_;
    FormCanvas *form_;
    QString fileName_;
    SchemeFileFormat format_;
    SchemeDiagram diagram_;
    struct Level { int layer; int ownerId; QString name; int childIndex = 0; };
    QList<Level> navigation_{{0, 0, QStringLiteral("ROOT")}};
    QStringList visiblePath_;
    QList<QGraphicsRectItem *> elementItems_;
    bool diagramDirty_ = true;
};
