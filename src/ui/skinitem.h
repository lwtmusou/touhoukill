#ifndef SKINITEM_H
#define SKINITEM_H

#include <QGraphicsObject>

const QRect SKIN_ITEM_AREA = QRect(4, 4, 93, 172);

class SkinItem : public QGraphicsObject
{
    Q_OBJECT

public:
    SkinItem(const QString &skinName, const QRect &clipRect, int skinIndex, bool used, QGraphicsItem *parent = 0);

    void setUsed(bool used)
    {
        m_used = used;
    }

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);

protected:
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *);

    virtual void mousePressEvent(QGraphicsSceneMouseEvent *)
    {
    }
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *);

private:
    static const QPixmap &getUsedIcon();
    static const QPixmap &getSelectFrameIcon();

private:
    const QPixmap m_skinPixmap;
    const QRect m_clipRect;
    const int m_skinIndex;
    bool m_used;
    bool m_hoverEnter;

signals:
    void clicked(int skinIndex);
};

#endif // SKINITEM_H
