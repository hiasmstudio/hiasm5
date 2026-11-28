#pragma once

#include "project_type.h"
#include "scheme_diagram.h"
#include <QByteArray>
#include <QString>

struct GeneratedScheme {
    QByteArray source;
    ProjectType type = ProjectType::Unknown;
    bool gui = false;
    bool release = false;
    bool core = false;
    SchemeWebWidgets webWidgets;
};

// Throws std::runtime_error with a diagnostic when the project cannot be built.
GeneratedScheme generateCppScheme(const QString &source, const QString &packs,
                                  const QString &library, const QString &projectName);
