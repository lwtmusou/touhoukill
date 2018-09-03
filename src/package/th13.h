#ifndef _th13_H
#define _th13_H

#include "card.h"
#include "package.h"
#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

class QingtingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QingtingCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class XihuaDialog : public QDialog
{
    Q_OBJECT

public:
    static XihuaDialog *getInstance(const QString &object, bool left = true, bool right = true);

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit XihuaDialog(const QString &object, bool left = true, bool right = true);

    QGroupBox *createLeft();
    QGroupBox *createRight();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    QString object_name;

signals:
    void onButtonClick();
};

class XihuaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XihuaCard();

    bool do_xihua(ServerPlayer *tanuki) const;
    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class ShijieCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShijieCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class LeishiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LeishiCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class XiefaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XiefaCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

/*class HuishengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuishengCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &card_use) const;
};*/

class BumingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BumingCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TH13Package : public Package
{
    Q_OBJECT

public:
    TH13Package();
};

#endif
