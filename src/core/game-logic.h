#ifndef TOUHOUKILL_GAME_LOGIC_H_
#define TOUHOUKILL_GAME_LOGIC_H_

#include "protocol.h"
#include "qsgscore.h"
#include "structs.h"

#include <QString>
#include <QStringList>
#include <QVariant>

class Player;
class RoomObject;
struct DamageStruct;

/**
 * @brief The game logic interface.
 * 
 * All game events should be triggered by functions in this interface.
 */
class QSGS_CORE_EXPORT GameLogic
{
public:
    virtual ~GameLogic();

    enum GuanxingType
    {
        GuanxingUpOnly = 1,
        GuanxingBothSides = 0,
        GuanxingDownOnly = -1
    };

    enum GiveMovementType
    {
        MovementDisable = 0x0,
        MovementEnable = 0x1,

        OneTimeMovement = 0x0, // for Rende
        SplittedMovement = 0x2, // for Yiji

        NonPreviewGiveMovement = 0x0, // for Rende
        PreviewGiveMovement = 0x4, // for Yiji
    };

    virtual void enterDying(Player *player, DamageStruct *reason) = 0;
    virtual void buryPlayer(Player *victim) = 0;
    virtual void killPlayer(Player *victim, DamageStruct *reason = nullptr) = 0;
    virtual void revivePlayer(Player *player, bool initialize = true) = 0;

    virtual void gameOver(const QString &winner, bool isSurrender = false) = 0;

    // TODO: Move all these functions to Slash::onEffect (Also remove all the events specific to slash)
    // virtual void slashEffect(const SlashEffectStruct &effect) = 0;
    // virtual void slashResult(const SlashEffectStruct &effect, const Card *jink) = 0;

    /**
     * Skill manipulation and trigger events.
     */
    virtual void handleAcquireLoseSkills(Player *player, const QStringList &skill_names, bool acquire_only = false) = 0;
    virtual void handleAcquireLoseSkills(Player *player, const QString &skill_names, bool acquire_only = false) = 0;

    virtual void showPlayerHiddenSkill(Player *player, const QString &skill_name) = 0;

    virtual void setPlayerHP(Player *player, int value) = 0;
    virtual void setPlayerMaxHP(Player *player, int value) = 0;
    virtual void setPlayerChained(Player *player, bool is_chained) = 0;
    virtual void setPlayerRemoved(Player *player, bool is_removed) = 0;
    virtual void setPlayerRoleShown(Player *player, bool is_role_shown) = 0;
    virtual void setPlayerKingdom(Player *player, const QString &kingdom) = 0;
    virtual void setPlayerGender(Player *player, QSanguosha::Gender gender) = 0;

    virtual void playerGainMark(Player *player, const QString &mark_name, int count = 1) = 0;
    virtual void playerLoseMark(Player *player, const QString &mark_name, int count = 1) = 0; // count == -1 means all mark

    virtual bool useCard(const CardUseStruct &card_use, bool add_history = true) = 0;

    // TODO: Decouple the slash for these functions
    virtual bool cardEffect(const CardEffectStruct &effect) = 0;

    virtual void damage(const DamageStruct &data) = 0;
    virtual void applyDamage(Player *victim, const DamageStruct &damage) = 0; // Change Player HP according to @param damage.

    virtual void loseHp(Player *victim, int lose = 1) = 0;
    virtual void loseMaxHp(Player *victim, int lose = 1) = 0;
    virtual bool changeMaxHpForAwakenSkill(Player *player, int magnitude = -1) = 0;

    virtual void recover(Player *player, const RecoverStruct &recover, bool set_emotion = false) = 0;

    virtual void turnPlayerOver(Player *player) = 0;

    virtual void letPlayerPlay(Player *target, const QList<QSanguosha::Phase> &phases = QList<QSanguosha::Phase>()) = 0;
    virtual void changePlayerPhase(Player *target, QSanguosha::Phase from, QSanguosha::Phase to) = 0;
    virtual void skipPlayerPhase(Player *player, QSanguosha::Phase phase, bool is_cost = false, bool send_log = true) = 0;
    virtual void insertPlayerPhases(Player *player, const QList<QSanguosha::Phase> &new_phases, int index = -1) = 0;
    virtual void exchangePlayerPhases(Player *player, QSanguosha::Phase from, QSanguosha::Phase to) = 0;

