#include "scheme_editor.h"
#include "element_config.h"
#include "form_canvas.h"
#include "package_paths.h"
#include "project_type.h"
#include "../RTCG/CGTShare.h"
#include <QMenu>
#include <QPainter>
#include <array>
#include <algorithm>

#include <QCoreApplication>
#include <QApplication>
#include <QGraphicsItem>
#include <QGraphicsScene>
#include <QHash>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QScrollBar>
#include <QSet>
#include <QTextBlock>
#include <QShortcut>
#include <QSignalBlocker>
#include <QTimer>
#include <QWheelEvent>

namespace {
QString trEditor(const char *text) {
    return QCoreApplication::translate("SchemeEditor", text);
}

enum ItemRole { FrameRole = 4, MinimumFrameSizeRole };

class LevelLink final : public QToolButton {
public:
    LevelLink(const QString &name, QWidget *parent) : QToolButton(parent) {
        setText(name);
        setAutoRaise(true);
        setCursor(Qt::PointingHandCursor);
        setFocusPolicy(Qt::StrongFocus);
    }

protected:
    void enterEvent(QEnterEvent *event) override {
        setUnderline(true);
        QToolButton::enterEvent(event);
    }

    void leaveEvent(QEvent *event) override {
        setUnderline(false);
        QToolButton::leaveEvent(event);
    }

private:
    void setUnderline(bool enabled) {
        auto style = font();
        style.setUnderline(enabled);
        setFont(style);
    }
};

class SdkFrameItem final : public QGraphicsRectItem {
public:
    using QGraphicsRectItem::QGraphicsRectItem;
    QRectF boundingRect() const override { return rect().adjusted(-4,-4,4,4); }
    QPainterPath shape() const override {
        QPainterPath border;
        border.addRect(boundingRect());
        border.addRect(rect().adjusted(4,4,-4,-4));
        return border;
    }
};

int frameProperty(const SchemeElement &element, const QString &name, int fallback) {
    for (const auto &[key, text] : element.properties) {
        if (key != name && key != u'@' + name) continue;
        bool valid;
        const int value = text.toInt(&valid);
        if (valid) return qMax(0, value);
    }
    return fallback;
}

// Intersect the line from the centre toward the next waypoint with the node.
QPointF wireAnchor(const QRectF &rect, const QPointF &toward) {
    const QPointF delta = toward - rect.center();
    const auto factor = qMax(qAbs(delta.x()) / (rect.width() / 2),
                             qAbs(delta.y()) / (rect.height() / 2));
    return factor > 0 ? rect.center() + delta / factor : rect.center();
}

QPainterPath wirePath(const QPointF &start, const QPointF &end, const QList<QPointF> &bends,
                      bool data, const QRectF &source, const QRectF &target) {
    QPainterPath path(start);
    if (!bends.isEmpty()) {
        for (const auto &bend : bends) path.lineTo(bend);
    } else {
        // Project event and data wires onto their outgoing axis (right or up).
        const auto project = [data](QPointF p) { return data ? QPointF(-p.y(), p.x()) : p; };
        const auto line = [&](qreal u, qreal v) { path.lineTo(data ? QPointF(v, -u) : QPointF(u, v)); };
        const auto a = project(start), b = project(end);
        constexpr int gap = 14;
        if (b.x() - a.x() >= 2 * gap) {
            const auto mid = (a.x() + b.x()) / 2;
            line(mid, a.y());
            line(mid, b.y());
        } else {
            const auto s1 = project(source.topLeft()).y(), s2 = project(source.bottomRight()).y();
            const auto t1 = project(target.topLeft()).y(), t2 = project(target.bottomRight()).y();
            qreal across;
            if (s2 < t1) across = (s2 + t1) / 2;
            else if (t2 < s1) across = (t2 + s1) / 2;
            else {
                const auto low = qMin(s1,t1) - gap, high = qMax(s2,t2) + gap;
                across = a.y() + b.y() - 2 * low <= 2 * high - a.y() - b.y() ? low : high;
            }
            line(a.x() + gap, a.y());
            line(a.x() + gap, across);
            line(b.x() - gap, across);
            line(b.x() - gap, b.y());
        }
    }
    path.lineTo(end);
    return path;
}

QGraphicsItem *elementItem(QGraphicsItem *item) {
    while (item && item->parentItem())
        item = item->parentItem();
    return item && item->data(0).isValid() ? item : nullptr;
}

bool isSourcePort(const SchemeLayer &layer, const SchemeConnection &link, int id, const QString &name) {
    return layer.elements.at(link.source).id == id && link.sourcePoint.compare(name, Qt::CaseInsensitive) == 0;
}

bool isTargetPort(const SchemeConnection &link, int id, const QString &name) {
    return link.targetId == id && link.targetPoint.compare(name, Qt::CaseInsensitive) == 0;
}

QList<int> connectionsAtPort(const SchemeLayer &layer, int id, const QString &name) {
    QList<int> found;
    for (int i = 0; i < layer.connections.size(); ++i) {
        const auto &link = layer.connections.at(i);
        if (isSourcePort(layer, link, id, name) || isTargetPort(link, id, name)) found.append(i);
    }
    return found;
}
}

SchemeEditor::SchemeEditor(const QString &source, const QString &fileName, const SchemeFileFormat &format,
                           QWidget *parent)
    : QWidget(parent), scene_(new QGraphicsScene(this)), fileName_(fileName), format_(format) {
    ui_.setupUi(this);
    form_ = new FormCanvas;
    form_->setObjectName("formDesigner");
    ui_.formScroll->setWidget(form_);
    connect(form_, &FormCanvas::elementSelected, this, [this](int index) {
        scene_->clearSelection();
        if (index >= 0 && index < elementItems_.size()) elementItems_.at(index)->setSelected(true);
    });
    connect(form_, &FormCanvas::geometryEdited, this, [this](int layer, int index, const QRect &before, const QRect &after, bool form) {
        commitGeometry(index, after, form, layer, before);
    });
    connect(form_, &FormCanvas::childActivated, this, [this](int index) { enterChild(index); });
    connect(form_, &FormCanvas::controlActivated, this, [this](int index) {
        if (index >= elementItems_.size()) return;
        if (!elementItems_.at(index)->isSelected() || scene_->selectedItems().size() != 1) {
            scene_->clearSelection();
            elementItems_.at(index)->setSelected(true);
        }
        emit primaryPropertyRequested();
    });
    connect(form_, &FormCanvas::placementRequested, this, [this](const QString &name, const QPoint &position) {
        insertElement(name, position);
    });
    ui_.pathLayout->setAlignment(Qt::AlignLeft);
    auto *children = new QMenu(ui_.childLevels);
    children->addAction(trEditor("No nested schemes"))->setEnabled(false);
    ui_.childLevels->setMenu(children);
    connect(children, &QMenu::aboutToShow, this, &SchemeEditor::populateChildLevels);
    connect(ui_.pathScroll->horizontalScrollBar(), &QScrollBar::rangeChanged, this, &SchemeEditor::updatePathHeight);
    ui_.sourceEditor->setPlainText(source);
    document()->setModified(false);
    ui_.canvas->setScene(scene_);
    ui_.canvas->setRenderHint(QPainter::Antialiasing);
    ui_.canvas->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
    ui_.canvas->viewport()->installEventFilter(this);
    ui_.canvas->installEventFilter(this);
    form_->installEventFilter(this);
    connect(ui_.zoomSlider, &QSlider::valueChanged, this, [this](int percent) {
        const qreal factor = percent / (100.0 * ui_.canvas->transform().m11());
        ui_.canvas->scale(factor, factor);
        ui_.zoomValue->setText(QString::number(percent) + u'%');
    });

    ui_.canvas->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(ui_.canvas, &QWidget::customContextMenuRequested, this, &SchemeEditor::canvasMenu);
    for (auto *view : {static_cast<QWidget *>(ui_.canvas), static_cast<QWidget *>(form_)}) {
        auto *remove = new QShortcut(QKeySequence::Delete, view);
        remove->setContext(Qt::WidgetWithChildrenShortcut);
        connect(remove, &QShortcut::activated, this, &SchemeEditor::deleteSelection);
    }
    connect(this, &SchemeEditor::selectionChanged, this, [this] {
        const auto count = scene_->selectedItems().size();
        ui_.selectionCount->setText(count ? trEditor("Selected: %1").arg(count) : QString());
    });
    connect(scene_, &QGraphicsScene::selectionChanged, this, [this] {
        if (ui_.editorViews->currentWidget() == ui_.formPage) {
            const auto items = scene_->selectedItems();
            form_->selectElement(items.size() == 1 ? items.first()->data(0).toInt() : -1);
        }
        emit selectionChanged();
    });
    connect(ui_.editorViews, &QTabWidget::currentChanged, this, [this] {
        cancelPlacement();
        cancelWireDrag();
        const auto *view = ui_.editorViews->currentWidget();
        ui_.zoomValue->setVisible(view == ui_.diagramPage);
        ui_.zoomSlider->setVisible(view == ui_.diagramPage);
        ui_.selectionCount->setVisible(view != ui_.sourcePage);
        if (view != ui_.sourcePage && diagramDirty_)
            refreshDiagram(false);
        else {
            updateNavigation();
            if (isFormView()) {
                refreshForm();
            } else {
                if (view == ui_.sourcePage) revealSourceLevel();
                emit selectionChanged();
            }
        }
    });
    connect(document(), &QTextDocument::contentsChanged, this, [this] {
        diagramDirty_ = true;
        if (ui_.editorViews->currentWidget() != ui_.sourcePage)
            refreshDiagram(false);
    });
    for (const auto key : {QKeySequence::Undo, QKeySequence::Redo}) {
        auto *shortcut = new QShortcut(QKeySequence(key), this);
        shortcut->setContext(Qt::WidgetWithChildrenShortcut);
        connect(shortcut, &QShortcut::activated, this, [this, key] {
            if (key == QKeySequence::Undo)
                document()->undo();
            else
                document()->redo();
        });
    }
    refreshDiagram(true);
}

