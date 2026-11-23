#ifdef HIASM_TOOLCHAIN_PROBE
#include <QApplication>
#include <QCheckBox>
#include <QTimer>

int main(int argc, char **argv) {
    QApplication app(argc, argv);
    QCheckBox box;
    box.show();
    QTimer::singleShot(0, &app, [&] { box.click(); app.exit(box.isChecked() ? 0 : 1); });
    return app.exec();
}
#else
#include <iostream>
int main() {
    std::cout << "LLVM-MinGW C++17\n";
    return std::cout ? 0 : 1;
}
#endif
