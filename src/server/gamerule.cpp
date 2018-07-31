#include "gamerule.h"
#include "engine.h"
#include "maneuvering.h"
#include "testCard.h"
#include "room.h"
#include "serverplayer.h"
#include "settings.h"
#include "standard.h"
#include <QTime>

GameRule::GameRule(QObject *)
    : TriggerSkill("game_rule")
{
    //@todo: this setParent is illegitimate in QT and is equivalent to calling
    // setParent(NULL). So taking it off at the moment until we figure out
    // a way to do it.
    //setParent(parent);

    events << GameStart << TurnStart << EventPhaseProceeding << EventPhaseEnd << EventPhaseChanging << PreCardUsed << CardUsed << CardFinished << CardEffected << PostHpReduced
           << EventLoseSkill << EventAcquireSkill << AskForPeaches << AskForPeachesDone << BuryVictim << GameOverJudge << SlashHit << SlashEffected << SlashProceed << ConfirmDamage
           << DamageDone << DamageComplete << StartJudge << FinishRetrial << FinishJudge << ChoiceMade << BeforeCardsMove << EventPhaseStart << JinkEffect;
}

int GameRule::getPriority() const
{
    return 0;
}

QList<SkillInvokeDetail> GameRule::triggerable(TriggerEvent, const Room *, const QVariant &) const
{
    return QList<SkillInvokeDetail>() << SkillInvokeDetail(this, NULL, NULL, NULL, true);
}

void GameRule::onPhaseProceed(ServerPlayer *player) const
{
    Room *room = player->getRoom();
    switch (player->getPhase()) {
    case Player::PhaseNone: {
        Q_ASSERT(false);
    }
    case Player::RoundStart: {
        break;
    }
    case Player::Start: {
        break;
    }
    case Player::Judge: {
        QList<const Card *> tricks = player->getJudgingArea();
        QList<const Card *> effected;
        while (!tricks.isEmpty() && player->isAlive()) {
            //Skill "jingxia" can discard tricks in this phase
            tricks = player->getJudgingArea();
            //the effected card may return to judgingArea, such as double Linghting by skill "tianqu"
            foreach (const Card *card, effected) {
                if (tricks.contains(card))
                    tricks.removeOne(card);
            }
            if (tricks.length() == 0)
                break;
            const Card *trick = tricks.takeLast();

            bool on_effect = room->cardEffect(trick, NULL, player);
            effected << trick;
            if (!on_effect)
                trick->onNullified(player);
        }
        break;
    }
    case Player::Draw: {
        int num = 2;
        if (player->hasFlag("Global_FirstRound")) {
            room->setPlayerFlag(player, "-Global_FirstRound");
            if (room->getMode() == "02_1v1")
                num--;
        }
        DrawNCardsStruct s;
        s.player = player;
        s.isInitial = false;
        s.n = num;
        QVariant qnum = QVariant::fromValue(s);
        Q_ASSERT(room->getThread() != NULL);
        room->getThread()->trigger(DrawNCards, room, qnum);
        s = qnum.value<DrawNCardsStruct>();
        num = s.n;
        if (num > 0)
            player->drawCards(num);
        s.n = num;
        qnum = QVariant::fromValue(s);
        room->getThread()->trigger(AfterDrawNCards, room, qnum);
        break;
    }
    case Player::Play: {
        while (player->isAlive()) {
            CardUseStruct card_use;
            room->activate(player, card_use);
            if (card_use.card != NULL)
                room->useCard(card_use);
            else
                break;
        }
        break;
    }
    case Player::Discard: {
        int discard_num = 0; //discard in one time
        discard_num = player->getHandcardNum() - player->getMaxCards();
        if (discard_num > 0)
            room->askForDiscard(player, "gamerule", discard_num, discard_num);
        break;
    }
    case Player::Finish: {
        break;
    }
    case Player::NotActive: {
        break;
    }
    }
}