SchemeEditor::~SchemeEditor() {
    // Removing selected graphics items emits selectionChanged during scene destruction.
    disconnect(scene_, nullptr, this, nullptr);
}

QTextDocument *SchemeEditor::document() const { return ui_.sourceEditor->document(); }
QString SchemeEditor::source() const { return document()->toRawText().replace(QChar::ParagraphSeparator, u'\n'); }

QString SchemeEditor::packName() const {
    for (auto block = document()->begin(); block.isValid(); block = block.next()) {
        const auto line = block.text().trimmed();
        if (line.isEmpty() || line.startsWith(u'*') || line.startsWith("ver(") || line.startsWith("Pos(")) continue;
        if (line.startsWith("Make(") && line.endsWith(u')'))
            return line.mid(5, line.size() - 6).trimmed().toLower();
        break;
    }
    return {};
}

bool SchemeEditor::insertElement(const QString &name) {
    const auto position = isFormView() ? form_->positionFor(name) : std::nullopt;
    if (isFormView() && !position) return false;
    cancelPlacement();
    return insertElement(name, position);
}

bool SchemeEditor::armPlacement(const QString &name, const QIcon &icon) {
    cancelPlacement();
    if (diagramDirty_ || diagram_.errorLine || name.isEmpty()) return false;
    if (isFormView()) {
        if (ui_.formScroll->isHidden() || !form_->positionFor(name)) return false;
        form_->armPlacement(name, icon);
    } else {
        if (ui_.editorViews->currentWidget() != ui_.diagramPage) return false;
        ui_.canvas->setFocus();
        placement_ = name;
        placementCursor_ = elementPlacementCursor(icon, palette());
        ui_.canvas->viewport()->setCursor(placementCursor_);
    }
    return true;
}

void SchemeEditor::cancelPlacement() {
    if (!placement_.isEmpty()) {
        placement_.clear();
        ui_.canvas->viewport()->unsetCursor();
    }
    if (form_->placementArmed()) form_->armPlacement({});
}

bool SchemeEditor::insertElement(const QString &name, std::optional<QPoint> at) {
    if (ui_.editorViews->currentWidget() == ui_.sourcePage || diagramDirty_ || diagram_.errorLine) return false;
    const auto &layer = diagram_.layers.at(navigation_.last().layer);
    QSet<int> ids;
    for (const auto &sdk : diagram_.layers)
        for (const auto &e : sdk.elements) ids.insert(e.id);
    int id = 1;
    auto allocateId = [&] {
        while (ids.contains(id)) ++id;
        ids.insert(id);
        return id++;
    };
    const int ownerId = allocateId();
    const auto pack = packName();
    const auto config = elementConfig(editorPackRoot(pack), pack, name);
    const auto formPosition = isFormView() ? at : std::nullopt;
    QPoint position;
    if (at && !formPosition) {
        position = *at;
    } else {
        position = ui_.canvas->mapToScene(ui_.canvas->viewport()->rect().center()).toPoint();
        for (const auto *item : elementItems_) {
            if (!item->data(FrameRole).toBool()) continue;
            const auto bounds = item->mapRectToScene(item->rect()).toRect();
            position = bounds.topLeft() + QPoint(42,42);
            while (elementItem(scene_->itemAt(position + QPoint(12,12), QTransform()))) {
                position.rx() += 56;
                if (position.x() + 32 >= bounds.right()) { position.setX(bounds.left() + 42); position.ry() += 49; }
            }
            break;
        }
        while (elementItem(scene_->itemAt(position, QTransform()))) position += QPoint(14, 28);
    }
    QTextCursor cursor(document());
    cursor.setPosition(int(layer.insertionOffset));
    QString entry;
    if (!cursor.atBlockStart()) entry += u'\n';
    const auto placement = formPosition ? QStringLiteral(" Left=%1\n Top=%2\n")
        .arg(formPosition->x()).arg(formPosition->y()) : QString();
    const bool pageControl = packName() == "cpp" && name == "PageControl";
    auto initial = config.sub.isEmpty() || pageControl ? placement : QString();
    if (pageControl) initial += QStringLiteral(" Childrens=#4:Page|\n Selected=\"Page\"\n");
    entry += QStringLiteral("Add(%1,%2,%3,%4)\n{\n%5}\n")
        .arg(name).arg(ownerId).arg(position.x()).arg(position.y()).arg(initial);
    if (pageControl) {
        const auto frame = [&](int id) {
            return QStringLiteral("BEGIN_SDK\n  Add(EditPolyMulti,%1,14,14)\n  {\n   WorkCount=#5:##add|8:##select|8:##delete|7:##clear|\n   VarCount=#7:##count|7:##index|\n  }\n").arg(id);
        };
        entry += frame(allocateId()) + QStringLiteral("END_SDK\n");
        entry += frame(allocateId());
        entry += QStringLiteral("  Add(PagePanel,%1,56,56)\n  {\n   Caption=\"Page\"\n  }\nEND_SDK\n").arg(allocateId());
    } else if (config.createsSdkOnInsertion()) {
        QByteArray frame = config.editClass;
        if (frame.isEmpty()) frame = config.className == "MultiElementEx" ? "EditMultiEx" :
                                     config.className == "HCMultiElement" ? "hcEditMulti" : "EditMulti";
        entry += QStringLiteral("BEGIN_SDK\n  Add(%1,%2,14,14)\n  {\n  }\n").arg(QString::fromUtf8(frame)).arg(allocateId());
        if (!config.sub.isEmpty())
            entry += QStringLiteral("  Add(%1,%2,56,56)\n  {\n%3  }\n").arg(QString::fromUtf8(config.sub)).arg(allocateId()).arg(placement);
        entry += QStringLiteral("END_SDK\n");
    }
    cursor.insertText(entry);
    scene_->clearSelection();
    const auto &updated = diagram_.layers.at(navigation_.last().layer).elements;
    for (int i = 0; i < updated.size(); ++i)
        if (updated.at(i).id == ownerId) { elementItems_.at(i)->setSelected(true); break; }
    return true;
}

