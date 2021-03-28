#ifndef TH16_H
#define TH16_H

#include "card.h"
#include "package.h"

class ZhaoweiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhaoweiCard();

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class ZhuzheCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhuzheCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class MenfeiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MenfeiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class LinsaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LinsaCard();
    void onEffect(const CardEffectStruct &effect) const override;
};

class HuyuanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuyuanCard();

    void onEffect(const CardEffectStruct &effect) const override;
};

class GuwuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GuwuCard();

    void onEffect(const CardEffectStruct &effect) const override;
};

class HuazhaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuazhaoCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class ChuntengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChuntengCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class Chunteng2Card : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Chunteng2Card();

    void onEffect(const CardEffectStruct &effect) const override;
};

class TH16Package : public Package
{
    Q_OBJECT

public:
    TH16Package();
};

#endif // TH16_H
