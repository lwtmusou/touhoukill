#ifndef THKILL_TH10_H
#define THKILL_TH10_H

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

    void use(Room *room, const CardUseStruct &card_use) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetFixed(const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
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

class JiliaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE JiliaoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class JiliaoUseCard : public SkillCard
{
    Q_OBJECT

public:
    JiliaoUseCard();

    bool targetFixed(const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    const Card *validate(CardUseStruct &cardUse) const override;
};

class BujuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE BujuCard();

    void use(Room *room, const CardUseStruct &card_use) const override;
};

class TH10Package : public Package
{
    Q_OBJECT

public:
    TH10Package();
};

#endif