QByteArray SchemeEditor::encodedSource() const {
    const auto unicode = source();
    auto legacy = format_.windows1251 ? encodeWindows1251(unicode) : std::nullopt;
    auto text = legacy ? std::move(*legacy) : unicode.toUtf8();
    if (format_.lineEnding != "\n")
        text.replace("\n", format_.lineEnding);
    if (format_.utf8Bom || (format_.windows1251 && !legacy))
        text.prepend("\xef\xbb\xbf", 3);
    return text;
}

const SchemeElement *SchemeEditor::inspectedElement() const {
    if (ui_.editorViews->currentWidget() == ui_.sourcePage || diagram_.errorLine)
        return nullptr;
    const auto selected = scene_->selectedItems();
    if (selected.size() != 1)
        return nullptr;
    const auto index = selected.first()->data(0).toInt();
    if (isFormView()) {
        if (ui_.formScroll->isHidden()) return nullptr;
        const auto target = form_->propertyTarget(index);
        return target ? &diagram_.layers.at(target->first).elements.at(target->second) : nullptr;
    }
    return &diagram_.layers.at(navigation_.last().layer).elements.at(index);
}

QList<QPair<QString, QString>> SchemeEditor::selectedProperties() const {
    const auto *inspected = inspectedElement();
    if (!inspected) return {};
    const auto &element = *inspected;
    QList<QPair<QString, QString>> result{
        {trEditor("Element"), element.name},
        {trEditor("ID"), QString::number(element.id)},
        {QStringLiteral("X"), QString::number(element.position.x())},
        {QStringLiteral("Y"), QString::number(element.position.y())}
    };
    result.append(element.properties);
    const auto pack = packName();
    const auto config = elementConfig(editorPackRoot(pack), pack, element.name);
    for (const auto &p : config.properties) {
        const QString name = QString::fromUtf8(p.name);
        const auto saved = std::any_of(element.properties.cbegin(), element.properties.cend(),
            [&](const auto &entry) { return entry.first.compare(name, Qt::CaseInsensitive) == 0; });
        if (!saved) result.append({name, QString::fromUtf8(p.initial)});
    }
    return result;
}

bool SchemeEditor::changeSelectedProperty(int row, const QString &value) {
    const auto properties = selectedProperties();
    return row >= firstPropertyRow && row < properties.size() && changeSelectedProperty(properties.at(row).first, value);
}

bool SchemeEditor::changeSelectedProperty(const QString &name, const QString &value) {
    const auto selected = scene_->selectedItems();
    if (selected.size() != 1 || diagramDirty_ || diagram_.errorLine || value.contains(u'\n') ||
        value.contains(u'\r') || value.contains(QChar::ParagraphSeparator)) return false;
    const auto *inspected = inspectedElement();
    if (!inspected) return false;
    const auto &e = *inspected;
    const auto found = std::find_if(e.properties.cbegin(), e.properties.cend(), [&](const auto &property) {
        return property.first.compare(name, Qt::CaseInsensitive) == 0;
    });
    const auto index = std::distance(e.properties.cbegin(), found);
    const auto property = qMakePair(name, found == e.properties.cend() ? QString() : found->second);
    if (found != e.properties.cend() && property.second == value) return true;
    const bool existing = index < e.propertyValues.size();
    const SchemeTextSpan span = existing ? e.propertyValues.at(index) : SchemeTextSpan{e.bodyEnd, 0};
    QList<QPair<SchemeTextSpan, QString>> edits{{span, existing ? value : " " + property.first + "=" + value + "\n"}};

    // A frame owns the same interface in two SDKs. Remove links to discarded
    // ports in both layers in the same undo operation as the property change.
    if (!isFormView() && navigation_.size() > 1 && selected.first()->data(FrameRole).toBool() && property.first.endsWith("Count")) {
        const auto pack = packName();
        const auto config = elementConfig(editorPackRoot(pack), pack, e.name);
        auto changed = e.properties;
        if (existing) changed[index].second = value;
        else changed.append({property.first, value});
        QSet<QString> removed;
        for (const auto &port : elementPorts(config, e.properties)) removed.insert(QString::fromUtf8(port.name).toCaseFolded());
        for (const auto &port : elementPorts(config, changed)) removed.remove(QString::fromUtf8(port.name).toCaseFolded());
        const auto removeLinks = [&](int layerId, int id) {
            const auto &layer = diagram_.layers.at(layerId);
            for (const auto &link : layer.connections) {
                if ((layer.elements.at(link.source).id == id && removed.contains(link.sourcePoint.toCaseFolded())) ||
                    (link.targetId == id && removed.contains(link.targetPoint.toCaseFolded())))
                    edits.append({link.line, QString()});
            }
        };
        removeLinks(navigation_.last().layer, e.id);
        removeLinks(navigation_.at(navigation_.size() - 2).layer, navigation_.last().ownerId);
    }
    std::stable_sort(edits.begin(), edits.end(), [](const auto &a, const auto &b) { return a.first.start > b.first.start; });
    QTextCursor cursor(document());
    cursor.beginEditBlock();
    for (const auto &[where, text] : edits) {
        cursor.setPosition(int(where.start));
        cursor.setPosition(int(where.start + where.length), QTextCursor::KeepAnchor);
        cursor.insertText(text);
    }
    cursor.endEditBlock();
    return true;
}

const SchemeLayer &SchemeEditor::inspectedLayer() const {
    const auto index = scene_->selectedItems().first()->data(0).toInt();
    return diagram_.layers.at(isFormView() ? form_->propertyTarget(index)->first : navigation_.last().layer);
}

bool SchemeEditor::selectedPointEnabled(const QString &name) const {
    const auto *element = inspectedElement();
    if (!element) return false;
    const auto &layer = inspectedLayer();
    return element->explicitPorts.contains(name, Qt::CaseInsensitive) ||
        std::any_of(layer.connections.cbegin(), layer.connections.cend(), [&](const auto &link) {
            return (&layer.elements.at(link.source) == element && link.sourcePoint.compare(name, Qt::CaseInsensitive) == 0) ||
                (link.targetId == element->id && link.targetPoint.compare(name, Qt::CaseInsensitive) == 0);
        });
}

bool SchemeEditor::changeSelectedPoint(const QString &name, bool enabled) {
    const auto *element = inspectedElement();
    if (!element || diagramDirty_) return false;
    const auto pack = packName();
    const auto config = elementConfig(editorPackRoot(pack), pack, element->name);
    const auto port = std::find_if(config.ports.cbegin(), config.ports.cend(), [&](const auto &p) {
        return p.optional && QString::fromUtf8(p.name).compare(name, Qt::CaseInsensitive) == 0;
    });
    if (port == config.ports.cend()) return false;
    if (selectedPointEnabled(name) == enabled) return true;
    QList<SchemeTextSpan> removed;
    if (!enabled) {
        for (qsizetype i = 0; i < element->explicitPorts.size(); ++i)
            if (element->explicitPorts.at(i).compare(name, Qt::CaseInsensitive) == 0)
                removed.append(element->explicitPortLines.at(i));
        const auto &layer = inspectedLayer();
        for (const auto &link : layer.connections)
            if ((&layer.elements.at(link.source) == element && link.sourcePoint.compare(name, Qt::CaseInsensitive) == 0) ||
                (link.targetId == element->id && link.targetPoint.compare(name, Qt::CaseInsensitive) == 0))
                removed.append(link.line);
        std::sort(removed.begin(), removed.end(), [](auto a, auto b) { return a.start > b.start; });
    }
    QTextCursor cursor(document());
    cursor.beginEditBlock();
    if (enabled) {
        cursor.setPosition(int(element->bodyEnd));
        cursor.insertText(QStringLiteral(" Point(%1)\n").arg(QString::fromUtf8(port->name)));
    } else {
        for (const auto span : removed) {
            cursor.setPosition(int(span.start));
            cursor.setPosition(int(span.start + span.length), QTextCursor::KeepAnchor);
            cursor.removeSelectedText();
        }
    }
    cursor.endEditBlock();
    return true;
}

