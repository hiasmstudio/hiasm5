#include "element_config.h"
#include "scheme_diagram.h"
#include "package_paths.h"
#include <QCoreApplication>
#include <QCursor>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QPalette>
#include <QRegularExpression>
#include <QSet>
#include <QStringDecoder>

namespace {
template<typename T>
T &entryFor(QList<T> &entries, const QByteArray &key) {
    for (auto &entry : entries)
        if (entry.name.compare(key, Qt::CaseInsensitive) == 0) return entry;
    return entries.emplaceBack(T{key});
}

void load(ElementConfig &config, const QDir &packs, const QString &pack, const QString &name, QSet<QString> &visited,
          bool legacy = false) {
    static const QRegularExpression identifier(QStringLiteral("^[A-Za-z0-9_]+$"));
    if (!identifier.match(name).hasMatch() || !identifier.match(pack).hasMatch()) {
        config.error = QStringLiteral("Invalid element or package name");
        return;
    }
    if (visited.contains(name.toLower()) || !config.error.isEmpty()) return;
    visited.insert(name.toLower());
    QFile file(packageDirectory(packs.path(), pack) + "/conf/" + name + ".ini");
    if (!file.exists()) file.setFileName(packs.filePath("_base/conf/" + name + ".ini"));
    if (!file.open(QIODevice::ReadOnly)) {
        config.error = file.fileName() + ": " + file.errorString();
        return;
    }
    const QString contents = decodePackageText(file.readAll());
    QByteArray section, ownClass, ownInterfaces, ownInherit, group;
    for (const auto &line : contents.toUtf8().split('\n')) {
        auto entry = line.trimmed();
        if (entry.startsWith('[')) { section = entry.toLower(); continue; }
        if (section == "[property]" && entry.startsWith("##")) {
            group = entry.mid(2);
            continue;
        }
        int equals = entry.indexOf('=');
        if (equals <= 0 || entry.startsWith(';') || entry.startsWith('#')) continue;
        auto key = entry.left(equals).trimmed();
        const auto value = entry.mid(equals + 1);
        if (section == "[type]" && key.toLower() == "class") ownClass = value.trimmed();
        else if (section == "[edit]" && key.toLower() == "class") config.editClass = value.trimmed();
        else if (section == "[type]" && key.toLower() == "sub") config.sub = value.trimmed();
        else if (section == "[type]" && key.toLower() == "interfaces") ownInterfaces = value.trimmed();
        else if (section == "[type]" && key.toLower() == "inherit") {
            ownInherit = value.trimmed();
            for (const auto &parent : value.split(','))
                load(config, packs, pack, QString::fromUtf8(parent.trimmed()), visited, legacy);
        } else if (section == "[property]" || section == "[methods]") {
            const bool optional = key.startsWith('*');
            bool setter = false;
            bool primary = false;
            while (key.startsWith('+') || key.startsWith('@') || key.startsWith('*')) {
                setter = setter || key.startsWith('@');
                primary = primary || key.startsWith('+');
                key.remove(0, 1);
            }
            const auto fields = value.split('|');
            if (section == "[property]") {
                // Delphi appends a display alias in brackets. The scheme stores the base key.
                const auto alias = key.indexOf('[');
                if (alias > 0 && key.endsWith(']')) key.truncate(alias);
                if (setter) {
                    auto &port = entryFor(config.ports, "do" + key);
                    port.kind = 1;
                    port.optional = true;
                    port.description = fields.value(0);
                }
                auto &p = entryFor(config.properties, key);
                p.description = fields.value(0);
                p.group = group;
                p.setter = p.setter || setter;
                p.primary = p.primary || primary;
                if (int type = fields.value(1).trimmed().toInt()) p.type = type;
                if (fields.size() >= 3) p.initial = fields.at(2).trimmed();
                if (fields.size() >= 4) p.choices = fields.at(3).trimmed().split(',');
            } else {
                // %Property% is displayed next to the port; schemes store its base name.
                const auto suffix = key.indexOf('%');
                const auto property = suffix > 0 ? key.mid(suffix + 1).split('%').value(0) : QByteArray();
                if (suffix > 0) key.truncate(suffix);
                auto &p = entryFor(config.ports, key);
                p.optional = optional;
                p.kind = fields.value(1).toInt();
                p.type = fields.value(2).toInt();
                p.propertySuffix = property;
                p.description = fields.value(0);
            }
        }
    }
    if (!ownClass.isEmpty()) config.className = ownClass;
    config.interfaces = ownInterfaces;
    config.inherit = ownInherit;
}
}

ElementConfig elementConfig(const QString &packs, const QString &pack, const QString &element) {
    ElementConfig result;
    QSet<QString> visited;
    if (packs.isEmpty()) result.error = QStringLiteral("Package not found");
    else load(result, QDir(packs), pack, element, visited);
    return result;
}

ElementConfig legacyElementConfig(const QString &packs, const QString &pack, const QString &element) {
    ElementConfig result;
    QSet<QString> visited;
    load(result, QDir(packs), pack, element, visited, true);
    return result;
}

bool decodeSchemeLines(const QString &source, QStringList &lines) {
    if (!source.startsWith('#')) return false;
    const auto data = source.toStdU32String();
    size_t pos = 1;
    while (pos < data.size()) {
        size_t count = 0, start = pos;
        while (pos < data.size() && data[pos] >= U'0' && data[pos] <= U'9') {
            count = 10 * count + data[pos++] - U'0';
            if (count > data.size()) return false;
        }
        if (pos == start || pos == data.size() || data[pos++] != U':' ||
            count >= data.size() - pos || data[pos + count] != U'|') return false;
        lines.append(QString::fromUcs4(data.data() + pos, qsizetype(count)));
        pos += count + 1;
    }
    return true;
}

