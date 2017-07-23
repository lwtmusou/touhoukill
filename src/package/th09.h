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

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TianrenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TianrenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual const Card *validate(CardUseStruct &cardUse) const;
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

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
};

class TH09Package : public Package
{
    Q_OBJECT

public:
    TH09Package();
};

#endif