void SchemeEditor::refreshDiagram(bool resetNavigation) {
    pendingPort_.clear();
    diagram_ = readSchemeDiagram(source());
    diagramDirty_ = false;
    if (resetNavigation || diagram_.errorLine) navigation_.resize(1);
    for (qsizetype i = 1; i < navigation_.size(); ++i) {
        const auto &siblings = diagram_.layers.at(navigation_[i - 1].layer).elements;
        const auto owner = std::find_if(siblings.begin(), siblings.end(), [&](const auto &e) {
            return e.id == navigation_[i].ownerId && e.child >= 0;
        });
        if (owner == siblings.end() || navigation_[i].childIndex >= owner->childLayers.size()) {
            navigation_.resize(i); resetNavigation = true; break;
        }
        navigation_[i].layer = owner->childLayers.at(navigation_[i].childIndex);
        navigation_[i].name = owner->name;
    }
    drawLayer(!resetNavigation);
    if (ui_.editorViews->currentWidget() == ui_.formPage) refreshForm();
}

void SchemeEditor::drawLayer(bool retainView) {
    const QSignalBlocker blockScene(scene_);
    QList<int> selected;
    if (retainView) {
        for (const auto *item : scene_->selectedItems())
            selected.append(item->data(0).toInt());
    }
    const int scrollX = ui_.canvas->horizontalScrollBar()->value();
    const int scrollY = ui_.canvas->verticalScrollBar()->value();
    resizePreview_ = nullptr;
    wireDrag_.reset();
    wires_.clear();
    scene_->clear();
    elementItems_.clear();
    updateNavigation();
    ui_.diagramError->setVisible(diagram_.errorLine != 0);
    ui_.canvas->setVisible(diagram_.errorLine == 0);
    if (diagram_.errorLine) {
        ui_.diagramError->setText(trEditor("Scheme syntax error near line %1. Edit the source to restore the diagram.")
            .arg(diagram_.errorLine));
        emit selectionChanged();
        return;
    }

    const auto &layer = diagram_.layers.at(navigation_.last().layer);
    const QFontMetrics metrics(font());
    const auto colors = palette();
    QPixmap grid(10, 10);
    grid.fill(colors.color(QPalette::Base));
    {
        QPainter painter(&grid);
        auto dot = colors.color(QPalette::Text);
        dot.setAlpha(75);
        painter.setPen(dot);
        painter.drawPoint(0, 0);
    }
    scene_->setBackgroundBrush(grid);

    const auto pack = packName(), root = editorPackRoot(pack);
    QHash<int, QGraphicsRectItem *> itemsById;
    QHash<int, QSet<QString>> usedPorts;
    QHash<int, QHash<QString,QPointF>> anchors;
    QSet<QPair<int,QString>> dataSources;
    for (const auto &link : layer.connections) {
        usedPorts[layer.elements.at(link.source).id].insert(link.sourcePoint.toCaseFolded());
        usedPorts[link.targetId].insert(link.targetPoint.toCaseFolded());
    }
    for (qsizetype i = 0; i < layer.elements.size(); ++i) {
        const auto &element = layer.elements.at(i);
        auto visible = usedPorts.value(element.id);
        for (const auto &name : element.explicitPorts) visible.insert(name.toCaseFolded());
        const auto config = elementConfig(root, pack, element.name);
        const bool frame = config.isSdkFrame();
        auto ports = element.child >= 0 ? schemeElementPorts(root, pack, diagram_, element) :
                                          elementPorts(config, element.properties);
        ports.removeIf([&](const ConfigPort &p) {
            return p.kind < pt_Work || p.kind > pt_Data ||
                (p.optional && !visible.contains(QString::fromUtf8(p.name).toCaseFolded()));
        });
        std::array<int, 5> counts{};
        for (const auto &p : ports) ++counts[p.kind];
        const auto icon = elementIcon(root, pack, element.name);
        const int labelWidth = icon.isNull() ? qBound(32, metrics.horizontalAdvance(element.name) + 10, 112) : 32;
        const int hoff = frame ? qMin(frameProperty(element, "HOffset", 0), 1000000) : 0;
        const int voff = frame ? qMin(frameProperty(element, "VOffset", 0), 1000000) : 0;
        const QSize minimum(qMax(frame ? 32 : labelWidth, hoff + 7 * qMax(counts[pt_Var], counts[pt_Data]) + 3),
                            qMax(32, voff + 7 * qMax(counts[pt_Work], counts[pt_Event]) + 3));
        const int width = frame ? qMax(minimum.width(), frameProperty(element, "Width", 300)) : minimum.width();
        const int height = frame ? qMax(minimum.height(), frameProperty(element, "Height", 200)) : minimum.height();
        QGraphicsRectItem *item;
        if (frame) {
            item = new SdkFrameItem(0, 0, width, height);
            item->setData(FrameRole, true);
            item->setData(MinimumFrameSizeRole, minimum);
            item->setPen(QPen(colors.color(QPalette::Mid), 2));
            item->setZValue(-0.5);
            scene_->addItem(item);
        } else {
            item = scene_->addRect(0, 0, width, height, QPen(colors.color(QPalette::Mid)),
                colors.brush(element.child >= 0 ? QPalette::AlternateBase : QPalette::Button));
        }
        item->setFlags(QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemIsMovable);
        item->setData(0, static_cast<int>(i));
        item->setPos(element.position);
        item->setToolTip(element.child >= 0
            ? trEditor("%1 (%2)\nDouble-click to open the nested scheme").arg(element.name).arg(element.id)
            : QStringLiteral("%1 (%2)").arg(element.name).arg(element.id));
        if (!frame && icon.isNull()) {
            auto *text = new QGraphicsSimpleTextItem(metrics.elidedText(element.name, Qt::ElideRight, width - 8), item);
            text->setFont(font());
            text->setBrush(colors.brush(QPalette::ButtonText));
            text->setPos(4, (height - text->boundingRect().height()) / 2);
            text->setAcceptedMouseButtons(Qt::NoButton);
        } else if (!frame) {
            auto *picture = new QGraphicsPixmapItem(icon.pixmap(24,24), item);
            picture->setPos((width - picture->boundingRect().width()) / 2, (height - picture->boundingRect().height()) / 2);
            picture->setAcceptedMouseButtons(Qt::NoButton);
        }
        counts.fill(0);
        for (const auto &p : ports) {
            const int offset = 6 + 7 * counts[p.kind]++;
            const bool side = p.kind <= pt_Event;
            const bool farEdge = (side ? p.kind == pt_Event : p.kind == pt_Var) != frame;
            const QPointF at = side ? QPointF(farEdge ? width : 0, offset + voff) :
                                      QPointF(offset + hoff, farEdge ? height : 0);
            const auto name = QString::fromUtf8(p.name);
            anchors[element.id].insert(name.toCaseFolded(), element.position + at);
            if (p.kind == pt_Data) dataSources.insert({element.id,name.toCaseFolded()});
            auto *point = new QGraphicsEllipseItem(-2.5,-2.5,5,5,item);
            point->setPos(at);
            point->setPen(QPen(QColor("#435043"), 0));
            point->setBrush(QColor("#79e65a"));
            point->setToolTip(name);
            point->setData(2, name);
            point->setData(3, p.kind);
            point->setCursor(Qt::CrossCursor);
        }
        elementItems_.append(item);
        itemsById.insert(element.id, item);
    }
    wires_.fill(nullptr, layer.connections.size());
    for (int l = 0; l < layer.connections.size(); ++l) {
        const auto &link = layer.connections.at(l);
        const auto *target = itemsById.value(link.targetId);
        if (!target) continue;
        const auto *source = elementItems_.at(link.source);
        const auto sourceRect = source->mapRectToScene(source->rect());
        const auto targetRect = target->mapRectToScene(target->rect());
        auto bends = link.bends;
        if (source == target && bends.isEmpty() && !anchors.value(link.targetId).contains(link.targetPoint.toCaseFolded()))
            bends = {sourceRect.topRight() + QPointF(14, 0), sourceRect.topRight() + QPointF(14,-14),
                     sourceRect.topLeft() + QPointF(-14,-14), sourceRect.topLeft() + QPointF(-14,0)};
        const int id = layer.elements.at(link.source).id;
        const auto start = anchors.value(id).value(link.sourcePoint.toCaseFolded(),
            wireAnchor(sourceRect, bends.isEmpty() ? targetRect.center() : bends.first()));
        const auto end = anchors.value(link.targetId).value(link.targetPoint.toCaseFolded(),
            wireAnchor(targetRect, bends.isEmpty() ? sourceRect.center() : bends.last()));
        const bool data = dataSources.contains({id,link.sourcePoint.toCaseFolded()});
        const bool dark = colors.color(QPalette::Base).lightness() < 128;
        QPen wirePen(data ? QColor(dark ? "#ff8888" : "#a72324") : QColor(dark ? "#89abff" : "#193991"));
        wirePen.setCosmetic(true);
        auto *pathItem = scene_->addPath(wirePath(start, end, bends, data, sourceRect, targetRect), wirePen);
        pathItem->setZValue(-1);
        pathItem->setData(1, l);
        wires_[l] = pathItem;
        pathItem->setToolTip(trEditor("%1 → %2").arg(link.sourcePoint, link.targetPoint));
    }
    for (int index : selected) {
        if (index < elementItems_.size())
            elementItems_.at(index)->setSelected(true);
    }
    auto bounds = scene_->itemsBoundingRect().united(QRectF(0, 0, 1, 1));
    scene_->setSceneRect(bounds.adjusted(-24, -24, 24, 24));
    ui_.canvas->horizontalScrollBar()->setValue(retainView ? scrollX : ui_.canvas->horizontalScrollBar()->minimum());
    ui_.canvas->verticalScrollBar()->setValue(retainView ? scrollY : ui_.canvas->verticalScrollBar()->minimum());
    if (!isFormView()) emit selectionChanged();
}

