#ifndef JOYPACKAGE_H
#define JOYPACKAGE_H

#include "package.h"
#include "standard.h"

class JoyPackage: public Package{
    Q_OBJECT

public:
    JoyPackage();
};

class DisasterPackage: public Package{
    Q_OBJECT

public:
    DisasterPackage();
};

class JoyEquipPackage: public Package{
    Q_OBJECT

public:
    JoyEquipPackage();
};

class Shit:public BasicCard{
    Q_OBJECT

public:
    Q_INVOKABLE Shit(Card::Suit suit, int number);
    virtual QString getSubtype() const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;

    static bool HasShit(const Card *card);
};



// five disasters:

class Deluge: public Disaster{
    Q_OBJECT

public:
    Q_INVOKABLE Deluge(Card::Suit suit, int number);
    virtual void takeEffect(ServerPlayer *target) const;
};

class Typhoon: public Disaster{
    Q_OBJECT

public:
    Q_INVOKABLE Typhoon(Card::Suit suit, int number);
    virtual void takeEffect(ServerPlayer *target) const;
};

class Earthquake: public Disaster{
    Q_OBJECT

public:
    Q_INVOKABLE Earthquake(Card::Suit suit, int number);
    virtual void takeEffect(ServerPlayer *target) const;
};

class Volcano: public Disaster{
    Q_OBJECT

public:
    Q_INVOKABLE Volcano(Card::Suit suit, int number);
    virtual void takeEffect(ServerPlayer *target) const;
};

class MudSlide: public Disaster{
    Q_OBJECT

public:
    Q_INVOKABLE MudSlide(Card::Suit suit, int number);
    virtual void takeEffect(ServerPlayer *target) const;
};

class Monkey: public OffensiveHorse{
    Q_OBJECT

public:
    Q_INVOKABLE Monkey(Card::Suit suit, int number);

    virtual QString getCommonEffectName() const;

private:
    TriggerSkill *grab_peach;
};

class GaleShell:public Armor{
    Q_OBJECT

public:
    Q_INVOKABLE GaleShell(Card::Suit suit, int number);

    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
};

class YxSword: public Weapon{
    Q_OBJECT

public:
    Q_INVOKABLE YxSword(Card::Suit suit, int number);
};

class YuluCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE YuluCard();
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class NumaNRNMCard: public SkillCard{
    Q_OBJECT

public:
    Q_INVOKABLE NumaNRNMCard();
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const;
};

class DCPackage: public Package{
    Q_OBJECT

public:
    DCPackage();
};

#endif // JOYPACKAGE_H
