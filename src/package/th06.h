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

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class SuodingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SuodingCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, int &maxVotes) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class BeishuiDialog : public QDialog
{
    Q_OBJECT

public:
    static BeishuiDialog *getInstance(const QString &object, bool left = true, bool right = true);

public slots:
    void popup(Player *Self);
    void selectCard(QAbstractButton *button);

private:
    explicit BeishuiDialog(const QString &object, bool left = true, bool right = true);

    QGroupBox *createLeft();
    //QGroupBox *createRight();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    QString object_name;

    Player *Self;

signals:
    void onButtonClick();
};

class SishuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SishuCard();

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class BanyueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BanyueCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class TH06Package : public Package
{
    Q_OBJECT

public:
    TH06Package();
};

#endif
