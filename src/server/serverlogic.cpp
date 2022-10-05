#include "serverlogic.h"

// ALL FUNCTIONS ARE TODO FOR NOW

ServerLogic::ServerLogic(RoomObject *room)
    : room(room)
{
}

ServerLogic::~ServerLogic()
{
}

void ServerLogic::enterDying(Player *player, DamageStruct *reason)
{
}

void ServerLogic::buryPlayer(Player *victim)
{
}

void ServerLogic::killPlayer(Player *victim, DamageStruct *reason)
{
}

void ServerLogic::revivePlayer(Player *player, bool initialize)
{
}

void ServerLogic::gameOver(const QString &winner, bool isSurrender)
{
}

void ServerLogic::handleAcquireLoseSkills(Player *player, const QStringList &acquireSkills, const QStringList &loseSkills, bool acquire_only)
{
}

void ServerLogic::showPlayerHiddenSkill(Player *player, const QString &skill_name)
{
}

void ServerLogic::setPlayerHP(Player *player, int value)
{
}

void ServerLogic::setPlayerMaxHP(Player *player, int value)
{
}

void ServerLogic::setPlayerChained(Player *player, bool is_chained)
{
}

void ServerLogic::setPlayerRemoved(Player *player, bool is_removed)
{
}

void ServerLogic::setPlayerRoleShown(Player *player, bool is_role_shown)
{
}

void ServerLogic::setPlayerKingdom(Player *player, const QString &kingdom)
{
}

void ServerLogic::setPlayerGender(Player *player, QSanguosha::Gender gender)
{
}

void ServerLogic::playerGainMark(Player *player, const QString &mark_name, int count)
{
}

void ServerLogic::playerLoseMark(Player *player, const QString &mark_name, int count)
{
}

bool ServerLogic::useCard(const CardUseStruct &card_use, bool add_history)
{
    return false;
}

bool ServerLogic::cardEffect(const CardEffectStruct &effect)
{
    return false;
}

void ServerLogic::damage(const DamageStruct &data)
{
}

void ServerLogic::applyDamage(Player *victim, const DamageStruct &damage)
{
}

void ServerLogic::loseHp(Player *victim, int lose)
{
}

void ServerLogic::loseMaxHp(Player *victim, int lose)
{
}

bool ServerLogic::changeMaxHpForAwakenSkill(Player *player, int magnitude)
{
    return false;
}

void ServerLogic::recover(Player *player, const RecoverStruct &recover, bool set_emotion)
{
}

void ServerLogic::turnPlayerOver(Player *player)
{
}

void ServerLogic::letPlayerPlay(Player *target, const QList<QSanguosha::Phase> &phases)
{
}

void ServerLogic::changePlayerPhase(Player *target, QSanguosha::Phase from, QSanguosha::Phase to)
{
}

void ServerLogic::skipPlayerPhase(Player *player, QSanguosha::Phase phase, bool is_cost, bool send_log)
{
}

void ServerLogic::insertPlayerPhases(Player *player, const QList<QSanguosha::Phase> &new_phases, int index)
{
}

void ServerLogic::exchangePlayerPhases(Player *player, QSanguosha::Phase from, QSanguosha::Phase to)
{
}

void ServerLogic::givePlayerAnExtraTurn(Player *benefiter)
{
}

void ServerLogic::judge(JudgeStruct &judge_struct)
{
}

void ServerLogic::retrial(const Card *card, Player *player, JudgeStruct *judge, const QString &skill_name, bool exchange)
{
}

void ServerLogic::sendJudgeResult(const JudgeStruct *judge)
{
}

bool ServerLogic::pindian(Player *source, Player *target, const QString &reason)
{
    return false;
}

QList<int> ServerLogic::peekCards(int n, bool bottom)
{
    return {};
}

void ServerLogic::shuffleDrawPile()
{
}

void ServerLogic::fillAG(const QList<int> &card_ids, const QList<int> &disabled_ids, const QList<Player *> &viewers)
{
}

void ServerLogic::takeAG(Player *player, int card_id, bool move_cards, QSanguosha::Place fromPlace)
{
}

void ServerLogic::clearAG(const QList<Player *> &viewers)
{
}

void ServerLogic::provide(const Card *card, Player *who)
{
}

void ServerLogic::sendLog(const LogStruct &log)
{
}

void ServerLogic::showCard(Player *player, int card_id, Player *only_viewer)
{
}

void ServerLogic::showAllCards(Player *player, Player *to)
{
}

void ServerLogic::notifySkillInvoked(Player *player, const QString &skill_name)
{
}

void ServerLogic::broadcastSkillInvoke(const QString &skillName)
{
}

void ServerLogic::broadcastSkillInvoke(const QString &skillName, const QString &category)
{
}

void ServerLogic::broadcastSkillInvoke(const QString &skillName, int type)
{
}

void ServerLogic::broadcastSkillInvoke(const QString &skillName, bool isMale, int type)
{
}

void ServerLogic::broadcastSkillInvoke(const Player *player, const Card *card)
{
}

