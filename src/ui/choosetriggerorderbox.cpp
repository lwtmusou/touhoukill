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
#include "engine.h"
#include "button.h"
#include "skinbank.h"
#include "client.h"
#include "clientplayer.h"
#include "timedprogressbar.h"

#include <QGraphicsSceneMouseEvent>
#include <QPropertyAnimation>
#include <QGraphicsProxyWidget>

static qreal initialOpacity = 0.8;
static int optionButtonHeight = 40;

const int ChooseTriggerOrderBox::top_dark_bar = 27;
const int ChooseTriggerOrderBox::m_topBlankWidth = 42;
const int ChooseTriggerOrderBox::bottom_blank_width = 25;
const int ChooseTriggerOrderBox::interval = 15;
const int ChooseTriggerOrderBox::m_leftBlankWidth = 37;

TriggerOptionButton::TriggerOptionButton(QGraphicsObject *parent, const QString &player, const QString &skillStr, const int width)
    : QGraphicsObject(parent),
    m_skillStr(skillStr), m_text(displayedTextOf(skillStr)),
    playerName(player), width(width)
{
    QString realSkill = skillStr;
    if (realSkill.contains("*")) {
        realSkill = skillStr.split("*").first();
    }

    if (realSkill.contains("'")) // "sgs1'songwei"
        realSkill = realSkill.split("'").last();

    const Skill *skill = Sanguosha->getSkill(realSkill);
    if (skill)
        setToolTip(skill->getDescription());

    setAcceptedMouseButtons(Qt::LeftButton);
    setAcceptHoverEvents(true);
    setOpacity(initialOpacity);
}

QString TriggerOptionButton::getGeneralNameBySkill() const
{
    QString generalName;
    const ClientPlayer *player = ClientInstance->getPlayer(playerName);
    QString skillName = m_skillStr;
    if (m_skillStr.contains("*"))
        skillName = m_skillStr.split("*").first();
    QString realSkillName = skillName;
    if (realSkillName.contains("'")) // "sgs1'songwei"
        realSkillName = realSkillName.split("'").last();

    generalName = player->getGeneralName();
    return generalName;
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

    const QString generalName = getGeneralNameBySkill();

    QPixmap pixmap = G_ROOM_SKIN.getGeneralPixmap(generalName, QSanRoomSkin::S_GENERAL_ICON_SIZE_TINY);

    pixmap = pixmap.scaledToHeight(optionButtonHeight, Qt::SmoothTransformation);
    QRect pixmapRect(QPoint(0, (rect.height() - pixmap.height()) / 2), pixmap.size());
    painter->setBrush(pixmap);
    painter->drawRoundedRect(pixmapRect, 5, 5);

    QRect textArea(optionButtonHeight, 0, width - optionButtonHeight,
        optionButtonHeight);

    G_COMMON_LAYOUT.optionButtonText.paintText(painter, textArea,
        Qt::AlignCenter,
        m_text);
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

QString TriggerOptionButton::displayedTextOf(const QString &str)
{
    int time = 1;
    QString skillName = str;
    if (str.contains("*")) {
        time = str.split("*").last().toInt();
        skillName = str.split("*").first();
    }
    QString text = Sanguosha->translate(skillName);
    if (skillName.contains("->")) { // "tieqi->sgs4&1"
        QString realSkill = skillName.split("->").first(); // "tieqi"
        QString targetObj = skillName.split("->").last().split("&").first(); // "sgs4"
        QString targetName = ClientInstance->getPlayer(targetObj)->getGeneralName();
        text = tr("%1 (use upon %2)").arg(Sanguosha->translate(realSkill))
            .arg(Sanguosha->translate(targetName));
    }
    if (skillName.contains("'")) {// "sgs1'songwei"
        QString targetObj = skillName.split("'").first(); // "sgs1'
        QString realSkill = skillName.split("'").last(); // "songwei'
        text = tr("%1").arg(Sanguosha->translate(realSkill));
    }
    if (time > 1)
        text += QString(" %1 %2").arg(tr("*")).arg(time);

    return text;
}

ChooseTriggerOrderBox::ChooseTriggerOrderBox()
    : optional(true), m_minimumWidth(0),
    cancel(new Button(tr("cancel"), 0.6)), progressBar(NULL)
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
    foreach (const QString &option, options) {
        const QString skill = option.split(":").last();

        const int w = fontMetrics.width(TriggerOptionButton::displayedTextOf(skill));
        if (w > width)
            width = w;
    }
    m_minimumWidth = width + optionButtonHeight + 20;
}

QRectF ChooseTriggerOrderBox::boundingRect() const
{
    int width = m_minimumWidth + m_leftBlankWidth * 2;

    int height = m_topBlankWidth
        + options.size() * optionButtonHeight
        + (options.size() - 1) * interval
        + bottom_blank_width;

    if (ServerInfo.OperationTimeout != 0)
        height += 12;

    if (optional)
        height += cancel->boundingRect().height() + interval;

    return QRectF(0, 0, width, height);
}

void ChooseTriggerOrderBox::chooseOption(const QString &reason, const QStringList &options, const bool optional)
{
    this->options = options;
    this->optional = optional;
    title = Sanguosha->translate(reason);

    storeMinimumWidth();

    prepareGeometryChange();

    int width = 0;

    width = m_minimumWidth;

    foreach (const QString &option, options) {
        QStringList pair = option.split(":");
        TriggerOptionButton *button = new TriggerOptionButton(this, pair.first(), pair.last(), width);
        button->setObjectName(option);
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

    foreach(TriggerOptionButton *button, optionButtons)
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
        else
            choice = options.first();
    }
    ClientInstance->onPlayerChooseTriggerOrder(choice);
}
