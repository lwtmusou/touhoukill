//*************deleted version of fsl001
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


//*************deleted version of yym004
/*class jianshu : public FilterSkill {
public:
    jianshu() : FilterSkill("jianshu") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        Room *room = Sanguosha->currentRoom();
        if (room->getCardPlace(to_select->getEffectiveId()) == Player::PlaceHand){
            ServerPlayer *youmu = room->getCardOwner(to_select->getEffectiveId());
            if (youmu != NULL && youmu->hasSkill(objectName())){
                return  to_select->isKindOf("Weapon");
            }
        }
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Slash *slash = new Slash(originalCard->getSuit(), originalCard->getNumber());
        slash->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(slash);
        return card;
    }
};

class jianshuTargetMod : public TargetModSkill {
public:
    jianshuTargetMod() : TargetModSkill("#jianshuTargetMod") {
        frequency = NotFrequent;
    }

    virtual int getResidueNum(const Player *from, const Card *card) const{
        if (from->hasSkill("jianshu") && card->isKindOf("NatureSlash"))
            return 1000;
        else
            return 0;
    }

    virtual int getDistanceLimit(const Player *from, const Card *card) const{
        if (from->hasSkill("jianshu") && !card->isKindOf("NatureSlash"))
            return 1000;
        else
            return 0;
    }
};

class jianshuWeapon : public TriggerSkill {
public:
    jianshuWeapon() : TriggerSkill("#jianshu") {
        frequency = Compulsory;
        events << CardsMoveOneTime << EventAcquireSkill;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QList<int> t_ids;
            if (move.to != NULL && move.to == player && move.to_place == Player::PlaceEquip){
                foreach(int id, move.card_ids){
                    if (Sanguosha->getCard(id)->isKindOf("Weapon"))
                        t_ids << id;
                }
                if (t_ids.length() > 0){
                    room->touhouLogmessage("#JianshuUninstall", player, "jianshu");
                    foreach(int id, t_ids){
                        room->throwCard(id, player, player);
                    }
                }
            }
        } else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != "jianshu")
                return false;
            if (player->hasSkill(objectName())){
                QList<int> weapon1;
                foreach(const Card *card, player->getCards("e")) {
                    if (card->isKindOf("Weapon"))
                        weapon1 << card->getId();
                }
                if (weapon1.length() > 0){
                    room->touhouLogmessage("#JianshuUninstall", player, "jianshu");
                    foreach(int id, weapon1){
                        room->throwCard(id, player, player);
                    }
                }
            }

        }
        return false;
    }
};


class louguan : public TriggerSkill {
public:
    louguan() : TriggerSkill("louguan") {
        frequency = Compulsory;
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from == player && use.card != NULL && use.card->isKindOf("Slash") && !use.card->isRed()){
            foreach(ServerPlayer *p, use.to) {
                p->addQinggangTag(use.card);
            }
            room->touhouLogmessage("#TriggerSkill", player, "louguan");
            room->notifySkillInvoked(player, objectName());
            room->setEmotion(player, "weapon/qinggang_sword");

        }
        return false;
    }
};


class bailou : public TriggerSkill {
public:
    bailou() : TriggerSkill("bailou") {
        events << TargetConfirmed;
    }

    virtual bool trigger(TriggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        CardUseStruct use = data.value<CardUseStruct>();
        if (use.from == player && use.card != NULL && use.card->isKindOf("Slash") && !use.card->isBlack()){
            foreach(ServerPlayer *p, use.to) {
                QVariant _data = QVariant::fromValue(p);
                if (player->canDiscard(p, "h") && room->askForSkillInvoke(player, objectName(), _data)){
                    room->setEmotion(player, "weapon/ice_sword");
                    room->throwCard(room->askForCardChosen(player, p, "h", objectName(), false, Card::MethodDiscard), p, player);
                }
            }
        }
        return false;
    }
};
*/





