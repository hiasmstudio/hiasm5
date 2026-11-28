#include "form_canvas.h"
#include "element_config.h"

#include <QBuffer>
#include <QCheckBox>
#include <QComboBox>
#include <QDateTimeEdit>
#include <QFontDatabase>
#include <QDragEnterEvent>
#include <QtWidgets/qdrawutil.h>
#include <QFrame>
#include <QGroupBox>
#include <QImageReader>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMimeData>
#include <QMouseEvent>
#include <QMenu>
#include <QPainter>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QProgressBar>
#include <QRadioButton>
#include <QRegularExpression>
#include <QStyleOptionTitleBar>
#include <QStyleFactory>
#include <QWindow>
#include <QScrollBar>
#include <QShortcut>
#include <QSlider>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QToolBar>
#include <QTreeWidget>
#include <QtEndian>

namespace {
using Values = QHash<QString,QString>;

QString displayString(QString value) {
    QStringList lines;
    if (decodeSchemeLines(value, lines)) return lines.join(u'\n');
    if (value.size() >= 2 && value.front() == '"' && value.back() == '"') value = value.mid(1, value.size() - 2);
    return value.replace(QStringLiteral("\\r\\n"), QStringLiteral("\n"));
}

Values formProperties(const SchemeElement &element, const ElementConfig &config) {
    Values values;
    for (const auto &p : config.properties) values.insert(QString::fromUtf8(p.name), QString::fromUtf8(p.initial));
    for (const auto &[name, value] : element.properties) values.insert(name, value);
    return values;
}

QString previewKind(const QString &name, const ElementConfig &config) {
    if (name == "StringTable" || name == "StringTableMT") return name;
    if (name == "AlphaImage" || name == "ImgBtn" || name == "Flash") return name;
    if (name == "LED" || name == "FontBox" || name == "BitBtn" || name == "DatePicker" || name == "Image") return name;
    if (!config.editClass.isEmpty()) return QString::fromUtf8(config.editClass);
    return config.className.startsWith("HCWE_") ? QString::fromUtf8(config.className.mid(5)) : name;
}

void applyFormFont(QWidget *widget, const Values &values, bool root = false) {
    if (!root && values.value("ParentFont", "1") == "0") return;
    const auto spec = values.value("Font");
    if (!spec.startsWith('[') || !spec.endsWith(']')) return;
    const auto fields = spec.mid(1, spec.size()-2).split(',');
    const auto size = fields.value(1).toInt();
    if (fields.size() < 3 || size < 1 || size > 256) return;
    QFont font(fields.first(), size);
    const int flags = fields.at(2).toInt();
    font.setBold(flags & 1);
    font.setItalic(flags & 2);
    font.setUnderline(flags & 4);
    font.setStrikeOut(flags & 8);
    widget->setFont(font);
}

class PanelPreview final : public QWidget {
public:
    PanelPreview(const Values &values, QWidget *parent) : QWidget(parent), values_(values) {
        setAutoFillBackground(true);
    }

protected:
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        QRect area = rect();
        const auto bevel = [&](int style) {
            const int width = qBound(0, values_.value("BevelWidth").toInt(), qMax(0, qMin(area.width(), area.height()) / 2));
            if (!style || !width) return;
            qDrawShadeRect(&painter, area, palette(), style == 1, width);
            area.adjust(width, width, -width, -width);
        };
        bevel(values_.value("BevelOuter").toInt());
        if (values_.value("BorderStyle").toInt() && !area.isEmpty()) {
            painter.setPen(palette().color(QPalette::Dark));
            painter.drawRect(area.adjusted(0, 0, -1, -1));
            area.adjust(1, 1, -1, -1);
        }
        const int padding = qBound(0, values_.value("BorderWidth").toInt(), qMax(0, qMin(area.width(), area.height()) / 2));
        area.adjust(padding, padding, -padding, -padding);
        bevel(values_.value("BevelInner").toInt());
        const Qt::Alignment align[] = {Qt::AlignLeft, Qt::AlignRight, Qt::AlignHCenter};
        painter.setPen(palette().color(QPalette::WindowText));
        painter.drawText(area.adjusted(2, 0, -2, 0), align[qBound(0, values_.value("Alignment").toInt(), 2)] |
            Qt::AlignVCenter, displayString(values_.value("Caption")));
    }

private:
    const Values values_;
};

QRect geometryFor(const Values &v) {
    return {v.value("Left").toInt(), v.value("Top").toInt(),
            qBound(1, v.value("Width").toInt(), 16384), qBound(1, v.value("Height").toInt(), 16384)};
}

int hostIndex(const SchemeLayer &layer, const QString &hostName) {
    for (int i = 0; i < layer.elements.size(); ++i) {
        const auto &e = layer.elements.at(i);
        if (!hostName.isEmpty() && e.name == hostName) return i;
        if (hostName.isEmpty() && (e.name == "MainForm" || e.name == "hcTplForm" ||
            e.name == "hcTplDialog" || e.name == "Form" || e.name == "Dialog")) return i;
    }
    return -1;
}

