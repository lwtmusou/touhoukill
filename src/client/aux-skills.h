#ifndef _AUX_SKILLS_H
#define _AUX_SKILLS_H

#include "skill.h"

class DiscardSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    explicit DiscardSkill();

    void setNum(int num);
    void setMinNum(int minnum);
    void setIncludeEquip(bool include_equip);
    void setIsDiscard(bool is_discard);

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override;
    const Card *viewAs(const QList<const Card *> &cards) const override;

private:
    DummyCard *card;
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

    virtual void setPattern(const QString &pattern);
    virtual void setRequest(const Card::HandlingMethod request);
    bool viewFilter(const Card *to_select) const override;
    const Card *viewAs(const Card *originalCard) const override;

    inline Card::HandlingMethod getRequest() const
    {
        return request;
    }

protected:
    const CardPattern *pattern;
    Card::HandlingMethod request;
};

class ShowOrPindianSkill : public ResponseSkill
{
    Q_OBJECT

public:
    ShowOrPindianSkill();
    bool matchPattern(const Player *player, const Card *card) const override;
};

class YijiCard;

class YijiViewAsSkill : public ViewAsSkill
{
    Q_OBJECT

public:
    explicit YijiViewAsSkill();
    void setCards(const QString &card_str);
    void setMaxNum(int max_num);
    void setPlayerNames(const QStringList &names);

    bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const override;
    const Card *viewAs(const QList<const Card *> &cards) const override;

private:
    YijiCard *card;
    QList<int> ids;
    int max_num;
};

class ChoosePlayerCard;

class ChoosePlayerSkill : public ZeroCardViewAsSkill
{
    Q_OBJECT

public:
    explicit ChoosePlayerSkill();
    void setPlayerNames(const QStringList &names);

    const Card *viewAs() const override;

private:
    ChoosePlayerCard *card;
};

#endif
