#include "mainwindow.h"

#include "appearancesettingsdialog.h"
#include "treemapview.h"

#include <QApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QProcess>
#include <QScrollArea>
#include <QSettings>
#include <QSplitter>
#include <QStatusBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <QDesktopServices>

#include <algorithm>
#include <utility>

namespace {

QString kindName(ProjectNodeKind kind)
{
    switch (kind) {
    case ProjectNodeKind::Project:
        return QStringLiteral("工程");
    case ProjectNodeKind::Folder:
        return QStringLiteral("文件夹");
    case ProjectNodeKind::File:
        return QStringLiteral("文件");
    }
    return {};
}

} // namespace

MainWindow::MainWindow(const QString &initialPath, QWidget *parent)
    : QMainWindow(parent)
    , m_projectPath(initialPath)
{
    setWindowTitle(QStringLiteral("ProgramViz"));
    resize(1280, 760);
    loadSettings();
    buildUi();
    scanPath(m_projectPath);
}

void MainWindow::buildUi()
{
    auto *toolbar = new QWidget(this);
    auto *toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(0, 0, 0, 0);
    toolbarLayout->setSpacing(8);

    auto *pathLabel = new QLabel(QStringLiteral("工程目录"), toolbar);
    m_pathEdit = new QLineEdit(toolbar);
    m_pathEdit->setPlaceholderText(QStringLiteral("选择工程根目录，默认扫描当前目录"));
    m_pathEdit->setText(m_projectPath);
    auto *browseButton = new QPushButton(QStringLiteral("选择"), toolbar);
    m_scanButton = new QPushButton(QStringLiteral("刷新"), toolbar);
    m_recentMenu = new QMenu(this);
    m_recentButton = new QToolButton(toolbar);
    m_recentButton->setArrowType(Qt::DownArrow);
    m_recentButton->setToolTip(QStringLiteral("选择最近扫描的工程目录"));
    m_recentButton->setMenu(m_recentMenu);
    m_recentButton->setPopupMode(QToolButton::InstantPopup);
    m_nameExpansionButton = new QPushButton(QStringLiteral("展开所有名称"), toolbar);
    m_nameExpansionButton->setToolTip(QStringLiteral("切换所有模块名称的最小显示高度"));
    m_appearanceButton = new QPushButton(QStringLiteral("外观"), toolbar);
    m_appearanceButton->setToolTip(QStringLiteral("打开外观设置"));
    toolbarLayout->addWidget(pathLabel);
    toolbarLayout->addWidget(m_pathEdit, 1);
    toolbarLayout->addWidget(m_recentButton);
    toolbarLayout->addWidget(browseButton);
    toolbarLayout->addWidget(m_scanButton);
    toolbarLayout->addSpacing(4);
    toolbarLayout->addWidget(m_nameExpansionButton);
    toolbarLayout->addWidget(m_appearanceButton);

    m_view = new TreeMapView(this);
    m_details = new QLabel(this);
    m_details->setWordWrap(true);
    m_details->setTextFormat(Qt::RichText);
    m_details->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    m_details->setMinimumWidth(290);
    m_details->setStyleSheet(QStringLiteral("QLabel { padding: 16px; color: #d8e2f0; }"));

    m_finderButton = new QPushButton(QStringLiteral("在 Finder 中打开"), this);
    m_vscodeButton = new QPushButton(QStringLiteral("使用 VS Code 打开"), this);
    m_finderButton->setEnabled(false);
    m_vscodeButton->setEnabled(false);
    auto *actionLayout = new QHBoxLayout;
    actionLayout->addWidget(m_finderButton);
    actionLayout->addWidget(m_vscodeButton);

    auto *detailsPanel = new QWidget(this);
    auto *detailsLayout = new QVBoxLayout(detailsPanel);
    detailsLayout->setContentsMargins(0, 0, 0, 0);
    detailsLayout->addWidget(m_details, 1);
    detailsLayout->addLayout(actionLayout);

    auto *viewScroll = new QScrollArea(this);
    viewScroll->setFrameShape(QFrame::NoFrame);
    viewScroll->setWidgetResizable(true);
    viewScroll->setWidget(m_view);

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(viewScroll);
    splitter->addWidget(detailsPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setSizes({980, 300});

    auto *central = new QWidget(this);
    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(10);
    layout->addWidget(toolbar);
    layout->addWidget(splitter, 1);
    setCentralWidget(central);

    connect(browseButton, &QPushButton::clicked, this, &MainWindow::chooseProject);
    connect(m_scanButton, &QPushButton::clicked, this, &MainWindow::rescanProject);
    connect(m_pathEdit, &QLineEdit::returnPressed, this, &MainWindow::rescanProject);
    connect(m_view, &TreeMapView::nodeSelected, this, &MainWindow::showNode);
    connect(m_view, &TreeMapView::nodeHovered, this, &MainWindow::showHover);
    connect(m_finderButton, &QPushButton::clicked, this, &MainWindow::openSelectedInFinder);
    connect(m_vscodeButton, &QPushButton::clicked, this, &MainWindow::openSelectedInVSCode);
    connect(m_nameExpansionButton, &QPushButton::clicked,
            this, &MainWindow::toggleNameExpansion);
    connect(m_appearanceButton, &QPushButton::clicked, this, &MainWindow::openAppearanceSettings);
    connect(m_recentMenu, &QMenu::aboutToShow, this, &MainWindow::updateRecentMenu);
    applyAppearance();
    updateRecentMenu();
}

void MainWindow::loadSettings()
{
    QSettings settings;
    m_visualSettings.theme = static_cast<ThemeMode>(
        settings.value(QStringLiteral("appearance/theme"), static_cast<int>(ThemeMode::System)).toInt());
    m_visualSettings.colorScheme = static_cast<ColorScheme>(
        settings.value(QStringLiteral("appearance/colorScheme"),
                       static_cast<int>(ColorScheme::DifferentHueByLevel)).toInt());
    m_visualSettings.fontSize = settings.value(QStringLiteral("appearance/fontSize"), 16).toInt();
    m_visualSettings.hue = settings.value(QStringLiteral("appearance/hue"), 212).toInt();
    m_visualSettings.saturation = settings.value(QStringLiteral("appearance/saturation"), 46).toInt();
    m_visualSettings.intensity = settings.value(QStringLiteral("appearance/intensity"), 84).toInt();
    m_visualSettings.minHeightRatio = settings.value(QStringLiteral("appearance/minHeightRatio"), 0.6).toDouble();
    m_visualSettings.cornerRatio = settings.value(QStringLiteral("appearance/cornerRatio"), 1.0 / 6.0).toDouble();
    m_visualSettings.columnGap = settings.value(QStringLiteral("appearance/columnGap"), 10).toInt();
    m_visualSettings.siblingGap = settings.value(QStringLiteral("appearance/siblingGap"), 6).toInt();
    m_visualSettings.livePreview = settings.value(QStringLiteral("appearance/livePreview"), true).toBool();
    m_visualSettings.fontSize = qBound(9, m_visualSettings.fontSize, 24);
    m_visualSettings.hue = (m_visualSettings.hue % 360 + 360) % 360;
    m_visualSettings.saturation = qBound(10, m_visualSettings.saturation, 100);
    m_visualSettings.intensity = qBound(35, m_visualSettings.intensity, 100);
    m_visualSettings.minHeightRatio = std::clamp(m_visualSettings.minHeightRatio, 0.01, 2.0);
    m_userMinHeightRatio = m_visualSettings.minHeightRatio;
    m_visualSettings.cornerRatio = std::clamp(m_visualSettings.cornerRatio, 0.05, 0.30);
    m_visualSettings.columnGap = qBound(0, m_visualSettings.columnGap, 30);
    m_visualSettings.siblingGap = qBound(0, m_visualSettings.siblingGap, 20);
    m_recentProjects = settings.value(QStringLiteral("recentProjects")).toStringList();
    m_recentProjects.erase(std::remove_if(m_recentProjects.begin(), m_recentProjects.end(),
                                          [](const QString &path) { return !QDir(path).exists(); }),
                           m_recentProjects.end());
}

void MainWindow::saveSettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("appearance/theme"), static_cast<int>(m_visualSettings.theme));
    settings.setValue(QStringLiteral("appearance/colorScheme"),
                      static_cast<int>(m_visualSettings.colorScheme));
    settings.setValue(QStringLiteral("appearance/fontSize"), m_visualSettings.fontSize);
    settings.setValue(QStringLiteral("appearance/hue"), m_visualSettings.hue);
    settings.setValue(QStringLiteral("appearance/saturation"), m_visualSettings.saturation);
    settings.setValue(QStringLiteral("appearance/intensity"), m_visualSettings.intensity);
    settings.setValue(QStringLiteral("appearance/minHeightRatio"), m_userMinHeightRatio);
    settings.setValue(QStringLiteral("appearance/cornerRatio"), m_visualSettings.cornerRatio);
    settings.setValue(QStringLiteral("appearance/columnGap"), m_visualSettings.columnGap);
    settings.setValue(QStringLiteral("appearance/siblingGap"), m_visualSettings.siblingGap);
    settings.setValue(QStringLiteral("appearance/livePreview"), m_visualSettings.livePreview);
}

