#include <QtWidgets>
#include <cmath>
#include <memory>
#include <vector>
#include "core_widgets.h"

namespace {
struct Control {
    void *element;
    QPointer<QWidget> widget;
    QPointer<QWidget> content;
    QByteArray text;
    QImage image;
};

const HiasmQtHost *host;
std::unique_ptr<QApplication> app;
std::vector<std::unique_ptr<Control>> controls;
bool ending = false;
int argc = 1;
char appName[] = "HiAsm Core";
char *argv[] = {appName, nullptr};

QString prop(Control *c, const char *name) {
    return QString::fromUtf8(host->property(c->element, name));
}

void send(Control *c, const char *name, const QString &value = {}, double number = 0, int type = 0) {
    if (!ending) host->event(c->element, name, value.toUtf8().constData(), number, type);
}

int start(const HiasmQtHost *callbacks, const char *directory) {
    if (host || (QCoreApplication::instance() && !qobject_cast<QApplication *>(QCoreApplication::instance()))) return 0;
    host = callbacks;
    if (*directory) QCoreApplication::addLibraryPath(QString::fromUtf8(directory));
    if (!QCoreApplication::instance()) {
        app = std::make_unique<QApplication>(argc, argv);
        app->setQuitOnLastWindowClosed(false);
    }
    return 1;
}

void stop() {
    ending = true;
    for (auto i = controls.rbegin(); i != controls.rend(); ++i) delete (*i)->widget.data();
    controls.clear();
    app.reset();
    host = nullptr;
    ending = false;
}

void pump() {
    QCoreApplication::processEvents();
    QCoreApplication::sendPostedEvents(nullptr, QEvent::DeferredDelete);
}

class Form : public QWidget {
    Control *control;
public:
    Form(Control *c, QWidget *owner) : QWidget(owner, Qt::Window), control(c) {}
    void closeEvent(QCloseEvent *event) override {
        QWidget::closeEvent(event);
        if (!parentWidget()) send(control, "@close");
    }
};

void addMenuItem(QMenu *menu, Control *control, const QString &label) {
    const int index = menu->actions().size();
    if (label == QStringLiteral("-")) {
        menu->addSeparator();
        return;
    }
    auto *action = menu->addAction(label);
    QObject::connect(action, &QAction::triggered, menu, [control, index] {
        send(control, "onClick", {}, index, 2);
    });
}

void *create(void *element, const char *kind, void *parentHandle) {
    auto record = std::make_unique<Control>();
    auto *c = record.get();
    c->element = element;
    auto *parent = static_cast<Control *>(parentHandle);
    QWidget *owner = parent ? (parent->content ? parent->content.data() : parent->widget.data()) : nullptr;
    const QByteArray cls(kind);
    const auto text = [&](const char *name) { return prop(c, name); };
    const auto number = [&](const char *name) { return text(name).toInt(); };
    const auto caption = text("Caption");
    QWidget *w = nullptr;
    const bool form = cls == "HCWE_Form" || cls == "HCWE_MainForm";
    const bool dialog = cls == "HCWE_Dialog";
    if (cls == "HCPopupMenu") {
        auto *menu = new QMenu(owner);
        for (const auto &label : text("Menu").split('\n', Qt::SkipEmptyParts))
            addMenuItem(menu, c, label);
        QObject::connect(menu, &QMenu::aboutToHide, menu, [c] { send(c, "onEndPopup"); });
        c->widget = menu;
        controls.push_back(std::move(record));
        return c;
    } else if (cls == "HCMessage") {
        auto *message = new QMessageBox(owner);
        message->setTextFormat(Qt::PlainText);
        const QMessageBox::StandardButtons buttons[] = {QMessageBox::NoButton, QMessageBox::Ok,
            QMessageBox::Close, QMessageBox::Cancel, QMessageBox::Yes | QMessageBox::No,
            QMessageBox::Ok | QMessageBox::Cancel};
        message->setStandardButtons(buttons[qBound(0, number("Type"), 5)]);
        QObject::connect(message, &QMessageBox::finished, message, [c](int result) {
            int response = -4;
            switch (result) {
            case QMessageBox::Ok: response = -5; break;
            case QMessageBox::Cancel: response = -6; break;
            case QMessageBox::Close: response = -7; break;
            case QMessageBox::Yes: response = -8; break;
            case QMessageBox::No: response = -9; break;
            }
            send(c, "onMessage", {}, response, 2);
        });
        c->widget = message;
        controls.push_back(std::move(record));
        return c;
    } else if (cls == "HCOpenDialog" || cls == "HCSaveDialog") {
        auto *chooser = new QFileDialog(owner, text("Title"), text("StartDir"), text("Filter"));
        chooser->setAcceptMode(cls == "HCSaveDialog" ? QFileDialog::AcceptSave : QFileDialog::AcceptOpen);
        chooser->setFileMode(cls == "HCSaveDialog" ? QFileDialog::AnyFile : QFileDialog::ExistingFile);
        QObject::connect(chooser, &QFileDialog::fileSelected, chooser, [c](const QString &file) { send(c, "onExecute", file, 0, 1); });
        QObject::connect(chooser, &QFileDialog::rejected, chooser, [c] { send(c, "onCancel"); });
        c->widget = chooser;
        controls.push_back(std::move(record));
        return c;
    } else if (form || dialog) {
        w = dialog ? static_cast<QWidget *>(new QDialog(owner)) : new Form(c, owner);
        w->setWindowTitle(caption);
        if (form && number("BorderStyle") == 0) w->setWindowFlag(Qt::FramelessWindowHint);
        if (form && number("Modal") == 0) w->setWindowModality(Qt::ApplicationModal);
        if (dialog) {
            auto *buttons = new QDialogButtonBox(w);
            const auto labels = text("Buttons").split('\n', Qt::SkipEmptyParts);
            for (int i = 0; i < labels.size(); ++i) {
                auto *button = buttons->addButton(labels.at(i), QDialogButtonBox::ActionRole);
                QObject::connect(button, &QPushButton::clicked, w, [c, i] { send(c, "onButtonDown", {}, i, 2); });
            }
            auto *layout = new QVBoxLayout(w);
            layout->addWidget(buttons);
            if (!owner) QObject::connect(static_cast<QDialog *>(w), &QDialog::finished, w, [c] { send(c, "@close"); });
        }
    } else if (cls == "HCWE_Button" || cls == "HCWE_BitBtn") {
        auto *button = new QPushButton(caption, owner);
        QObject::connect(button, &QPushButton::clicked, button, [c] { send(c, "onClick"); });
        w = button;
    } else if (cls == "HCWE_Label" || cls == "HCWE_Image") {
        auto *label = new QLabel(caption, owner);
        label->setTextFormat(Qt::PlainText);
        if (cls == "HCWE_Image") {
            c->image.loadFromData(QByteArray::fromBase64(text("Image").toLatin1()));
            if (c->image.isNull() && !text("FileName").isEmpty()) c->image.load(text("FileName"));
            label->setPixmap(QPixmap::fromImage(c->image));
        }
        w = label;
    } else if (cls == "HCWE_Edit") {
        auto *edit = new QLineEdit(text("Text"), owner);
        edit->setReadOnly(number("ReadOnly") == 0);
        if (number("Password") == 0) edit->setEchoMode(QLineEdit::Password);
        QObject::connect(edit, &QLineEdit::textChanged, edit, [c](const QString &value) { send(c, "onChange", value, 0, 1); });
        QObject::connect(edit, &QLineEdit::returnPressed, edit, [c, edit] {
            const auto value = edit->text();
            edit->clear();
            send(c, "onEnter", value, 0, 1);
        });
        w = edit;
    } else if (cls == "HCWE_Memo" || cls == "HCWE_SynEdit" || cls == "HCWE_SDKEdit") {
        auto *edit = new QPlainTextEdit(text("Strings"), owner);
        edit->setReadOnly(number("ReadOnly") == 0);
        QObject::connect(edit, &QPlainTextEdit::textChanged, edit, [c, edit] { send(c, "onChange", edit->toPlainText(), 0, 1); });
        w = edit;
    } else if (cls == "HCWE_CheckBox") {
        auto *check = new QCheckBox(caption, owner);
        check->setChecked(number("Checked") == 0);
        QObject::connect(check, &QCheckBox::toggled, check, [c](bool value) { send(c, "onCheck", {}, value, 2); });
        w = check;
    } else if (cls == "HCWE_ListBox" || cls == "HCWE_ComboBox") {
        const auto items = text("Strings").split('\n', Qt::SkipEmptyParts);
        if (cls == "HCWE_ListBox") {
            auto *list = new QListWidget(owner);
            list->addItems(items);
            QObject::connect(list, &QListWidget::currentRowChanged, list, [c, list](int row) {
                if (row < 0) return;
                if (prop(c, "DataType").toInt() == 0) send(c, "onClick", {}, row, 2);
                else send(c, "onClick", list->item(row)->text(), 0, 1);
            });
            w = list;
        } else {
            auto *combo = new QComboBox(owner);
            combo->setEditable(number("ReadOnly") != 0);
            combo->addItems(items);
            QObject::connect(combo, &QComboBox::currentIndexChanged, combo, [c, combo](int row) {
                if (prop(c, "DataType").toInt() == 0) send(c, "onClick", {}, row, 2);
                else send(c, "onClick", combo->currentText(), 0, 1);
            });
            w = combo;
        }
    } else if (cls == "HCWE_ScrollBar") {
        auto *bar = new QScrollBar(number("Kind") ? Qt::Vertical : Qt::Horizontal, owner);
        bar->setRange(qRound(text("Min").toDouble()), qRound(text("Max").toDouble()));
        bar->setValue(qRound(text("Position").toDouble()));
        QObject::connect(bar, &QScrollBar::valueChanged, bar, [c](int value) { send(c, "onPosition", {}, value, 2); });
        w = bar;
    } else if (cls == "HCWE_ProgressBar") {
        auto *bar = new QProgressBar(owner);
        bar->setRange(0, 1000);
        bar->setValue(0);
        bar->setOrientation(number("Orientation") >= 2 ? Qt::Vertical : Qt::Horizontal);
        bar->setInvertedAppearance(number("Orientation") % 2);
        if (!text("Text").isEmpty()) bar->setFormat(text("Text"));
        QObject::connect(bar, &QProgressBar::valueChanged, bar, [c](int value) { send(c, "onValue", {}, value / 1000.0, 3); });
        w = bar;
    } else if (cls == "HCWE_FileChooserButton" || cls == "HCWE_ColorButton" || cls == "HCWE_ColorPicker") {
        auto *button = new QPushButton(text("Title"), owner);
        if (cls == "HCWE_FileChooserButton") {
            QObject::connect(button, &QPushButton::clicked, button, [c, button] {
                const auto title = prop(c, "Title");
                const auto mode = prop(c, "Action").toInt();
                auto *chooser = new QFileDialog(button, title);
                chooser->setAttribute(Qt::WA_DeleteOnClose);
                if (mode >= 2) chooser->setFileMode(QFileDialog::Directory);
                if (mode == 1) chooser->setAcceptMode(QFileDialog::AcceptSave);
                QObject::connect(chooser, &QFileDialog::fileSelected, button, [c](const QString &file) { send(c, "onSelect", file, 0, 1); });
                chooser->open();
            });
        } else {
            button->setText(text("Color"));
            QObject::connect(button, &QPushButton::clicked, button, [c, button, cls] {
                auto *chooser = new QColorDialog(QColor(button->text()), button);
                chooser->setAttribute(Qt::WA_DeleteOnClose);
                QObject::connect(chooser, &QColorDialog::colorSelected, button, [c, button, cls](const QColor &color) {
                    button->setText(color.name());
                    if (cls == "HCWE_ColorPicker") send(c, "onClickColor", {}, color.red() | color.green() << 8 | color.blue() << 16, 2);
                    else send(c, "onSelect", color.name(), 0, 1);
                });
                chooser->open();
            });
        }
        w = button;
    } else if (cls == "HCWE_VPaned" || cls == "HCWE_HPaned") {
        w = new QSplitter(cls == "HCWE_VPaned" ? Qt::Vertical : Qt::Horizontal, owner);
    } else if (cls == "HCWE_TabControl") {
        auto *tabs = new QTabWidget(owner);
        const QTabWidget::TabPosition positions[] = {QTabWidget::North, QTabWidget::South, QTabWidget::West, QTabWidget::East};
        tabs->setTabPosition(positions[qBound(0, number("Position"), 3)]);
        QObject::connect(tabs, &QTabWidget::currentChanged, tabs, [c](int index) { send(c, "onChange", {}, index, 2); });
        w = tabs;
    } else if (cls == "HCWE_Expander") {
        w = new QWidget(owner);
        auto *layout = new QVBoxLayout(w);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(number("Spacing"));
        auto *toggle = new QToolButton(w);
        toggle->setText(text("Label"));
        toggle->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
        toggle->setCheckable(true);
        layout->addWidget(toggle, 0, Qt::AlignLeft);
        auto *content = new QWidget(w);
        c->content = content;
        auto *contentLayout = new QVBoxLayout(content);
        contentLayout->setContentsMargins(0, 0, 0, 0);
        layout->addWidget(content, 1);
        const auto expand = [toggle, content](bool expanded) {
            toggle->setArrowType(expanded ? Qt::DownArrow : Qt::RightArrow);
            content->setVisible(expanded);
        };
        QObject::connect(toggle, &QToolButton::toggled, content, expand);
        toggle->setChecked(number("State") == 1);
        expand(toggle->isChecked());
    } else if (cls == "HCWE_VBox" || cls == "HCWE_HBox" || cls.endsWith("ButtonBox") ||
               cls == "HCWE_GroupBox" || cls == "HCWE_Frame" || cls == "HCWE_Panel" ||
               cls == "HCWE_HandleBox" || cls == "HCWE_Fixed") {
        const bool framed = cls == "HCWE_GroupBox" || cls == "HCWE_Frame" || cls == "HCWE_Panel";
        w = framed ? static_cast<QWidget *>(new QGroupBox(caption, owner)) : new QWidget(owner);
        if (cls != "HCWE_Fixed" && cls != "HCWE_Panel") {
            const bool horizontal = cls == "HCWE_HBox" || cls == "HCWE_HButtonBox";
            auto *layout = new QBoxLayout(horizontal ? QBoxLayout::LeftToRight : QBoxLayout::TopToBottom, w);
            layout->setSpacing(number("Spacing"));
            if (!framed) layout->setContentsMargins(0, 0, 0, 0);
        }
    }
    if (!w) return nullptr;
    c->widget = w;
    w->setObjectName(QString::fromLatin1(kind));
    if (owner && !form && !dialog) {
        if (auto *split = qobject_cast<QSplitter *>(owner)) split->addWidget(w);
        else if (auto *tabs = qobject_cast<QTabWidget *>(owner)) {
            const QSignalBlocker blocker(tabs);
            tabs->addTab(w, caption);
        } else if (auto *layout = qobject_cast<QBoxLayout *>(owner->layout())) {
            const int position = qobject_cast<QDialog *>(owner) ? layout->count() - 1 : layout->count();
            layout->insertWidget(position, w, number("Expand") == 0 ? 1 : 0);
        }
    }
    const QSize size(qMax(1, number("Width")), qMax(1, number("Height")));
    w->resize(size);
    w->move(number("Left"), number("Top"));
    if (form && (number("Resizable") != 0 || number("BorderStyle") == 1 || number("BorderStyle") == 3 ||
                 number("BorderStyle") == 4 || number("BorderStyle") == 6 || number("BorderStyle") == 8))
        w->setFixedSize(size);
    w->setEnabled(number("Enabled") == 0);
    w->setToolTip(text("Hint"));
    if (owner && !form && !dialog) w->setVisible(number("Visible") == 0);
    controls.push_back(std::move(record));
    return c;
}

void show(void *handle) {
    auto *c = static_cast<Control *>(handle);
    if (c->widget && prop(c, "Visible").toInt() == 0) c->widget->show();
}

int work(void *handle, const char *name, const char *value, double number) {
    auto *c = static_cast<Control *>(handle);
    auto *widget = c->widget.data();
    if (!widget) return 0;
    const QByteArray command(name);
    const auto text = QString::fromUtf8(value);
    if (!std::isfinite(number)) number = 0;
    const int index = qBound(-2147483647.0, number, 2147483647.0);
    if (auto *menu = qobject_cast<QMenu *>(widget)) {
        if (command == "doPopup") menu->popup(QCursor::pos());
        else if (command == "doAddItem") addMenuItem(menu, c, text);
        else if (command == "doClear") menu->clear();
        else return 0;
    } else if (command == "doVisible") widget->setVisible(number != 0);
    else if (command == "doEnabled") widget->setEnabled(number != 0);
    else if (command == "doShow") widget->show();
    else if (command == "doClose") {
        if (widget->isWindow()) widget->close();
        else widget->hide();
    }
    else if (command == "doRun" && qobject_cast<QDialog *>(widget)) static_cast<QDialog *>(widget)->open();
    else if (auto *message = qobject_cast<QMessageBox *>(widget); message && command == "doMessage") {
        message->setWindowTitle(prop(c, "Caption"));
        message->setText(text);
        message->open();
    } else if (auto *chooser = qobject_cast<QFileDialog *>(widget); chooser && command == "doExecute") {
        if (!text.isEmpty()) chooser->selectFile(text);
        chooser->open();
    }
    else if (auto *edit = qobject_cast<QLineEdit *>(widget)) {
        if (command == "doText") edit->setText(text);
        else if (command == "doText2") { const QSignalBlocker blocker(edit); edit->setText(text); }
        else return 0;
    } else if (auto *edit = qobject_cast<QPlainTextEdit *>(widget)) {
        if (command == "doText") edit->setPlainText(text);
        else if (command == "doAdd") edit->appendPlainText(text);
        else if (command == "doClear") edit->clear();
        else if (command == "doSetSelect") edit->textCursor().insertText(text);
        else if (command == "doLoad") {
            QFile file(text);
            if (file.open(QIODevice::ReadOnly)) edit->setPlainText(QString::fromUtf8(file.readAll()));
        } else if (command == "doSave") {
            QSaveFile file(text);
            if (file.open(QIODevice::WriteOnly)) { file.write(edit->toPlainText().toUtf8()); file.commit(); }
        } else return 0;
    } else if (auto *label = qobject_cast<QLabel *>(widget)) {
        if (command == "doCaption") label->setText(text);
        else if (command == "doLoadFromFile") {
            c->image = QImage(text);
            label->setPixmap(QPixmap::fromImage(c->image));
        }
        else if (command == "doLoadFromImage") {
            c->image = QImage::fromData(QByteArray::fromBase64(text.toLatin1()));
            label->setPixmap(QPixmap::fromImage(c->image));
        }
        else return 0;
    } else if (auto *check = qobject_cast<QCheckBox *>(widget); check && command == "doCheck") check->setChecked(number != 0);
    else if (auto *scroll = qobject_cast<QScrollBar *>(widget); scroll && command == "doPosition") scroll->setValue(index);
    else if (auto *bar = qobject_cast<QProgressBar *>(widget); bar && command == "doValue") bar->setValue(qRound(qBound(0.0, number, 1.0) * 1000));
    else if (auto *tabs = qobject_cast<QTabWidget *>(widget); tabs && command == "doSelect") tabs->setCurrentIndex(index);
    else if (auto *combo = qobject_cast<QComboBox *>(widget)) {
        if (command == "doAdd") combo->addItem(text);
        else if (command == "doClear") combo->clear();
        else if (command == "doSelectByIndex") combo->setCurrentIndex(index);
        else return 0;
    } else if (auto *list = qobject_cast<QListWidget *>(widget)) {
        if (command == "doAdd") list->addItem(text);
        else if (command == "doText") { list->clear(); list->addItems(text.split('\n')); }
        else if (command == "doClear") list->clear();
        else if (command == "doDelete") delete list->takeItem(index);
        else if (command == "doInsert") list->insertItem(index, text);
        else if (command == "doSelectByIndex") list->setCurrentRow(index);
        else if (command == "doLoad") {
            QFile file(text);
            if (file.open(QIODevice::ReadOnly)) { list->clear(); list->addItems(QString::fromUtf8(file.readAll()).split('\n')); }
        } else if (command == "doSave") {
            QStringList lines;
            for (int i = 0; i < list->count(); ++i) lines.append(list->item(i)->text());
            QSaveFile file(text);
            if (file.open(QIODevice::WriteOnly)) { file.write(lines.join('\n').toUtf8()); file.commit(); }
        } else return 0;
    } else if (auto *button = qobject_cast<QPushButton *>(widget); button && command == "doColor") button->setText(text);
    else return 0;
    return 1;
}

int read(void *handle, const char *name, const char **value, double *number) {
    auto *c = static_cast<Control *>(handle);
    auto *widget = c->widget.data();
    const QByteArray point(name);
    QString text;
    if (auto *edit = qobject_cast<QLineEdit *>(widget); edit && point == "Text") text = edit->text();
    else if (auto *edit = qobject_cast<QPlainTextEdit *>(widget)) {
        if (point == "Count") { *number = edit->blockCount(); return 2; }
        if (point != "Text") return 0;
        text = edit->toPlainText();
    } else if (auto *list = qobject_cast<QListWidget *>(widget)) {
        if (point == "Index") { *number = list->currentRow(); return 2; }
        if (point == "Count") { *number = list->count(); return 2; }
        if (point != "Text") return 0;
        QStringList lines;
        for (int i = 0; i < list->count(); ++i) lines.append(list->item(i)->text());
        text = lines.join('\n');
    } else if (auto *combo = qobject_cast<QComboBox *>(widget)) {
        if (point == "Index") { *number = combo->currentIndex(); return 2; }
        if (point != "EditText") return 0;
        text = combo->currentText();
    } else if (qobject_cast<QLabel *>(widget) && point == "Picture") {
        QByteArray png;
        QBuffer buffer(&png);
        buffer.open(QIODevice::WriteOnly);
        if (!c->image.isNull() && !c->image.save(&buffer, "PNG")) return 0;
        c->text = png.toBase64();
        *value = c->text.constData();
        return 4;
    } else if (auto *label = qobject_cast<QLabel *>(widget); label && point == "Caption") text = label->text();
    else if (auto *check = qobject_cast<QCheckBox *>(widget); check && point == "Checked") { *number = check->isChecked(); return 2; }
    else if (auto *scroll = qobject_cast<QScrollBar *>(widget); scroll && point == "CurPos") { *number = scroll->value(); return 2; }
    else if (auto *tabs = qobject_cast<QTabWidget *>(widget); tabs && point == "CurrentIndex") { *number = tabs->currentIndex(); return 2; }
    else return 0;
    c->text = text.toUtf8();
    *value = c->text.constData();
    return 1;
}
}

extern "C" Q_DECL_EXPORT const HiasmQtWidgets *hiasm_qt_widgets(unsigned version) {
    static const HiasmQtWidgets api{start, stop, pump, create, show, work, read};
    return version == 2 ? &api : nullptr;
}
