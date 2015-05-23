
/*shendeDummyCard::shendeDummyCard() {
    //mute = true;
    will_throw = false;
    target_fixed = true;
}

shendeFakeMoveCard::shendeFakeMoveCard() {
    //mute = true;
    will_throw = false;
    target_fixed = true;
}
const Card *shendeFakeMoveCard::validate(CardUseStruct &use) const{
    Room *room = use.from->getRoom();
    QList<int> card_ids = use.from->getPile("shende");
    CardMoveReason reason(CardMoveReason::S_REASON_PREVIEWGIVE, use.from->objectName(), "shende", "shende");
    CardsMoveStruct move(card_ids, use.from, use.from, Player::PlaceSpecial, Player::PlaceHand, reason);
    QList<CardsMoveStruct> moves;
    moves << move;
    QList<ServerPlayer *> players;
    players << use.from;
    room->notifyMoveCards(true, moves, true, players);
    room->notifyMoveCards(false, moves, true, players);

    const Card *card = room->askForUseCard(use.from, "@@shende!", "@shende-twoCards");
    CardsMoveStruct move1(card_ids, use.from, NULL, Player::PlaceHand, Player::PlaceTable, reason);
    QList<CardsMoveStruct> moves1;
    moves1 << move1;
    room->notifyMoveCards(true, moves1, true, players);
    room->notifyMoveCards(false, moves1, true, players);

    if (card != NULL){
        Peach *peach = new Peach(Card::SuitToBeDecided, -1);
        foreach(int id, card->getSubcards())
            peach->addSubcard(id);
        peach->setSkillName("shende");
        return peach;
    }
    return NULL;
}*/



/*class shendevs : public ViewAsSkill {
public:
    shendevs() : ViewAsSkill("shende") {
    }

    virtual bool isEnabledAtPlay(const Player *player) const{
        if (player->getPile("shende").length() < 2)
            return false;
        Peach *peach = new Peach(Card::NoSuit, 0);
        peach->deleteLater();
        return peach->isAvailable(player);
    }
    virtual bool isEnabledAtResponse(const Player *player, const QString &pattern) const {
        if (player->getMark("Global_PreventPeach") > 0)
            return false;
        if (pattern == "@@shende!")
            return true;
        if (player->getPile("shende").length() >= 2)
            return pattern.contains("peach");
        return false;
    }

    virtual bool viewFilter(const QList<const Card *> &selected, const Card *to_select) const{
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@shende!"){
            QVariantList s_ids = Self->tag["shende_piles"].toList();
            QList<int> shendes;
            foreach(QVariant card_data, s_ids)
                shendes << card_data.toInt();
            return shendes.contains(to_select->getEffectiveId());
        }
        return false;
    }

    virtual const Card *viewAs(const QList<const Card *> &cards) const{
        QString pattern = Sanguosha->getCurrentCardUsePattern();
        if (pattern == "@@shende!"){
            if (cards.length() != 2)
                return NULL;
            shendeDummyCard *card = new shendeDummyCard;
            card->addSubcards(cards);
            return card;
        } else {
            QVariantList ids = Self->tag["GuzhengToGet"].toList();
            foreach(int card_id, Self->getPile("shende"))
                ids << card_id;
            Self->tag["shende_piles"] = ids;
            return new shendeFakeMoveCard;
        }
    }
};
*/


