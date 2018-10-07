#ifndef TH15_H
#define TH15_H

#include "card.h"
#include "package.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

/*
class ShayiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShayiCard();

    void onUse(Room *room, const CardUseStruct &card_use) const;

private:
    static bool putToPile(Room *room, ServerPlayer *mori);
    static void cleanUp(Room *room, ServerPlayer *mori);
};*/

class YuejianCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YuejianCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class YidanDialog : public QDialog
{
    Q_OBJECT

public:
    static YidanDialog *getInstance(const QString &object);

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit YidanDialog(const QString &object);

    QVBoxLayout *layout;
    QButtonGroup *group;
    //QHash<QString, const Card *> map;

    QString object_name;

signals:
    void onButtonClick();
};

class YidanCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE YidanCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    //virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
};

class TH15Package : public Package
{
    Q_OBJECT

public:
    TH15Package();
};

#endif
