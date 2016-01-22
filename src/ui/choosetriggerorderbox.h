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

#ifndef CHOOSETRIGGERORDERBOX_H
#define CHOOSETRIGGERORDERBOX_H

#include <QGraphicsObject>

#include "graphicsbox.h"

class Button;
class QGraphicsProxyWidget;
class QSanCommandProgressBar;

class TriggerOptionButton : public QGraphicsObject
{
    Q_OBJECT
    friend class ChooseTriggerOrderBox;

public:
    static QFont defaultFont();

signals:
    void clicked();
    void hovered(bool entering);

protected:
    virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *, QWidget *);
    virtual QRectF boundingRect() const;
    virtual void mousePressEvent(QGraphicsSceneMouseEvent *event);
    virtual void mouseReleaseEvent(QGraphicsSceneMouseEvent *event);
    virtual void hoverEnterEvent(QGraphicsSceneHoverEvent *event);
    virtual void hoverLeaveEvent(QGraphicsSceneHoverEvent *event);

    static QString displayedTextOf(const QString &str);

private:
    TriggerOptionButton(QGraphicsObject *parent, const QString &player, const QString &skillStr, const int width);

    QString getGeneralNameBySkill() const;

    QString m_skillStr;
    QString m_text;
    QString playerName;
    int width;
};

class ChooseTriggerOrderBox : public GraphicsBox
{
    Q_OBJECT

public:
    ChooseTriggerOrderBox();

    virtual QRectF boundingRect() const;
    void chooseOption(const QString &reason, const QStringList &options, const bool optional);
    void clear();

public slots:
    void reply();

private:
    QList<TriggerOptionButton *> optionButtons;
    static const int top_dark_bar;
    static const int m_topBlankWidth;
    static const int bottom_blank_width;
    static const int interval;
    static const int m_leftBlankWidth;

    QStringList options;
    bool optional;
    int m_minimumWidth;

    Button *cancel;
    QGraphicsProxyWidget *progress_bar_item;
    QSanCommandProgressBar *progressBar;

    void storeMinimumWidth();
};

#endif // CHOOSETRIGGERORDERBOX_H
