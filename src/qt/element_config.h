#pragma once

#include <QByteArray>
#include <QList>
#include <QIcon>
#include <QString>

class QCursor;
class QPalette;

struct ConfigProperty {
    QByteArray name;
    int type = 0;
    QByteArray initial;
    QList<QByteArray> choices;
    QByteArray description;
    QByteArray group;
    bool setter = false;
    bool primary = false;
};

struct ConfigPort {
    QByteArray name;
    int kind = 0;
    int type = 0;
    bool optional = false;
    QByteArray propertySuffix;
    QByteArray description;
};

struct ElementConfig {
    bool isSdkFrame() const { return className == "EditMulti" || className == "EditMultiEx" || className == "HCEditMulti" || className == "EditPoliMulti"; }
    bool createsSdkOnInsertion() const {
        return className == "MultiElement" || className == "MultiElementEx" || className == "HCMultiElement";
    }
    QByteArray sub;
    QByteArray editClass;
    QByteArray className;
    QByteArray interfaces;
    QByteArray inherit;
    QList<ConfigProperty> properties;
    QList<ConfigPort> ports;
    QString error;
};

ElementConfig elementConfig(const QString &packs, const QString &pack, const QString &element);
ElementConfig legacyElementConfig(const QString &packs, const QString &pack, const QString &element);
QList<ConfigPort> elementPorts(const ElementConfig &config, const QList<QPair<QString,QString>> &properties);
struct SchemeDiagram;
struct SchemeElement;
QList<ConfigPort> schemeElementPorts(const QString &packs, const QString &pack, const SchemeDiagram &diagram, const SchemeElement &element);
bool decodeSchemeLines(const QString &source, QStringList &lines);
QIcon elementIcon(const QString &packs, const QString &pack, const QString &name);
QCursor elementPlacementCursor(const QIcon &icon, const QPalette &palette);
QString editorPackRoot(const QString &pack);
