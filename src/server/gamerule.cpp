#include "gamerule.h"
#include "card.h"
#include "engine.h"
#include "general.h"
#include "room.h"
#include "serverplayer.h"
#include "settings.h"
#include "util.h"

#include <QTime>

GameRule::GameRule()
    : Rule(QStringLiteral("game_rule"))
{
    addTriggerEvents(TriggerEvents() << GameStart << TurnStart << EventPhaseProceeding << EventPhaseEnd << EventPhaseChanging << PreCardUsed << CardUsed << CardFinished
                                     << CardEffected << PostHpReduced << EventLoseSkill << EventAcquireSkill << AskForPeaches << AskForPeachesDone << BuryVictim
                                     << BeforeGameOverJudge << GameOverJudge << SlashHit << SlashEffected << SlashProceed << ConfirmDamage << DamageDone << DamageComplete
                                     << StartJudge << FinishRetrial << FinishJudge << ChoiceMade << BeforeCardsMove << EventPhaseStart << JinkEffect << GeneralShown);
}

void GameRule::onPhaseProceed(ServerPlayer *player) const
{
    Room *room = player->getRoom();
    switch (player->getPhase()) {
    case QSanguosha::PhaseNone: {
        Q_ASSERT(false);
    }
    case QSanguosha::PhaseRoundStart: {
        break;
    }
    case QSanguosha::PhaseStart: {
        break;
    }
    case QSanguosha::PhaseJudge: {
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

            bool on_effect = room->cardEffect(trick, nullptr, player);
            effected << trick;
            if (!on_effect)
                trick->face()->onNullified(player, trick);
        }
        break;
    }
    case QSanguosha::PhaseDraw: {
        int num = 2;
        if (player->hasFlag(QStringLiteral("Global_FirstRound"))) {
            room->setPlayerFlag(player, QStringLiteral("-Global_FirstRound"));
            if (room->getMode() == QStringLiteral("02_1v1"))
                num--;
        }
        DrawNCardsStruct s;
        s.player = player;
        s.isInitial = false;
        s.n = num;
        QVariant qnum = QVariant::fromValue(s);
        Q_ASSERT(room->getThread() != nullptr);
        room->getThread()->trigger(DrawNCards, qnum);
        s = qnum.value<DrawNCardsStruct>();
        num = s.n;
        if (num > 0)
            player->drawCards(num);
        s.n = num;
        qnum = QVariant::fromValue(s);
        room->getThread()->trigger(AfterDrawNCards, qnum);
        break;
    }
    case QSanguosha::PhasePlay: {
        while (player->isAlive()) {
            CardUseStruct card_use;
            room->activate(player, card_use);
            if (card_use.card != nullptr)
                room->useCard(card_use);
            else
                break;
        }
        break;
    }
    case QSanguosha::PhaseDiscard: {
        int discard_num = 0; //discard in one time
        discard_num = player->getHandcardNum() - player->getMaxCards();
        if (discard_num > 0)
            room->askForDiscard(player, QStringLiteral("gamerule"), discard_num, discard_num);
        break;
    }
    case QSanguosha::PhaseFinish: {
        break;
    }
    case QSanguosha::PhaseNotActive: {
        break;
    }
    }
}

