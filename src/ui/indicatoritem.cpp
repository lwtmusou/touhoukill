#include "indicatoritem.h"
#include "engine.h"
#include "uiUtils.h"

#include <QGraphicsBlurEffect>
#include <QPainter>
#include <QPauseAnimation>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>

IndicatorItem::IndicatorItem(QPointF start, QPointF real_finish, Player *player)
    : start(start)
    , finish(start)
    , real_finish(real_finish)
{
    color = QSanUiUtils::getKingdomColor(player->kingdom());
    width = player->isLord() ? 4 : 3;
}

void IndicatorItem::doAnimation()
{
    QSequentialAnimationGroup *group = new QSequentialAnimationGroup(this);

    QPropertyAnimation *animation = new QPropertyAnimation(this, "finish");
    animation->setEndValue(real_finish);
    animation->setEasingCurve(QEasingCurve::OutCubic);
    animation->setDuration(500);

    QPropertyAnimation *pause = new QPropertyAnimation(this, "opacity");
    pause->setEndValue(0);
    pause->setEasingCurve(QEasingCurve::InQuart);
    pause->setDuration(600);

    group->addAnimation(animation);
    group->addAnimation(pause);

    group->start(QAbstractAnimation::DeleteWhenStopped);

    connect(group, &QAbstractAnimation::finished, this, &QObject::deleteLater);
}

QPointF IndicatorItem::getFinish() const
{
    return finish;
}

void IndicatorItem::setFinish(QPointF finish)
{
    this->finish = finish;
    update();
}

void IndicatorItem::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    painter->setRenderHint(QPainter::Antialiasing);

    QPen pen(color);
    pen.setWidthF(width);

    int baseX = qMin(start.x(), finish.x());
    int baseY = qMin(start.y(), finish.y());

    QLinearGradient linearGrad(start - QPoint(baseX, baseY), finish - QPoint(baseX, baseY));
    QColor start_color(255, 255, 255, 0);
    linearGrad.setColorAt(0, start_color);
    linearGrad.setColorAt(1, color.lighter());

    QBrush brush(linearGrad);
    pen.setBrush(brush);

    painter->setPen(pen);
    painter->drawLine(mapFromScene(start), mapFromScene(finish));

    QPen pen2(QColor(200, 200, 200, 30));
    pen2.setWidth(6);
    painter->setPen(pen2);
    painter->drawLine(mapFromScene(start), mapFromScene(finish));
}

QRectF IndicatorItem::boundingRect() const
{
    qreal width = qAbs(start.x() - real_finish.x());
    qreal height = qAbs(start.y() - real_finish.y());

    return QRectF(0, 0, width, height).adjusted(-2, -2, 2, 2);
}
