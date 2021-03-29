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

#ifndef _HEGEMONY_ROLE_COMBO_BOX_H
#define _HEGEMONY_ROLE_COMBO_BOX_H

#include <QGraphicsObject>
#include <QGraphicsSceneEvent>
#include <QPainter>

class HegemonyRoleComboBox : public QGraphicsObject
{
    Q_OBJECT

public:
    explicit HegemonyRoleComboBox(QGraphicsItem *photo, bool circle = false);
    static const int COMPACT_BORDER_WIDTH = 1;
    static const int COMPACT_ITEM_LENGTH = 10;

    QRectF boundingRect() const override;
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override;
    inline bool isExpanding() const
    {
        return expanding;
    };

private:
    bool circle;
    bool expanding;
    QMap<QString, bool> kingdoms_excluded;
    QString fixed_role;

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;

public slots:
    void fix(const QString &role);
    void mouseClickedOutside();
};

#endif
