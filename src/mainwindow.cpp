#include "mainwindow.h"

#include "appearancesettingsdialog.h"
#include "treemapview.h"

#include <QApplication>
#include <QClipboard>
#include <QCheckBox>
#include <QComboBox>
#include <QDate>
#include <QDateTime>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileDialog>
#include <QFormLayout>
#include <QGuiApplication>
#include <QFileInfo>
#include <QGroupBox>
#include <QIntValidator>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QProcess>
#include <QProgressDialog>
#include <QRegularExpression>
#include <QScreen>
#include <QScrollArea>
#include <QSettings>
#include <QSlider>
#include <QStandardPaths>
#include <QSplitter>
#include <QStatusBar>
#include <QTemporaryDir>
#include <QTimeZone>
#include <QToolButton>
#include <QDialog>
#include <QAbstractItemView>
#include <QUrl>
#include <QVBoxLayout>

#include <QDesktopServices>

#include <algorithm>
#include <limits>
#include <utility>

namespace {

class BelowComboBox final : public QComboBox {
public:
    using QComboBox::QComboBox;

protected:
    void showPopup() override
    {
        QComboBox::showPopup();
        if (QWidget *popup = view()->window()) {
            popup->setFixedWidth(width());
            popup->move(mapToGlobal(QPoint(0, height())));
        }
    }
};

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

bool isGitVisualizableFile(const QString &path)
{
    const QFileInfo info(path);
    const QString fileName = info.fileName();
    if (fileName == QStringLiteral("CMakeLists.txt") || fileName == QStringLiteral("Makefile")
        || fileName == QStringLiteral("GNUmakefile") || fileName == QStringLiteral("meson.build")) {
        return true;
    }
    static const QStringList extensions = {
        QStringLiteral("c"), QStringLiteral("h"), QStringLiteral("cc"), QStringLiteral("cpp"),
        QStringLiteral("cxx"), QStringLiteral("hh"), QStringLiteral("hpp"), QStringLiteral("hxx"),
        QStringLiteral("qml"), QStringLiteral("ui"), QStringLiteral("qrc"), QStringLiteral("py"),
        QStringLiteral("js"), QStringLiteral("jsx"), QStringLiteral("ts"), QStringLiteral("tsx"),
        QStringLiteral("java"), QStringLiteral("kt"), QStringLiteral("kts"), QStringLiteral("rs"),
        QStringLiteral("go"), QStringLiteral("swift"), QStringLiteral("cs"), QStringLiteral("rb"),
        QStringLiteral("php"), QStringLiteral("sh"), QStringLiteral("bash"), QStringLiteral("zsh"),
        QStringLiteral("cmake"), QStringLiteral("json"), QStringLiteral("xml"),
        QStringLiteral("md"), QStringLiteral("markdown"),
    };
    return extensions.contains(info.suffix(), Qt::CaseInsensitive);
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

MainWindow::~MainWindow() = default;

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
    m_exportMenu = new QMenu(this);
    m_exportMenu->addAction(QStringLiteral("导出 JPG"), this, &MainWindow::exportCurrentJpeg);
    m_exportMenu->addAction(QStringLiteral("导出演进 GIF"), this, &MainWindow::exportGitEvolutionGif);
    m_exportButton = new QPushButton(QStringLiteral("导出"), toolbar);
    m_exportButton->setMenu(m_exportMenu);
    m_recentMenu = new QMenu(this);
    m_recentButton = new QToolButton(toolbar);
    m_recentButton->setArrowType(Qt::DownArrow);
    m_recentButton->setToolTip(QStringLiteral("选择最近扫描的工程目录"));
    m_recentButton->setMenu(m_recentMenu);
    m_recentButton->setPopupMode(QToolButton::InstantPopup);
    m_gitBranchMenu = new QMenu(this);
    m_gitHistoryMenu = new QMenu(this);
    m_gitBranchButton = new QPushButton(QStringLiteral("Git 分支"), toolbar);
    m_gitBranchButton->setToolTip(QStringLiteral("选择 Git 分支"));
    m_gitBranchButton->setMenu(m_gitBranchMenu);
    m_gitHistoryButton = new QPushButton(QStringLiteral("Git 历史"), toolbar);
    m_gitHistoryButton->setToolTip(QStringLiteral("选择 Git 历史版本"));
    m_gitHistoryButton->setMenu(m_gitHistoryMenu);
    m_nameExpansionButton = new QPushButton(QStringLiteral("展开所有名称"), toolbar);
    m_nameExpansionButton->setToolTip(QStringLiteral("切换所有模块名称的最小显示高度"));
    m_adaptiveButton = new QPushButton(QStringLiteral("适应窗口"), toolbar);
    m_adaptiveButton->setToolTip(QStringLiteral("自动调整每行像素系数以适应窗口高度"));
    m_appearanceButton = new QPushButton(QStringLiteral("外观"), toolbar);
    m_appearanceButton->setToolTip(QStringLiteral("打开外观设置"));
    toolbarLayout->addWidget(pathLabel);
    toolbarLayout->addWidget(m_pathEdit, 1);
    toolbarLayout->addWidget(m_recentButton);
    toolbarLayout->addWidget(m_gitBranchButton);
    toolbarLayout->addWidget(m_gitHistoryButton);
    toolbarLayout->addWidget(browseButton);
    toolbarLayout->addWidget(m_scanButton);
    toolbarLayout->addWidget(m_exportButton);
    toolbarLayout->addSpacing(4);
    toolbarLayout->addWidget(m_nameExpansionButton);
    toolbarLayout->addWidget(m_adaptiveButton);
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
    connect(m_adaptiveButton, &QPushButton::clicked,
            this, &MainWindow::toggleAdaptiveWindow);
    connect(m_view, &TreeMapView::adaptiveFitUnavailable,
            this, &MainWindow::handleAdaptiveFitUnavailable);
    connect(m_view, &TreeMapView::adaptiveFitAvailable,
            this, &MainWindow::handleAdaptiveFitAvailable);
    connect(m_appearanceButton, &QPushButton::clicked, this, &MainWindow::openAppearanceSettings);
    connect(m_recentMenu, &QMenu::aboutToShow, this, &MainWindow::updateRecentMenu);
    connect(m_gitBranchMenu, &QMenu::aboutToShow, this, &MainWindow::updateGitMenus);
    connect(m_gitHistoryMenu, &QMenu::aboutToShow, this, &MainWindow::updateGitMenus);
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
    m_cliExportGifFps = settings.value(QStringLiteral("export/fps"), 10).toInt();
    m_cliExportGifTransitionDuration = settings.value(QStringLiteral("export/transitionDuration"), 0.5).toDouble();
    m_cliExportGifPauseDuration = settings.value(QStringLiteral("export/pauseDuration"), 0.5).toDouble();
    m_cliExportGifResolution = settings.value(QStringLiteral("export/resolution"),
                                               QStringLiteral("custom:自适应x最大高度")).toString();
    m_cliExportGifFitHeight = settings.value(QStringLiteral("export/fitHeight"), false).toBool();
    m_cliExportGifExpandNames = settings.value(QStringLiteral("export/expandNames"), false).toBool();
    m_cliExportGifShowDate = settings.value(QStringLiteral("export/showDate"), true).toBool();
    m_cliExportTimeGranularity = settings.value(QStringLiteral("export/timeGranularity"), 3).toInt();
    m_cliExportGifFps = std::clamp(m_cliExportGifFps, 1, 30);
    m_cliExportGifTransitionDuration = std::clamp(m_cliExportGifTransitionDuration, 0.0, 5.0);
    m_cliExportGifPauseDuration = std::clamp(m_cliExportGifPauseDuration, 0.0, 5.0);
    m_cliExportTimeGranularity = std::clamp(m_cliExportTimeGranularity, 0, 5);
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
    settings.setValue(QStringLiteral("export/fps"), m_cliExportGifFps);
    settings.setValue(QStringLiteral("export/transitionDuration"), m_cliExportGifTransitionDuration);
    settings.setValue(QStringLiteral("export/pauseDuration"), m_cliExportGifPauseDuration);
    settings.setValue(QStringLiteral("export/resolution"), m_cliExportGifResolution);
    settings.setValue(QStringLiteral("export/fitHeight"), m_cliExportGifFitHeight);
    settings.setValue(QStringLiteral("export/expandNames"), m_cliExportGifExpandNames);
    settings.setValue(QStringLiteral("export/showDate"), m_cliExportGifShowDate);
    settings.setValue(QStringLiteral("export/timeGranularity"), m_cliExportTimeGranularity);
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
                      "QLabel:disabled, QComboBox:disabled { color: %9; }"
                      "QSplitter::handle { background: %5; }")
                      .arg(background, foreground)
                      .arg(m_visualSettings.fontSize)
                      .arg(panel, border, button, hover, secondary,
                           dark ? QStringLiteral("#68768c") : QStringLiteral("#a8b3c2")));
    m_view->setVisualSettings(m_visualSettings);
    const QString activeButtonStyle = QStringLiteral(
        "QPushButton { background: #d9f0df; color: #356043; border-color: #a8cfb1; }");
    if (m_adaptiveButton) {
        m_adaptiveButton->setStyleSheet(m_adaptiveWindow ? activeButtonStyle : QString());
        m_adaptiveButton->setText(m_adaptiveWindow ? QStringLiteral("取消适应")
                                                   : QStringLiteral("适应窗口"));
    }
    if (m_nameExpansionButton)
        m_nameExpansionButton->setStyleSheet(m_namesExpanded ? activeButtonStyle : QString());
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
    m_adaptiveRecoveryAllowed = false;
    m_adaptiveResizeAttempted = false;
    m_visualSettings.minHeightRatio = m_namesExpanded ? 1.5 : m_userMinHeightRatio;
    m_nameExpansionButton->setText(m_namesExpanded ? QStringLiteral("取消展开名称")
                                                    : QStringLiteral("展开所有名称"));
    applyAppearance();
}