QImage legacyBitmap(QStringView hex) {
    // HiAsm stores each BMP as a little-endian length followed by a zlib stream.
    if (hex.size() < 10 || hex.size() > 2 * (4 * 1024 * 1024 + 65536) || hex.size() % 2) return {};
    auto bytes = QByteArray::fromHex(hex.toLatin1());
    if (bytes.size() < 5) return {};
    const auto length = qFromLittleEndian<quint32>(bytes.constData());
    if (!length || length > 4 * 1024 * 1024) return {};
    qToBigEndian(length, bytes.data());
    auto bitmap = qUncompress(bytes);
    if (quint32(bitmap.size()) != length) return {};
    QBuffer buffer(&bitmap);
    buffer.open(QIODevice::ReadOnly);
    QImageReader reader(&buffer, "bmp");
    const QSize size = reader.size();
    if (size.isEmpty() || size.width() > 1024 || size.height() > 1024) return {};
    return reader.read();
}

QIcon toolbarIcon(QStringView hex) {
    return QIcon(QPixmap::fromImage(legacyBitmap(hex)));
}

QWidget *previewControl(const QString &kind, const Values &v, QWidget *parent) {
    const auto caption = displayString(v.value("Caption"));
    if (kind == "Image" || kind == "AlphaImage" || kind == "ImgBtn") {
        class ImagePreview final : public QWidget {
        public:
            ImagePreview(QImage bitmap, int mode, bool smooth, qreal opacity, QWidget *parent)
                : QWidget(parent), bitmap_(std::move(bitmap)), mode_(mode), smooth_(smooth), opacity_(opacity) {}

        protected:
            void paintEvent(QPaintEvent *) override {
                if (bitmap_.isNull()) return;
                QPainter painter(this);
                painter.setOpacity(opacity_);
                painter.setRenderHint(QPainter::SmoothPixmapTransform, smooth_);
                if (mode_ == 3) {
                    painter.drawTiledPixmap(rect(), QPixmap::fromImage(bitmap_));
                    return;
                }
                auto target = bitmap_.size();
                if (mode_ == 1) target = size();
                if (mode_ == 2) target.scale(size(), Qt::KeepAspectRatio);
                if (mode_ == 5) target.scale(size(), Qt::KeepAspectRatioByExpanding);
                QRect area(QPoint(), target);
                if (mode_ != 4) area.moveCenter(rect().center());
                painter.drawImage(area, bitmap_);
            }

        private:
            const QImage bitmap_;
            const int mode_;
            const bool smooth_;
            const qreal opacity_;
        };
        const bool button = kind == "ImgBtn";
        const bool useOpacity = kind == "AlphaImage" || (button && v.value("Alpha", "1") == "0");
        const auto picture = v.value(button ? "Normal" : "Picture");
        auto bitmap = picture.startsWith("[ZIP") && picture.endsWith(']')
            ? legacyBitmap(QStringView(picture).mid(4, picture.size() - 5))
            : QImage::fromData(QByteArray::fromBase64(picture.toLatin1()), "PNG");
        if ((kind == "AlphaImage" && v.value("AlphaMode", "0") != "0") || (button && !useOpacity))
            bitmap = bitmap.convertToFormat(QImage::Format_RGB32);
        const qreal opacity = useOpacity ? qBound(0, v.value("AlphaBlendValue", "255").toInt(), 255) / 255.0 : 1.0;
        return new ImagePreview(std::move(bitmap), button ? 4 : v.value("ViewStyle", "2").toInt(),
                                v.value("HalfTone", "0") == "0", opacity, parent);
    }
    if (kind == "LED") {
        class LedPreview final : public QWidget {
        public:
            LedPreview(const Values &values, QWidget *parent) : QWidget(parent), values_(values) {}

        protected:
            void paintEvent(QPaintEvent *) override {
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing);
                const QRectF area = QRectF(rect()).adjusted(1, 1, -1, -1);
                const bool lit = values_.value("Value", "1") == "0";
                const auto color = readColor(lit ? "ColorOn" : "ColorOff", lit ? QColor(Qt::red) : QColor(128, 0, 0));
                painter.setBrush(color);
                painter.setPen(color.darker());
                if (values_.value("Shape").toInt() == 0) painter.drawEllipse(area);
                else painter.drawRect(area);
                if (values_.value("Blick", "0") == "0") {
                    painter.setPen(Qt::NoPen);
                    painter.setBrush(readColor("ColorBlick", Qt::white));
                    painter.setOpacity(0.65);
                    painter.drawEllipse(QRectF(area.left() + area.width() * 0.23, area.top() + area.height() * 0.20,
                                               area.width() * 0.28, area.height() * 0.17));
                }
            }

        private:
            QColor readColor(const char *key, const QColor &fallback) const {
                auto value = values_.value(QString::fromLatin1(key));
                if (value.startsWith("cl", Qt::CaseInsensitive)) value.remove(0, 2);
                const auto named = QColor::fromString(value.toLower());
                if (named.isValid()) return named;
                bool ok = false;
                const auto rgb = value.toUInt(&ok);
                return ok ? QColor(rgb & 255, (rgb >> 8) & 255, (rgb >> 16) & 255) : fallback;
            }

            const Values values_;
        };
        return new LedPreview(v, parent);
    }
    if (kind == "Button" || kind == "BitBtn") {
        auto *button = new QPushButton(caption, parent);
        if (kind == "BitBtn") {
            const auto bitmap = v.value("Bitmap");
            if (bitmap.startsWith("[ZIP") && bitmap.endsWith(']'))
                button->setIcon(toolbarIcon(QStringView(bitmap).mid(4, bitmap.size() - 5)));
            button->setFlat(v.value("Flat", "0") == "0");
        }
        return button;
    }
    if (kind == "CheckBox") {
        auto *check = new QCheckBox(caption, parent);
        check->setChecked(v.value("Checked").toInt() == 0);
        return check;
    }
    if (kind == "Edit") return new QLineEdit(displayString(v.value("Text")), parent);
    if (kind == "RadioButton") {
        auto *radio = new QRadioButton(caption, parent);
        radio->setChecked(v.value("Selected", "1").toInt() == 0);
        return radio;
    }
    if (kind == "DatePicker") {
        auto *picker = new QDateTimeEdit(QDateTime::currentDateTime(), parent);
        const bool time = v.value("Time", "1") == "0";
        picker->setDisplayFormat(time ? picker->locale().timeFormat(QLocale::ShortFormat) :
            picker->locale().dateFormat(v.value("DateFormat") == "1" ? QLocale::LongFormat : QLocale::ShortFormat));
        picker->setCalendarPopup(!time && v.value("DateMode").toInt() == 0);
        return picker;
    }
    if (kind == "FontBox") {
        auto *fonts = new QComboBox(parent);
        fonts->addItem(QStringLiteral("???"));
        auto families = QFontDatabase::families();
        families.sort(Qt::CaseInsensitive);
        fonts->addItems(families);
        fonts->setCurrentIndex(qMax(0, fonts->findText(displayString(v.value("SelFont")), Qt::MatchFixedString)));
        return fonts;
    }
    if (kind == "ComboBox") {
        auto *combo = new QComboBox(parent);
        combo->addItems(displayString(v.value("Strings")).split(u'\n', Qt::SkipEmptyParts));
        return combo;
    }
    if (kind == "StringTable" || kind == "StringTableMT") {
        auto *table = new QTableWidget(parent);
        table->verticalHeader()->hide();
        table->horizontalHeader()->setMinimumSectionSize(0);
        table->setShowGrid(v.value("Grid", "1") == "0");
        const bool extended = kind == "StringTableMT";
        if (extended && v.value("Style") == "4") table->horizontalHeader()->hide();
        const auto definitions = displayString(v.value("Columns")).split(u'\n', Qt::SkipEmptyParts);
        for (auto definition : definitions) {
            const auto fields = definition.split(u'=');
            const int defaultWidth = extended ? qMax(0, v.value("ColumnWidth", "80").toInt()) : 80;
            const int width = fields.size() > 1 ? qMax(0, fields.at(1).toInt()) : defaultWidth;
            const int column = table->columnCount();
            table->insertColumn(column);
            table->setHorizontalHeaderItem(column, new QTableWidgetItem(fields.first()));
            table->setColumnWidth(column, width);
        }
        return table;
    }
    if (kind == "ListBox") {
        auto *list = new QListWidget(parent);
        list->addItems(displayString(v.value("Strings")).split(u'\n', Qt::SkipEmptyParts));
        return list;
    }
    if (kind == "TreeView") {
        auto *tree = new QTreeWidget(parent);
        tree->setHeaderHidden(true);
        return tree;
    }
    if (kind == "Progress" || kind == "ProgressBar") {
        auto *bar = new QProgressBar(parent);
        bar->setOrientation(v.value("Kind").toInt() == 1 ? Qt::Vertical : Qt::Horizontal);
        if (kind == "ProgressBar") bar->setTextVisible(false);
        bar->setMaximum(v.value("Max", "100").toInt());
        bar->setValue(v.value("Position").toInt());
        return bar;
    }
    if (kind == "ScrollBar" || kind == "TrackBar") {
        const auto orientation = v.value("Kind").toInt() == 1 ? Qt::Vertical : Qt::Horizontal;
        QAbstractSlider *slider = kind == "ScrollBar" ? static_cast<QAbstractSlider *>(new QScrollBar(orientation, parent))
                                                     : static_cast<QAbstractSlider *>(new QSlider(orientation, parent));
        slider->setRange(v.value("Min", "0").toInt(), v.value("Max", "100").toInt());
        slider->setValue(v.value("Position").toInt());
        return slider;
    }
    if (kind == "UpDown") {
        auto *spin = new QSpinBox(parent);
        spin->setRange(v.value("Min", "0").toInt(), v.value("Max", "10").toInt());
        spin->setSingleStep(qMax(1, v.value("Step", "1").toInt()));
        spin->setValue(v.value("Position").toInt());
        return spin;
    }
    if (kind == "Tab") {
        auto *tabs = new QTabWidget(parent);
        const bool vertical = v.value("Vertical", "1") == "0";
        const bool bottom = v.value("Bottom", "1") == "0";
        tabs->setTabPosition(vertical ? (bottom ? QTabWidget::East : QTabWidget::West)
                                     : (bottom ? QTabWidget::South : QTabWidget::North));
        tabs->setDocumentMode(v.value("Border", "1") != "0");
        QList<QIcon> icons;
        static const QRegularExpression bitmapPattern(QStringLiteral("\\[ZIP([0-9A-Fa-f]+)\\]"));
        const auto bitmaps = v.value("Bitmaps");
        for (auto matches = bitmapPattern.globalMatch(bitmaps); matches.hasNext();) {
            const auto match = matches.next();
            icons.append(toolbarIcon(match.capturedView(1)));
        }
        const auto titles = displayString(v.value("Tabs"));
        if (!titles.isEmpty()) {
            static const QRegularExpression iconPrefix(QStringLiteral("^<([0-9]+)>"));
            for (auto title : titles.split(u'\n')) {
                int icon = tabs->count();
                if (v.value("IconByIndex", "1") == "0") {
                    const auto match = iconPrefix.match(title);
                    icon = match.hasMatch() ? match.captured(1).toInt() : -1;
                    if (match.hasMatch()) title.remove(0, match.capturedLength());
                }
                tabs->addTab(new QWidget, icons.value(icon), title);
            }
        }
        return tabs;
    }
    if (kind == "Label") {
        auto *label = new QLabel(caption, parent);
        label->setTextFormat(Qt::PlainText);
        const Qt::Alignment align[] = {Qt::AlignLeft, Qt::AlignRight, Qt::AlignHCenter};
        label->setAlignment(align[qBound(0, v.value("Alignment").toInt(), 2)] | Qt::AlignVCenter);
        return label;
    }
    if (kind == "Memo") {
        auto *memo = new QPlainTextEdit(displayString(v.value("Strings")), parent);
        const int bars = v.value("ScrollBars").toInt();
        memo->setHorizontalScrollBarPolicy(bars & 1 ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
        memo->setVerticalScrollBarPolicy(bars & 2 ? Qt::ScrollBarAsNeeded : Qt::ScrollBarAlwaysOff);
        return memo;
    }
    if (kind == "ToolBar") {
        auto *bar = new QToolBar(parent);
        bar->setIconSize(QSize(16,16));
        bar->setToolButtonStyle(v.value("Caption").toInt() ? Qt::ToolButtonTextUnderIcon : Qt::ToolButtonTextBesideIcon);
        static const QRegularExpression entry(QStringLiteral("'([^']*)'=\\[(?:ZIP([0-9A-Fa-f]+))?\\]"));
        const auto buttons = v.value("Buttons");
        auto matches = entry.globalMatch(buttons);
        while (matches.hasNext()) {
            const auto match = matches.next();
            const auto label = match.captured(1).section('=', 0, 0);
            if (label == "-") bar->addSeparator();
            else bar->addAction(toolbarIcon(match.capturedView(2)), label);
        }
        return bar;
    }
    if (kind == "Panel") return new PanelPreview(v, parent);
    if (kind == "GroupBox") {
        auto *group = new QGroupBox(caption, parent);
        group->setFlat(v.value("Flat", "1") == "0");
        return group;
    }
    // Configs for toolkit-specific controls still describe their geometry.
    if (!kind.isEmpty() && v.contains("Width") && v.contains("Height")) {
        auto *placeholder = new QLabel(caption.isEmpty() ? kind : caption, parent);
        placeholder->setTextFormat(Qt::PlainText);
        placeholder->setFrameStyle(QFrame::StyledPanel);
        placeholder->setAlignment(Qt::AlignCenter);
        placeholder->setToolTip(kind);
        return placeholder;
    }
    return nullptr;
}

