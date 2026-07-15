#include <QApplication>
#include <QDir>
#include <QScreen>
#include <QTimer>
#include "mainwindow.h"

#include <algorithm>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("ProgramViz"));
    QApplication::setOrganizationName(QStringLiteral("ProgramViz"));

    QString initialPath = QDir::currentPath();
    QString screenshotPath;
    QString gifDialogScreenshotPath;
    QString exportGifPath;
    int exportGifFps = 10;
    double exportGifTransition = 0.5;
    double exportGifPause = 0.5;
    QString exportGifResolution = QStringLiteral("current");
    bool exportGifFitHeight = false;
    bool exportGifExpandNames = false;
    bool exportGifShowDate = true;
    int exportGifTimeGranularity = 3;
    for (int index = 1; index < argc; ++index) {
        const QString argument = QString::fromLocal8Bit(argv[index]);
        if (argument == QStringLiteral("--screenshot") && index + 1 < argc) {
            screenshotPath = QString::fromLocal8Bit(argv[++index]);
        } else if (argument == QStringLiteral("--screenshot-gif-dialog") && index + 1 < argc) {
            gifDialogScreenshotPath = QString::fromLocal8Bit(argv[++index]);
        } else if (argument == QStringLiteral("--export-gif") && index + 1 < argc) {
            exportGifPath = QString::fromLocal8Bit(argv[++index]);
        } else if (argument == QStringLiteral("--gif-fps") && index + 1 < argc) {
            exportGifFps = QString::fromLocal8Bit(argv[++index]).toInt();
        } else if (argument == QStringLiteral("--transition-duration") && index + 1 < argc) {
            exportGifTransition = QString::fromLocal8Bit(argv[++index]).toDouble();
        } else if (argument == QStringLiteral("--pause-duration") && index + 1 < argc) {
            exportGifPause = QString::fromLocal8Bit(argv[++index]).toDouble();
        } else if (argument == QStringLiteral("--gif-resolution") && index + 1 < argc) {
            exportGifResolution = QString::fromLocal8Bit(argv[++index]);
        } else if (argument == QStringLiteral("--gif-fit-height")) {
            exportGifFitHeight = true;
        } else if (argument == QStringLiteral("--gif-no-fit-height")) {
            exportGifFitHeight = false;
        } else if (argument == QStringLiteral("--gif-expand-names")) {
            exportGifExpandNames = true;
        } else if (argument == QStringLiteral("--gif-collapse-names")) {
            exportGifExpandNames = false;
        } else if (argument == QStringLiteral("--gif-show-date")) {
            exportGifShowDate = true;
        } else if (argument == QStringLiteral("--gif-no-date")) {
            exportGifShowDate = false;
        } else if (argument == QStringLiteral("--gif-time-granularity") && index + 1 < argc) {
            const QString granularity = QString::fromLocal8Bit(argv[++index]).toLower();
            bool ok = false;
            const int numeric = granularity.toInt(&ok);
            if (ok) {
                exportGifTimeGranularity = numeric;
            } else {
                const QStringList names = {QStringLiteral("秒"), QStringLiteral("分"),
                                           QStringLiteral("时"), QStringLiteral("日"),
                                           QStringLiteral("月"), QStringLiteral("年"),
                                           QStringLiteral("second"), QStringLiteral("minute"),
                                           QStringLiteral("hour"), QStringLiteral("day"),
                                           QStringLiteral("month"), QStringLiteral("year")};
                const int nameIndex = names.indexOf(granularity);
                exportGifTimeGranularity = nameIndex >= 6 ? nameIndex - 6 : nameIndex;
            }
        } else if (argument == QStringLiteral("--version-duration") && index + 1 < argc) {
            const double total = QString::fromLocal8Bit(argv[++index]).toDouble();
            exportGifPause = std::max(0.0, total - exportGifTransition);
        } else if (!argument.startsWith(QStringLiteral("--"))) {
            initialPath = argument;
        }
    }
    MainWindow window(initialPath);
    window.show();

    if (!gifDialogScreenshotPath.isEmpty()) {
        QTimer::singleShot(250, &app,
                           [&window]() { window.openGifExportDialogForScreenshot(); });
        QTimer::singleShot(1000, &app, [&app, gifDialogScreenshotPath]() {
            if (QScreen *screen = QGuiApplication::primaryScreen())
                screen->grabWindow(0).save(gifDialogScreenshotPath);
            app.quit();
        });
    } else if (!exportGifPath.isEmpty()) {
        QTimer::singleShot(250, &app, [&app, &window, exportGifPath, exportGifFps,
                                       exportGifTransition, exportGifPause, exportGifResolution,
                                       exportGifFitHeight, exportGifExpandNames,
                                       exportGifShowDate, exportGifTimeGranularity]() {
            window.exportGitEvolutionGifFile(exportGifPath, exportGifFps,
                                             exportGifTransition, exportGifPause,
                                             exportGifResolution, exportGifFitHeight,
                                             exportGifExpandNames, exportGifShowDate,
                                             exportGifTimeGranularity);
            app.quit();
        });
    } else if (!screenshotPath.isEmpty()) {
        QTimer::singleShot(250, &app, [&app, &window, screenshotPath]() {
            window.grab().save(screenshotPath);
            app.quit();
        });
    }

    return app.exec();
}