bool GameRule::effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail>, QVariant &data) const
{
    if (room->getTag("SkipGameRule").toBool()) {
        room->removeTag("SkipGameRule");
        return false;
    }

    switch (triggerEvent) {
    case GameStart: {
        if (data.isNull()) {
            foreach (ServerPlayer *player, room->getPlayers()) {
                Q_ASSERT(player->getGeneral() != NULL);
                if ((player->getGeneral()->getKingdom() == "zhu" || player->getGeneral()->getKingdom() == "touhougod") && player->getGeneralName() != "anjiang") {
                    QString new_kingdom = room->askForKingdom(player);
                    room->setPlayerProperty(player, "kingdom", new_kingdom);

                    LogMessage log;
                    log.type = "#ChooseKingdom";
                    log.from = player;
                    log.arg = new_kingdom;
                    room->sendLog(log);
                }
                foreach (const Skill *skill, player->getVisibleSkillList()) {
                    if (skill->getFrequency() == Skill::Limited && !skill->getLimitMark().isEmpty() && (!skill->isLordSkill() || player->hasLordSkill(skill->objectName())))
                        //room->addPlayerMark(player, skill->getLimitMark());
                        room->setPlayerMark(player, skill->getLimitMark(), 1);
                }
            }
            room->setTag("FirstRound", true);
            bool kof_mode = room->getMode() == "02_1v1" && Config.value("1v1/Rule", "2013").toString() != "Classical";
            QList<DrawNCardsStruct> s_list;
            foreach (ServerPlayer *p, room->getPlayers()) {
                int n = kof_mode ? p->getMaxHp() : 4;
                DrawNCardsStruct s;
                s.player = p;
                s.isInitial = true;
                s.n = n;
                QVariant data = QVariant::fromValue(s);
                room->getThread()->trigger(DrawInitialCards, room, data);
                s_list << data.value<DrawNCardsStruct>();
            }
            QList<int> n_list;
            foreach (DrawNCardsStruct s, s_list)
                n_list << s.n;
            room->drawCards(room->getPlayers(), n_list, "initialDraw");
            if (Config.LuckCardLimitation > 0)
                room->askForLuckCard();
            foreach (DrawNCardsStruct s, s_list) {
                QVariant _slistati = QVariant::fromValue(s);
                room->getThread()->trigger(AfterDrawInitialCards, room, _slistati);
            }
        }
        break;
    }
    case TurnStart: {
        ServerPlayer *player = room->getCurrent();
        if (player == NULL)
            return false;
        if (room->getTag("FirstRound").toBool()) {
            room->setTag("FirstRound", false);
            room->setPlayerFlag(player, "Global_FirstRound");
        }

        if (player->isLord() && !player->tag.value("touhou-extra", false).toBool()) {
            QString mode = Config.GameMode;
            if (!mode.endsWith("1v1") && !mode.endsWith("1v3") && mode != "06_XMode")
                room->setTag("Global_RoundCount", room->getTag("Global_RoundCount").toInt() + 1);
        }

        LogMessage log;
        log.type = "$AppendSeparator";
        room->sendLog(log);
        room->addPlayerMark(player, "Global_TurnCount");

        QList<Player::Phase> set_phases;
        ExtraTurnStruct extra = player->tag["ExtraTurnInfo"].value<ExtraTurnStruct>();
        if (!extra.set_phases.isEmpty())
            set_phases = extra.set_phases;
        //clear other's extraTurn infomation
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            p->tag.remove("ExtraTurnInfo");

        if (!player->faceUp()) {
            room->setPlayerFlag(player, "-Global_FirstRound");
            player->turnOver();
        } else if (player->isAlive()) {
            if (set_phases.isEmpty())
                player->play();
            else
                player->play(set_phases);
        }
        break;
    }
    case EventPhaseProceeding: {
        ServerPlayer *player = data.value<ServerPlayer *>();
        onPhaseProceed(player);
        break;
    }
    case EventPhaseStart: {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if (current && current->getPhase() == Player::Finish && !current->getBrokenEquips().isEmpty())
            current->removeBrokenEquips(current->getBrokenEquips());
        break;
    }
    case EventPhaseEnd: {
        /*foreach (ServerPlayer *p, room->getAllPlayers()) {
            if (p->getMark("drank") > 0) {
                LogMessage log;
                log.type = "#UnsetDrankEndOfTurn";
                log.from = p;
                room->sendLog(log);

                room->setPlayerMark(p, "drank", 0);
            }
            if (p->getMark("magic_drank") > 0) {
                LogMessage log;
                log.type = "#UnsetDrankEndOfTurn";
                log.from = p;
                room->sendLog(log);

                room->setPlayerMark(p, "magic_drank", 0);
            }
        }*/
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == Player::Play)
            room->addPlayerHistory(player, ".");
        break;
    }
    case EventPhaseChanging: {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        ServerPlayer *player = change.player;
        if (change.to == Player::NotActive) {
            foreach(ServerPlayer *p, room->getAllPlayers()) {
                if (p->getMark("drank") > 0) {
                    LogMessage log;
                    log.type = "#UnsetDrankEndOfTurn";
                    log.from = p;
                    room->sendLog(log);

                    room->setPlayerMark(p, "drank", 0);
                }
                if (p->getMark("magic_drank") > 0) {
                    LogMessage log;
                    log.type = "#UnsetDrankEndOfTurn";
                    log.from = p;
                    room->sendLog(log);

                    room->setPlayerMark(p, "magic_drank", 0);
                }
            }

            room->setPlayerFlag(player, ".");
            //room->clearPlayerCardLimitation(player, true);
            room->setPlayerMark(player, "touhou-extra", 0);

            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                room->clearPlayerCardLimitation(p, true);
                QMap<QString, int> marks = p->getMarkMap();
                QMap<QString, int>::iterator it;
                for (it = marks.begin(); it != marks.end(); ++it) {
                    if (it.value() > 0 && it.key().endsWith("_SingleTurn"))
                        room->setPlayerMark(player, it.key(), 0);
                }

                foreach (QString flag, p->getFlagList()) {
                    if (flag.endsWith("Animate"))
                        room->setPlayerFlag(p, "-" + flag);
                }
            }
        } else if (change.to == Player::Play) {
            room->addPlayerHistory(player, ".");
        }

        break;
    }
    case PreCardUsed: {
        if (data.canConvert<CardUseStruct>()) {
            CardUseStruct card_use = data.value<CardUseStruct>();
            if (card_use.from->hasFlag("Global_ForbidSurrender")) {
                card_use.from->setFlags("-Global_ForbidSurrender");
                room->doNotify(card_use.from, QSanProtocol::S_COMMAND_ENABLE_SURRENDER, QVariant(true));
            }

            card_use.from->broadcastSkillInvoke(card_use.card);
            if (!card_use.card->getSkillName().isNull() && card_use.card->getSkillName(true) == card_use.card->getSkillName(false) && card_use.m_isOwnerUse
                && card_use.from->hasSkill(card_use.card->getSkillName()))
                room->notifySkillInvoked(card_use.from, card_use.card->getSkillName());
            //clear magic_drank while using Nullification
            if (card_use.card->isKindOf("Nullification")) {
                if (card_use.from->getMark("magic_drank") > 0)
                    room->setPlayerMark(card_use.from, "magic_drank", 0);
            }

        }
        break;
    }
    case CardUsed: {
        if (data.canConvert<CardUseStruct>()) {
            CardUseStruct card_use = data.value<CardUseStruct>();
            RoomThread *thread = room->getThread();

            if (card_use.card->hasPreAction())
                card_use.card->doPreAction(room, card_use);

            if (card_use.from) {
                thread->trigger(TargetSpecifying, room, data);
                card_use = data.value<CardUseStruct>();
            }

            if (!card_use.to.isEmpty()) {
                thread->trigger(TargetConfirming, room, data);
                card_use = data.value<CardUseStruct>();
            }

            /*if (card_use.card && !(card_use.card->isVirtualCard() && card_use.card->getSubcards().isEmpty()) && !card_use.card->targetFixed() && card_use.to.isEmpty()) {
                    if (room->getCardPlace(card_use.card->getEffectiveId()) == Player::PlaceTable) {
                        CardMoveReason reason(CardMoveReason::S_REASON_NATURAL_ENTER, QString());
                        room->throwCard(card_use.card, reason, NULL);
                        break;
                    }
                }*/
            //1) exclude SkillCard 2)changed move reason (USE) 3)keep extraData
            if (card_use.card && card_use.card->getTypeId() != Card::TypeSkill && !(card_use.card->isVirtualCard() && card_use.card->getSubcards().isEmpty())
                && card_use.to.isEmpty()) {
                if (room->getCardPlace(card_use.card->getEffectiveId()) == Player::PlaceTable) {
                    CardMoveReason reason(CardMoveReason::S_REASON_USE, card_use.from->objectName(), QString(), card_use.card->getSkillName(), QString());
                    reason.m_extraData = QVariant::fromValue(card_use.card);
                    room->moveCardTo(card_use.card, card_use.from, NULL, Player::DiscardPile, reason, true);
                    //break;
                }
            }
            //since use.to is empty, break the whole process
            if (card_use.card && card_use.card->getTypeId() != Card::TypeSkill && card_use.to.isEmpty())
                break;

            try {
                QVariantList jink_list_backup;
                if (card_use.card->isKindOf("Slash")) {
                    jink_list_backup = card_use.from->tag["Jink_" + card_use.card->toString()].toList();
                    QVariantList jink_list;
                    int jink_num = 1;
                    if (card_use.card->hasFlag("ZeroJink"))
                        jink_num = 0;
                    for (int i = 0; i < card_use.to.length(); i++)
                        jink_list.append(QVariant(jink_num));
                    card_use.from->tag["Jink_" + card_use.card->toString()] = QVariant::fromValue(jink_list);
                }
                if (card_use.from && !card_use.to.isEmpty()) {
                    thread->trigger(TargetSpecified, room, data);
                    thread->trigger(TargetConfirmed, room, data);
                }
                card_use = data.value<CardUseStruct>();
                room->setTag("CardUseNullifiedList", QVariant::fromValue(card_use.nullified_list));
                if (card_use.card->isNDTrick() && !card_use.card->isKindOf("Nullification")) // && !card_use.to.isEmpty()
                    room->setCardFlag(card_use.card, "LastTrickTarget_" + card_use.to.last()->objectName());

                card_use.card->use(room, card_use.from, card_use.to);
                if (!jink_list_backup.isEmpty())
                    card_use.from->tag["Jink_" + card_use.card->toString()] = QVariant::fromValue(jink_list_backup);
            } catch (TriggerEvent triggerEvent) {
                if (triggerEvent == TurnBroken)
                    card_use.from->tag.remove("Jink_" + card_use.card->toString());
                throw triggerEvent;
            }
        }

        break;
    }
    case CardFinished: {
        CardUseStruct use = data.value<CardUseStruct>();
        room->clearCardFlag(use.card);

        if (use.card->isKindOf("AOE") || use.card->isKindOf("GlobalEffect")) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                room->doNotify(p, QSanProtocol::S_COMMAND_NULLIFICATION_ASKED, QVariant("."));
        }

        if (use.card->isKindOf("Slash"))
            use.from->tag.remove("Jink_" + use.card->toString());

        break;
    }
    case EventAcquireSkill:
    case EventLoseSkill: {
        SkillAcquireDetachStruct s = data.value<SkillAcquireDetachStruct>();

        const Skill *skill = s.skill;
        bool refilter = skill->inherits("FilterSkill");
        if (refilter)
            room->filterCards(s.player, s.player->getCards("hes"), triggerEvent == EventLoseSkill);

        break;
    }
    case PostHpReduced: {
        ServerPlayer *player = NULL;
        if (data.canConvert<DamageStruct>())
            player = data.value<DamageStruct>().to;
        else if (data.canConvert<HpLostStruct>())
            player = data.value<HpLostStruct>().player;
        if (player == NULL)
            break;

        if (player->getHp() >= player->dyingThreshold())
            break;

        if (data.canConvert<DamageStruct>()) {
            DamageStruct damage = data.value<DamageStruct>();
            room->enterDying(player, &damage);
        } else
            room->enterDying(player, NULL);

        break;
    }
    case AskForPeaches: {
        DyingStruct dying = data.value<DyingStruct>();
        const Card *peach = NULL;
        int threshold = dying.who->dyingThreshold();

        while (dying.who->getHp() < threshold) {
            peach = NULL;

            if (dying.who->isAlive())
                peach = room->askForSinglePeach(dying.nowAskingForPeaches, dying.who);

            if (peach == NULL)
                break;
            room->useCard(CardUseStruct(peach, dying.nowAskingForPeaches, dying.who), false);
            threshold = dying.who->dyingThreshold();
        }
        break;
    }
    case AskForPeachesDone: {
        DyingStruct dying = data.value<DyingStruct>();
        int threshold = dying.who->dyingThreshold();
        if (dying.who->getHp() < threshold && dying.who->isAlive())
            room->killPlayer(dying.who, dying.damage);

        break;
    }
    case ConfirmDamage: {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.card && damage.to->getMark("SlashIsDrank") > 0) {
            LogMessage log;
            log.type = "#AnalepticBuff";
            log.from = damage.from;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);

            damage.damage += damage.to->getMark("SlashIsDrank");
            damage.to->setMark("SlashIsDrank", 0);

            log.arg2 = QString::number(damage.damage);

            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        break;
    }
    case DamageDone: {
        DamageStruct damage = data.value<DamageStruct>();
        if (damage.from && !damage.from->isAlive())
            damage.from = NULL;
        data = QVariant::fromValue(damage);
        room->sendDamageLog(damage);

        room->applyDamage(damage.to, damage);
        if ((damage.nature != DamageStruct::Normal) && damage.to->isChained() && !damage.chain) {
            damage.trigger_chain = true;
            data = QVariant::fromValue(damage);
        }
        room->getThread()->trigger(PostHpReduced, room, data);

        break;
    }
    case DamageComplete: {
        DamageStruct damage = data.value<DamageStruct>();

        if ((damage.nature != DamageStruct::Normal) && damage.to->isChained())
            room->setPlayerProperty(damage.to, "chained", false);

        if (damage.trigger_chain) {
            if ((damage.nature != DamageStruct::Normal) && !damage.chain) {
                QList<ServerPlayer *> chained_players;
                if (room->getCurrent()->isDead())
                    chained_players = room->getOtherPlayers(room->getCurrent());
                else
                    chained_players = room->getAllPlayers();
                foreach (ServerPlayer *chained_player, chained_players) {
                    if (chained_player->isChained()) {
                        room->getThread()->delay();
                        LogMessage log;
                        log.type = "#IronChainDamage";
                        log.from = chained_player;
                        room->sendLog(log);

                        DamageStruct chain_damage = damage;
                        chain_damage.to = chained_player;
                        chain_damage.chain = true;

                        room->damage(chain_damage);
                    }
                }
            }
        }
        if (room->getMode() == "02_1v1" || room->getMode() == "06_XMode") {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasFlag("Global_DebutFlag")) {
                    p->setFlags("-Global_DebutFlag");
                    if (room->getMode() == "02_1v1") {
                        QVariant v = QVariant::fromValue(p);
                        room->getThread()->trigger(Debut, room, v);
                    }
                }
            }
        }
        break;
    }
    case CardEffected: {
        if (data.canConvert<CardEffectStruct>()) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (!effect.card->isKindOf("Slash") && effect.nullified) {
                LogMessage log;
                log.type = "#CardNullified";
                log.from = effect.to;
                log.arg = effect.card->objectName();
                room->sendLog(log);
                room->setEmotion(effect.to, "skill_nullify");
                return true;
            } else if (effect.card->getTypeId() == Card::TypeTrick) {
                if (effect.card->isNDTrick() && effect.from->getMark("magic_drank") > 0) {
                    //room->setCardFlag(effect.card, "magic_drank");
                    effect.effectValue.first() = effect.effectValue.first() + effect.from->getMark("magic_drank");
                    room->setPlayerMark(effect.from, "magic_drank", 0);
                }
                if (room->isCanceled(effect)) {
                    effect.to->setFlags("Global_NonSkillNullify");
                    return true;
                } else {
                    room->getThread()->trigger(TrickEffect, room, data);
                    
                }
            }
            if (effect.to->isAlive() || effect.card->isKindOf("Slash")) {
                //record for skill tianxie
                if (!effect.card->isKindOf("Slash"))
                    room->setCardFlag(effect.card, "tianxieEffected_" + effect.to->objectName());
                //do chunhua effect
                if (effect.card->hasFlag("chunhua") && !effect.card->isKindOf("Slash")) {
                    room->touhouLogmessage("#Chunhua", effect.to, effect.card->objectName());
                    //if (effect.card->isBlack()) {
                    if (effect.card->hasFlag("chunhua_black")) {
                        DamageStruct d = DamageStruct(effect.card, effect.from, effect.to, 1 + effect.effectValue.first(), DamageStruct::Normal);
                        room->damage(d);
                    } else if (effect.card->hasFlag("chunhua_red")) {
                        RecoverStruct recover;
                        recover.card = effect.card;
                        recover.who = effect.from;
                        recover.recover = 1 + effect.effectValue.first();
                        room->recover(effect.to, recover);
                    }
                } else
                    effect.card->onEffect(effect);
            }
        }

        break;
    }
    case SlashEffected: {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.nullified) {
            LogMessage log;
            log.type = "#CardNullified";
            log.from = effect.to;
            log.arg = effect.slash->objectName();
            room->sendLog(log);
            room->setEmotion(effect.to, "skill_nullify");
            return true;
        }

        QVariant data = QVariant::fromValue(effect);
        //if (effect.jink_num > 0)
        room->getThread()->trigger(SlashProceed, room, data);
        //else
        //    room->slashResult(effect, NULL);
        break;
    }
    case SlashProceed: {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        QString slasher = effect.from->objectName();
        if (!effect.to->isAlive())
            break;

        //process skill cancel, like kaungluan.
        if (room->getThread()->trigger(Cancel, room, data)) {
            effect = data.value<SlashEffectStruct>();
            room->slashResult(effect, NULL);
            break;
        }

        if (effect.jink_num == 0) {
            room->slashResult(effect, NULL);
            break;
        }
        if (effect.jink_num == 1) {
            const Card *jink = room->askForCard(effect.to, "jink", "slash-jink:" + slasher, data, Card::MethodUse, effect.from);
            room->slashResult(effect, room->isJinkEffected(effect, jink) ? jink : NULL);
        } else {
            DummyCard *jink = new DummyCard;
            // Since GameRule is created by RoomThread not Engine, and this function also runs at RoomThread not main thread, so this jink must be on the RoomThread
            // Because the RoomThread has no event loop, so a deleteLater is absolutely safe for it can be deleted only by the time of the deletion of RoomThread
            jink->deleteLater();
            const Card *asked_jink = NULL;
            for (int i = effect.jink_num; i > 0; i--) {
                QString prompt = QString("@multi-jink%1:%2::%3").arg(i == effect.jink_num ? "-start" : QString()).arg(slasher).arg(i);
                asked_jink = room->askForCard(effect.to, "jink", prompt, data, Card::MethodUse, effect.from);
                if (!room->isJinkEffected(effect, asked_jink)) {
                    delete jink;
                    room->slashResult(effect, NULL);
                    return false;
                } else {
                    jink->addSubcard(asked_jink->getEffectiveId());
                }
            }
            room->slashResult(effect, jink);
        }

        break;
    }
    case JinkEffect: {
        JinkEffectStruct j = data.value<JinkEffectStruct>();
        if (j.jink != NULL && j.jink->isKindOf("NatureJink")) {
            SlashEffectStruct effect = j.slashEffect;
            //process advanced_jink
            if (effect.from && effect.to && effect.from->isAlive() && effect.to->isAlive()) {
                CardEffectStruct new_effect;
                new_effect.card = j.jink;
                new_effect.from = effect.to;
                new_effect.to = effect.from;
                j.jink->onEffect(new_effect);
            }
        }

        break;
    }
    case SlashHit: {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        //do chunhua effect
        if (effect.slash->hasFlag("chunhua")) {
            room->touhouLogmessage("#Chunhua", effect.to, effect.slash->objectName());
            effect.nature = DamageStruct::Normal;
            if (effect.slash->hasFlag("chunhua_red")) {
                RecoverStruct recover;
                recover.card = effect.slash;
                recover.who = effect.from;
                recover.recover = 1 + effect.effectValue.first();
                room->recover(effect.to, recover);
                break;
            }
        }

        //@todo: I want IronSlash to obtain function debuffEffect() from "Slash"  as an inheritance, But the variant slasheffect.slash is "Card".
        //using dynamic_cast may bring some terrible troubles.
        if (effect.slash->isKindOf("DebuffSlash")) {
            if (effect.slash->isKindOf("IronSlash"))
                IronSlash::debuffEffect(effect);
            else if(effect.slash->isKindOf("LightSlash"))
                LightSlash::debuffEffect(effect);
            else if (effect.slash->isKindOf("PowerSlash"))
                PowerSlash::debuffEffect(effect);
        }

        if (effect.drank > 0)
            effect.to->setMark("SlashIsDrank", effect.drank);

        DamageStruct d = DamageStruct(effect.slash, effect.from, effect.to, 1 + effect.effectValue.last(), effect.nature);
        foreach (ServerPlayer *p, room->getAllPlayers(true)) {
            if (effect.slash->hasFlag("WushenDamage_" + p->objectName())) {
                d.from = p->isAlive() ? p : NULL;
                d.by_user = false;
                break;
            }
        }
        room->damage(d);
        //room->damage(DamageStruct(effect.slash, effect.from, effect.to, 1, effect.nature));

        break;
    }
    case GameOverJudge: {
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *player = death.who;
        if (room->getMode() == "02_1v1") {
            QStringList list = player->tag["1v1Arrange"].toStringList();
            QString rule = Config.value("1v1/Rule", "2013").toString();
            if (list.length() > ((rule == "OL") ? 3 : 0))
                break;
        }

        QString winner = getWinner(player);
        if (!winner.isNull()) {
            room->gameOver(winner);
            return true;
        }
        break;
    }
    case BuryVictim: {
        DeathStruct death = data.value<DeathStruct>();
        bool skipRewardAndPunish = death.who->hasFlag("skipRewardAndPunish") ? true : false;
        death.who->bury();

        if (room->getTag("SkipNormalDeathProcess").toBool())
            return false;

        ServerPlayer *killer = NULL;
        if (death.useViewAsKiller)
            killer = death.viewAsKiller;
        else if (death.damage)
            killer = death.damage->from;

        if (killer && !skipRewardAndPunish)
            rewardAndPunish(killer, death.who);

        if (room->getMode() == "02_1v1") {
            QStringList list = death.who->tag["1v1Arrange"].toStringList();
            QString rule = Config.value("1v1/Rule", "2013").toString();
            if (list.length() <= ((rule == "OL") ? 3 : 0))
                break;

            if (rule == "Classical") {
                death.who->tag["1v1ChangeGeneral"] = list.takeFirst();
                death.who->tag["1v1Arrange"] = list;
            } else {
                death.who->tag["1v1ChangeGeneral"] = list.first();
            }

            changeGeneral1v1(death.who);
            if (death.damage == NULL) {
                QVariant v = QVariant::fromValue(death.who);
                room->getThread()->trigger(Debut, room, v);
            } else
                death.who->setFlags("Global_DebutFlag");
            return false;
        } else if (room->getMode() == "06_XMode") {
            changeGeneralXMode(death.who);
            if (death.damage != NULL)
                death.who->setFlags("Global_DebutFlag");
            return false;
        }
        break;
    }
    case StartJudge: {
        int card_id = room->drawCard();

        JudgeStruct *judge = data.value<JudgeStruct *>();
        judge->card = Sanguosha->getCard(card_id);

        LogMessage log;
        log.type = "$InitialJudge";
        log.from = judge->who;
        log.card_str = QString::number(judge->card->getEffectiveId());
        room->sendLog(log);

        room->moveCardTo(judge->card, NULL, judge->who, Player::PlaceJudge,
                         CardMoveReason(CardMoveReason::S_REASON_JUDGE, judge->who->objectName(), QString(), QString(), judge->reason), true);
        judge->updateResult();
        break;
    }
    case FinishRetrial: {
        JudgeStruct *judge = data.value<JudgeStruct *>();

        LogMessage log;
        log.type = "$JudgeResult";
        log.from = judge->who;
        log.card_str = QString::number(judge->card->getEffectiveId());
        room->sendLog(log);

        int delay = Config.AIDelay;
        if (judge->time_consuming)
            delay /= 1.25;
        Q_ASSERT(room->getThread() != NULL);
        room->getThread()->delay(delay);
        if (judge->play_animation) {
            room->sendJudgeResult(judge);
            room->getThread()->delay(Config.S_JUDGE_LONG_DELAY);
        }

        break;
    }
    case FinishJudge: {
        JudgeStruct *judge = data.value<JudgeStruct *>();

        if (room->getCardPlace(judge->card->getEffectiveId()) == Player::PlaceJudge) {
            CardMoveReason reason(CardMoveReason::S_REASON_JUDGEDONE, judge->who->objectName(), QString(), judge->reason);
            if (judge->retrial_by_response) {
                reason.m_extraData = QVariant::fromValue(judge->retrial_by_response);
            }
            room->moveCardTo(judge->card, judge->who, NULL, Player::DiscardPile, reason, true);
        }

        break;
    }
    case ChoiceMade: {
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            foreach (QString flag, p->getFlagList()) {
                if (flag.startsWith("Global_") && flag.endsWith("Failed"))
                    room->setPlayerFlag(p, "-" + flag);
            }
        }
        break;
    }
    case BeforeCardsMove: { //to be record? not effect
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
        if (player != NULL) {
            QList<int> shownIds;
            foreach (int id, move.card_ids) {
                if (player->isShownHandcard(id))
                    shownIds << id;
            }
            if (!shownIds.isEmpty()) {
                player->removeShownHandCards(shownIds, false, true);
                move.shown_ids = shownIds;
                data = QVariant::fromValue(move);
            }

            QList<int> brokenIds;
            foreach (int id, move.card_ids) {
                if (player->isBrokenEquip(id))
                    brokenIds << id;
            }
            if (!brokenIds.isEmpty()) {
                player->removeBrokenEquips(brokenIds, false, true);
                move.broken_ids = brokenIds;
                data = QVariant::fromValue(move);
            }
        }
        break;
    }
    default:
        break;
    }
    return false;
}

