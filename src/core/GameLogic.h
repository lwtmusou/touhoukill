#ifndef _GAME_LOGIC_H_
#define _GAME_LOGIC_H_

#include <QString>

#include "structs.h"

class Player;
class RoomObject;
struct DamageStruct;
struct LogMessage;

#define QSGS_LOGIC virtual

namespace RefactorProposal {

/**
 * @interface The game logic interface.
 * 
 * All game events should be triggered by functions in this interface. 
 * The game state is modified by functions in this interface.
 */
class GameLogic
{
public:
    virtual ~GameLogic();

    enum GuanxingType
    {
        GuanxingUpOnly = 1,
        GuanxingBothSides = 0,
        GuanxingDownOnly = -1
    };

    QSGS_LOGIC void enterDying(Player *player, ::DamageStruct *reason) = 0;
    QSGS_LOGIC void killPlayer(Player *victim, DamageStruct *reason = nullptr) = 0;
    QSGS_LOGIC void revivePlayer(Player *player, bool initialize = true) = 0;

    QSGS_LOGIC void gameOver(const QString &winner, bool isSurrender = false) = 0;

    // TODO: Move all these functions to Slash::onEffect (Also the events for slash)
    // QSGS_LOGIC void slashEffect(const SlashEffectStruct &effect);
    // QSGS_LOGIC void slashResult(const SlashEffectStruct &effect, const Card *jink);

    /**
     * Add the skill to player. (No event)
     * 
     * TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
     */
    QSGS_LOGIC void attachSkillToPlayer(Player *player, const QString &skill_name, bool is_other_attach = false);

    /**
     * Remove the skill from player. (No event)
     * 
     * TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
     */
    QSGS_LOGIC void detachSkillFromPlayer(Player *player, const QString &skill_name, bool is_equip = false, bool acquire_only = false, bool sendlog = true, bool head = true);

    /**
     * Skill manipulation and trigger events.
     */
    QSGS_LOGIC void handleAcquireLoseSkills(Player *player, const QStringList &skill_names, bool acquire_only = false) = 0;
    QSGS_LOGIC void handleAcquireLoseSkills(Player *player, const QString &skill_names, bool acquire_only = false) = 0;

    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void setPlayerFlag(Player *player, const QString &flag);
    QSGS_LOGIC void setPlayerHP(Player *player, int value) = 0;
    QSGS_LOGIC void setPlayerMaxHP(Player *player, int value) = 0;
    QSGS_LOGIC void setPlayerChained(Player *player, bool is_chained) = 0;
    QSGS_LOGIC void setPlayerRemoved(Player *player, bool is_removed) = 0;
    QSGS_LOGIC void setPlayerRoleShown(Player *player, bool is_role_shown) = 0;
    QSGS_LOGIC void setPlayerKingdom(Player *player, const QString &kingdom) = 0;

    QSGS_LOGIC void playerGainMark(Player *player, const QString &mark_name, int count = 1) = 0;
    QSGS_LOGIC void playerLoseMark(Player *player, const QString &mark_name, int count = 1) = 0; // count == -1 means all mark

    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void setPlayerProperty(Player *player, const char *property_name, const QVariant &value);

    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void addPlayerMark(Player *player, const QString &mark, int add_num = 1);
    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void removePlayerMark(Player *player, const QString &mark, int remove_num = 1);

    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void setPlayerMark(Player *player, const QString &mark, int value);
    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void removePlayerMark(Player *player, const QString &mark, int remove_num = 1);

    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void setPlayerCardLimitation(Player *player, const QString &limit_list, const QString &pattern, const QString &reason, bool single_turn);
    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void removePlayerCardLimitation(Player *player, const QString &limit_list, const QString &pattern, const QString &reason, bool clearReason = false);
    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void clearPlayerCardLimitation(Player *player, bool single_turn);

    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void setPlayerDisableShow(Player *player, const QString &flags, const QString &reason);
    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void removePlayerDisableShow(Player *player, const QString &reason);

    // TODO: Remove notification and use Card::addFlag instead.
    // QSGS_LOGIC void setCardFlag(const Card *card, const QString &flag, Player *who = nullptr);
    // QSGS_LOGIC void setCardFlag(int card_id, const QString &flag, Player *who = nullptr);
    // QSGS_LOGIC void clearCardFlag(const Card *card, Player *who = nullptr);
    // QSGS_LOGIC void clearCardFlag(int card_id, Player *who = nullptr);

    QSGS_LOGIC bool useCard(const CardUseStruct &card_use, bool add_history = true);

    // TODO: Decouple the slash for these functions
    QSGS_LOGIC bool cardEffect(const Card *card, Player *from, Player *to, bool multiple = false);
    QSGS_LOGIC bool cardEffect(const CardEffectStruct &effect);