class FormTitle final : public QWidget {
public:
    using QWidget::QWidget;
    Qt::WindowFlags frameFlags;
    void paintEvent(QPaintEvent *) override {
        QPainter painter(this);
        QStyleOptionTitleBar opt;
        opt.initFrom(this);
        opt.rect = rect();
        opt.text = windowTitle();
        opt.icon = QIcon(":/hiasm/logo.png");
        opt.titleBarFlags = frameFlags;
        opt.titleBarState = Qt::WindowActive;
        opt.state |= QStyle::State_Active;
        style()->drawComplexControl(QStyle::CC_TitleBar, &opt, &painter, this);
    }
};

class SelectionHandles final : public QWidget {
public:
    using QWidget::QWidget;
    Qt::Edges allowed;
    void paintEvent(QPaintEvent *) override {
        QPainter p(this);
        const auto box = rect().adjusted(3,3,-3,-3);
        p.setPen(QPen(palette().color(QPalette::Highlight), 1, Qt::DashLine));
        p.drawRect(box);
        p.setPen(palette().color(QPalette::WindowText));
        p.setBrush(palette().brush(QPalette::Window));
        for (int x : {box.left(), box.center().x(), box.right()})
            for (int y : {box.top(), box.center().y(), box.bottom()})
                if ((x != box.center().x() || y != box.center().y()) &&
                    (x != box.left() || allowed.testFlag(Qt::LeftEdge)) &&
                    (x != box.right() || allowed.testFlag(Qt::RightEdge)) &&
                    (y != box.top() || allowed.testFlag(Qt::TopEdge)) &&
                    (y != box.bottom() || allowed.testFlag(Qt::BottomEdge))) p.drawRect(x-2,y-2,4,4);
    }
};
}