void GameRule::changeGeneral1v1(ServerPlayer *player) const
{
    Config.AIDelay = Config.OriginAIDelay;

    Room *room = player->getRoom();
    bool classical = (Config.value("1v1/Rule", "2013").toString() == "Classical");
    QString new_general;
    if (classical) {
        new_general = player->tag["1v1ChangeGeneral"].toString();
        player->tag.remove("1v1ChangeGeneral");
    } else {
        QStringList list = player->tag["1v1Arrange"].toStringList();
        if (player->getAI())
            new_general = list.first();
        else
            new_general = room->askForGeneral(player, list);
        list.removeOne(new_general);
        player->tag["1v1Arrange"] = QVariant::fromValue(list);
    }

    if (player->getPhase() != Player::NotActive) {
        player->setPhase(Player::NotActive);
        room->broadcastProperty(player, "phase");
    }
    room->revivePlayer(player);
    room->changeHero(player, new_general, true, true);
    Q_ASSERT(player->getGeneral() != NULL);
    if (player->getGeneral()->getKingdom() == "zhu" || player->getGeneral()->getKingdom() == "touhougod") {
        QString new_kingdom = room->askForKingdom(player);
        room->setPlayerProperty(player, "kingdom", new_kingdom);

        LogMessage log;
        log.type = "#ChooseKingdom";
        log.from = player;
        log.arg = new_kingdom;
        room->sendLog(log);
    }
    room->addPlayerHistory(player, ".");

    if (player->getKingdom() != player->getGeneral()->getKingdom())
        room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());

    QList<ServerPlayer *> notified = classical ? room->getOtherPlayers(player, true) : room->getPlayers();
    room->doBroadcastNotify(notified, QSanProtocol::S_COMMAND_REVEAL_GENERAL, JsonArray() << player->objectName() << new_general);

    if (!player->faceUp())
        player->turnOver();

    if (player->isChained())
        room->setPlayerProperty(player, "chained", false);

    room->setTag("FirstRound", true);
    int draw_num = classical ? 4 : player->getMaxHp();

    DrawNCardsStruct s;
    s.player = player;
    s.isInitial = true;
    s.n = draw_num;

    QVariant data = QVariant::fromValue(s);
    room->getThread()->trigger(DrawInitialCards, room, data);
    s = data.value<DrawNCardsStruct>();
    draw_num = s.n;

    try {
        player->drawCards(draw_num, "initialDraw");
        room->setTag("FirstRound", false);
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            room->setTag("FirstRound", false);
        throw triggerEvent;
    }
    room->getThread()->trigger(AfterDrawInitialCards, room, data);
}

