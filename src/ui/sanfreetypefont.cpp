#include "sanfreetypefont.h"

#include <QColor>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QPainter>
#include <QRect>
#include <QSize>
#include <QString>

#define NEW_FONT_PIXEL(x, y, channel) (newImage[((y)*cols + (x)) * 4 + channel])
#define FONT_PIXEL(x, y) (bitmap.buffer[(y)*rowStep + (x)])

SanFreeTypeFont *const SanFreeTypeFont::m_instance = new SanFreeTypeFont;

SanFreeTypeFont::SanFreeTypeFont()
    : m_ftLib(NULL)
{
    FT_Error error = FT_Init_FreeType(&m_ftLib);
    if (error) {
        qWarning("Error loading FreeType library.");
    }
}

QString SanFreeTypeFont::resolveFont(const QString &fontName)
{
    if (QFile::exists(fontName)) {
        return fontName;
    }

    QStringList dirsToResolve;
    QStringList extsToTry;
    QString sysfolder = QStandardPaths::writableLocation(QStandardPaths::FontsLocation);
    dirsToResolve.push_back(sysfolder);
    dirsToResolve.push_back(QDir::currentPath());
    dirsToResolve.push_back("./font");
    extsToTry.push_back("ttf");
    extsToTry.push_back("ttc");

    foreach (const QString &sdir, dirsToResolve) {
        foreach (const QString &ext, extsToTry) {
            QDir dir(sdir);
            QString filePath = dir.filePath(QString("%1.%2").arg(fontName).arg(ext));
            if (QFile::exists(filePath)) {
                return filePath;
            }
        }
    }

    return "";
}

const int *SanFreeTypeFont::loadFont(const QString &fontName)
{
    if (!m_ftLib) {
        return NULL;
    }

    QString resolvedPath = resolveFont(fontName);
    QByteArray arr = resolvedPath.toLatin1();
    const char *const fontPath = arr.constData();

    FT_Face face = NULL;
    FT_Error error = FT_New_Face(m_ftLib, fontPath, 0, &face);
    if (error) {
        if (error == FT_Err_Unknown_File_Format) {
            qWarning("Unsupported font format: %s.", fontPath);
        } else {
            qWarning("Cannot open font file: %s.", fontPath);
        }

        return NULL;
    } else {
        return (const int *const)face;
    }
}