void ServerLogic::broadcastSkillInvoke(const Player *player, const QString *card_name)
{
}

void ServerLogic::doIndicateAnimation(const Player *from, const Player *to)
{
}

void ServerLogic::doLightbox(const QString &lightboxName, int duration)
{
}

void ServerLogic::doHuashenAnimation(const Player *player, const QString &skill_name)
{
}

void ServerLogic::doNullificationAnimation(const Player *from, const Player *to)
{
}

void ServerLogic::doFireAnimation(const Player *player)
{
}

void ServerLogic::doLightningAnimation(const Player *player)
{
}

void ServerLogic::doBattleArrayAnimation(Player *player, Player *target)
{
}

void ServerLogic::setEmotion(Player *target, const QString &emotion)
{
}

void ServerLogic::doAnimation(QSanProtocol::AnimateType type, const QString &arg1, const QString &arg2, QList<Player *> players)
{
}

void ServerLogic::changeHero(Player *player, const QString &new_general, bool full_state, bool invoke_start, bool isSecondaryHero, bool sendLog)
{
}

void ServerLogic::transformGeneral(Player *player, QString general_name, int head)
{
}

void ServerLogic::setPlayerSkillInvalidity(Player *player, const QString &skill_name, bool invalidity, bool trigger_event)
{
}

void ServerLogic::reverseFor3v3(const Card *card, Player *player, QList<Player *> &list)
{
}

void ServerLogic::addToPlayerSelectedGeneral(Player *target, const QString &general_name)
{
}

void ServerLogic::clearPlayerSelectedGeneral(Player *target)
{
}

void ServerLogic::drawCards(Player *player, int n, const QString &reason)
{
}

void ServerLogic::drawCards(QList<Player *> players, int n, const QString &reason)
{
}

void ServerLogic::drawCards(QList<Player *> players, QList<int> n_list, const QString &reason)
{
}

void ServerLogic::obtainCard(Player *target, const Card *card, bool unhide)
{
}

void ServerLogic::obtainCard(Player *target, int card_id, bool unhide)
{
}

void ServerLogic::throwCard(int card_id, Player *who, Player *thrower, bool notifyLog)
{
}

void ServerLogic::throwCard(const Card *card, Player *who, Player *thrower, bool notifyLog)
{
}

void ServerLogic::throwPlayerAllEquips(const Player *victim)
{
}

void ServerLogic::throwPlayerAllHandCards(const Player *victim)
{
}

void ServerLogic::throwPlayerDelayTrickRegion(const Player *victim)
{
}

void ServerLogic::throwPlayerAllCards(const Player *victim)
{
}

void ServerLogic::addToPlayerPile(Player *owner, const QString &pile_name, const Card *card, bool open)
{
}

void ServerLogic::addToPlayerPile(Player *owner, const QString &pile_name, int card_id, bool open)
{
}

void ServerLogic::addToPlayerPile(Player *owner, const QString &pile_name, const IdSet &card_id, bool open)
{
}

void ServerLogic::addToPlayerShownHandCards(Player *target, const IdSet &card_ids)
{
}

void ServerLogic::removePlayerShownHandCards(Player *target, const IdSet &card_ids)
{
}

void ServerLogic::addPlayerBrokenEquip(Player *target, const IdSet &ids)
{
}

void ServerLogic::removePlayerBrokenEquip(Player *target, const IdSet &ids)
{
}

void ServerLogic::addPlayerHiddenGeneral(Player *target, const QStringList &generals)
{
}

void ServerLogic::removePlayerHiddenGeneral(Player *target, const QStringList)
{
}

void ServerLogic::clearPlayerPrivatePile(const Player *target, const QString &pile_name)
{
}

void ServerLogic::clearPlayerAllPrivatePiles(const Player *target)
{
}

void ServerLogic::doJileiShow(Player *player, const IdSet &jilei_ids)
{
}

void ServerLogic::forcePlayerDiscard(const Player *target, int discard_num, int include_equip, bool is_discard)
{
}

void ServerLogic::moveCardTo(const Card *card, Player *dstPlayer, QSanguosha::Place dstPlace, bool forceVisible)
{
}

void ServerLogic::moveCardsAtomic(CardsMoveStruct cards_move, bool forceVisible)
{
}

void ServerLogic::activate(CardUseStruct &use, Player *player)
{
}

void ServerLogic::askForGuanxing(QList<int> &upCards, QList<int> &downCards, Player *player, const QList<int> &cards, QSanguosha::GuanxingType guanxingType, QString skillName)
{
}

void ServerLogic::askForUseCard(CardUseStruct &use, Player *player, const QString &reason, const QString &pattern, bool optional, const QString &prompt, bool addHistory,
                                const QList<QSanguosha::HandlingMethod> &methods, bool enableConversion)
{
}

void ServerLogic::askForUseCard(CardUseStruct &use, Player *player, const QString &skillName, int patternIndex, bool optional, const QString &prompt,
                                const QList<QSanguosha::HandlingMethod> &methods)
{
}

