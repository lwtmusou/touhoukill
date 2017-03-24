#ifndef _th10_H
#define _th10_H

#include "card.h"
#include "package.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

class GongfengCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE GongfengCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class QijiDialog : public QDialog
{
    Q_OBJECT

public:
    static QijiDialog *getInstance(const QString &object, bool left = true, bool right = true);

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit QijiDialog(const QString &object, bool left = true, bool right = true);

    QGroupBox *createLeft();
    QGroupBox *createRight();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    QString object_name;

signals:
    void onButtonClick();
};

class QijiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QijiCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};

class FengshenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FengshenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class XinshangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XinshangCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

class FengrangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FengrangCard();

    virtual const Card *validate(CardUseStruct &card_use) const;
};

class JiliaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JiliaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class BujuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BujuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class TH10Package : public Package
{
    Q_OBJECT

public:
    TH10Package();
};

#endif