bool MainWindow::effectiveDarkTheme() const
{
    if (m_visualSettings.theme == ThemeMode::Dark)
        return true;
    if (m_visualSettings.theme == ThemeMode::Light)
        return false;
    return QApplication::palette().color(QPalette::Window).lightness() < 128;
}

void MainWindow::applyAppearance()
{
    const bool dark = effectiveDarkTheme();
    const QString background = dark ? QStringLiteral("#0f1726") : QStringLiteral("#f3f6fb");
    const QString panel = dark ? QStringLiteral("#182235") : QStringLiteral("#ffffff");
    const QString border = dark ? QStringLiteral("#34445d") : QStringLiteral("#c8d3e2");
    const QString foreground = dark ? QStringLiteral("#edf4ff") : QStringLiteral("#18263c");
    const QString secondary = dark ? QStringLiteral("#b9c7da") : QStringLiteral("#5e6f86");
    const QString button = dark ? QStringLiteral("#243552") : QStringLiteral("#e6edf7");
    const QString hover = dark ? QStringLiteral("#2f4770") : QStringLiteral("#d5e3f5");
    setStyleSheet(QStringLiteral(
                      "QMainWindow, QWidget { background: %1; color: %2; font-size: %3pt; }"
                      "QLineEdit, QComboBox, QSpinBox { background: %4; border: 1px solid %5; "
                      "border-radius: 6px; padding: 6px 8px; color: %2; }"
                      "QPushButton { background: %6; border: 1px solid %5; border-radius: 6px; "
                      "padding: 7px 12px; color: %2; }"
                      "QPushButton:hover, QComboBox:hover, QSpinBox:hover { background: %7; }"
                      "QPushButton:disabled { color: %8; background: %4; }"
                      "QLabel { color: %8; }"
                      "QSplitter::handle { background: %5; }")
                      .arg(background, foreground)
                      .arg(m_visualSettings.fontSize)
                      .arg(panel, border, button, hover, secondary));
    m_view->setVisualSettings(m_visualSettings);
    m_details->setStyleSheet(QStringLiteral("QLabel { padding: 16px; color: %1; }").arg(foreground));
}