void MainWindow::toggleAdaptiveWindow()
{
    m_adaptiveWindow = !m_adaptiveWindow;
    m_adaptiveRecoveryAllowed = m_adaptiveWindow;
    m_adaptiveResizeAttempted = false;
    if (!m_adaptiveWindow)
        statusBar()->clearMessage();
    m_view->setAdaptiveWindow(m_adaptiveWindow);
    applyAppearance();
}

void MainWindow::handleAdaptiveFitUnavailable(int requiredHeight)
{
    if (!m_adaptiveWindow || m_adaptiveResizeAttempted)
        return;

    m_adaptiveResizeAttempted = true;
    const int viewportHeight = m_view->parentWidget() ? m_view->parentWidget()->height()
                                                       : m_view->height();
    const int heightDelta = std::max(0, requiredHeight - viewportHeight);
    const QScreen *screen = QGuiApplication::screenAt(frameGeometry().center());
    const int screenHeight = screen ? screen->availableGeometry().height() : height() + heightDelta;
    const int desiredHeight = std::min(screenHeight, height() + heightDelta + 24);
    if (desiredHeight > height()) {
        resize(width(), desiredHeight);
        return;
    }

    if (m_adaptiveRecoveryAllowed && m_namesExpanded) {
        m_namesExpanded = false;
        m_visualSettings.minHeightRatio = m_userMinHeightRatio;
        m_nameExpansionButton->setText(QStringLiteral("展开所有名称"));
        m_adaptiveRecoveryAllowed = false;
        m_adaptiveResizeAttempted = false;
        applyAppearance();
        return;
    }

    statusBar()->setStyleSheet(QStringLiteral("QStatusBar { color: #9a6700; }"));
    statusBar()->showMessage(QStringLiteral("模块过多，无法完全自适应窗口，可滚动查看"));
}

void MainWindow::handleAdaptiveFitAvailable()
{
    m_adaptiveResizeAttempted = false;
    if (m_adaptiveWindow)
        statusBar()->setStyleSheet(QString());
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

void MainWindow::exportCurrentJpeg()
{
    if (!m_project || !m_view)
        return;

    const QString defaultName = QDir(m_projectPath).filePath(
        QFileInfo(m_projectPath).fileName() + QStringLiteral(".jpg"));
    const QString outputPath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出工程结构 JPG"), defaultName,
        QStringLiteral("JPEG 图片 (*.jpg *.jpeg)"));
    if (outputPath.isEmpty())
        return;

    m_view->setOverlayDate(m_cliExportGifShowDate
                               ? formatExportDateTime(QDateTime::currentDateTime(),
                                                      m_cliExportTimeGranularity)
                               : QString());
    const bool saved = m_view->grab().save(outputPath, "JPG", 95);
    m_view->setOverlayDate({});
    statusBar()->showMessage(saved ? QStringLiteral("JPG 已导出：%1").arg(outputPath)
                                   : QStringLiteral("JPG 导出失败：%1").arg(outputPath));
}

