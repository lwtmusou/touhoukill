#ifndef TH16_H
#define TH16_H

#include "card.h"
#include "package.h"

class MishenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MishenCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
};

class LijiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LijiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
};

class MenfeiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MenfeiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

/*class LinsaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LinsaCard();
    void onEffect(const CardEffectStruct &effect) const override;
};*/

class GakungWuCard : public SkillCard
{
    Q_OBJECT

public:
    GakungWuCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;

    // prevent GakungWuCard instance
    virtual void gakungwu() = 0;
};

class GuwuCard : public GakungWuCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GuwuCard();

    void gakungwu() override
    {
    }
};

class KuangwuCard : public GakungWuCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KuangwuCard();

    void gakungwu() override
    {
    }
};

class MiZhiungHteiCard : public SkillCard
{
    Q_OBJECT

public:
    MiZhiungHteiCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;

    // prevent GakungWuCard instance
    virtual void mizhiunhtei() = 0;
};

class MingheCard : public MiZhiungHteiCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MingheCard();

    void mizhiunhtei() override
    {
    }
};

class ZhutiCard : public MiZhiungHteiCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ZhutiCard();

    void mizhiunhtei() override
    {
    }
};

class HuazhaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuazhaoCard();

    void use(Room *room, const CardUseStruct &card_use) const override;
};

class ChuntengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChuntengCard();

    void use(Room *room, const CardUseStruct &card_use) const override;
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
