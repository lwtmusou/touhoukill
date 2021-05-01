#ifndef _AUX_SKILLS_H
#define _AUX_SKILLS_H

#include "skill.h"

class DiscardSkill : public ViewAsSkill
{
    Q_GADGET

public:
    explicit DiscardSkill();
    ~DiscardSkill() override;

    void setNum(int num);
    void setMinNum(int minnum);
    void setIncludeEquip(bool include_equip);
    void setIsDiscard(bool is_discard);

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const override;
    const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const override;

private:
    Card *card; // it should be a dummy card.
    int num;
    int minnum;
    bool include_equip;
    bool is_discard;
};

class CardPattern;

class ResponseSkill : public OneCardViewAsSkill
{
    Q_GADGET

public:
    ResponseSkill();
    virtual bool matchPattern(const Player *player, const Card *card) const;

    void setPattern(const QString &pattern);
    void setRequest(const Card::HandlingMethod request);
    bool viewFilter(const Card *to_select, const Player *Self) const override;
    const Card *viewAs(const Card *originalCard, const Player *Self) const override;

    inline virtual Card::HandlingMethod getRequest() const
    {
        return request;
    }

protected:
    const CardPattern *pattern;
    Card::HandlingMethod request;
};

class ShowOrPindianSkill : public ResponseSkill
{
    Q_GADGET

public:
    ShowOrPindianSkill();
    bool matchPattern(const Player *player, const Card *card) const override;
};

class YijiCard;

class YijiViewAsSkill : public ViewAsSkill
{
    Q_GADGET

public:
    explicit YijiViewAsSkill();
    ~YijiViewAsSkill() override;
    void setCards(const QString &card_str);
    void setMaxNum(int max_num);
    void setPlayerNames(const QStringList &names);

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select, const Player *Self) const override;
    const Card *viewAs(const QList<const Card *> &cards, const Player *Self) const override;

private:
    YijiCard *face;
    Card *card;
    QList<int> ids;
    int max_num;
};

class ChoosePlayerCard;

class ChoosePlayerSkill : public ZeroCardViewAsSkill
{
    Q_GADGET

public:
    explicit ChoosePlayerSkill();
    ~ChoosePlayerSkill() override;
    void setPlayerNames(const QStringList &names);

    const Card *viewAs(const Player *Self) const override;

private:
    ChoosePlayerCard *face;
    Card *card;
};

#endif