void GameRule::changeGeneralXMode(ServerPlayer *player) const
{
    Config.AIDelay = Config.OriginAIDelay;

    Room *room = player->getRoom();
    ServerPlayer *leader = player->tag["XModeLeader"].value<ServerPlayer *>();
    Q_ASSERT(leader);
    QStringList backup = leader->tag["XModeBackup"].toStringList();
    QString general = room->askForGeneral(leader, backup);
    if (backup.contains(general))
        backup.removeOne(general);
    else
        backup.takeFirst();
    leader->tag["XModeBackup"] = QVariant::fromValue(backup);
    room->revivePlayer(player);
    room->changeHero(player, general, true, true);
    Q_ASSERT(player->getGeneral() != NULL);
    if (player->getGeneral()->getKingdom() == "zhu" || player->getGeneral()->getKingdom() == "touhougod") {
        QString new_kingdom = room->askForKingdom(player);
        room->setPlayerProperty(player, "kingdom", new_kingdom);

        LogMessage log;
        log.type = "#ChooseKingdom";
        log.from = player;
        log.arg = new_kingdom;
        room->sendLog(log);
    }
    room->addPlayerHistory(player, ".");

    if (player->getKingdom() != player->getGeneral()->getKingdom())
        room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());

    if (!player->faceUp())
        player->turnOver();

    if (player->isChained())
        room->setPlayerProperty(player, "chained", false);

    room->setTag("FirstRound", true);
    DrawNCardsStruct s;
    s.player = player;
    s.isInitial = true;
    s.n = 4;
    QVariant data = QVariant::fromValue(s);
    room->getThread()->trigger(DrawInitialCards, room, data);
    s = data.value<DrawNCardsStruct>();
    int num = s.n;
    try {
        player->drawCards(num, "initialDraw");
        room->setTag("FirstRound", false);
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            room->setTag("FirstRound", false);
        throw triggerEvent;
    }
    room->getThread()->trigger(AfterDrawInitialCards, room, data);
}

