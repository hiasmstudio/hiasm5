#pragma once

#include <QFileInfo>
#include <QStringList>
#include <optional>

QStringList nativePackRoots();
QFileInfoList legacyPackDirectories();
bool isInLegacyInstallation(const QString &path);
QString packageDirectory(const QString &root, const QString &id);
QString packageDatabase(const QString &directory);
std::optional<QByteArray> encodeWindows1251(QStringView text);
QString decodePackageText(const QByteArray &bytes);
