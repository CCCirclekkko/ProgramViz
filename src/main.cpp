#include <QApplication>
#include <QDir>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ProgramViz"));
    QApplication::setOrganizationName(QStringLiteral("ProgramViz"));

    const QString initialPath = argc > 1 ? QString::fromLocal8Bit(argv[1]) : QDir::currentPath();
    MainWindow window(initialPath);
    window.show();

    return app.exec();
}