FormCanvas::FormCanvas(QWidget *parent) : QWidget(parent), frame_(new QFrame(this)), surface_(new QWidget(frame_)),
    title_(new FormTitle(frame_)), selection_(new SelectionHandles(this)) {
    setFocusPolicy(Qt::StrongFocus);
    setAcceptDrops(true);
    setMouseTracking(true);
    frame_->setBackgroundRole(QPalette::Window);
    frame_->setObjectName(QStringLiteral("formFrame"));
    title_->setObjectName(QStringLiteral("formTitle"));
    surface_->setObjectName(QStringLiteral("formClient"));
#ifdef Q_OS_WIN
    // The IDE theme must not change the simulated system caption controls.
    if (auto *nativeStyle = QStyleFactory::create(QStringLiteral("windows"))) {
        nativeStyle->setParent(title_);
        title_->setStyle(nativeStyle);
    }
#endif
    surface_->setBackgroundRole(QPalette::Window);
    static_cast<QFrame *>(frame_)->setFrameStyle(QFrame::StyledPanel | QFrame::Raised);
    frame_->setAutoFillBackground(true);
    frame_->setAttribute(Qt::WA_TransparentForMouseEvents);
    surface_->setAutoFillBackground(true);
    selection_->setAttribute(Qt::WA_TransparentForMouseEvents);
    setBackgroundRole(QPalette::Mid);
    setAutoFillBackground(true);
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, &QWidget::customContextMenuRequested, this, [this](const QPoint &position) {
        if (!windowPreviewAvailable_) return;
        QMenu menu(this);
        auto *previewAction = menu.addAction(tr("Preview in a native window"));
        if (menu.exec(mapToGlobal(position)) != previewAction) return;
        auto *preview = new QLabel(window(), previewFlags_);
        preview->setObjectName(QStringLiteral("nativeFormPreview"));
        preview->setAttribute(Qt::WA_DeleteOnClose);
        auto *close = new QShortcut(QKeySequence(Qt::Key_Escape), preview);
        connect(close, &QShortcut::activated, preview, &QWidget::close);
        preview->setWindowTitle(title_->windowTitle());
        preview->setWindowOpacity(previewOpacity_);
        preview->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        preview->setPixmap(surface_->grab());
        if (previewResizable_) preview->resize(surface_->size());
        else preview->setFixedSize(surface_->size());
        preview->show();
    });
}

