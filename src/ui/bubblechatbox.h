#ifndef _BUBBLECHATBOX_H
#define _BUBBLECHATBOX_H

#include <QGraphicsObject>
#include <QTimer>

class BubbleChatLabel;
class QPropertyAnimation;

class BubbleChatBox : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit BubbleChatBox(const QRect &area, QGraphicsItem *parent = 0);
    ~BubbleChatBox();

    virtual QRectF boundingRect() const;
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
    virtual QPainterPath shape() const;

    void setText(const QString &text);
    void setArea(const QRect &newArea);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value);

private:
    void updatePos();

    QPixmap m_backgroundPixmap;
    QRectF m_rect;
    QRect m_area;
    QTimer m_timer;
    BubbleChatLabel *m_chatLabel;
    QPropertyAnimation *m_appearAndDisappear;

private slots:
    void clear();
};

#endif
