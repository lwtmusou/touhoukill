#ifndef TOUHOUKILL_GAME_LOGIC_H_
#define TOUHOUKILL_GAME_LOGIC_H_

// BE WARE! THIS FILE IS USED IN BOTH SWIG AND C++.
// MAKE SURE THE GRAMMAR IS COMPATIBLE BETWEEN 2 LANGUAGES.

#ifndef SWIG

#include "protocol.h"
#include "qsgscore.h"
#include "structs.h"

#include <QString>
#include <QStringList>
#include <QVariant>

class Player;
class RoomObject;
struct DamageStruct;

class GameLogicPrivate;
#endif

/**
 * @brief The game logic interface.
 * 
 * All game events should be triggered by functions in this interface.
 */
class QSGS_CORE_EXPORT GameLogic
{
public:
    explicit GameLogic();
    ~GameLogic();

public:
    // --- the state model it modifies ---
    RoomObject *room();
    const RoomObject *room() const;
    void setRoom(RoomObject *room);

public:
    // --- logic manipulation only ---
    void enterDying(const DeathStruct &death);
    void killPlayer(const DeathStruct &death);
    void buryPlayer(Player *victim);
    void revivePlayer(Player *player, bool initialize = true);

    void gameOver(const QString &winner, bool isSurrender = false);

    // TODO: Move all these functions to Slash::onEffect (Also remove all the events specific to slash)
    // void slashEffect(const SlashEffectStruct &effect);
    // void slashResult(const SlashEffectStruct &effect, const Card *jink);

    void handleAcquireLoseSkills(Player *player, const QStringList &acquireSkills, const QStringList &loseSkills, bool acquire_only = false);

    void showPlayerHiddenSkill(Player *player, const QString &skill_name);

    void setPlayerHP(Player *player, int value);
    void setPlayerMaxHP(Player *player, int value);
    void setPlayerChained(Player *player, bool is_chained);
    void setPlayerRemoved(Player *player, bool is_removed);
    void setPlayerRoleShown(Player *player, bool is_role_shown);
    void setPlayerKingdom(Player *player, const QString &kingdom);
    void setPlayerGender(Player *player, QSanguosha::Gender gender);

    void playerGainMark(Player *player, const QString &mark_name, int count = 1);
    void playerLoseMark(Player *player, const QString &mark_name, int count = 1); // count == -1 means all mark

    bool useCard(const CardUseStruct &card_use, bool add_history = true);

    // TODO: Decouple the slash for these functions
    bool cardEffect(const CardEffectStruct &effect);

    void damage(const DamageStruct &data);
    void applyDamage(Player *victim, const DamageStruct &damage); // Change Player HP according to @param damage.

    void loseHp(Player *victim, int lose = 1);
    void loseMaxHp(Player *victim, int lose = 1);
    bool changeMaxHpForAwakenSkill(Player *player, int magnitude = -1);

    void recover(Player *player, const RecoverStruct &recover, bool set_emotion = false);

    void turnPlayerOver(Player *player);

    void letPlayerPlay(Player *target, const QList<QSanguosha::Phase> &phases = QList<QSanguosha::Phase>());
    void changePlayerPhase(Player *target, QSanguosha::Phase from, QSanguosha::Phase to);
    void skipPlayerPhase(Player *player, QSanguosha::Phase phase, bool is_cost = false, bool send_log = true);
    void insertPlayerPhases(Player *player, const QList<QSanguosha::Phase> &new_phases, int index = -1);
    void exchangePlayerPhases(Player *player, QSanguosha::Phase from, QSanguosha::Phase to);

    void givePlayerAnExtraTurn(Player *benefiter);

    void judge(JudgeStruct &judge_struct);
    void retrial(const Card *card, Player *player, JudgeStruct *judge, const QString &skill_name, bool exchange = false);
    void sendJudgeResult(const JudgeStruct *judge); // Send log

    // Move ServerPlayer's pindian to here.
    bool pindian(Player *source, Player *target, const QString &reason);

    QList<int> peekCards(int n = 1, bool bottom = false); // Cards still stay in the pile. shuffle the pile if the number left is not enough
    void shuffleDrawPile();
    // int drawCard(bool bottom = false);
    // QList<int> getNCards(int n, bool update_pile_number = true, bool bottom = false);
    // void returnToTopDrawPile(const QList<int> &cards);

    // Create the Frame for selecting cards. Since it is made for Amazing Grace, this is notified to all players
    void fillAG(const QList<int> &card_ids, const QList<int> &disabled_ids = {}, const QList<Player *> &viewers = {});
    // Trigger Moving Card
    void takeAG(Player *player, int card_id, bool move_cards = true, QSanguosha::Place fromPlace = QSanguosha::PlaceDrawPile);
    void clearAG(const QList<Player *> &viewers = {}); // disappear the selecting frame.