void MainWindow::exportGitEvolutionGif()
{
    if (m_gitRoot.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("当前工程不是 Git 工程，无法生成演进 GIF"));
        return;
    }

    const bool automated = !m_cliExportGifPath.isEmpty();
    QString outputPath = m_cliExportGifPath;
    int fps = m_cliExportGifFps;
    double transitionDuration = m_cliExportGifTransitionDuration;
    double pauseDuration = m_cliExportGifPauseDuration;
    QString resolutionSpec = m_cliExportGifResolution;
    bool fitHeight = m_cliExportGifFitHeight;
    bool expandNames = m_cliExportGifExpandNames;
    bool showDate = m_cliExportGifShowDate;
    int timeGranularity = m_cliExportTimeGranularity;
    if (!automated) {
        QDialog options(this);
        options.setWindowTitle(QStringLiteral("项目历史Gif导出"));
        auto *fpsCombo = new QComboBox(&options);
        for (const int optionFps : {1, 2, 3, 4, 5, 10, 20, 30})
            fpsCombo->addItem(QStringLiteral("%1 FPS").arg(optionFps), optionFps);
        fpsCombo->setCurrentIndex(fpsCombo->findData(m_cliExportGifFps));

        auto *advancedBox = new QGroupBox(QStringLiteral("导出设置"), &options);
        auto *advancedLayout = new QVBoxLayout(advancedBox);
        auto *form = new QFormLayout;
        advancedLayout->addLayout(form);
        auto *resolutionCombo = new QComboBox(advancedBox);
        resolutionCombo->addItem(QStringLiteral("当前窗口"), QStringLiteral("current"));
        resolutionCombo->addItem(QStringLiteral("全屏"), QStringLiteral("fullscreen"));
        resolutionCombo->addItem(QStringLiteral("自定义"), QStringLiteral("custom"));
        resolutionCombo->setMinimumWidth(
            QFontMetrics(resolutionCombo->font()).horizontalAdvance(QStringLiteral("当前窗口"))
            + 32);
        resolutionCombo->setEditable(false);

        auto *customResolution = new QWidget(advancedBox);
        auto *customResolutionLayout = new QHBoxLayout(customResolution);
        customResolutionLayout->setContentsMargins(0, 0, 0, 0);
        auto *customWidth = new QComboBox(customResolution);
        auto *customHeight = new QComboBox(customResolution);
        customWidth->setEditable(true);
        customHeight->setEditable(true);
        customWidth->lineEdit()->setValidator(new QIntValidator(1, std::numeric_limits<int>::max(),
                                                                 customWidth));
        customHeight->lineEdit()->setValidator(new QIntValidator(1, std::numeric_limits<int>::max(),
                                                                  customHeight));
        customWidth->addItem(QStringLiteral("自适应"));
        customWidth->addItem(QStringLiteral("当前宽度"));
        customWidth->addItem(QStringLiteral("最大宽度"));
        customHeight->addItem(QStringLiteral("当前高度"));
        customHeight->addItem(QStringLiteral("最大高度"));
        customWidth->setMinimumWidth(
            QFontMetrics(customWidth->font()).horizontalAdvance(QStringLiteral("当前宽度"))
            + 28);
        customHeight->setMinimumWidth(
            QFontMetrics(customHeight->font()).horizontalAdvance(QStringLiteral("当前高度"))
            + 28);
        customWidth->setFixedWidth(customWidth->minimumWidth());
        customHeight->setFixedWidth(customHeight->minimumWidth());
        customWidth->setEditText(QStringLiteral("自适应"));
        customHeight->setEditText(QStringLiteral("最大高度"));
        const QString savedResolution = m_cliExportGifResolution;
        if (savedResolution == QStringLiteral("current")) {
            resolutionCombo->setCurrentIndex(0);
        } else if (savedResolution == QStringLiteral("fullscreen")) {
            resolutionCombo->setCurrentIndex(1);
        } else {
            resolutionCombo->setCurrentIndex(2);
            const QString customSpec = savedResolution.startsWith(QStringLiteral("custom:"))
                                           ? savedResolution.mid(7)
                                           : savedResolution;
            const QStringList dimensions = customSpec.split(QChar('x'));
            if (dimensions.size() == 2) {
                customWidth->setEditText(dimensions.at(0));
                customHeight->setEditText(dimensions.at(1));
            }
        }
        customResolutionLayout->addWidget(customWidth);
        customResolutionLayout->addWidget(new QLabel(QStringLiteral("×"), customResolution));
        customResolutionLayout->addWidget(customHeight);
        customResolution->setVisible(false);
        connect(resolutionCombo, &QComboBox::currentIndexChanged, &options,
                [resolutionCombo, customResolution](int) {
                    customResolution->setVisible(
                        resolutionCombo->currentData().toString() == QStringLiteral("custom"));
                });
        customResolution->setVisible(
            resolutionCombo->currentData().toString() == QStringLiteral("custom"));
        auto *fitHeightCheck = new QCheckBox(QStringLiteral("自适应窗口高度"), advancedBox);
        auto *expandNamesCheck = new QCheckBox(QStringLiteral("展开所有内容名称"), advancedBox);
        auto *showDateCheck = new QCheckBox(QStringLiteral("显示时间"), advancedBox);
        auto *timeGranularityLabel = new QLabel(QStringLiteral("时间颗粒度"), advancedBox);
        auto *timeGranularityCombo = new BelowComboBox(advancedBox);
        timeGranularityCombo->addItem(QStringLiteral("秒"), 0);
        timeGranularityCombo->addItem(QStringLiteral("分"), 1);
        timeGranularityCombo->addItem(QStringLiteral("时"), 2);
        timeGranularityCombo->addItem(QStringLiteral("日"), 3);
        timeGranularityCombo->addItem(QStringLiteral("月"), 4);
        timeGranularityCombo->addItem(QStringLiteral("年"), 5);
        timeGranularityCombo->setCurrentIndex(timeGranularityCombo->findData(m_cliExportTimeGranularity));
        timeGranularityCombo->setFixedWidth(customWidth->width());
        timeGranularityCombo->setMaxVisibleItems(6);
        // Keep the selected-item marker and the combo arrow on the same side
        // as the popup options, so the option text stays aligned.
        timeGranularityCombo->setLayoutDirection(Qt::RightToLeft);
        auto *timeExample = new QLabel(advancedBox);
        const QStringList timeExamples = {
            QStringLiteral("2026-10-24, 08:03:12"), QStringLiteral("2026-10-24, 08:03"),
            QStringLiteral("2026-10-24, 08"), QStringLiteral("2026-10-24"),
            QStringLiteral("2026-10"), QStringLiteral("2026")};
        const auto updateTimeControls = [=]() {
            const int index = timeGranularityCombo->currentData().toInt();
            timeExample->setText(timeExamples.value(index));
            const bool enabled = showDateCheck->isChecked();
            timeGranularityLabel->setEnabled(enabled);
            timeGranularityCombo->setEnabled(enabled);
            timeExample->setEnabled(enabled);
        };
        connect(showDateCheck, &QCheckBox::toggled, &options, updateTimeControls);
        connect(timeGranularityCombo, &QComboBox::currentIndexChanged, &options,
                updateTimeControls);
        updateTimeControls();
        fitHeightCheck->setChecked(m_cliExportGifFitHeight);
        expandNamesCheck->setChecked(m_cliExportGifExpandNames);
        showDateCheck->setChecked(m_cliExportGifShowDate);
        auto *transitionSlider = new QSlider(Qt::Horizontal, advancedBox);
        auto *pauseSlider = new QSlider(Qt::Horizontal, advancedBox);
        transitionSlider->setRange(0, 50);
        pauseSlider->setRange(0, 50);
        transitionSlider->setValue(qRound(m_cliExportGifTransitionDuration * 10.0));
        pauseSlider->setValue(qRound(m_cliExportGifPauseDuration * 10.0));
        transitionSlider->setTickInterval(5);
        pauseSlider->setTickInterval(5);
        auto *transitionValue = new QLabel(advancedBox);
        auto *pauseValue = new QLabel(advancedBox);
        auto *totalValue = new QLabel(advancedBox);
        const auto updateDurationLabels = [=]() {
            const double transition = transitionSlider->value() / 10.0;
            const double pause = pauseSlider->value() / 10.0;
            transitionValue->setText(QStringLiteral("%1 秒").arg(transition, 0, 'f', 1));
            pauseValue->setText(QStringLiteral("%1 秒").arg(pause, 0, 'f', 1));
            totalValue->setText(QStringLiteral("%1 秒 * 版本")
                                    .arg(transition + pause, 0, 'f', 1));
        };
        connect(transitionSlider, &QSlider::valueChanged, &options, updateDurationLabels);
        connect(pauseSlider, &QSlider::valueChanged, &options, updateDurationLabels);
        updateDurationLabels();
        auto *transitionRow = new QWidget(advancedBox);
        auto *transitionLayout = new QHBoxLayout(transitionRow);
        transitionLayout->setContentsMargins(0, 0, 0, 0);
        transitionLayout->addWidget(transitionSlider);
        transitionLayout->addWidget(transitionValue);
        auto *pauseRow = new QWidget(advancedBox);
        auto *pauseLayout = new QHBoxLayout(pauseRow);
        pauseLayout->setContentsMargins(0, 0, 0, 0);
        pauseLayout->addWidget(pauseSlider);
        pauseLayout->addWidget(pauseValue);
        form->addRow(QStringLiteral("帧率"), fpsCombo);
        auto *resolutionRow = new QWidget(advancedBox);
        auto *resolutionRowLayout = new QHBoxLayout(resolutionRow);
        resolutionRowLayout->setContentsMargins(0, 0, 0, 0);
        resolutionRowLayout->addWidget(resolutionCombo);
        resolutionRowLayout->addWidget(customResolution, 1);
        form->addRow(QStringLiteral("窗口分辨率"), resolutionRow);
        form->addRow(QStringLiteral("过渡动画"), transitionRow);
        form->addRow(QStringLiteral("版本停顿"), pauseRow);
        form->addRow(QStringLiteral("总时长"), totalValue);
        auto *timeRow = new QWidget(advancedBox);
        auto *timeLayout = new QHBoxLayout(timeRow);
        timeLayout->setContentsMargins(0, 0, 0, 0);
        timeLayout->addWidget(showDateCheck);
        timeLayout->addWidget(timeGranularityLabel);
        timeLayout->addWidget(timeGranularityCombo);
        timeLayout->addWidget(timeExample);
        timeLayout->addStretch(1);
        auto *checksLayout = new QVBoxLayout;
        checksLayout->setContentsMargins(0, 0, 0, 0);
        const auto checkRow = [advancedBox](QCheckBox *check) {
            auto *row = new QWidget(advancedBox);
            auto *layout = new QHBoxLayout(row);
            layout->setContentsMargins(0, 0, 0, 0);
            layout->addWidget(check);
            layout->addStretch(1);
            return row;
        };
        checksLayout->addWidget(checkRow(fitHeightCheck), 0, Qt::AlignLeft);
        checksLayout->addWidget(checkRow(expandNamesCheck), 0, Qt::AlignLeft);
        checksLayout->addWidget(timeRow, 0, Qt::AlignLeft);
        advancedLayout->addLayout(checksLayout);

        auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, &options);
        buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
        buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
        connect(buttons, &QDialogButtonBox::accepted, &options, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, &options, &QDialog::reject);
        auto *dialogLayout = new QVBoxLayout(&options);
        dialogLayout->addWidget(advancedBox);
        dialogLayout->addWidget(buttons);
        if (options.exec() != QDialog::Accepted)
            return;

    const QString defaultName = QDir(m_projectPath).filePath(
        QFileInfo(m_projectPath).fileName() + QStringLiteral("-evolution.gif"));
    outputPath = QFileDialog::getSaveFileName(
        this, QStringLiteral("导出 Git 项目演进 GIF"), defaultName,
        QStringLiteral("GIF 动图 (*.gif)"));
    if (outputPath.isEmpty())
        return;

        fps = fpsCombo->currentData().toInt();
        transitionDuration = transitionSlider->value() / 10.0;
        pauseDuration = pauseSlider->value() / 10.0;
        if (resolutionCombo->currentData().toString() == QStringLiteral("custom")) {
            const QScreen *screen = QGuiApplication::screenAt(frameGeometry().center());
            if (!screen)
                screen = QGuiApplication::primaryScreen();
            const QRect available = screen ? screen->availableGeometry() : QRect();
            const auto dimensionText = [this, &available](const QString &text) {
                if (text == QStringLiteral("自适应"))
                    return QStringLiteral("自适应");
                if (text == QStringLiteral("当前宽度"))
                    return QString::number(m_view->width());
                if (text == QStringLiteral("当前高度"))
                    return QString::number(m_view->height());
                if (text == QStringLiteral("最大宽度"))
                    return QString::number(available.width());
                if (text == QStringLiteral("最大高度"))
                    return QString::number(available.height());
                return text;
            };
            resolutionSpec = QStringLiteral("custom:%1x%2")
                                 .arg(dimensionText(customWidth->currentText().trimmed()),
                                      dimensionText(customHeight->currentText().trimmed()));
        } else {
            resolutionSpec = resolutionCombo->currentData().toString();
        }
        fitHeight = fitHeightCheck->isChecked();
        expandNames = expandNamesCheck->isChecked();
        showDate = showDateCheck->isChecked();
        timeGranularity = timeGranularityCombo->currentData().toInt();
        m_cliExportGifFps = fps;
        m_cliExportGifTransitionDuration = transitionDuration;
        m_cliExportGifPauseDuration = pauseDuration;
        m_cliExportGifResolution = resolutionSpec;
        m_cliExportGifFitHeight = fitHeight;
        m_cliExportGifExpandNames = expandNames;
        m_cliExportGifShowDate = showDate;
        m_cliExportTimeGranularity = timeGranularity;
        saveSettings();
    }
    if (outputPath.isEmpty())
        return;
    const QString historyRef = m_gitBranch.isEmpty() ? QStringLiteral("HEAD") : m_gitBranch;
    const QStringList revisions = gitOutput({QStringLiteral("rev-list"),
                                              QStringLiteral("--reverse"), historyRef})
                                      .split(QChar('\n'), Qt::SkipEmptyParts);
    if (revisions.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("没有可用的 Git 历史版本"));
        return;
    }
    QVector<QDateTime> revisionDates;
    revisionDates.reserve(revisions.size());
    for (const QString &revision : revisions) {
        const QString dateText = gitOutput({QStringLiteral("show"), QStringLiteral("-s"),
                                            QStringLiteral("--format=%ad"),
                                            QStringLiteral("--date=format-local:%Y-%m-%d %H:%M:%S"),
                                            revision});
        QDateTime date = QDateTime::fromString(dateText,
                                               QStringLiteral("yyyy-MM-dd HH:mm:ss"));
        revisionDates.push_back(date);
    }

    QTemporaryDir snapshots(QStringLiteral("ProgramViz-history-XXXXXX"));
    QTemporaryDir frames(QStringLiteral("ProgramViz-frames-XXXXXX"));
    if (!snapshots.isValid() || !frames.isValid()) {
        statusBar()->showMessage(QStringLiteral("无法创建 GIF 临时目录"));
        return;
    }

    std::vector<std::unique_ptr<ProjectNode>> roots;
    roots.reserve(revisions.size());
    constexpr int stageCount = 3;
    QProgressDialog progress(this);
    progress.setWindowTitle(QStringLiteral("导出 Git 项目演进 GIF"));
    progress.setCancelButtonText(QStringLiteral("取消"));
    progress.setWindowModality(Qt::ApplicationModal);
    progress.setMinimumDuration(0);
    const auto updateStage = [&progress](int stage, const QString &name, int value, int maximum) {
        progress.setRange(0, std::max(1, maximum));
        progress.setValue(value);
        progress.setLabelText(QStringLiteral("%1/%2 %3  %4/%5")
                                  .arg(stage)
                                  .arg(stageCount)
                                  .arg(name)
                                  .arg(std::min(value, maximum))
                                  .arg(maximum));
        QApplication::processEvents();
    };
    for (int index = 0; index < revisions.size(); ++index) {
        updateStage(1, QStringLiteral("读取 Git 历史记录"), index, revisions.size());
        if (progress.wasCanceled())
            return;
        const QString snapshotPath = QDir(snapshots.path()).filePath(QStringLiteral("r%1").arg(index));
        if (!materializeGitRevisionTo(revisions.at(index), snapshotPath)) {
            statusBar()->showMessage(QStringLiteral("无法读取 Git 历史，GIF 生成已取消"));
            return;
        }
        auto root = ProjectScanner::scan(snapshotPath);
        if (!root) {
            statusBar()->showMessage(QStringLiteral("无法扫描 Git 历史快照，GIF 生成已取消"));
            return;
        }
        root->name = QFileInfo(m_projectPath).fileName();
        roots.push_back(std::move(root));
    }
    updateStage(1, QStringLiteral("读取 Git 历史记录"), revisions.size(), revisions.size());

    const QSize targetSize = resolveGifResolution(resolutionSpec);
    if (!targetSize.isValid()) {
        statusBar()->showMessage(QStringLiteral("无法识别 GIF 分辨率：%1").arg(resolutionSpec));
        return;
    }
    VisualSettings exportSettings = m_visualSettings;
    exportSettings.minHeightRatio = expandNames ? 1.5 : m_userMinHeightRatio;
    QVector<ProjectNode *> exportRoots;
    exportRoots.reserve(static_cast<int>(roots.size()));
    for (const auto &root : roots)
        exportRoots.push_back(root.get());
    // Width adaptation is always enabled. The requested width is a minimum;
    // the widest historical layout is used when it needs more room.
    const int adaptiveWidth = static_cast<int>(std::ceil(
        m_view->contentWidthForRoots(exportRoots, targetSize, exportSettings)));
    const QSize renderSize(std::max(targetSize.width(), adaptiveWidth), targetSize.height());
    const double fixedScale = fitHeight
                                  ? 0.0
                                  : m_view->lineHeightScaleForRoots(exportRoots, renderSize,
                                                                    exportSettings);
    const double versionDuration = transitionDuration + pauseDuration;
    const int framesPerVersion = std::max(1, qRound(versionDuration * fps));
    const int transitionFrames = fps == 1
                                     ? 0
                                     : qRound(transitionDuration * fps);
    const int totalFrames = framesPerVersion * static_cast<int>(roots.size());
    const auto dateForFrame = [this, &revisionDates, showDate, timeGranularity](int version,
                                                                                  double progress) {
        if (!showDate)
            return QString();
        if (revisionDates.isEmpty())
            return QString();
        const QDateTime toDate = revisionDates.at(
            std::min(version, static_cast<int>(revisionDates.size()) - 1));
        if (version <= 0 || progress >= 1.0 || !toDate.isValid())
            return toDate.isValid() ? formatExportDateTime(toDate, timeGranularity) : QString();
        const QDateTime fromDate = revisionDates.at(version - 1);
        if (!fromDate.isValid())
            return formatExportDateTime(toDate, timeGranularity);
        const qint64 fromMs = fromDate.toMSecsSinceEpoch();
        const qint64 toMs = toDate.toMSecsSinceEpoch();
        const qint64 interpolatedMs = fromMs + qRound64((toMs - fromMs) * progress);
        return formatExportDateTime(
            QDateTime::fromMSecsSinceEpoch(interpolatedMs, QTimeZone::systemTimeZone()),
            timeGranularity);
    };
    int frameIndex = 0;
    for (int version = 0; version < roots.size(); ++version) {
        for (int frame = 0; frame < framesPerVersion; ++frame) {
            updateStage(2, QStringLiteral("生成动画帧"), frameIndex, totalFrames);
            if (progress.wasCanceled())
                return;

            QImage image;
            if (version == 0 || transitionFrames == 0 || frame >= transitionFrames) {
                image = m_view->renderTransition(roots.at(version).get(),
                                                  roots.at(version).get(), 1.0, renderSize,
                                                  exportSettings, fitHeight, fixedScale,
                                                  dateForFrame(version, 1.0));
            } else {
                const double transitionProgress = (frame + 1.0) / transitionFrames;
                image = m_view->renderTransition(roots.at(version - 1).get(),
                                                  roots.at(version).get(),
                                                  transitionProgress, renderSize,
                                                  exportSettings, fitHeight, fixedScale,
                                                  dateForFrame(version, transitionProgress));
            }
            const QString framePath = QDir(frames.path()).filePath(
                QStringLiteral("frame_%1.png").arg(frameIndex, 6, 10, QChar('0')));
            if (!image.save(framePath, "PNG")) {
                statusBar()->showMessage(QStringLiteral("动画帧写入失败，GIF 生成已取消"));
                return;
            }
            ++frameIndex;
        }
    }
    updateStage(2, QStringLiteral("生成动画帧"), totalFrames, totalFrames);

    QString ffmpeg = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    if (ffmpeg.isEmpty()) {
        for (const QString &candidate : {QStringLiteral("/opt/homebrew/bin/ffmpeg"),
                                         QStringLiteral("/usr/local/bin/ffmpeg")}) {
            if (QFileInfo::exists(candidate)) {
                ffmpeg = candidate;
                break;
            }
        }
    }
    if (ffmpeg.isEmpty()) {
        statusBar()->showMessage(QStringLiteral("未找到 ffmpeg，无法生成 GIF"));
        return;
    }

    const QString framePattern = QDir(frames.path()).filePath(QStringLiteral("frame_%06d.png"));
    QProcess encoder;
    progress.setRange(0, 1);
    progress.setValue(0);
    progress.setLabelText(QStringLiteral("3/%1 编码 GIF  0/1").arg(stageCount));
    QApplication::processEvents();
    if (progress.wasCanceled())
        return;
    encoder.start(ffmpeg, {QStringLiteral("-y"), QStringLiteral("-framerate"), QString::number(fps),
                           QStringLiteral("-i"), framePattern, QStringLiteral("-vf"),
                           QStringLiteral("split[s0][s1];[s0]palettegen=stats_mode=diff[p];[s1][p]paletteuse=dither=sierra2_4a"),
                           QStringLiteral("-loop"), QStringLiteral("0"), outputPath});
    if (!encoder.waitForFinished(120000) || encoder.exitStatus() != QProcess::NormalExit
        || encoder.exitCode() != 0) {
        statusBar()->showMessage(QStringLiteral("GIF 编码失败：%1")
                                     .arg(QString::fromLocal8Bit(encoder.readAllStandardError()).trimmed()));
        return;
    }
    progress.setValue(1);
    progress.setLabelText(QStringLiteral("3/%1 编码 GIF  1/1").arg(stageCount));
    statusBar()->showMessage(QStringLiteral("Git 演进 GIF 已导出：%1").arg(outputPath));
}

