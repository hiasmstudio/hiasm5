#pragma once

#include <QJsonObject>

struct LegacyProject;
struct SchemeDiagram;
QJsonObject legacySnapshot(const LegacyProject &project, const SchemeDiagram &diagram,
                           const QString &name, const QString &work);
