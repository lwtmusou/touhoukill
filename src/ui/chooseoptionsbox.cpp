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

#include "chooseoptionsbox.h"
#include "button.h"
#include "client.h"
#include "clientstruct.h"
#include "engine.h"
//#include "timed-progressbar.h"
#include "TimedProgressBar.h"

#include <QGraphicsProxyWidget>

ChooseOptionsBox::ChooseOptionsBox()
    : progressBar(NULL)
{
}
//====================
//||================||
//|| Please Choose: ||
//||    _______     ||
//||   |   1   |    ||
//||    -------     ||
//||    _______     ||
//||   |   2   |    ||
//||    -------     ||
//||    _______     ||
//||   |   3   |    ||
//||    -------     ||
//====================

QRectF ChooseOptionsBox::boundingRect() const
{
    const int width = getButtonWidth() + outerBlankWidth * 2;
    int height = topBlankWidth + options.size() * defaultButtonHeight + (options.size() - 1) * interval + bottomBlankWidth;

    if (ServerInfo.OperationTimeout != 0)
        height += 12;

    return QRectF(0, 0, width, height);
}

void ChooseOptionsBox::chooseOption(const QStringList &options)
{
    //repaint background
    this->options = options;
    //title = QString("%1 %2").arg(Sanguosha->translate(skillName)).arg(tr("Please choose:"));
    title = QString("%1").arg(Sanguosha->translate(skillName));
    prepareGeometryChange();

    const int buttonWidth = getButtonWidth();
    QMap<Button *, QPoint> pos;
    int x = 0;
    int y = 0;
    foreach (const QString &option, options) {
        y = 0;
        ++x;
        foreach (const QString &choice, option.split("+")) {
            ++y;
            Button *button = new Button(translate(choice), QSizeF(buttonWidth, defaultButtonHeight));
            //Button *button = new Button(translate(choice), QSizeF(500, defaultButtonHeight));
            //Button *button = new Button(translate(choice), QSizeF(buttonWidth, defaultButtonHeight), Config.UIFont);

            button->setObjectName(choice);
            buttons << button;
            button->setParentItem(this);
            pos[button] = QPoint(x, y);

            QString original_tooltip = QString(":%1").arg(title);
            QString tooltip = Sanguosha->translate(original_tooltip);
            if (tooltip == original_tooltip) {
                original_tooltip = QString(":%1").arg(choice);
                tooltip = Sanguosha->translate(original_tooltip);
            }
            connect(button, &Button::clicked, this, &ChooseOptionsBox::reply);
            if (tooltip != original_tooltip)
                button->setToolTip(QString("<font color=yellow>%2</font>").arg(tooltip));
        }
    }

    moveToCenter();
    show();

    /*for (int i = 0; i < buttons.length(); ++i) {
        Button *button = buttons.at(i);

        QPoint p = pos[button];

        QPointF pos;
        pos.setX(outerBlankWidth + (p.x() - 1) * (getButtonWidth() + interval));
        pos.setY(topBlankWidth + defaultButtonHeight * (p.y() - 1) + (p.y() - 2) * interval + defaultButtonHeight / 2);

        button->setPos(pos);
    }*/

    y = topBlankWidth;
    for (int i = 0; i < buttons.length(); ++i) {
        Button *button = buttons.at(i);

        QPointF pos;
        pos.setX(outerBlankWidth);
        pos.setY(y);

        button->setPos(pos);
        y += button->boundingRect().height() + interval;
    }

    if (ServerInfo.OperationTimeout != 0) {
        if (!progressBar) {
            progressBar = new QSanCommandProgressBar();
            progressBar->setMaximumWidth(boundingRect().width() - 16);
            progressBar->setMaximumHeight(12);
            progressBar->setTimerEnabled(true);
            progressBarItem = new QGraphicsProxyWidget(this);
            progressBarItem->setWidget(progressBar);
            progressBarItem->setPos(boundingRect().center().x() - progressBarItem->boundingRect().width() / 2, boundingRect().height() - 20);
            connect(progressBar, &QSanCommandProgressBar::timedOut, this, &ChooseOptionsBox::reply);
        }
        progressBar->setCountdown(QSanProtocol::S_COMMAND_MULTIPLE_CHOICE);
        progressBar->show();
    }
}

void ChooseOptionsBox::reply()
{
    QString choice = sender()->objectName();
    if (choice.isEmpty())
        choice = options.first();
    //ClientInstance->onPlayerMakeChoice(choice);
    ClientInstance->onPlayerChooseOption(choice);
    //ClientInstance->onPlayerMakeChoice();
}

int ChooseOptionsBox::getButtonWidth() const
{
    if (options.isEmpty())
        return minButtonWidth;

    //QFontMetrics fontMetrics(Config.UIFont);
    QFontMetrics fontMetrics(Config.SmallFont);
    int biggest = 0;
    foreach (const QString &section, options) {
        foreach (const QString &choice, section.split("+")) {
            const int width = fontMetrics.width(translate(choice));
            if (width > biggest)
                biggest = width;
        }
    }

    // Otherwise it would look compact
    biggest += 20;

    int width = minButtonWidth;
    return qMax(biggest, width);
}

QString ChooseOptionsBox::translate(const QString &option) const
{
    QString title = QString("%1:%2").arg(skillName).arg(option);
    QString translated = Sanguosha->translate(title);
    if (translated == title)
        translated = Sanguosha->translate(option);
    return translated;
}

void ChooseOptionsBox::clear()
{
    if (progressBar != NULL) {
        progressBar->hide();
        progressBar->deleteLater();
        progressBar = NULL;
    }

    foreach (Button *button, buttons)
        button->deleteLater();

    buttons.clear();

    disappear();
}