QList<ConfigPort> elementPorts(const ElementConfig &config, const QList<QPair<QString,QString>> &values) {
    auto ports = config.ports;
    auto add = [&](QByteArray prefix, int kind, int count) {
        for (int n = 1; n <= count; ++n) ports.append({prefix + QByteArray::number(n), kind, 0});
    };
    if (config.className == "HubEx") { add("doWork", 1, 3); ports.append({"onEvent", 2, 0}); }
    if (config.className == "GetDataEx") { add("Var", 3, 3); ports.append({"Data", 4, 0}); }
    const bool hub = config.className == "Hub" || config.className == "HCHub" || config.className == "DPElement";
    const bool namedFrame = config.className == "EditMultiEx" || config.className == "EditPoliMulti";
    const bool namedPoints = config.className == "DPLElement";
    if (hub || namedFrame || namedPoints || config.className == "EditMulti" || config.className == "HCEditMulti") {
        if (namedFrame) ports.clear(); // Runtime instance commands belong to Dynamic mode.
        const auto names = hub || namedPoints ? config.sub.split(',') :
            QList<QByteArray>{"EventCount|onEvent","WorkCount|doWork","DataCount|Data","VarCount|Var"};
        for (int i = 0; i < names.size() && i < 4; ++i) {
            const auto field = names[i].split('|');
            if (field[0].isEmpty()) continue;
            if (namedFrame || namedPoints) {
                QStringList lines;
                for (const auto &p : config.properties)
                    if (p.name == field[0]) lines = QString::fromUtf8(p.initial).split(QStringLiteral("#13#10"));
                for (const auto &[key,value] : values) {
                    if (key.toUtf8() != field[0]) continue;
                    lines.clear();
                    if (!value.isEmpty() && !decodeSchemeLines(value, lines)) lines.clear();
                }
                for (const auto &line : lines) {
                    const auto name = line.section('=', 0, 0).trimmed().toUtf8();
                    if (!name.isEmpty()) ports.append({name, i + 1, 0});
                }
                continue;
            }
            if (field.size() != 2) continue;
            int count = 0;
            for (const auto &p : config.properties) if (p.name == field[0]) count = p.initial.toInt();
            for (const auto &p : values) if (p.first.toUtf8() == field[0]) count = p.second.toInt();
            if (count >= 0 && count <= 65535) add(field[1], i + 1, count);
        }
    }
    return ports;
}

QList<ConfigPort> schemeElementPorts(const QString &packs, const QString &pack, const SchemeDiagram &diagram,
                                     const SchemeElement &element) {
    if (element.child >= 0) {
        const int layer = element.name == "PageControl" ? element.childLayers.value(0, element.child) : element.child;
        for (const auto &frame : diagram.layers.at(layer).elements) {
            const auto config = elementConfig(packs, pack, frame.name);
            if (!config.isSdkFrame()) continue;
            auto ports = elementPorts(config, frame.properties);
            const bool polymorphic = config.className == "EditPoliMulti";
            for (auto &p : ports)
                if (!polymorphic || !p.name.startsWith("##")) p.kind = p.kind % 2 ? p.kind + 1 : p.kind - 1;
            return ports;
        }
    }
    return elementPorts(elementConfig(packs, pack, element.name), element.properties);
}

QIcon elementIcon(const QString &packs, const QString &pack, const QString &name) {
    static const QRegularExpression identifier(QStringLiteral("^[A-Za-z0-9_]+$"));
    if (packs.isEmpty() || !identifier.match(pack).hasMatch() || !identifier.match(name).hasMatch()) return {};
    for (const auto &package : {pack, QStringLiteral("_base")}) {
        const auto directory = packageDirectory(packs, package);
        if (directory.isEmpty()) continue;
        const auto base = directory + "/icon/" + name;
        for (const auto *suffix : {".ico", ".png"})
            if (QFileInfo::exists(base + suffix)) return QIcon(base + suffix);
    }
    return {};
}

QCursor elementPlacementCursor(const QIcon &icon, const QPalette &palette) {
    QPixmap cursor(QStringLiteral(":/hiasm/cursors/insert-element.png"));
    if (cursor.isNull()) return QCursor(Qt::CrossCursor);

    QPainter painter(&cursor);
    painter.setCompositionMode(QPainter::CompositionMode_SourceIn);
    painter.fillRect(cursor.rect(), palette.color(QPalette::Text));
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    const QIcon image = icon.isNull() ? QIcon(QStringLiteral(":/hiasm/logo.png")) : icon;
    image.paint(&painter, QRect(6, 6, 16, 16));
    painter.end();
    return QCursor(cursor, 3, 3);
}

QString editorPackRoot(const QString &pack) {
    static const QRegularExpression id(QStringLiteral("^[a-z0-9_]+$"));
    if (!id.match(pack).hasMatch()) return {};
    for (const auto &root : nativePackRoots())
        if (QFileInfo(QDir(root).filePath(pack + "/conf")).isDir()) return root;
    for (const auto &dir : legacyPackDirectories())
        if (!dir.fileName().compare(pack, Qt::CaseInsensitive)) return dir.absolutePath();
    return {};
}
