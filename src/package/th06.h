#ifndef _th06_H
#define _th06_H

#include "card.h"
#include "package.h"

class SkltKexueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SkltKexueCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class SuodingCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SuodingCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, int &maxVotes) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
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



class SishuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE SishuCard();

    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class BanyueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BanyueCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TH06Package : public Package
{
    Q_OBJECT

public:
    TH06Package();
};

#endif