bool MainWindow::exportGitEvolutionGifFile(const QString &outputPath, int fps,
                                           double transitionDuration,
                                           double pauseDuration,
                                           const QString &resolution,
                                           bool fitHeight,
                                           bool expandNames,
                                           bool showDate,
                                           int timeGranularity)
{
    m_cliExportGifPath = outputPath;
    m_cliExportGifFps = fps;
    m_cliExportGifTransitionDuration = transitionDuration;
    m_cliExportGifPauseDuration = pauseDuration;
    m_cliExportGifResolution = resolution;
    m_cliExportGifFitHeight = fitHeight;
    m_cliExportGifExpandNames = expandNames;
    m_cliExportGifShowDate = showDate;
    m_cliExportTimeGranularity = std::clamp(timeGranularity, 0, 5);
    exportGitEvolutionGif();
    m_cliExportGifPath.clear();
    return QFileInfo::exists(outputPath) && QFileInfo(outputPath).size() > 0;
}

void MainWindow::openGifExportDialogForScreenshot()
{
    exportGitEvolutionGif();
}

QString MainWindow::formatExportDateTime(const QDateTime &dateTime, int granularity) const
{
    if (!dateTime.isValid())
        return {};
    static const QStringList formats = {
        QStringLiteral("yyyy-MM-dd, HH:mm:ss"), QStringLiteral("yyyy-MM-dd, HH:mm"),
        QStringLiteral("yyyy-MM-dd, HH"), QStringLiteral("yyyy-MM-dd"),
        QStringLiteral("yyyy-MM"), QStringLiteral("yyyy")};
    return dateTime.toLocalTime().toString(formats.value(std::clamp(granularity, 0, 5)));
}

