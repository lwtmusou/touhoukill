#ifndef _th06_H
#define _th06_H

#include "card.h"
#include "package.h"

#include <QDialog>

class QAbstractButton;
class QGroupBox;
class QButtonGroup;

class SkltKexueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SkltKexueCard();

    void use(Room *room, const CardUseStruct &card_use) const override;
};

class SuodingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SuodingCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, int &maxVotes) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class BeishuiDialog : public QDialog
{
    Q_OBJECT

public:
    static BeishuiDialog *getInstance(const QString &object, bool left = true, bool right = true);

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit BeishuiDialog(const QString &object, bool left = true, bool right = true);

    QGroupBox *createLeft();
    //QGroupBox *createRight();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    QString object_name;

signals:
    void onButtonClick();
};

class HezhouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HezhouCard();

    bool do_hezhou(ServerPlayer *player) const;
    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
    const Card *validateInResponse(ServerPlayer *user) const override;
};

class ZhenyeCard : public SkillCard
{
    Q_OBJECT

private:
    static int X(const Player *p);

public:
    Q_INVOKABLE ZhenyeCard();

    void onEffect(const CardEffectStruct &effect) const override;
};

class SishuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SishuCard();

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class BanyueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BanyueCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class TH06Package : public Package
{
    Q_OBJECT

public:
    TH06Package();
};

#endif
