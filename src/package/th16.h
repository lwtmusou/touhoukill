#ifndef TH16_H
#define TH16_H

#include "card.h"
#include "package.h"

class ZhaoweiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhaoweiCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ZhuzheCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhuzheCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class MenfeiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MenfeiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class LinsaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LinsaCard();
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class HuyuanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuyuanCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class GuwuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GuwuCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class HuazhaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuazhaoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ChuntengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChuntengCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class Chunteng2Card : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Chunteng2Card();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class TH16Package : public Package
{
    Q_OBJECT

public:
    TH16Package();
};

#endif // TH16_H
