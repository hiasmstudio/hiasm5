#include "core_widgets.h"

#include <QApplication>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMap>
#include <QScopedValueRollback>
#include <QTextStream>
#include <QTimer>
#include <QVariant>

extern "C" const HiasmQtWidgets *hiasm_qt_widgets(unsigned version);

namespace {
struct Node {
    QByteArray kind;
    QHash<QByteArray,QByteArray> properties;
    QHash<QByteArray,QPair<int,QByteArray>> links;
    int parent = -1;
    QVariant stored;
    void *widget = nullptr;
};

struct Runtime {
    const HiasmQtWidgets *api = hiasm_qt_widgets(2);
    QMap<int,Node> nodes;
    bool ready = false;
    int depth = 0;

    QVariant input(const Node &node, const QByteArray &name, const QVariant &incoming) {
        const auto link = node.links.constFind(name);
        if (link != node.links.cend()) {
            const auto &source = *nodes.constFind(link->first);
            if (source.kind == "HCMemory" && link->second == "Value") return source.stored;
            const char *text = nullptr;
            double number = 0;
            const int type = source.widget ? api->read(source.widget, link->second.constData(), &text, &number) : 0;
            if (type == 1 || type == 4) return QString::fromUtf8(text);
            if (type == 2) return int(number);
            if (type == 3) return number;
        }
        const auto value = node.properties.value(name);
        return value.isEmpty() ? incoming : QVariant(QString::fromUtf8(value));
    }

    void event(Node &node, const QByteArray &name, QVariant value) {
        if (name == "@close") { QCoreApplication::quit(); return; }
        const auto link = node.links.constFind(name);
        if (!ready || link == node.links.cend()) return;
        if (depth >= 256) {
            QTextStream(stderr) << "Core event recursion limit" << Qt::endl;
            QCoreApplication::exit(1);
            return;
        }
        QScopedValueRollback<int> stack(depth, depth + 1);
        if (name == "onClick" && !value.isValid()) value = input(node, "Data", {});
        auto &target = nodes[link->first];
        const auto &command = link->second;
        if (target.kind == "HCDoData" && command == "doData") {
            event(target, "onEventData", input(target, "Data", value));
            return;
        }
        if (target.kind == "HCMemory" && (command == "doValue" || command == "doClear")) {
            target.stored = command == "doClear" ? QVariant(QString::fromUtf8(target.properties.value("Default"))) :
                                                   input(target, "Data", value);
            event(target, "onData", target.stored);
            return;
        }
        if (target.kind == "HCConsole" && command == "doPrint") {
            QTextStream(stdout) << value.toString() << Qt::endl;
            event(target, "onPrint", value);
            return;
        }
        static const QHash<QByteArray,QByteArray> inputs{
            {"doText","Str"},{"doText2","Str"},{"doCaption","Text"},{"doAdd","Str"},
            {"doAddItem","Text"},{"doPosition","Position"},{"doSelect","Index"},
            {"doSelectByIndex","Index"},{"doValue","Value"},{"doLoad","FileName"},
            {"doSave","FileName"},{"doLoadFromFile","FileName"},{"doLoadFromImage","Bitmap"}
        };
        if (inputs.contains(command)) value = input(target, inputs.value(command),value);
        if (!target.widget || !api->work(target.widget, command.constData(), value.toString().toUtf8().constData(), value.toDouble())) {
            QTextStream(stderr) << target.kind << ": unsupported action " << command << Qt::endl;
            QCoreApplication::exit(1);
        }
    }
};

Runtime *active;
}

int runCore(int argc, char **argv, const QByteArray &model) {
    QApplication app(argc,argv);
    app.setQuitOnLastWindowClosed(false);
    Runtime runtime;
    const auto project = QJsonDocument::fromJson(model).object();
    QList<int> creationOrder;
    for (const auto &value : project.value("nodes").toArray()) {
        const auto object = value.toObject();
        const int id = object.value("id").toInt();
        creationOrder.append(id);
        auto &node = runtime.nodes[id];
        node.parent = object.value("parent").toInt(-1);
        node.kind = object.value("kind").toString().toUtf8();
        const auto properties = object.value("props").toObject();
        for (auto it = properties.begin(); it != properties.end(); ++it)
            node.properties.insert(it.key().toUtf8(),it.value().toString().toUtf8());
        if (node.kind == "HCMemory") node.stored = QString::fromUtf8(node.properties.value("Default"));
    }
    for (const auto &value : project.value("links").toArray()) {
        const auto link = value.toArray();
        runtime.nodes[link.at(0).toInt()].links.insert(link.at(1).toString().toUtf8(),
            {link.at(2).toInt(),link.at(3).toString().toUtf8()});
    }
    const HiasmQtHost host{
        [](void *element,const char *name) -> const char * {
            const auto &properties = static_cast<Node *>(element)->properties;
            const auto it = properties.constFind(name);
            return it == properties.cend() ? "" : it.value().constData();
        },
        [](void *element,const char *point,const char *text,double number,int type) {
            QVariant value;
            if (type == 1) value = QString::fromUtf8(text);
            else if (type == 2) value = int(number);
            else if (type == 3) value = number;
            active->event(*static_cast<Node *>(element),point,value);
        }
    };
    active = &runtime;
    if (!runtime.api || !runtime.api->start(&host,"")) return 1;
    auto &root = runtime.nodes[project.value("root").toInt()];
    root.widget = runtime.api->create(&root,root.kind.constData(),nullptr);
    bool valid = root.widget != nullptr;
    for (int id : creationOrder) {
        auto &node = runtime.nodes[id];
        if (!valid || &node == &root || node.kind == "HCDoData" || node.kind == "HCConsole" || node.kind == "HCMemory") continue;
        node.widget = runtime.api->create(&node,node.kind.constData(),node.parent < 0 ? root.widget : runtime.nodes[node.parent].widget);
        if (!node.widget) {
            QTextStream(stderr) << "Unsupported Core widget: " << node.kind << Qt::endl;
            valid = false;
        }
    }
    int code = 1;
    if (valid) {
        runtime.ready = true;
        QTimer::singleShot(0, &app, [&] {
            runtime.api->show(root.widget);
            runtime.event(root,"onCreate",{});
        });
        code = app.exec();
    }
    runtime.ready = false;
    runtime.api->stop();
    active = nullptr;
    return code;
}
