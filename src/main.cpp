#include <QApplication>
#include <QDir>
#include <QTimer>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ProgramViz"));
    QApplication::setOrganizationName(QStringLiteral("ProgramViz"));

    QString initialPath = QDir::currentPath();
    QString screenshotPath;
    for (int index = 1; index < argc; ++index) {
        const QString argument = QString::fromLocal8Bit(argv[index]);
        if (argument == QStringLiteral("--screenshot") && index + 1 < argc) {
            screenshotPath = QString::fromLocal8Bit(argv[++index]);
        } else if (!argument.startsWith(QStringLiteral("--"))) {
            initialPath = argument;
        }
    }
    MainWindow window(initialPath);
    window.show();

    if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(250, &app, [&app, &window, screenshotPath]() {
            window.grab().save(screenshotPath);
            app.quit();
        });
    }

    return app.exec();
}
