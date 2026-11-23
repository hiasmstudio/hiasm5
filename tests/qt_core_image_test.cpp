#include "core_widgets.h"
#include <QBuffer>
#include <QImage>
#include <QLibrary>
#include <QApplication>
#include <QLabel>
#include <QToolButton>
#include <QTemporaryDir>
#include <QFile>
#include <cstdio>
#include <cstring>

int main(int argc, char **argv) {
    if (argc != 2 && argc != 3) return 2;
    QLibrary library(QString::fromLocal8Bit(argv[1]));
    auto getWidgets = reinterpret_cast<HiasmQtGetWidgets>(library.resolve("hiasm_qt_widgets"));
    if (!getWidgets) return 3;
    const auto *api = getWidgets(2);
    const HiasmQtHost host{
        [](void *element, const char *name) {
            if (std::strcmp(name, "Label") == 0) return "Details";
            if (std::strcmp(name, "State") == 0) return element ? "1" : "0";
            if (std::strcmp(name, "Width") == 0 || std::strcmp(name, "Height") == 0) return "160";
            return "";
        },
        [](void *, const char *, const char *, double, int) {}
    };
    if (!api || !api->start(&host, "")) return 4;
    const bool expander = argc == 3 && std::strcmp(argv[2], "expander") == 0;
    const bool sdkEdit = argc == 3 && std::strcmp(argv[2], "sdkedit") == 0;
    if (argc == 3 && !expander && !sdkEdit) return 2;
    const int status = [&] {
        if (expander) {
            int expandedState = 1;
            for (bool initiallyExpanded : {false, true}) {
                auto *container = api->create(initiallyExpanded ? &expandedState : nullptr, "HCWE_Expander", nullptr);
                if (!container || !api->create(nullptr, "HCWE_Label", container)) return 10;
                api->show(container);
                api->pump();
                QWidget *window = nullptr;
                for (auto *candidate : QApplication::topLevelWidgets())
                    if (candidate->objectName() == "HCWE_Expander" && candidate->isVisible()) window = candidate;
                if (!window) return 11;
                auto *toggle = window->findChild<QToolButton *>();
                auto *label = window->findChild<QLabel *>("HCWE_Label");
                if (!toggle || toggle->text() != "Details" || !label || label->isVisible() != initiallyExpanded) return 12;
                toggle->click();
                if (label->isVisible() == initiallyExpanded) return 13;
                toggle->click();
                if (label->isVisible() != initiallyExpanded) return 14;
                window->hide();
            }
            return 0;
        }
        if (sdkEdit) {
            QTemporaryDir directory;
            if (!directory.isValid()) return 15;
            const auto input = directory.filePath("input.sha");
            const auto output = directory.filePath("output.sha");
            QFile source(input);
            if (!source.open(QIODevice::WriteOnly) || source.write("Make(_base)\nElement=Value\n") <= 0) return 16;
            source.close();
            auto *editor = api->create(nullptr, "HCWE_SDKEdit", nullptr);
            if (!editor || !api->work(editor, "doLoad", input.toUtf8().constData(), 0)) return 17;
            const char *loaded = nullptr;
            double number = 0;
            if (api->read(editor, "Text", &loaded, &number) != 1 ||
                QByteArray(loaded) != QByteArrayLiteral("Make(_base)\nElement=Value\n")) return 18;
            if (!api->work(editor, "doSave", output.toUtf8().constData(), 0)) return 19;
            QFile saved(output);
            if (!saved.open(QIODevice::ReadOnly) || saved.readAll() != QByteArrayLiteral("Make(_base)\nElement=Value\n")) return 20;
            return 0;
        }
        QImage original(3, 2, QImage::Format_ARGB32);
        original.fill(qRgba(20, 80, 160, 200));
        QByteArray png;
        QBuffer buffer(&png);
        if (!buffer.open(QIODevice::WriteOnly) || !original.save(&buffer, "PNG")) return 5;
        auto *source = api->create(nullptr, "HCWE_Image", nullptr);
        auto *target = api->create(nullptr, "HCWE_Image", nullptr);
        if (!source || !target || !api->work(source, "doLoadFromImage", png.toBase64().constData(), 0)) return 6;
        const char *value = nullptr;
        double number = 0;
        if (api->read(source, "Picture", &value, &number) != 4 || !api->work(target, "doLoadFromImage", value, 0)) return 7;
        if (api->read(target, "Picture", &value, &number) != 4) return 8;
        const auto result = QImage::fromData(QByteArray::fromBase64(value), "PNG");
        if (result.size() != original.size() || result.pixel(1, 1) != original.pixel(1, 1)) return 9;
        return 0;
    }();
    api->stop();
    if (status) std::fprintf(stderr, "Core %s test failed at step %d\n", expander ? "expander" : "image", status);
    return status;
}
