#pragma once

#include <QList>
#include <QPointF>
#include <QString>

struct SchemeTextSpan {
    qsizetype start;
    qsizetype length;
};

struct SchemeElement {
    QString name;
    int id;
    QPointF position;
    SchemeTextSpan xText;
    SchemeTextSpan yText;
    QList<QPair<QString, QString>> properties;
    QList<SchemeTextSpan> propertyValues;
    int child = -1;
    qsizetype bodyEnd = 0;
    SchemeTextSpan declaration{};
    QStringList explicitPorts;
    QList<SchemeTextSpan> explicitPortLines;
    QList<int> childLayers;
};

struct SchemeConnection {
    int source;
    int targetId;
    QString sourcePoint;
    QString targetPoint;
    QList<QPointF> bends;
    SchemeTextSpan line{};
};

struct SchemeLayer {
    QList<SchemeElement> elements;
    QList<SchemeConnection> connections;
    qsizetype insertionOffset = 0;
    qsizetype startOffset = 0;
};

struct SchemeDiagram {
    QList<SchemeLayer> layers;
    int errorLine = 0;
};

SchemeDiagram readSchemeDiagram(const QString &source);
