#include "touhoucard.h"
#include "engine.h"
#include "maneuvering.h"
#include "settings.h"
#include "skill.h"
#include "standard.h"
#include "client.h"
#include "clientplayer.h"


touhoucardPackage::touhoucardPackage()
    :Package("touhoucard")
{
    QList<Card *> cards;

    cards << new IceSlash(Card::Spade, 4);


    foreach(Card *card, cards)
        card->setParent(this);

    type = CardPack;
}


ADD_PACKAGE(touhoucard)


