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
    explicit BubbleChatBox(const QRect &area, QGraphicsItem *parent = nullptr);
    ~BubbleChatBox() override;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *) override;
    QPainterPath shape() const override;

    void setText(const QString &text);
    void setArea(const QRect &newArea);

protected:
    QVariant itemChange(GraphicsItemChange change, const QVariant &value) override;

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