bool FormCanvas::setLayer(const SchemeDiagram &diagram, int layer, const QString &pack, const QString &hostName) {
    const int previous = selected_;
    dragging_ = false;
    windowPreviewAvailable_ = false;
    if (pack != pack_) armPlacement({});
    pack_ = pack;
    for (auto *widget : surface_->findChildren<QWidget *>(Qt::FindDirectChildrenOnly)) delete widget;
    controls_.clear();
    groups_.clear();
    childIndices_.clear();
    const auto &elements = diagram.layers.at(layer).elements;
    activeControls_.fill(-1, elements.size());
    const auto root = editorPackRoot(pack);
    formIndex_ = root.isEmpty() ? -1 : hostIndex(diagram.layers.at(layer), hostName);
    selected_ = -1;
    frame_->setVisible(formIndex_ >= 0);
    selection_->hide();
    if (formIndex_ < 0) { setMinimumSize({0,0}); return false; }
    QHash<QString, ElementConfig> configs;
    const auto configFor = [&](const SchemeElement &e) {
        auto found = configs.constFind(e.name);
        if (found != configs.cend()) return found.value();
        const auto config = elementConfig(root, pack, e.name);
        configs.insert(e.name, config);
        return config;
    };
    const auto &host = elements.at(formIndex_);
    const auto cfg = configFor(host);
    const auto values = formProperties(host, cfg);
    const bool window = cfg.editClass == "Form" || host.name == "MainForm" || host.name == "hcTplDialog" || host.name == "hcTplForm";
    const int border = window ? values.value("BorderStyle", "2").toInt() : 0;
    const bool titled = border >= 1 && border <= 5;
    const bool tool = border == 4 || border == 5;
    const bool resizable = values.value("Resizable", "0") == "0" &&
        (border == 0 || border == 2 || border == 5 || border == 7 || border == 9);
    auto flags = Qt::WindowFlags(tool ? Qt::Tool : border == 3 ? Qt::Dialog : Qt::Window);
    flags |= Qt::CustomizeWindowHint;
    if (titled) {
        flags |= Qt::WindowTitleHint | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint;
        if (!tool && border != 3) flags |= Qt::WindowMinimizeButtonHint;
        if (border == 2 && resizable) flags |= Qt::WindowMaximizeButtonHint;
    } else if (border < 6) flags |= Qt::FramelessWindowHint;
    if (!resizable) flags |= Qt::MSWindowsFixedSizeDialogHint;
    windowPreviewAvailable_ = window;
    previewFlags_ = flags;
    previewResizable_ = resizable;
    previewOpacity_ = qBound(0, values.value("AlphaBlendValue", "255").toInt(), 255) / 255.0;
    static_cast<FormTitle *>(title_)->frameFlags = flags;
    margins_ = {};
    if (window && border) {
        QWindow probe;
        probe.setFlags(flags);
        probe.create();
        margins_ = probe.frameMargins();
        const int edge = qMax(1, style()->pixelMetric(QStyle::PM_DefaultFrameWidth));
        if (margins_.isNull()) margins_ = QMargins(edge, edge, edge, edge);
        if (titled && margins_.top() <= margins_.bottom())
            margins_.setTop(margins_.top() + title_->style()->pixelMetric(QStyle::PM_TitleBarHeight));
    }
    decoration_ = (values.value("ClientSize", pack == "cpp" ? "1" : "0") == "0")
        ? QSize(margins_.left() + margins_.right(), margins_.top() + margins_.bottom()) : QSize(0,0);
    static_cast<QFrame *>(frame_)->setFrameStyle(margins_.isNull() ? QFrame::NoFrame : QFrame::StyledPanel | QFrame::Raised);
    title_->setVisible(titled);
    title_->setWindowTitle(displayString(values.value("Caption")));
    surface_->setFont(font());
    auto surfacePalette = palette();
    const auto color = QColor::fromString(values.value("Color"));
    if (color.isValid()) surfacePalette.setColor(QPalette::Window, color);
    surface_->setPalette(surfacePalette);
    applyFormFont(surface_, values, true);
    if (previewKind(host.name, cfg) == "Panel") {
        auto *preview = new PanelPreview(values, surface_);
        preview->setObjectName(QStringLiteral("panelSurface"));
        preview->show();
    }
    const auto requested = geometryFor(values);
    frame_->setGeometry(QRect(QPoint(10,10), (requested.size() + decoration_).expandedTo(
        QSize(margins_.left() + margins_.right() + 1, margins_.top() + margins_.bottom() + 1))));
    controls_.append({frame_, layer, formIndex_, 0, requested});
    activeControls_[formIndex_] = 0;

    // Parent groups precede their children so docking can be recalculated after a resize.
    struct Pending { int layer, host; QWidget *parent; };
    QList<Pending> pending{{layer, formIndex_, surface_}};
    for (qsizetype g = 0; g < pending.size(); ++g) {
        const auto current = pending.at(g);
        const auto &items = diagram.layers.at(current.layer).elements;
        Group group{current.parent, {}};
        for (int i = 0; i < items.size(); ++i) {
            if (i == current.host) continue;
            const auto &e = items.at(i);
            auto cfg = configFor(e);
            int propLayer = current.layer, propIndex = i;
            if (e.child >= 0) {
                if (current.layer == layer) childIndices_.append(i);
                if (cfg.sub.isEmpty()) continue;
                propLayer = e.child;
                propIndex = hostIndex(diagram.layers.at(propLayer), QString::fromUtf8(cfg.sub));
                if (propIndex < 0) continue;
                cfg = configFor(diagram.layers.at(propLayer).elements.at(propIndex));
            }
            const auto &target = diagram.layers.at(propLayer).elements.at(propIndex);
            const auto properties = formProperties(target, cfg);
            const auto kind = previewKind(target.name, cfg);
            if (kind == "Form" || kind == "Dialog" || target.name == "MainForm" ||
                (cfg.editClass.isEmpty() && !cfg.className.startsWith("HCWE_"))) continue;
            auto *widget = previewControl(kind, properties, current.parent);
            if (!widget) continue;
            widget->setObjectName(QStringLiteral("control_%1").arg(e.id));
            applyFormFont(widget, properties);
            widget->show();
            const int index = controls_.size();
            controls_.append({widget, propLayer, propIndex, properties.value("Align").toInt(), geometryFor(properties)});
            group.controls.append(index);
            if (current.layer == layer) activeControls_[i] = index;
            if (e.child >= 0) pending.append({e.child, propIndex, widget});
        }
        groups_.append(group);
    }
    arrange();
    selectElement(previous);
    return true;
}