void GameRule::rewardAndPunish(ServerPlayer *killer, ServerPlayer *victim) const
{
    Q_ASSERT(killer->getRoom() != NULL);
    Room *room = killer->getRoom();
    if (killer->isDead() || room->getMode() == "06_XMode")
        return;

    if (killer->getRoom()->getMode() == "06_3v3") {
        if (Config.value("3v3/OfficialRule", "2013").toString().startsWith("201"))
            killer->drawCards(2);
        else
            killer->drawCards(3);
    } else {
        if (victim->getRole() == "rebel" && killer != victim)
            killer->drawCards(3);
        else if (victim->getRole() == "loyalist" && killer->getRole() == "lord")
            killer->throwAllHandCardsAndEquips();
    }
}

QString GameRule::getWinner(ServerPlayer *victim) const
{
    Room *room = victim->getRoom();
    QString winner;

    if (room->getMode() == "06_3v3") {
        switch (victim->getRoleEnum()) {
        case Player::Lord:
            winner = "renegade+rebel";
            break;
        case Player::Renegade:
            winner = "lord+loyalist";
            break;
        default:
            break;
        }
    } else if (room->getMode() == "06_XMode") {
        QString role = victim->getRole();
        ServerPlayer *leader = victim->tag["XModeLeader"].value<ServerPlayer *>();
        if (leader->tag["XModeBackup"].toStringList().isEmpty()) {
            if (role.startsWith('r'))
                winner = "lord+loyalist";
            else
                winner = "renegade+rebel";
        }
    } else {
        if (victim->getMark("tianxiang") > 0) { //lord skill tianxiang need change gameover judge
            QStringList alive_roles = room->aliveRoles(victim);
            switch (victim->getRoleEnum()) {
            case Player::Lord: {
                if (!alive_roles.contains("loyalist") && !alive_roles.contains("renegade"))
                    winner = "rebel";
                if (!alive_roles.contains("loyalist") && !alive_roles.contains("rebel")) {
                    if (alive_roles.length() == 1)
                        winner = room->getAlivePlayers().first()->objectName();
                }
                break;
            }
            case Player::Rebel: {
                if (!alive_roles.contains("renegade") && !alive_roles.contains("rebel"))
                    winner = "lord+loyalist";
                if (!alive_roles.contains("loyalist") && !alive_roles.contains("rebel") && !alive_roles.contains("lord")) {
                    if (alive_roles.length() == 1)
                        winner = room->getAlivePlayers().first()->objectName();
                }
                break;
            }
            case Player::Renegade: {
                if (!alive_roles.contains("rebel") && !alive_roles.contains("renegade"))
                    winner = "lord+loyalist";
                if (!alive_roles.contains("lord") && !alive_roles.contains("loyalist"))
                    winner = "rebel";
                if (!alive_roles.contains("loyalist") && !alive_roles.contains("rebel") && !alive_roles.contains("lord")) {
                    if (alive_roles.length() == 1)
                        winner = room->getAlivePlayers().first()->objectName();
                }
                break;
            }
            case Player::Loyalist: {
                if (!alive_roles.contains("loyalist") && !alive_roles.contains("rebel") && !alive_roles.contains("lord")) {
                    if (alive_roles.length() == 1)
                        winner = room->getAlivePlayers().first()->objectName();
                }
                if (!alive_roles.contains("lord") && !alive_roles.contains("loyalist") && !alive_roles.contains("renegade"))
                    winner = "rebel";
                break;
            }
            default:
                break;
            }
        } else { //normal case (without the effect of lord skill tianxiang)
            QStringList alive_roles = room->aliveRoles(victim);
            switch (victim->getRoleEnum()) {
            case Player::Lord: {
                if (alive_roles.length() == 1 && alive_roles.first() == "renegade")
                    winner = room->getAlivePlayers().first()->objectName();
                else
                    winner = "rebel";
                break;
            }
            case Player::Rebel:
            case Player::Renegade: {
                if (!alive_roles.contains("rebel") && !alive_roles.contains("renegade")) {
                    winner = "lord+loyalist";
                    if (victim->getRole() == "renegade" && !alive_roles.contains("loyalist"))
                        room->setTag("RenegadeInFinalPK", true);
                }
                break;
            }
            default:
                break;
            }
        }
    }

    return winner;
}

