#include "MainWindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Redisc");
    QApplication::setOrganizationName("Redisc");

    MainWindow window;
    window.show();

    return app.exec();
}
