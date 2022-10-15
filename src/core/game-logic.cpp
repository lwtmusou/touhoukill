#include "game-logic.h"
#include "RoomObject.h"
#include "player.h"

using namespace QSanguosha;

class GameLogicPrivate
{
public:
    RoomObject *room;
    QList<Player *> dying;
};

GameLogic::GameLogic()
    : d(new GameLogicPrivate)
{
}

GameLogic::~GameLogic()
{
    delete d;
}

RoomObject *GameLogic::room()
{
    return d->room;
}

const RoomObject *GameLogic::room() const
{
    return d->room;
}

void GameLogic::setRoom(RoomObject *room)
{
    d->room = room;
}

void GameLogic::enterDying(const DeathStruct &_death)
{
    Player *player = _death.who;

    if (player->dyingFactor() > player->maxHp()) {
        killPlayer(_death);
        return;
    }

    DeathStruct death = _death;
    death.nowAskingForPeaches = nullptr;
    player->tag[QStringLiteral("GameLogic_Dying")] = player->tag[QStringLiteral("GameLogic_Dying")].toInt() + 1;
    d->dying << player;

#if 0
    QJsonArray arg;
    arg << QSanProtocol::S_GAME_EVENT_PLAYER_DYING << player->objectName();
    doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
#endif

    QVariant death_data = QVariant::fromValue(death);
    std::function<bool(const ProcessBreakStruct *self)> brokenCallback;

    try {
        do {
            bool disableEnterDying = trigger(EnterDying, death_data);
            if (disableEnterDying || player->isDead() || (player->hp() >= player->dyingFactor()))
                break;

            bool disableDying = trigger(Dying, death_data);
            if (disableDying || player->isDead() || (player->hp() >= player->dyingFactor()))
                break;

            LogStruct log;
            log.type = QStringLiteral("#AskForPeaches");
            log.from = player;
            log.to = player->roomObject()->players(false);
            log.arg = QString::number(player->dyingFactor() - player->hp());
            sendLog(log);

            foreach (Player *p, player->roomObject()->players(false)) {
                death = death_data.value<DeathStruct>();
                death.nowAskingForPeaches = p;
                death_data = QVariant::fromValue(death);
                bool disableAskForPeaches = trigger(AskForPeaches, death_data);
                if (disableAskForPeaches || player->isDead() || (player->hp() >= player->dyingFactor()))
                    break;
            }
            death = death_data.value<DeathStruct>();
            death.nowAskingForPeaches = nullptr;
            death_data = QVariant::fromValue(death);
            trigger(AskForPeachesDone, death_data);
        } while (false);
    } catch (ProcessBreakStruct breakStruct) {
        // defer the process broken
        //
        // DO NOT DO THE REMAINING PROCESS OF THIS FUNCTION HERE
        // Every process may throw so all we can do is not to run any process during catch.
        //
        // Actually we need to throw after cleanup has finished
        // But if cleanup introduce process we need another try - catch block for catching another process broken
        // All the stuff is for skills like "Danshou" (Zhu Ran - Sanguosha, legacy) or "Juhe" (Konpaku Youmu - TouhouSatsu)

        brokenCallback = breakStruct.stopHereCallback;

        death = death_data.value<DeathStruct>();
        death.nowAskingForPeaches = nullptr;
        death_data = QVariant::fromValue(death);
    } catch (GameOverException) {
        // free allocated memory here!
        // and then --
        throw;
    }

    d->dying.removeLast();
    player->tag[QStringLiteral("GameLogic_Dying")] = player->tag[QStringLiteral("GameLogic_Dying")].toInt() - 1;

    if (player->isAlive()) {
#if 0
        QJsonArray arg;
        arg << QSanProtocol::S_GAME_EVENT_PLAYER_QUITDYING << player->objectName();
        doBroadcastNotify(QSanProtocol::S_COMMAND_LOG_EVENT, arg);
#endif
    }

    try {
        trigger(QuitDying, death_data);
    } catch (ProcessBreakStruct breakStruct) {
        if (!brokenCallback) {
            brokenCallback = breakStruct.stopHereCallback;
        } else {
            // Break process again during process breaking
            // There is no way for this happening, but we'd prevent the crash here
            // anyway let's just do nothing and wait for bug report after all...
        }
    } catch (GameOverException) {
        // free allocated memory here!
        // and then --
        throw;
    }

    // Finally, the process broken
    if (brokenCallback) {
        ProcessBreakStruct breakStruct;
        breakStruct.currentData = &death_data;
        breakStruct.currentFunctionName = QString::fromUtf8(__FUNCTION__);
        breakStruct.currentProcessingEvent = NonTrigger;
        breakStruct.stopHereCallback = brokenCallback;

        if (!brokenCallback(&breakStruct))
            throw breakStruct;
    }
}

