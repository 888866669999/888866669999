#include <QApplication>
#include "ui/MainWindow.h"

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("OmniAssist");
    app.setOrganizationName("OmniAssist");

    MainWindow window;
    window.show();

    return app.exec();
}
