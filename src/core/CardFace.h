#pragma once

#include <QObject>
#include <QString>

class Player;
class ServerPlayer;
class Room;
class Card;

struct CardUseStruct;
struct CardEffectStruct;


/**
 * @interface The functional model of a given card. 
 */ 
class CardFace : public QObject {
    Q_OBJECT
public:
    // text property
    // FIXME: replace `name` with objectName ?
    virtual QString name() const;
    virtual QString description() const;
    virtual QString commmonEffectName() const;
    virtual QString effectName() const;
    // FIXME: Maybe move this to the SkillCard sub class. 
    virtual QString showSkillName() const;


    // type property
    enum CardType
    {
        TypeUnknown,
        TypeSkill,
        TypeBasic,
        TypeTrick,
        TypeEquip
    };

    virtual CardType type() const = 0;
    virtual QString typeName() const = 0;
    virtual QString subTypeName() const = 0;
    virtual bool isKindOf(const char *cardType) const;
    virtual bool matchType(const QString &pattern) const;
    // Can we have a better way to replace this function? Maybe using `match`
    virtual bool isNDTrick() const;

    // property identifier
    // FIXME: these identifier should be overriden by the Card.
    virtual bool canDamage() const;
    virtual bool canRecover() const;
    virtual bool canRecast() const;
    virtual bool hasEffectValue() const;
    virtual bool willThrow() const;
    // FIXME: Do we really this function? What will happen if we provide a PreAction that does nothing?
    virtual bool hasPreAction() const;

    // card handling method
    enum HandlingMethod
    {
        MethodNone,
        MethodUse,
        MethodResponse,
        MethodDiscard,
        MethodRecast,
        MethodPindian
    };
    // FIXME: I think the handling method is given by actual case (skill or rules) and is not part of the CardFace which is believed to be the pure functional model according to the description.
    virtual HandlingMethod defaultHandlingMethod() const;

    // Functions
    virtual bool targetFixed(const Player *Self, const Card *card) const;
    virtual bool targetsFeasible(const QList<const Player *> &targets, const Player *Self, const Card *card) const;
    // FIXME: the following two functions should be merged into one.
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card) const;
    // FIXME: return tuple/pair rather than a pure bool and combine two functions together?
    virtual bool targetFilter(const QList<const Player *> &targets, const Player *to_select, const Player *Self, const Card *card, int &maxVotes) const;

    virtual bool isAvailable(const Player *player, const Card *card) const;

    virtual bool ignoreCardValidity(const Player *player) const;
    virtual const Card *validate(const CardUseStruct &cardUse) const;
    virtual const Card *validateInResponse(ServerPlayer *user, const Card *original_card) const;

    virtual void doPreAction(Room *room, const CardUseStruct &card_use) const;
    virtual void onUse(Room *room, const CardUseStruct &card_use) const;
    virtual void use(Room *room, const CardUseStruct &use) const;

    virtual void onEffect(const CardEffectStruct &effect) const;
    virtual bool isCancelable(const CardEffectStruct &effect) const;
    virtual void onNullified(ServerPlayer *target) const;
};

