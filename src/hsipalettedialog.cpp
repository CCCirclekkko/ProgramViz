#include "hsipalettedialog.h"

#include "visualsettings.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QLabel>
#include <QSpinBox>
#include <QVBoxLayout>

HsiPaletteDialog::HsiPaletteDialog(int hue, int saturation, int intensity, QWidget *parent)
    : QDialog(parent)
{
    setWindowTitle(QStringLiteral("自定义 HSI 配色"));
    setModal(true);
    resize(360, 250);

    auto *description = new QLabel(
        QStringLiteral("调节色相、饱和度和强度，节点将使用中饱和度的同色系渐变。"), this);
    description->setWordWrap(true);

    m_hue = new QSpinBox(this);
    m_hue->setRange(0, 359);
    m_hue->setSuffix(QStringLiteral("°"));
    m_hue->setValue(hue);
    m_saturation = new QSpinBox(this);
    m_saturation->setRange(10, 100);
    m_saturation->setSuffix(QStringLiteral("%"));
    m_saturation->setValue(saturation);
    m_intensity = new QSpinBox(this);
    m_intensity->setRange(35, 100);
    m_intensity->setSuffix(QStringLiteral("%"));
    m_intensity->setValue(intensity);

    auto *preview = new QLabel(QStringLiteral("预览色"), this);
    preview->setObjectName(QStringLiteral("hsiPreview"));
    preview->setMinimumHeight(42);
    preview->setAlignment(Qt::AlignCenter);
    preview->setStyleSheet(QStringLiteral("#hsiPreview { border-radius: 8px; }"));

    auto *form = new QFormLayout;
    form->addRow(QStringLiteral("色相 H"), m_hue);
    form->addRow(QStringLiteral("饱和度 S"), m_saturation);
    form->addRow(QStringLiteral("强度 I"), m_intensity);

    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(m_hue, &QSpinBox::valueChanged, this, &HsiPaletteDialog::updatePreview);
    connect(m_saturation, &QSpinBox::valueChanged, this, &HsiPaletteDialog::updatePreview);
    connect(m_intensity, &QSpinBox::valueChanged, this, &HsiPaletteDialog::updatePreview);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(description);
    layout->addLayout(form);
    layout->addWidget(preview);
    layout->addWidget(buttons);
    updatePreview();
}

int HsiPaletteDialog::hue() const
{
    return m_hue->value();
}

int HsiPaletteDialog::saturation() const
{
    return m_saturation->value();
}

int HsiPaletteDialog::intensity() const
{
    return m_intensity->value();
}

void HsiPaletteDialog::updatePreview()
{
    auto *preview = findChild<QLabel *>(QStringLiteral("hsiPreview"));
    if (!preview)
        return;
    const QColor color = colorFromHsi(hue(), saturation(), intensity());
    preview->setText(color.name(QColor::HexRgb));
    preview->setStyleSheet(QStringLiteral(
                               "#hsiPreview { background: %1; color: %2; border-radius: 8px; }")
                               .arg(color.name(), color.lightness() < 140 ? QStringLiteral("white")
                                                                           : QStringLiteral("#182235")));
}
