#ifndef _protagonist_H
#define _protagonist_H

#include "card.h"
#include "package.h"

class MofaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MofaCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class WuyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WuyuCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
};

class SaiqianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SaiqianCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class ShoucangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShoucangCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class BaoyiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BaoyiCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class ChunxiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChunxiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
};

class BllmSeyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BllmSeyuCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const override;
    const Card *validate(CardUseStruct &cardUse) const override;
};

class BllmShiyuDummy : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BllmShiyuDummy();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &) const override;
};

class BllmShiyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BllmShiyuCard();

    const Card *validate(CardUseStruct &cardUse) const override;
    const Card *validateInResponse(ServerPlayer *user) const override;
};

class BllmWuyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BllmWuyuCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class DfgzmSiyuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE DfgzmSiyuCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class YinyangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YinyangCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class BodongCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BodongCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, int &maxVotes) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class ProtagonistPackage : public Package
{
    Q_OBJECT

public:
    ProtagonistPackage();
};

#endif
