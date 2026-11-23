#pragma once

#include "scheme_diagram.h"
#include <QIcon>
#include <QWidget>
#include <optional>

class QLabel;
class FormCanvas final : public QWidget {
    Q_OBJECT
public:
    explicit FormCanvas(QWidget *parent = nullptr);
    bool setLayer(const SchemeDiagram &diagram, int layer, const QString &pack, const QString &hostName);
    void selectElement(int index);
    void armPlacement(const QString &name, const QIcon &icon = {});
    bool placementArmed() const { return !placement_.isEmpty(); }
    std::optional<QPoint> positionFor(const QString &name, std::optional<QPoint> at = {}) const;
    std::optional<QPair<int, int>> propertyTarget(int index) const;
    int hostElement() const { return formIndex_; }
    static constexpr auto elementMime = "application/x-hiasm-element";

signals:
    void elementSelected(int index);
    void childActivated(int index);
    void controlActivated(int index);
    void placementRequested(const QString &name, const QPoint &position);
    void geometryEdited(int layer, int index, const QRect &before, const QRect &after, bool form);

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseDoubleClickEvent(QMouseEvent *event) override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

private:
    struct Control {
        QWidget *widget = nullptr;
        int layer = -1;
        int index = -1;
        int align = 0;
        QRect requested;
    };
    struct Group { QWidget *parent; QList<int> controls; };
    void arrange();
    int hitControl(const QPoint &position) const;
    Qt::Edges resizeEdges() const;
    Qt::Edges handleAt(const QPoint &position) const;
    QRect selectedRect() const;

    QWidget *frame_;
    QWidget *surface_;
    QWidget *title_;
    QWidget *selection_;
    QList<Control> controls_;
    QList<Group> groups_;
    QList<int> activeControls_;
    QList<int> childIndices_;
    int selected_ = -1;
    int formIndex_ = -1;
    QMargins margins_;
    QSize decoration_;
    bool windowPreviewAvailable_ = false;
    Qt::WindowFlags previewFlags_;
    bool previewResizable_ = true;
    qreal previewOpacity_ = 1.0;
    QPoint press_;
    QRect initial_;
    Qt::Edges edges_;
    bool dragging_ = false;
    QString pack_;
    QString placement_;
};
