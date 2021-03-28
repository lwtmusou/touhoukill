#include "magatamasItem.h"
#include "SkinBank.h"

#include <QParallelAnimationGroup>
#include <QPropertyAnimation>

MagatamasBoxItem::MagatamasBoxItem()
    : QGraphicsObject(nullptr)
{
    m_hp = 0;
    m_dyingHp = 1;
    m_maxHp = 0;
}

MagatamasBoxItem::MagatamasBoxItem(QGraphicsItem *parent)
    : QGraphicsObject(parent)
{
    m_hp = 0;
    m_dyingHp = 1;
    m_maxHp = 0;
}

void MagatamasBoxItem::setOrientation(Qt::Orientation orientation)
{
    m_orientation = orientation;
    _updateLayout();
}

void MagatamasBoxItem::_updateLayout()
{
    int xStep = 0, yStep = 0;
    if (m_orientation == Qt::Horizontal) {
        xStep = m_iconSize.width();
        yStep = 0;
    } else {
        xStep = 0;
        yStep = m_iconSize.height();
    }

    for (int i = 0; i < 6; i++) {
        _icons[i] = G_ROOM_SKIN.getPixmap(QString(QSanRoomSkin::S_SKIN_KEY_MAGATAMAS).arg(QString::number(i)), QString(), true)
                        .scaled(m_iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        _dyingIcons[i] = G_ROOM_SKIN.getPixmap(QString(QSanRoomSkin::S_SKIN_KEY_MAGATAMAS_DYINGLINE).arg(QString::number(i)), QString(), true)
                             .scaled(m_iconSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }

    for (int i = 1; i < 6; i++) {
        QSize bgSize;
        if (m_orientation == Qt::Horizontal) {
            bgSize.setWidth((xStep + 1) * i);
            bgSize.setHeight(m_iconSize.height());
        } else {
            bgSize.setWidth((yStep + 1) * i);
            bgSize.setHeight(m_iconSize.width());
        }
        _bgImages[i]
            = G_ROOM_SKIN.getPixmap(QString(QSanRoomSkin::S_SKIN_KEY_MAGATAMAS_BG).arg(QString::number(i))).scaled(bgSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    }
}

void MagatamasBoxItem::setIconSize(QSize size)
{
    m_iconSize = size;
    _updateLayout();
}

QRectF MagatamasBoxItem::boundingRect() const
{
    int buckets = qMin(m_maxHp, 5) + G_COMMON_LAYOUT.m_hpExtraSpaceHolder;
    if (m_orientation == Qt::Horizontal)
        return QRectF(0, 0, buckets * m_iconSize.width(), m_iconSize.height());
    else
        return QRectF(0, 0, m_iconSize.width(), buckets * m_iconSize.height());
}

void MagatamasBoxItem::setHp(int hp, int dying)
{
    _doHpChangeAnimation(hp);
    m_hp = hp;
    m_dyingHp = dying;
    update();
}

void MagatamasBoxItem::setAnchor(QPoint anchor, Qt::Alignment align)
{
    m_anchor = anchor;
    m_align = align;
}

void MagatamasBoxItem::setMaxHp(int maxHp)
{
    m_maxHp = maxHp;
    _autoAdjustPos();
}

void MagatamasBoxItem::_autoAdjustPos()
{
    if (!anchorEnabled)
        return;
    QRectF rect = boundingRect();
    Qt::Alignment hAlign = m_align & Qt::AlignHorizontal_Mask;
    if (hAlign == Qt::AlignRight)
        setX(m_anchor.x() - rect.width());
    else if (hAlign == Qt::AlignHCenter)
        setX(m_anchor.x() - rect.width() / 2);
    else
        setX(m_anchor.x());
    Qt::Alignment vAlign = m_align & Qt::AlignVertical_Mask;
    if (vAlign == Qt::AlignBottom)
        setY(m_anchor.y() - rect.height());
    else if (vAlign == Qt::AlignVCenter)
        setY(m_anchor.y() - rect.height() / 2);
    else
        setY(m_anchor.y());
}

void MagatamasBoxItem::update()
{
    _updateLayout();
    _autoAdjustPos();
    QGraphicsItem::update();
}

#include "sprite.h"
void MagatamasBoxItem::_doHpChangeAnimation(int newHp)
{
    if (newHp >= m_hp)
        return;

    int width = m_imageArea.width();
    int height = m_imageArea.height();
    int xStep = 0, yStep = 0;
    if (m_orientation == Qt::Horizontal) {
        xStep = width;
        yStep = 0;
    } else {
        xStep = 0;
        yStep = height;
    }

    int mHp = m_hp;
    if (m_hp < 0) {
        newHp -= m_hp;
        mHp = 0;
    }
    for (int i = qMax(newHp, mHp - 10); i < mHp; i++) {
        Sprite *aniMaga = new Sprite;
        aniMaga->setPixmap(_icons[qBound(0, i, 5)]);
        aniMaga->setParentItem(this);
        aniMaga->setOffset(QPoint(-(width - m_imageArea.left()) / 2, -(height - m_imageArea.top()) / 2));

        int pos = m_maxHp > 5 ? 0 : i;
        aniMaga->setPos(QPoint(xStep * pos - aniMaga->offset().x(), yStep * pos - aniMaga->offset().y()));

        QPropertyAnimation *fade = new QPropertyAnimation(aniMaga, "opacity");
        fade->setEndValue(0);
        fade->setDuration(500);
        QPropertyAnimation *grow = new QPropertyAnimation(aniMaga, "scale");
        grow->setEndValue(4);
        grow->setDuration(500);

        connect(fade, SIGNAL(finished()), aniMaga, SLOT(deleteLater()));

        QParallelAnimationGroup *group = new QParallelAnimationGroup;
        group->addAnimation(fade);
        group->addAnimation(grow);

        group->start(QAbstractAnimation::DeleteWhenStopped);

        aniMaga->show();
    }
}

void MagatamasBoxItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    if (m_maxHp <= 0)
        return;
    int imageIndex = qBound(0, m_hp, 5);
    if (m_hp == m_maxHp)
        imageIndex = 5;

    int xStep = 0, yStep = 0;
    if (m_orientation == Qt::Horizontal) {
        xStep = m_iconSize.width();
        yStep = 0;
    } else {
        xStep = 0;
        yStep = m_iconSize.height();
    }

    if (m_showBackground) {
        if (m_orientation == Qt::Vertical) {
            painter->save();
            painter->translate(m_iconSize.width(), 0);
            painter->rotate(90);
        }
        painter->drawPixmap(0, 0, _bgImages[qMin(m_maxHp, 5)]);
        if (m_orientation == Qt::Vertical)
            painter->restore();
    }

    if (m_maxHp <= 5) {
        int i = 0;

        //up-down
        int lostHp = qMin(m_maxHp - m_hp, m_maxHp);
        for (i = 0; i < lostHp; ++i) {
            QRect rect(xStep * i, yStep * i, m_imageArea.width(), m_imageArea.height());
            rect.translate(m_imageArea.topLeft());
            if ((m_maxHp - i) < m_dyingHp)
                painter->drawPixmap(rect, _dyingIcons[0]);
            else
                painter->drawPixmap(rect, _icons[0]);
        }
        for (; i < m_maxHp; ++i) {
            QRect rect(xStep * i, yStep * i, m_imageArea.width(), m_imageArea.height());
            rect.translate(m_imageArea.topLeft());
            if ((m_maxHp - i) < m_dyingHp)
                painter->drawPixmap(rect, _dyingIcons[imageIndex]);
            else
                painter->drawPixmap(rect, _icons[imageIndex]);
        }

    } else {
        painter->drawPixmap(m_imageArea, _icons[imageIndex]);
        QRect rect(xStep, yStep, m_imageArea.width(), m_imageArea.height());
        rect.translate(m_imageArea.topLeft());
        if (m_orientation == Qt::Horizontal)
            rect.translate(xStep * 0.5, yStep * 0.5);
        G_COMMON_LAYOUT.m_hpFont[imageIndex].paintText(painter, rect, Qt::AlignCenter, QString::number(m_hp));
        rect.translate(xStep, yStep);
        G_COMMON_LAYOUT.m_hpFont[imageIndex].paintText(painter, rect, Qt::AlignCenter, "/");
        rect.translate(xStep, yStep);
        G_COMMON_LAYOUT.m_hpFont[imageIndex].paintText(painter, rect, Qt::AlignCenter, QString::number(m_maxHp));
        if (m_dyingHp > 1) {
            rect.translate(xStep, yStep);
            QString dyingLine = QString("%1 %2 %3").arg(QString("(")).arg(QString::number(m_dyingHp - 1)).arg(QString(")"));
            G_COMMON_LAYOUT.m_hpFont[imageIndex].paintText(painter, rect, Qt::AlignLeft, dyingLine);
        }
    }
}
