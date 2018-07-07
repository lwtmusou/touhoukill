#ifndef SANFREETYPEFONT_H
#define SANFREETYPEFONT_H

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_OUTLINE_H

#include <QMutex>
#include <Qt>

class QString;
class QPainter;
class QColor;
class QSize;
class QRect;

class SanFreeTypeFont
{
public:
    static SanFreeTypeFont *getInstance()
    {
        return m_instance;
    }

    QString resolveFont(const QString &fontName);
    const int *loadFont(const QString &fontName);

    // @param painter
    //        Device to be painted on
    // @param text
    //        Text to be painted
    // @param font
    //        Pointer returned by loadFont used to index a font
    // @param fontSize [IN, OUT]
    //        Suggested width and height of each character in pixels. If the
    //        bounding box cannot contain the text using the suggested font
    //        size, font size may be shrinked. The output value will be the
    //        actual font size used.
    // @param boundingBox
    //        Text will be painted in the center of the bounding box on the device
    // @param orient
    //        Suggest whether the text is laid out horizontally or vertically.
    // @return true if succeed.
    bool paintString(QPainter *const painter, const QString &text, const int *const font, const QColor &color, QSize &fontSize, int spacing, int weight, QRect &boundingBox,
                     const Qt::Orientation &orient, const Qt::Alignment &align);

    // Currently, we online support horizotal layout for multiline text
    bool paintStringMultiLine(QPainter *const painter, const QString &text, const int *const font, const QColor &color, QSize &fontSize, int spacing, int weight,
                              QRect &boundingBox, const Qt::Alignment &align);

private:
    SanFreeTypeFont();

    FT_Library m_ftLib;
    QMutex m_paintTextMutex;

    static SanFreeTypeFont *const m_instance;
};

#endif // SANFREETYPEFONT_H