void MainWindow::openAppearanceSettings()
{
    if (m_appearanceDialog) {
        m_appearanceDialog->raise();
        m_appearanceDialog->activateWindow();
        return;
    }

    const VisualSettings original = m_visualSettings;
    const double originalUserMinHeightRatio = m_userMinHeightRatio;
    VisualSettings dialogSettings = m_visualSettings;
    dialogSettings.minHeightRatio = m_userMinHeightRatio;
    auto *dialog = new AppearanceSettingsDialog(dialogSettings, this);
    m_appearanceDialog = dialog;
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->setModal(false);
    dialog->setWindowModality(Qt::NonModal);
    connect(dialog, &AppearanceSettingsDialog::settingsChanged, this,
            [this](VisualSettings preview) {
                m_userMinHeightRatio = preview.minHeightRatio;
                preview.minHeightRatio = m_namesExpanded ? 1.5 : m_userMinHeightRatio;
                m_visualSettings = preview;
                applyAppearance();
            });
    connect(dialog, &QDialog::finished, this,
            [this, dialog, original, originalUserMinHeightRatio](int result) {
        if (result == QDialog::Accepted) {
            VisualSettings accepted = dialog->settings();
            m_userMinHeightRatio = accepted.minHeightRatio;
            accepted.minHeightRatio = m_namesExpanded ? 1.5 : m_userMinHeightRatio;
            m_visualSettings = accepted;
            saveSettings();
            applyAppearance();
        } else if (m_visualSettings.livePreview || dialog->livePreview()) {
            m_visualSettings = original;
            m_userMinHeightRatio = originalUserMinHeightRatio;
            applyAppearance();
        }
        m_appearanceDialog.clear();
    });
    dialog->show();
    dialog->raise();
    dialog->activateWindow();
}