void GameLogic::killPlayer(const DeathStruct &death)
{
}

void GameLogic::buryPlayer(Player *victim)
{
}

void GameLogic::revivePlayer(Player *player, bool initialize)
{
}

void GameLogic::gameOver(const QString &winner, bool isSurrender)
{
}

void GameLogic::handleAcquireLoseSkills(Player *player, const QStringList &acquireSkills, const QStringList &loseSkills, bool acquire_only)
{
}

void GameLogic::showPlayerHiddenSkill(Player *player, const QString &skill_name)
{
}

void GameLogic::setPlayerHP(Player *player, int value)
{
}

void GameLogic::setPlayerMaxHP(Player *player, int value)
{
}

void GameLogic::setPlayerChained(Player *player, bool is_chained)
{
}

void GameLogic::setPlayerRemoved(Player *player, bool is_removed)
{
}

void GameLogic::setPlayerRoleShown(Player *player, bool is_role_shown)
{
}

void GameLogic::setPlayerKingdom(Player *player, const QString &kingdom)
{
}

void GameLogic::setPlayerGender(Player *player, QSanguosha::Gender gender)
{
}

void GameLogic::playerGainMark(Player *player, const QString &mark_name, int count)
{
}

void GameLogic::playerLoseMark(Player *player, const QString &mark_name, int count)
{
}

bool GameLogic::useCard(const CardUseStruct &card_use, bool add_history)
{
    return false;
}

bool GameLogic::cardEffect(const CardEffectStruct &effect)
{
    return false;
}

void GameLogic::damage(const DamageStruct &data)
{
}

void GameLogic::applyDamage(Player *victim, const DamageStruct &damage)
{
}

void GameLogic::loseHp(Player *victim, int lose)
{
}

void GameLogic::loseMaxHp(Player *victim, int lose)
{
}

bool GameLogic::changeMaxHpForAwakenSkill(Player *player, int magnitude)
{
    return false;
}

void GameLogic::recover(Player *player, const RecoverStruct &recover, bool set_emotion)
{
}

void GameLogic::turnPlayerOver(Player *player)
{
}

void GameLogic::letPlayerPlay(Player *target, const QList<QSanguosha::Phase> &phases)
{
}

void GameLogic::changePlayerPhase(Player *target, QSanguosha::Phase from, QSanguosha::Phase to)
{
}

void GameLogic::skipPlayerPhase(Player *player, QSanguosha::Phase phase, bool is_cost, bool send_log)
{
}

void GameLogic::insertPlayerPhases(Player *player, const QList<QSanguosha::Phase> &new_phases, int index)
{
}

void GameLogic::exchangePlayerPhases(Player *player, QSanguosha::Phase from, QSanguosha::Phase to)
{
}

void GameLogic::givePlayerAnExtraTurn(Player *benefiter)
{
}

void GameLogic::judge(JudgeStruct &judge_struct)
{
}

void GameLogic::retrial(const Card *card, Player *player, JudgeStruct *judge, const QString &skill_name, bool exchange)
{
}

void GameLogic::sendJudgeResult(const JudgeStruct *judge)
{
}

bool GameLogic::pindian(Player *source, Player *target, const QString &reason)
{
    return false;
}

QList<int> GameLogic::peekCards(int n, bool bottom)
{
    return {};
}

void GameLogic::shuffleDrawPile()
{
}

void GameLogic::fillAG(const QList<int> &card_ids, const QList<int> &disabled_ids, const QList<Player *> &viewers)
{
}

void GameLogic::takeAG(Player *player, int card_id, bool move_cards, QSanguosha::Place fromPlace)
{
}

void GameLogic::clearAG(const QList<Player *> &viewers)
{
}

void GameLogic::provide(const Card *card, Player *who)
{
}

void GameLogic::sendLog(const LogStruct &log)
{
}

void GameLogic::showCard(Player *player, int card_id, Player *only_viewer)
{
}

void GameLogic::showAllCards(Player *player, Player *to)
{
}

void GameLogic::notifySkillInvoked(Player *player, const QString &skill_name)
{
}

void GameLogic::broadcastSkillInvoke(const QString &skillName)
{
}

void GameLogic::broadcastSkillInvoke(const QString &skillName, const QString &category)
{
}

void GameLogic::broadcastSkillInvoke(const QString &skillName, int type)
{
}

void GameLogic::broadcastSkillInvoke(const QString &skillName, bool isMale, int type)
{
}

void GameLogic::broadcastSkillInvoke(const Player *player, const Card *card)
{
}

void GameLogic::broadcastSkillInvoke(const Player *player, const QString *card_name)
{
}

void GameLogic::doIndicateAnimation(const Player *from, const Player *to)
{
}

void GameLogic::doLightbox(const QString &lightboxName, int duration)
{
}

