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

#include "playercardbox.h"
#include "SkinBank.h"
#include "TimedProgressBar.h"
#include "carditem.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "roomscene.h"

#include <QGraphicsProxyWidget>

const int PlayerCardBox::maxCardNumberInOneRow = 10;

const int PlayerCardBox::verticalBlankWidth = 37;
const int PlayerCardBox::placeNameAreaWidth = 15;
const int PlayerCardBox::intervalBetweenNameAndCard = 20;
const int PlayerCardBox::topBlankWidth = 42;
const int PlayerCardBox::bottomBlankWidth = 25;
const int PlayerCardBox::intervalBetweenAreas = 10;
const int PlayerCardBox::intervalBetweenRows = 5;
const int PlayerCardBox::intervalBetweenCards = 3;

PlayerCardBox::PlayerCardBox()
    : player(nullptr)
    , progressBar(nullptr)
    , rowCount(0)
    , intervalsBetweenAreas(-1)
    , intervalsBetweenRows(0)
    , maxCardsInOneRow(0)
{
}

void PlayerCardBox::chooseCard(const QString &reason, const Player *player, const QString &flags, bool handcardVisible, QSanguosha::HandlingMethod method,
                               const QList<int> &disabledIds, bool enableEmptyCard)
{
    nameRects.clear();
    rowCount = 0;
    intervalsBetweenAreas = -1;
    intervalsBetweenRows = 0;
    maxCardsInOneRow = 0;

    this->player = player;
    this->title = tr("%1: please choose %2's card").arg(reason).arg(ClientInstance->getPlayerName(player->objectName()));
    this->flags = flags;
    bool handcard = false;
    bool equip = false;
    bool judging = false;

    if ((flags.contains(QStringLiteral("h")) || flags.contains(QStringLiteral("s"))) && !player->isKongcheng()) {
        updateNumbers(player->getHandcardNum());
        handcard = true;
    }

    if (flags.contains(QStringLiteral("e")) && player->hasEquip()) {
        updateNumbers(player->getEquips().length());
        equip = true;
    }

    if (flags.contains(QStringLiteral("j")) && !player->getJudgingArea().isEmpty()) {
        updateNumbers(player->getJudgingArea().length());
        judging = true;
    }

    int max = maxCardsInOneRow;
    int maxNumber = maxCardNumberInOneRow;
    maxCardsInOneRow = qMin(max, maxNumber);

    prepareGeometryChange();

    moveToCenter();
    show();

    this->handcardVisible = handcardVisible;
    this->method = method;
    this->disabledIds = disabledIds;

    const int startX = verticalBlankWidth + placeNameAreaWidth + intervalBetweenNameAndCard;
    int index = 0;

    if (handcard) {
        QList<const Card *> handcards;
        QList<const Card *> shownHandcards;
        foreach (int id, player->getShownHandcards()) {
            const Card *c = ClientInstance->getCard(id);
            shownHandcards << c;
        }

        if (!handcardVisible && Self != player) {
            foreach (int id, player->getShownHandcards()) {
                const Card *c = ClientInstance->getCard(id);
                handcards << c;
            }
            int hidden = player->getHandcardNum() - handcards.length();
            for (int i = 0; i < hidden; ++i)
                handcards << NULL;
        } else
            handcards = player->getHandcards();

        arrangeCards(handcards, QPoint(startX, nameRects.at(index).y()), enableEmptyCard, shownHandcards);
        ++index;
    }

    if (equip) {
        arrangeCards(player->getEquips(), QPoint(startX, nameRects.at(index).y()));
        ++index;
    }

    if (judging)
        arrangeCards(player->getJudgingArea(), QPoint(startX, nameRects.at(index).y()));

    if (ServerInfo.OperationTimeout != 0) {
        if (progressBar == nullptr) {
            progressBar = new QSanCommandProgressBar();
            progressBar->setMaximumWidth(qMin(boundingRect().width() - 16, (qreal)150));
            progressBar->setMaximumHeight(12);
            progressBar->setTimerEnabled(true);
            progressBarItem = new QGraphicsProxyWidget(this);
            progressBarItem->setWidget(progressBar);
            progressBarItem->setPos(boundingRect().center().x() - progressBarItem->boundingRect().width() / 2, boundingRect().height() - 20);
            connect(progressBar, &QSanCommandProgressBar::timedOut, this, &PlayerCardBox::reply);
        }
        progressBar->setCountdown(QSanProtocol::S_COMMAND_CHOOSE_CARD);
        progressBar->show();
    }
}

QRectF PlayerCardBox::boundingRect() const
{
    if (player == nullptr)
        return QRectF();

    if (rowCount == 0)
        return QRectF();

    const int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int cardHeight = G_COMMON_LAYOUT.m_cardNormalHeight;

    int width = verticalBlankWidth * 2 + placeNameAreaWidth + intervalBetweenNameAndCard;

    if (maxCardsInOneRow > maxCardNumberInOneRow / 2) {
        width += cardWidth * maxCardNumberInOneRow / 2 + intervalBetweenCards * (maxCardNumberInOneRow / 2 - 1);
    } else {
        width += cardWidth * maxCardsInOneRow + intervalBetweenCards * (maxCardsInOneRow - 1);
    }

    int areaInterval = intervalBetweenAreas;
    int height = topBlankWidth + bottomBlankWidth + cardHeight * rowCount + intervalsBetweenAreas * qMax(areaInterval, 0) + intervalsBetweenRows * intervalBetweenRows;

    if (ServerInfo.OperationTimeout != 0)
        height += 12;

    return QRectF(0, 0, width, height);
}

