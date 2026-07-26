#include "roomscene.h"

#include "aux-skills.h"
#include "choosegeneraldialog.h"
#include "client.h"
#include "clientplayer.h"
#include "engine.h"
#include "gameoverdialog.h"
#include "mainwindow.h"
#include "protocol.h"
#include "roleassigndialog.h"
#include "skill.h"
#include "structs.h"
#include "util.h"

#include <QApplication>
#include <QString>
#include <QStringView>
#include <QtQml>

using namespace Qt::Literals::StringLiterals;

QPointer<RoomScene> RoomSceneInstance;

RoomScene::RoomScene(QQuickItem *parent)
    : QQuickItem(parent)
    , gameStarted(false)
    , gameOver(false)
    , m_clientConnected(false)
    , m_responseSkill(new ResponseSkill)
    , m_discardSkill(new DiscardSkill)
    , m_showOrPindianSkill(new ShowOrPindianSkill)
    , m_choosePlayerSkill(new ChoosePlayerSkill)
{
    // ???
    if (RoomSceneInstance != nullptr)
        qFatal("RoomScene should be singleton");

    RoomSceneInstance = this;

    // Own the aux-skills so they stay alive for the lifetime of this RoomScene.
    m_responseSkill->setParent(this);
    m_discardSkill->setParent(this);
    m_showOrPindianSkill->setParent(this);
    m_choosePlayerSkill->setParent(this);

    // RoomScene is created by QML when entering room scene, at which point
    // ClientInstance / Self are guaranteed to be valid. Connect immediately.
    connectClientSignals();
}

RoomScene::~RoomScene() = default;

ClientPlayer *RoomScene::selfHelper() const
{
    return Self;
}

Client *RoomScene::clientHelper() const
{
    // TODO: consider how to get this after Client is no longer global singleton.
    // This RoomScene class is created by QML and can only have default constructor.
    return ClientInstance;
}