    // Only available for CardAsked
    void provide(const Card *card, Player *who = nullptr);

    void sendLog(const LogStruct &log);

    void showCard(Player *player, int card_id, Player *only_viewer = nullptr);
    void showAllCards(Player *player, Player *to = nullptr);

    // For FilterSkill
    // Fs: filter skills should be calculated in GameState?
    // bool notifyUpdateCard(Player *player, int cardId, const Card *newCard);
    // bool broadcastUpdateCard(const QList<Player *> &players, int cardId, const Card *newCard);
    // bool notifyResetCard(Player *player, int cardId);
    // bool broadcastResetCard(const QList<Player *> &players, int cardId);
    // void filterCards(Player *player, QList<const Card *> cards, bool refilter);

    void notifySkillInvoked(Player *player, const QString &skill_name);

    // Play Audio Effect
    void broadcastSkillInvoke(const QString &skillName);
    void broadcastSkillInvoke(const QString &skillName, const QString &category);
    void broadcastSkillInvoke(const QString &skillName, int type);
    void broadcastSkillInvoke(const QString &skillName, bool isMale, int type);

    // Let player play audio effect?
    void broadcastSkillInvoke(const Player *player, const Card *card);
    void broadcastSkillInvoke(const Player *player, const QString *card_name);

    // Animation Helpers
    void doIndicateAnimation(const Player *from, const Player *to);
    void doLightbox(const QString &lightboxName, int duration = 4000);
    void doHuashenAnimation(const Player *player, const QString &skill_name);
    void doNullificationAnimation(const Player *from, const Player *to);
    void doFireAnimation(const Player *player);
    void doLightningAnimation(const Player *player);
    void doBattleArrayAnimation(Player *player, Player *target = nullptr);
    void setEmotion(Player *target, const QString &emotion);

    void doAnimation(QSanProtocol::AnimateType type, const QString &arg1 = QString(), const QString &arg2 = QString(), QList<Player *> players = QList<Player *>());

    // TODO: Move this function to Mode
    // void preparePlayers();

    void changeHero(Player *player, const QString &new_general, bool full_state, bool invoke_start = true, bool isSecondaryHero = false, bool sendLog = true);
    void transformGeneral(Player *player, QString general_name, int head);

    void setPlayerSkillInvalidity(Player *player, const QString &skill_name, bool invalidity, bool trigger_event = true);

    void reverseFor3v3(const Card *card, Player *player, QList<Player *> &list); // askForOrder
    // For 3v3 choose general
    void addToPlayerSelectedGeneral(Player *target, const QString &general_name);
    void clearPlayerSelectedGeneral(Player *target);

    void drawCards(Player *player, int n, const QString &reason = QString());
    void drawCards(QList<Player *> players, int n, const QString &reason = QString());
    void drawCards(QList<Player *> players, QList<int> n_list, const QString &reason = QString());

    void obtainCard(Player *target, const Card *card, bool unhide = true);
    void obtainCard(Player *target, int card_id, bool unhide = true);
    // void obtainCard(Player *target, const Card *card, const CardMoveReason &reason, bool unhide = true);

    void throwCard(int card_id, Player *who, Player *thrower = nullptr, bool notifyLog = true);
    void throwCard(const Card *card, Player *who, Player *thrower = nullptr, bool notifyLog = true);
    // void throwCard(const Card *card, const CardMoveReason &reason, Player *who, Player *thrower = nullptr, bool notifyLog = true);

    void throwPlayerAllEquips(const Player *victim);
    void throwPlayerAllHandCards(const Player *victim);
    void throwPlayerDelayTrickRegion(const Player *victim);
    void throwPlayerAllCards(const Player *victim);

    // TODO: remove the design of partitially open private pile since it sucks
    // Sanguosha never needs this kind of private pile to run
    // Record it in each client if needed
    void addToPlayerPile(Player *owner, const QString &pile_name, const Card *card, bool open = true);
    void addToPlayerPile(Player *owner, const QString &pile_name, int card_id, bool open = true);
    void addToPlayerPile(Player *owner, const QString &pile_name, const IdSet &card_id, bool open = true);
    // void addToPlayerPile(Player *owner, const QString &pile_name, const IdSet &card_id, bool open, CardMoveReason reason);

    void addToPlayerShownHandCards(Player *target, const IdSet &card_ids);
    void removePlayerShownHandCards(Player *target, const IdSet &card_ids);

    void addPlayerBrokenEquip(Player *target, const IdSet &ids);
    void removePlayerBrokenEquip(Player *target, const IdSet &ids);

    void addPlayerHiddenGeneral(Player *target, const QStringList &generals);
    void removePlayerHiddenGeneral(Player *target, const QStringList);

