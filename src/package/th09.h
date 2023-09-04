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

class YanhuiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YanhuiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    const Card *validate(CardUseStruct &card_use) const override;
    const Card *validateInResponse(ServerPlayer *user) const override;
};

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

class TH09Package : public Package
{
    Q_OBJECT

public:
    TH09Package();
};

#endif