void RoomScene::connectClientSignals()
{
    // Idempotent: avoid double-connecting if called more than once.
    if (m_clientConnected)
        return;

    Client *client = ClientInstance;
    if (client == nullptr) {
        qWarning() << "RoomScene::connectClientSignals: ClientInstance is null, deferring connection";
        return;
    }

    m_clientConnected = true;

    // ---- Popup-type signals (core TODO: askFor*/show*/*_got) ----
    connect(client, &Client::generals_got, this, [this](const QStringList &generals, bool single_result, bool can_convert) {
        qDebug().noquote() << "[bridge] generals_got count=" << generals.size();
        emit notifyGeneralsGot(generals, single_result, can_convert);
    });
    connect(client, &Client::kingdoms_got, this, [this](const QStringList &kingdoms) {
        qDebug().noquote() << "[bridge] kingdoms_got" << kingdoms;
        emit notifyKingdomsGot(kingdoms);
    });
    connect(client, &Client::suits_got, this, [this](const QStringList &suits) {
        qDebug().noquote() << "[bridge] suits_got" << suits;
        emit notifySuitsGot(suits);
    });
    connect(client, &Client::options_got, this, [this](const QString &skillName, const QStringList &options) {
        qDebug().noquote() << "[bridge] options_got skill=" << skillName << "options=" << options;
        emit notifyOptionsGot(skillName, options);
    });
    connect(client, &Client::cards_got, this,
            [this](ClientPlayer *player, const QString &flags, const QString &reason, bool handcard_visible, Card::HandlingMethod method, const QList<int> &disabled_ids,
                   bool enableEmptyCard) {
                qDebug().noquote() << "[bridge] cards_got reason=" << reason << "flags=" << flags;
                emit notifyCardsGot(player, flags, reason, handcard_visible, static_cast<int>(method), IntList2VariantList(disabled_ids), enableEmptyCard);
            });
    connect(client, &Client::roles_got, this, [this](const QString &scheme, const QStringList &roles) {
        qDebug().noquote() << "[bridge] roles_got scheme=" << scheme << roles;
        emit notifyRolesGot(scheme, roles);
    });
    connect(client, &Client::directions_got, this, [this]() {
        qDebug().noquote() << "[bridge] directions_got";
        emit notifyDirectionsGot();
    });
    connect(client, &Client::orders_got, this, [this](QSanProtocol::Game3v3ChooseOrderCommand reason) {
        qDebug().noquote() << "[bridge] orders_got reason=" << reason;
        emit notifyOrdersGot(static_cast<int>(reason));
    });
    connect(client, &Client::triggers_got, this, [this](const QVariantList &options, bool optional) {
        qDebug().noquote() << "[bridge] triggers_got count=" << options.size() << "optional=" << optional;
        emit notifyTriggersGot(options, optional);
    });
    connect(client, &Client::guanxing, this, [this](const QList<int> &card_ids, bool single_side, const QString &skillName) {
        qDebug().noquote() << "[bridge] guanxing skill=" << skillName << "count=" << card_ids.size();
        emit notifyGuanxing(IntList2VariantList(card_ids), single_side, skillName);
    });
    connect(client, &Client::gongxin, this, [this](const QList<int> &card_ids, bool enable_heart, const QList<int> &enabled_ids, const QList<int> &shownHandcard_ids) {
        qDebug().noquote() << "[bridge] gongxin count=" << card_ids.size();
        emit notifyGongxin(IntList2VariantList(card_ids), enable_heart, IntList2VariantList(enabled_ids), IntList2VariantList(shownHandcard_ids));
    });
    connect(client, &Client::ag_filled, this, [this](const QList<int> &card_ids, const QList<int> &disabled_ids, const QList<int> &shownHandcard_ids) {
        qDebug().noquote() << "[bridge] ag_filled count=" << card_ids.size();
        emit notifyAgFilled(IntList2VariantList(card_ids), IntList2VariantList(disabled_ids), IntList2VariantList(shownHandcard_ids));
    });
    connect(client, &Client::ag_taken, this, [this](ClientPlayer *taker, int card_id, bool move_cards) {
        qDebug().noquote() << "[bridge] ag_taken card=" << card_id;
        emit notifyAgTaken(taker, card_id, move_cards);
    });
    connect(client, &Client::ag_cleared, this, [this]() {
        qDebug().noquote() << "[bridge] ag_cleared";
        emit notifyAgCleared();
    });
    connect(client, &Client::generals_filled, this, [this](const QStringList &general_names) {
        qDebug().noquote() << "[bridge] generals_filled count=" << general_names.size();
        emit notifyGeneralsFilled(general_names);
    });
    connect(client, &Client::general_taken, this, [this](const QString &who, const QString &name, const QString &rule) {
        qDebug().noquote() << "[bridge] general_taken who=" << who << "name=" << name;
        emit notifyGeneralTaken(who, name, rule);
    });
    connect(client, &Client::general_asked, this, [this]() {
        qDebug().noquote() << "[bridge] general_asked";
        emit notifyGeneralAsked();
    });
    connect(client, &Client::arrange_started, this, [this](const QString &to_arrange) {
        qDebug().noquote() << "[bridge] arrange_started" << to_arrange;
        emit notifyArrangeStarted(to_arrange);
    });
    connect(client, &Client::general_recovered, this, [this](int index, const QString &name) {
        qDebug().noquote() << "[bridge] general_recovered index=" << index << name;
        emit notifyGeneralRecovered(index, name);
    });
    connect(client, &Client::general_revealed, this, [this](bool self, const QString &general) {
        qDebug().noquote() << "[bridge] general_revealed self=" << self << general;
        emit notifyGeneralRevealed(self, general);
    });
    connect(client, &Client::assign_asked, this, [this]() {
        qDebug().noquote() << "[bridge] assign_asked";
        emit notifyAssignAsked();
    });

    // ---- Event-type signals ----
    connect(client, &Client::start_in_xs, this, [this]() {
        emit notifyStartInXs();
    });
    connect(client, &Client::head_preshowed, this, [this]() {
        emit notifyHeadPreshowed();
    });
    connect(client, &Client::deputy_preshowed, this, [this]() {
        emit notifyDeputyPreshowed();
    });
    connect(client, &Client::log_received, this, [this](const QStringList &log_str) {
        emit notifyLogReceived(log_str);
    });
    connect(client, &Client::emotion_set, this, [this](const QString &target, const QString &emotion) {
        emit notifyEmotionSet(target, emotion);
    });
    connect(client, &Client::skill_invoked, this, [this](const QString &who, const QString &skill_name) {
        emit notifySkillInvoked(who, skill_name);
    });
    connect(client, &Client::animated, this, [this](int name, const QStringList &args) {
        emit notifyAnimated(name, args);
    });
    connect(client, &Client::text_spoken, this, [this](const QString &text) {
        emit notifyTextSpoken(text);
    });
    connect(client, &Client::line_spoken, this, [this](const QString &line) {
        emit notifyLineSpoken(line);
    });
    connect(client, &Client::player_spoken, this, [this](const QString &who, const QString &line) {
        emit notifyPlayerSpoken(who, line);
    });
    connect(client, &Client::focus_moved, this, [this](const QStringList &focus, QSanProtocol::Countdown countdown) {
        emit notifyFocusMoved(focus, countdown.toVariant());
    });
    connect(client, &Client::game_started, this, [this]() {
        // Sync gameStarted so QML bindings (Photo.gameStarted -> roomScene.gameStarted)
        // flip to true, showing magatamas/handcardNum/roleComboBox/kingdom frame, etc.
        if (!gameStarted) {
            gameStarted = true;
            emit gameStartedChanged(true);
        }
        emit notifyGameStarted();
    });
    connect(client, &Client::game_over, this, [this]() {
        if (!gameOver) {
            gameOver = true;
            emit gameOverChanged(true);
        }
        emit notifyGameOver();
    });
    connect(client, &Client::standoff, this, [this]() {
        if (!gameOver) {
            gameOver = true;
            emit gameOverChanged(true);
        }
        emit notifyStandoff();
    });
    connect(client, &Client::player_added, this, [this](ClientPlayer *new_player) {
        emit notifyPlayerAdded(new_player);
    });
    connect(client, &Client::player_removed, this, [this](const QString &player_name) {
        emit notifyPlayerRemoved(player_name);
    });
    connect(client, &Client::seats_arranged, this, [this]() {
        // Seat is assigned by arrangeSeats; QML reads photo.player.seat directly.
        emit notifySeatsArranged();
    });
    connect(client, &Client::status_changed, this, [this](Client::Status newStatus) {
        updateAuxSkill();
        updateDashboardStatus();
        emit notifyStatusChanged(static_cast<int>(newStatus));
    });
    connect(client, &Client::player_killed, this, [this](const QString &who) {
        emit notifyPlayerKilled(who);
    });
    connect(client, &Client::player_revived, this, [this](const QString &who) {
        emit notifyPlayerRevived(who);
    });
    connect(client, &Client::dashboard_death, this, [this](const QString &who) {
        emit notifyDashboardDeath(who);
    });
    connect(client, &Client::card_shown, this, [this](const QString &player_name, int card_id) {
        emit notifyCardShown(player_name, card_id);
    });
    connect(client, &Client::nullification_asked, this, [this](bool asked) {
        emit notifyNullificationAsked(asked);
    });
    connect(client, &Client::surrender_enabled, this, [this](bool enabled) {
        emit notifySurrenderEnabled(enabled);
    });
    connect(client, &Client::skill_acquired, this, [this](ClientPlayer *player, const QString &skill_name, const bool &head) {
        emit notifySkillAcquired(player, skill_name, head);
    });
    connect(client, &Client::skill_invalidity_changed, this, [this](ClientPlayer *player) {
        emit notifySkillInvalidityChanged(player);
    });
    connect(client, &Client::skill_attached, this, [this](const QString &skill_name, bool from_left) {
        emit notifySkillAttached(skill_name, from_left);
    });
    connect(client, &Client::skill_detached, this, [this](const QString &skill_name, bool head) {
        emit notifySkillDetached(skill_name, head);
    });
    // Game events forwarded as-is (Client::handleGameEvent already did UI-independent
    // state updates). QML reacts to UI-side aspects via notifyEventReceived.
    connect(client, &Client::event_received, this, &RoomScene::notifyEventReceived);
    connect(client, &Client::perspective_changed, this, [this](const QString &targetName, const QList<int> &handCardIds, const QVariantMap &piles) {
        emit notifyPerspectiveChanged(targetName, IntList2VariantList(handCardIds), piles);
    });
    connect(client, &Client::role_state_changed, this, [this](const QString &state_str) {
        emit notifyRoleStateChanged(state_str);
    });
    connect(client, &Client::generals_viewed, this, [this](const QString &reason, const QStringList &names) {
        emit notifyGeneralsViewed(reason, names);
    });
    // Convert CardsMoveStruct to QML-friendly QVariantMap (bridge parses, does not
    // expose CardsMoveStruct to QML). Used by both move_cards_got/lost lambdas.
    auto toQmlMove = [](const CardsMoveStruct &m) {
        return QVariantMap {
            {u"cardIds"_s, IntList2VariantList(m.card_ids)},
            {u"fromPlace"_s, static_cast<int>(m.from_place)},
            {u"toPlace"_s, static_cast<int>(m.to_place)},
            {u"fromPlayer"_s, QVariant::fromValue(m.from)},
            {u"toPlayer"_s, QVariant::fromValue(m.to)},
            {u"fromPileName"_s, m.from_pile_name},
            {u"toPileName"_s, m.to_pile_name},
        };
    };
    connect(client, &Client::move_cards_got, this, [this, toQmlMove](int moveId, const QList<CardsMoveStruct> &moves) {
        QVariantList qmlMoves;
        for (const CardsMoveStruct &m : moves)
            qmlMoves.append(toQmlMove(m));
        qDebug().noquote() << "[bridge] move_cards_got moveId=" << moveId << "count=" << moves.size();
        emit notifyMoveCardsGot(moveId, qmlMoves);
    });
    connect(client, &Client::move_cards_lost, this, [this, toQmlMove](int moveId, const QList<CardsMoveStruct> &moves) {
        QVariantList qmlMoves;
        for (const CardsMoveStruct &m : moves)
            qmlMoves.append(toQmlMove(m));
        qDebug().noquote() << "[bridge] move_cards_lost moveId=" << moveId << "count=" << moves.size();
        emit notifyMoveCardsLost(moveId, qmlMoves);
    });

    qDebug().noquote() << "[bridge] RoomScene connected to Client signals";
}

