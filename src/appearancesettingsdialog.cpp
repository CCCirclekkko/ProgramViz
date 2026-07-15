#include "appearancesettingsdialog.h"

#include "hsipalettedialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSlider>
#include <QVBoxLayout>

namespace {

QWidget *sliderRow(QSlider *slider, QLabel **valueLabel, QWidget *parent)
{
    auto *row = new QWidget(parent);
    auto *layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    *valueLabel = new QLabel(row);
    (*valueLabel)->setMinimumWidth(52);
    (*valueLabel)->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    layout->addWidget(slider, 1);
    layout->addWidget(*valueLabel);
    return row;
}

} // namespace

AppearanceSettingsDialog::AppearanceSettingsDialog(const VisualSettings &initial,
                                                   QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("外观设置"));
    setModal(true);
    resize(520, 430);
    m_hue = initial.hue;
    m_saturation = initial.saturation;
    m_intensity = initial.intensity;

    m_themeCombo = new QComboBox(this);
    m_themeCombo->addItem(QStringLiteral("跟随系统"), static_cast<int>(ThemeMode::System));
    m_themeCombo->addItem(QStringLiteral("浅色模式"), static_cast<int>(ThemeMode::Light));
    m_themeCombo->addItem(QStringLiteral("深色模式"), static_cast<int>(ThemeMode::Dark));
    m_themeCombo->setCurrentIndex(m_themeCombo->findData(static_cast<int>(initial.theme)));

    m_schemeCombo = new QComboBox(this);
    m_schemeCombo->addItem(QStringLiteral("同级同色"), static_cast<int>(ColorScheme::SameHueByLevel));
    m_schemeCombo->addItem(QStringLiteral("逐级不同色系"), static_cast<int>(ColorScheme::DifferentHueByLevel));
    m_schemeCombo->setCurrentIndex(
        m_schemeCombo->findData(static_cast<int>(initial.colorScheme)));

    m_paletteButton = new QPushButton(QStringLiteral("HSI 调色盘…"), this);
    m_fontSlider = new QSlider(Qt::Horizontal, this);
    m_fontSlider->setRange(9, 24);
    m_fontSlider->setValue(initial.fontSize);
    m_minHeightSlider = new QSlider(Qt::Horizontal, this);
    m_minHeightSlider->setRange(1, 200);
    m_minHeightSlider->setValue(qRound(initial.minHeightRatio * 100.0));
    m_cornerSlider = new QSlider(Qt::Horizontal, this);
    m_cornerSlider->setRange(5, 30);
    m_cornerSlider->setValue(qRound(initial.cornerRatio * 100.0));
    m_columnGapSlider = new QSlider(Qt::Horizontal, this);
    m_columnGapSlider->setRange(0, 30);
    m_columnGapSlider->setValue(initial.columnGap);
    m_siblingGapSlider = new QSlider(Qt::Horizontal, this);
    m_siblingGapSlider->setRange(0, 20);
    m_siblingGapSlider->setValue(initial.siblingGap);
    m_livePreviewCheck = new QCheckBox(QStringLiteral("实时应用预览"), this);
    m_livePreviewCheck->setChecked(initial.livePreview);

    auto *form = new QFormLayout;
    form->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
    form->addRow(QStringLiteral("主题"), m_themeCombo);
    form->addRow(QStringLiteral("上色方案"), m_schemeCombo);
    form->addRow(QStringLiteral("调色盘"), m_paletteButton);
    form->addRow(QStringLiteral("字号"), sliderRow(m_fontSlider, &m_fontValue, this));
    form->addRow(QStringLiteral("最小高度比例"),
                 sliderRow(m_minHeightSlider, &m_minHeightValue, this));
    form->addRow(QStringLiteral("圆角比例"), sliderRow(m_cornerSlider, &m_cornerValue, this));
    form->addRow(QStringLiteral("级间距"), sliderRow(m_columnGapSlider, &m_columnGapValue, this));
    form->addRow(QStringLiteral("同级间距"), sliderRow(m_siblingGapSlider, &m_siblingGapValue, this));

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    buttons->button(QDialogButtonBox::Ok)->setText(QStringLiteral("确定"));
    buttons->button(QDialogButtonBox::Cancel)->setText(QStringLiteral("取消"));
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addWidget(m_livePreviewCheck);
    layout->addStretch(1);
    layout->addWidget(buttons);

    connect(m_themeCombo, &QComboBox::currentIndexChanged, this,
            &AppearanceSettingsDialog::emitPreviewIfEnabled);
    connect(m_schemeCombo, &QComboBox::currentIndexChanged, this,
            &AppearanceSettingsDialog::emitPreviewIfEnabled);
    connect(m_fontSlider, &QSlider::valueChanged, this,
            &AppearanceSettingsDialog::emitPreviewIfEnabled);
    connect(m_minHeightSlider, &QSlider::valueChanged, this,
            &AppearanceSettingsDialog::emitPreviewIfEnabled);
    connect(m_cornerSlider, &QSlider::valueChanged, this,
            &AppearanceSettingsDialog::emitPreviewIfEnabled);
    connect(m_columnGapSlider, &QSlider::valueChanged, this,
            &AppearanceSettingsDialog::emitPreviewIfEnabled);
    connect(m_siblingGapSlider, &QSlider::valueChanged, this,
            &AppearanceSettingsDialog::emitPreviewIfEnabled);
    connect(m_livePreviewCheck, &QCheckBox::toggled, this, [this]() {
        updateValueLabels();
        emitPreviewIfEnabled();
    });
    connect(m_paletteButton, &QPushButton::clicked, this, &AppearanceSettingsDialog::openPalette);
    updateValueLabels();
}

