#ifndef _th09_H
#define _th09_H

#include "card.h"
#include "package.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

class ToupaiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ToupaiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class TianrenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TianrenCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    const Card *validate(CardUseStruct &cardUse) const override;
};

class NianliDialog : public QDialog
{
    Q_OBJECT

public:
    static NianliDialog *getInstance(const QString &object);

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit NianliDialog(const QString &object);

    QVBoxLayout *layout;
    QButtonGroup *group;
    //QHash<QString, const Card *> map;

    QString object_name;

signals:
    void onButtonClick();
};

class NianliCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE NianliCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
};

class MengxiangTargetCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MengxiangTargetCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class MengxiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MengxiangCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
};

class JishiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JishiCard();

    void use(Room *room, const CardUseStruct &card_use) const override;
};

class MianLingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE MianLingCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
    const Card *validateInResponse(ServerPlayer *user) const override;
};

class KuaizhaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KuaizhaoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class ShizaiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShizaiCard();

    void use(Room *room, const CardUseStruct &card_use) const override;
};

class YsJieCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YsJieCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class YucanSelectCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YucanSelectCard();

public:
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, int &maxVotes) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
};

class YucanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YucanCard();

    // Card interface

    QList<int> do_yucan(Room *room, ServerPlayer *eat) const;

    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    const Card *validate(CardUseStruct &cardUse) const override;
    const Card *validateInResponse(ServerPlayer *user) const override;
};

class YucanDialog : public QDialog
{
    Q_OBJECT

public:
    static YucanDialog *getInstance();

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit YucanDialog();

    QVBoxLayout *create();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    static QString object_name;

signals:
    void onButtonClick();
};

class HuiranCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuiranCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
};

class YuyanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YuyanCard();

    void onEffect(const CardEffectStruct &effect) const override;
};

class TH09Package : public Package
{
    Q_OBJECT

public:
    TH09Package();
};

#endif
