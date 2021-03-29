#ifndef _touhougod_H
#define _touhougod_H

#include "card.h"
#include "package.h"
#include "skill.h"

#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QVBoxLayout>

class HongwuCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HongwuCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class ShenqiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShenqiangCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class HuimieCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuimieCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class ShenshouCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShenshouCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class FengyinCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE FengyinCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class HuaxiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE HuaxiangCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
    const Card *validateInResponse(ServerPlayer *user) const override;
};

class ChaowoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ChaowoCard();

    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class ShenbaoDialog : public QDialog
{
    Q_OBJECT

public:
    static ShenbaoDialog *getInstance(const QString &object);
    static QStringList getAvailableChoices(const Player *player, CardUseStruct::CardUseReason cardUseReason, const QString &cardUsePattern);
    static QStringList getAvailableNullificationChoices(const ServerPlayer *player);

public slots:
    void popup(Player *Self);
    void selectSkill(QAbstractButton *button);

private:
    explicit ShenbaoDialog(const QString &object);

    QButtonGroup *group;
    Player *Self;

    static const QStringList equipViewAsSkills;

signals:
    void onButtonClick();
};

class ShowShenbaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE ShowShenbaoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    const Card *validate(CardUseStruct &card_use) const override;
};

class WendaoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WendaoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class XinhuaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XinhuaCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
    const Card *validateInResponse(ServerPlayer *user) const override;
};

class RumoCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE RumoCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class AnyunDialog : public QDialog
{
    Q_OBJECT

public:
    static AnyunDialog *getInstance(const QString &object);

public slots:
    void popup(Player *Self);
    void selectSkill(QAbstractButton *button);

private:
    explicit AnyunDialog(const QString &object);

    QVBoxLayout *layout;
    QButtonGroup *group;
    Player *Self;

    QString object_name;

signals:
    void onButtonClick();
};

class XianshiDialog : public QDialog
{
    Q_OBJECT

public:
    static XianshiDialog *getInstance(const QString &object, bool left = true, bool right = true);

public slots:
    void popup(Player *Self);
    void selectCard(QAbstractButton *button);

private:
    explicit XianshiDialog(const QString &object, bool left = true, bool right = true);

    QGroupBox *createLeft();
    QGroupBox *createRight();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;
    Player *Self;

    QString object_name;

signals:
    void onButtonClick();
};

class XianshiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XianshiCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
    const Card *validateInResponse(ServerPlayer *user) const override;
};

class WenyueCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WenyueCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class QianqiangCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE QianqiangCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class KuangjiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE KuangjiCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class XiuyeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE XiuyeCard();

    bool targetFixed(const Player *Self) const override;
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const override;

    const Card *validate(CardUseStruct &card_use) const override;
    const Card *validateInResponse(ServerPlayer *user) const override;
};

class TouhouGodPackage : public Package
{
    Q_OBJECT

public:
    TouhouGodPackage();
};

#endif
