#ifndef PLAYGROUND_H
#define PLAYGROUND_H

#include "card.h"
#include "package.h"

#include <QDialog>

class QAbstractButton;
class QButtonGroup;

class Fsu0413GainianDialog : public QDialog
{
    Q_OBJECT

public:
    static Fsu0413GainianDialog *getInstance(const QString &object);
    bool isResponseOk(const Player *player, const QString &pattern) const;

public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit Fsu0413GainianDialog(const QString &object);

    void createButtons();

    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    QString object_name;

signals:
    void onButtonClick();
};

class Fsu0413Fei2ZhaiCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Fsu0413Fei2ZhaiCard();

    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const override;
};

class Fsu0413JbdNashaCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE Fsu0413JbdNashaCard();

    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onEffect(const CardEffectStruct &effect) const override;
};

class PlaygroundPackage : public Package
{
    Q_OBJECT

public:
    PlaygroundPackage();
};

#endif // TH16_H
