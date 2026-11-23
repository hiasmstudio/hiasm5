#include <QApplication>
#include <QCommandLineParser>
#include <QDockWidget>
#include <QMessageBox>
#include <QTabWidget>
#include <QTranslator>
#include <QListWidget>
#include "main_window.h"
#include "scheme_build.h"

int main(int argc, char **argv) {
    if (argc > 1 && (qstrcmp(argv[1], "--build") == 0 || qstrcmp(argv[1], "--generate") == 0)) {
        QCoreApplication app(argc, argv);
        const auto args = app.arguments();
        return buildSchemeCommand(args.mid(2), args.at(1) == "--generate");
    }
    QApplication app(argc, argv);
    QTranslator translator;
    if (translator.load(QStringLiteral(":/i18n/hiasm_ru.qm"))) app.installTranslator(&translator);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("HiAsm IDE\n"
        "  hiasm --build <scheme.sha> <program>\n  hiasm --generate <scheme.sha> <source.cpp>"));
    parser.addHelpOption();
    parser.addOption({QStringLiteral("check"), QStringLiteral("Check startup and exit.")});
    parser.addPositionalArgument(QStringLiteral("scheme"),
        QStringLiteral("HiAsm schemes to open."), QStringLiteral("[scheme...]"));
    parser.process(app);

    HiasmQtMainWindow window;
    const auto *paletteDock = window.findChild<QDockWidget *>(QStringLiteral("paletteDock"));
    const auto *palette = paletteDock ? paletteDock->findChild<QListWidget *>() : nullptr;
    const bool defaultCatalogLoaded = palette && palette->count() > 0;
    const auto paths = parser.positionalArguments();
    const bool check = parser.isSet(QStringLiteral("check"));
    QStringList failed;
    for (const auto &path : paths) {
        if (!window.openScheme(path))
            failed.append(path);
    }
    window.show();
    if (check) {
        app.processEvents();
        return defaultCatalogLoaded && failed.isEmpty() && window.editorCount() == paths.size() + 1 && window.dockCount() == 8
            && window.findChild<QTabWidget *>(QStringLiteral("tabs"))
            && window.findChild<QDockWidget *>(QStringLiteral("paletteDock"))
            && window.findChild<QDockWidget *>(QStringLiteral("propertyDock"))
            && window.findChild<QDockWidget *>(QStringLiteral("debugDock"))
            && window.findChild<QDockWidget *>(QStringLiteral("catalogDock"))
            && window.findChild<QDockWidget *>(QStringLiteral("repositoryDock"))
            && window.findChild<QDockWidget *>(QStringLiteral("buildSettingsDock"))
            && window.findChild<QDockWidget *>(QStringLiteral("errorsDock"))
            && window.findChild<QDockWidget *>(QStringLiteral("terminalDock")) ? 0 : 2;
    }
    if (!failed.isEmpty()) {
        QMessageBox message(QMessageBox::Warning,
            QCoreApplication::translate("MainWindow", "Open scheme"),
            QCoreApplication::translate("MainWindow", "Could not open the following schemes:\n%1")
                .arg(failed.join(u'\n')),
            QMessageBox::Ok, &window);
        message.setTextFormat(Qt::PlainText);
        message.exec();
    }
    return app.exec();
}
