#ifndef TOUHOUKILL_ROOMSCENE_H_
#define TOUHOUKILL_ROOMSCENE_H_

#include "client.h"
#include "clientplayer.h"

#include <QPointer>
#include <QQuickItem>

class ViewAsSkill;
class ResponseSkill;
class DiscardSkill;
class ShowOrPindianSkill;
class ChoosePlayerSkill;

class RoomScene : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool gameStarted MEMBER gameStarted NOTIFY gameStartedChanged)
    Q_PROPERTY(bool gameOver MEMBER gameOver NOTIFY gameOverChanged)
    Q_PROPERTY(ClientPlayer *Self READ selfHelper STORED false)
    Q_PROPERTY(Client *ClientInstance READ clientHelper STORED false)

public:
    explicit RoomScene(QQuickItem *parent = nullptr);
    ~RoomScene() override;

    Q_DISABLE_COPY_MOVE(RoomScene)

    // These function exists since it is needed to refactor Self and ClientInstance from singleton
    [[nodiscard]] ClientPlayer *selfHelper() const;
    [[nodiscard]] Client *clientHelper() const;

    // Bridge entry: connect Client's askFor*/show*/*_got signals to QML notify* signals.
    // Safe to call multiple times; idempotent via QPointer tracking.
    void connectClientSignals();

    // Player response path (QML -> Client). commandType is QSanProtocol::CommandType as int.
    // TODO: full implementation in the next phase (player response submission).
    Q_INVOKABLE void replyToServer(int commandType, const QVariant &data = QVariant());
    Q_INVOKABLE void notifyServer(int commandType, const QVariant &data = QVariant());

    // ---- Phase 1: card selection (Dashboard) ----
    // Explicit ViewAsSkill: check whether a card passes viewFilter.
    // selectedIds are already-picked material cards (for multi-card skills).
    [[nodiscard]] Q_INVOKABLE bool skillViewFilter(const QString &skillName, const QVariantList &selectedIds, int cardId) const;

    // ---- Phase 2: target selection (RoomScene) ----
    // Generate virtual card from materials. Returns const Card* (nullptr if invalid).
    // QML reads card.targetFixed(Self) directly, calls enabledTargetsForCard(card) for targets.
    // Real cards from hand go through the same enabledTargetsForCard path:
    //   var card = Sanguosha.getEngineCard(id);  // real
    //   var card = roomScene.skillViewAs(name, ids);  // virtual
    // Both are Card* and use the same target-selection pipeline.
    [[nodiscard]] Q_INVOKABLE const Card *skillViewAs(const QString &skillName, const QVariantList &cardIds);

    // Returns valid target player objectNames for any Card (real or virtual).
    // Unified path for both hand cards and skill-generated virtual cards.
    [[nodiscard]] Q_INVOKABLE QStringList enabledTargetsForCard(const Card *card) const;

    // Convenience: enabledTargetsForCard via engine cardId.
    [[nodiscard]] Q_INVOKABLE QStringList enabledTargets(int cardId) const;

    // ---- Phase 3: submit ----
    // Unified card-response: QML has a Card* (real from Sanguosha.getCard(id),
    // virtual from skillViewAs) + target names. Bridge converts targets and calls
    // Client::onPlayerResponseCard. Works for both hand cards and skill cards.
    Q_INVOKABLE void respondCard(const Card *card, const QStringList &targetNames = {});

    // Aux-skill submit (auto-detected: ResponseSkill/DiscardSkill via m_currentViewAsSkill).
    // For simple patterns like "slash"/"peach"/discard/exchange where QML just passes
    // raw cardIds without explicitly getting a Card*.
    Q_INVOKABLE void submitCardResponse(const QVariantList &cardIds, const QStringList &targetNames = {});
    Q_INVOKABLE void submitDiscard(const QVariantList &cardIds);
    Q_INVOKABLE void discardCard(const Card *card);

    // Simple Client wrappers (all onPlayer* calls go through bridge, never direct from QML).
    Q_INVOKABLE void respondToSkillInvoke(bool invoke);
    Q_INVOKABLE void finishPlayPhase();
    Q_INVOKABLE void cancelResponse(int status);

    // Pops up the C++ FreeChooseDialog (modal) and returns the chosen general name,
    // or an empty string if cancelled. Used by ChooseGeneralBox right-click to swap a general.
    Q_INVOKABLE QString freeChooseGeneral();

    // Pops up the C++ RoleAssignDialog (modal). The dialog is self-contained:
    // accept() calls ClientInstance->onPlayerAssignRole() and reject() calls replyToServer(),
    // so no return value or further QML handling is needed. Used on notifyAssignAsked.
    Q_INVOKABLE void showRoleAssignDialog();

    // Pops up the C++ GameOverDialog (modal). standoff == true shows the standoff
    // result (single table); false shows the winner/loser split. The dialog owns its
    // own "Return to main menu" button (deferred scene switch via QTimer).
    Q_INVOKABLE void showGameOverDialog(bool standoff);

    // Returns a player's visible non-equip skills as QVariantMaps for QML skill buttons /
    // tooltips. Each map: skillName, skillType (proactive/frequent/compulsory/awaken/oneoff/
    // array/attachedlord), translatedName, description, viewAsSkillName (empty if none).
    // Lord skills are filtered for non-lord players (mirror old updateSkillButtons).
    [[nodiscard]] Q_INVOKABLE QVariantList getPlayerSkillButtons(ClientPlayer *player) const;