    QSGS_LOGIC void damage(const DamageStruct &data);
    QSGS_LOGIC void applyDamage(Player *victim, const DamageStruct &damage); // Change Player HP according to @param damage.
    QSGS_LOGIC void sendDamageLog(const DamageStruct &data);

    QSGS_LOGIC void loseHp(Player *victim, int lose = 1);
    QSGS_LOGIC void loseMaxHp(Player *victim, int lose = 1);
    QSGS_LOGIC bool changeMaxHpForAwakenSkill(Player *player, int magnitude = -1);

    QSGS_LOGIC void recover(Player *player, const RecoverStruct &recover, bool set_emotion = false);

    // TODO: Remove it when fixing the Jink.
    QSGS_LOGIC bool isJinkEffected(const SlashEffectStruct &effect, const Card *jink);

    QSGS_LOGIC void judge(JudgeStruct &judge_struct);
    QSGS_LOGIC void retrial(const Card *card, Player *player, JudgeStruct *judge, const QString &skill_name, bool exchange = false);
    QSGS_LOGIC void sendJudgeResult(const JudgeStruct *judge); // Send log

    QSGS_LOGIC QList<int> peekCards(int n = 1, bool bottom = false); // Cards still stay in the pile. shuffle the pile if the number left is not enough
    QSGS_LOGIC void shuffleDrawPile();
    // QSGS_LOGIC int drawCard(bool bottom = false);
    // QSGS_LOGIC QList<int> getNCards(int n, bool update_pile_number = true, bool bottom = false);
    // QSGS_LOGIC void returnToTopDrawPile(const QList<int> &cards);

    QSGS_LOGIC void askForGuanxing(Player *player, const QList<int> &cards, GuanxingType guanxing_type = GuanxingBothSides, QString skillName = "");

    // Create the Frame for selecting cards
    QSGS_LOGIC void fillAG(const QList<int> &card_ids, Player *who = nullptr, const QList<int> &disabled_ids = QList<int>(), const QList<int> &shownHandcard_ids = QList<int>());
    // Let player select from the frame.
    QSGS_LOGIC int askForAG(Player *player, const QList<int> &card_ids, bool refusable, const QString &reason);
    // Trigger Moving Card
    QSGS_LOGIC void takeAG(Player *player, int card_id, bool move_cards = true, QList<Player *> to_notify = QList<Player *>(), Player::Place fromPlace = Player::DrawPile);
    QSGS_LOGIC void clearAG(Player *player = nullptr); // disappear the selecting frame.

    // Only available for CardAsked
    QSGS_LOGIC void provide(const Card *card, Player *who = nullptr);

    QSGS_LOGIC void sendLog(const LogMessage &log);
    QSGS_LOGIC void sendLog(const QString &logtype, Player *logfrom, const QString &logarg = QString(), const QList<Player *> &logto = QList<Player *>(),
                            const QString &logarg2 = QString());

    QSGS_LOGIC void showCard(Player *player, int card_id, Player *only_viewer = nullptr);
    QSGS_LOGIC void showAllCards(Player *player, Player *to = nullptr);

    // For FilterSkill
    // QSGS_LOGIC bool notifyUpdateCard(Player *player, int cardId, const Card *newCard);
    // QSGS_LOGIC bool broadcastUpdateCard(const QList<Player *> &players, int cardId, const Card *newCard);
    // QSGS_LOGIC bool notifyResetCard(Player *player, int cardId);
    // QSGS_LOGIC bool broadcastResetCard(const QList<Player *> &players, int cardId);
    // QSGS_LOGIC void filterCards(Player *player, QList<const Card *> cards, bool refilter);

    QSGS_LOGIC void notifySkillInvoked(Player *player, const QString &skill_name);

    // Play Audio Effect
    QSGS_LOGIC void broadcastSkillInvoke(const QString &skillName);
    QSGS_LOGIC void broadcastSkillInvoke(const QString &skillName, const QString &category);
    QSGS_LOGIC void broadcastSkillInvoke(const QString &skillName, int type);
    QSGS_LOGIC void broadcastSkillInvoke(const QString &skillName, bool isMale, int type);

    // Animation Helpers
    QSGS_LOGIC void doIndicateAnimation(const Player *from, const Player *to) = 0;
    QSGS_LOGIC void doLightbox(const QString &lightboxName, int duration = 2000) = 0;
    QSGS_LOGIC void doHuashenAnimation(const Player *player, const QString &skill_name) = 0;
    QSGS_LOGIC void doNullificationAnimation(const Player *from, const Player *to) = 0;
    QSGS_LOGIC void doFireAnimation(const Player *player);
    QSGS_LOGIC void doLightningAnimation(const Player *player);
    QSGS_LOGIC void doBattleArrayAnimation(Player *player, Player *target = nullptr);
    QSGS_LOGIC void setEmotion(Player *target, const QString &emotion);