void SchemeEditor::enterChild(int index, int childIndex) {
    const auto &e = diagram_.layers.at(navigation_.last().layer).elements.at(index);
    if (e.childLayers.isEmpty()) return;
    if (childIndex < 0) childIndex = int(e.childLayers.size()) - 1;
    if (childIndex >= e.childLayers.size()) return;
    navigation_.append({e.childLayers.at(childIndex), e.id, e.name, childIndex});
    showCurrentLevel();
}

void SchemeEditor::goToAncestor(int level) {
    if (diagramDirty_) refreshDiagram(false);
    if (level + 1 >= navigation_.size()) return;
    const int owner = navigation_.at(level + 1).ownerId;
    navigation_.resize(level + 1);
    showCurrentLevel();
    const auto &elements = diagram_.layers.at(navigation_.last().layer).elements;
    for (int i = 0; i < elements.size(); ++i)
        if (elements.at(i).id == owner) { elementItems_.at(i)->setSelected(true); break; }
}

void SchemeEditor::showCurrentLevel() {
    cancelPlacement();
    pendingPort_.clear();
    drawLayer();
    if (isFormView()) refreshForm();
    else if (ui_.editorViews->currentWidget() == ui_.sourcePage) revealSourceLevel();
}

void SchemeEditor::revealSourceLevel() {
    if (diagram_.errorLine) return;
    const auto offset = diagram_.layers.at(navigation_.last().layer).startOffset;
    ui_.sourceEditor->setTextCursor(QTextCursor(document()->findBlock(int(offset))));
    ui_.sourceEditor->centerCursor();
}

void SchemeEditor::updateNavigation() {
    bool canEnter = ui_.editorViews->currentWidget() == ui_.sourcePage;
    if (!diagram_.errorLine) {
        const auto &elements = diagram_.layers.at(navigation_.last().layer).elements;
        canEnter |= std::any_of(elements.cbegin(), elements.cend(), [](const auto &element) { return element.child >= 0; });
    }
    ui_.childLevels->setEnabled(canEnter);

    QStringList path;
    for (const auto &level : navigation_) path.append(level.name);
    if (path == visiblePath_) return;
    visiblePath_ = path;
    while (ui_.pathLayout->count() > 1) {
        auto *item = ui_.pathLayout->takeAt(0);
        auto *widget = item->widget();
        widget->hide();
        widget->setObjectName({});
        widget->deleteLater();
        delete item;
    }
    for (int i = 0; i < path.size(); ++i) {
        const bool current = i == path.size() - 1;
        if (current) {
            auto *label = new QLabel(path.at(i), ui_.schemePath);
            label->setTextFormat(Qt::PlainText);
            label->setObjectName("currentSchemeLevel");
            ui_.pathLayout->insertWidget(2 * i, label);
        } else {
            auto *link = new LevelLink(path.at(i), ui_.schemePath);
            link->setObjectName(QStringLiteral("schemeLevel%1").arg(i));
            connect(link, &QToolButton::clicked, this, [this, i] { goToAncestor(i); });
            ui_.pathLayout->insertWidget(2 * i, link);
        }
        auto *separator = new QLabel(QStringLiteral("\\"), ui_.schemePath);
        ui_.pathLayout->insertWidget(2 * i + 1, separator);
    }
    updatePathHeight();
}

void SchemeEditor::updatePathHeight() {
    const auto *bar = ui_.pathScroll->horizontalScrollBar();
    const auto barHeight = bar->maximum() > 0 ? bar->sizeHint().height() : 0;
    ui_.pathScroll->setFixedHeight(ui_.pathLayout->sizeHint().height() + barHeight);
}

void SchemeEditor::populateChildLevels() {
    if (diagramDirty_) refreshDiagram(false);
    auto *menu = ui_.childLevels->menu();
    menu->clear();
    if (!diagram_.errorLine) {
        const auto &elements = diagram_.layers.at(navigation_.last().layer).elements;
        for (int i = 0; i < elements.size(); ++i) {
            const auto &element = elements.at(i);
            if (element.child < 0) continue;
            const auto title = QStringLiteral("%1 [%2]").arg(element.name).arg(element.id);
            if (element.name == "PageControl" && element.childLayers.size() > 1) {
                auto *pages = menu->addMenu(title);
                QStringList names;
                for (const auto &[key, value] : element.properties)
                    if (key == "Childrens") decodeSchemeLines(value, names);
                pages->addAction(QStringLiteral("EditPolyMulti"), this, [this, i] { enterChild(i, 0); });
                for (int child = 1; child < element.childLayers.size(); ++child)
                    pages->addAction(names.value(child - 1, QString::number(child)), this, [this, i, child] { enterChild(i, child); });
            } else menu->addAction(title, this, [this, i] { enterChild(i); });
        }
    }
    if (menu->isEmpty()) menu->addAction(trEditor("No nested schemes"))->setEnabled(false);
}

void SchemeEditor::refreshForm() {
    const auto pack = packName();
    const int layer = navigation_.last().layer;
    QString host;
    if (navigation_.size() > 1)
        host = QString::fromUtf8(elementConfig(editorPackRoot(pack), pack, navigation_.last().name).sub);
    const bool available = !diagram_.errorLine && (pack == "cpp" || pack == "_base" || pack == "delphi") &&
        form_->setLayer(diagram_, layer, pack, host);
    ui_.formScroll->setVisible(available);
    ui_.formHint->setText(available ? trEditor("Drag a control to move it. Drag a selection handle to resize.") :
                                     trEditor("This scheme has no editable form."));
    if (available) {
        const auto selected = scene_->selectedItems();
        form_->selectElement(selected.size() == 1 ? selected.first()->data(0).toInt() : -1);
    }
    emit selectionChanged();
}