    // Note: Cards will move to Discard Pile
    void clearPlayerPrivatePile(const Player *target, const QString &pile_name);
    void clearPlayerAllPrivatePiles(const Player *target);

    void doJileiShow(Player *player, const IdSet &jilei_ids);
    void forcePlayerDiscard(const Player *target, int discard_num, int include_equip, bool is_discard = true);

    void moveCardTo(const Card *card, Player *dstPlayer, QSanguosha::Place dstPlace, bool forceVisible = false);
    // void moveCardTo(const Card *card, Player *dstPlayer, QSanguosha::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible = false);
    // void moveCardTo(const Card *card, Player *srcPlayer, Player *dstPlayer, QSanguosha::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible = false);
    // void moveCardTo(const Card *card, Player *srcPlayer, Player *dstPlayer, QSanguosha::Place dstPlace, const QString &pileName, const CardMoveReason &reason, bool forceMoveVisible = false);

    // only _fillMoveInfo from legacy room is absolutely useful
    // since the CardsMoveStruct here is already broken down and no need to merge it anywhere to something like CardsMoveOneTimeStruct...
    void moveCardsAtomic(CardsMoveStruct cards_move, bool forceVisible = false);

    // use move.toPile = QStringLiteral("bottom") instead
    // void moveCardsToEndOfDrawpile(QList<int> card_ids, bool forceVisible = false);

    // The DRIVER
    bool trigger(QSanguosha::TriggerEvent e, QVariant &data);

public:
    // --- interactive methods ---
    // Legacy implementation of following functions are doing the actual manipulation after successfully asked
    //  activate
    //  askForGuanxing
    //  askForUseCard
    //  askForNullification
    //  askForSinglePeach
    //  askForCard (splitted to askForJink, askForResponseCard, askForDiscard, askForExchange)
    //  askForDiscard
    //  askForRende / askForYiji (merged to askForCardGive)
    //  askForLuckCard
    //  ServerPlayer::pindian (askFroPindian here)
    // I'd like only do the interatcive things here instead of combination of interactive and manipulation
    // So....
    // We need out parameters for receiving our results and do the actual manipulation afterwards

    void activate(CardUseStruct &use, Player *player);

    void askForGuanxing(QList<int> &upCards, QList<int> &downCards, Player *player, const QList<int> &cards, QSanguosha::GuanxingType guanxingType = QSanguosha::GuanxingBothSides,
                        QString skillName = QString());

    // askForUseCard / askForUseSlashTo (QSanguosha::MethodUse)
    // by default no recasting is enabled for askForUseCard. (To enable recasting add QSanguosha::MethodRecast to methods)
    void askForUseCard(CardUseStruct &use, Player *player, const QString &reason, const QString &pattern, bool optional = true, const QString &prompt = QString(),
                       bool addHistory = true, const QList<QSanguosha::HandlingMethod> &methods = {QSanguosha::MethodUse}, bool enableConversion = true);
    // wrapper to askForUseCard. use SkillName instead of reason / pattern and add pattern index.
    // use MethodDiscard and MethodNone for default handling method since most SkillCards use them
    void askForUseCard(CardUseStruct &use, Player *player, const QString &skillName, int patternIndex, bool optional = true, const QString &prompt = QString(),
                       const QList<QSanguosha::HandlingMethod> &methods = {QSanguosha::MethodDiscard, QSanguosha::MethodNone});
    // wrapper to askForUseCard. To support skill Luanwu from Jiaxu in Sanguosha victim should be list
    void askForUseSlashTo(CardUseStruct &use, Player *slasher, const QString &reason, Player *victim, bool disableExtra = false, bool optional = true,
                          const QString &prompt = QString(), bool addHistory = false, bool enableConversion = true);
    void askForUseSlashTo(CardUseStruct &use, Player *slasher, const QString &reason, const QList<Player *> &victims, bool disableExtra = false, bool optional = true,
                          const QString &prompt = QString(), bool addHistory = false, bool enableConversion = true);

    // variants of askForUseCard
    void askForJink(CardUseStruct &use, Player *player, const CardEffectStruct &slashUse, bool enableConversion = true);
    void askForSinglePeach(CardUseStruct &use, Player *toAsk, const DeathStruct &dying, bool enableConversion = true);
    // Is there any skill or something which can prevent asking for nullification?
    void askForNullification(CardUseStruct &use, const CardEffectStruct &trickEffect, bool enableConversion = true);
    void askForNullification(CardUseStruct &use, const QList<Player *> &toAsk, const CardEffectStruct &trickEffect, bool enableConversion = true);