QSize MainWindow::resolveGifResolution(const QString &specification) const
{
    const QString spec = specification.trimmed().toLower();
    const QScreen *screen = QGuiApplication::screenAt(frameGeometry().center());
    if (!screen)
        screen = QGuiApplication::primaryScreen();
    if (!screen)
        return {};

    const QRect available = screen->availableGeometry();
    const QRect full = screen->geometry();
    if (spec.isEmpty() || spec == QStringLiteral("current") || spec == QStringLiteral("window")
        || spec == QStringLiteral("当前窗口"))
        return QSize(480, m_view->size().height()).expandedTo(QSize(480, 360));
    if (spec == QStringLiteral("fullscreen") || spec == QStringLiteral("full-screen")
        || spec == QStringLiteral("全屏"))
        return full.size();
    if (spec == QStringLiteral("max-height") || spec == QStringLiteral("最大高度"))
        return QSize(480, available.height());
    if (spec == QStringLiteral("max-width") || spec == QStringLiteral("最大宽度"))
        return QSize(available.width(), m_view->size().height()).expandedTo(QSize(480, 360));

    QString customSpec = spec;
    if (customSpec.startsWith(QStringLiteral("custom:")))
        customSpec = customSpec.mid(7);
    const QStringList dimensions = customSpec.split(QRegularExpression(QStringLiteral("[x×, ]")),
                                                    Qt::SkipEmptyParts);
    if (dimensions.size() != 2)
        return {};
    bool widthOk = false;
    bool heightOk = false;
    const auto dimensionValue = [this, &available](const QString &value) {
        if (value == QStringLiteral("自适应"))
            return 480;
        if (value == QStringLiteral("当前宽度"))
            return m_view->width();
        if (value == QStringLiteral("当前高度"))
            return m_view->height();
        if (value == QStringLiteral("最大宽度"))
            return available.width();
        if (value == QStringLiteral("最大高度"))
            return available.height();
        return value.toInt();
    };
    const int width = dimensionValue(dimensions.at(0));
    const int height = dimensionValue(dimensions.at(1));
    widthOk = width > 0;
    heightOk = height > 0;
    if (!widthOk || !heightOk || width <= 0 || height <= 0)
        return {};
    return QSize(width, height);
}