void MainWindow::toggleNameExpansion()
{
    m_namesExpanded = !m_namesExpanded;
    m_visualSettings.minHeightRatio = m_namesExpanded ? 1.5 : m_userMinHeightRatio;
    m_nameExpansionButton->setText(m_namesExpanded ? QStringLiteral("取消展开名称")
                                                    : QStringLiteral("展开所有名称"));
    applyAppearance();
}

void MainWindow::chooseProject()
{
    const QString directory = QFileDialog::getExistingDirectory(
        this, QStringLiteral("选择工程目录"), m_projectPath);
    if (directory.isEmpty())
        return;
    m_pathEdit->setText(directory);
    scanPath(directory);
}

void MainWindow::rescanProject()
{
    scanPath(m_pathEdit->text().trimmed());
}

void MainWindow::scanPath(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        statusBar()->showMessage(QStringLiteral("目录不存在：%1").arg(path));
        return;
    }

    m_projectPath = info.absoluteFilePath();
    addRecentProject(m_projectPath);
    m_pathEdit->setText(m_projectPath);
    m_scanButton->setEnabled(false);
    statusBar()->showMessage(QStringLiteral("正在扫描 %1 …").arg(m_projectPath));
    QApplication::processEvents();

    m_project = ProjectScanner::scan(m_projectPath);
    m_scanButton->setEnabled(true);
    if (!m_project) {
        m_view->setRoot(nullptr);
        statusBar()->showMessage(QStringLiteral("扫描失败"));
        return;
    }

    m_view->setRoot(m_project.get());
    statusBar()->showMessage(QStringLiteral("扫描完成：%1 个文件，%2 行代码")
                                 .arg(m_project->fileCount())
                                 .arg(m_project->lines.code));
}

void MainWindow::addRecentProject(const QString &path)
{
    m_recentProjects.removeAll(path);
    m_recentProjects.prepend(path);
    while (m_recentProjects.size() > 12)
        m_recentProjects.removeLast();
    QSettings settings;
    settings.setValue(QStringLiteral("recentProjects"), m_recentProjects);
    updateRecentMenu();
}

void MainWindow::updateRecentMenu()
{
    if (!m_recentMenu)
        return;
    m_recentMenu->clear();
    if (m_recentProjects.isEmpty()) {
        QAction *emptyAction = m_recentMenu->addAction(QStringLiteral("暂无最近工程"));
        emptyAction->setEnabled(false);
        return;
    }
    for (const QString &path : std::as_const(m_recentProjects)) {
        const QFileInfo info(path);
        QAction *action = m_recentMenu->addAction(info.fileName().isEmpty() ? path : info.fileName());
        action->setToolTip(path);
        connect(action, &QAction::triggered, this, [this, path]() {
            m_pathEdit->setText(path);
            scanPath(path);
        });
    }
}

