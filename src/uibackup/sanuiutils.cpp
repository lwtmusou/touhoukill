#include "sanuiutils.h"

#include <QColor>
#include <QImage>
#include <QPixmap>

#define NEW_PIXEL_CHANNEL(x, y, channel) (newImage[(y * cols + x) * 4 + channel])
#define NEW_PIXEL(x, y) NEW_PIXEL_CHANNEL(x, y, 3)
#define OLD_PIXEL(x, y) (oldImage[(y * cols + x) * 4 + 3])

void SanUiUtils::makeGray(QPixmap &pixmap)
{
    QImage img = pixmap.toImage();

    for (int i = 0, w = img.width(); i < w; ++i) {
        for (int j = 0, h = img.height(); j < h; ++j) {
            QColor color = QColor::fromRgba(img.pixel(i, j));
            int gray = qGray(color.rgb());
            img.setPixel(i, j, qRgba(gray, gray, gray, color.alpha()));
        }
    }

    pixmap = QPixmap::fromImage(img);
}

QImage SanUiUtils::produceShadow(const QImage &image, const QColor &shadowColor, int radius, double decade)
{
    const uchar *const oldImage = image.bits();
    int cols = image.width();
    int rows = image.height();

    int alpha = shadowColor.alpha();

    uchar *const newImage = new uchar[cols * rows * 4];

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            NEW_PIXEL_CHANNEL(x, y, 0) = shadowColor.blue();
            NEW_PIXEL_CHANNEL(x, y, 1) = shadowColor.green();
            NEW_PIXEL_CHANNEL(x, y, 2) = shadowColor.red();
            NEW_PIXEL_CHANNEL(x, y, 3) = 0;
        }
    }

    for (int y = 0; y < rows; ++y) {
        for (int x = 0; x < cols; ++x) {
            uchar oldVal = OLD_PIXEL(x, y);
            if (oldVal == 0) {
                continue;
            }

            for (int dy = -radius; dy <= radius; ++dy) {
                for (int dx = -radius; dx <= radius; ++dx) {
                    int wx = x + dx;
                    int wy = y + dy;
                    int dist = dx * dx + dy * dy;

                    if (wx < 0 || wy < 0 || wx >= cols || wy >= rows) {
                        continue;
                    }

                    if (dx * dx + dy * dy > radius * radius) {
                        continue;
                    }

                    Q_ASSERT((wy * cols + wx) * 4 < cols * rows * 4);

                    int newVal = alpha - decade * dist;
                    NEW_PIXEL(wx, wy) = (uchar)qMax((int)NEW_PIXEL(wx, wy), newVal);
                }
            }
        }
    }

    QImage shadowImage(newImage, cols, rows, QImage::Format_ARGB32);
    QImage result(shadowImage.copy());
    delete[] newImage;

    return result;
}
