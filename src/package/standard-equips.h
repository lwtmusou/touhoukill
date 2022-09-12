#ifndef _STANDARD_EQUIPS_H
#define _STANDARD_EQUIPS_H

#include "standard.h"

class Crossbow : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Crossbow(Card::Suit suit, int number = 1);
};

class Triblade : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Triblade(Card::Suit suit, int number = 1);
};

class TribladeCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE TribladeCard();
    bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self) const override;
    void onUse(Room *room, const CardUseStruct &card_use) const override;
    void use(Room *room, const CardUseStruct &card_use) const override;
};

class DoubleSword : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit DoubleSword(Card::Suit suit = Spade, int number = 2);
};

class QinggangSword : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit QinggangSword(Card::Suit suit = Spade, int number = 6);
};

class Blade : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Blade(Card::Suit suit = Spade, int number = 5);
};

class Spear : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Spear(Card::Suit suit = Spade, int number = 12);
};

class Axe : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Axe(Card::Suit suit = Diamond, int number = 5);
};

class Halberd : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit Halberd(Card::Suit suit = Diamond, int number = 12);
};

class KylinBow : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit KylinBow(Card::Suit suit = Heart, int number = 5);
};

class EightDiagram : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit EightDiagram(Card::Suit suit, int number = 2);
};

class BreastPlate : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit BreastPlate(Card::Suit suit = Card::Club, int number = 2);
};

class IceSword : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE IceSword(Card::Suit suit, int number);
};

class RenwangShield : public Armor
{
    Q_OBJECT

public:
    Q_INVOKABLE RenwangShield(Card::Suit suit, int number);
};

class WoodenOxCard : public SkillCard
{
    Q_OBJECT

public:
    Q_INVOKABLE WoodenOxCard();

    void use(Room *room, const CardUseStruct &card_use) const override;
};

class WoodenOx : public Treasure
{
    Q_OBJECT

public:
    Q_INVOKABLE explicit WoodenOx(Card::Suit suit = Diamond, int number = 4);

    void onUninstall(ServerPlayer *player) const override;
};

class DeathSickle : public Weapon
{
    Q_OBJECT

public:
    Q_INVOKABLE DeathSickle(Card::Suit suit, int number);
};

class StandardCardPackage : public Package
{
    Q_OBJECT

public:
    StandardCardPackage();
};

class StandardExCardPackage : public Package
{
    Q_OBJECT

public:
    StandardExCardPackage();
};

#endif