void ServerLogic::askForUseSlashTo(CardUseStruct &use, Player *slasher, const QString &reason, Player *victim, bool disableExtra, bool optional, const QString &prompt,
                                   bool addHistory, bool enableConversion)
{
}

void ServerLogic::askForUseSlashTo(CardUseStruct &use, Player *slasher, const QString &reason, const QList<Player *> &victims, bool disableExtra, bool optional,
                                   const QString &prompt, bool addHistory, bool enableConversion)
{
}

void ServerLogic::askForJink(CardUseStruct &use, Player *player, const CardEffectStruct &slashUse, bool enableConversion)
{
}

void ServerLogic::askForSinglePeach(CardUseStruct &use, Player *toAsk, const DeathStruct &dying, bool enableConversion)
{
}

void ServerLogic::askForNullification(CardUseStruct &use, const CardEffectStruct &trickEffect, bool enableConversion)
{
}

void ServerLogic::askForNullification(CardUseStruct &use, const QList<Player *> &toAsk, const CardEffectStruct &trickEffect, bool enableConversion)
{
}

void ServerLogic::askForResponseCard(CardResponseStruct &response, Player *player, const QString &reason, const QString &pattern, bool optional, const QString &prompt,
                                     bool isRetrial, bool isProvision, const QList<QSanguosha::HandlingMethod> &methods, bool enableConversion)
{
}

void ServerLogic::askForResponseCard(CardResponseStruct &response, Player *player, const QString &skillName, int patternIndex, bool optional, const QString &prompt, bool isRetrial,
                                     bool isProvision, const QList<QSanguosha::HandlingMethod> &methods)
{
}

void ServerLogic::askForDiscard(IdSet &discardedIds, Player *target, const QString &reason, int max_num, int min_num, bool include_equip, bool optional, const QString &prompt)
{
}

void ServerLogic::askForDiscard(IdSet &discardedIds, Player *target, const QString &reason, const QString &pattern, bool optional, const QString &prompt)
{
}

void ServerLogic::askForDiscard(IdSet &discardedIds, Player *target, const QString &skillName, int patternIndex, bool optional, const QString &prompt)
{
}

void ServerLogic::askForExchange(IdSet &discardedIds, Player *target, const QString &reason, int max_num, int min_num, bool include_equip, bool optional, const QString &prompt)
{
}

void ServerLogic::askForExchange(IdSet &discardedIds, Player *target, const QString &reason, const QString &pattern, bool optional, const QString &prompt)
{
}

void ServerLogic::askForExchange(IdSet &discardedIds, Player *target, const QString &skillName, int patternIndex, bool optional, const QString &prompt)
{
}

void ServerLogic::askForCardGive(QHash<Player *, int> &give, Player *giver, const QString &reason, const IdSet &cardIds, bool optional, const QString &prompt,
                                 const QList<Player *> players)
{
}

QSanguosha::Suit ServerLogic::askForSuit(Player *player, const QString &reason)
{
    return QSanguosha::NoSuit;
}

QString ServerLogic::askForKingdom(Player *player, const QString &reason)
{
    return QString();
}

bool ServerLogic::askForSkillInvoke(Player *player, const QString &skill_name, const QString &prompt)
{
    return false;
}

QString ServerLogic::askForChoice(Player *player, const QString &reason, const QStringList &choices, const QString &defaultChoice)
{
    return QString();
}

QString ServerLogic::askForGeneral(Player *player, const QStringList &generals, const QString &defaultChoice)
{
    return QString();
}

int ServerLogic::askForCardChosen(Player *player, Player *who, const QString &flags, const QString &reason, bool handcard_visible, QSanguosha::HandlingMethod method,
                                  const QList<int> &disabled_ids)
{
    return -1;
}

int ServerLogic::askForAG(Player *player, const QList<int> &card_ids, bool refusable, const QString &reason)
{
    return -1;
}

const Card *ServerLogic::askForPindian(PindianStruct &pindian)
{
    return nullptr;
}

QList<const Card *> ServerLogic::askForPindianRace(Player *from, Player *to, const QString &reason)
{
    return {};
}

QList<Player *> ServerLogic::askForPlayersChosen(Player *chooser, const QString &reason, const QList<Player *> targets, int max_num, int min_num, bool optional,
                                                 const QString &prompt)
{
    return {};
}

Player *ServerLogic::askForPlayerChosen(Player *chooser, const QString &reason, const QList<Player *> targets, bool optional, const QString &prompt)
{
    QList<Player *> p = askForPlayersChosen(chooser, reason, targets, 1, 1, optional, prompt);
    return p.isEmpty() ? nullptr : p.first();
}

TriggerDetail ServerLogic::askForTriggerOrder(Player *player, const QList<TriggerDetail> &sameTiming, bool cancelable)
{
    return TriggerDetail(nullptr);
}

void ServerLogic::cheat(Player *player, const QVariant &args)
{
}

bool ServerLogic::makeSurrender(Player *player)
{
    return false;
}

void ServerLogic::newPlayer(Player *player)
{
    (void)room;
}

void ServerLogic::marshalPlayerInfo(Player *player)
{
}