void RoomScene::replyToServer(int commandType, const QVariant &data)
{
    Client *client = ClientInstance;
    if (client == nullptr) {
        qWarning() << "RoomScene::replyToServer: ClientInstance is null";
        return;
    }
    client->replyToServer(static_cast<QSanProtocol::CommandType>(commandType), data);
}

void RoomScene::notifyServer(int commandType, const QVariant &data)
{
    Client *client = ClientInstance;
    if (client == nullptr) {
        qWarning() << "RoomScene::notifyServer: ClientInstance is null";
        return;
    }
    client->notifyServer(static_cast<QSanProtocol::CommandType>(commandType), data);
}

QString RoomScene::freeChooseGeneral()
{
    FreeChooseDialog dialog(MainWindowInstance);
    QString chosen;
    connect(&dialog, &FreeChooseDialog::general_chosen, [&chosen](const QString &name) {
        chosen = name;
    });
    dialog.exec();
    return chosen;
}

void RoomScene::showRoleAssignDialog()
{
    // RoleAssignDialog is self-contained: accept() forwards onPlayerAssignRole() to the
    // server and reject() replies with an empty role list, so nothing to return to QML.
    RoleAssignDialog dialog(MainWindowInstance);
    dialog.exec();
}

void RoomScene::showGameOverDialog(bool standoff)
{
    GameOverDialog dialog(standoff, MainWindowInstance);
    dialog.exec();
}

