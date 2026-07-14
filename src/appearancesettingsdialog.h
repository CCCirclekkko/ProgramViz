#pragma once

#include "visualsettings.h"

#include <QDialog>

class QCheckBox;
class QComboBox;
class QLabel;
class QPushButton;
class QSlider;

class AppearanceSettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit AppearanceSettingsDialog(const VisualSettings &settings,
                                      QWidget *parent = nullptr);

    VisualSettings settings() const;
    bool livePreview() const;

signals:
    void settingsChanged(const VisualSettings &settings);

private:
    void emitPreviewIfEnabled();
    void updateValueLabels();
    void openPalette();

    QComboBox *m_themeCombo = nullptr;
    QComboBox *m_schemeCombo = nullptr;
    QLabel *m_fontValue = nullptr;
    QLabel *m_minHeightValue = nullptr;
    QLabel *m_cornerValue = nullptr;
    QLabel *m_columnGapValue = nullptr;
    QLabel *m_siblingGapValue = nullptr;
    QSlider *m_fontSlider = nullptr;
    QSlider *m_minHeightSlider = nullptr;
    QSlider *m_cornerSlider = nullptr;
    QSlider *m_columnGapSlider = nullptr;
    QSlider *m_siblingGapSlider = nullptr;
    QCheckBox *m_livePreviewCheck = nullptr;
    QPushButton *m_paletteButton = nullptr;
    int m_hue = 212;
    int m_saturation = 46;
    int m_intensity = 84;
};
