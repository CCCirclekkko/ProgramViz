#pragma once

#include "projectmodel.h"
#include "visualsettings.h"

#include <QMainWindow>

#include <memory>

class QLabel;
class QLineEdit;
class QPushButton;
class QComboBox;
class QSpinBox;
class TreeMapView;

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
    void openPaletteDialog();
    void applyAppearance();

private:
    void buildUi();
    void loadSettings();
    void saveSettings() const;
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
    QLabel *m_details = nullptr;
    QPushButton *m_finderButton = nullptr;
    QPushButton *m_vscodeButton = nullptr;
    QComboBox *m_themeCombo = nullptr;
    QComboBox *m_schemeCombo = nullptr;
    QSpinBox *m_fontSizeSpin = nullptr;
    QPushButton *m_paletteButton = nullptr;
    TreeMapView *m_view = nullptr;
    ProjectNode *m_selected = nullptr;
    VisualSettings m_visualSettings;
};