// ---- Phase 1: card selection (Dashboard calls per card) ----

bool RoomScene::skillViewFilter(const QString &skillName, const QVariantList &selectedIds, int cardId) const
{
    const Skill *skill = Sanguosha->getSkill(skillName);
    if (skill == nullptr)
        return false;

    const ViewAsSkill *vas = ViewAsSkill::parseViewAsSkill(skill);
    if (vas == nullptr)
        return false;

    const Card *toSelect = ClientInstance->getCard(cardId);
    if (toSelect == nullptr)
        return false;

    QList<const Card *> selected;
    for (const QVariant &id : selectedIds) {
        const Card *c = ClientInstance->getCard(id.toInt());
        if (c != nullptr)
            selected.append(c);
    }

    return vas->viewFilter(selected, toSelect);
}

// ---- Phase 2: target selection (RoomScene calls after cards are picked) ----

const Card *RoomScene::skillViewAs(const QString &skillName, const QVariantList &cardIds)
{
    const Skill *skill = Sanguosha->getSkill(skillName);
    if (skill == nullptr)
        return nullptr;

    const ViewAsSkill *vas = ViewAsSkill::parseViewAsSkill(skill);
    if (vas == nullptr)
        return nullptr;

    QList<const Card *> cards;
    // ChoosePlayerSkill is ZeroCardViewAsSkill: no material cards needed.
    if (vas->objectName() != u"choose_player"_s) {
        if (cardIds.isEmpty())
            return nullptr;
        for (const QVariant &id : cardIds) {
            const Card *c = ClientInstance->getCard(id.toInt());
            if (c != nullptr)
                cards.append(c);
        }
    }

    return vas->viewAs(cards);
}