void MainWindow::showNode(ProjectNode *node)
{
    m_selected = node;
    updateDetails(node);
}

void MainWindow::showHover(ProjectNode *node)
{
    if (node)
        updateDetails(node);
    else
        updateDetails(m_selected);
}

void MainWindow::updateDetails(ProjectNode *node)
{
    const bool enabled = node != nullptr;
    m_finderButton->setEnabled(enabled);
    m_vscodeButton->setEnabled(enabled && node->kind == ProjectNodeKind::File);
    if (!node) {
        m_details->setText(QStringLiteral("<h2>未选择节点</h2><p>悬停或点击左侧节点查看详细信息。</p>"));
        return;
    }

    const QString path = node->path.toHtmlEscaped();
    const QString language = node->language.isEmpty() ? QStringLiteral("—") : node->language;
    m_details->setText(QStringLiteral(
        "<h2>%1</h2>"
        "<p style='color:#91a4bf'>%2</p>"
        "<table cellspacing='7'>"
        "<tr><td>类型</td><td><b>%3</b></td></tr>"
        "<tr><td>代码量</td><td><b>%4 行</b></td></tr>"
        "<tr><td>总行数</td><td>%5</td></tr>"
        "<tr><td>空行 / 注释</td><td>%6 / %7</td></tr>"
        "<tr><td>大小</td><td>%8</td></tr>"
        "<tr><td>语言</td><td>%9</td></tr>"
        "<tr><td>文件数</td><td>%10</td></tr>"
        "<tr><td>文件夹数</td><td>%11</td></tr>"
        "<tr><td>创建日期</td><td>%12</td></tr>"
        "<tr><td>上次修改</td><td>%13</td></tr>"
        "</table>")
                             .arg(node->name.toHtmlEscaped())
                             .arg(path)
                             .arg(kindName(node->kind))
                             .arg(node->lines.code)
                             .arg(node->lines.total)
                             .arg(node->lines.blank)
                             .arg(node->lines.comment)
                             .arg(formatBytes(node->sizeBytes))
                             .arg(language.toHtmlEscaped())
                             .arg(node->fileCount())
                             .arg(node->folderCount())
                             .arg(formatDate(node->createdAt))
                             .arg(formatDate(node->modifiedAt)));
}

void MainWindow::openSelectedInFinder()
{
    ProjectNode *node = selectedNode();
    if (!node)
        return;
#ifdef Q_OS_MACOS
    QStringList arguments;
    if (node->kind == ProjectNodeKind::File)
        arguments << QStringLiteral("-R");
    arguments << node->path;
    const bool started = QProcess::startDetached(QStringLiteral("open"), arguments);
#else
    const bool started = QDesktopServices::openUrl(QUrl::fromLocalFile(node->path));
#endif
    if (!started)
        statusBar()->showMessage(QStringLiteral("无法打开路径：%1").arg(node->path));
}

void MainWindow::openSelectedInVSCode()
{
    ProjectNode *node = selectedNode();
    if (!node || node->kind != ProjectNodeKind::File)
        return;
    if (!QProcess::startDetached(QStringLiteral("code"), {node->path}))
        statusBar()->showMessage(QStringLiteral("未找到 VS Code 命令 code，请确认已加入 PATH"));
}

QString MainWindow::formatBytes(qint64 bytes) const
{
    if (bytes < 1024)
        return QStringLiteral("%1 B").arg(bytes);
    if (bytes < 1024 * 1024)
        return QStringLiteral("%1 KB").arg(QString::number(bytes / 1024.0, 'f', 1));
    return QStringLiteral("%1 MB").arg(QString::number(bytes / (1024.0 * 1024.0), 'f', 1));
}

QString MainWindow::formatDate(const QDateTime &dateTime) const
{
    return dateTime.isValid() ? dateTime.toLocalTime().toString(QStringLiteral("yyyy-MM-dd HH:mm"))
                              : QStringLiteral("—");
}

ProjectNode *MainWindow::selectedNode() const
{
    return m_selected;
}