    virtual void givePlayerAnExtraTurn(Player *benefiter) = 0;

    virtual void judge(JudgeStruct &judge_struct) = 0;
    virtual void retrial(const Card *card, Player *player, JudgeStruct *judge, const QString &skill_name, bool exchange = false) = 0;
    virtual void sendJudgeResult(const JudgeStruct *judge) = 0; // Send log

    // Move ServerPlayer's pindian to here.
    virtual bool pindian(Player *source, Player *target, const QString &reason) = 0;

    virtual QList<int> peekCards(int n = 1, bool bottom = false) = 0; // Cards still stay in the pile. shuffle the pile if the number left is not enough
    virtual void shuffleDrawPile() = 0;
    // virtual int drawCard(bool bottom = false) = 0;
    // virtual QList<int> getNCards(int n, bool update_pile_number = true, bool bottom = false) = 0;
    // virtual void returnToTopDrawPile(const QList<int> &cards) = 0;

    virtual void askForGuanxing(Player *player, const QList<int> &cards, GuanxingType guanxing_type = GuanxingBothSides, QString skillName = QString()) = 0;

    // Create the Frame for selecting cards. Since it is made for Amazing Grace, this is notified to all players
    virtual void fillAG(const QList<int> &card_ids, const QList<int> &disabled_ids = {}, const QList<Player *> &viewers = {}) = 0;
    // Let player select from the frame. (todo: make card_ids argument optional since we must use fillAG)
    virtual int askForAG(Player *player, const QList<int> &card_ids, bool refusable, const QString &reason) = 0;
    // Trigger Moving Card
    virtual void takeAG(Player *player, int card_id, bool move_cards = true, QSanguosha::Place fromPlace = QSanguosha::PlaceDrawPile) = 0;
    virtual void clearAG(const QList<Player *> &viewers = {}) = 0; // disappear the selecting frame.

    // Only available for CardAsked
    virtual void provide(const Card *card, Player *who = nullptr) = 0;

    virtual void sendLog(const LogStruct &log) = 0;

    // weird argument sequence...... well, remove it by adding constructor to LogStruct
    // virtual void sendLog(const QString &logtype, Player *logfrom, const QString &logarg = QString(), const QList<Player *> &logto = QList<Player *>(),
    //                      const QString &logarg2 = QString())
    //    = 0;

    virtual void showCard(Player *player, int card_id, Player *only_viewer = nullptr) = 0;
    virtual void showAllCards(Player *player, Player *to = nullptr) = 0;

    // For FilterSkill
    // Fs: filter skills should be calculated in both client and server. Current implementation is on server only
    // virtual bool notifyUpdateCard(Player *player, int cardId, const Card *newCard) = 0;
    // virtual bool broadcastUpdateCard(const QList<Player *> &players, int cardId, const Card *newCard) = 0;
    // virtual bool notifyResetCard(Player *player, int cardId) = 0;
    // virtual bool broadcastResetCard(const QList<Player *> &players, int cardId) = 0;
    // virtual void filterCards(Player *player, QList<const Card *> cards, bool refilter) = 0;

    virtual void notifySkillInvoked(Player *player, const QString &skill_name) = 0;

    // Play Audio Effect
    virtual void broadcastSkillInvoke(const QString &skillName) = 0;
    virtual void broadcastSkillInvoke(const QString &skillName, const QString &category) = 0;
    virtual void broadcastSkillInvoke(const QString &skillName, int type) = 0;
    virtual void broadcastSkillInvoke(const QString &skillName, bool isMale, int type) = 0;

    // Let player play audio effect?
    virtual void broadcastSkillInvoke(const Player *player, const Card *card) = 0;
    virtual void broadcastSkillInvoke(const Player *player, const QString *card_name) = 0;