VisualSettings AppearanceSettingsDialog::settings() const
{
    VisualSettings current;
    current.theme = static_cast<ThemeMode>(m_themeCombo->currentData().toInt());
    current.colorScheme = static_cast<ColorScheme>(m_schemeCombo->currentData().toInt());
    current.fontSize = m_fontSlider->value();
    current.minHeightRatio = m_minHeightSlider->value() / 100.0;
    current.cornerRatio = m_cornerSlider->value() / 100.0;
    current.columnGap = m_columnGapSlider->value();
    current.siblingGap = m_siblingGapSlider->value();
    current.livePreview = m_livePreviewCheck->isChecked();
    current.hue = m_hue;
    current.saturation = m_saturation;
    current.intensity = m_intensity;
    return current;
}

bool AppearanceSettingsDialog::livePreview() const
{
    return m_livePreviewCheck->isChecked();
}

void AppearanceSettingsDialog::emitPreviewIfEnabled()
{
    updateValueLabels();
    if (livePreview())
        emit settingsChanged(settings());
}

void AppearanceSettingsDialog::updateValueLabels()
{
    m_fontValue->setText(QStringLiteral("%1 pt").arg(m_fontSlider->value()));
    m_minHeightValue->setText(QStringLiteral("%1×")
                                  .arg(m_minHeightSlider->value() / 100.0, 0, 'f', 2));
    m_cornerValue->setText(QStringLiteral("%1%").arg(m_cornerSlider->value()));
    m_columnGapValue->setText(QStringLiteral("%1 px").arg(m_columnGapSlider->value()));
    m_siblingGapValue->setText(QStringLiteral("%1 px").arg(m_siblingGapSlider->value()));
}

void AppearanceSettingsDialog::openPalette()
{
    const VisualSettings current = settings();
    HsiPaletteDialog dialog(current.hue, current.saturation, current.intensity, this);
    if (dialog.exec() != QDialog::Accepted)
        return;

    m_hue = dialog.hue();
    m_saturation = dialog.saturation();
    m_intensity = dialog.intensity();
    emitPreviewIfEnabled();
}
