#pragma once

#include "projectmodel.h"
#include "visualsettings.h"

#include <QMainWindow>
#include <QPointer>

#include <memory>

class QLabel;
class QLineEdit;
class QPushButton;
class QMenu;
class QToolButton;
class TreeMapView;
class AppearanceSettingsDialog;
class QTemporaryDir;

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const QString &initialPath, QWidget *parent = nullptr);
    ~MainWindow() override;

    bool exportGitEvolutionGifFile(const QString &outputPath, int fps = 10,
                                   double transitionDuration = 0.5,
                                   double pauseDuration = 0.5,
                                   const QString &resolution = QStringLiteral("current"),
                                   bool fitHeight = false,
                                   bool expandNames = false,
                                   bool showDate = true,
                                   int timeGranularity = 3);
    void openGifExportDialogForScreenshot();

private slots:
    void chooseProject();
    void rescanProject();
    void showNode(ProjectNode *node);
    void showHover(ProjectNode *node);
    void openSelectedInFinder();
    void openSelectedInVSCode();
    void openAppearanceSettings();
    void toggleNameExpansion();
    void toggleAdaptiveWindow();
    void handleAdaptiveFitAvailable();
    void handleAdaptiveFitUnavailable(int requiredHeight);
    void selectGitBranch(const QString &branch);
    void selectGitRevision(const QString &revision);
    void exportCurrentJpeg();
    void exportGitEvolutionGif();
    void applyAppearance();
    void updateRecentMenu();

private:
    void buildUi();
    void loadSettings();
    void saveSettings() const;
    void addRecentProject(const QString &path);
    bool effectiveDarkTheme() const;
    void updateDetails(ProjectNode *node);
    void scanPath(const QString &path);
    QString gitOutput(const QStringList &arguments) const;
    void refreshGitMetadata(const QString &path);
    void updateGitMenus();
    bool materializeGitRevision(const QString &revision, QString *snapshotPath);
    bool materializeGitRevisionTo(const QString &revision, const QString &snapshotPath) const;
    QSize resolveGifResolution(const QString &specification) const;
    QString formatExportDateTime(const QDateTime &dateTime, int granularity) const;
    QString formatBytes(qint64 bytes) const;
    QString formatDate(const QDateTime &dateTime) const;
    ProjectNode *selectedNode() const;

    QString m_projectPath;
    std::unique_ptr<ProjectNode> m_project;
    QLineEdit *m_pathEdit = nullptr;
    QPushButton *m_scanButton = nullptr;
    QToolButton *m_recentButton = nullptr;
    QMenu *m_recentMenu = nullptr;
    QPushButton *m_gitBranchButton = nullptr;
    QPushButton *m_gitHistoryButton = nullptr;
    QMenu *m_gitBranchMenu = nullptr;
    QMenu *m_gitHistoryMenu = nullptr;
    QMenu *m_exportMenu = nullptr;
    QLabel *m_details = nullptr;
    QPushButton *m_finderButton = nullptr;
    QPushButton *m_vscodeButton = nullptr;
    QPushButton *m_nameExpansionButton = nullptr;
    QPushButton *m_appearanceButton = nullptr;
    QPushButton *m_adaptiveButton = nullptr;
    QPushButton *m_exportButton = nullptr;
    TreeMapView *m_view = nullptr;
    ProjectNode *m_selected = nullptr;
    VisualSettings m_visualSettings;
    double m_userMinHeightRatio = 0.6;
    bool m_namesExpanded = false;
    bool m_adaptiveWindow = false;
    bool m_adaptiveRecoveryAllowed = false;
    bool m_adaptiveResizeAttempted = false;
    QStringList m_recentProjects;
    QPointer<AppearanceSettingsDialog> m_appearanceDialog;
    QString m_gitRoot;
    QString m_gitBranch;
    QString m_gitRevision;
    std::unique_ptr<QTemporaryDir> m_gitSnapshot;
    QString m_cliExportGifPath;
    int m_cliExportGifFps = 10;
    double m_cliExportGifTransitionDuration = 0.5;
    double m_cliExportGifPauseDuration = 0.5;
    QString m_cliExportGifResolution = QStringLiteral("current");
    bool m_cliExportGifFitHeight = false;
    bool m_cliExportGifExpandNames = false;
    bool m_cliExportGifShowDate = true;
    int m_cliExportTimeGranularity = 3;
};
