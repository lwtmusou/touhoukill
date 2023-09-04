#ifndef SGSWINDOW_H_
#define SGSWINDOW_H_

#include "button.h"

#include <QGraphicsObject>
#include <QGraphicsScale>

class Window : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit Window(const QString &title, const QSizeF &size, const QString &path = QString());
    void addContent(const QString &content);
    Button *addCloseButton(const QString &label);
    void shift(int pos_x = 0, int pos_y = 0);
    void keepWhenDisappear();
    void setTitle(const QString &title);

    QRectF boundingRect() const override;

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

public slots:
    void appear();
    void disappear();

private:
    QGraphicsTextItem *titleItem;
    QGraphicsScale *scaleTransform;
    QSizeF size;
    bool keep_when_disappear;
    QImage *outimg;
};

#endif