QStringList RoomScene::enabledTargetsForCard(const Card *card, const QStringList &selectedTargetNames) const
{
    QStringList result;
    if (card == nullptr || Self == nullptr)
        return result;

    Client *client = ClientInstance;
    if (client == nullptr)
        return result;

    // Build already-selected targets list from names. targetFilter semantics: query
    // feasibility of to_select given the currently-selected targets.
    QList<const Player *> selected;
    selected.reserve(selectedTargetNames.size());
    for (const QString &name : selectedTargetNames) {
        ClientPlayer *p = client->getPlayer(name);
        if (p != nullptr)
            selected.append(p);
    }

    const QList<ClientPlayer *> players = client->getPlayers();
    for (const ClientPlayer *player : players) {
        if (player == Self)
            continue;
        int maxVotes = 0;
        card->targetFilter(selected, player, Self, maxVotes);
        if (maxVotes > 0)
            result.append(player->objectName());
    }
    return result;
}

QStringList RoomScene::enabledTargets(int cardId, const QStringList &selectedTargetNames) const
{
    const Card *card = ClientInstance->getCard(cardId);
    return enabledTargetsForCard(card, selectedTargetNames);
}

bool RoomScene::isCardTargetsFeasible(const Card *card, const QStringList &selectedTargetNames) const
{
    if (card == nullptr || Self == nullptr)
        return false;

    Client *client = ClientInstance;
    if (client == nullptr)
        return false;

    QList<const Player *> selected;
    selected.reserve(selectedTargetNames.size());
    for (const QString &name : selectedTargetNames) {
        ClientPlayer *p = client->getPlayer(name);
        if (p != nullptr)
            selected.append(p);
    }
    return card->targetsFeasible(selected, Self);
}

// ---- Phase 3: submit ----

void RoomScene::respondCard(const Card *card)
{
    Client *c = ClientInstance;
    if (c == nullptr) {
        qWarning() << "RoomScene::respondCard: ClientInstance is null";
        return;
    }

    QList<const Player *> targets;
    for (const QString &name : m_selectedTargets) {
        if (Player *p = c->getPlayer(name))
            targets.append(p);
    }

    c->onPlayerResponseCard(card, targets.isEmpty() ? QList<const Player *>() : targets);
}

void RoomScene::discardCard(const Card *card)
{
    Client *c = ClientInstance;
    if (c == nullptr) {
        qWarning() << "RoomScene::discardCard: ClientInstance is null";
        return;
    }
    c->onPlayerDiscardCards(card);
}

void RoomScene::submitCardResponse(const QVariantList &cardIds)
{
    Client *c = ClientInstance;
    if (c == nullptr) {
        qWarning() << "RoomScene::submitCardResponse: ClientInstance is null";
        return;
    }

    const Card *responseCard = nullptr;
    if (m_currentViewAsSkill != nullptr && !cardIds.isEmpty()) {
        QList<const Card *> cards;
        for (const QVariant &id : cardIds) {
            const Card *card = ClientInstance->getCard(id.toInt());
            if (card != nullptr)
                cards.append(card);
        }
        responseCard = m_currentViewAsSkill->viewAs(cards);
    } else if (!cardIds.isEmpty()) {
        responseCard = ClientInstance->getCard(cardIds.first().toInt());
    }

    QList<const Player *> targets;
    for (const QString &name : m_selectedTargets) {
        if (Player *p = c->getPlayer(name))
            targets.append(p);
    }

    c->onPlayerResponseCard(responseCard, targets.isEmpty() ? QList<const Player *>() : targets);
}