    QSGS_LOGIC void doAnimation(QSanProtocol::AnimateType type, const QString &arg1 = QString(), const QString &arg2 = QString(), QList<Player *> players = QList<Player *>());

    // TODO: Move this function to Mode
    // QSGS_LOGIC void preparePlayers();

    QSGS_LOGIC void changeHero(Player *player, const QString &new_general, bool full_state, bool invoke_start = true, bool isSecondaryHero = false, bool sendLog = true);
    QSGS_LOGIC void transformGeneral(Player *player, QString general_name, int head);

    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void changePlayerGeneral(Player *player, const QString &new_general);
    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void changePlayerGeneral2(Player *player, const QString &new_general);

    QSGS_LOGIC void setPlayerSkillInvalidity(Player *player, const QString &skill_name, bool invalidity, bool trigger_event = true);

    QSGS_LOGIC void adjustSeats();
    QSGS_LOGIC void swapSeat(Player *a, Player *b);

    QSGS_LOGIC void setFixedDistance(Player *from, const Player *to, int distance);

    QSGS_LOGIC void reverseFor3v3(const Card *card, Player *player, QList<Player *> &list); // askForOrder

    QSGS_LOGIC void updateRoleStatistic();

    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void marshal(Player *player);

    // TODO: use sendLog() instead

    QSGS_LOGIC void drawCards(Player *player, int n, const QString &reason = QString());
    QSGS_LOGIC void drawCards(QList<Player *> players, int n, const QString &reason = QString());
    QSGS_LOGIC void drawCards(QList<Player *> players, QList<int> n_list, const QString &reason = QString());

    QSGS_LOGIC void obtainCard(Player *target, const Card *card, bool unhide = true);
    QSGS_LOGIC void obtainCard(Player *target, int card_id, bool unhide = true);
    QSGS_LOGIC void obtainCard(Player *target, const Card *card, const CardMoveReason &reason, bool unhide = true);

    QSGS_LOGIC void throwCard(int card_id, Player *who, Player *thrower = nullptr, bool notifyLog = true);
    QSGS_LOGIC void throwCard(const Card *card, Player *who, Player *thrower = nullptr, bool notifyLog = true);
    QSGS_LOGIC void throwCard(const Card *card, const CardMoveReason &reason, Player *who, Player *thrower = nullptr, bool notifyLog = true);

    QSGS_LOGIC void moveCardTo(const Card *card, Player *dstPlayer, Player::Place dstPlace, bool forceMoveVisible = false);
    QSGS_LOGIC void moveCardTo(const Card *card, Player *dstPlayer, Player::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible = false);
    QSGS_LOGIC void moveCardTo(const Card *card, Player *srcPlayer, Player *dstPlayer, Player::Place dstPlace, const CardMoveReason &reason, bool forceMoveVisible = false);
    QSGS_LOGIC void moveCardTo(const Card *card, Player *srcPlayer, Player *dstPlayer, Player::Place dstPlace, const QString &pileName, const CardMoveReason &reason,
                               bool forceMoveVisible = false);

    QSGS_LOGIC void moveCardsAtomic(QList<CardsMoveStruct> cards_move, bool forceMoveVisible);
    QSGS_LOGIC void moveCardsAtomic(CardsMoveStruct cards_move, bool forceMoveVisible);
    QSGS_LOGIC QList<CardsMoveStruct> _breakDownCardMoves(QList<CardsMoveStruct> &cards_moves);

    QSGS_LOGIC void moveCardsToEndOfDrawpile(QList<int> card_ids, bool forceVisible = false);

    // interactive methods
    QSGS_LOGIC bool activate(Player *player);
    // askForUseCard / askForUseSlashTo (Card::MethodUse)
    // TODO: askForUseCard supports the recast here. (Add additional parameter to this function, or change method -> QList<Card::HandlingMethod>)
    QSGS_LOGIC const Card *askForUseCard(Player *player, const QString &pattern, const QString &prompt, int notice_index = -1, Card::HandlingMethod method = Card::MethodUse,
                                         bool addHistory = true, const QString &skill_name = QString());
    QSGS_LOGIC const Card *askForUseSlashTo(Player *slasher, Player *victim, const QString &prompt, bool distance_limit = true, bool disable_extra = false,
                                            bool addHistory = false);
    QSGS_LOGIC const Card *askForUseSlashTo(Player *slasher, QList<Player *> victims, const QString &prompt, bool distance_limit = true, bool disable_extra = false,
                                            bool addHistory = false);
    QSGS_LOGIC const Card *askForUseJink(Player *player, ...) = 0;

