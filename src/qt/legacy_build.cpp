#include "legacy_build.h"
#include "legacy_snapshot.h"
#include "package_paths.h"
#include "scheme_diagram.h"

#include <QCoreApplication>
#include <QDir>
#include <QDirIterator>
#include <QFile>
#include <QJsonDocument>
#include <QProcess>
#include <QRegularExpression>
#include <QSaveFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QTemporaryDir>
#include <QTextStream>
#include <QUuid>
#include <stdexcept>

namespace {
[[noreturn]] void fail(const QString &message) { throw std::runtime_error(message.toStdString()); }

struct Catalog {
    QString connection = QUuid::createUuid().toString();
    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", connection);
    ~Catalog() {
        db = {};
        QSqlDatabase::removeDatabase(connection);
    }
};

#ifdef Q_OS_WIN
void invoke(const QString &tool, const QStringList &args, const QString &cwd, const QString &nativeArgs = {}) {
    QProcess process;
    process.setWorkingDirectory(cwd);
    process.setNativeArguments(nativeArgs);
    process.start(tool, args);
    if (!process.waitForStarted()) fail(tool + ": " + process.errorString());
    if (!process.waitForFinished(-1)) fail(tool + ": " + process.errorString());
    QTextStream(stdout) << decodePackageText(process.readAllStandardOutput()).replace("\r\n", "\n") << Qt::flush;
    QTextStream(stderr) << decodePackageText(process.readAllStandardError()).replace("\r\n", "\n") << Qt::flush;
    if (process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0)
        fail(QStringLiteral("%1 failed (%2)").arg(QFileInfo(tool).fileName()).arg(process.exitCode()));
}

void copy(const QString &from, const QString &to) {
    if (!QFile::copy(from, to)) fail(QStringLiteral("Cannot copy %1 to %2").arg(from, to));
}

void copyCode(const QDir &source, const QDir &destination) {
    if (!source.exists()) fail(QStringLiteral("Package code directory is missing: %1").arg(source.path()));
    QDirIterator files(source.path(), QDir::Files | QDir::NoSymLinks, QDirIterator::Subdirectories);
    while (files.hasNext()) {
        const auto path = files.next();
        const auto target = destination.filePath(source.relativeFilePath(path));
        if (!QDir().mkpath(QFileInfo(target).absolutePath())) fail(QStringLiteral("Cannot stage package code"));
        copy(path, target);
    }
}

void publish(const QString &from, const QString &to) {
    QFile source(from);
    QSaveFile target(to);
    if (!source.open(QIODevice::ReadOnly)) fail(from + ": " + source.errorString());
    if (!target.open(QIODevice::WriteOnly)) fail(to + ": " + target.errorString());
    while (!source.atEnd()) {
        const auto chunk = source.read(1024 * 1024);
        if (chunk.isEmpty() || target.write(chunk) != chunk.size()) fail(QStringLiteral("Cannot publish %1").arg(to));
    }
    if (!target.commit()) fail(to + ": " + target.errorString());
}
#endif
}

QString schemePackage(const QString &source) {
    for (const auto &line : source.split(u'\n')) {
        const auto text = line.trimmed();
        if (text.isEmpty() || text.startsWith('*') || text.startsWith("ver(") || text.startsWith("Pos(")) continue;
        if (text.startsWith("Make(") && text.endsWith(')')) return text.mid(5, text.size() - 6).trimmed().toLower();
        break;
    }
    return {};
}