    // Animation Helpers
    virtual void doIndicateAnimation(const Player *from, const Player *to) = 0;
    virtual void doLightbox(const QString &lightboxName, int duration = 4000) = 0;
    virtual void doHuashenAnimation(const Player *player, const QString &skill_name) = 0;
    virtual void doNullificationAnimation(const Player *from, const Player *to) = 0;
    virtual void doFireAnimation(const Player *player) = 0;
    virtual void doLightningAnimation(const Player *player) = 0;
    virtual void doBattleArrayAnimation(Player *player, Player *target = nullptr) = 0;
    virtual void setEmotion(Player *target, const QString &emotion) = 0;

    virtual void doAnimation(QSanProtocol::AnimateType type, const QString &arg1 = QString(), const QString &arg2 = QString(), QList<Player *> players = QList<Player *>()) = 0;

    // TODO: Move this function to Mode
    // virtual void preparePlayers() = 0;

    virtual void changeHero(Player *player, const QString &new_general, bool full_state, bool invoke_start = true, bool isSecondaryHero = false, bool sendLog = true) = 0;
    virtual void transformGeneral(Player *player, QString general_name, int head) = 0;

    virtual void setPlayerSkillInvalidity(Player *player, const QString &skill_name, bool invalidity, bool trigger_event = true) = 0;

    virtual void reverseFor3v3(const Card *card, Player *player, QList<Player *> &list) = 0; // askForOrder
    // For 3v3 choose general
    virtual void addToPlayerSelectedGeneral(Player *target, const QString &general_name) = 0;
    virtual void clearPlayerSelectedGeneral(Player *target) = 0;

    virtual void drawCards(Player *player, int n, const QString &reason = QString()) = 0;
    virtual void drawCards(QList<Player *> players, int n, const QString &reason = QString()) = 0;
    virtual void drawCards(QList<Player *> players, QList<int> n_list, const QString &reason = QString()) = 0;

    virtual void obtainCard(Player *target, const Card *card, bool unhide = true) = 0;
    virtual void obtainCard(Player *target, int card_id, bool unhide = true) = 0;
    // virtual void obtainCard(Player *target, const Card *card, const CardMoveReason &reason, bool unhide = true) = 0;

    virtual void throwCard(int card_id, Player *who, Player *thrower = nullptr, bool notifyLog = true) = 0;
    virtual void throwCard(const Card *card, Player *who, Player *thrower = nullptr, bool notifyLog = true) = 0;
    // virtual void throwCard(const Card *card, const CardMoveReason &reason, Player *who, Player *thrower = nullptr, bool notifyLog = true) = 0;

    virtual void throwPlayerAllEquips(const Player *victim) = 0;
    virtual void throwPlayerAllHandCards(const Player *victim) = 0;
    virtual void throwPlayerDelayTrickRegion(const Player *victim) = 0;
    virtual void throwPlayerAllCards(const Player *victim) = 0;

    // TODO: remove the design of partitially open private pile since it sucks
    // Sanguosha never needs this kind of private pile to run
    // Record it in each client if needed
    virtual void addToPlayerPile(Player *owner, const QString &pile_name, const Card *card, bool open = true) = 0;
    virtual void addToPlayerPile(Player *owner, const QString &pile_name, int card_id, bool open = true) = 0;
    virtual void addToPlayerPile(Player *owner, const QString &pile_name, const IdSet &card_id, bool open = true) = 0;
    // virtual void addToPlayerPile(Player *owner, const QString &pile_name, const IdSet &card_id, bool open, CardMoveReason reason) = 0;

    virtual void addToPlayerShownHandCards(Player *target, const IdSet &card_ids) = 0;
    virtual void removePlayerShownHandCards(Player *target, const IdSet &card_ids) = 0;

    virtual void addPlayerBrokenEquip(Player *target, const IdSet &ids) = 0;
    virtual void removePlayerBrokenEquip(Player *target, const IdSet &ids) = 0;

    virtual void addPlayerHiddenGeneral(Player *target, const QStringList &generals) = 0;
    virtual void removePlayerHiddenGeneral(Player *target, const QStringList) = 0;

    // Note: Cards will move to Discard Pile
    virtual void clearPlayerPrivatePile(const Player *target, const QString &pile_name) = 0;
    virtual void clearPlayerAllPrivatePiles(const Player *target) = 0;

