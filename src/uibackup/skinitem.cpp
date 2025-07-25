#include "skinitem.h"

#include <QPainter>

const char *HEROSKIN_USED_ICON = "image/system/heroskin-used.png";
const char *HEROSKIN_SELECT_FRAME_ICON = "image/system/frame/heroskin-select.png";

SkinItem::SkinItem(const QString &skinName, const QRect &clipRect, int skinIndex, bool used, QGraphicsItem *parent /* = 0*/)
    : QGraphicsObject(parent)
    , m_skinPixmap(skinName)
    , m_clipRect(clipRect)
    , m_skinIndex(skinIndex)
    , m_used(used)
    , m_hoverEnter(false)
{
    setAcceptHoverEvents(true);
    setAcceptedMouseButtons(Qt::LeftButton);
}

QRectF SkinItem::boundingRect() const
{
    return QRectF(getSelectFrameIcon().rect());
}

void SkinItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    static QPixmap tempPix(getSelectFrameIcon().size());
    tempPix.fill(Qt::transparent);

    QPainter tempPainter(&tempPix);
    tempPainter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    QPainterPath roundRectPath;
    roundRectPath.addRoundedRect(SKIN_ITEM_AREA, 8, 8);
    tempPainter.setClipPath(roundRectPath);
    tempPainter.drawPixmap(SKIN_ITEM_AREA, m_skinPixmap, m_clipRect);

    if (m_used) {
        tempPainter.drawPixmap(4, 64, getUsedIcon());
    }

    QPen pen(Qt::black);
    pen.setWidthF(1);
    tempPainter.setPen(pen);
    tempPainter.drawRoundedRect(SKIN_ITEM_AREA, 8, 8);

    if (m_hoverEnter) {
        tempPainter.setClipRect(getSelectFrameIcon().rect());
        tempPainter.drawPixmap(0, 0, getSelectFrameIcon());
    }

    painter->drawPixmap(0, 0, tempPix);
}

void SkinItem::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    if (!m_used) {
        m_hoverEnter = true;
        update();
    }
}

void SkinItem::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    if (!m_used) {
        m_hoverEnter = false;
        update();
    }
}

void SkinItem::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    if (!m_used && isUnderMouse()) {
        m_hoverEnter = false;
        update();

        emit clicked(m_skinIndex);
    }
}

const QPixmap &SkinItem::getUsedIcon()
{
    static const QPixmap usedIcon(HEROSKIN_USED_ICON);
    return usedIcon;
}

const QPixmap &SkinItem::getSelectFrameIcon()
{
    static const QPixmap selectFrameIcon(HEROSKIN_SELECT_FRAME_ICON);
    return selectFrameIcon;
}
