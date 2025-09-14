#ifndef LIGHTBOXANIMATION_H
#define LIGHTBOXANIMATION_H

#include <QBrush>
#include <QGraphicsObject>
#include <QGraphicsRectItem>

class QSanSelectableItem;
class QGraphicsTextItem;

class RectObject : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit RectObject(const QBrush &brush = QBrush(), QGraphicsItem *parent = nullptr);
    explicit RectObject(const QRectF &rect, const QBrush &brush = QBrush(), QGraphicsItem *parent = nullptr);
    explicit RectObject(qreal x, qreal y, qreal w, qreal h, const QBrush &brush = QBrush(), QGraphicsItem *parent = nullptr);

protected:
    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/) override;

private:
    QRectF m_boundingRect;
    QBrush m_brush;

public slots:
    void show();
    void hide();
};

class LightboxAnimation : public QGraphicsObject
{
    Q_OBJECT

public:
    LightboxAnimation(const QString &general_name, const QString &skill_name, const QRectF &rect);

protected:
    QRectF boundingRect() const override;

private:
    void paint(QPainter * /*painter*/, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/) override
    {
    }

    QRectF rect;

    RectObject *background;
    QSanSelectableItem *generalPixmap;
    RectObject *flick;
    QGraphicsTextItem *skillName;

    QString general_name;
    QString skill_name;

signals:
    void finished();
};

#endif