    virtual void moveCardTo(const Card *card, Player *dstPlayer, QSanguosha::Place dstPlace, bool forceVisible = false) = 0;
    // virtual void moveCardTo(const Card *card, Player *dstPlayer, QSanguosha::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible = false) = 0;
    // virtual void moveCardTo(const Card *card, Player *srcPlayer, Player *dstPlayer, QSanguosha::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible = false) = 0;
    // virtual void moveCardTo(const Card *card, Player *srcPlayer, Player *dstPlayer, QSanguosha::Place dstPlace, const QString &pileName, const CardMoveReason &reason, bool forceMoveVisible = false) = 0;

    // only _fillMoveInfo from legacy room is absolutely useful
    // since the CardsMoveStruct here is already broken down and no need to merge it anywhere to something like CardsMoveOneTimeStruct...
    virtual void moveCardsAtomic(CardsMoveStruct cards_move, bool forceVisible = false) = 0;

    // use move.toPile = QStringLiteral("bottom") instead
    // virtual void moveCardsToEndOfDrawpile(QList<int> card_ids, bool forceVisible = false) = 0;

    // interactive methods
    virtual bool activate(Player *player) = 0;
    // askForUseCard / askForUseSlashTo (QSanguosha::MethodUse)
    // by default no recasting is enabled for askForUseCard. (To enable recasting add QSanguosha::MethodRecast to methods)
    virtual const Card *askForUseCard(Player *player, const QString &reason, const QString &pattern, bool optional = true, const QString &prompt = QString(),
                                      bool addHistory = true, const QList<QSanguosha::HandlingMethod> &methods = {QSanguosha::MethodUse}, bool enableConversion = true)
        = 0;
    // wrapper to askForUseCard. use SkillName instead of reason / pattern and add pattern index.
    // use MethodDiscard and MethodNone for default handling method since most SkillCards use them
    virtual const Card *askForUseCard(Player *player, const QString &skillName, int patternIndex = 0, bool optional = true, const QString &prompt = QString(),
                                      const QList<QSanguosha::HandlingMethod> &methods = {QSanguosha::MethodDiscard, QSanguosha::MethodNone})
        = 0;
    // wrapper to askForUseCard. To support skill Luanwu from Jiaxu in Sanguosha victim should be list
    virtual const Card *askForUseSlashTo(Player *slasher, const QString &reason, Player *victim, bool disableExtra = false, bool optional = true, const QString &prompt = QString(),
                                         bool addHistory = false, bool enableConversion = true)
        = 0;
    virtual const Card *askForUseSlashTo(Player *slasher, const QString &reason, const QList<Player *> &victims, bool disableExtra = false, bool optional = true,
                                         const QString &prompt = QString(), bool addHistory = false, bool enableConversion = true)
        = 0;

    // variants of askForUseCard
    virtual const Card *askForJink(Player *player, const CardEffectStruct &slashUse, bool enableConversion = true) = 0;
    virtual const Card *askForSinglePeach(Player *toAsk, const DeathStruct &dying, bool enableConversion = true) = 0;
    // Is there any skill or something which can prevent asking for nullification?
    virtual bool askForNullification(const CardEffectStruct &trickEffect, bool enableConversion = true) = 0;
    virtual bool askForNullification(const QList<Player *> &toAsk, const CardEffectStruct &trickEffect, bool enableConversion = true) = 0;

    // a.k.a. Beriberi Card (Jiao qi ka in Chinese)
    virtual void askForLuckCard(const QList<Player *> &toAsk = {}) = 0;

    virtual QSanguosha::Suit askForSuit(Player *player, const QString &reason) = 0;

    virtual QString askForKingdom(Player *player, const QString &reason = QString()) = 0;

    virtual bool askForSkillInvoke(Player *player, const QString &skill_name, const QString &prompt = QString()) = 0;

    virtual QString askForChoice(Player *player, const QString &reason, const QStringList &choices, const QString &defaultChoice = QString()) = 0;
    // wrapper to askForChoice
    virtual QString askForGeneral(Player *player, const QStringList &generals, const QString &defaultChoice = QString()) = 0;

