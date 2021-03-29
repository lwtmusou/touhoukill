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
#include "TimedProgressBar.h"
#include "button.h"
#include "client.h"
#include "clientstruct.h"
#include "engine.h"
#include "roomscene.h"

#include <QGraphicsProxyWidget>

ChooseOptionsBox::ChooseOptionsBox()
    : progressBar(nullptr)
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
            QStringList choices = choice.split("%");
            QString choice_ = choices.at(0);
            QString text = translate(choice_);
            foreach (const QString &element, choices) {
                if (element.startsWith("from:")) {
                    QStringList froms = element.split(":");
                    if (!froms.at(1).isEmpty()) {
                        QString from = ClientInstance->getPlayerName(froms.at(1));
                        text.replace("%from", from);
                    }
                } else if (element.startsWith("to:")) {
                    QStringList tos = element.split(":");
                    QStringList to_list;
                    for (int i = 1; i < tos.length(); i++)
                        to_list << ClientInstance->getPlayerName(tos.at(i));
                    QString to = to_list.join(", ");
                    text.replace("%to", to);
                } else if (element.startsWith("log:")) {
                    QStringList logs = element.split(":");
                    if (!logs.at(1).isEmpty()) {
                        QString log = logs.at(1);
                        text.replace("%log", log);
                    }
                }
            }

            Button *button = new Button(text, QSizeF(buttonWidth, defaultButtonHeight));

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
    ClientInstance->onPlayerChooseOption(choice);
}

int ChooseOptionsBox::getButtonWidth() const
{
    if (options.isEmpty())
        return minButtonWidth;

    QFontMetrics fontMetrics(Config.SmallFont);
    int biggest = 0;
    foreach (const QString &section, options) {
        foreach (const QString &choice, section.split("+")) {
            const int width = fontMetrics.horizontalAdvance(translate(choice));
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
    if (progressBar != nullptr) {
        progressBar->hide();
        progressBar->deleteLater();
        progressBar = nullptr;
    }

    foreach (Button *button, buttons)
        button->deleteLater();

    buttons.clear();

    disappear();
}
