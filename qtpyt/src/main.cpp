#include "MainWindow.h"

#include <qtpyt/globalinit.h>
#include <qtpyt/qpythreadpool.h>

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Redisc");
    QApplication::setOrganizationName("Redisc");
    qtpyt::init();
    qtpyt::QPyThreadPool::initialize(1, false);

    MainWindow window;
    window.show();

    return app.exec();
}