LegacyProject legacyProject(const QString &package, const SchemeDiagram &diagram) {
    if (package.isEmpty()) return {};
    for (const auto &root : nativePackRoots()) {
        const auto dir = packageDirectory(root, package);
        if (!dir.isEmpty() && QFileInfo(dir + "/conf").isDir()) return {};
    }
    for (const auto &dir : legacyPackDirectories()) {
        if (dir.fileName().compare(package, Qt::CaseInsensitive) != 0) continue;
        LegacyProject result;
        result.directory = dir.absoluteFilePath();
        result.root = QDir::cleanPath(QDir(dir.absolutePath()).absoluteFilePath(".."));
        Catalog catalog;
        catalog.db.setConnectOptions("QSQLITE_OPEN_READONLY");
        catalog.db.setDatabaseName(QDir(result.root).filePath("Int/hiasm.db"));
        if (!catalog.db.open()) fail(QStringLiteral("Cannot read HiAsm 4 catalog for %1 (%2): %3")
            .arg(package, catalog.db.databaseName(), catalog.db.lastError().text()));
        QSqlQuery query(catalog.db);
        query.prepare("SELECT pr.entry, pr.ext, CAST(pr.make AS BLOB), CAST(c.path AS BLOB), CAST(c.cmd AS BLOB) "
                      "FROM projects pr JOIN packs p ON p.id=pr.pack_id "
                      "LEFT JOIN pack_compilers pc ON pc.pack_id=p.id AND pc.active=1 "
                      "LEFT JOIN compilers c ON c.id=pc.cmp_id WHERE lower(p.codename)=:package");
        query.bindValue(":package", package);
        if (!query.exec()) fail(query.lastError().text());
        QList<LegacyProject> entries;
        while (query.next()) {
            auto item = result;
            item.entry = query.value(0).toString();
            item.suffix = query.value(1).toString();
            item.maker = decodePackageText(query.value(2).toByteArray());
            item.compiler = decodePackageText(query.value(3).toByteArray());
            item.arguments = decodePackageText(query.value(4).toByteArray());
            entries.append(item);
        }
        if (!diagram.layers.isEmpty()) {
            for (const auto &element : diagram.layers.first().elements)
                for (const auto &item : entries)
                    if (element.name.compare(item.entry, Qt::CaseInsensitive) == 0) {
                        static const QRegularExpression extension("^[A-Za-z0-9]+$");
                        if (!extension.match(item.suffix).hasMatch() || item.maker.isEmpty() || item.compiler.isEmpty())
                            fail(QStringLiteral("HiAsm 4 project %1 has no build configuration in Int/hiasm.db").arg(item.entry));
                        return item;
                    }
        }
        fail(QStringLiteral("No project entry for %1 in HiAsm 4 Int/hiasm.db").arg(package));
    }
    return {};
}

