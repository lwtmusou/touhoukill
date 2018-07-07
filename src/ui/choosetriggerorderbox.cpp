/********************************************************************
    Copyright (c) 2013-2015 - Mogara

    This file is part of QSanguosha-Hegemony.

    This game is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 3.0
    of the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    See the LICENSE file for more details.

    Mogara
    *********************************************************************/

#include "choosetriggerorderbox.h"
#include "SkinBank.h"
#include "TimedProgressBar.h"
#include "button.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"

#include <QGraphicsProxyWidget>
#include <QGraphicsSceneMouseEvent>
#include <QPropertyAnimation>

static qreal initialOpacity = 0.8;
static int optionButtonHeight = 40;

const int ChooseTriggerOrderBox::top_dark_bar = 27;
const int ChooseTriggerOrderBox::m_topBlankWidth = 42;
const int ChooseTriggerOrderBox::bottom_blank_width = 25;
const int ChooseTriggerOrderBox::interval = 15;
const int ChooseTriggerOrderBox::m_leftBlankWidth = 37;

SkillInvokeDetailForClient::SkillInvokeDetailForClient()
    : skill(NULL)
    , owner(NULL)
    , invoker(NULL)
    , preferredTarget(NULL)
    , preferredTargetSeat(-1)
{
}

bool SkillInvokeDetailForClient::operator==(const SkillInvokeDetailForClient &arg2) const
{
    return skill == arg2.skill && owner == arg2.owner && invoker == arg2.invoker && preferredTarget == arg2.preferredTarget && preferredTargetSeat == arg2.preferredTargetSeat;
}

bool SkillInvokeDetailForClient::operator==(const QVariantMap &arg2) const
{
    SkillInvokeDetailForClient arg2str;
    arg2str.tryParse(arg2);
    return (*this) == arg2str;
}

bool operator==(const QVariantMap &arg1, const SkillInvokeDetailForClient &arg2)
{
    SkillInvokeDetailForClient arg1str;
    arg1str.tryParse(arg1);
    return arg1str == arg2;
}

bool SkillInvokeDetailForClient::tryParse(const QVariantMap &map)
{
    *this = SkillInvokeDetailForClient();

    if (map.contains("skill"))
        skill = Sanguosha->getSkill(map.value("skill").toString());
    if (skill == NULL)
        return false;

    if (map.contains("invoker"))
        invoker = ClientInstance->getPlayer(map.value("invoker").toString());
    if (invoker == NULL)
        return false;

    if (map.contains("owner"))
        owner = ClientInstance->getPlayer(map.value("owner").toString());
    if (owner == NULL)
        owner = invoker;

    if (map.contains("preferredtarget"))
        preferredTarget = ClientInstance->getPlayer(map.value("preferredtarget").toString());

    if (map.contains("preferredtargetseat"))
        preferredTargetSeat = map.value("preferredtargetseat").toInt();

    return true;
}

bool SkillInvokeDetailForClient::tryParse(const QString &str)
{
    QStringList l = str.split(":");
    skill = Sanguosha->getSkill(l.first());
    if (skill == NULL)
        return false;
    invoker = ClientInstance->getPlayer(l.value(2));
    if (invoker == NULL)
        return false;
    owner = ClientInstance->getPlayer(l.value(1));
    if (owner == NULL)
        owner = invoker;

    if (l.length() > 3) {
        preferredTarget = ClientInstance->getPlayer(l.value(3));
        preferredTargetSeat = l.value(4).toInt();
    }

    return true;
}

QString SkillInvokeDetailForClient::toString() const
{
    QStringList l;
    l << skill->objectName();
    l << owner->objectName();
    l << invoker->objectName();
    if (preferredTarget) {
        l << preferredTarget->objectName();
        l << QString::number(preferredTargetSeat);
    }
    return l.join(":");
}

TriggerOptionButton::TriggerOptionButton(QGraphicsObject *parent, const QVariantMap &skillDetail, int width)
    : QGraphicsObject(parent)
    , times(1)
    , width(width)
{
    detail.tryParse(skillDetail);
    construct();
}

TriggerOptionButton::TriggerOptionButton(QGraphicsObject *parent, const SkillInvokeDetailForClient &skillDetail, int width)
    : QGraphicsObject(parent)
    , detail(skillDetail)
    , times(1)
    , width(width)
{
    construct();
}

