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

#ifndef GRAPHICSBOX_H
#define GRAPHICSBOX_H

#include <QGraphicsObject>

class GraphicsBox : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit GraphicsBox(const QString &title = QString());
    ~GraphicsBox() override;

    static void paintGraphicsBoxStyle(QPainter *painter, const QString &title, const QRectF &rect);
    static void stylize(QGraphicsObject *target);
    static void moveToCenter(QGraphicsObject *target);

protected:
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;

    QRectF boundingRect() const override = 0;

    virtual void paintLayout(QPainter *painter)
    {
        Q_UNUSED(painter)
    }

    void moveToCenter();
    void disappear();

    QString title;
};

#endif // GRAPHICSBOX_H