void FormCanvas::arrange() {
    title_->setGeometry(2, 1, frame_->width()-4, qMax(0, margins_.top()-2));
    surface_->setGeometry(frame_->rect().marginsRemoved(margins_));
    if (auto *preview = surface_->findChild<QWidget *>(QStringLiteral("panelSurface"), Qt::FindDirectChildrenOnly))
        preview->setGeometry(surface_->rect());
    for (const auto &group : groups_) {
        auto free = group.parent->rect();
        for (int i : group.controls) {
            const auto &c = controls_[i];
            auto r = c.requested;
            switch (c.align) {
            case 1: r = QRect(free.topLeft(), QSize(qMin(r.width(), free.width()), free.height())); free.setLeft(r.right()+1); break;
            case 2: r = QRect(free.topLeft(), QSize(free.width(), qMin(r.height(), free.height()))); free.setTop(r.bottom()+1); break;
            case 3: r = QRect(free.topRight() + QPoint(1-qMin(r.width(),free.width()),0), QSize(qMin(r.width(),free.width()),free.height())); free.setRight(r.left()-1); break;
            case 4: r = QRect(free.bottomLeft() + QPoint(0,1-qMin(r.height(),free.height())), QSize(free.width(),qMin(r.height(),free.height()))); free.setBottom(r.top()-1); break;
            case 5: continue;
            }
            c.widget->setGeometry(r);
        }
        for (int i : group.controls) if (controls_[i].align == 5) controls_[i].widget->setGeometry(free);
    }
    setMinimumSize(frame_->size() + QSize(20,20));
}

