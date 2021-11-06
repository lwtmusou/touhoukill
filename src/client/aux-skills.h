#ifndef _AUX_SKILLS_H
#define _AUX_SKILLS_H

#include "skill.h"

class DiscardSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    explicit DiscardSkill();
    ~DiscardSkill() override = default;

    void setNum(int num);
    void setMinNum(int minnum);
    void setIncludeEquip(bool include_equip);
    void setIsDiscard(bool is_discard);

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self, const QStringList &CurrentViewAsSkillChain) const override;
    const Card *viewAs(const QList<const Card *> &cards, const Player *Self, const QStringList &CurrentViewAsSkillChain) const override;

private:
    int num;
    int minnum;
    bool include_equip;
    bool is_discard;
};

class CardPattern;

class ResponseSkill : public OneCardViewAsSkill
{
    Q_OBJECT

public:
    ResponseSkill();
    virtual bool matchPattern(const Player *player, const Card *card) const;

    void setPattern(const QString &pattern);
    void setRequest(const QSanguosha::HandlingMethod request);
    bool viewFilter(const Card *to_select, const Player *Self, const QStringList &CurrentViewAsSkillChain) const override;
    const Card *viewAs(const Card *originalCard, const Player *Self, const QStringList &CurrentViewAsSkillChain) const override;

    inline virtual QSanguosha::HandlingMethod getRequest() const
    {
        return request;
    }

protected:
    const CardPattern *pattern;
    QSanguosha::HandlingMethod request;
};

class ShowOrPindianSkill : public ResponseSkill
{
    Q_OBJECT

public:
    ShowOrPindianSkill();
    bool matchPattern(const Player *player, const Card *card) const override;
};

class YijiCard : public SkillCard
{
public:
    YijiCard();

    void setPlayerNames(const QStringList &names);

    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *, const Card *card) const override;

    void use(RoomObject *, const CardUseStruct &) const override;

private:
    QSet<QString> set;
};

class YijiViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    explicit YijiViewAsSkill();
    ~YijiViewAsSkill() override = default;
    void setCards(const QString &card_str);
    void setMaxNum(int max_num);
    void setPlayerNames(const QStringList &names);

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self, const QStringList &CurrentViewAsSkillChain) const override;
    const Card *viewAs(const QList<const Card *> &cards, const Player *Self, const QStringList &CurrentViewAsSkillChain) const override;

private:
    QList<int> ids;
    int max_num;
};

class ChoosePlayerCard : public SkillCard
{
public:
    ChoosePlayerCard();

    void setPlayerNames(const QStringList &names);

    int targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *, const Card *) const override;

private:
    QSet<QString> set;
};

class ChoosePlayerSkill : public ZeroCardViewAsSkill
{
    Q_OBJECT

public:
    explicit ChoosePlayerSkill();
    ~ChoosePlayerSkill() override = default;
    void setPlayerNames(const QStringList &names);

    const Card *viewAs(const Player *Self, const QStringList &CurrentViewAsSkillChain) const override;
};

#endif