void PlayerCardBox::paintLayout(QPainter *painter)
{
    if (nameRects.isEmpty())
        return;

    foreach (const QRect &rect, nameRects)
        painter->drawRoundedRect(rect, 3, 3);

    // font
    IQSanComponentSkin::QSanSimpleTextFont font;
    JsonArray array;
    array << QStringLiteral("@DroidSansFallback") << 16 << 2;
    JsonArray array4;
    array4 << 228 << 213 << 160 << 255;
    array << QVariant::fromValue(array4);
    font.tryParse(array);

    int index = 0;

    if ((flags.contains(QStringLiteral("h")) || flags.contains(QStringLiteral("s"))) && !player->isKongcheng()) {
        font.paintText(painter, nameRects.at(index), Qt::AlignCenter, tr("Handcard area"));
        ++index;
    }
    if (flags.contains(QStringLiteral("e")) && player->hasEquip()) {
        font.paintText(painter, nameRects.at(index), Qt::AlignCenter, tr("Equip area"));
        ++index;
    }
    if (flags.contains(QStringLiteral("j")) && !player->getJudgingArea().isEmpty()) {
        font.paintText(painter, nameRects.at(index), Qt::AlignCenter, tr("Judging area"));
        ++index;
    }
}

void PlayerCardBox::clear()
{
    if (progressBar != nullptr) {
        progressBar->hide();
        progressBar->deleteLater();
        progressBar = nullptr;

        progressBarItem->deleteLater();
    }

    foreach (CardItem *item, items)
        item->deleteLater();
    items.clear();

    disappear();
}

int PlayerCardBox::getRowCount(int cardNumber) const
{
    return (cardNumber + maxCardNumberInOneRow - 1) / maxCardNumberInOneRow;
}

void PlayerCardBox::updateNumbers(int cardNumber)
{
    ++intervalsBetweenAreas;
    if (cardNumber > maxCardsInOneRow)
        maxCardsInOneRow = cardNumber;

    const int cardHeight = G_COMMON_LAYOUT.m_cardNormalHeight;
    const int y = topBlankWidth + rowCount * cardHeight + intervalsBetweenAreas * intervalBetweenAreas + intervalsBetweenRows * intervalBetweenRows;

    const int count = getRowCount(cardNumber);
    rowCount += count;
    intervalsBetweenRows += count - 1;

    const int height = count * cardHeight + (count - 1) * intervalsBetweenRows;

    nameRects << QRect(verticalBlankWidth, y, placeNameAreaWidth, height);
}

void PlayerCardBox::arrangeCards(const QList<const Card *> &cards, QPoint topLeft, bool enableEmptyCard, const QList<const Card *> &shownCards)
{
    QList<CardItem *> areaItems;
    foreach (const Card *card, cards) {
        CardItem *item = new CardItem(card);
        item->setAutoBack(false);
        item->resetTransform();
        item->setParentItem(this);
        item->setFlag(ItemIsMovable, false);
        if (card != nullptr) {
            item->setEnabled(!disabledIds.contains(card->effectiveID()) && (method != QSanguosha::MethodDiscard || Self->canDiscard(player, card->effectiveID())));
            if (shownCards.contains(card)) {
                item->setFootnote(Sanguosha->translate(QStringLiteral("shown_card")));
                item->showFootnote();
            }
            if (this->player->getJudgingArea().contains(card)) {
                item->setFootnote(Sanguosha->translate(card->faceName()));
                item->showFootnote();
            }
        } else
            item->setEnabled(enableEmptyCard);

        connect(item, &CardItem::clicked, this, &PlayerCardBox::reply);
        item->setAcceptedMouseButtons(Qt::LeftButton); //the source of hegemony has not set LeftButton???
        connect(item, &CardItem::enter_hover, RoomSceneInstance->getDashboard(), &Dashboard::onCardItemHover);
        connect(item, &CardItem::leave_hover, RoomSceneInstance->getDashboard(), &Dashboard::onCardItemLeaveHover);
        items << item;
        areaItems << item;
    }

    int n = items.size();
    if (n == 0)
        return;

    const int rows = (n + maxCardNumberInOneRow - 1) / maxCardNumberInOneRow;
    const int cardWidth = G_COMMON_LAYOUT.m_cardNormalWidth;
    const int cardHeight = G_COMMON_LAYOUT.m_cardNormalHeight;
    const int min = qMin(maxCardsInOneRow, maxCardNumberInOneRow / 2);
    const int maxWidth = min * cardWidth + intervalBetweenCards * (min - 1);
    for (int row = 0; row < rows; ++row) {
        int count = qMin(maxCardNumberInOneRow, areaItems.size());
        double step = 0;
        if (count > 1) {
            step = qMin((double)cardWidth + intervalBetweenCards, (double)(maxWidth - cardWidth) / qMax(count - 1, 0));
        }
        for (int i = 0; i < count; ++i) {
            CardItem *item = areaItems.takeFirst();
            const double x = topLeft.x() + step * i;
            const double y = topLeft.y() + (cardHeight + intervalBetweenRows) * row;
            item->setPos(x, y);
        }
    }
}

void PlayerCardBox::reply()
{
    CardItem *item = qobject_cast<CardItem *>(sender());

    int id = -2;
    if (item != nullptr)
        id = item->getId();

    ClientInstance->onPlayerChooseCard(id);
}
