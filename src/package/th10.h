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

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
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

class FengshenCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FengshenCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class ShowFengsu : public ShowDistanceCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShowFengsu();
};

class XinshangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XinshangCard();

    void onEffect(const CardEffectStruct &effect) const override;
};

class FengrangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FengrangCard();

    const Card *validate(CardUseStruct &card_use) const override;
};

class JiliaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JiliaoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class BujuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BujuCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class TH10Package : public Package
{
    Q_OBJECT

public:
    TH10Package();
};

#endif