void SchemeEditor::commitGeometry(int index, const QRect &rect, bool form, int layer, const QRect &before) {
    const bool sdkFrame = layer < 0 && elementItems_.at(index)->data(FrameRole).toBool();
    const auto &element = diagram_.layers.at(layer < 0 ? navigation_.last().layer : layer).elements.at(index);
    QList<QPair<QString, int>> values{{"Width", rect.width()}, {"Height", rect.height()}};
    if (!form) { values.append({"Left", rect.x()}); values.append({"Top", rect.y()}); }
    if (before.isValid()) {
        const QHash<QString,int> previous{{"Width",before.width()},{"Height",before.height()},{"Left",before.x()},{"Top",before.y()}};
        values.removeIf([&](const auto &value) { return previous.value(value.first) == value.second; });
    }
    QList<QPair<SchemeTextSpan,QString>> edits;
    QString added;
    for (const auto &entry : values) {
        const QString &key = entry.first;
        const int value = entry.second;
        auto p = std::find_if(element.properties.cbegin(), element.properties.cend(),
                              [&](const auto &entry) { return entry.first == key ||
                                  (sdkFrame && entry.first == u'@' + key); });
        if (p == element.properties.cend()) added += " " + key + "=" + QString::number(value) + "\n";
        else if (p->second.toInt() != value)
            edits.append({element.propertyValues.at(p - element.properties.cbegin()), QString::number(value)});
    }
    if (!added.isEmpty()) edits.append({{element.bodyEnd, 0}, added});
    std::sort(edits.begin(), edits.end(), [](const auto &a,const auto &b){ return a.first.start > b.first.start; });
    QTextCursor cursor(document());
    cursor.beginEditBlock();
    for (const auto &[span,value] : edits) {
        cursor.setPosition(int(span.start));
        cursor.setPosition(int(span.start + span.length), QTextCursor::KeepAnchor);
        cursor.insertText(value);
    }
    cursor.endEditBlock();
}

void SchemeEditor::commitPositions() {
    if (ui_.editorViews->currentWidget() != ui_.diagramPage || diagram_.errorLine || diagramDirty_)
        return;
    const auto &elements = diagram_.layers.at(navigation_.last().layer).elements;
    QTextCursor cursor(document());
    cursor.beginEditBlock();
    auto replace = [&cursor](const SchemeTextSpan &span, qreal value) {
        cursor.setPosition(static_cast<int>(span.start));
        cursor.setPosition(static_cast<int>(span.start + span.length), QTextCursor::KeepAnchor);
        cursor.insertText(QString::number(qRound64(value)));
    };
    // Work backwards so text replacements cannot shift the remaining offsets.
    for (qsizetype index = elements.size(); index-- > 0;) {
        const auto &element = elements.at(index);
        const auto newPosition = elementItems_.at(index)->pos();
        if (!qFuzzyCompare(newPosition.y() + 1, element.position.y() + 1))
            replace(element.yText, newPosition.y());
        if (!qFuzzyCompare(newPosition.x() + 1, element.position.x() + 1))
            replace(element.xText, newPosition.x());
    }
    cursor.endEditBlock();
}

bool SchemeEditor::portUsed(int id, const QString &name) const {
    const auto &layer = diagram_.layers.at(navigation_.last().layer);
    return std::any_of(layer.connections.cbegin(), layer.connections.cend(), [&](const auto &link) {
        return isSourcePort(layer, link, id, name) || isTargetPort(link, id, name);
    });
}

bool SchemeEditor::connectPorts(int fromId, const QString &fromPoint, int toId, const QString &toPoint) {
    return connectPorts(fromId, fromPoint, toId, toPoint, -1);
}

bool SchemeEditor::connectPorts(int fromId, const QString &fromPoint, int toId, const QString &toPoint, int replaced) {
    if (diagramDirty_ || diagram_.errorLine) return false;
    const auto &layer = diagram_.layers.at(navigation_.last().layer);
    const auto *old = replaced < 0 ? nullptr : &layer.connections.at(replaced);
    const bool sameSource = old && isSourcePort(layer, *old, fromId, fromPoint);
    const bool sameTarget = old && isTargetPort(*old, toId, toPoint);
    if (sameSource && sameTarget) return true;
    if ((!sameSource && portUsed(fromId, fromPoint)) || (!sameTarget && portUsed(toId, toPoint))) return false;
    const auto &elements = layer.elements;
    auto find = [&](int id) { return std::find_if(elements.cbegin(), elements.cend(), [id](const auto &e) { return e.id == id; }); };
    const auto from = find(fromId), to = find(toId);
    if (from == elements.cend() || to == elements.cend()) return false;
    const auto pack = packName(), root = editorPackRoot(pack);
    auto kind = [&](const SchemeElement &element, const QString &point) {
        for (const auto &port : schemeElementPorts(root, pack, diagram_, element))
            if (QString::fromUtf8(port.name).compare(point, Qt::CaseInsensitive) == 0) return port.kind;
        return 0;
    };
    const int a = kind(*from, fromPoint), b = kind(*to, toPoint);
    if (!((a == pt_Event && b == pt_Work) || (a == pt_Data && b == pt_Var))) return false;

    const auto line = QStringLiteral(" link(%1,%2:%3,[])\n").arg(fromPoint).arg(toId).arg(toPoint);
    QTextCursor cursor(document());
    cursor.setPosition(int(from->bodyEnd));
    if (!old) {
        cursor.insertText(line);
        return true;
    }
    if (elements.at(old->source).id == fromId) {
        cursor.setPosition(int(old->line.start));
        cursor.setPosition(int(old->line.start + old->line.length), QTextCursor::KeepAnchor);
        cursor.insertText(line);
        return true;
    }
    cursor.beginEditBlock();
    removeConnection(replaced);
    cursor.insertText(line);
    cursor.endEditBlock();
    return true;
}

// A numbered SDK frame can grow its public interface when a wire is dropped on its edge.
bool SchemeEditor::connectFrameEdge(QGraphicsRectItem *frame, const QPointF &at, const WireDrag &drag) {
    const auto &elements = diagram_.layers.at(navigation_.last().layer).elements;
    const auto &entry = elements.at(frame->data(0).toInt());
    if (entry.id == drag.anchorId || portUsed(drag.anchorId, drag.anchorPort)) return false;
    const auto pack = packName();
    const auto config = elementConfig(editorPackRoot(pack), pack, entry.name);
    if (config.className != "EditMulti" && config.className != "HCEditMulti") return false;
    const auto pos = frame->mapFromScene(at);
    const char *key, *prefix;
    switch (drag.anchorKind) {
    case pt_Event:
        if (qAbs(pos.x() - frame->rect().right()) > 4) return false;
        key = "EventCount"; prefix = "onEvent"; break;
    case pt_Work:
        if (qAbs(pos.x()) > 4) return false;
        key = "WorkCount"; prefix = "doWork"; break;
    case pt_Var:
        if (qAbs(pos.y() - frame->rect().bottom()) > 4) return false;
        key = "VarCount"; prefix = "Var"; break;
    case pt_Data:
        if (qAbs(pos.y()) > 4) return false;
        key = "DataCount"; prefix = "Data"; break;
    default: return false;
    }
    int count = 0;
    for (const auto &p : config.properties) if (p.name == key) count = p.initial.toInt();
    const auto property = std::find_if(entry.properties.begin(), entry.properties.end(),
        [&](const auto &p) { return p.first == QLatin1String(key); });
    if (property != entry.properties.end()) count = property->second.toInt();
    if (count < 0 || count >= 65535) return false;
    const auto source = std::find_if(elements.begin(), elements.end(), [&](const auto &e) { return e.id == drag.anchorId; });
    if (source == elements.end()) return false;
    const auto next = QString::number(count + 1);
    const auto name = QLatin1String(prefix) + next;
    const bool outgoing = drag.anchorKind == pt_Event || drag.anchorKind == pt_Data;
    const auto link = QStringLiteral(" link(%1,%2:%3,[])\n")
        .arg(outgoing ? drag.anchorPort : name).arg(outgoing ? entry.id : source->id).arg(outgoing ? name : drag.anchorPort);
    QTextCursor cursor(document());
    cursor.beginEditBlock();
    // A separate cursor tracks the link insertion point as the count changes.
    QTextCursor insertLink(document());
    insertLink.setPosition(int(outgoing ? source->bodyEnd : entry.bodyEnd));
    if (property == entry.properties.end()) {
        cursor.setPosition(int(entry.bodyEnd));
        cursor.insertText(u' ' + QLatin1String(key) + u'=' + next + u'\n');
    } else {
        const auto span = entry.propertyValues.at(property - entry.properties.begin());
        cursor.setPosition(int(span.start));
        cursor.setPosition(int(span.start + span.length), QTextCursor::KeepAnchor);
        cursor.insertText(next);
    }
    insertLink.insertText(link);
    cursor.endEditBlock();
    return true;
}

