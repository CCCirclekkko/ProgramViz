#include "mainwindow.h"

#include "hsipalettedialog.h"
#include "treemapview.h"

#include <QApplication>
#include <QClipboard>
#include <QComboBox>
#include <QDateTime>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QProcess>
#include <QSettings>
#include <QSpinBox>
#include <QSplitter>
#include <QStatusBar>
#include <QUrl>
#include <QVBoxLayout>

#include <QDesktopServices>

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
    m_scanButton = new QPushButton(QStringLiteral("扫描"), toolbar);
    m_themeCombo = new QComboBox(toolbar);
    m_themeCombo->addItem(QStringLiteral("跟随系统"), static_cast<int>(ThemeMode::System));
    m_themeCombo->addItem(QStringLiteral("浅色模式"), static_cast<int>(ThemeMode::Light));
    m_themeCombo->addItem(QStringLiteral("深色模式"), static_cast<int>(ThemeMode::Dark));
    m_themeCombo->setCurrentIndex(m_themeCombo->findData(static_cast<int>(m_visualSettings.theme)));
    m_schemeCombo = new QComboBox(toolbar);
    m_schemeCombo->addItem(QStringLiteral("同级同色"), static_cast<int>(ColorScheme::SameHueByLevel));
    m_schemeCombo->addItem(QStringLiteral("逐级不同色系"), static_cast<int>(ColorScheme::DifferentHueByLevel));
    m_schemeCombo->setCurrentIndex(
        m_schemeCombo->findData(static_cast<int>(m_visualSettings.colorScheme)));
    m_fontSizeSpin = new QSpinBox(toolbar);
    m_fontSizeSpin->setRange(9, 24);
    m_fontSizeSpin->setSuffix(QStringLiteral(" pt"));
    m_fontSizeSpin->setValue(m_visualSettings.fontSize);
    m_fontSizeSpin->setToolTip(QStringLiteral("节点文字字号"));
    m_paletteButton = new QPushButton(QStringLiteral("HSI 配色"), toolbar);
    toolbarLayout->addWidget(pathLabel);
    toolbarLayout->addWidget(m_pathEdit, 1);
    toolbarLayout->addWidget(browseButton);
    toolbarLayout->addWidget(m_scanButton);
    toolbarLayout->addSpacing(4);
    toolbarLayout->addWidget(m_themeCombo);
    toolbarLayout->addWidget(m_schemeCombo);
    toolbarLayout->addWidget(m_fontSizeSpin);
    toolbarLayout->addWidget(m_paletteButton);

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

    auto *splitter = new QSplitter(Qt::Horizontal, this);
    splitter->addWidget(m_view);
    splitter->addWidget(detailsPanel);
    splitter->setStretchFactor(0, 1);
    splitter->setSizes({900, 320});

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
    connect(m_themeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_visualSettings.theme = static_cast<ThemeMode>(m_themeCombo->itemData(index).toInt());
        saveSettings();
        applyAppearance();
    });
    connect(m_schemeCombo, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_visualSettings.colorScheme = static_cast<ColorScheme>(m_schemeCombo->itemData(index).toInt());
        saveSettings();
        applyAppearance();
    });
    connect(m_fontSizeSpin, &QSpinBox::valueChanged, this, [this](int value) {
        m_visualSettings.fontSize = value;
        saveSettings();
        applyAppearance();
    });
    connect(m_paletteButton, &QPushButton::clicked, this, &MainWindow::openPaletteDialog);
    applyAppearance();
}

void MainWindow::loadSettings()
{
    QSettings settings;
    m_visualSettings.theme = static_cast<ThemeMode>(
        settings.value(QStringLiteral("appearance/theme"), static_cast<int>(ThemeMode::System)).toInt());
    m_visualSettings.colorScheme = static_cast<ColorScheme>(
        settings.value(QStringLiteral("appearance/colorScheme"),
                       static_cast<int>(ColorScheme::DifferentHueByLevel)).toInt());
    m_visualSettings.fontSize = settings.value(QStringLiteral("appearance/fontSize"), 11).toInt();
    m_visualSettings.hue = settings.value(QStringLiteral("appearance/hue"), 212).toInt();
    m_visualSettings.saturation = settings.value(QStringLiteral("appearance/saturation"), 46).toInt();
    m_visualSettings.intensity = settings.value(QStringLiteral("appearance/intensity"), 84).toInt();
    m_visualSettings.fontSize = qBound(9, m_visualSettings.fontSize, 24);
    m_visualSettings.hue = (m_visualSettings.hue % 360 + 360) % 360;
    m_visualSettings.saturation = qBound(10, m_visualSettings.saturation, 100);
    m_visualSettings.intensity = qBound(35, m_visualSettings.intensity, 100);
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

void MainWindow::openPaletteDialog()
{
    HsiPaletteDialog dialog(m_visualSettings.hue,
                            m_visualSettings.saturation,
                            m_visualSettings.intensity,
                            this);
    if (dialog.exec() != QDialog::Accepted)
        return;
    m_visualSettings.hue = dialog.hue();
    m_visualSettings.saturation = dialog.saturation();
    m_visualSettings.intensity = dialog.intensity();
    saveSettings();
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
