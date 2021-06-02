#ifndef _GAME_LOGIC_H_
#define _GAME_LOGIC_H_

#include <QString>

#include "structs.h"

class Player;
class RoomObject;
struct DamageStruct;
struct LogMessage;

namespace RefactorProposal {

class GameState;

/**
 * @interface The game logic interface.
 * 
 * All game events should be triggered by functions in this interface. 
 * The game state is modified by functions in this interface.
 */
class GameLogic
{
public:
    virtual ~GameLogic() = 0;

    enum GuanxingType
    {
        GuanxingUpOnly = 1,
        GuanxingBothSides = 0,
        GuanxingDownOnly = -1
    };

    virtual void registerGameState(const GameState *state) = 0;

    virtual void enterDying(Player *player, ::DamageStruct *reason) = 0;
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
    virtual void setPlayerGender(Player *player, General::Gender gender) = 0;

    virtual void playerGainMark(Player *player, const QString &mark_name, int count = 1) = 0;
    virtual void playerLoseMark(Player *player, const QString &mark_name, int count = 1) = 0; // count == -1 means all mark

    virtual bool useCard(const CardUseStruct &card_use, bool add_history = true) = 0;

    // TODO: Decouple the slash for these functions
    virtual bool cardEffect(const Card *card, Player *from, Player *to, bool multiple = false) = 0;
    virtual bool cardEffect(const CardEffectStruct &effect) = 0;

    virtual void damage(const DamageStruct &data) = 0;
    virtual void applyDamage(Player *victim, const DamageStruct &damage) = 0; // Change Player HP according to @param damage.
    virtual void sendDamageLog(const DamageStruct &data) = 0;

    virtual void loseHp(Player *victim, int lose = 1) = 0;
    virtual void loseMaxHp(Player *victim, int lose = 1) = 0;
    virtual bool changeMaxHpForAwakenSkill(Player *player, int magnitude = -1) = 0;

    virtual void recover(Player *player, const RecoverStruct &recover, bool set_emotion = false) = 0;

    virtual void turnPlayerOver(Player *player) = 0;

    virtual void letPlayerPlay(Player *target, QList<Player::Phase> phases = QList<Player::Phase>()) = 0;
    virtual void changePlayerPhase(Player *target, Player::Phase from, Player::Phase to) = 0;
    virtual void skipPlayerPhase(Player *player, Player::Phase phase, bool is_cost = false, bool send_log = true) = 0;
    virtual void insertPlayerPhases(Player *player, QList<Player::Phase> new_phases, int index = -1) = 0;
    virtual void exchangePlayerPhases(Player *player, Player::Phase from, Player::Phase to) = 0;

    virtual void givePlayerAnExtraTurn(Player *benefiter) = 0;

    // TODO: Remove it when fixing the Jink.
    virtual bool isJinkEffected(const SlashEffectStruct &effect, const Card *jink) = 0;

    virtual void judge(JudgeStruct &judge_struct) = 0;
    virtual void retrial(const Card *card, Player *player, JudgeStruct *judge, const QString &skill_name, bool exchange = false) = 0;
    virtual void sendJudgeResult(const JudgeStruct *judge) = 0; // Send log

    // FIXME(xusine): I move ServerPlayer's pindian to here. Please help check if this makes sense.
    virtual bool pindian(Player *source, Player *target, const QString &reason, const Card *card1 = nullptr) = 0;

    virtual QList<int> peekCards(int n = 1, bool bottom = false) = 0; // Cards still stay in the pile. shuffle the pile if the number left is not enough
    virtual void shuffleDrawPile() = 0;
    // virtual int drawCard(bool bottom = false) = 0;
    // virtual QList<int> getNCards(int n, bool update_pile_number = true, bool bottom = false) = 0;
    // virtual void returnToTopDrawPile(const QList<int> &cards) = 0;

    virtual void askForGuanxing(Player *player, const QList<int> &cards, GuanxingType guanxing_type = GuanxingBothSides, QString skillName = "") = 0;

    // Create the Frame for selecting cards
    virtual void fillAG(const QList<int> &card_ids, Player *who = nullptr, const QList<int> &disabled_ids = QList<int>(), const QList<int> &shownHandcard_ids = QList<int>()) = 0;
    // Let player select from the frame.
    virtual int askForAG(Player *player, const QList<int> &card_ids, bool refusable, const QString &reason) = 0;
    // Trigger Moving Card
    virtual void takeAG(Player *player, int card_id, bool move_cards = true, QList<Player *> to_notify = QList<Player *>(), Player::Place fromPlace = Player::DrawPile) = 0;
    virtual void clearAG(Player *player = nullptr) = 0; // disappear the selecting frame.