void RoomScene::submitDiscard(const QVariantList &cardIds)
{
    Client *c = ClientInstance;
    if (c == nullptr) {
        qWarning() << "RoomScene::submitDiscard: ClientInstance is null";
        return;
    }

    const Card *discardCard = nullptr;
    Client::Status status = c->getStatus();
    Client::Status basic = static_cast<Client::Status>(status & Client::ClientStatusBasicMask);
    if ((basic == Client::Discarding || basic == Client::Exchanging) && !cardIds.isEmpty()) {
        QList<const Card *> cards;
        for (const QVariant &id : cardIds) {
            const Card *card = ClientInstance->getCard(id.toInt());
            if (card != nullptr)
                cards.append(card);
        }
        discardCard = m_discardSkill->viewAs(cards);
    } else if (!cardIds.isEmpty()) {
        discardCard = ClientInstance->getCard(cardIds.first().toInt());
    }

    c->onPlayerDiscardCards(discardCard);
}

void RoomScene::respondToSkillInvoke(bool invoke)
{
    Client *c = ClientInstance;
    if (c == nullptr) {
        qWarning() << "RoomScene::respondToSkillInvoke: ClientInstance is null";
        return;
    }
    c->onPlayerInvokeSkill(invoke);
}

void RoomScene::finishPlayPhase()
{
    Client *c = ClientInstance;
    if (c == nullptr) {
        qWarning() << "RoomScene::finishPlayPhase: ClientInstance is null";
        return;
    }
    c->onPlayerResponseCard(nullptr);
}

void RoomScene::cancelResponse(int status)
{
    Client *c = ClientInstance;
    if (c == nullptr) {
        qWarning() << "RoomScene::cancelResponse: ClientInstance is null";
        return;
    }

    Client::Status s = static_cast<Client::Status>(status);
    Client::Status basic = static_cast<Client::Status>(s & Client::ClientStatusBasicMask);

    switch (basic) {
    case Client::Responding:
    case Client::AskForShowOrPindian:
        c->onPlayerResponseCard(nullptr);
        break;
    case Client::Discarding:
    case Client::Exchanging:
        c->onPlayerDiscardCards(nullptr);
        break;
    case Client::AskForSkillInvoke:
        c->onPlayerInvokeSkill(false);
        break;
    case Client::AskForPlayerChoose:
        c->onPlayerChoosePlayer(nullptr);
        break;
    default:
        break;
    }
}

// ---- Internal: aux-skill auto-configuration ----

void RoomScene::updateAuxSkill()
{
    Client *client = ClientInstance;
    if (client == nullptr) {
        m_currentViewAsSkill = nullptr;
        return;
    }

    Client::Status status = client->getStatus();
    Client::Status basic = static_cast<Client::Status>(status & Client::ClientStatusBasicMask);
    QString pattern = client->getRoomState()->getCurrentCardUsePattern();

    m_currentViewAsSkill = nullptr;

    switch (basic) {
    case Client::Responding:
    case Client::Playing: {
        if (pattern.startsWith(u"@@"_s) && !pattern.startsWith(u"@@sp_convert!"_s)) {
            // Real skill: let QML drive via skillViewFilter / skillViewAs / respondCard.
            break;
        }
        if (!pattern.isEmpty()) {
            // Card pattern: "slash", "peach", "jink" -- use ResponseSkill.
            QString p = pattern;
            if (p.endsWith(u'!'))
                p.chop(1);
            m_responseSkill->setPattern(p);
            m_currentViewAsSkill = m_responseSkill;
        }
        break;
    }
    case Client::Discarding:
    case Client::Exchanging:
        m_discardSkill->setNum(client->discard_num);
        m_discardSkill->setMinNum(client->min_num);
        m_discardSkill->setIncludeEquip(client->m_canDiscardEquip);
        m_discardSkill->setIsDiscard(basic != Client::Exchanging);
        m_currentViewAsSkill = m_discardSkill;
        break;
    case Client::AskForShowOrPindian: {
        m_showOrPindianSkill->setPattern(pattern);
        m_currentViewAsSkill = m_showOrPindianSkill;
        break;
    }
    case Client::AskForPlayerChoose:
        m_choosePlayerSkill->setPlayerNames(client->players_to_choose);
        m_currentViewAsSkill = m_choosePlayerSkill;
        break;
    default:
        break;
    }
}