    // QSanguosha::MethodDiscard
    virtual bool askForDiscard(Player *target, const QString &reason, int max_num, int min_num = 1, bool include_equip = false, bool optional = false,
                               const QString &prompt = QString())
        = 0;
    virtual bool askForDiscard(Player *target, const QString &reason, const QString &pattern, bool optional = false, const QString &prompt = QString()) = 0;
    virtual bool askForDiscard(Player *target, const QString &skillName, int patternIndex = 0, bool optional = false, const QString &prompt = QString()) = 0;

    virtual void doJileiShow(Player *player, const IdSet &jilei_ids) = 0;
    virtual void forcePlayerDiscard(const Player *target, int discard_num, int include_equip, bool is_discard = true) = 0;

    // QSanguosha::MethodNone
    virtual bool askForExchange(Player *target, const QString &reason, int max_num, int min_num = 1, bool include_equip = false, bool optional = false,
                                const QString &prompt = QString())
        = 0;
    virtual bool askForExchange(Player *target, const QString &reason, const QString &pattern, bool optional = false, const QString &prompt = QString()) = 0;
    virtual bool askForExchangeSkillCard(Player *target, const QString &skillName, int patternIndex = 0, bool optional = false, const QString &prompt = QString()) = 0;

    // use askForNullification instead
    // virtual bool isCanceled(const CardEffectStruct &effect) = 0;

    virtual int askForCardChosen(Player *player, Player *who, const QString &flags, const QString &reason, bool handcard_visible = false,
                                 QSanguosha::HandlingMethod method = QSanguosha::MethodNone, const QList<int> &disabled_ids = QList<int>())
        = 0;

    // use askForUseCard / askForResponseCard / askForExchange(pattern) / askForDiscard(pattern) instead
    // virtual const Card *askForCard(Player *player, const QString &pattern, const QString &prompt, const QVariant &data, const QString &skill_name, int notice_index = -1) = 0;
    // virtual const Card *askForCard(Player *player, const QString &pattern, const QString &prompt, const QVariant &data = QVariant(),
    //                                QSanguosha::HandlingMethod method = QSanguosha::MethodDiscard, Player *to = nullptr, bool isRetrial = false,
    //                                const QString &skill_name = QString(), bool isProvision = false)
    //     = 0;

    virtual const Card *askForResponseCard(Player *player, const QString &reason, const QString &pattern, bool optional = true, const QString &prompt = QString(),
                                           bool isRetrial = false, bool isProvision = false, const QList<QSanguosha::HandlingMethod> &methods = {QSanguosha::MethodResponse},
                                           bool enableConversion = true)
        = 0;
    // Is it of any actual use?
    virtual const Card *askForResponseCard(Player *player, const QString &skillName, int patternIndex = 0, bool optional = true, const QString &prompt = QString(),
                                           bool isRetrial = false, bool isProvision = false, const QList<QSanguosha::HandlingMethod> &methods = {QSanguosha::MethodResponse})
        = 0;

    // exclusive to God-Patchouli
    // virtual void doExtraAmazingGrace(Player *from, Player *target, int times) = 0;

    // use askForExchange instead
    // virtual const Card *askForCardShow(Player *player, Player *requestor, const QString &reason) = 0;

    // replacement of askForRende / askForYiji
    virtual int askForCardGive(Player *giver, const QString &reason, const IdSet &cardIds, bool optional = true, const QString &prompt = QString(),
                               GiveMovementType movement = MovementEnable, bool movementVisible = false, const QList<Player *> players = {})
        = 0;

    virtual const Card *askForPindian(PindianStruct &pindian) = 0;
    virtual QList<const Card *> askForPindianRace(Player *from, Player *to, const QString &reason) = 0;

    virtual QList<Player *> askForPlayersChosen(Player *chooser, const QString &reason, const QList<Player *> targets, int max_num, int min_num = 1, bool optional = true,
                                                const QString &prompt = QString())
        = 0;
    // wrapper to askForPlayersChosen
    virtual Player *askForPlayerChosen(Player *chooser, const QString &reason, const QList<Player *> targets, bool optional = true, const QString &prompt = QString()) = 0;

    virtual TriggerDetail askForTriggerOrder(Player *player, const QList<TriggerDetail> &sameTiming, bool cancelable) = 0;

    virtual void cheat(Player *player, const QVariant &args) = 0;
    virtual bool makeSurrender(Player *player) = 0;
};

#endif
