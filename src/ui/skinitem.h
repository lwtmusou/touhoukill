#ifndef SKINITEM_H
#define SKINITEM_H

#include <QGraphicsObject>

const QRect SKIN_ITEM_AREA = QRect(4, 4, 93, 172);

class SkinItem : public QGraphicsObject
{
    Q_OBJECT

public:
    SkinItem(const QString &skinName, QRect clipRect, int skinIndex, bool used, QGraphicsItem *parent = nullptr);

    void setUsed(bool used)
    {
        m_used = used;
    }

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;

protected:
    void hoverEnterEvent(QGraphicsSceneHoverEvent *) override;
    void hoverLeaveEvent(QGraphicsSceneHoverEvent *) override;

    void mousePressEvent(QGraphicsSceneMouseEvent *) override
    {
    }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *) override;

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
