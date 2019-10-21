#ifndef _touhougod_H
#define _touhougod_H

#include "card.h"
#include "package.h"
#include "skill.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

class HongwuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HongwuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ShenqiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShenqiangCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HuimieCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuimieCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ShenshouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShenshouCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class FengyinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FengyinCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class HuaxiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuaxiangCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class ChaowoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChaowoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ShenbaoDialog : public QDialog
{
    Q_OBJECT

public:
    static ShenbaoDialog *getInstance(const QString &object);
    static QStringList getAvailableChoices(const Player *player, CardUseStruct::CardUseReason cardUseReason, const QString &cardUsePattern);
    static QStringList getAvailableNullificationChoices(const ServerPlayer *player);

public slots:
    void popup();
    void selectSkill(QAbstractButton *button);

private:
    explicit ShenbaoDialog(const QString &object);

    QButtonGroup *group;

    static const QStringList equipViewAsSkills;

signals:
    void onButtonClick();
};

class ShowShenbaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShowShenbaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    const Card *validate(CardUseStruct &card_use) const;
};


class WendaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WendaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class XinhuaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XinhuaCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class RumoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RumoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class AnyunDialog : public QDialog
{
    Q_OBJECT

public:
    static AnyunDialog *getInstance(const QString &object);

public slots:
    void popup();
    void selectSkill(QAbstractButton *button);

private:
    explicit AnyunDialog(const QString &object);

    QVBoxLayout *layout;
    QButtonGroup *group;

    QString object_name;

signals:
    void onButtonClick();
};

class XianshiDialog : public QDialog
{
    Q_OBJECT

public:
    static XianshiDialog *getInstance(const QString &object, bool left = true, bool right = true);

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit XianshiDialog(const QString &object, bool left = true, bool right = true);

    QGroupBox *createLeft();
    QGroupBox *createRight();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    QString object_name;

signals:
    void onButtonClick();
};

class XianshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XianshiCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class WenyueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WenyueCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class QianqiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QianqiangCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class KuangjiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KuangjiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    //virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class QizhiCard : public DelayedTrick
{
    Q_OBJECT

public:
    Q_INVOKABLE QizhiCard(Card::Suit suit, int number);

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    //void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    void takeEffect(ServerPlayer *) const;

    QString getType() const
    {
        return "skill_card";
    }
    QString getSubtype() const
    {
        return "skill_card";
    }
    CardType getTypeId() const
    {
        return TypeSkill;
    }
    bool isKindOf(const char *cardType) const
    {
        if (strcmp(cardType, "SkillCard") == 0)
            return true;
        else
            return inherits(cardType);
    }
};

class TouhouGodPackage : public Package
{
    Q_OBJECT

public:
    TouhouGodPackage();
};

#endif