//*************deleted version of xlc003
/*
class jinghua : public TriggerSkill {
public:
    jinghua() : TriggerSkill("jinghua") {
        events << EventPhaseStart;
    }

    virtual bool triggerable(const ServerPlayer *target) const{
        return (target != NULL);
    }

    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{

        ServerPlayer *source = room->findPlayerBySkillName(objectName());
        if (source == NULL)
            return false;
        if (player->getPhase() == Player::Start){
            QList<ServerPlayer *> xx;
            foreach(ServerPlayer *p, room->getAlivePlayers()){
                if (p->getCards("j").length() > 0)
                    xx << p;
            }
            if (xx.length() > 0){
                ServerPlayer * target = room->askForPlayerChosen(source, xx, objectName(), "@targetchoose", true, true);
                if (target != NULL){
                    int card_id = room->askForCardChosen(source, target, "j", objectName());
                    QList<int> card_ids;
                    card_ids << card_id;
                    room->moveCardsToEndOfDrawpile(card_ids);
                    if (player != source)
                        room->loseHp(source, 1);
                }
            }
        }
        return false;
    }
};



class zhengyiEffect : public TriggerSkill {
public:
    zhengyiEffect() : TriggerSkill("#zhengyi") {
        frequency = Compulsory;
        events << TargetConfirming << CardEffected << SlashEffected;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == TargetConfirming){
            CardUseStruct use = data.value<CardUseStruct>();
            if (use.card->isBlack() && (use.card->isNDTrick() || use.card->isKindOf("Slash"))
                && use.to.contains(player)){
                room->setCardFlag(use.card, "zhengyi" + player->objectName());
                if (use.from->isAlive()){
                    room->touhouLogmessage("#TriggerSkill", player, "zhengyi");
                    CardsMoveStruct move;
                    move.to = use.from;
                    move.to_place = Player::PlaceHand;
                    move.card_ids << (room->drawCard(true));//(room->getDrawPile().last());
                    room->moveCardsAtomic(move, false);
                }
            }
        }
        else if (triggerEvent == CardEffected){
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (effect.card->isNDTrick() && effect.card->hasFlag("zhengyi" + effect.to->objectName())){
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.card->objectName(), QList<ServerPlayer *>(), "zhengyi");
                room->notifySkillInvoked(effect.to, "zhengyi");
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            }
        }
        else if (triggerEvent == SlashEffected){
            SlashEffectStruct effect = data.value<SlashEffectStruct>();
            if (effect.slash->hasFlag("zhengyi" + effect.to->objectName())){
                room->touhouLogmessage("#LingqiAvoid", effect.to, effect.slash->objectName(), QList<ServerPlayer *>(), "zhengyi");
                room->notifySkillInvoked(effect.to, "zhengyi");
                room->setEmotion(effect.to, "skill_nullify");
                return true;

            }
        }
        return false;
    }
};

class zhengyiArmor : public TriggerSkill {
public:
    zhengyiArmor() : TriggerSkill("#zhengyiArmor") {
        frequency = Compulsory;
        events << CardsMoveOneTime << EventAcquireSkill;
    }


    virtual bool trigger(TriggerEvent triggerEvent, Room *room, ServerPlayer *player, QVariant &data) const{
        if (triggerEvent == CardsMoveOneTime) {
            CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
            QList<int> t_ids;
            if (move.to != NULL && move.to == player && move.to_place == Player::PlaceEquip){
                foreach(int id, move.card_ids){
                    if (Sanguosha->getCard(id)->isKindOf("Armor")) {
                        t_ids << id;
                    }
                }
                if (t_ids.length() > 0){
                    room->touhouLogmessage("#ZhengyiUninstall", player, "zhengyi");
                    foreach(int id, t_ids){
                        room->throwCard(id, player, player);

                    }
                }
            }
        }
        else if (triggerEvent == EventAcquireSkill) {
            if (data.toString() != "zhengyi")
                return false;
            if (player->hasSkill(objectName())){
                QList<int> t_ids;
                foreach(const Card *card, player->getCards("e")) {
                    if (card->isKindOf("Armor"))
                        t_ids << card->getId();
                }
                if (t_ids.length() > 0){
                    room->touhouLogmessage("#ZhengyiUninstall", player, "zhengyi");
                    foreach(int id, t_ids){
                        room->throwCard(id, player, player);
                    }
                }
            }

        }
        return false;
    }
};

class zhengyi : public FilterSkill {
public:
    zhengyi() : FilterSkill("zhengyi") {
    }

    virtual bool viewFilter(const Card *to_select) const{
        Room *room = Sanguosha->currentRoom();
        if (room->getCardPlace(to_select->getEffectiveId()) == Player::PlaceHand){
            ServerPlayer *xing = room->getCardOwner(to_select->getEffectiveId());
            if (xing != NULL && xing->hasSkill(objectName())){
                return to_select->isKindOf("Armor");
            }
        }
        return false;
    }

    virtual const Card *viewAs(const Card *originalCard) const{
        Nullification *nul = new Nullification(originalCard->getSuit(), originalCard->getNumber());
        nul->setSkillName(objectName());
        WrappedCard *card = Sanguosha->getWrappedCard(originalCard->getId());
        card->takeOver(nul);
        return card;
    }
};
*/

