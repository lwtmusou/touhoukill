#include "sanshadowtextfont.h"
#include "sanuiutils.h"

#include <QPixmap>
#include <QPainter>
#include <QGraphicsPixmapItem>

SanShadowTextFont::SanShadowTextFont() : m_shadowRadius(-1),
m_shadowDecadeFactor(1.0), m_shadowColor(Qt::black), m_shadowOffset(QPoint(0, 0))
{
}

SanShadowTextFont::SanShadowTextFont(const QString &fontName)
    : SanSimpleTextFont(fontName),
    m_shadowRadius(-1),
    m_shadowDecadeFactor(1.0),
    m_shadowColor(Qt::black),
    m_shadowOffset(QPoint(0, 0))
{
}

SanShadowTextFont::SanShadowTextFont(const QString &fontName, const QSize &fontSize,
    int shadowRadius, double shadowDecadeFactor, const QColor &shadowColor,
    const QPoint &shadowOffset, const QColor &color, int spacing, int weight)
    : SanSimpleTextFont(fontName, fontSize, color, spacing, weight),
    m_shadowRadius(shadowRadius), m_shadowDecadeFactor(shadowDecadeFactor),
    m_shadowColor(shadowColor), m_shadowOffset(shadowOffset)
{
}

bool SanShadowTextFont::tryParse(const QVariant &arg)
{
    if (!arg.isArray() || arg.size() < 4) {
        return false;
    }

    if (!SanSimpleTextFont::tryParse(arg)) {
        return false;
    }

    if (arg.size() >= 8) {
        m_shadowRadius = arg[4].asInt();
        m_shadowDecadeFactor = arg[5].asDouble();
        JsonUtils::tryParse(arg[6], m_shadowOffset);
        m_shadowColor = QColor(arg[7][0].asInt(), arg[7][1].asInt(), arg[7][2].asInt(), arg[7][3].asInt());
    } else {
        m_shadowRadius = -1;
    }

    return true;
}

void SanShadowTextFont::paintText(QPainter *const painter, const QRect &pos,
    const Qt::Alignment &align, const QString &text) const
{
    QPixmap pixmap;
    if (_paintTextHelper(pos, align, text, pixmap)) {
        painter->drawPixmap(pos.topLeft(), pixmap);
    }
}

void SanShadowTextFont::paintText(QGraphicsPixmapItem *const item, const QRect &pos,
    const Qt::Alignment &align, const QString &text) const
{
    QPixmap pixmap;
    if (_paintTextHelper(pos, align, text, pixmap)) {
        item->setPixmap(pixmap);
        item->setPos(pos.x(), pos.y());
    }
}

bool SanShadowTextFont::_paintTextHelper(const QRect &pos,
    const Qt::Alignment &align, const QString &text,
    QPixmap &pixmap) const
{
    if (pos.width() <= 0 || pos.height() <= 0
        || m_fontSize.width() <= 0 || m_fontSize.height() <= 0) {
        return false;
    }

    QImage image(pos.width(), pos.height(), QImage::Format_ARGB32);
    image.fill(Qt::transparent);

    // @todo: currently, we have not considered m_shadowOffset yet
    QPainter imagePainter(&image);
    SanSimpleTextFont::paintText(&imagePainter, QRect(m_shadowRadius, m_shadowRadius,
        pos.width() - m_shadowRadius * 2, pos.height() - m_shadowRadius * 2), align, text);

    if (m_shadowRadius < 0
        || (m_shadowRadius == 0
        && m_shadowOffset.x() == 0
        && m_shadowOffset.y() == 0)) {
        pixmap = QPixmap::fromImage(image);
        return true;
    }

    QImage shadow = SanUiUtils::produceShadow(image, m_shadowColor,
        m_shadowRadius, m_shadowDecadeFactor);

    pixmap = QPixmap::fromImage(shadow);
    QPainter shadowPainter(&pixmap);
    shadowPainter.drawImage(0, 0, image);

    return true;
}
