#include "MainWindow.h"

#include <qtpyt/globalinit.h>
#include <qtpyt/qpythreadpool.h>

#include <QApplication>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName("Redisc");
    QApplication::setOrganizationName("Redisc");
    QIcon appIcon;
    appIcon.addFile(":/icons/icons/redisc32x32.png", QSize(32, 32));
    appIcon.addFile(":/icons/icons/redisc64x64.png", QSize(64, 64));
    appIcon.addFile(":/icons/icons/redisc.png");
    QApplication::setWindowIcon(appIcon);
    qtpyt::init();
    qtpyt::QPyThreadPool::initialize(1, false);

    MainWindow window;
    window.setWindowIcon(appIcon);
    window.show();

    return app.exec();
}