QRect FormCanvas::selectedRect() const {
    const auto *w = controls_.at(activeControls_.at(selected_)).widget;
    return {w->mapTo(this, QPoint()), w->size()};
}

void FormCanvas::selectElement(int index) {
    selected_ = index >= 0 && index < activeControls_.size() && activeControls_.at(index) >= 0 ? index : -1;
    selection_->setVisible(selected_ >= 0);
    if (selected_ < 0) return;
    static_cast<SelectionHandles *>(selection_)->allowed = resizeEdges();
    selection_->update();
    selection_->setGeometry(selectedRect().adjusted(-3,-3,3,3));
    selection_->raise();
}

int FormCanvas::hitControl(const QPoint &position) const {
    if (!frame_->geometry().contains(position)) return -1;
    if (!surface_->rect().contains(surface_->mapFrom(this, position))) return formIndex_;
    for (int i = activeControls_.size()-1; i >= 0; --i) {
        if (i == formIndex_ || activeControls_[i] < 0) continue;
        const auto *w = controls_[activeControls_[i]].widget;
        if (w->rect().contains(w->mapFrom(this, position))) return i;
    }
    return formIndex_;
}

void FormCanvas::armPlacement(const QString &name, const QIcon &icon) {
    placement_ = name;
    if (name.isEmpty()) unsetCursor();
    else {
        setCursor(elementPlacementCursor(icon, palette()));
        setFocus();
    }
}

std::optional<QPoint> FormCanvas::positionFor(const QString &name, std::optional<QPoint> at) const {
    if (formIndex_ < 0) return {};
    static const QRegularExpression identifier(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]*$"));
    if (!identifier.match(name).hasMatch()) return {};
    const auto root = editorPackRoot(pack_);
    auto config = elementConfig(root, pack_, name);
    if (!config.sub.isEmpty()) {
        if (!config.createsSdkOnInsertion()) return {};
        config = elementConfig(root, pack_, QString::fromUtf8(config.sub));
    }
    if (!config.error.isEmpty() || config.editClass == "Form" ||
        (config.editClass.isEmpty() && !config.className.startsWith("HCWE_"))) return {};
    const auto properties = formProperties({}, config);
    if (!properties.contains("Width") || !properties.contains("Height")) return {};
    const auto pos = at ? surface_->mapFrom(this, *at) : surface_->rect().center();
    if (!surface_->rect().contains(pos)) return {};
    return pos;
}

std::optional<QPair<int, int>> FormCanvas::propertyTarget(int index) const {
    if (index < 0 || index >= activeControls_.size() || activeControls_.at(index) < 0) return {};
    const auto &control = controls_.at(activeControls_.at(index));
    return QPair<int, int>{control.layer, control.index};
}

Qt::Edges FormCanvas::handleAt(const QPoint &pos) const {
    if (selected_ < 0) return {};
    const auto r = selectedRect();
    if (!r.adjusted(-4,-4,4,4).contains(pos)) return {};
    const auto near = [](int a, int b) { return qAbs(a-b) <= 4; };
    Qt::Edges edges;
    if (near(pos.x(),r.left())) edges |= Qt::LeftEdge;
    else if (near(pos.x(),r.right())) edges |= Qt::RightEdge;
    if (near(pos.y(),r.top())) edges |= Qt::TopEdge;
    else if (near(pos.y(),r.bottom())) edges |= Qt::BottomEdge;
    if (!(edges & (Qt::LeftEdge|Qt::RightEdge)) && !near(pos.x(),r.center().x())) return {};
    if (!(edges & (Qt::TopEdge|Qt::BottomEdge)) && !near(pos.y(),r.center().y())) return {};
    return edges & resizeEdges();
}

