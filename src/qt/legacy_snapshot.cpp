#include "legacy_snapshot.h"
#include "legacy_build.h"
#include "element_config.h"
#include "scheme_diagram.h"
#include "../RTCG/CGTShare.h"

#include <QColor>
#include <QFileInfo>
#include <QJsonArray>
#include <QRegularExpression>
#include <QSet>
#include <stdexcept>

namespace {
[[noreturn]] void fail(const QString &error) { throw std::runtime_error(error.toStdString()); }

int delphiColor(const QString &value) {
    static const QStringList systemColors = {
        "clScrollBar", "clBackground", "clActiveCaption", "clInactiveCaption", "clMenu", "clWindow",
        "clWindowFrame", "clMenuText", "clWindowText", "clCaptionText", "clActiveBorder", "clInactiveBorder",
        "clAppWorkSpace", "clHighlight", "clHighlightText", "clBtnFace", "clBtnShadow", "clGrayText",
        "clBtnText", "clInactiveCaptionText", "clBtnHighlight", "cl3DDkShadow", "cl3DLight", "clInfoText", "clInfoBk"
    };
    const auto index = systemColors.indexOf(value);
    if (index >= 0) return -16777216 + int(index);
    if (value == "clNone") return 0x1fffffff;
    if (value == "clDefault") return 0x20000000;
    if (value.startsWith('$')) return int(value.mid(1).toUInt(nullptr, 16));
    static const QHash<QString, int> extra{{"clMoneyGreen", 0xc0dcc0}, {"clSkyBlue", 0xf0caa6},
                                          {"clCream", 0xf0fbff}, {"clMedGray", 0xa4a0a0}};
    if (extra.contains(value)) return extra.value(value);
    QColor color(value.startsWith("cl") ? value.mid(2) : value);
    if (color.isValid()) return color.red() | (color.green() << 8) | (color.blue() << 16);
    return int(value.toLongLong());
}

QString textValue(QString value, bool list = false) {
    if (value.startsWith('#')) {
        QStringList lines;
        if (!decodeSchemeLines(value, lines)) fail(QStringLiteral("Invalid length-prefixed text property"));
        return lines.join("\r\n");
    }
    if (list) return value.replace("#13#10", "\r\n");
    if (value.startsWith('"') && value.endsWith('"') && value.size() >= 2) return value.mid(1, value.size() - 2);
    return value;
}

QJsonObject propertyValue(const ConfigProperty &p, const QString &value, bool defaults = false) {
    QJsonObject result;
    switch (p.type) {
    case data_int: case data_combo: case data_comboEx: case data_flags:
        result.insert("int", int(value.toLongLong()));
        if (p.type == data_comboEx) result.insert("str", QString::fromUtf8(p.choices.value(value.toInt())));
        break;
    case data_color:
        result.insert("int", delphiColor(value));
        result.insert("str", value);
        break;
    case data_real:
        result.insert("real", value.toDouble());
        break;
    case data_font: {
        const auto fields = value.isEmpty() ? QStringList{"MS Sans Serif", "8", "0", "0", "1"}
            : (value.startsWith('[') && value.endsWith(']') ? value.mid(1, value.size() - 2) : value).split(',');
        result.insert("font", QJsonObject{{"name", fields.value(0)}, {"size", fields.value(1).toInt()},
            {"style", fields.value(2).toInt()}, {"color", fields.value(3).toInt()}, {"charset", fields.value(4).toInt()}});
        break;
    }
    case data_data: {
        QJsonObject data{{"type", data_null}};
        if (!value.isEmpty()) {
            const auto open = value.indexOf('(');
            const auto content = value.mid(open + 1, value.size() - open - 2);
            const auto kind = value.left(open);
            if (open < 0 || !value.endsWith(')')) fail(QStringLiteral("Invalid HiAsm 4 Data property"));
            if (kind == "String") data = {{"type", data_str}, {"str", content}};
            else if (kind == "Integer") data = {{"type", data_int}, {"int", content.toInt()}};
            else if (kind == "Real") data = {{"type", data_real}, {"real", content.toDouble()}};
            else fail(QStringLiteral("Unsupported HiAsm 4 Data type: %1").arg(kind));
        }
        result.insert("data", data);
        break;
    }
    case data_array: {
        const auto itemType = p.initial.toInt();
        QJsonArray items;
        if (!defaults && value != "[]") {
            const auto bytes = value.toStdU32String();
            size_t pos = 1;
            if (bytes.size() < 2 || bytes.front() != U'[' || bytes.back() != U']' || itemType == data_array)
                fail(QStringLiteral("Invalid or nested HiAsm 4 array"));
            const size_t end = bytes.size() - 1;
            while (pos < end) {
                if (bytes[pos++] != U'\'') fail(QStringLiteral("Invalid HiAsm 4 array item"));
                const auto keyEnd = bytes.find(U'\'', pos);
                if (keyEnd == std::u32string::npos || keyEnd + 1 >= end || bytes[keyEnd + 1] != U'=')
                    fail(QStringLiteral("Invalid HiAsm 4 array item name"));
                const auto key = QString::fromUcs4(bytes.data() + pos, keyEnd - pos);
                pos = keyEnd + 2;
                const auto start = pos;
                int nesting = 0;
                bool quoted = false;
                while (pos < end) {
                    const auto c = bytes[pos];
                    if (c == U'"') quoted = !quoted;
                    if (!quoted) {
                        if (c == U',' && nesting == 0) break;
                        if (c == U'(' || c == U'[') ++nesting;
                        if (c == U')' || c == U']') --nesting;
                        if (c == U'#' && pos == start) {
                            ++pos;
                            while (pos < end && bytes[pos] >= U'0' && bytes[pos] <= U'9') {
                                size_t count = 0;
                                while (pos < end && bytes[pos] >= U'0' && bytes[pos] <= U'9') {
                                    count = count * 10 + bytes[pos++] - U'0';
                                    if (count > end) fail(QStringLiteral("Invalid HiAsm 4 array list"));
                                }
                                if (pos >= end || bytes[pos++] != U':' || count >= end - pos || bytes[pos + count] != U'|')
                                    fail(QStringLiteral("Invalid HiAsm 4 array list"));
                                pos += count + 1;
                            }
                            continue;
                        }
                    }
                    ++pos;
                }
                if (quoted || nesting) fail(QStringLiteral("Invalid HiAsm 4 array value"));
                auto item = propertyValue({{}, itemType, {}, {}}, QString::fromUcs4(bytes.data() + start, pos - start));
                item.insert("name", key);
                item.insert("type", itemType);
                items.append(item);
                if (pos < end) ++pos;
            }
        }
        result.insert("array", QJsonObject{{"type", itemType}, {"items", items}});
        break;
    }
    case data_icon: case data_stream: case data_bitmap: case data_wave: case data_jpeg: case data_menu:
        if (!value.isEmpty()) result.insert("resource", value);
        break;
    case data_list: case data_code: case data_script: case data_str: case data_stock: case data_element:
        result.insert("str", textValue(value, p.type == data_list || p.type == data_code || p.type == data_script));
        break;
    default:
        if (!defaults) fail(QStringLiteral("Unsupported HiAsm 4 property type %1").arg(p.type));
    }
    return result;
}

struct Endpoint { int element = -1; int port = -1; };
struct Port {
    QByteArray name;
    QByteArray legacyName;
    QByteArray dynamicName;
    int kind;
    int type;
    Endpoint link;
};
struct Element {
    const SchemeElement *scheme;
    ElementConfig config;
    QList<Port> ports;
    QJsonObject json;
    int classIndex;
};

int portIndex(const Element &element, const QByteArray &name) {
    for (int i = 0; i < element.ports.size(); ++i) {
        const auto &port = element.ports.at(i);
        if (port.name.compare(name, Qt::CaseInsensitive) == 0 || port.legacyName.compare(name, Qt::CaseInsensitive) == 0)
            return i;
    }
    return -1;
}

QString property(const SchemeElement &element, const ElementConfig &config, const QByteArray &name) {
    for (const auto &[key, value] : element.properties)
        if (key.toUtf8().compare(name, Qt::CaseInsensitive) == 0) return value;
    for (const auto &p : config.properties)
        if (p.name.compare(name, Qt::CaseInsensitive) == 0) return QString::fromUtf8(p.initial);
    return {};
}

constexpr int HubEx = 20, GetDataEx = 21;

Endpoint realLink(const QList<Element> &elements, Endpoint endpoint) {
    QSet<QPair<int, int>> seen;
    while (endpoint.element >= 0 && endpoint.port >= 0) {
        const auto &node = elements.at(endpoint.element);
        QByteArray across;
        if (node.classIndex == HubEx) across = "onEvent";
        else if (node.classIndex == GetDataEx) across = "Data";
        else return endpoint;
        const QPair<int,int> key{endpoint.element, endpoint.port};
        if (seen.contains(key)) break;
        seen.insert(key);
        const auto index = portIndex(node, across);
        if (index < 0) break;
        endpoint = node.ports.at(index).link;
    }
    return {};
}
}