    // Only available for CardAsked
    virtual void provide(const Card *card, Player *who = nullptr) = 0;

    virtual void sendLog(const LogMessage &log) = 0;
    virtual void sendLog(const QString &logtype, Player *logfrom, const QString &logarg = QString(), const QList<Player *> &logto = QList<Player *>(),
                         const QString &logarg2 = QString())
        = 0;

    virtual void showCard(Player *player, int card_id, Player *only_viewer = nullptr) = 0;
    virtual void showAllCards(Player *player, Player *to = nullptr) = 0;

    // For FilterSkill
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
    virtual void doLightbox(const QString &lightboxName, int duration = 2000) = 0;
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
    virtual void obtainCard(Player *target, const Card *card, const CardMoveReason &reason, bool unhide = true) = 0;

    virtual void throwCard(int card_id, Player *who, Player *thrower = nullptr, bool notifyLog = true) = 0;
    virtual void throwCard(const Card *card, Player *who, Player *thrower = nullptr, bool notifyLog = true) = 0;
    virtual void throwCard(const Card *card, const CardMoveReason &reason, Player *who, Player *thrower = nullptr, bool notifyLog = true) = 0;

    virtual void throwPlayerAllEquips(const Player *victim) = 0;
    virtual void throwPlayerAllHandCards(const Player *victim) = 0;
    virtual void throwPlayerDelayTrickRegion(const Player *victim) = 0;
    virtual void throwPlayerAllCards(const Player *victim) = 0;

    virtual void addToPlayerPile(Player *owner, const QString &pile_name, const Card *card, bool open = true, QList<Player *> open_target = QList<Player *>()) = 0;
    virtual void addToPlayerPile(Player *owner, const QString &pile_name, int card_id, bool open = true, QList<Player *> open_target = QList<Player *>()) = 0;
    virtual void addToPlayerPile(Player *owner, const QString &pile_name, const IDSet &card_id, bool open = true, QList<Player *> open_target = QList<Player *>()) = 0;
    virtual void addToPlayerPile(Player *owner, const QString &pile_name, const IDSet &card_id, bool open, CardMoveReason reason, QList<Player *> open_target = QList<Player *>())
        = 0;

    virtual void addToPlayerShowHandCards(Player *target, const IDSet &card_ids) = 0;
    virtual void addToPlayerShowHandCards(Player *target, const IDSet &card_ids, bool send_log = false, bool move_from_hand = false) = 0;

    virtual void addPlayerBrokenEquip(Player *target, const IDSet &ids) = 0;
    virtual void removePlayerBrokenEquip(Player *target, const IDSet &ids) = 0;

    virtual void addPlayerHiddenGeneral(Player *target, const QStringList &generals) = 0;
    virtual void removePlayerHiddenGeneral(Player *target, const QStringList) = 0;

    virtual void clearPlayerPrivatePile(const Player *target, const QString &pile_name) = 0;
    virtual void clearPlayerAllPrivatePiles(const Player *target) = 0;

    virtual void moveCardTo(const Card *card, Player *dstPlayer, Player::Place dstPlace, bool forceMoveVisible = false) = 0;
    virtual void moveCardTo(const Card *card, Player *dstPlayer, Player::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible = false) = 0;
    virtual void moveCardTo(const Card *card, Player *srcPlayer, Player *dstPlayer, Player::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible = false) = 0;
    virtual void moveCardTo(const Card *card, Player *srcPlayer, Player *dstPlayer, Player::Place dstPlace, const QString &pileName, const CardMoveReason &reason,
                            bool forceMoveVisible = false)
        = 0;

    virtual void moveCardsAtomic(QList<CardsMoveStruct> cards_move, bool forceMoveVisible) = 0;
    virtual void moveCardsAtomic(CardsMoveStruct cards_move, bool forceMoveVisible) = 0;
    virtual QList<CardsMoveStruct> _breakDownCardMoves(QList<CardsMoveStruct> &cards_moves) = 0;

    virtual void moveCardsToEndOfDrawpile(QList<int> card_ids, bool forceVisible = false) = 0;

    // interactive methods
    virtual bool activate(Player *player) = 0;
    // askForUseCard / askForUseSlashTo (Card::MethodUse)
    // TODO: askForUseCard supports the recast here. (Add additional parameter to this function, or change method -> QList<Card::HandlingMethod>)
    virtual const Card *askForUseCard(Player *player, const QString &pattern, const QString &prompt, int notice_index = -1, Card::HandlingMethod method = Card::MethodUse,
                                      bool addHistory = true, const QString &skill_name = QString())
        = 0;
    virtual const Card *askForUseSlashTo(Player *slasher, Player *victim, const QString &prompt, bool distance_limit = true, bool disable_extra = false, bool addHistory = false)
        = 0;
    virtual const Card *askForUseSlashTo(Player *slasher, QList<Player *> victims, const QString &prompt, bool distance_limit = true, bool disable_extra = false,
                                         bool addHistory = false)
        = 0;
    virtual const Card *askForUseJink(Player *player, ...) = 0;

