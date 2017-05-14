#include "sansimpletextfont.h"
#include "sanfreetypefont.h"

#include <QGraphicsPixmapItem>
#include <QHash>
#include <QPainter>
#include <QPixmap>
#include <QRect>

using namespace JsonUtils;

QHash<QString, const int *> SanSimpleTextFont::m_fontBank;

SanSimpleTextFont::SanSimpleTextFont()
    : m_fontFace(NULL)
    , m_fontSize(QSize(12, 12))
    , m_color(Qt::white)
    , m_spacing(0)
    , m_weight(0)
    , m_vertical(false)
{
}

SanSimpleTextFont::SanSimpleTextFont(const QString &fontName)
    : m_fontSize(QSize(12, 12))
    , m_color(Qt::white)
    , m_spacing(0)
    , m_weight(0)
    , m_vertical(false)
{
    _initFontFace(fontName);
}

SanSimpleTextFont::SanSimpleTextFont(const QString &fontName, const QSize &fontSize, const QColor &color, int spacing, int weight)
    : m_vertical(false)
{
    _initFontFace(fontName);

    m_fontSize = fontSize;
    m_color = color;
    m_spacing = spacing;
    m_weight = weight;
}

bool SanSimpleTextFont::tryParse(const QVariant &argvar)
{
    JsonArray arg = argvar.value<JsonArray>();
    if (arg.length() < 4)
        return false;

    m_vertical = false;
    _initFontFace(arg[0].toString());

    if (arg[1].toInt()) {
        m_fontSize.setWidth(arg[1].toInt());
        m_fontSize.setHeight(arg[1].toInt());
        m_spacing = 0;
    } else {
        JsonArray arg1 = arg[1].value<JsonArray>();
        m_fontSize.setWidth(arg1[0].toInt());
        m_fontSize.setHeight(arg1[1].toInt());
        m_spacing = arg1[2].toInt();
    }

    m_weight = arg[2].toInt();

    JsonArray arg3 = arg[3].value<JsonArray>();
    m_color = QColor(arg3[0].toInt(), arg3[1].toInt(), arg3[2].toInt(), arg3[3].toInt());

    return true;
}

void SanSimpleTextFont::paintText(QPainter *const painter, const QRect &pos, const Qt::Alignment &align, const QString &text) const
{
    if (pos.width() <= 0 || pos.height() <= 0 || m_fontSize.width() <= 0 || m_fontSize.height() <= 0) {
        return;
    }

    QSize actualSize = m_fontSize;
    QRect actualRect = pos;
    if ((align & Qt::TextWrapAnywhere) && !m_vertical) {
        SanFreeTypeFont::getInstance()->paintStringMultiLine(painter, text, m_fontFace, m_color, actualSize, m_spacing, m_weight, actualRect, align);
    } else {
        SanFreeTypeFont::getInstance()->paintString(painter, text, m_fontFace, m_color, actualSize, m_spacing, m_weight, actualRect, m_vertical ? Qt::Vertical : Qt::Horizontal,
                                                    align);
    }
}

void SanSimpleTextFont::paintText(QGraphicsPixmapItem *const item, const QRect &pos, const Qt::Alignment &align, const QString &text) const
{
    QPixmap pixmap(pos.size());
    pixmap.fill(Qt::transparent);

    QPainter painter(&pixmap);
    paintText(&painter, QRect(0, 0, pos.width(), pos.height()), align, text);

    item->setPixmap(pixmap);
    item->setPos(pos.x(), pos.y());
}

void SanSimpleTextFont::_initFontFace(const QString &fontName)
{
    QString fontPath(fontName);
    if (fontPath.startsWith("@")) {
        m_vertical = true;
        fontPath.remove(0, 1);
    }

    if (m_fontBank.contains(fontPath)) {
        m_fontFace = m_fontBank[fontPath];
    } else {
        m_fontFace = SanFreeTypeFont::getInstance()->loadFont(fontPath);
        m_fontBank[fontPath] = m_fontFace;
    }
}