HulaoPassMode::HulaoPassMode(QObject *parent)
    : GameRule(parent)
{
    setObjectName("hulaopass_mode");
}

bool HulaoPassMode::effect(TriggerEvent triggerEvent, Room *room, QSharedPointer<SkillInvokeDetail> invoke, QVariant &data) const
{
    switch (triggerEvent) {
    case GameStart: {
        // Handle global events
        if (data.isNull()) {
            QList<DrawNCardsStruct> s_list;
            foreach (ServerPlayer *p, room->getPlayers()) {
                int n = p->isLord() ? 8 : p->getSeat() + 1;
                DrawNCardsStruct s;
                s.player = p;
                s.isInitial = true;
                s.n = n;
                QVariant data = QVariant::fromValue(s);
                room->getThread()->trigger(DrawInitialCards, room, data);
                s_list << data.value<DrawNCardsStruct>();
            }
            QList<int> n_list;
            foreach (DrawNCardsStruct s, s_list)
                n_list << s.n;
            room->drawCards(room->getPlayers(), n_list, "initialDraw");
            if (Config.LuckCardLimitation > 0)
                room->askForLuckCard();
            foreach (DrawNCardsStruct s, s_list) {
                QVariant _slistati = QVariant::fromValue(s);
                room->getThread()->trigger(AfterDrawInitialCards, room, _slistati);
            }

            return false;
            break;
        }
    }
    case GameOverJudge: {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who->isLord())
            room->gameOver("rebel");
        else if (room->aliveRoles(death.who).length() == 1)
            room->gameOver("lord");

        return false;
    }
    case BuryVictim: {
        DeathStruct death = data.value<DeathStruct>();
        if (death.who->hasFlag("actioned"))
            room->setPlayerFlag(death.who, "-actioned");

        LogMessage log;
        log.type = "#Reforming";
        log.from = death.who;
        room->sendLog(log);

        death.who->bury();
        room->setPlayerProperty(death.who, "hp", 0);

        foreach (ServerPlayer *p, room->getOtherPlayers(room->getLord())) {
            if (p->isAlive() && p->askForSkillInvoke("draw_1v3"))
                p->drawCards(1);
        }

        return false;
    }
    case TurnStart: {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->isDead()) {
            JsonArray arg;
            arg << (int)QSanProtocol::S_GAME_EVENT_PLAYER_REFORM;
            arg << player->objectName();
            room->doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);

            QString choice = player->isWounded() ? "recover" : "draw";
            if (player->isWounded() && player->getHp() > 0)
                choice = room->askForChoice(player, "Hulaopass", "recover+draw");

            if (choice == "draw") {
                LogMessage log;
                log.type = "#ReformingDraw";
                log.from = player;
                log.arg = "1";
                room->sendLog(log);
                player->drawCards(1, "reform");
            } else {
                LogMessage log;
                log.type = "#ReformingRecover";
                log.from = player;
                log.arg = "1";
                room->sendLog(log);
                room->setPlayerProperty(player, "hp", player->getHp() + 1);
            }

            if (player->getHp() + player->getHandcardNum() == 6) {
                LogMessage log;
                log.type = "#ReformingRevive";
                log.from = player;
                room->sendLog(log);

                room->revivePlayer(player);
            }
        } else {
            LogMessage log;
            log.type = "$AppendSeparator";
            room->sendLog(log);
            room->addPlayerMark(player, "Global_TurnCount");

            if (!player->faceUp())
                player->turnOver();
            else
                player->play();
        }

        return false;
    }
    default:
        break;
    }

    return GameRule::effect(triggerEvent, room, invoke, data);
}