bool SanFreeTypeFont::paintString(QPainter *const painter, const QString &text, const int *const font, const QColor &color, QSize &fontSize, int spacing, int weight,
                                  QRect &boundingBox, const Qt::Orientation &orient, const Qt::Alignment &align)
{
    if (!m_ftLib || font == NULL || painter == NULL || text.isEmpty()) {
        return false;
    }

    Qt::Alignment hAlign = align & Qt::AlignHorizontal_Mask;
    Qt::Alignment vAlign = align & Qt::AlignVertical_Mask;
    if (hAlign == 0) {
        hAlign = Qt::AlignHCenter;
    }
    if (vAlign == 0) {
        vAlign = Qt::AlignVCenter;
    }

    QPoint topLeft = boundingBox.topLeft();
    boundingBox.moveTopLeft(QPoint(0, 0));

    QVector<uint> charcodes = text.toUcs4();
    int len = charcodes.size();
    int xstep, ystep;

    if (orient == Qt::Vertical) {
        xstep = 0;
        if (fontSize.width() > boundingBox.width()) {
            fontSize.setWidth(boundingBox.width());
        }
        ystep = spacing + fontSize.height();
        // AlignJustify means the text should fill out the whole rect space
        // so we increase the step
        if (align & Qt::AlignJustify) {
            ystep = boundingBox.height() / len;
            if (fontSize.height() + spacing > ystep) {
                fontSize.setHeight(ystep - spacing);
            }
        }
    } else {
        ystep = 0;
        if (fontSize.height() > boundingBox.height()) {
            fontSize.setHeight(boundingBox.height());
        }
        xstep = spacing + fontSize.width();
        // AlignJustifx means the text should fill out the whole rect space
        // so we increase the step
        if (align & Qt::AlignJustify) {
            xstep = boundingBox.width() / len;
            if (fontSize.width() + spacing > xstep) {
                fontSize.setWidth(xstep - spacing);
            }
        }
    }

    if (fontSize.width() <= 0 || fontSize.height() <= 0) {
        return false;
    }

    // we allocate larger area than necessary in case we need bold font
    int pixelsAdded = (weight >> 6) * 2;
    int rows = boundingBox.height() + pixelsAdded + 3;
    int cols = boundingBox.width() + pixelsAdded + 3;
    int imageSize = rows * cols;
    int imageBytes = imageSize * 4;
    uchar *const newImage = new uchar[imageBytes];

    for (int i = 0; i < imageBytes;) {
        newImage[i++] = color.blue();
        newImage[i++] = color.green();
        newImage[i++] = color.red();
        newImage[i++] = 0;
    }

    m_paintTextMutex.lock();

    // we do not do kerning for vertical layout for now
    bool useKerning = ((orient == Qt::Horizontal) && !(align & Qt::AlignJustify));
    FT_Face face = (FT_Face)font;
    FT_GlyphSlot slot = face->glyph;
    FT_Error error = FT_Set_Pixel_Sizes(face, fontSize.width(), fontSize.height());
    FT_UInt previous = 0;
    int currentX = 0;
    int currentY = 0;

    for (int i = 0; i < len; ++i) {
        FT_Vector delta;
        FT_UInt glyph_index = FT_Get_Char_Index(face, charcodes[i]);
        error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
        if (error) {
            continue;
        }

        if (useKerning && previous && glyph_index) {
            error = FT_Get_Kerning(face, previous, glyph_index, FT_KERNING_DEFAULT, &delta);
            currentX += delta.x >> 6;
        }
        previous = glyph_index;

        FT_Bitmap bitmap;
        if (weight == 0) {
            FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER);
        } else {
            FT_Outline_Embolden(&slot->outline, weight);
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        }

        bitmap = slot->bitmap;

        Q_ASSERT(bitmap.pitch == bitmap.width || bitmap.pitch == (bitmap.width - 1) / 8 + 1);

        bool mono = true;
        if ((unsigned int)(bitmap.pitch) == bitmap.width) {
            mono = false;
        }

        int fontRows = bitmap.rows;
        int fontCols = bitmap.width;
        int rowStep = bitmap.pitch;
        int tmpYOffset = fontSize.height() - slot->bitmap_top;
        currentY = currentY + tmpYOffset;

        if (orient == Qt::Vertical) {
            currentX = (fontSize.width() - bitmap.width) / 2;
        }

        // now paint the bitmap to the new region
        if (currentX < 0) {
            currentX = 0;
        }
        if (currentY < 0) {
            currentY = 0;
        }

        for (int y = 0; y < fontRows; ++y) {
            if (currentY + y >= rows) {
                break;
            }

            const uchar *fontPtr = &FONT_PIXEL(0, y);
            uchar *imagePtr = &NEW_FONT_PIXEL(currentX, currentY + y, 3);

            int fontClippedCols;
            if (fontCols + currentX < cols) {
                fontClippedCols = fontCols;
            } else {
                fontClippedCols = cols - 1 - currentX;
            }

            if (!mono) {
                for (int x = 0; x < fontClippedCols; ++x) {
                    *imagePtr = *fontPtr;
                    ++fontPtr;
                    imagePtr += 4;
                }
            } else {
                int mask = 0x80;
                for (int x = 0; x < fontClippedCols; ++x) {
                    if (*fontPtr & mask) {
                        *imagePtr = 255;
                    }

                    mask = mask >> 1;
                    if (mask == 0) {
                        ++fontPtr;
                        mask = 0x80;
                    }

                    imagePtr += 4;
                }
            }
        }

        if (useKerning) {
            currentX += (slot->advance.x >> 6) + spacing;
        } else {
            currentX += xstep;
        }

        currentY = currentY - tmpYOffset + ystep;
    }

    m_paintTextMutex.unlock();

    int xstart, ystart;
    if (orient == Qt::Vertical) {
        if (hAlign & Qt::AlignLeft) {
            xstart = spacing;
        } else if (hAlign & Qt::AlignHCenter) {
            xstart = (boundingBox.width() - fontSize.width()) / 2;
        } else if (hAlign & Qt::AlignRight) {
            xstart = boundingBox.right() - spacing - fontSize.width();
        } else {
            xstart = 0;
            Q_ASSERT(false);
        }

        if (vAlign & Qt::AlignTop) {
            ystart = spacing;
        } else if (vAlign & Qt::AlignVCenter || align & Qt::AlignJustify) {
            ystart = (boundingBox.height() - currentY) / 2;
        } else if (vAlign & Qt::AlignBottom) {
            ystart = boundingBox.height() - currentY - spacing;
        } else {
            ystart = 0;
            Q_ASSERT(false);
        }
    } else {
        if (vAlign & Qt::AlignTop) {
            ystart = spacing;
        } else if (vAlign & Qt::AlignVCenter) {
            ystart = (boundingBox.height() - fontSize.height()) / 2;
        } else if (vAlign & Qt::AlignBottom) {
            ystart = boundingBox.bottom() - spacing - fontSize.height();
        } else {
            ystart = 0;
            Q_ASSERT(false);
        }

        if (hAlign & Qt::AlignLeft) {
            xstart = spacing;
        } else if (hAlign & Qt::AlignHCenter || align & Qt::AlignJustify) {
            xstart = (boundingBox.width() - currentX) / 2;
        } else if (hAlign & Qt::AlignRight) {
            xstart = boundingBox.right() - currentX - spacing;
        } else {
            xstart = 0;
            Q_ASSERT(false);
        }
    }

    if (xstart < 0) {
        xstart = 0;
    }
    if (ystart < 0) {
        ystart = 0;
    }

    QImage result(newImage, cols, rows, QImage::Format_ARGB32);
    painter->drawImage(topLeft.x() + xstart, topLeft.y() + ystart, result);

    delete[] newImage;

    return true;
}

