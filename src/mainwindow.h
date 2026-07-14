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

class MainWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(const QString &initialPath, QWidget *parent = nullptr);

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
    QString formatBytes(qint64 bytes) const;
    QString formatDate(const QDateTime &dateTime) const;
    ProjectNode *selectedNode() const;

    QString m_projectPath;
    std::unique_ptr<ProjectNode> m_project;
    QLineEdit *m_pathEdit = nullptr;
    QPushButton *m_scanButton = nullptr;
    QToolButton *m_recentButton = nullptr;
    QMenu *m_recentMenu = nullptr;
    QLabel *m_details = nullptr;
    QPushButton *m_finderButton = nullptr;
    QPushButton *m_vscodeButton = nullptr;
    QPushButton *m_nameExpansionButton = nullptr;
    QPushButton *m_appearanceButton = nullptr;
    QPushButton *m_adaptiveButton = nullptr;
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
};
