#include "roomscene.h"

#include "client.h"
#include "clientplayer.h"
#include "protocol.h"
#include "util.h"

#include <QApplication>
#include <QtQml>

QPointer<RoomScene> RoomSceneInstance;

RoomScene::RoomScene(QQuickItem *parent)
    : QQuickItem(parent)
    , gameStarted(false)
    , gameOver(false)
    , m_clientConnected(false)
{
    RoomSceneInstance = this;

    // RoomScene is created by QML when entering room scene, at which point
    // ClientInstance / Self are guaranteed to be valid. Connect immediately.
    connectClientSignals();
}

RoomScene::~RoomScene() = default;

QObject *RoomScene::selfHelper() const
{
    return Self;
}

QObject *RoomScene::clientHelper() const
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
        emit notifyGameStarted();
    });
    connect(client, &Client::game_over, this, [this]() {
        emit notifyGameOver();
    });
    connect(client, &Client::standoff, this, [this]() {
        emit notifyStandoff();
    });
    connect(client, &Client::player_added, this, [this](ClientPlayer *new_player) {
        emit notifyPlayerAdded(new_player);
    });
    connect(client, &Client::player_removed, this, [this](const QString &player_name) {
        emit notifyPlayerRemoved(player_name);
    });
    connect(client, &Client::seats_arranged, this, [this](const QList<const ClientPlayer *> &) {
        // Seats list is not forwarded: QML can query ClientInstance.getPlayers() directly.
        emit notifySeatsArranged();
    });
    connect(client, &Client::status_changed, this, [this](Client::Status oldStatus, Client::Status newStatus) {
        emit notifyStatusChanged(static_cast<int>(oldStatus), static_cast<int>(newStatus));
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
    connect(client, &Client::perspective_changed, this, [this](const QString &targetName, const QList<int> &handCardIds, const QVariantMap &piles) {
        emit notifyPerspectiveChanged(targetName, IntList2VariantList(handCardIds), piles);
    });
    connect(client, &Client::role_state_changed, this, [this](const QString &state_str) {
        emit notifyRoleStateChanged(state_str);
    });
    connect(client, &Client::generals_viewed, this, [this](const QString &reason, const QStringList &names) {
        emit notifyGeneralsViewed(reason, names);
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

namespace {
void registerRoomScene()
{
    int ret = qmlRegisterType<RoomScene>("rocks.touhousatsu", 1, 0, "CppRoomScene");

    if (ret == -1)
        qDebug() << "Failed to register RoomScene to Qml";
}
} // namespace

// NOLINTNEXTLINE
Q_COREAPP_STARTUP_FUNCTION(registerRoomScene);