void GameLogic::doHuashenAnimation(const Player *player, const QString &skill_name)
{
}

void GameLogic::doNullificationAnimation(const Player *from, const Player *to)
{
}

void GameLogic::doFireAnimation(const Player *player)
{
}

void GameLogic::doLightningAnimation(const Player *player)
{
}

void GameLogic::doBattleArrayAnimation(Player *player, Player *target)
{
}

void GameLogic::setEmotion(Player *target, const QString &emotion)
{
}

void GameLogic::doAnimation(QSanProtocol::AnimateType type, const QString &arg1, const QString &arg2, QList<Player *> players)
{
}

void GameLogic::changeHero(Player *player, const QString &new_general, bool full_state, bool invoke_start, bool isSecondaryHero, bool sendLog)
{
}

void GameLogic::transformGeneral(Player *player, QString general_name, int head)
{
}

void GameLogic::setPlayerSkillInvalidity(Player *player, const QString &skill_name, bool invalidity, bool trigger_event)
{
}

void GameLogic::reverseFor3v3(const Card *card, Player *player, QList<Player *> &list)
{
}

void GameLogic::addToPlayerSelectedGeneral(Player *target, const QString &general_name)
{
}

void GameLogic::clearPlayerSelectedGeneral(Player *target)
{
}

void GameLogic::drawCards(Player *player, int n, const QString &reason)
{
}

void GameLogic::drawCards(QList<Player *> players, int n, const QString &reason)
{
}

void GameLogic::drawCards(QList<Player *> players, QList<int> n_list, const QString &reason)
{
}

void GameLogic::obtainCard(Player *target, const Card *card, bool unhide)
{
}

void GameLogic::obtainCard(Player *target, int card_id, bool unhide)
{
}

void GameLogic::throwCard(int card_id, Player *who, Player *thrower, bool notifyLog)
{
}

void GameLogic::throwCard(const Card *card, Player *who, Player *thrower, bool notifyLog)
{
}

void GameLogic::throwPlayerAllEquips(const Player *victim)
{
}

void GameLogic::throwPlayerAllHandCards(const Player *victim)
{
}

void GameLogic::throwPlayerDelayTrickRegion(const Player *victim)
{
}

void GameLogic::throwPlayerAllCards(const Player *victim)
{
}

void GameLogic::addToPlayerPile(Player *owner, const QString &pile_name, const Card *card, bool open)
{
}

void GameLogic::addToPlayerPile(Player *owner, const QString &pile_name, int card_id, bool open)
{
}

void GameLogic::addToPlayerPile(Player *owner, const QString &pile_name, const IdSet &card_id, bool open)
{
}

void GameLogic::addToPlayerShownHandCards(Player *target, const IdSet &card_ids)
{
}

void GameLogic::removePlayerShownHandCards(Player *target, const IdSet &card_ids)
{
}

void GameLogic::addPlayerBrokenEquip(Player *target, const IdSet &ids)
{
}

void GameLogic::removePlayerBrokenEquip(Player *target, const IdSet &ids)
{
}

void GameLogic::addPlayerHiddenGeneral(Player *target, const QStringList &generals)
{
}

void GameLogic::removePlayerHiddenGeneral(Player *target, const QStringList)
{
}

void GameLogic::clearPlayerPrivatePile(const Player *target, const QString &pile_name)
{
}

void GameLogic::clearPlayerAllPrivatePiles(const Player *target)
{
}

void GameLogic::doJileiShow(Player *player, const IdSet &jilei_ids)
{
}

void GameLogic::forcePlayerDiscard(const Player *target, int discard_num, int include_equip, bool is_discard)
{
}

void GameLogic::moveCardTo(const Card *card, Player *dstPlayer, QSanguosha::Place dstPlace, bool forceVisible)
{
}

void GameLogic::moveCardsAtomic(CardsMoveStruct cards_move, bool forceVisible)
{
}

bool GameLogic::trigger(QSanguosha::TriggerEvent e, QVariant &data)
{
    return true;
}

void GameLogic::activate(CardUseStruct &use, Player *player)
{
}

void GameLogic::askForGuanxing(QList<int> &upCards, QList<int> &downCards, Player *player, const QList<int> &cards, QSanguosha::GuanxingType guanxingType, QString skillName)
{
}

void GameLogic::askForUseCard(CardUseStruct &use, Player *player, const QString &reason, const QString &pattern, bool optional, const QString &prompt, bool addHistory,
                              const QList<QSanguosha::HandlingMethod> &methods, bool enableConversion)
{
}

void GameLogic::askForUseCard(CardUseStruct &use, Player *player, const QString &skillName, int patternIndex, bool optional, const QString &prompt,
                              const QList<QSanguosha::HandlingMethod> &methods)
{
}