namespace {
// Mirror old QSanSkillButton::setSkill (qsanbutton.cpp:279-338) skill-type deduction.
[[nodiscard]] QString skillTypeString(const Skill *skill)
{
    const bool is_optional_trigger = skill->inherits("TriggerSkill") && !skill->isEquipSkill() && ViewAsSkill::parseViewAsSkill(skill) == nullptr && !skill->isCompulsory()
        && !skill->isLimited() && !skill->isWake() && !skill->isEternal();
    const bool is_passive_modifier = skill->inherits("ProhibitSkill") || skill->inherits("DistanceSkill") || skill->inherits("MaxCardsSkill") || skill->inherits("TargetModSkill")
        || skill->inherits("AttackRangeSkill") || skill->inherits("ViewHasSkill");

    if (skill->inherits("BattleArraySkill"))
        return u"array"_s;
    if (skill->isWake())
        return u"awaken"_s;
    if (skill->isLimited())
        return skill->isAttachedLordSkill() ? u"attachedlord"_s : u"oneoff"_s;
    if (skill->isFrequent() || is_optional_trigger)
        return u"frequent"_s;
    if (skill->isCompulsory() || skill->isEternal() || is_passive_modifier)
        return u"compulsory"_s;
    return skill->isAttachedLordSkill() ? u"attachedlord"_s : u"proactive"_s;
}
} // namespace

QVariantList RoomScene::getPlayerSkillButtons(ClientPlayer *player) const
{
    QVariantList result;
    if (player == nullptr)
        return result;

    const bool isLord = (player->getRole() == u"lord"_s);
    const QList<const Skill *> skills = player->getVisibleSkillList(false);
    for (const Skill *skill : skills) {
        if (skill == nullptr)
            continue;
        // filter lord skill for non-lord (mirror old updateSkillButtons)
        if (skill->isLordSkill() && !isLord)
            continue;

        const ViewAsSkill *vas = ViewAsSkill::parseViewAsSkill(skill);
        QVariantMap m {
            {u"skillName"_s, skill->objectName()},
            {u"skillType"_s, skillTypeString(skill)},
            {u"translatedName"_s, Sanguosha->translate(skill->objectName())},
            {u"description"_s, skill->getDescription(true, false)},
            {u"viewAsSkillName"_s, vas != nullptr ? vas->objectName() : QString()},
        };
        result.append(m);
    }
    return result;
}

bool RoomScene::okEnabled() const
{
    return m_okEnabled;
}

bool RoomScene::cancelEnabled() const
{
    return m_cancelEnabled;
}

bool RoomScene::discardEnabled() const
{
    return m_discardEnabled;
}

void RoomScene::selectCard(const QVariantList &selectedCardIds)
{
    m_selectedCardIds = selectedCardIds;
    m_selectedTargets.clear();
    m_enabledTargetNames.clear();
    m_targetSelectionActive = false;
    syncPhotoTargets();

    if (selectedCardIds.isEmpty() || ClientInstance == nullptr || Self == nullptr) {
        m_selectedCard = nullptr;
        recomputeOkReadiness();
        return;
    }

    m_selectedCard = ClientInstance->getCard(selectedCardIds.first().toInt());
    if (m_selectedCard == nullptr) {
        recomputeOkReadiness();
        return;
    }

    if (m_selectedCard->targetFixed(Self)) {
        recomputeOkReadiness();
        return;
    }

    const int basic = static_cast<int>(ClientInstance->getStatus()) & Client::ClientStatusBasicMask;
    if (basic != Client::Playing && basic != Client::Responding) {
        recomputeOkReadiness();
        return;
    }

    m_enabledTargetNames = enabledTargetsForCard(m_selectedCard, m_selectedTargets);
    m_targetSelectionActive = !m_enabledTargetNames.isEmpty();
    syncPhotoTargets();
    recomputeOkReadiness();
}