QJsonObject legacySnapshot(const LegacyProject &project, const SchemeDiagram &diagram,
                           const QString &name, const QString &work) {
    static const QHash<QByteArray, int> classes{{"Hub", CI_DPElement}, {"DPElement", CI_DPElement},
        {"DPLElement", CI_DPLElement}, {"MultiElement", CI_MultiElement}, {"MultiElementEx", CI_MultiElement},
        {"EditMulti", CI_EditMulti}, {"EditMultiEx", CI_EditMultiEx}, {"WinElement", CI_WinElement},
        {"InlineCode", CI_InlineCode}, {"DrawElement", CI_DrawElement}, {"UseHiDLL", CI_UseHiDLL},
        {"HubEx", HubEx}, {"GetDataEx", GetDataEx}};
    const QFileInfo package(project.directory);
    QHash<QString, ElementConfig> configs;
    QList<Element> elements;
    QList<int> owners(diagram.layers.size(), -1), offsets;
    QHash<int, int> ids;
    bool entrySet = false;
    for (int layer = 0; layer < diagram.layers.size(); ++layer) {
        offsets.append(int(elements.size()));
        for (const auto &element : diagram.layers.at(layer).elements) {
            if (!configs.contains(element.name))
                configs.insert(element.name, legacyElementConfig(package.absolutePath(), package.fileName(), element.name));
            auto config = configs.value(element.name);
            if (!config.error.isEmpty()) fail(config.error);
            if (ids.contains(element.id)) fail(QStringLiteral("Duplicate element ID %1").arg(element.id));
            ids.insert(element.id, int(elements.size()));
            if (element.child >= 0) owners[element.child] = int(elements.size());

            const bool entry = !entrySet && layer == 0 && element.name.compare(project.entry, Qt::CaseInsensitive) == 0;
            entrySet |= entry;
            const bool editorOnly = config.className == "ITElement" || config.className == "HubEx" || config.className == "GetDataEx";
            // HiAsm 4 flag values (CGTShare.pas), not the native Qt/RTCG flags.
            const int flags = (entry ? 0x202 : 0) | (config.editClass.isEmpty() ? 0 : 8) |
                (element.child >= 0 ? 0x40 : 0) | (editorOnly ? 0x400 : 0);
            const int classIndex = classes.value(config.className, element.child >= 0 ? CI_MultiElement : 0);
            QJsonArray props;
            for (const auto &p : config.properties) {
                const auto initial = propertyValue(p, QString::fromUtf8(p.initial), true);
                auto value = initial;
                for (const auto &[key, text] : element.properties) {
                    if (key.toUtf8().compare(p.name, Qt::CaseInsensitive) != 0) continue;
                    try {
                        value = propertyValue(p, text);
                    } catch (const std::exception &error) {
                        fail(QStringLiteral("%1.%2: %3").arg(element.name, QString::fromUtf8(p.name), QString::fromUtf8(error.what())));
                    }
                    break;
                }
                value.insert("default", value == initial);
                value.insert("name", QString::fromUtf8(p.name));
                value.insert("type", p.type);
                props.append(value);
            }
            QJsonObject json{{"class", element.name}, {"sub", QString::fromUtf8(config.sub)},
                {"interfaces", QString::fromUtf8(config.interfaces)}, {"inherit", QString::fromUtf8(config.inherit)},
                {"flags", flags}, {"classIndex", classIndex}, {"eid", element.id}, {"group", 0},
                {"x", int(element.position.x())}, {"y", int(element.position.y())},
                {"w", property(element, config, "Width").toInt()}, {"h", property(element, config, "Height").toInt()},
                {"parent", layer}, {"inner", element.child}, {"props", props}};
            elements.append(Element{&element, std::move(config), {}, json, classIndex});
        }
    }

    for (auto &element : elements) {
        const auto &scheme = *element.scheme;
        ElementConfig portConfig = element.config;
        const auto *values = &scheme.properties;
        bool mirror = false;
        if (scheme.child >= 0) {
            for (const auto &frame : diagram.layers.at(scheme.child).elements) {
                const auto config = configs.value(frame.name);
                if (!config.isSdkFrame()) continue;
                portConfig = config;
                values = &frame.properties;
                mirror = true;
                break;
            }
        }
        if (portConfig.className == "InlineCode") portConfig.className = "DPLElement";
        auto ports = elementPorts(portConfig, *values);
        if (portConfig.className == "EditMultiEx") {
            for (const auto &p : portConfig.ports)
                if (scheme.explicitPorts.contains(QString::fromUtf8(p.name), Qt::CaseInsensitive)) ports.append(p);
        }
        const auto requested = [&](const QByteArray &name) {
            if (scheme.explicitPorts.contains(QString::fromUtf8(name), Qt::CaseInsensitive)) return true;
            // Some HiAsm 4 files store a connection to an optional port without Point().
            const auto &layer = diagram.layers.at(element.json.value("parent").toInt());
            for (const auto &wire : layer.connections) {
                if ((&layer.elements.at(wire.source) == &scheme && wire.sourcePoint.toUtf8().compare(name, Qt::CaseInsensitive) == 0) ||
                    (wire.targetId == scheme.id && wire.targetPoint.toUtf8().compare(name, Qt::CaseInsensitive) == 0)) return true;
            }
            return false;
        };
        for (const auto &p : ports) {
            if (p.kind < 1 || p.kind > 4) continue;
            if (p.optional && !requested(p.name)) continue;
            const auto kind = mirror ? ((p.kind - 1) ^ 1) + 1 : p.kind;
            QByteArray dpe, legacyName = p.name;
            if (!p.propertySuffix.isEmpty())
                legacyName += QByteArray::number(property(scheme, element.config, p.propertySuffix).toInt());
            static const QList<QByteArray> multiNames{"doWork", "OnEvent", "getVar", "_Data"};
            static const QList<QByteArray> frameNames{"onEvent", "doWork", "Var", "Data"};
            if (portConfig.className == "EditMultiEx") dpe = multiNames.at(kind - 1);
            else if (portConfig.className == "EditMulti") dpe = frameNames.at(p.kind - 1);
            else if (element.classIndex == CI_DPElement || element.classIndex == CI_DPLElement) {
                const auto fields = element.config.sub.split(',').value(kind - 1).split('|');
                dpe = element.classIndex == CI_DPElement ? fields.value(1) : fields.value(0);
            }
            element.ports.append({p.name, legacyName, dpe, kind, p.type, {}});
        }
    }

    QJsonArray sdks;
    for (int layer = 0; layer < diagram.layers.size(); ++layer) {
        const auto &sdk = diagram.layers.at(layer);
        QJsonArray members;
        for (int i = 0; i < sdk.elements.size(); ++i) members.append(offsets.at(layer) + i);
        sdks.append(QJsonObject{{"parent", owners.at(layer)}, {"elements", members}});
        for (const auto &wire : sdk.connections) {
            const int from = offsets.at(layer) + wire.source;
            const int to = ids.value(wire.targetId, -1);
            const int a = portIndex(elements.at(from), wire.sourcePoint.toUtf8());
            const int b = to < 0 ? -1 : portIndex(elements.at(to), wire.targetPoint.toUtf8());
            if (a < 0 || b < 0 || elements.at(to).json.value("parent").toInt() != layer)
                fail(QStringLiteral("Cannot resolve HiAsm 4 link %1.%2 to %3.%4").arg(sdk.elements.at(wire.source).name,
                    wire.sourcePoint, QString::number(wire.targetId), wire.targetPoint));
            if (elements[from].ports[a].link.element >= 0 || elements[to].ports[b].link.element >= 0)
                fail(QStringLiteral("A HiAsm 4 point has multiple links"));
            elements[from].ports[a].link = {to, b};
            elements[to].ports[b].link = {from, a};
        }
    }

    QJsonArray nodes;
    for (auto &element : elements) {
        QJsonArray points;
        int indices[4]{};
        for (const auto &p : element.ports) {
            QJsonObject point{{"name", QString::fromUtf8(p.legacyName)}, {"dpe", QString::fromUtf8(p.dynamicName)},
                {"index", indices[p.kind - 1]++}, {"type", p.kind}, {"dataType", p.type}};
            if (p.link.element >= 0) {
                point.insert("link", QJsonArray{p.link.element, p.link.port});
                const auto real = realLink(elements, p.link);
                if (real.element >= 0) point.insert("rlink", QJsonArray{real.element, real.port});
            }
            points.append(point);
        }
        element.json.insert("points", points);
        nodes.append(element.json);
    }
    return {{"version", 1}, {"params", QJsonObject{{"projectPath", work}, {"projectName", name}}},
            {"sdks", sdks}, {"elements", nodes}};
}