QString MainWindow::gitOutput(const QStringList &arguments) const
{
    if (m_gitRoot.isEmpty())
        return {};

    QProcess process;
    process.setWorkingDirectory(m_gitRoot);
    process.start(QStringLiteral("git"), arguments);
    if (!process.waitForFinished(8000) || process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != 0) {
        return {};
    }
    return QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed();
}

void MainWindow::refreshGitMetadata(const QString &path)
{
    QProcess process;
    process.setWorkingDirectory(path);
    process.start(QStringLiteral("git"), {QStringLiteral("rev-parse"),
                                           QStringLiteral("--show-toplevel")});
    if (!process.waitForFinished(5000) || process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != 0) {
        m_gitRoot.clear();
        m_gitBranch.clear();
        m_gitRevision.clear();
        m_gitSnapshot.reset();
        updateGitMenus();
        return;
    }

    m_gitRoot = QFileInfo(QString::fromLocal8Bit(process.readAllStandardOutput()).trimmed())
                    .absoluteFilePath();
    m_gitBranch = gitOutput({QStringLiteral("branch"), QStringLiteral("--show-current")});
    if (m_gitBranch.isEmpty())
        m_gitBranch = gitOutput({QStringLiteral("symbolic-ref"), QStringLiteral("--short"),
                                 QStringLiteral("-q"), QStringLiteral("HEAD")});
    updateGitMenus();
}