void TriggerOptionButton::construct()
{
    setToolTip(detail.skill->getDescription());

    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setOpacity(initialOpacity);
}

QFont TriggerOptionButton::defaultFont()
{
    QFont font = Config.SmallFont;
    font.setPixelSize(Config.TinyFont.pixelSize());
    return font;
}

void TriggerOptionButton::paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *)
{
    painter->setRenderHint(QPainter::HighQualityAntialiasing);
    painter->save();
    painter->setBrush(Qt::black);
    painter->setPen(Sanguosha->getKingdomColor(Self->getGeneral()->getKingdom()));
    QRectF rect = boundingRect();
    painter->drawRoundedRect(rect, 5, 5);
    painter->restore();

    QString generalName;
    if (detail.preferredTarget != NULL)
        generalName = detail.preferredTarget->getGeneralName();
    else
        generalName = detail.owner->getGeneralName();

    QPixmap pixmap = G_ROOM_SKIN.getGeneralPixmap(generalName, QSanRoomSkin::S_GENERAL_ICON_SIZE_TINY, false);

    pixmap = pixmap.scaledToHeight(optionButtonHeight, Qt::SmoothTransformation);
    QRect pixmapRect(QPoint(0, (rect.height() - pixmap.height()) / 2), pixmap.size());
    painter->setBrush(pixmap);
    painter->drawRoundedRect(pixmapRect, 5, 5);

    QRect textArea(optionButtonHeight, 0, width - optionButtonHeight, optionButtonHeight);

    G_COMMON_LAYOUT.optionButtonText.paintText(painter, textArea, Qt::AlignCenter, displayedTextOf(detail, times));
}

QRectF TriggerOptionButton::boundingRect() const
{
    return QRectF(0, 0, width, optionButtonHeight);
}

void TriggerOptionButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
}

void TriggerOptionButton::mouseReleaseEvent(QGraphicsSceneMouseEvent *)
{
    emit clicked();
}

void TriggerOptionButton::hoverEnterEvent(QGraphicsSceneHoverEvent *)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setEndValue(1.0);
    animation->setDuration(100);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    emit hovered(true);
}

void TriggerOptionButton::hoverLeaveEvent(QGraphicsSceneHoverEvent *)
{
    QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
    animation->setEndValue(initialOpacity);
    animation->setDuration(100);
    animation->start(QAbstractAnimation::DeleteWhenStopped);
    emit hovered(false);
}

QString TriggerOptionButton::displayedTextOf(const SkillInvokeDetailForClient &detail, int times)
{
    QString skillName = detail.skill->objectName();
    QString text = Sanguosha->translate(skillName);
    if (detail.preferredTarget) {
        QString targetName = detail.preferredTarget->getGeneralName();
        text = tr("%1 (use upon %2)").arg(text).arg(Sanguosha->translate(targetName));
    }
    if (detail.owner != detail.invoker)
        text = tr("%1 (of %2's)").arg(text).arg(Sanguosha->translate(detail.owner->getGeneralName()));

    if (times > 1)
        text += QString(" * %1").arg(times);

    return text;
}

bool TriggerOptionButton::isPreferentialSkillOf(const TriggerOptionButton *other) const
{
    return detail.skill == other->detail.skill && detail.preferredTargetSeat < other->detail.preferredTargetSeat;
}

void TriggerOptionButton::needDisabled(bool disabled)
{
    if (disabled) {
        QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
        animation->setEndValue(0.2);
        animation->setDuration(100);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    } else {
        QPropertyAnimation *animation = new QPropertyAnimation(this, "opacity");
        animation->setEndValue(initialOpacity);
        animation->setDuration(100);
        animation->start(QAbstractAnimation::DeleteWhenStopped);
    }
}

ChooseTriggerOrderBox::ChooseTriggerOrderBox()
    : optional(true)
    , m_minimumWidth(0)
    , cancel(new Button(tr("cancel"), 0.6))
    , progressBar(NULL)
{
    cancel->hide();
    cancel->setParentItem(this);
    cancel->setObjectName("cancel");
    connect(cancel, &Button::clicked, this, &ChooseTriggerOrderBox::reply);
}