void buildLegacyScheme(const LegacyProject &project, const SchemeDiagram &diagram,
                       const QString &name, const QString &output) {
#ifndef Q_OS_WIN
    fail(QStringLiteral("The x86 HiAsm 4 CodeGen bridge requires Windows"));
#else
    if (QFileInfo(project.maker).fileName() != project.maker)
        fail(QStringLiteral("Invalid HiAsm 4 project maker name"));
    const QDir app(QCoreApplication::applicationDirPath());
    QString bridgePath = app.filePath("hiasm4_cgbridge.exe");
#ifdef HIASM_BUILD_BRIDGE
    if (!QFileInfo(bridgePath).isFile()) bridgePath = QStringLiteral(HIASM_BUILD_BRIDGE);
#endif
    const QFileInfo bridgeFile(qEnvironmentVariable("HIASM4_CGBRIDGE", bridgePath));
    if (!bridgeFile.isFile()) fail(QStringLiteral("The x86 HiAsm 4 CodeGen bridge is missing. Build with -DHIASM_BUILD_CGBRIDGE=ON or set HIASM4_CGBRIDGE."));
    QTemporaryDir temp(QDir::temp().filePath("hiasm-legacy-XXXXXX"));
    if (!temp.isValid()) fail(temp.errorString());
    const QDir work(temp.path()), root(project.root), pack(project.directory);
    const auto path = [](const QString &value) { return QDir::toNativeSeparators(QDir::cleanPath(value)); };
    const auto workPath = [&](const QString &file) { return path(work.filePath(file)); };
    const auto source = workPath("HiAsm.dpr");
    const auto staged = workPath("result/HiAsm." + project.suffix);
    const auto bridge = workPath("host/hiasm4_cgbridge.exe");
    const auto maker = path(pack.filePath("make/" + project.maker));
    try {
        if (!work.mkpath("host/Elements/Delphi") || !work.mkpath("result")) fail(QStringLiteral("Cannot create legacy build directories"));
        copyCode(QDir(pack.filePath("code")), work);
        copy(bridgeFile.absoluteFilePath(), bridge);
        const auto ftcg = pack.filePath("FTCG_CodeGen.dll");
        if (QFileInfo::exists(ftcg)) copy(ftcg, workPath("host/Elements/Delphi/FTCG_CodeGen.dll"));
        const auto snapshot = workPath("scheme.json");
        QFile json(snapshot);
        const auto bytes = QJsonDocument(legacySnapshot(project, diagram, name, path(work.path()))).toJson(QJsonDocument::Compact);
        if (!json.open(QIODevice::WriteOnly) || json.write(bytes) != bytes.size() || !json.flush())
            fail(snapshot + ": " + json.errorString());
        json.close();

        invoke(bridge, {"--codegen", path(pack.filePath("CodeGen.dll")), "--make", maker, "--scheme", snapshot,
                        "--work", path(work.path()), "--project", source, "--rc", workPath("allres.rc"),
                        "--default-icon", path(root.filePath("Int/main.ico"))}, work.path());
        invoke(path(root.filePath("compiler/GoRC.exe")), {"/r", "allres.rc"}, work.path());
        if (!QFileInfo::exists(workPath("allres.res"))) fail(QStringLiteral("GoRC did not produce allres.res"));

        const auto rootPath = path(root.path()) + '\\';
        QString compiler = project.compiler;
        compiler.replace("%curdir%", rootPath);
        compiler = path(QDir::isAbsolutePath(compiler) ? compiler : root.filePath(compiler));
        if (!QFileInfo(compiler).isFile()) fail(QStringLiteral("HiAsm 4 compiler is missing: %1").arg(compiler));
        QString arguments = project.arguments;
        arguments.replace("%fname%", source).replace("%opath%", path(work.path()) + '\\')
            .replace("%upath%", path(work.path())).replace("%curdir%", rootPath)
            .replace("%oname%", QStringLiteral("HiAsm.") + project.suffix);
        QString compilerCwd = work.path();
        if (QFileInfo(compiler).fileName().compare("dcc32.exe", Qt::CaseInsensitive) == 0) {
            // Delphi resolves its RTL and RLINK32 relative to the compiler directory.
            // Stage that directory so even compiler side files stay outside the installation.
            compilerCwd = work.filePath("compiler");
            const QFileInfo original(compiler);
            copyCode(original.absoluteDir(), QDir(compilerCwd));
            compiler = path(QDir(compilerCwd).filePath(original.fileName()));
        }
        invoke(compiler, {}, compilerCwd, arguments);
        QString artifact = workPath("HiAsm." + project.suffix);
        if (!QFileInfo(artifact).isFile()) {
            // The .NET maker writes the assembly name into its project file.
            QFile generated(source);
            if (generated.open(QIODevice::ReadOnly)) {
                static const QRegularExpression assembly("<AssemblyName>([^<>/\\\\]+)</AssemblyName>");
                const auto match = assembly.match(decodePackageText(generated.readAll()));
                if (match.hasMatch()) artifact = workPath(match.captured(1) + '.' + project.suffix);
            }
        }
        if (!QFileInfo(artifact).isFile()) fail(QStringLiteral("The HiAsm 4 compiler did not produce %1").arg(artifact));
        invoke(bridge, {"--complete", "--make", maker, "--project", source, "--app", staged}, work.path());
        publish(QFileInfo(staged).isFile() ? staged : artifact, output);
    } catch (const std::exception &) {
        QTextStream(stderr) << "HiAsm 4 workspace kept for inspection: " << work.path() << Qt::endl;
        temp.setAutoRemove(false);
        throw;
    }
#endif
}