void MainWindow::updateGitMenus()
{
    if (!m_gitBranchMenu || !m_gitHistoryMenu)
        return;

    m_gitBranchMenu->clear();
    m_gitHistoryMenu->clear();
    const bool hasGit = !m_gitRoot.isEmpty();
    m_gitBranchButton->setEnabled(hasGit);
    m_gitHistoryButton->setEnabled(hasGit);
    m_gitBranchButton->setText(hasGit && !m_gitBranch.isEmpty()
                                   ? QStringLiteral("分支 %1").arg(m_gitBranch)
                                   : QStringLiteral("Git 分支"));
    m_gitHistoryButton->setText(m_gitRevision.isEmpty() ? QStringLiteral("Git 历史")
                                                         : QStringLiteral("历史版本"));

    if (!hasGit) {
        auto *branchAction = m_gitBranchMenu->addAction(QStringLiteral("当前目录不是 Git 工程"));
        branchAction->setEnabled(false);
        auto *historyAction = m_gitHistoryMenu->addAction(QStringLiteral("当前目录不是 Git 工程"));
        historyAction->setEnabled(false);
        return;
    }

    QStringList branches = gitOutput({QStringLiteral("for-each-ref"),
                                      QStringLiteral("--format=%(refname:short)"),
                                      QStringLiteral("refs/heads"),
                                      QStringLiteral("refs/remotes")})
                               .split(QChar('\n'), Qt::SkipEmptyParts);
    branches.removeDuplicates();
    branches.removeAll(QStringLiteral("HEAD"));
    if (branches.isEmpty() && !m_gitBranch.isEmpty())
        branches << m_gitBranch;
    for (const QString &branch : std::as_const(branches)) {
        auto *action = m_gitBranchMenu->addAction(branch);
        action->setCheckable(true);
        action->setChecked(branch == m_gitBranch);
        connect(action, &QAction::triggered, this, [this, branch]() {
            selectGitBranch(branch);
        });
    }

    auto *workingTree = m_gitHistoryMenu->addAction(QStringLiteral("当前工作区（未选择历史版本）"));
    workingTree->setCheckable(true);
    workingTree->setChecked(m_gitRevision.isEmpty());
    connect(workingTree, &QAction::triggered, this, [this]() {
        m_gitRevision.clear();
        m_gitSnapshot.reset();
        scanPath(m_projectPath);
    });
    m_gitHistoryMenu->addSeparator();

    const QString historyRef = m_gitBranch.isEmpty() ? QStringLiteral("HEAD") : m_gitBranch;
    const QString log = gitOutput({QStringLiteral("log"), historyRef,
                                   QStringLiteral("--format=%H%x09%ad%x09%s"),
                                   QStringLiteral("--date=format-local:%Y-%m-%d %H:%M:%S"),
                                   QStringLiteral("-n"),
                                   QStringLiteral("100")});
    const QStringList records = log.split(QChar('\n'), Qt::SkipEmptyParts);
    if (records.isEmpty()) {
        auto *empty = m_gitHistoryMenu->addAction(QStringLiteral("没有可用的 Git 历史记录"));
        empty->setEnabled(false);
        return;
    }
    for (const QString &record : records) {
        const QStringList fields = record.split(QChar('\t'));
        if (fields.size() < 3)
            continue;
        const QString hash = fields.at(0);
        const QString label = QStringLiteral("%1  %2")
                                  .arg(fields.at(1), fields.mid(2).join(QStringLiteral(" ")));
        auto *action = m_gitHistoryMenu->addAction(label);
        action->setData(hash);
        action->setCheckable(true);
        action->setChecked(hash == m_gitRevision);
        connect(action, &QAction::triggered, this, [this, hash]() {
            selectGitRevision(hash);
        });
    }
}

