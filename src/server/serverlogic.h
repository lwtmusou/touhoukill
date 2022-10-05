#ifndef TOUHOUKILL_SERVERLOGIC_H_
#define TOUHOUKILL_SERVERLOGIC_H_

#include "game-logic.h"

class ServerLogic : public GameLogic
{
public:
    ServerLogic(RoomObject *room);
    ~ServerLogic() override;

public:
    // --- logic manipulation only ---
    void enterDying(Player *player, DamageStruct *reason) override;
    void buryPlayer(Player *victim) override;
    void killPlayer(Player *victim, DamageStruct *reason) override;
    void revivePlayer(Player *player, bool initialize) override;
    void gameOver(const QString &winner, bool isSurrender) override;
    void handleAcquireLoseSkills(Player *player, const QStringList &acquireSkills, const QStringList &loseSkills, bool acquire_only) override;
    void showPlayerHiddenSkill(Player *player, const QString &skill_name) override;
    void setPlayerHP(Player *player, int value) override;
    void setPlayerMaxHP(Player *player, int value) override;
    void setPlayerChained(Player *player, bool is_chained) override;
    void setPlayerRemoved(Player *player, bool is_removed) override;
    void setPlayerRoleShown(Player *player, bool is_role_shown) override;
    void setPlayerKingdom(Player *player, const QString &kingdom) override;
    void setPlayerGender(Player *player, QSanguosha::Gender gender) override;
    void playerGainMark(Player *player, const QString &mark_name, int count) override;
    void playerLoseMark(Player *player, const QString &mark_name, int count) override;
    bool useCard(const CardUseStruct &card_use, bool add_history) override;
    bool cardEffect(const CardEffectStruct &effect) override;
    void damage(const DamageStruct &data) override;
    void applyDamage(Player *victim, const DamageStruct &damage) override;
    void loseHp(Player *victim, int lose) override;
    void loseMaxHp(Player *victim, int lose) override;
    bool changeMaxHpForAwakenSkill(Player *player, int magnitude) override;
    void recover(Player *player, const RecoverStruct &recover, bool set_emotion) override;
    void turnPlayerOver(Player *player) override;
    void letPlayerPlay(Player *target, const QList<QSanguosha::Phase> &phases) override;
    void changePlayerPhase(Player *target, QSanguosha::Phase from, QSanguosha::Phase to) override;
    void skipPlayerPhase(Player *player, QSanguosha::Phase phase, bool is_cost, bool send_log) override;
    void insertPlayerPhases(Player *player, const QList<QSanguosha::Phase> &new_phases, int index) override;
    void exchangePlayerPhases(Player *player, QSanguosha::Phase from, QSanguosha::Phase to) override;
    void givePlayerAnExtraTurn(Player *benefiter) override;
    void judge(JudgeStruct &judge_struct) override;
    void retrial(const Card *card, Player *player, JudgeStruct *judge, const QString &skill_name, bool exchange) override;
    void sendJudgeResult(const JudgeStruct *judge) override;
    bool pindian(Player *source, Player *target, const QString &reason) override;
    QList<int> peekCards(int n, bool bottom) override;
    void shuffleDrawPile() override;
    void fillAG(const QList<int> &card_ids, const QList<int> &disabled_ids, const QList<Player *> &viewers) override;
    void takeAG(Player *player, int card_id, bool move_cards, QSanguosha::Place fromPlace) override;
    void clearAG(const QList<Player *> &viewers) override;
    void provide(const Card *card, Player *who) override;
    void sendLog(const LogStruct &log) override;
    void showCard(Player *player, int card_id, Player *only_viewer) override;
    void showAllCards(Player *player, Player *to) override;
    void notifySkillInvoked(Player *player, const QString &skill_name) override;
    void broadcastSkillInvoke(const QString &skillName) override;
    void broadcastSkillInvoke(const QString &skillName, const QString &category) override;
    void broadcastSkillInvoke(const QString &skillName, int type) override;
    void broadcastSkillInvoke(const QString &skillName, bool isMale, int type) override;
    void broadcastSkillInvoke(const Player *player, const Card *card) override;
    void broadcastSkillInvoke(const Player *player, const QString *card_name) override;
    void doIndicateAnimation(const Player *from, const Player *to) override;
    void doLightbox(const QString &lightboxName, int duration) override;
    void doHuashenAnimation(const Player *player, const QString &skill_name) override;
    void doNullificationAnimation(const Player *from, const Player *to) override;
    void doFireAnimation(const Player *player) override;
    void doLightningAnimation(const Player *player) override;
    void doBattleArrayAnimation(Player *player, Player *target) override;
    void setEmotion(Player *target, const QString &emotion) override;
    void doAnimation(QSanProtocol::AnimateType type, const QString &arg1, const QString &arg2, QList<Player *> players) override;
    void changeHero(Player *player, const QString &new_general, bool full_state, bool invoke_start, bool isSecondaryHero, bool sendLog) override;
    void transformGeneral(Player *player, QString general_name, int head) override;
    void setPlayerSkillInvalidity(Player *player, const QString &skill_name, bool invalidity, bool trigger_event) override;
    void reverseFor3v3(const Card *card, Player *player, QList<Player *> &list) override;
    void addToPlayerSelectedGeneral(Player *target, const QString &general_name) override;
    void clearPlayerSelectedGeneral(Player *target) override;
    void drawCards(Player *player, int n, const QString &reason) override;
    void drawCards(QList<Player *> players, int n, const QString &reason) override;
    void drawCards(QList<Player *> players, QList<int> n_list, const QString &reason) override;
    void obtainCard(Player *target, const Card *card, bool unhide) override;
    void obtainCard(Player *target, int card_id, bool unhide) override;
    void throwCard(int card_id, Player *who, Player *thrower, bool notifyLog) override;
    void throwCard(const Card *card, Player *who, Player *thrower, bool notifyLog) override;
    void throwPlayerAllEquips(const Player *victim) override;
    void throwPlayerAllHandCards(const Player *victim) override;
    void throwPlayerDelayTrickRegion(const Player *victim) override;
    void throwPlayerAllCards(const Player *victim) override;
    void addToPlayerPile(Player *owner, const QString &pile_name, const Card *card, bool open) override;
    void addToPlayerPile(Player *owner, const QString &pile_name, int card_id, bool open) override;
    void addToPlayerPile(Player *owner, const QString &pile_name, const IdSet &card_id, bool open) override;
    void addToPlayerShownHandCards(Player *target, const IdSet &card_ids) override;
    void removePlayerShownHandCards(Player *target, const IdSet &card_ids) override;
    void addPlayerBrokenEquip(Player *target, const IdSet &ids) override;
    void removePlayerBrokenEquip(Player *target, const IdSet &ids) override;
    void addPlayerHiddenGeneral(Player *target, const QStringList &generals) override;
    void removePlayerHiddenGeneral(Player *target, const QStringList) override;
    void clearPlayerPrivatePile(const Player *target, const QString &pile_name) override;
    void clearPlayerAllPrivatePiles(const Player *target) override;
    void doJileiShow(Player *player, const IdSet &jilei_ids) override;
    void forcePlayerDiscard(const Player *target, int discard_num, int include_equip, bool is_discard) override;
    void moveCardTo(const Card *card, Player *dstPlayer, QSanguosha::Place dstPlace, bool forceVisible) override;
    void moveCardsAtomic(CardsMoveStruct cards_move, bool forceVisible) override;

public:
    // --- interactive methods ---
    void activate(CardUseStruct &use, Player *player) override;
    void askForGuanxing(QList<int> &upCards, QList<int> &downCards, Player *player, const QList<int> &cards, QSanguosha::GuanxingType guanxingType, QString skillName) override;
    void askForUseCard(CardUseStruct &use, Player *player, const QString &reason, const QString &pattern, bool optional, const QString &prompt, bool addHistory,
                       const QList<QSanguosha::HandlingMethod> &methods, bool enableConversion) override;
    void askForUseCard(CardUseStruct &use, Player *player, const QString &skillName, int patternIndex, bool optional, const QString &prompt,
                       const QList<QSanguosha::HandlingMethod> &methods) override;
    void askForUseSlashTo(CardUseStruct &use, Player *slasher, const QString &reason, Player *victim, bool disableExtra, bool optional, const QString &prompt, bool addHistory,
                          bool enableConversion) override;
    void askForUseSlashTo(CardUseStruct &use, Player *slasher, const QString &reason, const QList<Player *> &victims, bool disableExtra, bool optional, const QString &prompt,
                          bool addHistory, bool enableConversion) override;
    void askForJink(CardUseStruct &use, Player *player, const CardEffectStruct &slashUse, bool enableConversion) override;
    void askForSinglePeach(CardUseStruct &use, Player *toAsk, const DeathStruct &dying, bool enableConversion) override;
    void askForNullification(CardUseStruct &use, const CardEffectStruct &trickEffect, bool enableConversion) override;
    void askForNullification(CardUseStruct &use, const QList<Player *> &toAsk, const CardEffectStruct &trickEffect, bool enableConversion) override;
    void askForResponseCard(CardResponseStruct &response, Player *player, const QString &reason, const QString &pattern, bool optional, const QString &prompt, bool isRetrial,
                            bool isProvision, const QList<QSanguosha::HandlingMethod> &methods, bool enableConversion) override;
    void askForResponseCard(CardResponseStruct &response, Player *player, const QString &skillName, int patternIndex, bool optional, const QString &prompt, bool isRetrial,
                            bool isProvision, const QList<QSanguosha::HandlingMethod> &methods) override;
    void askForDiscard(IdSet &discardedIds, Player *target, const QString &reason, int max_num, int min_num, bool include_equip, bool optional, const QString &prompt) override;
    void askForDiscard(IdSet &discardedIds, Player *target, const QString &reason, const QString &pattern, bool optional, const QString &prompt) override;
    void askForDiscard(IdSet &discardedIds, Player *target, const QString &skillName, int patternIndex, bool optional, const QString &prompt) override;
    void askForExchange(IdSet &discardedIds, Player *target, const QString &reason, int max_num, int min_num, bool include_equip, bool optional, const QString &prompt) override;
    void askForExchange(IdSet &discardedIds, Player *target, const QString &reason, const QString &pattern, bool optional, const QString &prompt) override;
    void askForExchange(IdSet &discardedIds, Player *target, const QString &skillName, int patternIndex, bool optional, const QString &prompt) override;
    void askForCardGive(QHash<Player *, int> &give, Player *giver, const QString &reason, const IdSet &cardIds, bool optional, const QString &prompt,
                        const QList<Player *> players) override;
    QSanguosha::Suit askForSuit(Player *player, const QString &reason) override;
    QString askForKingdom(Player *player, const QString &reason) override;
    bool askForSkillInvoke(Player *player, const QString &skill_name, const QString &prompt) override;
    QString askForChoice(Player *player, const QString &reason, const QStringList &choices, const QString &defaultChoice) override;
    QString askForGeneral(Player *player, const QStringList &generals, const QString &defaultChoice) override;
    int askForCardChosen(Player *player, Player *who, const QString &flags, const QString &reason, bool handcard_visible, QSanguosha::HandlingMethod method,
                         const QList<int> &disabled_ids) override;
    int askForAG(Player *player, const QList<int> &card_ids, bool refusable, const QString &reason) override;
    const Card *askForPindian(PindianStruct &pindian) override;
    QList<const Card *> askForPindianRace(Player *from, Player *to, const QString &reason) override;
    QList<Player *> askForPlayersChosen(Player *chooser, const QString &reason, const QList<Player *> targets, int max_num, int min_num, bool optional,
                                        const QString &prompt) override;
    Player *askForPlayerChosen(Player *chooser, const QString &reason, const QList<Player *> targets, bool optional, const QString &prompt) override;
    TriggerDetail askForTriggerOrder(Player *player, const QList<TriggerDetail> &sameTiming, bool cancelable) override;

public:
    // --- cheat related ---
    void cheat(Player *player, const QVariant &args) override;
    bool makeSurrender(Player *player) override;

public:
    // --- Server logic related ---
    void newPlayer(Player *player);
    void marshalPlayerInfo(Player *player);

private:
    // The one and only state model maintained by Logic
    RoomObject *room;
};

#endif