Qt::Edges FormCanvas::resizeEdges() const {
    if (selected_ == formIndex_) return Qt::RightEdge | Qt::BottomEdge;
    switch (controls_[activeControls_[selected_]].align) {
    case 1: return Qt::RightEdge;
    case 2: return Qt::BottomEdge;
    case 3: return Qt::LeftEdge;
    case 4: return Qt::TopEdge;
    case 5: return {};
    }
    return Qt::TopEdge | Qt::BottomEdge | Qt::LeftEdge | Qt::RightEdge;
}

void FormCanvas::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::RightButton && placementArmed()) {
        armPlacement({});
        return;
    }
    if (event->button() != Qt::LeftButton || formIndex_ < 0) return;
    press_ = event->position().toPoint();
    if (!placement_.isEmpty()) {
        const auto name = placement_;
        if (const auto position = positionFor(name, press_)) {
            armPlacement({});
            emit placementRequested(name, *position);
        }
        return;
    }
    edges_ = handleAt(press_);
    if (!edges_) {
        const int hit = hitControl(press_);
        selectElement(hit);
        emit elementSelected(hit);
        edges_ = handleAt(press_);
    }
    if (selected_ < 0) return;
    const auto &c = controls_[activeControls_[selected_]];
    initial_ = c.requested;
    dragging_ = edges_ || (!c.align && selected_ != formIndex_);
}

void FormCanvas::mouseMoveEvent(QMouseEvent *event) {
    if (!placement_.isEmpty()) return;
    if (!dragging_) {
        const auto edges = handleAt(event->position().toPoint());
        const bool horizontal = edges & (Qt::LeftEdge | Qt::RightEdge);
        const bool vertical = edges & (Qt::TopEdge | Qt::BottomEdge);
        if (horizontal && vertical)
            setCursor(edges.testFlag(Qt::LeftEdge) == edges.testFlag(Qt::TopEdge) ? Qt::SizeFDiagCursor : Qt::SizeBDiagCursor);
        else if (horizontal) setCursor(Qt::SizeHorCursor);
        else if (vertical) setCursor(Qt::SizeVerCursor);
        else unsetCursor();
        return;
    }
    if (selected_ < 0) return;
    const auto delta = event->position().toPoint() - press_;
    auto rect = initial_;
    if (!edges_) rect.translate(delta);
    else {
        if (edges_.testFlag(Qt::LeftEdge)) rect.setLeft(qBound(rect.right()-16383,rect.left()+delta.x(),rect.right()-7));
        if (edges_.testFlag(Qt::RightEdge)) rect.setWidth(qBound(8,rect.width()+delta.x(),16384));
        if (edges_.testFlag(Qt::TopEdge)) rect.setTop(qBound(rect.bottom()-16383,rect.top()+delta.y(),rect.bottom()-7));
        if (edges_.testFlag(Qt::BottomEdge)) rect.setHeight(qBound(8,rect.height()+delta.y(),16384));
    }
    auto &c = controls_[activeControls_[selected_]];
    if (c.align) rect.moveTopLeft(initial_.topLeft());
    c.requested = rect;
    if (selected_ == formIndex_) frame_->resize((rect.size() + decoration_).expandedTo(
        QSize(margins_.left() + margins_.right() + 1, margins_.top() + margins_.bottom() + 1)));
    arrange();
    selectElement(selected_);
}

void FormCanvas::mouseReleaseEvent(QMouseEvent *event) {
    if (!dragging_ || event->button() != Qt::LeftButton || selected_ < 0) return;
    mouseMoveEvent(event);
    dragging_ = false;
    const auto c = controls_.at(activeControls_.at(selected_));
    if (c.requested != initial_) emit geometryEdited(c.layer, c.index, initial_, c.requested, selected_ == formIndex_);
}

void FormCanvas::mouseDoubleClickEvent(QMouseEvent *event) {
    if (event->button() != Qt::LeftButton || formIndex_ < 0) return;
    const int hit = hitControl(event->position().toPoint());
    if (childIndices_.contains(hit)) { dragging_ = false; emit childActivated(hit); }
    else if (hit >= 0) { dragging_ = false; emit controlActivated(hit); }
}

void FormCanvas::dragEnterEvent(QDragEnterEvent *event) {
    if (event->mimeData()->hasFormat(elementMime)) event->acceptProposedAction();
}

void FormCanvas::dragMoveEvent(QDragMoveEvent *event) {
    if (positionFor(QString::fromUtf8(event->mimeData()->data(elementMime)), event->position().toPoint()))
        event->acceptProposedAction();
}

void FormCanvas::dropEvent(QDropEvent *event) {
    const auto name = QString::fromUtf8(event->mimeData()->data(elementMime));
    if (const auto position = positionFor(name, event->position().toPoint())) {
        event->acceptProposedAction();
        armPlacement({});
        emit placementRequested(name, *position);
    }
}