    QSGS_LOGIC void askForLuckCard();

    QSGS_LOGIC Card::Suit askForSuit(Player *player, const QString &reason);

    QSGS_LOGIC QString askForKingdom(Player *player);

    QSGS_LOGIC bool askForSkillInvoke(Player *player, const QString &skill_name, const QVariant &data = QVariant(), const QString &prompt = QString());

    QSGS_LOGIC QString askForChoice(Player *player, const QString &skill_name, const QString &choices, const QVariant &data = QVariant());

    // TODO: Add pattern to askForDiscard (Card::MethodDiscard)
    QSGS_LOGIC bool askForDiscard(Player *target, const QString &reason, int discard_num, int min_num, bool optional = false, bool include_equip = false,
                                  const QString &prompt = QString());
    QSGS_LOGIC void doJileiShow(Player *player, const IDSet &jilei_ids);

    // TODO: Add pattern to askForExchange (Card::MethodNone)
    QSGS_LOGIC const Card *askForExchange(Player *player, const QString &reason, int discard_num, int min_num, bool include_equip = false, const QString &prompt = QString(),
                                          bool optional = false);

    // TODO: Fix the process of nullification.
    QSGS_LOGIC bool askForNullification(const Card *trick, Player *from, Player *to, bool positive);
    QSGS_LOGIC bool isCanceled(const CardEffectStruct &effect);

    QSGS_LOGIC int askForCardChosen(Player *player, Player *who, const QString &flags, const QString &reason, bool handcard_visible = false,
                                    Card::HandlingMethod method = Card::MethodNone, const QList<int> &disabled_ids = QList<int>());
    
    QSGS_LOGIC const Card *askForCard(Player *player, const QString &pattern, const QString &prompt, const QVariant &data, const QString &skill_name, int notice_index = -1);
    QSGS_LOGIC const Card *askForCard(Player *player, const QString &pattern, const QString &prompt, const QVariant &data = QVariant(),
                                      Card::HandlingMethod method = Card::MethodDiscard, Player *to = nullptr, bool isRetrial = false, const QString &skill_name = QString(),
                                      bool isProvision = false, int notice_index = -1);

    QSGS_LOGIC const Card *askForResponse(Player *, ...);

    QSGS_LOGIC void doExtraAmazingGrace(Player *from, Player *target, int times);

    QSGS_LOGIC const Card *askForCardShow(Player *player, Player *requestor, const QString &reason);

    QSGS_LOGIC int askForRende(Player *liubei, QList<int> &cards, const QString &skill_name = QString(), bool visible = false, bool optional = true, int max_num = -1,
                               QList<Player *> players = QList<Player *>(), CardMoveReason reason = CardMoveReason(), const QString &prompt = QString(), bool notify_skill = false);
    QSGS_LOGIC bool askForYiji(Player *guojia, QList<int> &cards, const QString &skill_name = QString(), bool is_preview = false, bool visible = false, bool optional = true,
                               int max_num = -1, QList<Player *> players = QList<Player *>(), CardMoveReason reason = CardMoveReason(), const QString &prompt = QString(),
                               bool notify_skill = false);

    QSGS_LOGIC const Card *askForPindian(PindianStruct &pindian);
    QSGS_LOGIC QList<const Card *> askForPindianRace(Player *from, Player *to, const QString &reason);

    QSGS_LOGIC Player *askForPlayerChosen(Player *player, const QList<Player *> &targets, const QString &reason, const QString &prompt = QString(), bool optional = false,
                                          bool notify_skill = false);

    QSGS_LOGIC QString askForGeneral(Player *player, const QStringList &generals, QString default_choice = QString());
    QSGS_LOGIC QString askForGeneral(Player *player, const QString &generals, QString default_choice = QString());

    QSGS_LOGIC const Card *askForSinglePeach(Player *player, Player *dying); // Replace with askForUseCard

    QSGS_LOGIC QSharedPointer<SkillInvokeDetail> askForTriggerOrder(Player *player, const QList<QSharedPointer<SkillInvokeDetail> > &sameTiming, bool cancelable,
                                                                    const QVariant &data);
    
    // TODO: Move this function to the Player since it will not trigger events, but need the communication to the client.
    // QSGS_LOGIC void addPlayerHistory(Player *player, const QString &key, int times = 1);
    // QSGS_LOGIC bool roleStatusCommand(Player *player); 

    QSGS_LOGIC void updateCardsOnLose(const CardsMoveStruct &move);
    QSGS_LOGIC void updateCardsOnGet(const CardsMoveStruct &move);

    QSGS_LOGIC void cheat(Player *player, const QVariant &args);
    QSGS_LOGIC bool makeSurrender(Player *player);

protected:
    RoomObject *state;
};

};

#endif