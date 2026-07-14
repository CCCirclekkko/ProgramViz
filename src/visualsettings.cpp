#include "visualsettings.h"

#include <QtMath>

QColor colorFromHsi(int hue, int saturation, int intensity)
{
    const double h = (hue % 360 + 360) % 360 / 60.0;
    const double s = qBound(0.0, saturation / 100.0, 1.0);
    const double i = qBound(0.0, intensity / 100.0, 1.0);
    double red = i;
    double green = i;
    double blue = i;

    if (s > 0.0001) {
        if (h < 2.0) {
            blue = i * (1.0 - s);
            red = i * (1.0 + s * qCos(M_PI * h / 3.0)
                       / qCos(M_PI * (1.0 - h) / 3.0));
            green = 3.0 * i - red - blue;
        } else if (h < 4.0) {
            const double localHue = h - 2.0;
            red = i * (1.0 - s);
            green = i * (1.0 + s * qCos(M_PI * localHue / 3.0)
                         / qCos(M_PI * (1.0 - localHue) / 3.0));
            blue = 3.0 * i - red - green;
        } else {
            const double localHue = h - 4.0;
            green = i * (1.0 - s);
            blue = i * (1.0 + s * qCos(M_PI * localHue / 3.0)
                        / qCos(M_PI * (1.0 - localHue) / 3.0));
            red = 3.0 * i - green - blue;
        }
    }

    return QColor::fromRgbF(qBound(0.0, red, 1.0),
                            qBound(0.0, green, 1.0),
                            qBound(0.0, blue, 1.0));
}
