#include "goalsdashboard.h"
#include <QApplication>

#include <QIcon>
#include <QDebug>


int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    qputenv("QTWEBENGINE_DISABLE_SANDBOX", "1");

    GoalsDashboard window;
    window.setWindowTitle("Goals Dashboard");
    window.resize(800,600);
    window.show();

    return a.exec();
}