bool MainWindow::materializeGitRevision(const QString &revision, QString *snapshotPath)
{
    if (m_gitRoot.isEmpty() || revision.isEmpty() || !snapshotPath)
        return false;

    m_gitSnapshot = std::make_unique<QTemporaryDir>(QStringLiteral("ProgramViz-git-XXXXXX"));
    if (!m_gitSnapshot->isValid()) {
        m_gitSnapshot.reset();
        return false;
    }
    if (!materializeGitRevisionTo(revision, m_gitSnapshot->path())) {
        m_gitSnapshot.reset();
        return false;
    }
    *snapshotPath = m_gitSnapshot->path();
    return true;
}

bool MainWindow::materializeGitRevisionTo(const QString &revision,
                                           const QString &snapshotPath) const
{
    if (m_gitRoot.isEmpty() || revision.isEmpty() || snapshotPath.isEmpty())
        return false;

    if (!QDir().mkpath(snapshotPath))
        return false;
    QProcess list;
    list.setWorkingDirectory(m_gitRoot);
    list.start(QStringLiteral("git"), {QStringLiteral("ls-tree"), QStringLiteral("-r"),
                                        QStringLiteral("-z"), QStringLiteral("--name-only"), revision});
    if (!list.waitForFinished(30000) || list.exitStatus() != QProcess::NormalExit
        || list.exitCode() != 0) {
        return false;
    }

    QStringList sourcePaths;
    const QList<QByteArray> entries = list.readAllStandardOutput().split('\0');
    for (const QByteArray &entry : entries) {
        const QString path = QString::fromUtf8(entry);
        if (!path.isEmpty() && isGitVisualizableFile(path))
            sourcePaths.push_back(path);
    }

    QProcess batch;
    batch.setWorkingDirectory(m_gitRoot);
    batch.start(QStringLiteral("git"), {QStringLiteral("cat-file"), QStringLiteral("--batch")});
    if (!batch.waitForStarted(5000)) {
        return false;
    }

    for (const QString &path : std::as_const(sourcePaths))
        batch.write((revision + QStringLiteral(":") + path).toUtf8() + '\n');
    batch.closeWriteChannel();
    if (!batch.waitForFinished(30000) || batch.exitStatus() != QProcess::NormalExit
        || batch.exitCode() != 0) {
        return false;
    }

    const QByteArray output = batch.readAllStandardOutput();
    qsizetype offset = 0;
    for (const QString &path : std::as_const(sourcePaths)) {
        const qsizetype headerEnd = output.indexOf('\n', offset);
        if (headerEnd < 0)
            break;
        const QList<QByteArray> header = output.mid(offset, headerEnd - offset).split(' ');
        offset = headerEnd + 1;
        if (header.size() < 3)
            break;
        const QByteArray type = header.at(1);
        bool sizeOk = false;
        const qsizetype size = header.at(2).toLongLong(&sizeOk);
        if (!sizeOk || size < 0 || offset + size > output.size())
            break;
        const QByteArray contents = output.mid(offset, size);
        offset += size;
        if (offset < output.size() && output.at(offset) == '\n')
            ++offset;
        if (type != "blob")
            continue;

        const QString absoluteFilePath = QDir(snapshotPath).filePath(path);
        if (!QDir().mkpath(QFileInfo(absoluteFilePath).path()))
            continue;
        QFile file(absoluteFilePath);
        if (file.open(QIODevice::WriteOnly))
            file.write(contents);
    }
    return true;
}

void MainWindow::selectGitBranch(const QString &branch)
{
    const QString revision = gitOutput({QStringLiteral("rev-parse"), branch});
    if (revision.isEmpty())
        return;
    m_gitBranch = branch;
    m_gitRevision = revision;
    m_gitSnapshot.reset();
    scanPath(m_projectPath);
}

void MainWindow::selectGitRevision(const QString &revision)
{
    if (revision.isEmpty())
        return;
    m_gitRevision = revision;
    m_gitSnapshot.reset();
    scanPath(m_projectPath);
}

void MainWindow::scanPath(const QString &path)
{
    const QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        statusBar()->showMessage(QStringLiteral("目录不存在：%1").arg(path));
        return;
    }

    const QString absolutePath = info.absoluteFilePath();
    if (m_projectPath != absolutePath) {
        m_gitRoot.clear();
        m_gitBranch.clear();
        m_gitRevision.clear();
        m_gitSnapshot.reset();
    }
    m_projectPath = absolutePath;
    if (m_gitRoot.isEmpty())
        refreshGitMetadata(m_projectPath);
    addRecentProject(m_projectPath);
    m_pathEdit->setText(m_projectPath);
    m_scanButton->setEnabled(false);
    statusBar()->showMessage(QStringLiteral("正在扫描 %1 …").arg(m_projectPath));
    QApplication::processEvents();

    QString scanRoot = m_projectPath;
    if (!m_gitRevision.isEmpty() && !materializeGitRevision(m_gitRevision, &scanRoot)) {
        const QString failedRevision = m_gitRevision;
        m_gitRevision.clear();
        m_gitSnapshot.reset();
        updateGitMenus();
        m_scanButton->setEnabled(true);
        statusBar()->showMessage(QStringLiteral("无法读取 Git 历史版本：%1，已恢复当前工作区")
                                     .arg(failedRevision));
        return;
    }

    m_project = ProjectScanner::scan(scanRoot);
    m_scanButton->setEnabled(true);
    if (!m_project) {
        m_view->setRoot(nullptr);
        statusBar()->showMessage(QStringLiteral("扫描失败"));
        return;
    }

    if (!m_gitRevision.isEmpty()) {
        m_project->name = QFileInfo(m_projectPath).fileName();
        m_project->path = m_projectPath;
    }

    m_view->setRoot(m_project.get());
    const QString revisionText = m_gitRevision.isEmpty() ? QString()
                                                          : QStringLiteral("（Git 历史版本）");
    statusBar()->showMessage(QStringLiteral("扫描完成%1：%2 个文件，%3 行代码")
                                 .arg(revisionText)
                                 .arg(m_project->fileCount())
                                 .arg(m_project->lines.code));
    updateGitMenus();
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
