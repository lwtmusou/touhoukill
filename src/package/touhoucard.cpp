#include "touhoucard.h"
#include "engine.h"
#include "maneuvering.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"



NosRendeCard::NosRendeCard() {
    mute = true;
    will_throw = false;
    handling_method = Card::MethodNone;
}

void NosRendeCard::use(Room *room, ServerPlayer *source, QList<ServerPlayer *> &targets) const{
    ServerPlayer *target = targets.first();

    room->broadcastSkillInvoke("rende");
    CardMoveReason reason(CardMoveReason::S_REASON_GIVE, source->objectName(), target->objectName(), "nosrende", QString());
    room->obtainCard(target, this, reason, false);

    int old_value = source->getMark("nosrende");
    int new_value = old_value + subcards.length();
    room->setPlayerMark(source, "nosrende", new_value);

    if (old_value < 2 && new_value >= 2) {
        RecoverStruct recover;
        recover.card = this;
        recover.who = source;
        room->recover(source, recover);
    }
}


touhoucardPackage::touhoucardPackage()
    :Package("touhoucard")
{
    QList<Card *> cards;

    cards << new IceSlash(Card::Spade, 4);


    foreach(Card *card, cards)
        card->setParent(this);

    type = CardPack;
}

touhouskillcardPackage::touhouskillcardPackage()
    : Package("touhouskillcard")
{
    addMetaObject<NosRendeCard>();

}

ADD_PACKAGE(touhoucard)
ADD_PACKAGE(touhouskillcard)


