#ifndef SANUIUTILS_H
#define SANUIUTILS_H

#include "json.h"

class QPixmap;
class QImage;
class QColor;

namespace SanUiUtils {
void makeGray(QPixmap &pixmap);

// This is in no way a generic diation fuction. It is some dirty trick that
// produces a shadow image for a pixmap whose foreground mask is binaryImage
QImage produceShadow(const QImage &image, const QColor &shadowColor, int radius, double decade);
}

#endif // SANUIUTILS_H
