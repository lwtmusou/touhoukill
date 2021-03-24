#include "lightboxanimation.h"
#include "QSanSelectableItem.h"
#include "SkinBank.h"
#include "engine.h"

#include <QBrush>
#include <QGraphicsTextItem>
#include <QParallelAnimationGroup>
#include <QPauseAnimation>
#include <QPen>
#include <QPropertyAnimation>
#include <QSequentialAnimationGroup>

RectObject::RectObject(const QBrush &brush /*= QBrush()*/, QGraphicsItem *parent /*= NULL*/)
    : QGraphicsObject(parent)
    , m_brush(brush)
{
}

RectObject::RectObject(const QRectF &rect, const QBrush &brush /*= QBrush()*/, QGraphicsItem *parent /*= NULL*/)
    : QGraphicsObject(parent)
    , m_boundingRect(rect)
    , m_brush(brush)
{
}

RectObject::RectObject(qreal x, qreal y, qreal w, qreal h, const QBrush &brush /*= QBrush()*/, QGraphicsItem *parent /*= NULL*/)
    : QGraphicsObject(parent)
    , m_boundingRect(x, y, w, h)
    , m_brush(brush)
{
}

QRectF RectObject::boundingRect() const
{
    return m_boundingRect;
}

void RectObject::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setPen(Qt::NoPen);
    painter->setBrush(m_brush);
    painter->drawRect(m_boundingRect);
}

void RectObject::show()
{
    QGraphicsObject::show();
}

void RectObject::hide()
{
    QGraphicsObject::hide();
}

LightboxAnimation::LightboxAnimation(const QString &general_name, const QString &skill_name, const QRectF &rect)
    : rect(rect)
    , background(nullptr)
    , generalPixmap(nullptr)
    , flick(nullptr)
    , general_name(general_name)
    , skill_name(skill_name)
{
    hide();

    background = new RectObject(rect, Qt::black, this);
    background->setPos(0, 0);
    background->setOpacity(0);
    background->setZValue(-1);

    generalPixmap = new QSanSelectableItem;
    generalPixmap->setParentItem(this);
    generalPixmap->load("image/animate/skill_bg.png", QSize(1000, 550), false);
    generalPixmap->setTransformOriginPoint(generalPixmap->boundingRect().width() / 2, generalPixmap->boundingRect().height() / 2);
    generalPixmap->setPixmap(G_ROOM_SKIN.getPixmap(G_ROOM_SKIN.S_SKIN_KEY_LIGHTBOX, general_name, true, false));
    generalPixmap->setScale(0.3);
    generalPixmap->setPos(-generalPixmap->boundingRect().width(), rect.height() / 2.5 - generalPixmap->boundingRect().height() / 2);
    generalPixmap->setZValue(0);

    flick = new RectObject(rect, Qt::white, this);
    flick->setPos(0, 0);
    flick->hide();
    flick->setZValue(2);

    skillName = new QGraphicsTextItem(Sanguosha->translate(skill_name), this);
    skillName->setScale(10);
    QFont font("LiSu");
    font.setPixelSize(90);
    skillName->setFont(font);
    skillName->setDefaultTextColor(Qt::white);
    skillName->setOpacity(0);
    skillName->setZValue(1);
    skillName->setPos(rect.width() / 2, rect.height() / 2);

    show();

    QPropertyAnimation *step1_1 = new QPropertyAnimation(background, "opacity");
    step1_1->setStartValue(0);
    step1_1->setEndValue(0.7);
    step1_1->setDuration(880);
    step1_1->setEasingCurve(QEasingCurve::OutQuad);

    QPropertyAnimation *step1_2 = new QPropertyAnimation(generalPixmap, "x");
    step1_2->setStartValue(-generalPixmap->boundingRect().width());
    step1_2->setEndValue(boundingRect().width() / 2.3 - generalPixmap->boundingRect().width() / 2);
    step1_2->setDuration(600);

    QParallelAnimationGroup *step1 = new QParallelAnimationGroup;
    step1->addAnimation(step1_1);
    step1->addAnimation(step1_2);
    connect(step1, &QParallelAnimationGroup::finished, flick, &RectObject::show);

    QPauseAnimation *step2 = new QPauseAnimation(20);
    connect(step2, &QPauseAnimation::finished, flick, &RectObject::hide);

    QPauseAnimation *step3 = new QPauseAnimation(80);
    connect(step3, &QPauseAnimation::finished, flick, &RectObject::show);

    QPauseAnimation *step4 = new QPauseAnimation(20);
    connect(step4, &QPauseAnimation::finished, flick, &RectObject::hide);

    QPropertyAnimation *step5 = new QPropertyAnimation(generalPixmap, "scale");
    QEasingCurve ec = QEasingCurve::OutBack;
    ec.setOvershoot(6.252);
    step5->setEasingCurve(ec);
    step5->setEndValue(1);
    step5->setDuration(800);

    QPropertyAnimation *step6_1 = new QPropertyAnimation(skillName, "scale");
    step6_1->setStartValue(10);
    step6_1->setEndValue(1);
    step6_1->setDuration(800);

    QPropertyAnimation *step6_2 = new QPropertyAnimation(skillName, "opacity");
    step6_2->setStartValue(0);
    step6_2->setEndValue(1);
    step6_2->setDuration(800);

    QParallelAnimationGroup *step6 = new QParallelAnimationGroup;
    step6->addAnimation(step6_1);
    step6->addAnimation(step6_2);

    QPauseAnimation *step7 = new QPauseAnimation(1700);

    QSequentialAnimationGroup *animation = new QSequentialAnimationGroup;
    animation->addAnimation(step1);
    animation->addAnimation(step2);
    animation->addAnimation(step3);
    animation->addAnimation(step4);
    animation->addAnimation(step5);
    animation->addAnimation(step6);
    animation->addAnimation(step7);

    connect(animation, &QSequentialAnimationGroup::finished, this, &LightboxAnimation::finished);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}

QRectF LightboxAnimation::boundingRect() const
{
    return rect;
}
