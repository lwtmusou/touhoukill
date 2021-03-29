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

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class XihuaDialog : public QDialog
{
    Q_OBJECT

public:
    static XihuaDialog *getInstance(const QString &object, bool left = true, bool right = true);

public slots:
    void popup(Player *Self);
    void selectCard(QAbstractButton *button);

private:
    explicit XihuaDialog(const QString &object, bool left = true, bool right = true);

    QGroupBox *createLeft();
    QGroupBox *createRight();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    QString object_name;

    Player *Self;

signals:
    void onButtonClick();
};

class XihuaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XihuaCard();

    bool do_xihua(ServerPlayer *tanuki) const;
    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
    const Card *validateInResponse(ServerPlayer *user) const override;
};

class ShijieCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShijieCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class LeishiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE LeishiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class XiefaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XiefaCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class BumingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BumingCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class TH13Package : public Package
{
    Q_OBJECT

public:
    TH13Package();
};

#endif