bool GameRule::trigger(TriggerEvent triggerEvent, RoomObject *_room, const TriggerDetail & /*detail*/, QVariant &data) const
{
    Room *room = qobject_cast<Room *>(_room);

    if (room->getTag(QStringLiteral("SkipGameRule")).toBool()) {
        room->removeTag(QStringLiteral("SkipGameRule"));
        return false;
    }

    switch (triggerEvent) {
    case GameStart: {
        if (data.isNull()) {
            foreach (ServerPlayer *player, room->getPlayers()) {
                Q_ASSERT(player->getGeneral() != nullptr);
                if (!isHegemonyGameMode(room->getMode())
                    && (player->getGeneral()->getKingdom() == QStringLiteral("zhu") || player->getGeneral()->getKingdom() == QStringLiteral("touhougod"))
                    && player->getGeneralName() != QStringLiteral("anjiang")) {
                    QString new_kingdom = room->askForKingdom(player);
                    room->setPlayerProperty(player, "kingdom", new_kingdom);

                    LogMessage log;
                    log.type = QStringLiteral("#ChooseKingdom");
                    log.from = player;
                    log.arg = new_kingdom;
                    room->sendLog(log);
                }
                if (isHegemonyGameMode(room->getMode())) {
                    foreach (const Skill *skill, player->getVisibleSkillList()) {
                        if (skill->isLimited() && !skill->limitMark().isEmpty() && (!skill->isLordSkill() || player->hasLordSkill(skill->objectName()))) {
                            JsonArray arg;
                            arg << player->objectName();
                            arg << skill->limitMark();
                            arg << 1;
                            room->doNotify(player, QSanProtocol::S_COMMAND_SET_MARK, arg);
                            player->setMark(skill->limitMark(), 1);
                        }
                    }
                } else {
                    foreach (const Skill *skill, player->getVisibleSkillList()) {
                        if (skill->isLimited() && !skill->limitMark().isEmpty() && (!skill->isLordSkill() || player->hasLordSkill(skill->objectName())))
                            room->setPlayerMark(player, skill->limitMark(), 1);
                    }
                }
            }
            room->setTag(QStringLiteral("FirstRound"), true);
            bool kof_mode
                = room->getMode() == QStringLiteral("02_1v1") && Config.value(QStringLiteral("1v1/Rule"), QStringLiteral("2013")).toString() != QStringLiteral("Classical");
            QList<DrawNCardsStruct> s_list;
            foreach (ServerPlayer *p, room->getPlayers()) {
                int n = kof_mode ? p->getMaxHp() : 4;
                DrawNCardsStruct s;
                s.player = p;
                s.isInitial = true;
                s.n = n;
                QVariant data = QVariant::fromValue(s);
                room->getThread()->trigger(DrawInitialCards, data);
                s_list << data.value<DrawNCardsStruct>();
            }
            QList<int> n_list;
            foreach (DrawNCardsStruct s, s_list)
                n_list << s.n;
            room->drawCards(room->getPlayers(), n_list, QStringLiteral("initialDraw"));
            if (Config.LuckCardLimitation > 0)
                room->askForLuckCard();
            foreach (DrawNCardsStruct s, s_list) {
                QVariant _slistati = QVariant::fromValue(s);
                room->getThread()->trigger(AfterDrawInitialCards, _slistati);
            }
        }
        break;
    }
    case TurnStart: {
        ServerPlayer *player = room->getCurrent();
        if (player == nullptr)
            return false;
        if (room->getTag(QStringLiteral("FirstRound")).toBool()) {
            room->setTag(QStringLiteral("FirstRound"), false);
            room->setPlayerFlag(player, QStringLiteral("Global_FirstRound"));
        }

        if (player->isLord() && !player->tag.value(QStringLiteral("touhou-extra"), false).toBool()) {
            QString mode = Config.GameMode;
            if (!mode.endsWith(QStringLiteral("1v1")) && !mode.endsWith(QStringLiteral("1v3")) && mode != QStringLiteral("06_XMode"))
                room->setTag(QStringLiteral("Global_RoundCount"), room->getTag(QStringLiteral("Global_RoundCount")).toInt() + 1);
        }

        LogMessage log;
        log.type = QStringLiteral("$AppendSeparator");
        room->sendLog(log);
        room->addPlayerMark(player, QStringLiteral("Global_TurnCount"));

        QList<QSanguosha::Phase> set_phases;
        ExtraTurnStruct extra = player->tag[QStringLiteral("ExtraTurnInfo")].value<ExtraTurnStruct>();
        if (!extra.set_phases.isEmpty())
            set_phases = extra.set_phases;
        //clear other's extraTurn infomation
        foreach (ServerPlayer *p, room->getOtherPlayers(player))
            p->tag.remove(QStringLiteral("ExtraTurnInfo"));

        if (!player->faceUp()) {
            room->setPlayerFlag(player, QStringLiteral("-Global_FirstRound"));
            player->turnOver();
        } else if (player->isAlive()) {
            if (set_phases.isEmpty())
                player->play();
            else
                player->play(set_phases);
        }
        // FIXME: replace with valid function here.
        // room->autoCleanupClonedCards();
        break;
    }
    case EventPhaseProceeding: {
        ServerPlayer *player = data.value<ServerPlayer *>();
        onPhaseProceed(player);
        break;
    }
    case EventPhaseStart: {
        ServerPlayer *current = data.value<ServerPlayer *>();
        if ((current != nullptr) && current->getPhase() == QSanguosha::PhaseFinish && !current->getBrokenEquips().isEmpty()
            && !current->hasFlag(QStringLiteral("GameRule_brokenEquips")))
            current->removeBrokenEquips(current->getBrokenEquips());
        break;
    }
    case EventPhaseEnd: {
        ServerPlayer *player = data.value<ServerPlayer *>();
        if (player->getPhase() == QSanguosha::PhasePlay)
            room->addPlayerHistory(player, QStringLiteral("."));
        if (player->getPhase() == QSanguosha::PhaseFinish) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (player->getMark(QStringLiteral("multi_kill_count")) > 0)
                    room->setPlayerMark(p, QStringLiteral("multi_kill_count"), 0);
            }
        }
        break;
    }
    case EventPhaseChanging: {
        PhaseChangeStruct change = data.value<PhaseChangeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(change.player);
        if (change.to == QSanguosha::PhaseNotActive) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->getMark(QStringLiteral("drank")) > 0) {
                    LogMessage log;
                    log.type = QStringLiteral("#UnsetDrankEndOfTurn");
                    log.from = p;
                    room->sendLog(log);

                    room->setPlayerMark(p, QStringLiteral("drank"), 0);
                }
                if (p->getMark(QStringLiteral("magic_drank")) > 0) {
                    LogMessage log;
                    log.type = QStringLiteral("#UnsetDrankEndOfTurn");
                    log.from = p;
                    room->sendLog(log);

                    room->setPlayerMark(p, QStringLiteral("magic_drank"), 0);
                }
            }

            room->setPlayerFlag(player, QStringLiteral("."));
            room->setPlayerMark(player, QStringLiteral("touhou-extra"), 0);

            foreach (ServerPlayer *p, room->getAlivePlayers()) {
                room->clearPlayerCardLimitation(p, true);
                QMap<QString, int> marks = p->getMarkMap();
                QMap<QString, int>::iterator it;
                for (it = marks.begin(); it != marks.end(); ++it) {
                    if (it.value() > 0 && it.key().endsWith(QStringLiteral("_SingleTurn")))
                        room->setPlayerMark(player, it.key(), 0);
                }

                foreach (QString flag, p->getFlagList()) {
                    if (flag.endsWith(QStringLiteral("Animate")))
                        room->setPlayerFlag(p, QStringLiteral("-") + flag);
                }
            }
        } else if (change.to == QSanguosha::PhasePlay) {
            room->addPlayerHistory(player, QStringLiteral("."));
        }

        break;
    }
    case PreCardUsed: {
        if (data.canConvert<CardUseStruct>()) {
            CardUseStruct card_use = data.value<CardUseStruct>();
            if (card_use.from->hasFlag(QStringLiteral("Global_ForbidSurrender"))) {
                card_use.from->setFlags(QStringLiteral("-Global_ForbidSurrender"));
                room->doNotify(qobject_cast<ServerPlayer *>(card_use.from), QSanProtocol::S_COMMAND_ENABLE_SURRENDER, QVariant(true));
            }

            qobject_cast<ServerPlayer *>(card_use.from)->broadcastSkillInvoke(card_use.card);
            if (!card_use.card->skillName().isNull() && card_use.card->skillName(true) == card_use.card->skillName(false) && card_use.m_isOwnerUse
                && card_use.from->hasSkill(card_use.card->skillName()))
                room->notifySkillInvoked(qobject_cast<ServerPlayer *>(card_use.from), card_use.card->skillName());
        }
        break;
    }
    case CardUsed: {
        if (data.canConvert<CardUseStruct>()) {
            CardUseStruct card_use = data.value<CardUseStruct>();
            RoomThread *thread = room->getThread();

            if (card_use.card->face()->hasPreAction())
                card_use.card->face()->doPreAction(room, card_use);

            if (card_use.from != nullptr) {
                thread->trigger(TargetSpecifying, data);
                card_use = data.value<CardUseStruct>();
            }

            if (!card_use.to.isEmpty()) {
                thread->trigger(TargetConfirming, data);
                card_use = data.value<CardUseStruct>();
            }

            //1) exclude SkillCard 2)changed move reason (USE) 3)keep extraData
            if ((card_use.card != nullptr) && card_use.card->face()->type() != QSanguosha::TypeSkill && !(card_use.card->isVirtualCard() && card_use.card->subcards().isEmpty())
                && card_use.to.isEmpty()) {
                if (room->getCardPlace(card_use.card->effectiveID()) == QSanguosha::PlaceTable) {
                    CardMoveReason reason(CardMoveReason::S_REASON_USE, card_use.from->objectName(), QString(), card_use.card->skillName(), QString());
                    reason.m_extraData = QVariant::fromValue(card_use.card);
                    room->moveCardTo(card_use.card, qobject_cast<ServerPlayer *>(card_use.from), nullptr, QSanguosha::PlaceDiscardPile, reason, true);
                }
            }
            //since use.to is empty, break the whole process
            if ((card_use.card != nullptr) && card_use.card->face()->type() != QSanguosha::TypeSkill && card_use.to.isEmpty()) {
                if (card_use.card->face()->isKindOf("Slash") && card_use.from->isAlive())
                    room->setPlayerMark(qobject_cast<ServerPlayer *>(card_use.from), QStringLiteral("drank"), 0);
                if (card_use.card->face()->isNDTrick() && card_use.from->isAlive()) //clear magic_drank while using Nullification
                    room->setPlayerMark(qobject_cast<ServerPlayer *>(card_use.from), QStringLiteral("magic_drank"), 0);
                break;
            }

            try {
                QVariantList jink_list_backup;
                if (card_use.card->face()->isKindOf("Slash")) {
                    jink_list_backup = card_use.from->tag[QStringLiteral("Jink_") + card_use.card->toString()].toList();
                    QVariantList jink_list;
                    int jink_num = 1;
                    if (card_use.card->hasFlag(QStringLiteral("ZeroJink")))
                        jink_num = 0;
                    for (int i = 0; i < card_use.to.length(); i++)
                        jink_list.append(QVariant(jink_num));
                    card_use.from->tag[QStringLiteral("Jink_") + card_use.card->toString()] = QVariant::fromValue(jink_list);
                }
                if ((card_use.from != nullptr) && !card_use.to.isEmpty()) {
                    thread->trigger(TargetSpecified, data);
                    thread->trigger(TargetConfirmed, data);
                }
                card_use = data.value<CardUseStruct>();
                room->setTag(QStringLiteral("CardUseNullifiedList"), QVariant::fromValue(card_use.nullified_list));
                if (card_use.card->face()->isNDTrick() && !card_use.card->face()->isKindOf("Nullification"))
                    room->setCardFlag(card_use.card, QStringLiteral("LastTrickTarget_") + card_use.to.last()->objectName());

                card_use.card->face()->use(room, card_use);
                if (!jink_list_backup.isEmpty())
                    card_use.from->tag[QStringLiteral("Jink_") + card_use.card->toString()] = QVariant::fromValue(jink_list_backup);
            } catch (TriggerEvent triggerEvent) {
                if (triggerEvent == TurnBroken)
                    card_use.from->tag.remove(QStringLiteral("Jink_") + card_use.card->toString());

                //copy from Room::useCard()
                if (triggerEvent == TurnBroken) {
                    if (room->getCardPlace(card_use.card->effectiveID()) == QSanguosha::PlaceTable) {
                        CardMoveReason reason(CardMoveReason::S_REASON_UNKNOWN, card_use.from->objectName(), QString(), card_use.card->skillName(), QString());
                        if (card_use.to.size() == 1)
                            reason.m_targetId = card_use.to.first()->objectName();
                        room->moveCardTo(card_use.card, qobject_cast<ServerPlayer *>(card_use.from), nullptr, QSanguosha::PlaceDiscardPile, reason, true);
                    }
                    QVariant data = QVariant::fromValue(card_use);
                    card_use.from->setFlags(QStringLiteral("Global_ProcessBroken"));
                    thread->trigger(CardFinished, data);
                    card_use.from->setFlags(QStringLiteral("-Global_ProcessBroken"));

                    foreach (ServerPlayer *p, room->getAlivePlayers()) {
                        p->tag.remove(QStringLiteral("Qinggang"));

                        foreach (QString flag, p->getFlagList()) {
                            if (flag == QStringLiteral("Global_GongxinOperator"))
                                p->setFlags(QStringLiteral("-") + flag);
                            else if (flag.endsWith(QStringLiteral("_InTempMoving")))
                                room->setPlayerFlag(p, QStringLiteral("-") + flag);
                        }
                    }

                    foreach (int id, Sanguosha->getRandomCards()) {
                        if (room->getCardPlace(id) == QSanguosha::PlaceTable || room->getCardPlace(id) == QSanguosha::PlaceJudge)
                            room->moveCardTo(room->getCard(id), nullptr, QSanguosha::PlaceDiscardPile, true);
                        if (room->getCard(id)->hasFlag(QStringLiteral("using")))
                            room->setCardFlag(id, QStringLiteral("-using"));
                    }
                }

                throw triggerEvent;
            }
        }

        break;
    }
    case CardFinished: {
        CardUseStruct use = data.value<CardUseStruct>();
        room->clearCardFlag(use.card);

        if (use.card->face()->isNDTrick())
            room->removeTag(use.card->toString() + QStringLiteral("HegNullificationTargets"));

        if (use.card->face()->isKindOf("AOE") || use.card->face()->isKindOf("GlobalEffect")) {
            foreach (ServerPlayer *p, room->getAlivePlayers())
                room->doNotify(p, QSanProtocol::S_COMMAND_NULLIFICATION_ASKED, QStringLiteral("."));
        }

        if (use.card->face()->isKindOf("Slash"))
            use.from->tag.remove(QStringLiteral("Jink_") + use.card->toString());

        break;
    }
    case EventAcquireSkill:
    case EventLoseSkill: {
        SkillAcquireDetachStruct s = data.value<SkillAcquireDetachStruct>();

        const Skill *skill = s.skill;
        bool refilter = skill->inherits("FilterSkill");
        if (refilter)
            room->filterCards(qobject_cast<ServerPlayer *>(s.player), qobject_cast<ServerPlayer *>(s.player)->getCards(QStringLiteral("hes")), triggerEvent == EventLoseSkill);

        break;
    }
    case PostHpReduced: {
        ServerPlayer *player = nullptr;
        if (data.canConvert<DamageStruct>())
            player = qobject_cast<ServerPlayer *>(data.value<DamageStruct>().to);
        else if (data.canConvert<HpLostStruct>())
            player = qobject_cast<ServerPlayer *>(data.value<HpLostStruct>().player);
        if (player == nullptr)
            break;

        if (player->getHp() >= player->dyingThreshold())
            break;

        if (data.canConvert<DamageStruct>()) {
            DamageStruct damage = data.value<DamageStruct>();
            room->enterDying(player, &damage);
        } else
            room->enterDying(player, nullptr);

        break;
    }
    case AskForPeaches: {
        DyingStruct dying = data.value<DyingStruct>();
        const Card *peach = nullptr;
        int threshold = dying.who->dyingThreshold();

        while (dying.who->getHp() < threshold) {
            peach = nullptr;

            if (dying.who->isAlive())
                peach = room->askForSinglePeach(qobject_cast<ServerPlayer *>(dying.nowAskingForPeaches), qobject_cast<ServerPlayer *>(dying.who));

            if (peach == nullptr)
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
            room->killPlayer(qobject_cast<ServerPlayer *>(dying.who), dying.damage);

        break;
    }
    case ConfirmDamage: {
        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.card != nullptr) && damage.to->getMark(QStringLiteral("SlashIsDrank")) > 0) {
            LogMessage log;
            log.type = QStringLiteral("#AnalepticBuff");
            log.from = damage.from;
            log.to << damage.to;
            log.arg = QString::number(damage.damage);

            damage.damage += damage.to->getMark(QStringLiteral("SlashIsDrank"));
            damage.to->setMark(QStringLiteral("SlashIsDrank"), 0);

            log.arg2 = QString::number(damage.damage);

            room->sendLog(log);

            data = QVariant::fromValue(damage);
        }

        break;
    }
    case DamageDone: {
        DamageStruct damage = data.value<DamageStruct>();
        if ((damage.from != nullptr) && !damage.from->isAlive())
            damage.from = nullptr;
        data = QVariant::fromValue(damage);
        room->sendDamageLog(damage);

        room->applyDamage(qobject_cast<ServerPlayer *>(damage.to), damage);
        if ((damage.nature != DamageStruct::Normal) && damage.to->isChained() && !damage.chain) {
            damage.trigger_chain = true;
            data = QVariant::fromValue(damage);
        }
        room->getThread()->trigger(PostHpReduced, data);

        break;
    }
    case DamageComplete: {
        DamageStruct damage = data.value<DamageStruct>();

        if ((damage.nature != DamageStruct::Normal) && damage.to->isChained())
            room->setPlayerProperty(qobject_cast<ServerPlayer *>(damage.to), "chained", false);

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
                        log.type = QStringLiteral("#IronChainDamage");
                        log.from = chained_player;
                        room->sendLog(log);

                        DamageStruct chain_damage = damage;
                        chain_damage.to = chained_player;
                        chain_damage.chain = true;
                        chain_damage.trigger_info = QString(); //clear trigger_info.  eg. shihui

                        room->damage(chain_damage);
                    }
                }
            }
        }
        if (room->getMode() == QStringLiteral("02_1v1") || room->getMode() == QStringLiteral("06_XMode")) {
            foreach (ServerPlayer *p, room->getAllPlayers()) {
                if (p->hasFlag(QStringLiteral("Global_DebutFlag"))) {
                    p->setFlags(QStringLiteral("-Global_DebutFlag"));
                    if (room->getMode() == QStringLiteral("02_1v1")) {
                        QVariant v = QVariant::fromValue(p);
                        room->getThread()->trigger(Debut, v);
                    }
                }
            }
        }
        break;
    }
    case CardEffected: {
        if (data.canConvert<CardEffectStruct>()) {
            CardEffectStruct effect = data.value<CardEffectStruct>();
            if (!effect.card->face()->isKindOf("Slash") && effect.nullified) {
                LogMessage log;
                log.type = QStringLiteral("#CardNullified");
                log.from = qobject_cast<ServerPlayer *>(effect.to);
                log.arg = effect.card->faceName();
                room->sendLog(log);
                room->setEmotion(qobject_cast<ServerPlayer *>(effect.to), QStringLiteral("skill_nullify"));
                return true;
            } else if (effect.card->face()->type() == QSanguosha::TypeTrick) {
                if (room->isCanceled(effect)) {
                    effect.to->setFlags(QStringLiteral("Global_NonSkillNullify"));
                    return true;
                } else {
                    room->getThread()->trigger(TrickEffect, data);
                }
            }
            if (effect.to->isAlive() || effect.card->face()->isKindOf("Slash")) {
                //record for skill tianxie
                if (!effect.card->face()->isKindOf("Slash"))
                    room->setCardFlag(effect.card, QStringLiteral("tianxieEffected_") + effect.to->objectName());
                //do chunhua effect
                if (effect.card->hasFlag(QStringLiteral("chunhua")) && !effect.card->face()->isKindOf("Slash")) {
                    room->touhouLogmessage(QStringLiteral("#Chunhua"), qobject_cast<ServerPlayer *>(effect.to), effect.card->faceName());
                    if (effect.card->hasFlag(QStringLiteral("chunhua_black"))) {
                        DamageStruct d = DamageStruct(effect.card, effect.from, effect.to, 1 + effect.effectValue.first(), DamageStruct::Normal);
                        room->damage(d);
                    } else if (effect.card->hasFlag(QStringLiteral("chunhua_red"))) {
                        RecoverStruct recover;
                        recover.card = effect.card;
                        recover.who = effect.from;
                        recover.recover = 1 + effect.effectValue.first();
                        room->recover(qobject_cast<ServerPlayer *>(effect.to), recover);
                    }
                } else if (effect.card->skillName() == QStringLiteral("xianshi")) { // deal xianshi extra effect and original effect
                    QString xianshi_name;
                    QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
                    foreach (const Card *card, cards) {
                        if (card->face()->isNDTrick() || card->face()->isKindOf("BasicCard")) {
                            if (effect.card->hasFlag(QStringLiteral("xianshi_") + card->faceName())) {
                                xianshi_name = card->faceName();
                                break;
                            }
                        }
                    }

                    Card *extraCard = room->cloneCard(xianshi_name);
                    if (extraCard->face()->isKindOf("Slash")) {
                        DamageStruct::Nature nature = DamageStruct::Normal;
                        if (extraCard->face()->isKindOf("FireSlash"))
                            nature = DamageStruct::Fire;
                        else if (extraCard->face()->isKindOf("ThunderSlash"))
                            nature = DamageStruct::Thunder;
                        int damageValue = 1;

                        if (extraCard->face()->isKindOf("DebuffSlash")) {
                            SlashEffectStruct extraEffect;
                            extraEffect.from = effect.from;
                            extraEffect.slash = extraCard;

                            extraEffect.to = effect.to;
                            extraEffect.effectValue.first() = effect.effectValue.first();
#if 0
                            if (extraCard->face()->isKindOf("IronSlash"))
                                IronSlash::debuffEffect(extraEffect);
                            else if (extraCard->face()->isKindOf("LightSlash"))
                                LightSlash::debuffEffect(extraEffect);
                            else if (extraCard->face()->isKindOf("PowerSlash"))
                                PowerSlash::debuffEffect(extraEffect);
#endif
                        }

                        if (!extraCard->face()->isKindOf("LightSlash") && !extraCard->face()->isKindOf("PowerSlash")) {
                            damageValue = damageValue + effect.effectValue.first();
                        }

                        DamageStruct d = DamageStruct(effect.card, effect.from, effect.to, damageValue, nature);
                        room->damage(d);

                    } else if (!effect.card->face()->isKindOf("Slash")) { //if original effect is slash, deal extra effect after slash hit.
                        if (extraCard->face()->isKindOf("Peach")) {
                            CardEffectStruct extraEffect;
                            extraCard->addSubcards(effect.card->subcards());

                            extraEffect.card = effect.card;
                            extraEffect.from = effect.from;
                            extraEffect.to = effect.to;
                            extraEffect.multiple = effect.multiple;
                            if (effect.card->face()->isNDTrick())
                                extraEffect.effectValue.first() = effect.effectValue.first();
                            extraCard->face()->onEffect(extraEffect);
                        } else if (extraCard->face()->isKindOf("Analeptic")) {
                            RecoverStruct recover;
                            recover.card = effect.card;
                            recover.who = effect.from;
                            if (effect.card->face()->isNDTrick())
                                recover.recover = 1 + effect.effectValue.first();
                            room->recover(qobject_cast<ServerPlayer *>(effect.to), recover);
                        } else if (extraCard->face()->isKindOf("AmazingGrace")) {
                            room->doExtraAmazingGrace(qobject_cast<ServerPlayer *>(effect.from), qobject_cast<ServerPlayer *>(effect.to), 1 + effect.effectValue.first());
                        } else {
                            CardEffectStruct extraEffect;
                            extraCard->addSubcards(effect.card->subcards());
                            // extraCard->deleteLater();
                            extraEffect.card = effect.card;
                            extraEffect.from = effect.from;
                            extraEffect.to = effect.to;
                            extraEffect.multiple = effect.multiple;
                            if (effect.card->face()->isNDTrick())
                                extraEffect.effectValue.first() = effect.effectValue.first();
                            extraCard->face()->onEffect(extraEffect);
                        }
                    }
                    //xianshi_extra effect will use magic_drank whilefirst effect, then clean it.  //need check
                    if (effect.effectValue.first() > 0 && effect.card->face()->isNDTrick() && extraCard->hasEffectValue())
                        effect.effectValue.first() = 0;

                    room->cardDeleting(extraCard);

                    if (effect.to->isAlive())
                        effect.card->face()->onEffect(effect); //do original effect

                } else
                    effect.card->face()->onEffect(effect);
            }
        }

        break;
    }
    case SlashEffected: {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        if (effect.nullified) {
            LogMessage log;
            log.type = QStringLiteral("#CardNullified");
            log.from = effect.to;
            log.arg = effect.slash->faceName();
            room->sendLog(log);
            room->setEmotion(qobject_cast<ServerPlayer *>(effect.to), QStringLiteral("skill_nullify"));
            return true;
        }

        QVariant data = QVariant::fromValue(effect);
        room->getThread()->trigger(SlashProceed, data);
        break;
    }
    case SlashProceed: {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        QString slasher = effect.from->objectName();
        if (!effect.to->isAlive())
            break;

        // process skill cancel, like (obsolete) kuangluan and (current) bmmaoji.
        if (room->getThread()->trigger(Cancel, data)) {
            effect = data.value<SlashEffectStruct>();
            room->slashResult(effect, nullptr);
            break;
        }

        if (effect.jink_num == 0) {
            room->slashResult(effect, nullptr);
            break;
        }
        if (effect.jink_num == 1) {
            const Card *jink = room->askForCard(qobject_cast<ServerPlayer *>(effect.to), QStringLiteral("jink"), QStringLiteral("slash-jink:") + slasher, data,
                                                QSanguosha::MethodUse, qobject_cast<ServerPlayer *>(effect.from));
            room->slashResult(effect, room->isJinkEffected(effect, jink) ? jink : nullptr);
        } else {
            Card *jink = room->cloneCard(QStringLiteral("DummyCard"));
            // Since GameRule is created by RoomThread not Engine, and this function also runs at RoomThread not main thread, so this jink must be on the RoomThread
            // Because the RoomThread has no event loop, so a deleteLater is absolutely safe for it can be deleted only by the time of the deletion of RoomThread
            // jink->deleteLater();
            const Card *asked_jink = nullptr;
            for (int i = effect.jink_num; i > 0; i--) {
                QString prompt = QStringLiteral("@multi-jink%1:%2::%3").arg(i == effect.jink_num ? QStringLiteral("-start") : QString()).arg(slasher).arg(i);
                asked_jink = room->askForCard(qobject_cast<ServerPlayer *>(effect.to), QStringLiteral("jink"), prompt, data, QSanguosha::MethodUse,
                                              qobject_cast<ServerPlayer *>(effect.from));
                if (!room->isJinkEffected(effect, asked_jink)) {
                    room->cardDeleting(jink);
                    room->slashResult(effect, nullptr);
                    return false;
                } else {
                    jink->addSubcard(asked_jink->effectiveID());
                }
            }
            room->slashResult(effect, jink);
            room->cardDeleting(jink);
        }

        break;
    }
    case JinkEffect: {
        JinkEffectStruct j = data.value<JinkEffectStruct>();
        if (j.jink != nullptr && j.jink->skillName() == QStringLiteral("xianshi")) {
            SlashEffectStruct effect = j.slashEffect;

            QString xianshi_name = effect.to->property("xianshi_card").toString();
            if (!xianshi_name.isNull() && (effect.from != nullptr) && (effect.to != nullptr) && effect.from->isAlive() && effect.to->isAlive()) {
                Card *extraCard = room->cloneCard(xianshi_name);
                if (extraCard->face()->isKindOf("Slash")) {
                    DamageStruct::Nature nature = DamageStruct::Normal;
                    if (extraCard->face()->isKindOf("FireSlash"))
                        nature = DamageStruct::Fire;
                    else if (extraCard->face()->isKindOf("ThunderSlash"))
                        nature = DamageStruct::Thunder;
                    int damageValue = 1;

                    if (extraCard->face()->isKindOf("DebuffSlash")) {
                        SlashEffectStruct extraEffect;
                        extraEffect.from = effect.to;
                        extraEffect.slash = extraCard;
                        extraEffect.to = effect.from;
#if 0
                        if (extraCard->face()->isKindOf("IronSlash"))
                            IronSlash::debuffEffect(extraEffect);
                        else if (extraCard->face()->isKindOf("LightSlash"))
                            LightSlash::debuffEffect(extraEffect);
                        else if (extraCard->face()->isKindOf("PowerSlash"))
                            PowerSlash::debuffEffect(extraEffect);
#endif
                    }

                    DamageStruct d = DamageStruct(j.jink, effect.to, effect.from, damageValue, nature);
                    room->damage(d);

                } else if (extraCard->face()->isKindOf("Peach")) {
                    CardEffectStruct extraEffect;
                    extraCard->addSubcards(j.jink->subcards());
                    // extraCard->deleteLater();

                    extraEffect.card = j.jink;
                    extraEffect.from = effect.to;
                    extraEffect.to = effect.from;
                    extraEffect.multiple = effect.multiple;
                    extraCard->face()->onEffect(extraEffect);
                } else if (extraCard->face()->isKindOf("Analeptic")) {
                    RecoverStruct recover;
                    recover.card = j.jink;
                    recover.to = effect.from;
                    recover.who = effect.to;
                    room->recover(qobject_cast<ServerPlayer *>(effect.from), recover);
                } else if (extraCard->face()->isKindOf("AmazingGrace")) {
                    room->doExtraAmazingGrace(qobject_cast<ServerPlayer *>(effect.from), qobject_cast<ServerPlayer *>(effect.from), 1);
                } else { // trick card
                    CardEffectStruct extraEffect;
                    extraCard->addSubcards(j.jink->subcards());
                    // extraCard->deleteLater();
                    extraEffect.card = j.jink;
                    extraEffect.from = effect.to;
                    extraEffect.to = effect.from;
                    extraEffect.multiple = effect.multiple;
                    extraCard->face()->onEffect(extraEffect);
                }

                room->cardDeleting(extraCard);
            }
        }

        if (j.jink != nullptr && j.jink->face()->isKindOf("NatureJink")) {
            SlashEffectStruct effect = j.slashEffect;
            //process advanced_jink
            if ((effect.from != nullptr) && (effect.to != nullptr) && effect.from->isAlive() && effect.to->isAlive()) {
                CardEffectStruct new_effect;
                new_effect.card = j.jink;
                new_effect.from = effect.to;
                new_effect.to = effect.from;
                j.jink->face()->onEffect(new_effect);
            }
        }

        break;
    }
    case SlashHit: {
        SlashEffectStruct effect = data.value<SlashEffectStruct>();
        //do chunhua effect
        if (effect.slash->hasFlag(QStringLiteral("chunhua"))) {
            room->touhouLogmessage(QStringLiteral("#Chunhua"), qobject_cast<ServerPlayer *>(effect.to), effect.slash->faceName());
            effect.nature = DamageStruct::Normal;
            if (effect.slash->hasFlag(QStringLiteral("chunhua_red"))) {
                RecoverStruct recover;
                recover.card = effect.slash;
                recover.who = effect.from;
                recover.recover = 1 + effect.effectValue.first();
                room->recover(qobject_cast<ServerPlayer *>(effect.to), recover);
                break;
            }
        } else if (effect.slash->skillName() == QStringLiteral("xianshi")) {
            QString xianshi_name;
            QList<const Card *> cards = Sanguosha->findChildren<const Card *>();
            foreach (const Card *card, cards) {
                if (effect.slash->hasFlag(QStringLiteral("xianshi_") + card->faceName())) {
                    xianshi_name = card->faceName();
                    break;
                }
            }

            CardEffectStruct extraEffect;
            Card *extraCard = room->cloneCard(xianshi_name);
            extraCard->addSubcards(effect.slash->subcards());
            // extraCard->deleteLater();
            if (extraCard->face()->isKindOf("Peach")) {
                extraEffect.card = effect.slash;
                extraEffect.from = effect.from;
                extraEffect.to = effect.to;
                extraEffect.multiple = effect.multiple;
                extraEffect.effectValue.first() = extraEffect.effectValue.first();
                extraCard->face()->onEffect(extraEffect);
            } else if (extraCard->face()->isKindOf("Analeptic")) {
                RecoverStruct recover;
                recover.card = effect.slash;
                recover.who = effect.from;
                recover.recover = 1;
                room->recover(qobject_cast<ServerPlayer *>(effect.to), recover);
            } else if (extraCard->face()->isKindOf("AmazingGrace")) {
                room->doExtraAmazingGrace(qobject_cast<ServerPlayer *>(effect.from), qobject_cast<ServerPlayer *>(effect.to), 1);
            } else {
                extraEffect.card = effect.slash;
                extraEffect.from = effect.from;
                extraEffect.to = effect.to;
                extraEffect.multiple = effect.multiple;
                //Trick damage effect + drank
                if (extraCard->canDamage()) {
                    if (extraCard->face()->isKindOf("BoneHealing"))
                        extraEffect.effectValue.first() = extraEffect.effectValue.first() + effect.drank;
                    else
                        extraEffect.effectValue.last() = extraEffect.effectValue.last() + effect.drank;
                }
                extraCard->face()->onEffect(extraEffect);
            }

            room->cardDeleting(extraCard);
        }

        if (effect.to->isDead())
            break;

        //@todo: I want IronSlash to obtain function debuffEffect() from "Slash"  as an inheritance, But the variant slasheffect.slash is "Card".
        //using dynamic_cast may bring some terrible troubles.
        if (effect.slash->face()->isKindOf("DebuffSlash") && !effect.slash->hasFlag(QStringLiteral("chunhua_black"))) {
#if 0
            if (effect.slash->face()->isKindOf("IronSlash"))
                IronSlash::debuffEffect(effect);
            else if (effect.slash->face()->isKindOf("LightSlash"))
                LightSlash::debuffEffect(effect);
            else if (effect.slash->face()->isKindOf("PowerSlash"))
                PowerSlash::debuffEffect(effect);
#endif
        }

        if (effect.drank > 0)
            effect.to->setMark(QStringLiteral("SlashIsDrank"), effect.drank);

        DamageStruct d = DamageStruct(effect.slash, effect.from, effect.to, 1 + effect.effectValue.last(), effect.nature);
        foreach (ServerPlayer *p, room->getAllPlayers(true)) {
            if (effect.slash->hasFlag(QStringLiteral("WushenDamage_") + p->objectName())) {
                d.from = p->isAlive() ? p : nullptr;
                d.by_user = false;
                break;
            }
        }
        room->damage(d);

        break;
    }
    case BeforeGameOverJudge: {
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(death.who);
        if (isHegemonyGameMode(room->getMode())) {
            if (!player->hasShownGeneral())
                player->showGeneral(true, false, false);
            if ((player->getGeneral2() != nullptr) && !player->hasShownGeneral2())
                player->showGeneral(false, false, false);
        }

        break;
    }
    case GameOverJudge: {
        DeathStruct death = data.value<DeathStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(death.who);
        if (room->getMode() == QStringLiteral("02_1v1")) {
            QStringList list = player->tag[QStringLiteral("1v1Arrange")].toStringList();
            QString rule = Config.value(QStringLiteral("1v1/Rule"), QStringLiteral("2013")).toString();
            if (list.length() > ((rule == QStringLiteral("OL")) ? 3 : 0))
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
        bool skipRewardAndPunish = death.who->hasFlag(QStringLiteral("skipRewardAndPunish"));
        qobject_cast<ServerPlayer *>(death.who)->bury();

        ServerPlayer *killer = nullptr;
        if (death.useViewAsKiller)
            killer = qobject_cast<ServerPlayer *>(death.viewAsKiller);
        else if (death.damage != nullptr)
            killer = qobject_cast<ServerPlayer *>(death.damage->from);

        if (killer != nullptr) {
            room->setPlayerMark(killer, QStringLiteral("multi_kill_count"), killer->getMark(QStringLiteral("multi_kill_count")) + 1);
            int kill_count = killer->getMark(QStringLiteral("multi_kill_count"));
            if (kill_count > 1 && kill_count < 8)
                room->setEmotion(killer, QStringLiteral("multi_kill%1").arg(QString::number(kill_count)));
        }

        if (room->getTag(QStringLiteral("SkipNormalDeathProcess")).toBool())
            return false;

        if ((killer != nullptr) && !skipRewardAndPunish)
            rewardAndPunish(killer, qobject_cast<ServerPlayer *>(death.who));

        //if lord dead in hegemony mode?

        if (room->getMode() == QStringLiteral("02_1v1")) {
            QStringList list = death.who->tag[QStringLiteral("1v1Arrange")].toStringList();
            QString rule = Config.value(QStringLiteral("1v1/Rule"), QStringLiteral("2013")).toString();
            if (list.length() <= ((rule == QStringLiteral("OL")) ? 3 : 0))
                break;

            if (rule == QStringLiteral("Classical")) {
                death.who->tag[QStringLiteral("1v1ChangeGeneral")] = list.takeFirst();
                death.who->tag[QStringLiteral("1v1Arrange")] = list;
            } else {
                death.who->tag[QStringLiteral("1v1ChangeGeneral")] = list.first();
            }

            changeGeneral1v1(qobject_cast<ServerPlayer *>(death.who));
            if (death.damage == nullptr) {
                QVariant v = QVariant::fromValue(death.who);
                room->getThread()->trigger(Debut, v);
            } else
                death.who->setFlags(QStringLiteral("Global_DebutFlag"));
            return false;
        } else if (room->getMode() == QStringLiteral("06_XMode")) {
            changeGeneralXMode(qobject_cast<ServerPlayer *>(death.who));
            if (death.damage != nullptr)
                death.who->setFlags(QStringLiteral("Global_DebutFlag"));
            return false;
        }
        break;
    }
    case StartJudge: {
        int card_id = room->drawCard();

        JudgeStruct *judge = data.value<JudgeStruct *>();

        LogMessage log;
        log.type = QStringLiteral("$InitialJudge");
        log.from = judge->who;
        log.card_str = QString::number(card_id);
        room->sendLog(log);

        room->moveCardTo(room->getCard(card_id), nullptr, qobject_cast<ServerPlayer *>(judge->who), QSanguosha::PlaceJudge,
                         CardMoveReason(CardMoveReason::S_REASON_JUDGE, judge->who->objectName(), QString(), QString(), judge->reason), true);

        judge->setCard(room->getCard(card_id));
        break;
    }
    case FinishRetrial: {
        JudgeStruct *judge = data.value<JudgeStruct *>();

        LogMessage log;
        log.type = QStringLiteral("$JudgeResult");
        log.from = judge->who;
        log.card_str = QString::number(judge->card()->effectiveID());
        room->sendLog(log);

        int delay = Config.AIDelay;
        if (judge->time_consuming)
            delay /= 1.25;
        Q_ASSERT(room->getThread() != nullptr);
        room->getThread()->delay(delay);
        if (judge->play_animation) {
            room->sendJudgeResult(judge);
            room->getThread()->delay(Config.S_JUDGE_LONG_DELAY);
        }

        break;
    }
    case FinishJudge: {
        JudgeStruct *judge = data.value<JudgeStruct *>();

        if (room->getCardPlace(judge->card()->effectiveID()) == QSanguosha::PlaceJudge) {
            CardMoveReason reason(CardMoveReason::S_REASON_JUDGEDONE, judge->who->objectName(), QString(), judge->reason);
            if (judge->retrial_by_response != nullptr) {
                reason.m_extraData = QVariant::fromValue(judge->retrial_by_response);
            }

            room->moveCardTo(judge->card(), qobject_cast<ServerPlayer *>(judge->who), nullptr, QSanguosha::PlaceDiscardPile, reason, true);
        }

        break;
    }
    case ChoiceMade: {
        foreach (ServerPlayer *p, room->getAlivePlayers()) {
            foreach (QString flag, p->getFlagList()) {
                if (flag.startsWith(QStringLiteral("Global_")) && flag.endsWith(QStringLiteral("Failed")))
                    room->setPlayerFlag(p, QStringLiteral("-") + flag);
            }
        }
        break;
    }
    case GeneralShown: {
        //Only for Hegemony (Enable2ndGeneral is default setting)
        if (!isHegemonyGameMode(room->getMode()))
            break;

        ShowGeneralStruct s = data.value<ShowGeneralStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(s.player);
        QString winner = getWinner(player);
        if (!winner.isNull()) {
            room->gameOver(winner); // if all hasShownGenreal, and they are all friend, game over.
            return true;
        }
        if (player->getMark(QStringLiteral("TheFirstToShowReward")) > 0) {
            player->setMark(QStringLiteral("TheFirstToShowReward"), 0);
            if (Config.HegemonyFirstShowReward == QStringLiteral("Postponed")) {
                player->gainMark(QStringLiteral("@Pioneer"));
                QString attachName = QStringLiteral("pioneer_attach");
                if ((player != nullptr) && !player->hasSkill(attachName))
                    room->attachSkillToPlayer(player, attachName);
            } else if (Config.HegemonyFirstShowReward == QStringLiteral("Instant")) {
                if (player->askForSkillInvoke(QStringLiteral("FirstShowReward"))) {
                    LogMessage log;
                    log.type = QStringLiteral("#FirstShowReward");
                    log.from = player;
                    room->sendLog(log);
                    player->drawCards(2);
                }
            }
        }

        //CompanionEffect  and  HalfMaxHpLeft
        if (player->isAlive() && player->hasShownAllGenerals()) {
            if (player->getMark(QStringLiteral("CompanionEffect")) > 0) {
                player->setMark(QStringLiteral("CompanionEffect"), 0);

                //bonus Postpone
                if (Config.HegemonyCompanionReward == QStringLiteral("Postponed")) {
                    player->gainMark(QStringLiteral("@CompanionEffect"));
                    QString attachName = QStringLiteral("companion_attach");
                    if ((player != nullptr) && !player->hasSkill(attachName))
                        room->attachSkillToPlayer(player, attachName);
                } else {
                    QStringList choices;
                    if (player->isWounded())
                        choices << QStringLiteral("recover");
                    choices << QStringLiteral("draw") << QStringLiteral("cancel");
                    LogMessage log;
                    log.type = QStringLiteral("#CompanionEffect");
                    log.from = player;
                    room->sendLog(log);
                    QString choice = room->askForChoice(player, QStringLiteral("CompanionEffect"), choices.join(QStringLiteral("+")));
                    if (choice == QStringLiteral("recover")) {
                        RecoverStruct recover;
                        recover.who = player;
                        recover.recover = 1;
                        room->recover(player, recover);
                    } else if (choice == QStringLiteral("draw"))
                        player->drawCards(2);
                }
                room->setEmotion(player, QStringLiteral("companion"));
            }
            if (player->getMark(QStringLiteral("HalfMaxHpLeft")) > 0) {
                player->setMark(QStringLiteral("HalfMaxHpLeft"), 0);

                if (Config.HegemonyHalfHpReward == QStringLiteral("Postponed")) {
                    //bonus Postpone
                    player->gainMark(QStringLiteral("@HalfLife"));
                    QString attachName = QStringLiteral("halflife_attach");
                    if ((player != nullptr) && !player->hasSkill(attachName))
                        room->attachSkillToPlayer(player, attachName);
                } else {
                    LogMessage log;
                    log.type = QStringLiteral("#HalfMaxHpLeft");
                    log.from = player;
                    room->sendLog(log);
                    if (player->askForSkillInvoke(QStringLiteral("userdefine:halfmaxhp")))
                        player->drawCards(1);
                }
            }
        }
    }

    case BeforeCardsMove: { //to be record? not effect
        CardsMoveOneTimeStruct move = data.value<CardsMoveOneTimeStruct>();
        ServerPlayer *player = qobject_cast<ServerPlayer *>(move.from);
        if (player != nullptr) {
            IDSet shownIds;
            foreach (int id, move.card_ids) {
                if (player->isShownHandcard(id))
                    shownIds << id;
            }
            if (!shownIds.isEmpty()) {
                player->removeShownHandCards(shownIds, false, true);
                move.shown_ids = shownIds.values();
                data = QVariant::fromValue(move);
            }

            IDSet brokenIds;
            foreach (int id, move.card_ids) {
                if (player->isBrokenEquip(id))
                    brokenIds << id;
            }
            if (!brokenIds.isEmpty()) {
                player->removeBrokenEquips(brokenIds, false, true);
                move.broken_ids = brokenIds.values();
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
    bool classical = (Config.value(QStringLiteral("1v1/Rule"), QStringLiteral("2013")).toString() == QStringLiteral("Classical"));
    QString new_general;
    if (classical) {
        new_general = player->tag[QStringLiteral("1v1ChangeGeneral")].toString();
        player->tag.remove(QStringLiteral("1v1ChangeGeneral"));
    } else {
        QStringList list = player->tag[QStringLiteral("1v1Arrange")].toStringList();
        new_general = room->askForGeneral(player, list);
        list.removeOne(new_general);
        player->tag[QStringLiteral("1v1Arrange")] = QVariant::fromValue(list);
    }

    if (player->getPhase() != QSanguosha::PhaseNotActive) {
        player->setPhase(QSanguosha::PhaseNotActive);
        room->broadcastProperty(player, "phase");
    }
    room->revivePlayer(player, false);
    room->changeHero(player, new_general, true, true);
    Q_ASSERT(player->getGeneral() != nullptr);
    if (player->getGeneral()->getKingdom() == QStringLiteral("zhu") || player->getGeneral()->getKingdom() == QStringLiteral("touhougod")) {
        QString new_kingdom = room->askForKingdom(player);
        room->setPlayerProperty(player, "kingdom", new_kingdom);

        LogMessage log;
        log.type = QStringLiteral("#ChooseKingdom");
        log.from = player;
        log.arg = new_kingdom;
        room->sendLog(log);
    }
    room->addPlayerHistory(player, QStringLiteral("."));

    if (player->getKingdom() != player->getGeneral()->getKingdom())
        room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());

    QList<ServerPlayer *> notified = classical ? room->getOtherPlayers(player, true) : room->getPlayers();
    room->doBroadcastNotify(notified, QSanProtocol::S_COMMAND_REVEAL_GENERAL, JsonArray() << player->objectName() << new_general);

    if (!player->faceUp())
        player->turnOver();

    if (player->isChained())
        room->setPlayerProperty(player, "chained", false);

    room->setTag(QStringLiteral("FirstRound"), true);
    int draw_num = classical ? 4 : player->getMaxHp();

    DrawNCardsStruct s;
    s.player = player;
    s.isInitial = true;
    s.n = draw_num;

    QVariant data = QVariant::fromValue(s);
    room->getThread()->trigger(DrawInitialCards, data);
    s = data.value<DrawNCardsStruct>();
    draw_num = s.n;

    try {
        player->drawCards(draw_num, QStringLiteral("initialDraw"));
        room->setTag(QStringLiteral("FirstRound"), false);
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            room->setTag(QStringLiteral("FirstRound"), false);
        throw triggerEvent;
    }
    room->getThread()->trigger(AfterDrawInitialCards, data);
}

void GameRule::changeGeneralXMode(ServerPlayer *player) const
{
    Config.AIDelay = Config.OriginAIDelay;

    Room *room = player->getRoom();
    ServerPlayer *leader = player->tag[QStringLiteral("XModeLeader")].value<ServerPlayer *>();
    Q_ASSERT(leader);
    QStringList backup = leader->tag[QStringLiteral("XModeBackup")].toStringList();
    QString general = room->askForGeneral(leader, backup);
    if (backup.contains(general))
        backup.removeOne(general);
    else
        backup.takeFirst();
    leader->tag[QStringLiteral("XModeBackup")] = QVariant::fromValue(backup);
    room->revivePlayer(player);
    room->changeHero(player, general, true, true);
    Q_ASSERT(player->getGeneral() != nullptr);
    if (player->getGeneral()->getKingdom() == QStringLiteral("zhu") || player->getGeneral()->getKingdom() == QStringLiteral("touhougod")) {
        QString new_kingdom = room->askForKingdom(player);
        room->setPlayerProperty(player, "kingdom", new_kingdom);

        LogMessage log;
        log.type = QStringLiteral("#ChooseKingdom");
        log.from = player;
        log.arg = new_kingdom;
        room->sendLog(log);
    }
    room->addPlayerHistory(player, QStringLiteral("."));

    if (player->getKingdom() != player->getGeneral()->getKingdom())
        room->setPlayerProperty(player, "kingdom", player->getGeneral()->getKingdom());

    if (!player->faceUp())
        player->turnOver();

    if (player->isChained())
        room->setPlayerProperty(player, "chained", false);

    room->setTag(QStringLiteral("FirstRound"), true);
    DrawNCardsStruct s;
    s.player = player;
    s.isInitial = true;
    s.n = 4;
    QVariant data = QVariant::fromValue(s);
    room->getThread()->trigger(DrawInitialCards, data);
    s = data.value<DrawNCardsStruct>();
    int num = s.n;
    try {
        player->drawCards(num, QStringLiteral("initialDraw"));
        room->setTag(QStringLiteral("FirstRound"), false);
    } catch (TriggerEvent triggerEvent) {
        if (triggerEvent == TurnBroken)
            room->setTag(QStringLiteral("FirstRound"), false);
        throw triggerEvent;
    }
    room->getThread()->trigger(AfterDrawInitialCards, data);
}

void GameRule::rewardAndPunish(ServerPlayer *killer, ServerPlayer *victim) const
{
    Q_ASSERT(killer->getRoom() != nullptr);
    Room *room = killer->getRoom();
    if (killer->isDead() || room->getMode() == QStringLiteral("06_XMode"))
        return;

    if (isHegemonyGameMode(room->getMode()) && !killer->hasShownOneGeneral())
        return;

    if (killer->getRoom()->getMode() == QStringLiteral("06_3v3")) {
        if (Config.value(QStringLiteral("3v3/OfficialRule"), QStringLiteral("2013")).toString().startsWith(QStringLiteral("201")))
            killer->drawCards(2);
        else
            killer->drawCards(3);
    } else if (isHegemonyGameMode(room->getMode())) {
        if (!killer->isFriendWith(victim)) {
            int n = 1;
            if (Config.HegemonyCareeristKillReward == QStringLiteral("AlwaysDraw3") && killer->getRoleString() == QStringLiteral("careerist")) {
                n = 3;
            } else {
                foreach (ServerPlayer *p, room->getOtherPlayers(victim)) {
                    if (victim->isFriendWith(p))
                        ++n;
                }
            }
            killer->drawCards(n);
        } else
            killer->throwAllHandCardsAndEquips();
    } else {
        if (victim->getRoleString() == QStringLiteral("rebel") && killer != victim)
            killer->drawCards(3);
        else if (victim->getRoleString() == QStringLiteral("loyalist") && killer->getRoleString() == QStringLiteral("lord"))
            killer->throwAllHandCardsAndEquips();
    }
}

QString GameRule::getWinner(ServerPlayer *victim) const
{
    Room *room = victim->getRoom();
    QString winner;

    if (room->getMode() == QStringLiteral("06_3v3")) {
        switch (victim->getRole()) {
        case QSanguosha::RoleLord:
            winner = QStringLiteral("renegade+rebel");
            break;
        case QSanguosha::RoleRenegade:
            winner = QStringLiteral("lord+loyalist");
            break;
        default:
            break;
        }
    } else if (room->getMode() == QStringLiteral("06_XMode")) {
        QString role = victim->getRoleString();
        ServerPlayer *leader = victim->tag[QStringLiteral("XModeLeader")].value<ServerPlayer *>();
        if (leader->tag[QStringLiteral("XModeBackup")].toStringList().isEmpty()) {
            if (role.startsWith(QLatin1Char('r')))
                winner = QStringLiteral("lord+loyalist");
            else
                winner = QStringLiteral("renegade+rebel");
        }
    } else if (isHegemonyGameMode(room->getMode())) {
        QList<ServerPlayer *> players = room->getAlivePlayers();
        ServerPlayer *win_player = players.first();
        if (players.length() == 1) {
            QStringList winners;
            if (!win_player->hasShownGeneral())
                win_player->showGeneral(true, false, false);
            if ((win_player->getGeneral2() != nullptr) && !win_player->hasShownGeneral2())
                win_player->showGeneral(false, false, false);

            foreach (ServerPlayer *p, room->getPlayers()) {
                if (win_player->isFriendWith(p))
                    winners << p->objectName();
            }
            winner = winners.join(QStringLiteral("+"));
        } else {
            QList<ServerPlayer *> winners;
            int careerist_threshold = (room->getPlayers().length() / 2);
            QMap<QString, QList<ServerPlayer *>> role_count;
            QMap<QString, QList<ServerPlayer *>> dead_role_count;
            foreach (ServerPlayer *p, room->getAllPlayers(true)) {
                QString role = p->getRoleString();
                if (role_count.contains(role)) {
                    QList<ServerPlayer *> players = role_count[role];
                    players.append(p);
                    role_count[role] = players;
                } else {
                    QList<ServerPlayer *> players;
                    players.append(p);
                    role_count[role] = players;
                }

                if (p->isDead()) {
                    if (dead_role_count.contains(role)) {
                        QList<ServerPlayer *> players = dead_role_count[role];
                        players.append(p);
                        dead_role_count[role] = players;
                    } else {
                        QList<ServerPlayer *> players;
                        players.append(p);
                        dead_role_count[role] = players;
                    }
                }
            }

            QStringList roles = role_count.keys();
            foreach (QString role, roles) {
                QList<ServerPlayer *> players = role_count[role];
                if (players.length() == dead_role_count[role].length()) //all dead
                    role_count.remove(role);
            }

            if (role_count.keys().length() == 1) {
                QString role = role_count.keys().first();
                if (role == QStringLiteral("careerist")) {
                    foreach (ServerPlayer *p, role_count[role]) {
                        if (p->isAlive())
                            winners << p;
                    }
                    if (winners.length() > 1)
                        winners.clear();
                } else {
                    if (role_count[role].length() <= careerist_threshold)
                        winners = role_count[role];
                    else { //for hidden careerist
                        if (dead_role_count[role].length() >= careerist_threshold && (role_count[role].length() - dead_role_count[role].length()) == 1) {
                            foreach (ServerPlayer *p, role_count[role]) {
                                if (p->isAlive())
                                    winners << p;
                            }
                        }
                    }
                }
            }

            QStringList winner_names;
            foreach (ServerPlayer *p, winners) {
                winner_names << p->objectName();
                if (!p->hasShownGeneral())
                    p->showGeneral(true, false, false);
                if ((p->getGeneral2() != nullptr) && !p->hasShownGeneral2())
                    p->showGeneral(false, false, false);
            }
            winner = winner_names.join(QStringLiteral("+"));
        }

    } else {
        QStringList alive_roles = room->aliveRoles(victim);
        switch (victim->getRole()) {
        case QSanguosha::RoleLord: {
            if (alive_roles.length() == 1 && alive_roles.first() == QStringLiteral("renegade"))
                winner = room->getAlivePlayers().first()->objectName();
            else
                winner = QStringLiteral("rebel");
            break;
        }
        case QSanguosha::RoleRebel:
        case QSanguosha::RoleRenegade: {
            if (!alive_roles.contains(QStringLiteral("rebel")) && !alive_roles.contains(QStringLiteral("renegade"))) {
                winner = QStringLiteral("lord+loyalist");
                if (victim->getRoleString() == QStringLiteral("renegade") && !alive_roles.contains(QStringLiteral("loyalist")))
                    room->setTag(QStringLiteral("RenegadeInFinalPK"), true);
            }
            break;
        }
        default:
            break;
        }
    }

    return winner;
}