void RoomScene::toggleTarget(const QString &playerName)
{
    if (m_selectedCard == nullptr)
        return;
    if (m_selectedTargets.contains(playerName))
        m_selectedTargets.removeAll(playerName);
    else
        m_selectedTargets.append(playerName);
    syncPhotoTargets();
    recomputeOkReadiness();
}

void RoomScene::updateDashboardStatus()
{
    bool newCancel = false;
    bool newDiscard = false;

    if (ClientInstance != nullptr) {
        const int basic = static_cast<int>(ClientInstance->getStatus()) & Client::ClientStatusBasicMask;
        const bool refusable = ClientInstance->isDiscardActionRefusable();
        switch (basic) {
        case Client::Responding:
            newCancel = refusable;
            break;
        case Client::Playing:
            newDiscard = true;
            break;
        case Client::Discarding:
        case Client::Exchanging:
            newCancel = refusable;
            break;
        case Client::ExecDialog:
            newCancel = true;
            break;
        case Client::AskForSkillInvoke:
            newCancel = true;
            break;
        case Client::AskForPlayerChoose:
            newCancel = refusable;
            break;
        default:
            break;
        }

        // Clear selections on status change away from selection modes.
        if (basic != Client::Playing && basic != Client::Responding && basic != Client::Discarding && basic != Client::Exchanging && basic != Client::AskForShowOrPindian
            && basic != Client::AskForGeneralTaken) {
            m_selectedCard = nullptr;
            m_selectedCardIds.clear();
            m_selectedTargets.clear();
            m_enabledTargetNames.clear();
            m_targetSelectionActive = false;
            syncPhotoTargets();
        }
    }

    if (m_cancelEnabled != newCancel) {
        m_cancelEnabled = newCancel;
        emit cancelEnabledChanged();
    }
    if (m_discardEnabled != newDiscard) {
        m_discardEnabled = newDiscard;
        emit discardEnabledChanged();
    }
    recomputeOkReadiness();
}

void RoomScene::registerPhoto(Photo *photo)
{
    if (photo != nullptr && !m_photos.contains(photo))
        m_photos.append(photo);
}

void RoomScene::unregisterPhoto(Photo *photo)
{
    m_photos.removeAll(photo);
}

void RoomScene::recomputeOkReadiness()
{
    bool newOk = false;
    if (ClientInstance != nullptr) {
        const int s = static_cast<int>(ClientInstance->getStatus()) & Client::ClientStatusBasicMask;
        if (s == Client::AskForSkillInvoke) {
            newOk = true;
        } else if (s == Client::Playing || s == Client::Responding || s == Client::AskForShowOrPindian) {
            if (m_selectedCard != nullptr && Self != nullptr)
                newOk = m_selectedCard->targetFixed(Self) || isCardTargetsFeasible(m_selectedCard, m_selectedTargets);
        } else if (s == Client::Discarding || s == Client::Exchanging) {
            newOk = !m_selectedCardIds.isEmpty();
        }
        // AskForGeneralTaken: okEnabled driven by QML activeBox.canAccept binding, not here.
    }
    if (m_okEnabled != newOk) {
        m_okEnabled = newOk;
        emit okEnabledChanged();
    }
}

void RoomScene::syncPhotoTargets()
{
    for (const QPointer<Photo> &p : m_photos) {
        if (p == nullptr)
            continue;
        const QString name = p->playerName();
        const bool can = m_targetSelectionActive && m_enabledTargetNames.contains(name);
        p->setTargetable(can);
        p->setTargetSelected(m_selectedTargets.contains(name));
    }
}

namespace {
void registerRoomScene()
{
    int ret = qmlRegisterType<RoomScene>("rocks.touhousatsu", 1, 0, "CppRoomScene");

    if (ret == -1)
        qDebug() << "Failed to register RoomScene to Qml";

    ret = qmlRegisterType<Photo>("rocks.touhousatsu", 1, 0, "CppPhoto");

    if (ret == -1)
        qDebug() << "Failed to register CppPhoto to Qml";
}
} // namespace

// NOLINTNEXTLINE
Q_COREAPP_STARTUP_FUNCTION(registerRoomScene);
