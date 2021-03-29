#ifndef _MAGATAMAS_ITEM_H
#define _MAGATAMAS_ITEM_H

#include "SkinBank.h"
#include <QGraphicsObject>
#include <QPixmap>

class MagatamasBoxItem : public QGraphicsObject
{
    Q_OBJECT

public:
    MagatamasBoxItem();
    explicit MagatamasBoxItem(QGraphicsItem *parent);
    inline int getHp() const
    {
        return m_hp;
    }
    void setHp(int hp, int dying = 1);
    inline int getMaxHp() const
    {
        return m_maxHp;
    }
    void setMaxHp(int maxHp);
    void setOrientation(Qt::Orientation orientation);
    inline Qt::Orientation getOrientation() const
    {
        return m_orientation;
    }
    inline void setBackgroundVisible(bool visible)
    {
        m_showBackground = visible;
    }
    inline bool isBackgroundVisible() const
    {
        return m_showBackground;
    }
    void setAnchor(QPoint anchor, Qt::Alignment align);
    inline void setAnchorEnable(bool enabled)
    {
        anchorEnabled = enabled;
    }
    inline bool isAnchorEnable()
    {
        return anchorEnabled;
    }
    void setIconSize(QSize size);
    inline void setImageArea(const QRect &rect)
    {
        m_imageArea = rect;
    }
    inline QSize getIconSize() const
    {
        return m_iconSize;
    }
    QRectF boundingRect() const override;
    virtual void update();
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

protected:
    void _autoAdjustPos();
    void _updateLayout();
    void _doHpChangeAnimation(int newHp);
    QPoint m_anchor;
    Qt::Alignment m_align;
    bool anchorEnabled;
    int m_hp;
    int m_dyingHp;
    int m_maxHp;
    Qt::Orientation m_orientation;
    bool m_showBackground;
    QSize m_iconSize;
    QRect m_imageArea;
    QPixmap _icons[6];
    QPixmap _dyingIcons[6];
    QPixmap _bgImages[6];
};
#endif
