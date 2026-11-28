#include "scheme_build.h"
#include "scheme_codegen.h"
#include "legacy_build.h"
#include "scheme_diagram.h"

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QProcess>
#include <QSaveFile>
#include <QStandardPaths>
#include "package_paths.h"
#include <QTemporaryDir>
#include <QTextStream>
#include <stdexcept>

namespace {
void fail(const QString &message) { throw std::runtime_error(message.toStdString()); }

QString tool(const char *variable, const QStringList &candidates, const QString &name) {
    const QString override = qEnvironmentVariable(variable);
    if (!override.isEmpty()) return override;
    for (const auto &candidate : candidates)
        if (QFileInfo(candidate).isExecutable()) return candidate;
    return QStandardPaths::findExecutable(name);
}

void execute(const QString &command, const QStringList &args, const QProcessEnvironment &env) {
    QProcess process;
    process.setProcessEnvironment(env);
    process.setProcessChannelMode(QProcess::ForwardedChannels);
    process.start(command, args);
    if (!process.waitForStarted()) fail(command + ": " + process.errorString());
    if (!process.waitForFinished(-1) || process.exitStatus() != QProcess::NormalExit || process.exitCode())
        fail(QStringLiteral("%1 failed (%2)").arg(command).arg(process.exitCode()));
}

void writeFile(const QString &path, const QByteArray &bytes) {
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit())
        fail(path + ": " + file.errorString());
}

#ifdef Q_OS_WIN
void deployFile(const QString &source, const QString &destination) {
    QFile input(source);
    if (!input.open(QIODevice::ReadOnly)) fail(source + ": " + input.errorString());
    const auto bytes = input.readAll();
    QFile existing(destination);
    if (existing.open(QIODevice::ReadOnly) && existing.readAll() == bytes) return;
    existing.close();
    writeFile(destination, bytes);
}
#endif

QString ruffleBundle(const QString &packs) {
    const QString configured = qEnvironmentVariable("HIASM_RUFFLE_DIR");
    const QDir cpp(packs + "/cpp");
    const QStringList candidates = configured.isEmpty()
        ? QStringList{cpp.absoluteFilePath("qt/ruffle"),
                      cpp.absoluteFilePath("../../thirdparty/ruffle/web/packages/selfhosted/dist")}
        : QStringList{QDir(configured).absolutePath()};
    for (const auto &candidate : candidates) {
        const QDir bundle(candidate);
        if (QFileInfo::exists(bundle.filePath("ruffle.js")) &&
            !bundle.entryList({"*.wasm"}, QDir::Files).isEmpty())
            return bundle.absolutePath();
    }
    fail(QStringLiteral("Flash requires Ruffle Self-Hosted. Build thirdparty/ruffle/web or set "
                        "HIASM_RUFFLE_DIR to the directory with ruffle.js and its .wasm files."));
    return {};
}

QString packPath() {
    const QDir app(QCoreApplication::applicationDirPath());
    const QString configured = qEnvironmentVariable("HIASM_PACKS_DIR");
    if (!configured.isEmpty()) return configured;
    for (const auto &path : {app.absoluteFilePath("../packs"), app.absoluteFilePath("packs"),
                             QStringLiteral(HIASM_SOURCE_PACKS)})
        if (QFileInfo::exists(path + "/cpp/code/codegen.ini")) return QDir(path).absolutePath();
    fail(QStringLiteral("C++ package not found (HIASM_PACKS_DIR)"));
    return {};
}

