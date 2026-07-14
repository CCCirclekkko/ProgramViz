#pragma once

#include <QDialog>

class QSpinBox;

class HsiPaletteDialog final : public QDialog {
    Q_OBJECT

public:
    HsiPaletteDialog(int hue, int saturation, int intensity, QWidget *parent = nullptr);

    int hue() const;
    int saturation() const;
    int intensity() const;

private:
    void updatePreview();

    QSpinBox *m_hue = nullptr;
    QSpinBox *m_saturation = nullptr;
    QSpinBox *m_intensity = nullptr;
};
