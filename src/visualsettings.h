#pragma once

#include <QColor>

enum class ThemeMode {
    System = 0,
    Light = 1,
    Dark = 2,
};

enum class ColorScheme {
    SameHueByLevel = 0,
    DifferentHueByLevel = 1,
};

struct VisualSettings {
    ThemeMode theme = ThemeMode::System;
    ColorScheme colorScheme = ColorScheme::DifferentHueByLevel;
    int fontSize = 16;
    int hue = 212;
    int saturation = 46;
    int intensity = 84;
    double minHeightRatio = 0.6;
    double cornerRatio = 1.0 / 6.0;
    int columnGap = 10;
    int siblingGap = 6;
    bool livePreview = true;
};

QColor colorFromHsi(int hue, int saturation, int intensity);