void GameLogic::askForUseSlashTo(CardUseStruct &use, Player *slasher, const QString &reason, Player *victim, bool disableExtra, bool optional, const QString &prompt,
                                 bool addHistory, bool enableConversion)
{
}

void GameLogic::askForUseSlashTo(CardUseStruct &use, Player *slasher, const QString &reason, const QList<Player *> &victims, bool disableExtra, bool optional,
                                 const QString &prompt, bool addHistory, bool enableConversion)
{
}

void GameLogic::askForJink(CardUseStruct &use, Player *player, const CardEffectStruct &slashUse, bool enableConversion)
{
}

void GameLogic::askForSinglePeach(CardUseStruct &use, Player *toAsk, const DeathStruct &dying, bool enableConversion)
{
}

void GameLogic::askForNullification(CardUseStruct &use, const CardEffectStruct &trickEffect, bool enableConversion)
{
}

void GameLogic::askForNullification(CardUseStruct &use, const QList<Player *> &toAsk, const CardEffectStruct &trickEffect, bool enableConversion)
{
}

void GameLogic::askForResponseCard(CardResponseStruct &response, Player *player, const QString &reason, const QString &pattern, bool optional, const QString &prompt,
                                   bool isRetrial, bool isProvision, const QList<QSanguosha::HandlingMethod> &methods, bool enableConversion)
{
}

void GameLogic::askForResponseCard(CardResponseStruct &response, Player *player, const QString &skillName, int patternIndex, bool optional, const QString &prompt, bool isRetrial,
                                   bool isProvision, const QList<QSanguosha::HandlingMethod> &methods)
{
}

void GameLogic::askForDiscard(IdSet &discardedIds, Player *target, const QString &reason, int max_num, int min_num, bool include_equip, bool optional, const QString &prompt)
{
}

void GameLogic::askForDiscard(IdSet &discardedIds, Player *target, const QString &reason, const QString &pattern, bool optional, const QString &prompt)
{
}

void GameLogic::askForDiscard(IdSet &discardedIds, Player *target, const QString &skillName, int patternIndex, bool optional, const QString &prompt)
{
}

void GameLogic::askForExchange(IdSet &discardedIds, Player *target, const QString &reason, int max_num, int min_num, bool include_equip, bool optional, const QString &prompt)
{
}

void GameLogic::askForExchange(IdSet &discardedIds, Player *target, const QString &reason, const QString &pattern, bool optional, const QString &prompt)
{
}

void GameLogic::askForExchange(IdSet &discardedIds, Player *target, const QString &skillName, int patternIndex, bool optional, const QString &prompt)
{
}

void GameLogic::askForCardGive(QHash<Player *, int> &give, Player *giver, const QString &reason, const IdSet &cardIds, bool optional, const QString &prompt,
                               const QList<Player *> players)
{
}

QSanguosha::Suit GameLogic::askForSuit(Player *player, const QString &reason)
{
    return QSanguosha::NoSuit;
}

QString GameLogic::askForKingdom(Player *player, const QString &reason)
{
    return QString();
}

bool GameLogic::askForSkillInvoke(Player *player, const QString &skill_name, const QString &prompt)
{
    return false;
}

QString GameLogic::askForChoice(Player *player, const QString &reason, const QStringList &choices, const QString &defaultChoice)
{
    return QString();
}

QString GameLogic::askForGeneral(Player *player, const QStringList &generals, const QString &defaultChoice)
{
    return QString();
}

int GameLogic::askForCardChosen(Player *player, Player *who, const QString &flags, const QString &reason, bool handcard_visible, QSanguosha::HandlingMethod method,
                                const QList<int> &disabled_ids)
{
    return -1;
}

int GameLogic::askForAG(Player *player, const QList<int> &card_ids, bool refusable, const QString &reason)
{
    return -1;
}

const Card *GameLogic::askForPindian(PindianStruct &pindian)
{
    return nullptr;
}

QList<const Card *> GameLogic::askForPindianRace(Player *from, Player *to, const QString &reason)
{
    return {};
}

QList<Player *> GameLogic::askForPlayersChosen(Player *chooser, const QString &reason, const QList<Player *> targets, int max_num, int min_num, bool optional,
                                               const QString &prompt)
{
    return {};
}

Player *GameLogic::askForPlayerChosen(Player *chooser, const QString &reason, const QList<Player *> targets, bool optional, const QString &prompt)
{
    QList<Player *> p = askForPlayersChosen(chooser, reason, targets, 1, 1, optional, prompt);
    return p.isEmpty() ? nullptr : p.first();
}

TriggerDetail GameLogic::askForTriggerOrder(Player *player, const QList<TriggerDetail> &sameTiming, bool cancelable)
{
    return TriggerDetail(nullptr);
}

void GameLogic::cheat(Player *player, const QVariant &args)
{
}

bool GameLogic::makeSurrender(Player *player)
{
    return false;
}
