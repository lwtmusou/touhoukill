%{

#include "standard.h"
#include "maneuvering.h"

%}

class BasicCard: public Card {
public:
    BasicCard(Suit suit, int number);
    virtual QString getType() const;
    virtual CardType getTypeId() const;
};

class TrickCard: public Card {
public:
    TrickCard(Suit suit, int number);
    void setCancelable(bool cancelable);

    virtual QString getType() const;
    virtual CardType getTypeId() const;
    virtual bool isCancelable(const CardEffectStruct &effect) const;
};

class DelayedTrick: public TrickCard {
public:
    DelayedTrick(Suit suit, int number, bool movable = false);

private:
    bool movable;
};

class EquipCard: public Card {
public:
    enum Location {
        WeaponLocation,
        ArmorLocation,
        DefensiveHorseLocation,
        OffensiveHorseLocation
    };

    EquipCard(Suit suit, int number): Card(suit, number, true) { handling_method = MethodUse; }

    virtual QString getType() const;
    virtual CardType getTypeId() const;

    virtual void onInstall(ServerPlayer *player) const;
    virtual void onUninstall(ServerPlayer *player) const;

    virtual Location location() const = 0;
};

%extend EquipCard {
    void equipOnInstall(ServerPlayer *player) const{
        $self->EquipCard::onInstall(player);
    }

    void equipOnUninstall(ServerPlayer *player) const{
        $self->EquipCard::onUninstall(player);
    }
};

class Weapon: public EquipCard {
public:
    Weapon(Suit suit, int number, int range);
    int getRange() const;
    virtual QString getSubtype() const;

    virtual Location location() const;

    virtual void onInstall(ServerPlayer *player) const;
    virtual void onUninstall(ServerPlayer *player) const;
};

class Armor: public EquipCard {
public:
    Armor(Suit suit, int number): EquipCard(suit, number) {}
    virtual QString getSubtype() const;

    virtual Location location() const;
};

class Horse: public EquipCard {
public:
    Horse(Suit suit, int number, int correct);
    int getCorrect() const;

    virtual Location location() const;
    virtual void onInstall(ServerPlayer *player) const;
    virtual void onUninstall(ServerPlayer *player) const;
};

class OffensiveHorse: public Horse {
public:
    OffensiveHorse(Card::Suit suit, int number, int correct = -1);
    virtual QString getSubtype() const;
};

class DefensiveHorse: public Horse {
public:
    DefensiveHorse(Card::Suit suit, int number, int correct = +1);
    virtual QString getSubtype() const;
};

class Treasure : public EquipCard
{
public:
    Treasure(Suit suit, int number);
    virtual QString getSubtype() const;
    virtual Location location() const;
};

class Slash: public BasicCard {
public:
    Slash(Card::Suit suit, int number);
    DamageStruct::Nature getNature() const;
    void setNature(DamageStruct::Nature nature);

    static bool IsAvailable(const Player *player, const Card *slash = NULL, bool considerSpecificAssignee = true);
    static bool IsSpecificAssignee(const Player *player, const Player *from, const Card *slash);

protected:
    DamageStruct::Nature nature;
    mutable int drank;
};

class Analeptic: public BasicCard {
public:
    Analeptic(Card::Suit suit, int number);

    static bool IsAvailable(const Player *player, const Card *analeptic = NULL);
};
