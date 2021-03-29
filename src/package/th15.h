#ifndef TH15_H
#define TH15_H

#include "card.h"
#include "package.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

class YuejianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YuejianCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};


class YidanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YidanCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
};

class TH15Package : public Package
{
    Q_OBJECT

public:
    TH15Package();
};

#endif
