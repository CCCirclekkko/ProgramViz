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
    int fontSize = 11;
    int hue = 212;
    int saturation = 46;
    int intensity = 84;
};

QColor colorFromHsi(int hue, int saturation, int intensity);