void SchemeEditor::removeConnection(int index) {
    const auto span = diagram_.layers.at(navigation_.last().layer).connections.at(index).line;
    QTextCursor cursor(document());
    cursor.setPosition(int(span.start));
    cursor.setPosition(int(span.start + span.length), QTextCursor::KeepAnchor);
    cursor.removeSelectedText();
}

void SchemeEditor::beginWireDrag(QGraphicsItem *point, const QPoint &start) {
    cancelWireDrag();
    const auto &layer = diagram_.layers.at(navigation_.last().layer);
    int id = layer.elements.at(point->parentItem()->data(0).toInt()).id;
    auto name = point->data(2).toString();
    int kind = point->data(3).toInt();
    auto anchor = point->scenePos();
    const auto links = connectionsAtPort(layer, id, name);
    if (links.size() > 1) return; // Choose a specific wire from the point's menu.
    const int connection = links.isEmpty() ? -1 : links.first();
    if (connection >= 0) {
        const auto &link = layer.connections.at(connection);
        const bool source = isSourcePort(layer, link, id, name);
        id = source ? link.targetId : layer.elements.at(link.source).id;
        name = source ? link.targetPoint : link.sourcePoint;
        kind = kind % 2 ? kind + 1 : kind - 1;
        if (auto *wire = wires_.at(connection)) anchor = wire->path().pointAtPercent(source ? 1 : 0);
    }
    const QPen pen(palette().color(QPalette::Highlight), 2, Qt::DashLine);
    auto *preview = scene_->addLine(QLineF(anchor, anchor), pen);
    preview->setZValue(-2);
    preview->setVisible(connection < 0);
    wireDrag_ = WireDrag{preview, id, name, kind, connection, start};
}

void SchemeEditor::cancelWireDrag() {
    if (!wireDrag_) return;
    if (auto *wire = wires_.value(wireDrag_->connection)) wire->show();
    delete wireDrag_->preview;
    wireDrag_.reset();
}

void SchemeEditor::finishWireDrag(const QPoint &position) {
    const auto drag = *wireDrag_;
    auto *point = ui_.canvas->itemAt(position);
    if (point == drag.preview) point = nullptr;
    cancelWireDrag();
    if (!ui_.canvas->viewport()->rect().contains(position)) return;
    if (drag.connection >= 0 && (position - drag.start).manhattanLength() < QApplication::startDragDistance()) return;

    if (point && point->data(2).isValid()) {
        const auto &layer = diagram_.layers.at(navigation_.last().layer);
        const int id = layer.elements.at(point->parentItem()->data(0).toInt()).id;
        const auto name = point->data(2).toString();
        if (drag.anchorKind == pt_Event || drag.anchorKind == pt_Data)
            connectPorts(drag.anchorId, drag.anchorPort, id, name, drag.connection);
        else
            connectPorts(id, name, drag.anchorId, drag.anchorPort, drag.connection);
    } else if (point && point->data(FrameRole).toBool() && drag.connection < 0) {
        connectFrameEdge(static_cast<QGraphicsRectItem *>(point), ui_.canvas->mapToScene(position), drag);
    } else if (!point && drag.connection >= 0) {
        removeConnection(drag.connection);
    }
}

void SchemeEditor::deleteSelection() {
    if (diagramDirty_ || diagram_.errorLine || scene_->selectedItems().isEmpty()) return;
    const auto &layer = diagram_.layers.at(navigation_.last().layer);
    QList<SchemeTextSpan> spans;
    QSet<int> ids;
    for (auto *item : scene_->selectedItems()) {
        if (item->data(FrameRole).toBool() || (isFormView() && item->data(0).toInt() == form_->hostElement())) continue;
        const auto &e = layer.elements.at(item->data(0).toInt());
        ids.insert(e.id);
        spans.append(e.declaration);
    }
    if (ids.isEmpty()) return;
    for (const auto &link : layer.connections)
        if (!ids.contains(layer.elements.at(link.source).id) && ids.contains(link.targetId))
            spans.append(link.line);
    std::sort(spans.begin(), spans.end(), [](auto a, auto b) { return a.start > b.start; });
    scene_->clearSelection();
    QTextCursor cursor(document());
    cursor.beginEditBlock();
    for (const auto &span : spans) {
        cursor.setPosition(int(span.start));
        cursor.setPosition(int(span.start + span.length), QTextCursor::KeepAnchor);
        cursor.removeSelectedText();
    }
    cursor.endEditBlock();
}

void SchemeEditor::showSourceLine(int line) {
    ui_.editorViews->setCurrentWidget(ui_.sourcePage);
    const auto block = document()->findBlockByNumber(qMax(0, line - 1));
    if (block.isValid()) ui_.sourceEditor->setTextCursor(QTextCursor(block));
    ui_.sourceEditor->setFocus();
}

std::optional<int> SchemeEditor::buildMode() const {
    if (packName() != "cpp") return {};
    const auto diagram = readSchemeDiagram(source());
    if (diagram.errorLine || diagram.layers.isEmpty()) return {};
    for (const auto &element : diagram.layers.first().elements) {
        if (projectType(element.name, "cpp") == ProjectType::Unknown) continue;
        const auto config = elementConfig(editorPackRoot("cpp"), "cpp", element.name);
        for (const auto &p : config.properties) {
            if (p.name != "BuildMode") continue;
            for (const auto &[name, value] : element.properties)
                if (name == "BuildMode") return value.toInt();
            return p.initial.toInt();
        }
        break;
    }
    return {};
}

void SchemeEditor::setBuildMode(int mode) {
    const auto current = buildMode();
    if (!current || *current == mode) return;
    const auto diagram = readSchemeDiagram(source());
    for (const auto &element : diagram.layers.first().elements) {
        if (projectType(element.name, "cpp") == ProjectType::Unknown) continue;
        QTextCursor cursor(document());
        for (int i = 0; i < element.properties.size(); ++i) {
            if (element.properties.at(i).first != "BuildMode") continue;
            const auto span = element.propertyValues.at(i);
            cursor.setPosition(int(span.start));
            cursor.setPosition(int(span.start + span.length), QTextCursor::KeepAnchor);
            cursor.insertText(QString::number(mode));
            return;
        }
        cursor.setPosition(int(element.bodyEnd));
        cursor.insertText(QStringLiteral(" BuildMode=%1\n").arg(mode));
        return;
    }
}