void ChooseTriggerOrderBox::storeMinimumWidth()
{
    int width = 0;
    static QFontMetrics fontMetrics(TriggerOptionButton::defaultFont());
    foreach (const QVariant &option, options) {
        SkillInvokeDetailForClient skillDetail;
        skillDetail.tryParse(option.toMap());

        const int w = fontMetrics.width(TriggerOptionButton::displayedTextOf(skillDetail, 2));
        if (w > width)
            width = w;
    }
    m_minimumWidth = width + optionButtonHeight + 20;
}

QRectF ChooseTriggerOrderBox::boundingRect() const
{
    int width = m_minimumWidth + m_leftBlankWidth * 2;

    int height = m_topBlankWidth + options.size() * optionButtonHeight + (options.size() - 1) * interval + bottom_blank_width;

    if (ServerInfo.OperationTimeout != 0)
        height += 12;

    if (optional)
        height += cancel->boundingRect().height() + interval;

    return QRectF(0, 0, width, height);
}

void ChooseTriggerOrderBox::chooseOption(const QVariantList &options, bool optional)
{
    this->options = options;
    this->optional = optional;
    title = tr("Please Select Trigger Order");

    storeMinimumWidth();

    prepareGeometryChange();

    foreach (const QVariant &option, options) {
        QVariantMap map = option.toMap();
        SkillInvokeDetailForClient detail;
        detail.tryParse(map);

        bool duplicate = false;
        foreach (TriggerOptionButton *otherButton, optionButtons) {
            if (otherButton->detail == detail) {
                ++otherButton->times;
                duplicate = true;
                break;
            }
        }
        if (duplicate)
            continue;

        TriggerOptionButton *button = new TriggerOptionButton(this, detail, m_minimumWidth);
        button->setObjectName(detail.toString());
        foreach (TriggerOptionButton *otherButton, optionButtons) {
            if (otherButton->isPreferentialSkillOf(button))
                connect(button, &TriggerOptionButton::hovered, otherButton, &TriggerOptionButton::needDisabled);
            if (button->isPreferentialSkillOf(otherButton))
                connect(otherButton, &TriggerOptionButton::hovered, button, &TriggerOptionButton::needDisabled);
        }
        optionButtons << button;
    }

    moveToCenter();
    show();

    int y = m_topBlankWidth;
    foreach (TriggerOptionButton *button, optionButtons) {
        QPointF pos;
        pos.setX(m_leftBlankWidth);
        pos.setY(y);

        button->setPos(pos);
        connect(button, &TriggerOptionButton::clicked, this, &ChooseTriggerOrderBox::reply);
        y += button->boundingRect().height() + interval;
    }

    if (optional) {
        cancel->setPos((boundingRect().width() - cancel->boundingRect().width()) / 2, y + interval);
        cancel->show();
    }

    if (ServerInfo.OperationTimeout != 0) {
        if (!progressBar) {
            progressBar = new QSanCommandProgressBar;
            progressBar->setMaximumWidth(boundingRect().width() - 16);
            progressBar->setMaximumHeight(12);
            progressBar->setTimerEnabled(true);
            progress_bar_item = new QGraphicsProxyWidget(this);
            progress_bar_item->setWidget(progressBar);
            progress_bar_item->setPos(boundingRect().center().x() - progress_bar_item->boundingRect().width() / 2, boundingRect().height() - 20);
            connect(progressBar, &QSanCommandProgressBar::timedOut, this, &ChooseTriggerOrderBox::reply);
        }
        progressBar->setCountdown(QSanProtocol::S_COMMAND_TRIGGER_ORDER);
        progressBar->show();
    }
}

void ChooseTriggerOrderBox::clear()
{
    if (progressBar != NULL) {
        progressBar->hide();
        progressBar->deleteLater();
        progressBar = NULL;
    }

    foreach (TriggerOptionButton *button, optionButtons)
        button->deleteLater();

    optionButtons.clear();

    cancel->hide();

    disappear();
}

void ChooseTriggerOrderBox::reply()
{
    QString choice;
    if (sender())
        choice = sender()->objectName();

    if (choice.isEmpty()) {
        if (optional)
            choice = "cancel";
        else {
            QVariantMap m = options.first().toMap();
            SkillInvokeDetailForClient detail;
            detail.tryParse(m);
            choice = detail.toString();
        }
    }
    ClientInstance->onPlayerChooseTriggerOrder(choice);
}