    // QSanguosha::MethodResponse
    void askForResponseCard(CardResponseStruct &response, Player *player, const QString &reason, const QString &pattern, bool optional = true, const QString &prompt = QString(),
                            bool isRetrial = false, bool isProvision = false, const QList<QSanguosha::HandlingMethod> &methods = {QSanguosha::MethodResponse},
                            bool enableConversion = true);
    // Is it of any actual use?
    void askForResponseCard(CardResponseStruct &response, Player *player, const QString &skillName, int patternIndex, bool optional = true, const QString &prompt = QString(),
                            bool isRetrial = false, bool isProvision = false, const QList<QSanguosha::HandlingMethod> &methods = {QSanguosha::MethodResponse});

    // QSanguosha::MethodDiscard
    void askForDiscard(IdSet &discardedIds, Player *target, const QString &reason, int max_num, int min_num, bool include_equip = false, bool optional = false,
                       const QString &prompt = QString());
    void askForDiscard(IdSet &discardedIds, Player *target, const QString &reason, const QString &pattern, bool optional = false, const QString &prompt = QString());
    void askForDiscard(IdSet &discardedIds, Player *target, const QString &skillName, int patternIndex, bool optional = false, const QString &prompt = QString());

    // QSanguosha::MethodNone
    void askForExchange(IdSet &discardedIds, Player *target, const QString &reason, int max_num, int min_num, bool include_equip = false, bool optional = false,
                        const QString &prompt = QString());
    void askForExchange(IdSet &discardedIds, Player *target, const QString &reason, const QString &pattern, bool optional = false, const QString &prompt = QString());
    void askForExchange(IdSet &discardedIds, Player *target, const QString &skillName, int patternIndex, bool optional = false, const QString &prompt = QString());

    // replacement of askForRende / askForYiji
    // I don't expect the LUA interface be complicated, but currently the first parameter seems impossible to operate in LUA
    // maybe separate the QHash to 2 QLists?
    void askForCardGive(QHash<Player *, int> &give, Player *giver, const QString &reason, const IdSet &cardIds, bool optional = true, const QString &prompt = QString(),
                        const QList<Player *> players = {});

    // a.k.a. Beriberi Card (Jiao qi ka in Chinese) Temporarily put it away until I found a solution for this
    // void askForLuckCard(const QList<Player *> &toAsk = {});

    QSanguosha::Suit askForSuit(Player *player, const QString &reason);

    QString askForKingdom(Player *player, const QString &reason = QString());

    bool askForSkillInvoke(Player *player, const QString &skill_name, const QString &prompt = QString());

    QString askForChoice(Player *player, const QString &reason, const QStringList &choices, const QString &defaultChoice = QString());
    // wrapper to askForChoice
    QString askForGeneral(Player *player, const QStringList &generals, const QString &defaultChoice = QString());

    // use askForNullification instead
    // bool isCanceled(const CardEffectStruct &effect);

    int askForCardChosen(Player *player, Player *who, const QString &flags, const QString &reason, bool handcard_visible = false,
                         QSanguosha::HandlingMethod method = QSanguosha::MethodNone, const QList<int> &disabled_ids = QList<int>());

    // Let player select from the frame. (todo: make card_ids argument optional since we must use fillAG)
    int askForAG(Player *player, const QList<int> &card_ids, bool refusable, const QString &reason);

    // use askForUseCard / askForResponseCard / askForExchange(pattern) / askForDiscard(pattern) instead
    // const Card *askForCard(Player *player, const QString &pattern, const QString &prompt, const QVariant &data, const QString &skill_name, int notice_index = -1);
    // const Card *askForCard(Player *player, const QString &pattern, const QString &prompt, const QVariant &data = QVariant(),
    //                               QSanguosha::HandlingMethod method = QSanguosha::MethodDiscard, Player *to = nullptr, bool isRetrial = false,
    //                               const QString &skill_name = QString(), bool isProvision = false);

    // exclusive to God-Patchouli
    // void doExtraAmazingGrace(Player *from, Player *target, int times);

    // use askForExchange instead
    // const Card *askForCardShow(Player *player, Player *requestor, const QString &reason);

    const Card *askForPindian(PindianStruct &pindian);
    QList<const Card *> askForPindianRace(Player *from, Player *to, const QString &reason);

    QList<Player *> askForPlayersChosen(Player *chooser, const QString &reason, const QList<Player *> targets, int max_num, int min_num = 1, bool optional = true,
                                        const QString &prompt = QString());
    // wrapper to askForPlayersChosen
    Player *askForPlayerChosen(Player *chooser, const QString &reason, const QList<Player *> targets, bool optional = true, const QString &prompt = QString());

    TriggerDetail askForTriggerOrder(Player *player, const QList<TriggerDetail> &sameTiming, bool cancelable);

public:
    // --- cheat related ---
    void cheat(Player *player, const QVariant &args);
    bool makeSurrender(Player *player);

private:
    Q_DISABLE_COPY_MOVE(GameLogic)
    GameLogicPrivate *d;
};

#endif
