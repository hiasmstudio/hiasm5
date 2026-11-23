#include "scheme_diagram.h"
#include "../SchemeSyntax.h"

#include <QRegularExpression>
#include <limits>

SchemeDiagram readSchemeDiagram(const QString &source) {
    // Keep the same structural rules as the legacy loader, including opaque properties.
    int errorLine;
    if (!validateSchemeSyntax(source.toStdString(), errorLine))
        return {{}, qMax(1, errorLine)};

    static const QRegularExpression add(R"(^Add\(([^,]+),([^,]+),\s*([^,\s]+)\s*,\s*([^,\s)]+)\s*\)$)");
    static const QRegularExpression link(R"(^link\(([^,]+),([^:]+):([^,]+),\[(.*)\]\)$)");
    static const QRegularExpression bend(R"(\(\s*(-?\d+(?:\.\d+)?)\s*,\s*(-?\d+(?:\.\d+)?)\s*\))");
    SchemeDiagram diagram;
    diagram.layers.append(SchemeLayer{});
    diagram.layers[0].insertionOffset = source.size();
    QList<int> stack{0};
    qsizetype lineStart = 0;
    int lineNumber = 0;
    bool inProperties = false;
    for (const auto &line : source.split(u'\n')) {
        const qsizetype start = lineStart;
        const auto directive = line.trimmed();
        const auto position = lineStart + line.indexOf(directive);
        lineStart += line.size() + 1;
        ++lineNumber;
        auto &layer = diagram.layers[stack.last()];
        if (directive.startsWith(QStringLiteral("Add("))) {
            const auto match = add.match(directive);
            bool validId, validX, validY;
            const auto id = match.captured(2).trimmed().toInt(&validId);
            const auto x = match.captured(3).toDouble(&validX);
            const auto y = match.captured(4).toDouble(&validY);
            constexpr double limit = std::numeric_limits<int>::max();
            if (!validId || !validX || !validY || qAbs(x) > limit || qAbs(y) > limit)
                return {{}, lineNumber};
            layer.elements.append({match.captured(1).trimmed(), id, QPointF(x, y),
                {position + match.capturedStart(3), match.capturedLength(3)},
                {position + match.capturedStart(4), match.capturedLength(4)}, {}, {}});
            layer.elements.last().declaration.start = start;
        } else if (directive == QStringLiteral("BEGIN_SDK")) {
            const int child = static_cast<int>(diagram.layers.size());
            layer.elements.last().child = child;
            layer.elements.last().childLayers.append(child);
            diagram.layers.append(SchemeLayer{});
            diagram.layers.last().startOffset = start;
            stack.append(child);
        } else if (directive == QStringLiteral("END_SDK")) {
            layer.insertionOffset = lineStart - line.size() - 1;
            stack.removeLast();
            auto &parent = diagram.layers[stack.last()].elements.last();
            parent.declaration.length = qMin(lineStart, source.size()) - parent.declaration.start;
        } else if (directive == QStringLiteral("{") || directive == QStringLiteral("}")) {
            inProperties = directive == QStringLiteral("{");
            if (!inProperties) {
                auto &e = layer.elements.last();
                e.bodyEnd = start;
                e.declaration.length = qMin(lineStart, source.size()) - e.declaration.start;
            }
        } else if (inProperties && !directive.startsWith(u'*')) {
            if (const auto match = link.match(directive); match.hasMatch()) {
                bool validTarget;
                const auto target = match.captured(2).trimmed().toInt(&validTarget);
                if (!validTarget)
                    return {{}, lineNumber};
                SchemeConnection connection{static_cast<int>(layer.elements.size() - 1),
                    target, match.captured(1).trimmed(), match.captured(3).trimmed(), {}};
                connection.line = {start, qMin(lineStart, source.size()) - start};
                auto bends = bend.globalMatch(match.captured(4));
                while (bends.hasNext()) {
                    const auto point = bends.next();
                    const QPointF coordinates(point.captured(1).toDouble(), point.captured(2).toDouble());
                    if (qAbs(coordinates.x()) > std::numeric_limits<int>::max() ||
                        qAbs(coordinates.y()) > std::numeric_limits<int>::max())
                        return {{}, lineNumber};
                    connection.bends.append(coordinates);
                }
                layer.connections.append(connection);
            } else if (directive.startsWith("Point(") && directive.endsWith(')')) {
                layer.elements.last().explicitPorts.append(directive.mid(6, directive.size() - 7).trimmed());
                layer.elements.last().explicitPortLines.append({start, qMin(lineStart, source.size()) - start});
            } else if (const auto equals = line.indexOf(u'='); equals > 0) {
                layer.elements.last().properties.append({line.first(equals).trimmed(), line.sliced(equals + 1)});
                layer.elements.last().propertyValues.append({lineStart - line.size() + equals, line.size() - equals - 1});
            }
        }
    }
    return diagram;
}