    virtual void askForLuckCard() = 0;

    virtual Card::Suit askForSuit(Player *player, const QString &reason) = 0;

    virtual QString askForKingdom(Player *player) = 0;

    virtual bool askForSkillInvoke(Player *player, const QString &skill_name, const QVariant &data = QVariant(), const QString &prompt = QString()) = 0;

    virtual QString askForChoice(Player *player, const QString &skill_name, const QString &choices, const QVariant &data = QVariant()) = 0;

    // TODO: Add pattern to askForDiscard (Card::MethodDiscard)
    virtual bool askForDiscard(Player *target, const QString &reason, int discard_num, int min_num, bool optional = false, bool include_equip = false,
                               const QString &prompt = QString())
        = 0;
    virtual void doJileiShow(Player *player, const IDSet &jilei_ids) = 0;
    virtual void forcePlayerDiscard(const Player *target, int discard_num, int include_equip, bool is_discard = true) = 0;

    // TODO: Add pattern to askForExchange (Card::MethodNone)
    virtual const Card *askForExchange(Player *player, const QString &reason, int discard_num, int min_num, bool include_equip = false, const QString &prompt = QString(),
                                       bool optional = false)
        = 0;

    // TODO: Fix the process of nullification.
    virtual bool askForNullification(const Card *trick, Player *from, Player *to, bool positive) = 0;
    virtual bool isCanceled(const CardEffectStruct &effect) = 0;

    virtual int askForCardChosen(Player *player, Player *who, const QString &flags, const QString &reason, bool handcard_visible = false,
                                 Card::HandlingMethod method = Card::MethodNone, const QList<int> &disabled_ids = QList<int>())
        = 0;

    virtual const Card *askForCard(Player *player, const QString &pattern, const QString &prompt, const QVariant &data, const QString &skill_name, int notice_index = -1) = 0;
    virtual const Card *askForCard(Player *player, const QString &pattern, const QString &prompt, const QVariant &data = QVariant(),
                                   Card::HandlingMethod method = Card::MethodDiscard, Player *to = nullptr, bool isRetrial = false, const QString &skill_name = QString(),
                                   bool isProvision = false, int notice_index = -1)
        = 0;

    virtual const Card *askForResponse(Player *, ...) = 0;

    virtual void doExtraAmazingGrace(Player *from, Player *target, int times) = 0;

    virtual const Card *askForCardShow(Player *player, Player *requestor, const QString &reason) = 0;

    virtual int askForRende(Player *liubei, QList<int> &cards, const QString &skill_name = QString(), bool visible = false, bool optional = true, int max_num = -1,
                            QList<Player *> players = QList<Player *>(), CardMoveReason reason = CardMoveReason(), const QString &prompt = QString(), bool notify_skill = false)
        = 0;
    virtual bool askForYiji(Player *guojia, QList<int> &cards, const QString &skill_name = QString(), bool is_preview = false, bool visible = false, bool optional = true,
                            int max_num = -1, QList<Player *> players = QList<Player *>(), CardMoveReason reason = CardMoveReason(), const QString &prompt = QString(),
                            bool notify_skill = false)
        = 0;

    virtual const Card *askForPindian(PindianStruct &pindian) = 0;
    virtual QList<const Card *> askForPindianRace(Player *from, Player *to, const QString &reason) = 0;

    virtual Player *askForPlayerChosen(Player *player, const QList<Player *> &targets, const QString &reason, const QString &prompt = QString(), bool optional = false,
                                       bool notify_skill = false)
        = 0;

    virtual QString askForGeneral(Player *player, const QStringList &generals, QString default_choice = QString()) = 0;
    virtual QString askForGeneral(Player *player, const QString &generals, QString default_choice = QString()) = 0;

    virtual const Card *askForSinglePeach(Player *player, Player *dying) = 0; // Replace with askForUseCard

    // virtual QSharedPointer<SkillInvokeDetail> askForTriggerOrder(Player *player, const QList<QSharedPointer<SkillInvokeDetail> > &sameTiming, bool cancelable,
    //                                                                  const QVariant &data) = 0;

    virtual void updateCardsOnLose(const CardsMoveStruct &move) = 0;
    virtual void updateCardsOnGet(const CardsMoveStruct &move) = 0;

    virtual void cheat(Player *player, const QVariant &args) = 0;
    virtual bool makeSurrender(Player *player) = 0;
};

}

#endif
