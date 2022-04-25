#include "uiUtils.h"

#include "engine.h"

#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QImage>
#include <QMutex>
#include <QPixmap>

QImage QSanUiUtils::produceShadow(const QImage &image, QColor shadowColor, int radius, double decade)
{
    const uchar *oldImage = image.bits();
    int cols = image.width();
    int rows = image.height();
    int alpha = shadowColor.alpha();
    uchar *newImage = new uchar[cols * rows * 4];
#define _NEW_PIXEL_CHANNEL(x, y, channel) (newImage[(y * cols + x) * 4 + channel])
#define _NEW_PIXEL(x, y) _NEW_PIXEL_CHANNEL(x, y, 3)
#define _OLD_PIXEL(x, y) (oldImage[(y * cols + x) * 4 + 3])
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            _NEW_PIXEL_CHANNEL(x, y, 0) = shadowColor.blue();
            _NEW_PIXEL_CHANNEL(x, y, 1) = shadowColor.green();
            _NEW_PIXEL_CHANNEL(x, y, 2) = shadowColor.red();
            _NEW_PIXEL_CHANNEL(x, y, 3) = 0;
        }
    }

    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            uchar oldVal = _OLD_PIXEL(x, y);
            if (oldVal == 0)
                continue;
            for (int dy = -radius; dy <= radius; dy++) {
                for (int dx = -radius; dx <= radius; dx++) {
                    int wx = x + dx;
                    int wy = y + dy;
                    int dist = dx * dx + dy * dy;
                    if (wx < 0 || wy < 0 || wx >= cols || wy >= rows)
                        continue;
                    if (dx * dx + dy * dy > radius * radius)
                        continue;
                    int newVal = alpha - decade * dist;
                    Q_ASSERT((wy * cols + wx) * 4 < cols * rows * 4);
                    _NEW_PIXEL(wx, wy) = (uchar)qMax((int)_NEW_PIXEL(wx, wy), newVal);
                }
            }
        }
    }
#undef _NEW_PIXEL_CHANNEL
#undef _NEW_PIXEL
#undef _OLD_PIXEL
    QImage result(newImage, cols, rows, QImage::Format_ARGB32);
    return result;
}

void QSanUiUtils::makeGray(QPixmap &pixmap)
{
    QImage img = pixmap.toImage();
    for (int i = 0; i < img.width(); i++) {
        for (int j = 0; j < img.height(); j++) {
            QColor color = QColor::fromRgba(img.pixel(i, j));
            int gray = qGray(color.rgb());
            img.setPixel(i, j, qRgba(gray, gray, gray, color.alpha()));
        }
    }
    pixmap = QPixmap::fromImage(img);
}

QColor QSanUiUtils::getKingdomColor(const QString &kingdom)
{
    static QMap<QString, QColor> color_map;
    if (color_map.isEmpty()) {
        QVariantMap map = Sanguosha->getConfigFromConfigFile(QStringLiteral("kingdom_colors")).toMap();
        QMapIterator<QString, QVariant> itor(map);
        while (itor.hasNext()) {
            itor.next();
            QColor color(itor.value().toString());
            if (!color.isValid()) {
                qWarning("Invalid color for kingdom %s", qPrintable(itor.key()));
                color = QColor(128, 128, 128);
            }
            color_map[itor.key()] = color;
        }

        Q_ASSERT(!color_map.isEmpty());
    }

    return color_map.value(kingdom);
}
