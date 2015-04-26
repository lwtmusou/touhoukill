#ifndef _th10_H
#define _th10_H

#include "package.h"
#include "card.h"



#include <QGroupBox>
#include <QAbstractButton>
#include <QButtonGroup>
#include <QDialog>
#include <QHBoxLayout>
#include <QVBoxLayout>
//#include <QCommandLinkButton>

/*class shendeDummyCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE shendeDummyCard();

};

class shendeFakeMoveCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE shendeFakeMoveCard();

    virtual const Card *validate(CardUseStruct &card_use) const;
};*/


class gongfengCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE gongfengCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};


class qijiDialog : public QDialog {
    Q_OBJECT

public:
    static qijiDialog *getInstance(const QString &object, bool left = true, bool right = true);

    public slots:
    void popup();
    void selectCard(QAbstractButton *button);

private:
    explicit qijiDialog(const QString &object, bool left = true, bool right = true);

    QGroupBox *createLeft();
    QGroupBox *createRight();
    QAbstractButton *createButton(const Card *card);
    QButtonGroup *group;
    QHash<QString, const Card *> map;

    QString object_name;

signals:
    void onButtonClick();
};

class qijiCard : public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE qijiCard();

    virtual bool targetFixed() const;
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;

    virtual const Card *validate(CardUseStruct &card_use) const;
    virtual const Card *validateInResponse(ServerPlayer *user) const;
};






class fengshenCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE fengshenCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onEffect(const CardEffectStruct &effect) const;
};

class xinshangCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE xinshangCard();

    virtual void onEffect(const CardEffectStruct &effect) const;
};

/*class zaihuoCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE zaihuoCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};*/

class tianyanCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE tianyanCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class fengrangCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE fengrangCard();

    virtual const Card *validate(CardUseStruct &card_use) const;
};



class jiliaoCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE jiliaoCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class dfgzmsiyuCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE dfgzmsiyuCard();

    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class ExtraCollateralCard : public SkillCard {
    Q_OBJECT

public:
    Q_INVOKABLE ExtraCollateralCard();

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
};

class th10Package : public Package {
    Q_OBJECT

public:
    th10Package();
};

#endif

