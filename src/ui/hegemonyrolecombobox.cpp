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

#include "hegemonyrolecombobox.h"
#include "SkinBank.h"
#include "engine.h"
#include "roomscene.h"

void HegemonyRoleComboBox::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (!fixed_role.isEmpty() || circle)
        return;
    QPoint point = QPoint(event->pos().x(), event->pos().y());
    ;
    if (expanding && !boundingRect().contains(point)) {
        expanding = false;
        update();
        return;
    } else if (!expanding) {
        expanding = true;
        update();
        return;
    }
    QStringList kingdoms = Sanguosha->hegemonyKingdoms();
    kingdoms.removeAll(QStringLiteral("god"));
    foreach (const QString &kingdom, kingdoms) {
        if (G_COMMON_LAYOUT.m_rolesRect.value(kingdom, QRect()).contains(point)) {
            kingdoms_excluded[kingdom] = !kingdoms_excluded.value(kingdom);
            break;
        }
    }
    update();
}

void HegemonyRoleComboBox::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    /*
      --------------------
      --------------------
      ||       ||       ||
      ||  WEI  ||  QUN  ||
      ||       ||       ||
      --------------------
      --------------------
      ||       ||       ||
      ||  SHU  ||  WU   ||
      ||       ||       ||
      --------------------
      --------------------
      */
    double scale = G_ROOM_LAYOUT.scale;
    if (!fixed_role.isEmpty()) {
        QPixmap pix;
        pix.load(QStringLiteral("image/system/roles/%1.png").arg(fixed_role));
        painter->drawPixmap(0, 0, (int)(pix.width() * scale), (int)(pix.height() * scale), pix);
        return;
    }
    QStringList kingdoms = Sanguosha->hegemonyKingdoms();
    kingdoms.removeAll(QStringLiteral("god"));

    if (!expanding) {
        if (circle) {
            QPixmap pix;
            pix.load(QStringLiteral("image/system/roles/unknown.png"));
            painter->drawPixmap(1, 0, (int)(28 * scale), (int)(28 * scale), pix);
        } else {
            QColor grey = G_COMMON_LAYOUT.m_roleDarkColor;
            QPen pen(Qt::black);
            pen.setWidth(1);
            painter->setPen(pen);

            int index = 0;
            foreach (const QString &kingdom, kingdoms) {
                painter->setBrush(QBrush(kingdoms_excluded.value(kingdom) ? grey : G_COMMON_LAYOUT.m_rolesColor.value(kingdom)));
                painter->drawRect(COMPACT_BORDER_WIDTH + ((index % 2) != 0 ? COMPACT_BORDER_WIDTH + COMPACT_ITEM_LENGTH : 0),
                                  COMPACT_BORDER_WIDTH + (COMPACT_BORDER_WIDTH + COMPACT_ITEM_LENGTH) * (index / 2), COMPACT_ITEM_LENGTH, COMPACT_ITEM_LENGTH);
                ++index;
            }
        }
    } else {
        QPixmap pix = G_ROOM_SKIN.getPixmap(QString::fromUtf8(QSanRoomSkin::S_SKIN_KEY_EXPANDING_ROLE_BOX));
        painter->drawPixmap(0, 0, (int)(pix.width() * scale), (int)(pix.height() * scale), pix);
        foreach (const QString &kingdom, kingdoms) {
            if (kingdoms_excluded.value(kingdom))
                painter->drawPixmap(G_COMMON_LAYOUT.m_rolesRect.value(kingdom), G_ROOM_SKIN.getPixmap(QString::fromUtf8(QSanRoomSkin::S_SKIN_KEY_ROLE_BOX_KINGDOM_MASK), kingdom));
        }
    }
}

QRectF HegemonyRoleComboBox::boundingRect() const
{
    double scale = G_ROOM_LAYOUT.scale;
    static QRect rect = G_ROOM_SKIN.getPixmap(QString::fromUtf8(QSanRoomSkin::S_SKIN_KEY_EXPANDING_ROLE_BOX)).rect();
    return QRectF(rect.x(), rect.y(), (int)(rect.width() * scale), (int)(rect.height() * scale));
}

HegemonyRoleComboBox::HegemonyRoleComboBox(QGraphicsItem *photo, bool circle)
    : QGraphicsObject(photo)
    , circle(circle)
    , expanding(false)
{
    QStringList kingdoms = Sanguosha->hegemonyKingdoms();
    kingdoms.removeAll(QStringLiteral("god"));
    foreach (const QString &kingdom, kingdoms)
        kingdoms_excluded[kingdom] = false;

    connect(RoomSceneInstance, &RoomScene::cancel_role_box_expanding, this, &HegemonyRoleComboBox::mouseClickedOutside);
    setAcceptedMouseButtons(Qt::LeftButton);
}

void HegemonyRoleComboBox::fix(const QString &role)
{
    if (role == QStringLiteral("god"))
        return;
    fixed_role = role;
    update();
}

void HegemonyRoleComboBox::mouseClickedOutside()
{
    if (!expanding)
        return;
    expanding = false;
    update();
}
