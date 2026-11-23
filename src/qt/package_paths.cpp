#include "package_paths.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QRegularExpression>
#include <QSet>
#include <QStringDecoder>

QStringList nativePackRoots() {
    const QDir app(QCoreApplication::applicationDirPath());
    QStringList roots{qEnvironmentVariable("HIASM_PACKS_DIR"), app.absoluteFilePath("packs"),
                      app.absoluteFilePath("../packs"), QStringLiteral(HIASM_SOURCE_PACKS)};
    roots.removeAll(QString());
    return roots;
}

namespace {
// Older HiAsm files and SQLite TEXT fields use Windows-1251. The minimal
// Qt runtime does not always include ICU or the optional legacy codecs.
constexpr char16_t cp1251Extended[] =
        u"ЂЃ‚ѓ„…†‡€‰Љ‹ЊЌЋЏђ‘’“”•–—\uFFFD™љ›њќћџ"
        u"\u00A0ЎўЈ¤Ґ¦§Ё©Є«¬\u00AD®Ї°±Ііґµ¶·ё№є»јЅѕї";
}

QString decodePackageText(const QByteArray &bytes) {
    QStringDecoder utf8(QStringDecoder::Utf8, QStringConverter::Flag::Stateless);
    QString text = utf8(bytes);
    if (!utf8.hasError()) return text;

    text.clear();
    for (unsigned char ch : bytes)
        text.append(QChar(ch < 0x80 ? ch : ch >= 0xc0 ? u'А' + ch - 0xc0 : cp1251Extended[ch - 0x80]));
    return text;
}

std::optional<QByteArray> encodeWindows1251(QStringView text) {
    QByteArray bytes;
    bytes.reserve(text.size());
    const QStringView extended(cp1251Extended);
    for (const auto ch : text) {
        if (ch.unicode() < 0x80) bytes.append(char(ch.unicode()));
        else if (ch >= u'А' && ch <= u'я') bytes.append(char(0xc0 + ch.unicode() - u'А'));
        else {
            const auto index = extended.indexOf(ch);
            if (index < 0) return std::nullopt;
            bytes.append(char(0x80 + index));
        }
    }
    return bytes;
}

QString packageDirectory(const QString &root, const QString &id) {
    static const QRegularExpression identifier(QStringLiteral("^[a-zA-Z0-9_]+$"));
    if (root.isEmpty() || !identifier.match(id).hasMatch()) return {};
    const QDir parent(root);
    const auto direct = parent.filePath(id);
    if (QFileInfo(direct).isDir()) return direct;
    for (const auto &dir : parent.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot))
        if (!dir.fileName().compare(id, Qt::CaseInsensitive)) return dir.absoluteFilePath();
    return {};
}

QString packageDatabase(const QString &directory) {
    if (directory.isEmpty()) return {};
    for (const auto &name : {"elements.db", "Elements.db"}) {
        const auto path = QDir(directory).filePath(name);
        if (QFileInfo(path).isFile()) return path;
    }
    return {};
}

static QString expandRoot(QString path) {
    static const QRegularExpression variable(QStringLiteral(R"(%([^%]+)%|\$\{([^}]+)\})"));
    auto matches = variable.globalMatch(path);
    QString expanded;
    qsizetype end = 0;
    while (matches.hasNext()) {
        const auto match = matches.next();
        const auto key = match.captured(match.captured(1).isEmpty() ? 2 : 1).toUtf8();
        expanded += path.mid(end, match.capturedStart() - end) + qEnvironmentVariable(key.constData(), match.captured());
        end = match.capturedEnd();
    }
    expanded += path.mid(end);
    expanded.replace(u'\\', u'/');
    if (expanded == "~" || expanded.startsWith("~/")) expanded.replace(0, 1, QDir::homePath());
    return expanded;
}

static QStringList legacyInstallationRoots() {
    QStringList roots;
    QDir configDir;
    for (const auto &root : nativePackRoots()) {
        QFile config(QDir(root).filePath("_hiasm4/paths.ini"));
        if (!config.open(QIODevice::ReadOnly)) continue;
        configDir = QFileInfo(config).absoluteDir();
        roots = decodePackageText(config.readAll()).split(u'\n');
        break;
    }
    if (qEnvironmentVariableIsSet("HIASM4_ROOTS"))
        roots = qEnvironmentVariable("HIASM4_ROOTS").split(u';');

    QStringList found;
    for (auto line : roots) {
        line = line.trimmed();
        if (line.isEmpty() || line.startsWith('#') || line.startsWith(';') || line.startsWith('[')) continue;
        if (line.size() >= 2 && line.front() == '"' && line.back() == '"') line = line.mid(1, line.size() - 2);
        const auto installation = QFileInfo(configDir.absoluteFilePath(expandRoot(line))).canonicalFilePath();
        if (!installation.isEmpty() && !found.contains(installation)) found.append(installation);
    }
    return found;
}

QFileInfoList legacyPackDirectories() {
    QFileInfoList found;
    QSet<QString> seen{"_base", "cpp", "web", "modules"};
    static const QRegularExpression id(QStringLiteral("^[a-zA-Z0-9][a-zA-Z0-9_]*$"));
    for (const auto &root : legacyInstallationRoots()) {
        const auto elements = packageDirectory(root, "Elements");
        if (elements.isEmpty()) continue;
        for (const auto &dir : QDir(elements).entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name)) {
            const auto name = dir.fileName().toLower();
            if (seen.contains(name) || !id.match(name).hasMatch() ||
                !QFileInfo(dir.filePath() + "/conf").isDir() || packageDatabase(dir.filePath()).isEmpty()) continue;
            seen.insert(name);
            found.append(dir);
        }
    }
    return found;
}

bool isInLegacyInstallation(const QString &path) {
    if (path.isEmpty()) return false;
    // Resolve the closest existing ancestor so new paths and links are covered.
    QFileInfo location(QFileInfo(path).absoluteFilePath());
    while (!location.exists() && !location.dir().isRoot())
        location.setFile(location.absolutePath());
    const QString resolved = location.exists() ? location.canonicalFilePath() : location.absolutePath();
#ifdef Q_OS_WIN
    constexpr auto caseSensitivity = Qt::CaseInsensitive;
#else
    constexpr auto caseSensitivity = Qt::CaseSensitive;
#endif
    for (const auto &root : legacyInstallationRoots()) {
        if (!root.isEmpty() && (resolved.compare(root, caseSensitivity) == 0 ||
                               resolved.startsWith(root + '/', caseSensitivity))) return true;
    }
    return false;
}
