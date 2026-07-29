#ifndef TOUHOUKILL_ROOMSCENE_H_
#define TOUHOUKILL_ROOMSCENE_H_

#include "client.h"
#include "clientplayer.h"
#include "photo.h"

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
    Q_PROPERTY(bool okEnabled READ okEnabled NOTIFY okEnabledChanged)
    Q_PROPERTY(bool cancelEnabled READ cancelEnabled NOTIFY cancelEnabledChanged)
    Q_PROPERTY(bool discardEnabled READ discardEnabled NOTIFY discardEnabledChanged)
    Q_PROPERTY(ClientPlayer *Self READ selfHelper STORED false)
    Q_PROPERTY(Client *ClientInstance READ clientHelper STORED false)

public:
    explicit RoomScene(QQuickItem *parent = nullptr);
    ~RoomScene() override;

    Q_DISABLE_COPY_MOVE(RoomScene)

    // These function exists since it is needed to refactor Self and ClientInstance from singleton
    [[nodiscard]] ClientPlayer *selfHelper() const;
    [[nodiscard]] Client *clientHelper() const;

    [[nodiscard]] bool okEnabled() const;
    [[nodiscard]] bool cancelEnabled() const;
    [[nodiscard]] bool discardEnabled() const;

    // Bridge entry: connect Client's askFor*/show*/*_got signals to QML notify* signals.
    // Safe to call multiple times; idempotent via QPointer tracking.
    void connectClientSignals();

    // QML -> C++ server request path. commandType is QSanProtocol::CommandType as int.
    Q_INVOKABLE void replyToServer(int commandType, const QVariant &data = QVariant());
    Q_INVOKABLE void notifyServer(int commandType, const QVariant &data = QVariant());

    // ---- Phase 1: card selection (Dashboard) ----
    [[nodiscard]] Q_INVOKABLE bool skillViewFilter(const QString &skillName, const QVariantList &selectedIds, int cardId) const;

    // ---- Phase 2: target selection (RoomScene) ----
    [[nodiscard]] Q_INVOKABLE const Card *skillViewAs(const QString &skillName, const QVariantList &cardIds);

    [[nodiscard]] Q_INVOKABLE QStringList enabledTargetsForCard(const Card *card, const QStringList &selectedTargetNames) const;
    [[nodiscard]] Q_INVOKABLE QStringList enabledTargets(int cardId, const QStringList &selectedTargetNames) const;
    [[nodiscard]] Q_INVOKABLE bool isCardTargetsFeasible(const Card *card, const QStringList &selectedTargetNames) const;

    // ---- Card/target selection (state held in C++, QML only calls these) ----
    // Update selected card state from Dashboard. Passes the full selectedCardIds list
    // (supports multi-select for Discarding/Exchanging). Resets target state, computes
    // enabled targets, syncs Photo targetable flags, recomputes OK readiness.
    Q_INVOKABLE void selectCard(const QVariantList &selectedCardIds);
    // Toggle a player in/out of the selected target list, then recompute OK readiness.
    Q_INVOKABLE void toggleTarget(const QString &playerName);
    // Status-driven button enablement (cancel/discard) + clear selections on status
    // change away from selection modes. Hand-card isAvailable enablement stays in QML
    // (Dashboard.updateStatus) until CardContainer is bridged.
    Q_INVOKABLE void updateDashboardStatus();
    // Register/unregister a Photo so C++ can drive its targetable/targetSelected.
    Q_INVOKABLE void registerPhoto(Photo *photo);
    Q_INVOKABLE void unregisterPhoto(Photo *photo);
    // QML notifies activeBox.canAccept changes so C++ can drive OK readiness for
    // AskForGeneralTaken (choose-general) without QML binding activeBox itself.
    Q_INVOKABLE void setActiveBoxCanAccept(bool canAccept);

    // ---- Phase 3: submit ----
    Q_INVOKABLE void respondCard(const Card *card);
    // Aux-skill submit. Uses internally-held m_selectedTargets (no targetNames from QML).
    Q_INVOKABLE void submitCardResponse(const QVariantList &cardIds);
    Q_INVOKABLE void submitDiscard(const QVariantList &cardIds);
    Q_INVOKABLE void discardCard(const Card *card);

    Q_INVOKABLE void respondToSkillInvoke(bool invoke);
    Q_INVOKABLE void finishPlayPhase();
    Q_INVOKABLE void cancelResponse(int status);

    Q_INVOKABLE QString freeChooseGeneral();
    Q_INVOKABLE void showRoleAssignDialog();
    Q_INVOKABLE void showGameOverDialog(bool standoff);

    [[nodiscard]] Q_INVOKABLE QVariantList getPlayerSkillButtons(ClientPlayer *player) const;

signals:
    void gameStartedChanged(bool newGameStarted);
    void gameOverChanged(bool newGameOver);
    void okEnabledChanged();
    void cancelEnabledChanged();
    void discardEnabledChanged();

    // ---- C++ -> QML notification signals (one per Client signal that QML needs to react to) ----
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
    void notifyEventReceived(const QVariant &args);
    void notifyPerspectiveChanged(const QString &targetName, const QVariantList &handCardIds, const QVariantMap &piles);
    void notifyRoleStateChanged(const QString &stateStr);
    void notifyGeneralsViewed(const QString &reason, const QStringList &names);

private:
    void updateAuxSkill();

    // Card/target selection state (held in C++, QML reads via Q_PROPERTY/signals).
    QVariantList m_selectedCardIds;
    const Card *m_selectedCard = nullptr;
    QStringList m_selectedTargets;
    QStringList m_enabledTargetNames;
    bool m_targetSelectionActive = false;
    QList<QPointer<Photo>> m_photos;

    // Button enablement (Q_PROPERTY + NOTIFY).
    bool m_okEnabled = false;
    bool m_cancelEnabled = false;
    bool m_discardEnabled = false;
    bool m_activeBoxCanAccept = false; // activeBox.canAccept for AskForGeneralTaken OK

    void recomputeOkReadiness();
    void syncPhotoTargets();

    bool gameStarted;
    bool gameOver;
    bool m_clientConnected;

    ResponseSkill *m_responseSkill = nullptr;
    DiscardSkill *m_discardSkill = nullptr;
    ShowOrPindianSkill *m_showOrPindianSkill = nullptr;
    ChoosePlayerSkill *m_choosePlayerSkill = nullptr;
    const ViewAsSkill *m_currentViewAsSkill = nullptr;
};

extern QPointer<RoomScene> RoomSceneInstance;

#endif
