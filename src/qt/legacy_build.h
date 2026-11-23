#pragma once

#include <QString>

struct LegacyProject {
    QString directory;
    QString root;
    QString entry;
    QString suffix;
    QString maker;
    QString compiler;
    QString arguments;
};

struct SchemeDiagram;
QString schemePackage(const QString &source);
LegacyProject legacyProject(const QString &package, const SchemeDiagram &diagram);
void buildLegacyScheme(const LegacyProject &project, const SchemeDiagram &diagram,
                       const QString &name, const QString &output);