int compile(const QString &scheme, const QString &output, bool generateOnly) {
    QFile file(scheme);
    if (!file.open(QIODevice::ReadOnly)) fail(scheme + ": " + file.errorString());
    const auto bytes = file.readAll();
    if (bytes.contains('\0') || (bytes.startsWith("\xef\xbb\xbf") && !bytes.isValidUtf8()))
        fail(QStringLiteral("Invalid scheme encoding"));
    QString text = decodePackageText(bytes);
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n")).replace(u'\r', u'\n');
    const auto package = schemePackage(text);
    if (!package.isEmpty() && package != "cpp" && package != "_base" && package != "web" && package != "modules") {
        const auto diagram = readSchemeDiagram(text);
        if (diagram.errorLine) fail(QStringLiteral("Invalid scheme at line %1").arg(diagram.errorLine));
        const auto legacy = legacyProject(package, diagram);
        if (!legacy.directory.isEmpty()) {
            if (generateOnly) fail(QStringLiteral("HiAsm 4 produces a multi-file project. Use --build to compile it."));
            buildLegacyScheme(legacy, diagram, QFileInfo(scheme).completeBaseName(), output);
            QTextStream(stdout) << output << Qt::endl;
            return 0;
        }
        fail(QStringLiteral("Package %1 is not configured. Check HIASM4_ROOTS or packs/_hiasm4/paths.ini.").arg(package));
    }
    const QString packs = packPath();
    const QString compilerDir = QDir(packs).absoluteFilePath("cpp/compiler");
#ifdef Q_OS_WIN
    const QString libName = "/CodeGen.dll";
    const QString suffix = ".exe";
#elif defined(Q_OS_MACOS)
    const QString libName = "/libCodeGen.dylib";
    const QString suffix;
#else
    const QString libName = "/libCodeGen.so";
    const QString suffix;
#endif
    QString library = compilerDir + libName;
    if (!QFileInfo::exists(library))
        library = QCoreApplication::applicationDirPath() + "/packs/cpp/compiler" + libName;
    const auto generated = generateCppScheme(text, packs, library, QFileInfo(scheme).completeBaseName());
    if (generateOnly || textArtifact(generated.type)) { writeFile(output, generated.source); return 0; }

    QTemporaryDir temp;
    if (!temp.isValid()) fail(temp.errorString());
    const QString source = temp.filePath("scheme.cpp");
    writeFile(source, generated.source);
    const QString bundledQt = compilerDir + "/qt6";
    QString prefix = qEnvironmentVariable("HIASM_QT_DIR",
        QFileInfo::exists(bundledQt + "/lib/cmake/Qt6/Qt6Config.cmake") ? bundledQt :
        QStringLiteral(HIASM_BUILD_QT_PREFIX));
    const bool web = generated.webWidgets.needsWebEngine();
#ifndef Q_OS_WIN
    if (web) prefix = qEnvironmentVariable("HIASM_WEB_QT_DIR", prefix);
#endif
    const QString ruffle = generated.webWidgets.flash ? ruffleBundle(packs) : QString();
    const QString bundledBin = compilerDir + "/llvm-mingw/bin";
    const QString compilerBin = qEnvironmentVariable("HIASM_QT_COMPILER_DIR",
        QDir(bundledBin).exists() ? bundledBin : QFileInfo(QStringLiteral(HIASM_BUILD_COMPILER)).absolutePath());
    auto env = QProcessEnvironment::systemEnvironment();
    env.insert("PATH", compilerBin + QDir::listSeparator() + prefix + "/bin" + QDir::listSeparator() + env.value("PATH"));
    const QString compiler = tool("CXX", {compilerBin + "/" + QFileInfo(QStringLiteral(HIASM_BUILD_COMPILER)).fileName()},
                                  "c++");
    const QString cmake = tool("HIASM_CMAKE", {compilerDir + "/tools/bin/cmake" + suffix, QStringLiteral(HIASM_BUILD_CMAKE)}, "cmake");
    const QString ninja = tool("HIASM_NINJA", {compilerDir + "/tools/bin/ninja" + suffix, QStringLiteral(HIASM_BUILD_NINJA)}, "ninja");
    if (cmake.isEmpty() || compiler.isEmpty() || ninja.isEmpty())
        fail(QStringLiteral("CMake, Ninja and a compatible C++ compiler are required."));
    const QString build = temp.filePath("build");
    const QString config = generated.release ? "Release" : "Debug";
    QStringList configure{"-S", packs + "/cpp/qt", "-B", build, "-G", "Ninja",
                    "-DCMAKE_PREFIX_PATH=" + prefix, "-DCMAKE_BUILD_TYPE=" + config,
                    "-DCMAKE_CXX_COMPILER=" + compiler, "-DCMAKE_MAKE_PROGRAM=" + ninja,
                    "-DHIASM_SOURCE=" + source,
                    QString("-DHIASM_LIBRARY=") + (sharedArtifact(generated.type) ? "ON" : "OFF"),
                    QString("-DHIASM_WORKER=") + (generated.type == ProjectType::Worker || generated.type == ProjectType::WindowsService ? "ON" : "OFF"),
                    QString("-DHIASM_COM=") + (generated.type == ProjectType::ComExtension ? "ON" : "OFF"),
                    "-DHIASM_SUFFIX=" + artifactSuffix(generated.type),
                    QString("-DHIASM_CORE=") + (generated.core ? "ON" : "OFF"),
                    QString("-DHIASM_CONSOLE=") + (generated.gui ? "OFF" : "ON")};
    if (web)
        configure << "-DHIASM_WEBENGINE=ON" << "-DHIASM_RUFFLE_DIR=" + ruffle;
#ifdef Q_OS_WIN
    if (web) configure << "-DHIASM_WEBVIEW2_CACHE=" + QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
#endif
    execute(cmake, configure, env);
    execute(cmake, {"--build", build, "--config", config}, env);
    const auto executable = build + "/program" + artifactSuffix(generated.type);
#ifdef Q_OS_WIN
    if (web) {
        const auto deploy = QDir(prefix).absoluteFilePath("bin/windeployqt.exe");
        const auto destination = QFileInfo(output).absoluteDir();
        execute(deploy, {"--no-translations", "--dir", destination.absolutePath(), executable}, env);
        deployFile(build + "/WebView2Loader.dll", destination.filePath("WebView2Loader.dll"));
        for (const auto &runtime : {"libc++.dll", "libunwind.dll"}) {
            const auto path = QDir(compilerBin).filePath(runtime);
            if (QFileInfo(path).isFile()) deployFile(path, destination.filePath(runtime));
        }
    }
#endif
    QFile binary(executable);
    if (!binary.open(QIODevice::ReadOnly)) fail(binary.errorString());
    QSaveFile result(output);
    if (!result.open(QIODevice::WriteOnly)) fail(result.errorString());
    while (!binary.atEnd()) {
        const auto chunk = binary.read(1024 * 1024);
        if (chunk.isEmpty() || result.write(chunk) != chunk.size()) fail(QStringLiteral("Failed to copy executable"));
    }
    if (!result.setPermissions(binary.permissions()) || !result.commit()) fail(result.errorString());
    QTextStream(stdout) << output << Qt::endl;
    return 0;
}
}

int buildSchemeCommand(const QStringList &args, bool generateOnly) {
    if (args.size() != 2) {
        QTextStream(stderr) << "Usage: hiasm --build|--generate <scheme.sha> <output>" << Qt::endl;
        return 2;
    }
    try {
        const auto input = QFileInfo(args[0]);
        const auto output = QFileInfo(args[1]);
        if (input == output) fail(QStringLiteral("The output must differ from the scheme"));
        if (isInLegacyInstallation(output.absoluteFilePath()))
            fail(QStringLiteral("Cannot write build output inside a HiAsm 4 installation"));
        return compile(input.absoluteFilePath(), output.absoluteFilePath(), generateOnly);
    } catch (const std::exception &e) {
        QTextStream(stderr) << QString::fromUtf8(e.what()) << Qt::endl;
        return 1;
    }
}