void SchemeEditor::canvasMenu(const QPoint &point) {
    if (!placement_.isEmpty()) { cancelPlacement(); return; }
    if (wireDrag_) { cancelWireDrag(); return; }
    if (diagramDirty_ || diagram_.errorLine) return;
    auto *hit = ui_.canvas->itemAt(point);
    if (!hit) return;
    QMenu menu(this);
    const auto &layer = diagram_.layers.at(navigation_.last().layer);
    if (auto *item = elementItem(hit)) {
        if (!item->isSelected()) { scene_->clearSelection(); item->setSelected(true); }
        const auto &e = layer.elements.at(item->data(0).toInt());
        const int id = e.id;
        if (hit->data(2).isValid()) {
            const auto links = connectionsAtPort(layer, id, hit->data(2).toString());
            if (links.size() == 1) {
                menu.addAction(trEditor("Remove connection"), this, [this, index = links.first()] { removeConnection(index); });
            } else if (!links.isEmpty()) {
                auto *remove = menu.addMenu(trEditor("Remove connection"));
                for (int index : links) {
                    const auto &wire = layer.connections.at(index);
                    const auto label = QStringLiteral("%1.%2 → %3.%4").arg(layer.elements.at(wire.source).id)
                        .arg(wire.sourcePoint).arg(wire.targetId).arg(wire.targetPoint);
                    remove->addAction(label, this, [this, index] { removeConnection(index); });
                }
            }
        }
        const auto pack = packName();
        const auto root = editorPackRoot(pack);
        auto *start = menu.addMenu(trEditor("Start connection"));
        auto *finish = menu.addMenu(trEditor("Finish connection"));
        int requiredKind = 0;
        if (!pendingPort_.isEmpty()) {
            for (const auto &source : layer.elements) if (source.id == pendingSource_) {
                for (const auto &p : schemeElementPorts(root, pack, diagram_, source))
                    if (QString::fromUtf8(p.name) == pendingPort_) requiredKind = p.kind == pt_Event ? pt_Work : pt_Var;
                break;
            }
        }
        for (const auto &p : schemeElementPorts(root, pack, diagram_, e)) {
            const auto name = QString::fromUtf8(p.name);
            if (portUsed(id, name)) continue;
            if (p.kind == pt_Event || p.kind == pt_Data)
                start->addAction(name, this, [this, id, name] { pendingSource_ = id; pendingPort_ = name; });
            else if (requiredKind && p.kind == requiredKind)
                finish->addAction(name, this, [this, id, name] { connectPorts(pendingSource_, pendingPort_, id, name); });
        }
        start->setEnabled(!start->isEmpty());
        finish->setEnabled(!finish->isEmpty());
        if (!item->data(FrameRole).toBool()) menu.addAction(trEditor("Delete element"), this, &SchemeEditor::deleteSelection);
    } else if (hit->data(1).isValid()) {
        menu.addAction(trEditor("Remove connection"), this, [this, index = hit->data(1).toInt()] { removeConnection(index); });
    }
    if (!menu.isEmpty()) menu.exec(ui_.canvas->viewport()->mapToGlobal(point));
}

void SchemeEditor::changeEvent(QEvent *event) {
    QWidget::changeEvent(event);
    if (event->type() == QEvent::PaletteChange && !diagramDirty_)
        drawLayer(true);
}

bool SchemeEditor::eventFilter(QObject *watched, QEvent *event) {
    if (event->type() == QEvent::KeyPress && static_cast<QKeyEvent *>(event)->key() == Qt::Key_Escape &&
        (!placement_.isEmpty() || form_->placementArmed() || wireDrag_)) {
        cancelPlacement();
        cancelWireDrag();
        return true;
    }
    if (watched == ui_.canvas->viewport()) {
        if (!placement_.isEmpty()) {
            if (event->type() == QEvent::Enter || event->type() == QEvent::MouseMove) {
                ui_.canvas->viewport()->setCursor(placementCursor_);
                if (event->type() == QEvent::MouseMove) return true;
            }
            if (event->type() == QEvent::MouseButtonPress) {
                const auto *mouse = static_cast<QMouseEvent *>(event);
                if (mouse->button() == Qt::LeftButton) {
                    const auto name = placement_;
                    cancelPlacement();
                    insertElement(name, ui_.canvas->mapToScene(mouse->position().toPoint()).toPoint());
                    return true;
                }
            }
        }
        if (event->type() == QEvent::MouseButtonPress && !diagram_.errorLine) {
            auto *mouse = static_cast<QMouseEvent *>(event);
            auto *point = ui_.canvas->itemAt(mouse->position().toPoint());
            if (mouse->button() == Qt::LeftButton && point && point->data(FrameRole).toBool()) {
                auto *frame = static_cast<QGraphicsRectItem *>(point);
                const auto pos = frame->mapFromScene(ui_.canvas->mapToScene(mouse->position().toPoint()));
                resizeEdges_ = {};
                if (qAbs(pos.x() - frame->rect().right()) <= 4) resizeEdges_ |= Qt::RightEdge;
                if (qAbs(pos.y() - frame->rect().bottom()) <= 4) resizeEdges_ |= Qt::BottomEdge;
                if (resizeEdges_) {
                    scene_->clearSelection();
                    frame->setSelected(true);
                    resizeIndex_ = frame->data(0).toInt();
                    resizePreview_ = scene_->addRect(frame->mapRectToScene(frame->rect()),
                        QPen(palette().color(QPalette::Highlight), 1, Qt::DashLine));
                    return true;
                }
            }
            if (mouse->button() == Qt::LeftButton && point && point->data(2).isValid()) {
                beginWireDrag(point, mouse->position().toPoint());
                return true;
            }
        } else if (resizePreview_ && (event->type() == QEvent::MouseMove || event->type() == QEvent::MouseButtonRelease)) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            const auto *item = elementItems_.at(resizeIndex_);
            const auto pos = item->mapFromScene(ui_.canvas->mapToScene(mouse->position().toPoint()));
            const auto minimum = item->data(MinimumFrameSizeRole).toSize();
            auto rect = item->rect().toRect();
            if (resizeEdges_.testFlag(Qt::RightEdge)) rect.setWidth(qMax(minimum.width(), qRound(pos.x())));
            if (resizeEdges_.testFlag(Qt::BottomEdge)) rect.setHeight(qMax(minimum.height(), qRound(pos.y())));
            resizePreview_->setRect(item->mapRectToScene(rect));
            if (event->type() == QEvent::MouseButtonRelease && mouse->button() == Qt::LeftButton) {
                delete resizePreview_;
                resizePreview_ = nullptr;
                if (rect != item->rect().toRect()) commitGeometry(resizeIndex_, rect, true);
            }
            return true;
        } else if (wireDrag_ && event->type() == QEvent::MouseMove) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            const auto position = mouse->position().toPoint();
            auto &drag = *wireDrag_;
            if (!drag.preview->isVisible() && (position - drag.start).manhattanLength() >= QApplication::startDragDistance()) {
                if (auto *wire = wires_.value(drag.connection)) wire->hide();
                drag.preview->show();
            }
            drag.preview->setLine(QLineF(drag.preview->line().p1(), ui_.canvas->mapToScene(position)));
            return true;
        } else if (event->type() == QEvent::MouseButtonDblClick && !diagram_.errorLine) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            auto *item = elementItem(ui_.canvas->itemAt(mouse->position().toPoint()));
            if (mouse->button() == Qt::LeftButton && item) {
                const auto &element = diagram_.layers.at(navigation_.last().layer).elements.at(item->data(0).toInt());
                if (element.child >= 0) {
                    enterChild(item->data(0).toInt());
                    return true;
                }
                if (!item->isSelected() || scene_->selectedItems().size() != 1) {
                    scene_->clearSelection();
                    item->setSelected(true);
                }
                emit primaryPropertyRequested();
                return true;
            }
        } else if (event->type() == QEvent::MouseButtonRelease) {
            const auto *mouse = static_cast<QMouseEvent *>(event);
            if (mouse->button() == Qt::LeftButton && wireDrag_) {
                finishWireDrag(mouse->position().toPoint());
                return true;
            }
            if (mouse->button() == Qt::LeftButton)
                QTimer::singleShot(0, this, &SchemeEditor::commitPositions);
        } else if (event->type() == QEvent::Wheel) {
            const auto *wheel = static_cast<QWheelEvent *>(event);
            if (wheel->modifiers().testFlag(Qt::ControlModifier) && wheel->angleDelta().y() != 0) {
                const auto step = wheel->angleDelta().y() > 0 ? 1.2 : 1 / 1.2;
                ui_.zoomSlider->setValue(qRound(ui_.zoomSlider->value() * step));
                return true;
            }
        }
    }
    return QWidget::eventFilter(watched, event);
}
