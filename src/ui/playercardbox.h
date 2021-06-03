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

#ifndef PLAYERCARDBOX_H
#define PLAYERCARDBOX_H

#include "card.h"
#include "graphicsbox.h"
#include "player.h"

class ClientPlayer;
class QGraphicsProxyWidget;
class QSanCommandProgressBar;
class CardItem;

class PlayerCardBox : public GraphicsBox
{
    Q_OBJECT

public:
    explicit PlayerCardBox();

    void chooseCard(const QString &reason, const ClientPlayer *player, const QString &flags = QStringLiteral("hej"), bool handcardVisible = false,
                    QSanguosha::HandlingMethod method = QSanguosha::MethodNone, const QList<int> &disabledIds = QList<int>(), bool enableEmptyCard = true);
    void clear();
    QRectF boundingRect() const override;

protected:
    // GraphicsBox interface
    void paintLayout(QPainter *painter) override;

private:
    void paintArea(const QString &name, QPainter *painter);
    int getRowCount(int cardNumber) const;
    void updateNumbers(int cardNumber);
    void arrangeCards(const QList<const Card *> &cards, QPoint topLeft, bool enableEmptyCard = true, const QList<const Card *> &shownCards = QList<const Card *>());

    const ClientPlayer *player;
    QString flags;
    bool handcardVisible;
    QSanguosha::HandlingMethod method;
    QList<int> disabledIds;
    QList<CardItem *> items;

    QGraphicsProxyWidget *progressBarItem;
    QSanCommandProgressBar *progressBar;

    QList<QRect> nameRects;

    int rowCount;
    int intervalsBetweenAreas;
    int intervalsBetweenRows;
    int maxCardsInOneRow;

    static const int maxCardNumberInOneRow;

    static const int verticalBlankWidth;
    static const int placeNameAreaWidth;
    static const int intervalBetweenNameAndCard;
    static const int topBlankWidth;
    static const int bottomBlankWidth;
    static const int intervalBetweenAreas;
    static const int intervalBetweenRows;
    static const int intervalBetweenCards;

public slots:
    void reply();
};

#endif // PLAYERCARDBOX_H