signals:
    void gameStartedChanged(bool newGameStarted);
    void gameOverChanged(bool newGameOver);

    // ---- C++ -> QML notification signals (one per Client signal that QML needs to react to) ----
    // Popup-type notifications (core TODO from plan.md)
    void notifyGeneralsGot(const QStringList &generals, bool singleResult, bool canConvert);
    void notifyKingdomsGot(const QStringList &kingdoms);
    void notifySuitsGot(const QStringList &suits);
    void notifyOptionsGot(const QString &skillName, const QStringList &options);
    void notifyCardsGot(ClientPlayer *player, const QString &flags, const QString &reason, bool handcardVisible, int method, const QVariantList &disabledIds, bool enableEmptyCard);
    void notifyRolesGot(const QString &scheme, const QStringList &roles);
    void notifyDirectionsGot();
    void notifyOrdersGot(int reason);
    void notifyTriggersGot(const QVariantList &options, bool optional);
    void notifyGuanxing(const QVariantList &cardIds, bool singleSide, const QString &skillName);
    void notifyGongxin(const QVariantList &cardIds, bool enableHeart, const QVariantList &enabledIds, const QVariantList &shownHandcardIds);
    void notifyAgFilled(const QVariantList &cardIds, const QVariantList &disabledIds, const QVariantList &shownHandcardIds);
    void notifyAgTaken(ClientPlayer *taker, int cardId, bool moveCards);
    void notifyAgCleared();
    void notifyGeneralsFilled(const QStringList &generalNames);
    void notifyGeneralTaken(const QString &who, const QString &name, const QString &rule);
    void notifyGeneralAsked();
    void notifyArrangeStarted(const QString &toArrange);
    void notifyGeneralRecovered(int index, const QString &name);
    void notifyGeneralRevealed(bool self, const QString &general);
    void notifyAssignAsked();

    // Event-type notifications
    void notifyStartInXs();
    void notifyHeadPreshowed();
    void notifyDeputyPreshowed();
    void notifyLogReceived(const QStringList &logStr);
    void notifyEmotionSet(const QString &target, const QString &emotion);
    void notifySkillInvoked(const QString &who, const QString &skillName);
    void notifyAnimated(int name, const QStringList &args);
    void notifyTextSpoken(const QString &text);
    void notifyLineSpoken(const QString &line);
    void notifyPlayerSpoken(const QString &who, const QString &line);
    void notifyFocusMoved(const QStringList &focus, const QVariant &countdown);
    void notifyGameStarted();
    void notifyGameOver();
    void notifyStandoff();
    void notifyPlayerAdded(ClientPlayer *newPlayer);
    void notifyPlayerRemoved(const QString &playerName);
    void notifySeatsArranged();
    void notifyStatusChanged(int newStatus);
    void notifyMoveCardsGot(int moveId, const QVariantList &moves);
    void notifyMoveCardsLost(int moveId, const QVariantList &moves);
    void notifyPlayerKilled(const QString &who);
    void notifyPlayerRevived(const QString &who);
    void notifyDashboardDeath(const QString &who);
    void notifyCardShown(const QString &playerName, int cardId);
    void notifyNullificationAsked(bool asked);
    void notifySurrenderEnabled(bool enabled);
    void notifySkillAcquired(ClientPlayer *player, const QString &skillName, bool head);
    void notifySkillInvalidityChanged(ClientPlayer *player);
    void notifySkillAttached(const QString &skillName, bool fromLeft);
    void notifySkillDetached(const QString &skillName, bool head);
    // Game event forwarded from Client::event_received (mirror old handleGameEvent).
    // Client::handleGameEvent does UI-independent state updates first, then emits
    // event_received; this signal lets QML react to UI-side aspects. args is the raw
    // JsonArray ([eventType, ...]).
    void notifyEventReceived(const QVariant &args);
    void notifyPerspectiveChanged(const QString &targetName, const QVariantList &handCardIds, const QVariantMap &piles);
    void notifyRoleStateChanged(const QString &stateStr);
    void notifyGeneralsViewed(const QString &reason, const QStringList &names);

private:
    // Configure the current ViewAsSkill / aux-skill from Client status + pattern.
    // Called internally from status_changed handler before emit notifyStatusChanged.
    void updateAuxSkill();

    bool gameStarted;
    bool gameOver;
    bool m_clientConnected; // tracks whether connectClientSignals() has wired Client signals

    // Aux-skills: created once, reconfigured on each status change. Owned by this.
    ResponseSkill *m_responseSkill = nullptr;
    DiscardSkill *m_discardSkill = nullptr;
    ShowOrPindianSkill *m_showOrPindianSkill = nullptr;
    ChoosePlayerSkill *m_choosePlayerSkill = nullptr;

    // Points to whichever aux-skill (or real ViewAsSkill) is currently active.
    // nullptr when no card/response is expected. tryViewAsCard / submitCardResponse
    // / submitDiscard consult this pointer to generate the virtual card.
    const ViewAsSkill *m_currentViewAsSkill = nullptr;
};

extern QPointer<RoomScene> RoomSceneInstance;

#endif
