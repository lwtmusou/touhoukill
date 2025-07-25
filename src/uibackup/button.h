#ifndef _BUTTON_H
#define _BUTTON_H

#include "settings.h"

#include <QFont>
#include <QFontMetrics>
#include <QGraphicsObject>

class Button : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit Button(const QString &label, qreal scale = 1.0);
    explicit Button(const QString &label, const QSizeF &size);
    void setMute(bool mute);
    void setFont(const QFont &font);

    QRectF boundingRect() const override;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    void hoverEnterEvent(QGraphicsSceneHoverEvent *event) override;
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

    void timerEvent(QTimerEvent *) override;

private:
    QString label;
    QSizeF size;
    bool mute;
    QFont font;
    QImage *outimg;
    QPixmap *title;
    QGraphicsPixmapItem *title_item;
    int glow;
    int timer_id;

    void init();

signals:
    void clicked();
};

#endif