bool SanFreeTypeFont::paintStringMultiLine(QPainter *const painter, const QString &text, const int *const font, const QColor &color, QSize &fontSize, int spacing, int weight,
                                           QRect &boundingBox, const Qt::Alignment &align)
{
    if (!m_ftLib || font == NULL || painter == NULL || text.isEmpty()) {
        return false;
    }

    QVector<uint> charcodes = text.toUcs4();
    int len = charcodes.size();
    int charsPerLine = boundingBox.width() / fontSize.width();
    int numLines = (len - 1) / charsPerLine + 1;

    QPoint topLeft = boundingBox.topLeft();
    boundingBox.moveTopLeft(QPoint(0, 0));

    int xstep;
    // AlignJustifx means the text should fill out the whole rect space
    // so we increase the step
    if (align & Qt::AlignJustify) {
        xstep = boundingBox.width() / len;
        if (fontSize.width() + spacing > xstep) {
            fontSize.setWidth(xstep - spacing);
        }
    } else {
        xstep = spacing + fontSize.width();
    }

    if (fontSize.height() * numLines > boundingBox.height()) {
        fontSize.setHeight(boundingBox.height() / numLines - spacing);
    }

    int ystep = fontSize.height() + spacing;

    if (fontSize.width() <= 0 || fontSize.height() <= 0) {
        return false;
    }

    // we allocate larger area than necessary in case we need bold font
    int pixelsAdded = (weight >> 6) * 2;
    int rows = boundingBox.height() + pixelsAdded + 3;
    int cols = boundingBox.width() + pixelsAdded + 3;
    int imageSize = rows * cols;
    int imageBytes = imageSize * 4;
    uchar *const newImage = new uchar[imageBytes];

    for (int i = 0; i < imageBytes;) {
        newImage[i++] = color.blue();
        newImage[i++] = color.green();
        newImage[i++] = color.red();
        newImage[i++] = 0;
    }

    m_paintTextMutex.lock();

    // we do not do kerning for vertical layout for now
    bool useKerning = (!(align & Qt::AlignJustify));
    FT_UInt previous = 0;
    int currentX = 0;
    int currentY = 0;
    int maxX = 0;
    int maxY = 0;
    FT_Face face = (FT_Face)font;

    FT_GlyphSlot slot = face->glyph;
    FT_Error error = FT_Set_Pixel_Sizes(face, fontSize.width(), fontSize.height());
    for (int i = 0, j = 0; i < len; ++i, ++j) {
        if (QChar('\n').unicode() == charcodes[i]) {
            currentY += ystep;
            currentX = 0;
            j = -1;

            continue;
        }

        int line = j / charsPerLine;
        int cursor = j % charsPerLine;
        // whenever we start a new line, reset X and increment Y
        if (cursor == 0 && line > 0) {
            currentY += ystep;
            currentX = 0;
            j = 0;
        }

        FT_Vector delta;
        FT_UInt glyph_index = FT_Get_Char_Index(face, charcodes[i]);
        error = FT_Load_Glyph(face, glyph_index, FT_LOAD_DEFAULT | FT_LOAD_NO_BITMAP);
        if (error) {
            continue;
        }

        if (useKerning && previous && glyph_index) {
            error = FT_Get_Kerning(face, previous, glyph_index, FT_KERNING_DEFAULT, &delta);
            currentX += delta.x >> 6;
        }
        previous = glyph_index;

        if (weight == 0) {
            FT_Load_Glyph(face, glyph_index, FT_LOAD_RENDER);
        } else {
            FT_Outline_Embolden(&slot->outline, weight);
            FT_Render_Glyph(face->glyph, FT_RENDER_MODE_NORMAL);
        }

        FT_Bitmap bitmap = slot->bitmap;
        int fontRows = bitmap.rows;
        int fontCols = bitmap.width;
        int rowStep = bitmap.pitch;
        int tmpYOffset = fontSize.height() - slot->bitmap_top;
        currentY = currentY + tmpYOffset;

        Q_ASSERT(bitmap.pitch == bitmap.width || bitmap.pitch == (bitmap.width - 1) / 8 + 1);

        //@todo put it back
        bool mono = true;
        if ((unsigned int)(bitmap.pitch) == bitmap.width) {
            mono = false;
        }

        // now paint the bitmap to the new region
        Q_ASSERT(currentX >= 0 && currentY >= 0);

        for (int y = 0; y < fontRows; ++y) {
            if (currentY + y >= rows) {
                break;
            }

            const uchar *fontPtr = &FONT_PIXEL(0, y);
            uchar *imagePtr = &NEW_FONT_PIXEL(currentX, currentY + y, 3);

            int fontClippedCols;
            if (fontCols + currentX < cols) {
                fontClippedCols = fontCols;
            } else {
                fontClippedCols = cols - 1 - currentX;
            }

            if (!mono) {
                for (int x = 0; x < fontClippedCols; ++x) {
                    *imagePtr = *fontPtr;
                    ++fontPtr;
                    imagePtr += 4;
                }
            } else {
                int mask = 0x80;
                for (int x = 0; x < fontClippedCols; ++x) {
                    if (*fontPtr & mask) {
                        *imagePtr = 255;
                    }

                    mask = mask >> 1;
                    if (mask == 0) {
                        ++fontPtr;
                        mask = 0x80;
                    }

                    imagePtr += 4;
                }
            }
        }

        if (useKerning) {
            currentX += (slot->metrics.width >> 6) + spacing;
        } else {
            currentX += xstep;
        }

        if (currentX > maxX) {
            maxX = currentX;
        }

        currentY -= tmpYOffset;
    }

    m_paintTextMutex.unlock();

    maxY = currentY + ystep;

    Qt::Alignment hAlign = align & Qt::AlignHorizontal_Mask;
    Qt::Alignment vAlign = align & Qt::AlignVertical_Mask;

    int xstart, ystart;
    if (hAlign & Qt::AlignLeft) {
        xstart = spacing;
    } else if (hAlign & Qt::AlignHCenter || align & Qt::AlignJustify) {
        xstart = (boundingBox.width() - maxX) / 2;
    } else if (hAlign & Qt::AlignRight) {
        xstart = boundingBox.right() - maxX - spacing;
    } else {
        xstart = 0;
        Q_ASSERT(false);
    }

    if (vAlign & Qt::AlignTop) {
        ystart = spacing;
    } else if (vAlign & Qt::AlignVCenter) {
        ystart = (boundingBox.height() - maxY) / 2;
    } else if (vAlign & Qt::AlignBottom) {
        ystart = boundingBox.height() - maxY - spacing;
    } else {
        ystart = 0;
        Q_ASSERT(false);
    }

    if (xstart < 0) {
        xstart = 0;
    }
    if (ystart < 0) {
        ystart = 0;
    }

    QImage result(newImage, cols, rows, QImage::Format_ARGB32);
    painter->drawImage(topLeft.x() + xstart, topLeft.y() + ystart, result);

    delete[] newImage;

    return true;
}
