#include "client.h"
#include "CardFace.h"
#include "SkinBank.h"
#include "audio.h"
#include "choosegeneraldialog.h"
#include "engine.h"
#include "general.h"
#include "legacysocket.h"
#include "recorder.h"
#include "settings.h"
#include "uiUtils.h"
#include "util.h"

#include <QApplication>
#include <QCheckBox>
#include <QCommandLinkButton>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QMutex>
#include <QRandomGenerator>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QTextCursor>
#include <QTextDocument>
#include <QTimer>
#include <QVBoxLayout>
#include <chrono>

using namespace std::chrono_literals;

using namespace std;
using namespace QSanProtocol;
using namespace QSgsJsonUtils;

Client::Client(QObject *parent, const QString &filename)
    : RoomObject(parent)
    , m_isDiscardActionRefusable(true)
    , status(NotActive)
    , alive_count(1)
    , swap_pile(0)
    , heartbeatTimer(nullptr)
    , m_isObjectNameRecorded(false)
    , Self(nullptr)
{
    m_isGameOver = false;

    m_callbacks[S_COMMAND_CHECK_VERSION] = &Client::checkVersion;
    m_callbacks[S_COMMAND_SETUP_LEGACY] = &Client::legacySetup;
    m_callbacks[S_COMMAND_SETUP] = &Client::setup;
    m_callbacks[S_COMMAND_NETWORK_DELAY_TEST] = &Client::networkDelayTest;
    m_callbacks[S_COMMAND_ADD_PLAYER] = &Client::addPlayer;
    m_callbacks[S_COMMAND_REMOVE_PLAYER] = &Client::removePlayer;
    m_callbacks[S_COMMAND_START_IN_X_SECONDS] = &Client::startInXs;
    m_callbacks[S_COMMAND_ARRANGE_SEATS] = &Client::arrangeSeats;
    m_callbacks[S_COMMAND_WARN] = &Client::warn;
    m_callbacks[S_COMMAND_SPEAK] = &Client::speak;
    m_callbacks[S_COMMAND_HEARTBEAT] = &Client::heartbeat;

    m_callbacks[S_COMMAND_GAME_START] = &Client::startGame;
    m_callbacks[S_COMMAND_GAME_OVER] = &Client::gameOver;

    m_callbacks[S_COMMAND_CHANGE_HP] = &Client::hpChange;
    m_callbacks[S_COMMAND_CHANGE_MAXHP] = &Client::maxhpChange;
    m_callbacks[S_COMMAND_KILL_PLAYER] = &Client::killPlayer;
    m_callbacks[S_COMMAND_REVIVE_PLAYER] = &Client::revivePlayer;
    m_callbacks[S_COMMAND_SHOW_CARD] = &Client::showCard;
    m_callbacks[S_COMMAND_UPDATE_CARD] = &Client::updateCard;
    m_callbacks[S_COMMAND_SET_MARK] = &Client::setMark;
    m_callbacks[S_COMMAND_LOG_SKILL] = &Client::log;
    m_callbacks[S_COMMAND_ATTACH_SKILL] = &Client::attachSkill;
    m_callbacks[S_COMMAND_MOVE_FOCUS] = &Client::moveFocus;
    m_callbacks[S_COMMAND_SET_EMOTION] = &Client::setEmotion;
    m_callbacks[S_COMMAND_INVOKE_SKILL] = &Client::skillInvoked;
    m_callbacks[S_COMMAND_SHOW_ALL_CARDS] = &Client::showAllCards;
    m_callbacks[S_COMMAND_SKILL_GONGXIN] = &Client::askForGongxin;
    m_callbacks[S_COMMAND_LOG_EVENT] = &Client::handleGameEvent;
    m_callbacks[S_COMMAND_ADD_HISTORY] = &Client::addHistory;
    m_callbacks[S_COMMAND_ANIMATE] = &Client::animate;
    m_callbacks[S_COMMAND_FIXED_DISTANCE] = &Client::setFixedDistance;
    m_callbacks[S_COMMAND_CARD_LIMITATION] = &Client::cardLimitation;
    m_callbacks[S_COMMAND_DISABLE_SHOW] = &Client::disableShow;
    m_callbacks[S_COMMAND_NULLIFICATION_ASKED] = &Client::setNullification;
    m_callbacks[S_COMMAND_ENABLE_SURRENDER] = &Client::enableSurrender;
    m_callbacks[S_COMMAND_EXCHANGE_KNOWN_CARDS] = &Client::exchangeKnownCards;
    m_callbacks[S_COMMAND_SET_KNOWN_CARDS] = &Client::setKnownCards;
    m_callbacks[S_COMMAND_VIEW_GENERALS] = &Client::viewGenerals;
    m_callbacks[S_COMMAND_SET_DASHBOARD_SHADOW] = &Client::setDashboardShadow;

    m_callbacks[S_COMMAND_UPDATE_STATE_ITEM] = &Client::updateStateItem;
    m_callbacks[S_COMMAND_AVAILABLE_CARDS] = &Client::setAvailableCards;

    m_callbacks[S_COMMAND_GET_CARD] = &Client::getCards;
    m_callbacks[S_COMMAND_LOSE_CARD] = &Client::loseCards;
    m_callbacks[S_COMMAND_SET_PROPERTY] = &Client::updateProperty;
    m_callbacks[S_COMMAND_RESET_PILE] = &Client::resetPiles;
    m_callbacks[S_COMMAND_UPDATE_PILE] = &Client::setPileNumber;
    m_callbacks[S_COMMAND_SYNCHRONIZE_DISCARD_PILE] = &Client::synchronizeDiscardPile;
    m_callbacks[S_COMMAND_CARD_FLAG] = &Client::setCardFlag;
    m_callbacks[S_COMMAND_SET_SKILL_INVALIDITY] = &Client::setPlayerSkillInvalidity;
    m_callbacks[S_COMMAND_SET_SHOWN_HANDCARD] = &Client::setShownHandCards;
    m_callbacks[S_COMMAND_SET_BROKEN_EQUIP] = &Client::setBrokenEquips;
    m_callbacks[S_COMMAND_SET_HIDDEN_GENERAL] = &Client::setHiddenGenerals;
    m_callbacks[S_COMMAND_SET_SHOWN_HIDDEN_GENERAL] = &Client::setShownHiddenGeneral;

    // interactive methods
    m_interactions[S_COMMAND_CHOOSE_GENERAL] = &Client::askForGeneral;
    m_interactions[S_COMMAND_CHOOSE_PLAYER] = &Client::askForPlayerChosen;
    m_interactions[S_COMMAND_CHOOSE_ROLE] = &Client::askForAssign;
    m_interactions[S_COMMAND_CHOOSE_DIRECTION] = &Client::askForDirection;
    m_interactions[S_COMMAND_EXCHANGE_CARD] = &Client::askForExchange;
    m_interactions[S_COMMAND_ASK_PEACH] = &Client::askForSinglePeach;
    m_interactions[S_COMMAND_SKILL_GUANXING] = &Client::askForGuanxing;
    m_interactions[S_COMMAND_SKILL_GONGXIN] = &Client::askForGongxin;
    m_interactions[S_COMMAND_SKILL_YIJI] = &Client::askForYiji;
    m_interactions[S_COMMAND_PLAY_CARD] = &Client::activate;
    m_interactions[S_COMMAND_DISCARD_CARD] = &Client::askForDiscard;
    m_interactions[S_COMMAND_CHOOSE_SUIT] = &Client::askForSuit;
    m_interactions[S_COMMAND_CHOOSE_KINGDOM] = &Client::askForKingdom;
    m_interactions[S_COMMAND_RESPONSE_CARD] = &Client::askForCardOrUseCard;
    m_interactions[S_COMMAND_INVOKE_SKILL] = &Client::askForSkillInvoke;
    m_interactions[S_COMMAND_MULTIPLE_CHOICE] = &Client::askForChoice;
    m_interactions[S_COMMAND_NULLIFICATION] = &Client::askForNullification;
    m_interactions[S_COMMAND_SHOW_CARD] = &Client::askForCardShow;
    m_interactions[S_COMMAND_AMAZING_GRACE] = &Client::askForAG;
    m_interactions[S_COMMAND_PINDIAN] = &Client::askForPindian;
    m_interactions[S_COMMAND_CHOOSE_CARD] = &Client::askForCardChosen;
    m_interactions[S_COMMAND_CHOOSE_ORDER] = &Client::askForOrder;
    m_interactions[S_COMMAND_CHOOSE_ROLE_3V3] = &Client::askForRole3v3;
    m_interactions[S_COMMAND_SURRENDER] = &Client::askForSurrender;
    m_interactions[S_COMMAND_LUCK_CARD] = &Client::askForLuckCard;
    m_interactions[S_COMMAND_TRIGGER_ORDER] = &Client::askForTriggerOrder;

    m_callbacks[S_COMMAND_FILL_AMAZING_GRACE] = &Client::fillAG;
    m_callbacks[S_COMMAND_TAKE_AMAZING_GRACE] = &Client::takeAG;
    m_callbacks[S_COMMAND_CLEAR_AMAZING_GRACE] = &Client::clearAG;

    // 3v3 mode & 1v1 mode
    m_interactions[S_COMMAND_ASK_GENERAL] = &Client::askForGeneral3v3;
    m_interactions[S_COMMAND_ARRANGE_GENERAL] = &Client::startArrange;

    m_callbacks[S_COMMAND_FILL_GENERAL] = &Client::fillGenerals;
    m_callbacks[S_COMMAND_TAKE_GENERAL] = &Client::takeGeneral;
    m_callbacks[S_COMMAND_RECOVER_GENERAL] = &Client::recoverGeneral;
    m_callbacks[S_COMMAND_REVEAL_GENERAL] = &Client::revealGeneral;

    m_noNullificationThisTime = false;
    m_noNullificationTrickName = QStringLiteral(".");

    Self = new Player(this);
    Self->setScreenName(Config.UserName);
    Self->setProperty("avatar", Config.UserAvatar);
    // connect(Self, &Player::phase_changed, this, &Client::alertFocus);
    // connect(Self, &Player::role_changed, this, &Client::notifyRoleChange);

    registerPlayer(Self);

    if (!filename.isEmpty()) {
        socket = nullptr;
        recorder = nullptr;

        replayer = new Replayer(this, filename);
        connect(replayer, SIGNAL(command_parsed(QString)), this, SLOT(processServerPacket(QString)));
    } else {
        socket = new LegacyClientSocket;
        socket->setParent(this);

        recorder = new Recorder(this);

        connect(socket, &LegacyClientSocket::message_got, recorder, &Recorder::record);
        connect(socket, SIGNAL(message_got(const char *)), this, SLOT(processServerPacket(const char *)));
        connect(socket, &LegacyClientSocket::error_message, this, &Client::error_message);
        socket->connectToHost();

        replayer = nullptr;
    }

    lines_doc = new QTextDocument(this);

    prompt_doc = new QTextDocument(this);
    prompt_doc->setTextWidth(350);
#ifdef Q_OS_LINUX
    prompt_doc->setDefaultFont(QFont(QStringLiteral("DroidSansFallback")));
#else
    prompt_doc->setDefaultFont(QFont(QStringLiteral("SimHei")));
#endif
}

void Client::updateCard(const QJsonValue &val)
{
    if (QSgsJsonUtils::isNumber(val)) {
        // reset card
        int cardId = val.toInt();
        Card *card_ = card(cardId);
        // TODO: How to handle the Filter skill?
        if (!card_->isModified())
            return;
        resetCard(cardId);
    } else {
        // update card
        QJsonArray args = val.toArray();
        Q_ASSERT(args.size() >= 5);
        int cardId = args[0].toInt();
        QSanguosha::Suit suit = (QSanguosha::Suit)args[1].toInt();
        QSanguosha::Number number = static_cast<QSanguosha::Number>(args[2].toInt());
        QString cardName = args[3].toString();
        QString skillName = args[4].toString();
        // QString objectName = args[5].toString();
        QStringList flags;
        QSgsJsonUtils::tryParse(args[6], flags);

        const CardFace *face = Sanguosha->cardFace(cardName);
        // since we are modifying actual card, the Face must not be nullptr
        if (face != nullptr) {
            Card *c = card(cardId);
            c->setSuit(suit);
            c->setNumber(number);
            c->setSkillName(skillName);
            c->setFace(face);
        }
        // TODO: Figure out the objectName here.
        //card->setObjectName(objectName);
    }
}

void Client::setPlayerSkillInvalidity(const QJsonValue &arg)
{
    QJsonArray a = arg.toArray();
    if (a.count() == 3) {
        QString playerName = a.first().toString();
        Player *player = findPlayer(playerName);
        if (player == nullptr)
            return;

        QString skill_name = a.at(1).toString();
        bool invalid = a.at(2).toBool();

        player->setSkillInvalidity(skill_name, invalid);
        emit skill_invalidity_changed(player);
    }
}

void Client::setShownHandCards(const QJsonValue &card_var)
{
    QJsonArray card_str = card_var.toArray();
    if (card_str.size() != 2)
        return;
    if (!QSgsJsonUtils::isString(card_str[0]))
        return;

    QString who = card_str[0].toString();
    QList<int> card_ids;
    QSgsJsonUtils::tryParse(card_str[1], card_ids);

    Player *player = findPlayer(who);
    player->setShownHandcards(IdSet(card_ids.begin(), card_ids.end()));
    //  player->changePile(QStringLiteral("shown_card"), true, card_ids);
}

void Client::setBrokenEquips(const QJsonValue &card_var)
{
    QJsonArray card_str = card_var.toArray();
    if (card_str.size() != 2)
        return;
    if (!QSgsJsonUtils::isString(card_str[0]))
        return;

    QString who = card_str[0].toString();
    QList<int> card_ids;
    QSgsJsonUtils::tryParse(card_str[1], card_ids);

    Player *player = findPlayer(who);

    player->setBrokenEquips(IdSet(card_ids.begin(), card_ids.end()));
}

void Client::setHiddenGenerals(const QJsonValue &arg)
{
    QJsonArray str = arg.toArray();
    if (str.size() != 2)
        return;

    QString who = str[0].toString();
    QStringList names;
    if (QSgsJsonUtils::isString(str[1])) {
        QString generalString = str[1].toString();
        names = generalString.split(QStringLiteral("|"));
    } else {
        int n = str[1].toInt();
        while (n-- > 0)
            names << QStringLiteral("sujiangf");
    }
    Player *player = findPlayer(who);
    // player->setHiddenGenerals(names);
    // player->changePile(QStringLiteral("huashencard"), false, QList<int>());
}

void Client::setShownHiddenGeneral(const QJsonValue &arg)
{
    QJsonArray str = arg.toArray();
    if (str.size() != 2)
        return;

    QString who = str[0].toString();
    QString general = str[1].toString();

    Player *player = findPlayer(who);
}

void Client::signup()
{
    if (replayer != nullptr)
        replayer->start();
    else {
        QJsonArray arg;
        arg << QUrl(Config.HostAddress).path();
        arg << QString::fromUtf8(Config.UserName.toUtf8().toBase64());
        arg << Config.UserAvatar;
        notifyServer(S_COMMAND_SIGNUP, arg);
    }
}

void Client::networkDelayTest(const QJsonValue & /*unused*/)
{
    notifyServer(S_COMMAND_NETWORK_DELAY_TEST);
}

void Client::replyToServer(CommandType command, const QJsonValue &arg)
{
    if (socket != nullptr) {
        Packet packet(PacketDescriptionFlag(S_SRC_CLIENT) | S_TYPE_REPLY | S_DEST_ROOM, command);
        packet.setMessageBody(arg);
        socket->send(QString::fromUtf8(packet.toJson()));
    }
}

void Client::handleGameEvent(const QJsonValue &arg)
{
    emit event_received(arg);
}

void Client::notifyServer(CommandType command, const QJsonValue &arg)
{
    if (socket != nullptr) {
        Packet packet(PacketDescriptionFlag(S_SRC_CLIENT) | S_TYPE_NOTIFICATION | S_DEST_ROOM, command);
        packet.setMessageBody(arg);
        socket->send(QString::fromUtf8(packet.toJson()));
    }
}

void Client::requestServer(CommandType command, const QJsonValue &arg)
{
    if (socket != nullptr) {
        Packet packet(PacketDescriptionFlag(S_SRC_CLIENT) | S_TYPE_REQUEST | S_DEST_ROOM, command);
        packet.setMessageBody(arg);
        socket->send(QString::fromUtf8(packet.toJson()));
    }
}

void Client::checkVersion(const QJsonValue &server_version)
{
    QString version = server_version.toString();
    QString version_number;
    QString mod_name;
    if (version.contains(QLatin1Char(':'))) {
        QStringList texts = version.split(QLatin1Char(':'));
        version_number = texts.value(0);
        mod_name = texts.value(1);
    } else {
        version_number = version;
        mod_name = QStringLiteral("official");
    }

    emit version_checked(version_number, mod_name);
}

void Client::legacySetup(const QJsonValue &setup_json)
{
    if ((socket != nullptr) && !socket->isConnected())
        return;

    QString setup_str = setup_json.toString();

    if (ServerInfo.parseLegacy(setup_str)) {
        emit server_connected();

        heartbeatTimer = new QTimer(this);
        connect(heartbeatTimer, &QTimer::timeout, [this]() -> void {
            notifyServer(S_COMMAND_HEARTBEAT);
        });
        heartbeatTimer->setSingleShot(false);
        heartbeatTimer->setInterval(1min);
        heartbeatTimer->start();

        notifyServer(S_COMMAND_TOGGLE_READY);
    } else {
        QMessageBox::warning(nullptr, tr("Warning"), tr("Setup string can not be parsed: %1").arg(setup_str));
    }
}

void Client::setup(const QJsonValue &setup_str)
{
    if ((socket != nullptr) && !socket->isConnected())
        return;

    if (ServerInfo.parse(setup_str)) {
        emit server_connected();

        heartbeatTimer = new QTimer(this);
        connect(heartbeatTimer, &QTimer::timeout, [this]() -> void {
            notifyServer(S_COMMAND_HEARTBEAT);
        });
        heartbeatTimer->setSingleShot(false);
        heartbeatTimer->setInterval(1min);
        heartbeatTimer->start();

        notifyServer(S_COMMAND_TOGGLE_READY);
    } else {
        QMessageBox::warning(nullptr, tr("Warning"), tr("Setup string can not be parsed"));
    }
}

void Client::disconnectFromHost()
{
    if (socket != nullptr) {
        socket->disconnectFromHost();
        socket = nullptr;
    }
}

void Client::processServerPacket(const QString &cmd)
{
    processServerPacket(cmd.toLatin1().data());
}

void Client::processServerPacket(const char *cmd)
{
    if (m_isGameOver)
        return;
    Packet packet;
    if (packet.parse(cmd)) {
        if (packet.getPacketType() == S_TYPE_NOTIFICATION) {
            Callback callback = m_callbacks[packet.getCommandType()];
            if (callback != nullptr) {
                (this->*callback)(packet.getMessageBody());
            }
        } else if (packet.getPacketType() == S_TYPE_REQUEST) {
            if (replayer == nullptr)
                processServerRequest(packet);
            else if (packet.getCommandType() == QSanProtocol::S_COMMAND_CHOOSE_GENERAL) {
                if (isHegemonyGameMode(ServerInfo.GameModeStr) && ServerInfo.isMultiGeneralEnabled()) {
                    Callback callback = m_interactions[S_COMMAND_CHOOSE_GENERAL];
                    if (callback != nullptr)
                        (this->*callback)(packet.getMessageBody());
                } else
                    processShowGeneral(packet);
            }
        }
    }
}

bool Client::processServerRequest(const Packet &packet)
{
    setStatus(NotActive);
    CommandType command = packet.getCommandType();
    QJsonValue msg = packet.getMessageBody();

    Callback callback = m_interactions[command];
    if (callback == nullptr)
        return false;
    (this->*callback)(msg);
    return true;
}

void Client::processShowGeneral(const Packet &packet)
{
    const QJsonValue &arg = packet.getMessageBody();
    QStringList names;
    if (!QSgsJsonUtils::tryParse(arg, names))
        return;

    emit generals_viewed(QStringLiteral("View Generals"), names);
}

void Client::addPlayer(const QJsonValue &player_info)
{
    if (!player_info.isArray())
        return;

    QJsonArray info = player_info.toArray();
    if (info.size() < 3)
        return;

    QString name = info[0].toString();
    QString screen_name = QString::fromUtf8(QByteArray::fromBase64(info[1].toString().toLatin1()));
    QString avatar = info[2].toString();

    Player *player = new Player(this);
    player->setObjectName(name);
    player->setScreenName(screen_name);
    player->setProperty("avatar", avatar);

    registerPlayer(player);
    alive_count++;
    emit player_added(player);
}

void Client::updateProperty(const QJsonValue &arg)
{
    QJsonArray args = arg.toArray();
    if (!QSgsJsonUtils::isStringArray(args, 0, 2))
        return;
    QString object_name = args[0].toString();
    Player *player = findPlayer(object_name);
    if (player == nullptr)
        return;
    player->setProperty(args[1].toString().toLatin1().constData(), args[2].toString());
}

void Client::removePlayer(const QJsonValue &player_name)
{
    QString name = player_name.toString();
    Player *player = findChild<Player *>(name);
    if (player != nullptr) {
        player->setParent(nullptr);
        alive_count--;
        emit player_removed(name);
        unregisterPlayer(player);
        delete player;
    }
}

bool Client::_loseSingleCard(int card_id, const LegacyCardsMoveStruct &move)
{
    const Card *card_ = card(card_id);
    if (move.from != nullptr)
        move.from->removeCard(card_, move.from_place);
    else {
        if (move.from_place == QSanguosha::PlaceDiscardPile)
            discardPile().removeOne(card_id);
        else if (move.from_place == QSanguosha::PlaceDrawPile && !Self->hasFlag(QStringLiteral("marshalling")))
            pile_num--;
    }
    return true;
}

bool Client::_getSingleCard(int card_id, const LegacyCardsMoveStruct &move)
{
    const Card *card_ = card(card_id);
    if (move.to != nullptr)
        move.to->addCard(card_, move.to_place, move.to_pile_name);
    else {
        if (move.to_place == QSanguosha::PlaceDrawPile)
            pile_num++;
        else if (move.to_place == QSanguosha::PlaceDiscardPile)
            discardPile().prepend(card_id);
    }
    return true;
}

void Client::getCards(const QJsonValue &arg)
{
    QJsonArray args = arg.toArray();
    Q_ASSERT(!args.empty());
    int moveId = args[0].toInt();
    QList<LegacyCardsMoveStruct> moves;
    for (int i = 1; i < args.count(); i++) {
        LegacyCardsMoveStruct move;
        if (!move.tryParse(args[i]))
            return;
        move.from = findPlayer(move.from_player_name);
        move.to = findPlayer(move.to_player_name);
        foreach (int card_id, move.card_ids)
            _getSingleCard(card_id, move);

        moves.append(move);
    }
    updatePileNum();
    emit move_cards_got(moveId, moves);
}

void Client::loseCards(const QJsonValue &arg)
{
    QJsonArray args = arg.toArray();
    Q_ASSERT(!args.empty());
    int moveId = args[0].toInt();
    QList<LegacyCardsMoveStruct> moves;
    for (int i = 1; i < args.count(); i++) {
        LegacyCardsMoveStruct move;
        if (!move.tryParse(args[i]))
            return;
        move.from = findPlayer(move.from_player_name);
        move.to = findPlayer(move.to_player_name);

        foreach (int card_id, move.card_ids)
            _loseSingleCard(card_id, move);

        moves.append(move);
    }
    updatePileNum();
    emit move_cards_lost(moveId, moves);
}

void Client::onPlayerChooseGeneral(const QString &item_name)
{
    setStatus(NotActive);
    if (!item_name.isEmpty()) {
        replyToServer(S_COMMAND_CHOOSE_GENERAL, item_name);
        Audio::playSystemAudioEffect(QStringLiteral("choose-item"));
    }
}

void Client::requestCheatRunScript(const QString &script)
{
    if (getStatus() != Playing)
        return;

    QJsonArray cheatReq;
    cheatReq << (int)S_CHEAT_RUN_SCRIPT;
    cheatReq << script;
    QJsonDocument doc(cheatReq);
    QString cheatStr = QString::fromUtf8(doc.toJson());
    Card *card = cloneCard(QStringLiteral("CheatCard"));
    card->setHandleMethod(QSanguosha::MethodNone);
    card->setUserString(cheatStr);
    onPlayerResponseCard(card);
}

void Client::requestCheatRevive(const QString &name)
{
    if (getStatus() != Playing)
        return;

    QJsonArray cheatReq;
    cheatReq << (int)S_CHEAT_REVIVE_PLAYER;
    cheatReq << name;
    QJsonDocument doc(cheatReq);
    QString cheatStr = QString::fromUtf8(doc.toJson());
    Card *card = cloneCard(QStringLiteral("CheatCard"));
    card->setHandleMethod(QSanguosha::MethodNone);
    onPlayerResponseCard(card);
}

void Client::requestCheatDamage(const QString &source, const QString &target, QSanguosha::DamageNature nature, int points)
{
    if (getStatus() != Playing)
        return;

    QJsonArray cheatReq;
    QJsonArray cheatArg;
    cheatArg << source;
    cheatArg << target;
    cheatArg << (int)nature;
    cheatArg << points;

    cheatReq << (int)S_CHEAT_MAKE_DAMAGE;
    cheatReq << cheatArg;
    QJsonDocument doc(cheatReq);
    QString cheatStr = QString::fromUtf8(doc.toJson());
    Card *card = cloneCard(QStringLiteral("CheatCard"));
    card->setUserString(cheatStr);
    card->setHandleMethod(QSanguosha::MethodNone);
    onPlayerResponseCard(card);
}

void Client::requestCheatKill(const QString &killer, const QString &victim)
{
    if (getStatus() != Playing)
        return;

    QJsonArray cheatReq;
    cheatReq << (int)S_CHEAT_KILL_PLAYER;
    cheatReq << (QJsonArray() << killer << victim);
    QJsonDocument doc(cheatReq);
    QString cheatStr = QString::fromUtf8(doc.toJson());
    Card *card = cloneCard(QStringLiteral("CheatCard"));
    card->setUserString(cheatStr);
    card->setHandleMethod(QSanguosha::MethodNone);
    onPlayerResponseCard(card);
}

void Client::requestCheatGetOneCard(int card_id)
{
    if (getStatus() != Playing)
        return;

    QJsonArray cheatReq;
    cheatReq << (int)S_CHEAT_GET_ONE_CARD;
    cheatReq << card_id;
    QJsonDocument doc(cheatReq);
    QString cheatStr = QString::fromUtf8(doc.toJson());
    Card *card = cloneCard(QStringLiteral("CheatCard"));
    card->setUserString(cheatStr);
    card->setHandleMethod(QSanguosha::MethodNone);
    onPlayerResponseCard(card);
}

void Client::requestCheatChangeGeneral(const QString &name, bool isSecondaryHero)
{
    if (getStatus() != Playing)
        return;

    QJsonArray cheatReq;
    cheatReq << (int)S_CHEAT_CHANGE_GENERAL;
    cheatReq << name;
    cheatReq << isSecondaryHero;
    QJsonDocument doc(cheatReq);
    QString cheatStr = QString::fromUtf8(doc.toJson());
    Card *card = cloneCard(QStringLiteral("CheatCard"));
    card->setUserString(cheatStr);
    card->setHandleMethod(QSanguosha::MethodNone);
    onPlayerResponseCard(card);
}

void Client::addRobot()
{
    notifyServer(S_COMMAND_ADD_ROBOT);
}

void Client::fillRobots()
{
    notifyServer(S_COMMAND_FILL_ROBOTS);
}

void Client::onPlayerResponseCard(const Card *card, const QList<const Player *> &targets)
{
    if (Self->hasFlag(QStringLiteral("Client_PreventPeach"))) {
        Self->setFlag(QStringLiteral("-Client_PreventPeach"));
        Self->removeCardLimitation(QStringLiteral("use"), QStringLiteral("Peach$0"), QStringLiteral("Global_PreventPeach"));
    }
    if ((status & ClientStatusBasicMask) == Responding)
        setCurrentCardUsePattern(QString());
    if (card == nullptr) {
        replyToServer(S_COMMAND_RESPONSE_CARD);
    } else {
        QJsonArray targetNames;
        if (!card->face()->targetFixed(Self, card)) {
            foreach (const Player *target, targets)
                targetNames << target->objectName();
        }

        replyToServer(S_COMMAND_RESPONSE_CARD, QJsonArray() << card->toString() << targetNames);

        // FIXME: When to recycle the card?
        if (card->isVirtualCard())
            // delete card;
            cardDeleting(card);
    }

    setStatus(NotActive);
}

void Client::startInXs(const QJsonValue &left_seconds)
{
    if (!m_isObjectNameRecorded) {
        m_isObjectNameRecorded = true;
        Config.setValue(QStringLiteral("LastSelfObjectName"), Self->objectName());
    }

    int seconds = left_seconds.toInt();
    if (seconds > 0)
        lines_doc->setHtml(tr("<p align = \"center\">Game will start in <b>%1</b> seconds...</p>").arg(seconds));
    else
        lines_doc->setHtml(QString());

    emit start_in_xs();
    if (seconds == 0) {
        emit avatars_hiden();
    }
}

void Client::arrangeSeats(const QJsonValue &seats_arr)
{
    QStringList player_names;
    if (seats_arr.isArray()) {
        QJsonArray seats = seats_arr.toArray();
        for (const QJsonValue &seat : seats)
            player_names << seat.toString();
    }

    arrangeSeat(player_names);

    QList<const Player *> seats;
    int self_index = players().indexOf(Self);

    Q_ASSERT(self_index != -1);

    for (int i = self_index + 1; i < players().length(); i++)
        seats.append(players().at(i));
    for (int i = 0; i < self_index; i++)
        seats.append(players().at(i));

    Q_ASSERT(seats.length() == players().length() - 1);

    emit seats_arranged(seats);
}

void Client::notifyRoleChange(const QString &new_role)
{
    if (isRoleGameMode(ServerInfo.GameModeStr) && !new_role.isEmpty()) {
        QString prompt_str = tr("Your role is %1").arg(Sanguosha->translate(new_role));
        if (new_role != QStringLiteral("lord"))
            prompt_str += tr("\n wait for the lord player choosing general, please");
        lines_doc->setHtml(QStringLiteral("<p align = \"center\">%1</p>").arg(prompt_str));
    }
}

void Client::activate(const QJsonValue &playerId)
{
    setStatus(playerId.toString() == Self->objectName() ? Playing : NotActive);
}

void Client::startGame(const QJsonValue &arg)
{
    resetAllCards();

    QJsonArray arr = arg.toArray();
    lord_name = arr[0].toString();

    alive_count = players().count();

    emit game_started();
}

void Client::hpChange(const QJsonValue &change_str)
{
    QJsonArray change = change_str.toArray();
    if (change.size() != 3)
        return;
    if (!QSgsJsonUtils::isString(change[0]) || !QSgsJsonUtils::isNumber(change[1]) || !QSgsJsonUtils::isNumber(change[2]))
        return;

    QString who = change[0].toString();
    int delta = change[1].toInt();

    int nature_index = change[2].toInt();
    QSanguosha::DamageNature nature = QSanguosha::DamageNormal;
    if (nature_index > 0)
        nature = (QSanguosha::DamageNature)nature_index;

    emit hp_changed(who, delta, nature, nature_index == -1);
}

void Client::maxhpChange(const QJsonValue &change_str)
{
    QJsonArray change = change_str.toArray();
    if (change.size() != 2)
        return;
    if (!QSgsJsonUtils::isString(change[0]) || !QSgsJsonUtils::isNumber(change[1]))
        return;

    QString who = change[0].toString();
    int delta = change[1].toInt();
    emit maxhp_changed(who, delta);
}

void Client::setStatus(Status status)
{
    Status old_status = this->status;
    this->status = status;
    if (status == Client::Playing)
        setCurrentCardUseReason(QSanguosha::CardUseReasonPlay);
    else if (status == Responding)
        setCurrentCardUseReason(QSanguosha::CardUseReasonResponse);
    else if (status == RespondingUse)
        setCurrentCardUseReason(QSanguosha::CardUseReasonResponseUse);
    else
        setCurrentCardUseReason(QSanguosha::CardUseReasonUnknown);

    emit status_changed(old_status, status);
}

Client::Status Client::getStatus() const
{
    return status;
}

void Client::cardLimitation(const QJsonValue &limit)
{
    QJsonArray args = limit.toArray();
    if (args.size() != 7)
        return;

    QString object_name = args[4].toString();
    Player *player = findPlayer(object_name);
    if (player == nullptr)
        return;

    bool set = args[0].toBool();
    bool single_turn = args[3].toBool();
    if (args[1].isNull() && args[2].isNull()) {
        player->clearCardLimitation(single_turn);
    } else {
        if (!QSgsJsonUtils::isString(args[1]) || !QSgsJsonUtils::isString(args[2]) || !QSgsJsonUtils::isString(args[5]))
            return;
        QString limit_list = args[1].toString();
        QString pattern = args[2].toString();
        QString reason = args[5].toString();
        bool clearReason = args[6].toBool();
        if (set)
            player->setCardLimitation(limit_list, pattern, reason, single_turn);
        else
            player->removeCardLimitation(limit_list, pattern, reason, clearReason);
    }
}

void Client::disableShow(const QJsonValue &arg)
{
    QJsonArray args = arg.toArray();
    if (args.size() != 4)
        return;

    Player *p = findPlayer(args[0].toString());
    if (p == nullptr)
        return;

    bool set = args[1].toBool();
    QString reason = args[3].toString();
    if (set) {
        QString flags = args[2].toString();
        p->setDisableShow(flags, reason);
    } else
        p->removeDisableShow(reason);
}

void Client::setNullification(const QJsonValue &str)
{
    if (!QSgsJsonUtils::isString(str))
        return;
    QString astr = str.toString();
    if (astr != QStringLiteral(".")) {
        if (m_noNullificationTrickName == QStringLiteral(".")) {
            m_noNullificationThisTime = false;
            m_noNullificationTrickName = astr;
            emit nullification_asked(true);
        }
    } else {
        m_noNullificationThisTime = false;
        m_noNullificationTrickName = QStringLiteral(".");
        emit nullification_asked(false);
    }
}

void Client::enableSurrender(const QJsonValue &enabled)
{
    if (!QSgsJsonUtils::isBool(enabled))
        return;
    bool en = enabled.toBool();
    emit surrender_enabled(en);
}

void Client::exchangeKnownCards(const QJsonValue &players)
{
    QJsonArray args = players.toArray();
    if (args.size() != 2 || !QSgsJsonUtils::isString(args[0]) || !QSgsJsonUtils::isString(args[1]))
        return;
    Player *a = findPlayer(args[0].toString());
    Player *b = findPlayer(args[1].toString());
    IdSet a_known;
    IdSet b_known;
    foreach (const Card *card, a->handCards())
        a_known << card->id();
    foreach (const Card *card, b->handCards())
        b_known << card->id();
    a->setHandCards(b_known);
    b->setHandCards(a_known);
}

void Client::setKnownCards(const QJsonValue &set_str)
{
    QJsonArray set = set_str.toArray();
    if (set.size() != 2)
        return;
    QString name = set[0].toString();
    Player *player = findPlayer(name);
    if (player == nullptr)
        return;
    QList<int> ids;
    QSgsJsonUtils::tryParse(set[1], ids);
    player->setHandCards(List2Set(ids));
}

void Client::viewGenerals(const QJsonValue &arg)
{
    QJsonArray args = arg.toArray();
    if (args.size() != 2 || !QSgsJsonUtils::isString(args[0]))
        return;
    QString reason = args[0].toString();
    QStringList names;
    if (!QSgsJsonUtils::tryParse(args[1], names))
        return;
    emit generals_viewed(reason, names);
}

Replayer *Client::getReplayer() const
{
    return replayer;
}

QString Client::getPlayerName(const QString &str)
{
    QRegularExpression rx(QRegularExpression::anchoredPattern(QStringLiteral("sgs\\d+")));
    QString general_name;
    if (rx.match(str).hasMatch()) {
        Player *player = findPlayer(str);
        general_name = player->generalName();
        general_name = Sanguosha->translate(general_name);
        if (player->getGeneral2() != nullptr)
            general_name.append(QStringLiteral("/") + Sanguosha->translate(player->getGeneral2Name()));

        if (isHegemonyGameMode(ServerInfo.GameModeStr)) {
            if (ServerInfo.isMultiGeneralEnabled()) {
                if (player->generalName() == QStringLiteral("anjiang") && player->getGeneral2() != nullptr && player->getGeneral2Name() == QStringLiteral("anjiang")) {
                    general_name = Sanguosha->translate(QStringLiteral("SEAT(%1)").arg(QString::number(player->seat())));
                }
            } else if (player->generalName() == QStringLiteral("anjiang")) {
                general_name = Sanguosha->translate(QStringLiteral("SEAT(%1)").arg(QString::number(player->seat())));
            }
        }

        return general_name;
    } else
        return Sanguosha->translate(str);
}

QString Client::getSkillNameToInvoke() const
{
    return skill_to_invoke;
}

void Client::onPlayerInvokeSkill(bool invoke)
{
    if (skill_name == QStringLiteral("surrender")) {
        replyToServer(S_COMMAND_SURRENDER, invoke);
        skill_name.clear();
    } else
        replyToServer(S_COMMAND_INVOKE_SKILL, invoke);
    setStatus(NotActive);
}

QString Client::setPromptList(const QStringList &texts)
{
    QString prompt = Sanguosha->translate(texts.at(0));
    if (texts.length() >= 2)
        prompt.replace(QStringLiteral("%src"), getPlayerName(texts.at(1)));

    if (texts.length() >= 3)
        prompt.replace(QStringLiteral("%dest"), getPlayerName(texts.at(2)));

    if (texts.length() >= 5) {
        QString arg2 = Sanguosha->translate(texts.at(4));
        prompt.replace(QStringLiteral("%arg2"), arg2);
    }

    if (texts.length() >= 4) {
        QString arg = Sanguosha->translate(texts.at(3));
        prompt.replace(QStringLiteral("%arg"), arg);
    }

    prompt_doc->setHtml(prompt);
    return prompt;
}

QString Client::_processCardPattern(const QString &pattern)
{
    const QChar c = pattern.at(pattern.length() - 1);
    if (c.toLatin1() == '!' || c.isNumber())
        return pattern.left(pattern.length() - 1);

    return pattern;
}

void Client::askForCardOrUseCard(const QJsonValue &cardUsage)
{
    QJsonArray usage = cardUsage.toArray();
    if (usage.size() < 2 || !QSgsJsonUtils::isString(usage[0]) || !QSgsJsonUtils::isString(usage[1]))
        return;
    QString card_pattern = usage[0].toString();
    setCurrentCardUsePattern(card_pattern);
    QString textsString = usage[1].toString();
    QStringList texts = textsString.split(QStringLiteral(":"));
    int index = usage[3].toInt();
    highlight_skill_name = usage.at(4).toString();

    if (texts.isEmpty())
        return;
    else
        setPromptList(texts);

    m_isDiscardActionRefusable = !card_pattern.endsWith(QStringLiteral("!"));

    QString temp_pattern = _processCardPattern(card_pattern);
    QRegularExpression rx(QRegularExpression::anchoredPattern(QStringLiteral("^@@?(\\w+)(-card)?$")));
    QRegularExpressionMatch match;
    if ((match = rx.match(temp_pattern)).hasMatch()) {
        QString skill_name = match.capturedTexts().at(1);
        QString text = prompt_doc->toHtml();
        text.append(tr("<br/> <b>Notice</b>: %1<br/>").arg(getSkillNotice(skill_name, index)));
        prompt_doc->setHtml(text);
    }

    Status status = Responding;
    if (usage.size() >= 3 && QSgsJsonUtils::isNumber(usage[2])) {
        QSanguosha::HandlingMethod method = (QSanguosha::HandlingMethod)(usage[2].toInt());
        switch (method) {
        case QSanguosha::MethodDiscard:
            status = RespondingForDiscard;
            break;
        case QSanguosha::MethodUse:
            status = RespondingUse;
            break;
        case QSanguosha::MethodResponse:
            status = Responding;
            break;
        default:
            status = RespondingNonTrigger;
            break;
        }
    }
    setStatus(status);
}

void Client::askForSkillInvoke(const QJsonValue &arg)
{
    QJsonArray args = arg.toArray();
    if (!QSgsJsonUtils::isStringArray(args, 0, 1))
        return;

    QString skill_name = args[0].toString();
    QString data = args[1].toString();

    skill_to_invoke = skill_name;
    highlight_skill_name = skill_name;

    QString text;
    if (data.isEmpty()) {
        text = tr("Do you want to invoke skill [%1] ?").arg(Sanguosha->translate(skill_name));
        prompt_doc->setHtml(text);
    } else if (data.startsWith(QStringLiteral("playerdata:"))) {
        QString name = getPlayerName(data.split(QStringLiteral(":")).last());
        text = tr("Do you want to invoke skill [%1] to %2 ?").arg(Sanguosha->translate(skill_name), name);
        prompt_doc->setHtml(text);
    } else if (skill_name.startsWith(QStringLiteral("cv_"))) {
        setPromptList(QStringList() << QStringLiteral("@sp_convert") << QString() << QString() << data);
    } else {
        QStringList texts = data.split(QStringLiteral(":"));
        text = QStringLiteral("%1:%2").arg(skill_name, texts.first());
        texts.replace(0, text);
        setPromptList(texts);
    }

    setStatus(AskForSkillInvoke);
}

void Client::onPlayerMakeChoice()
{
    QString option = sender()->objectName();
    replyToServer(S_COMMAND_MULTIPLE_CHOICE, option);
    setStatus(NotActive);
}

void Client::askForSurrender(const QJsonValue &initiator)
{
    if (!QSgsJsonUtils::isString(initiator))
        return;

    QString text = tr("%1 initiated a vote for disadvataged side to claim "
                      "capitulation. Click \"OK\" to surrender or \"Cancel\" to resist.")
                       .arg(Sanguosha->translate(initiator.toString()));
    text.append(tr("<br/> <b>Notice</b>: if all people on your side decides to surrender. "
                   "You'll lose this game."));
    skill_name = QStringLiteral("surrender");

    prompt_doc->setHtml(text);
    setStatus(AskForSkillInvoke);
}

void Client::askForLuckCard(const QJsonValue & /*unused*/)
{
    skill_to_invoke = QStringLiteral("luck_card");
    prompt_doc->setHtml(tr("Do you want to use the luck card?"));
    setStatus(AskForSkillInvoke);
}

void Client::askForNullification(const QJsonValue &arg)
{
    QJsonArray args = arg.toArray();
    if (args.size() != 3 || !QSgsJsonUtils::isString(args[0]) || !(args[1].isNull() || QSgsJsonUtils::isString(args[1])) || !QSgsJsonUtils::isString(args[2]))
        return;

    QString trick_name = args[0].toString();
    const QJsonValue &source_name = args[1];
    Player *target_player = findPlayer(args[2].toString());

    if ((target_player == nullptr) || (target_player->general() == nullptr))
        return;

    Player *source = nullptr;
    if (!source_name.isNull())
        source = findPlayer(source_name.toString());

    const CardFace *trick_card = Sanguosha->cardFace(trick_name);
    if (Config.NeverNullifyMyTrick && source == Self) {
        if (trick_card->isKindOf(QStringLiteral("SingleTargetTrick")) || trick_card->isKindOf(QStringLiteral("IronChain"))) {
            onPlayerResponseCard(nullptr);
            return;
        }
    }
    if (m_noNullificationThisTime && m_noNullificationTrickName == trick_name) {
        if (trick_card->isKindOf(QStringLiteral("AOE")) || trick_card->isKindOf(QStringLiteral("GlobalEffect"))) {
            onPlayerResponseCard(nullptr);
            return;
        }
    }

    if (source == nullptr) {
        prompt_doc->setHtml(
            tr("Do you want to use nullification to trick card %1 from %2?").arg(Sanguosha->translate(trick_card->name()), getPlayerName(target_player->objectName())));
    } else {
        prompt_doc->setHtml(tr("%1 used trick card %2 to %3 <br>Do you want to use nullification?")
                                .arg(getPlayerName(source->objectName()))
                                .arg(Sanguosha->translate(trick_name))
                                .arg(getPlayerName(target_player->objectName())));
    }

    setCurrentCardUsePattern(QStringLiteral("nullification"));
    m_isDiscardActionRefusable = true;

    setStatus(RespondingUse);
}

void Client::onPlayerChooseCard(int card_id)
{
    QJsonValue reply;
    if (card_id != -2)
        reply = card_id;
    replyToServer(S_COMMAND_CHOOSE_CARD, reply);
    setStatus(NotActive);
}

void Client::onPlayerChoosePlayer(const Player *player)
{
    if (player == nullptr && !m_isDiscardActionRefusable)
        player = findPlayer(players_to_choose.first());

    replyToServer(S_COMMAND_CHOOSE_PLAYER, (player == nullptr) ? QJsonValue() : player->objectName());
    setStatus(NotActive);
}

void Client::onPlayerChooseOption(const QString &choice)
{
    replyToServer(S_COMMAND_MULTIPLE_CHOICE, choice);
    setStatus(NotActive);
}

void Client::onPlayerChooseTriggerOrder(const QString &choice)
{
    replyToServer(S_COMMAND_TRIGGER_ORDER, choice);
    setStatus(NotActive);
}

void Client::trust()
{
    notifyServer(S_COMMAND_TRUST);

    if (Self->getState() == QStringLiteral("trust"))
        Audio::playSystemAudioEffect(QStringLiteral("untrust"));
    else
        Audio::playSystemAudioEffect(QStringLiteral("trust"));

    setStatus(NotActive);
}

void Client::preshow(const QString &skill_name, bool isPreshowed)
{
    QJsonArray arg;
    arg << skill_name;
    arg << isPreshowed;
    requestServer(S_COMMAND_PRESHOW, arg);
    Self->setSkillPreshowed(skill_name, isPreshowed);

    emit head_preshowed();
    emit deputy_preshowed();
}

void Client::requestSurrender()
{
    if (getStatus() != Playing)
        return;

    auto *card = cloneCard(QStringLiteral("SurrenderCard"));
    card->setHandleMethod(QSanguosha::MethodNone);
    onPlayerResponseCard(card);
}

void Client::speakToServer(const QString &text)
{
    if (text.isEmpty())
        return;

    QByteArray data = text.toUtf8().toBase64();
    notifyServer(S_COMMAND_SPEAK, QString::fromUtf8(data));
}

void Client::addHistory(const QJsonValue &history)
{
    QJsonArray args = history.toArray();
    if (args.size() != 2 || !QSgsJsonUtils::isString(args[0]) || !QSgsJsonUtils::isNumber(args[1]))
        return;

    QString add_str = args[0].toString();
    int times = args[1].toInt();
    if (add_str == QStringLiteral("pushPile")) {
        emit card_used();
        return;
    } else if (add_str == QStringLiteral(".")) {
        Self->clearHistory();
        return;
    }

    Self->addHistory(add_str, times);
}

int Client::alivePlayerCount() const
{
    return alive_count;
}

bool Client::save(const QString &filename) const
{
    if (recorder != nullptr)
        return recorder->save(filename);
    else
        return false;
}

QList<QByteArray> Client::getRecords() const
{
    if (recorder != nullptr)
        return recorder->getRecords();
    else
        return QList<QByteArray>();
}

QString Client::getReplayPath() const
{
    if (replayer != nullptr)
        return replayer->getPath();
    else
        return QString();
}

QTextDocument *Client::getLinesDoc() const
{
    return lines_doc;
}

QTextDocument *Client::getPromptDoc() const
{
    return prompt_doc;
}

Player *Client::getSelf() const
{
    return Self;
}

void Client::resetPiles(const QJsonValue & /*unused*/)
{
    discardPile().clear();
    swap_pile++;
    updatePileNum();
    emit pile_reset();
}

void Client::setPileNumber(const QJsonValue &pile_str)
{
    if (!pile_str.isDouble())
        return;
    pile_num = pile_str.toInt();
    updatePileNum();
}

void Client::synchronizeDiscardPile(const QJsonValue &discard_pile)
{
    if (!discard_pile.isArray())
        return;

    if (QSgsJsonUtils::isNumberArray(discard_pile, 0, discard_pile.toArray().count() - 1))
        return;

    QList<int> discard;
    if (QSgsJsonUtils::tryParse(discard_pile, discard)) {
        foreach (int id, discard)
            discardPile().append(id);

        updatePileNum();
    }
}

void Client::setCardFlag(const QJsonValue &pattern_str)
{
    QJsonArray pattern = pattern_str.toArray();
    if (pattern.size() != 2 || !QSgsJsonUtils::isNumber(pattern[0]) || !QSgsJsonUtils::isString(pattern[1]))
        return;

    int id = pattern[0].toInt();
    QString flag = pattern[1].toString();

    Card *card_ = card(id);
    if (card_ != nullptr)
        card_->addFlag(flag);
}

void Client::updatePileNum()
{
    QString pile_str = tr("Draw pile: <b>%1</b>, discard pile: <b>%2</b>, swap times: <b>%3</b>").arg(pile_num, discardPile().length(), swap_pile);
    lines_doc->setHtml((QStringLiteral("<font color='%1'><p align = \"center\">") + pile_str + QStringLiteral("</p></font>")).arg(Config.TextEditColor.name()));
}

void Client::askForDiscard(const QJsonValue &reqvar)
{
    QJsonArray req = reqvar.toArray();
    if (req.size() != 6 || !QSgsJsonUtils::isNumber(req[0]) || !QSgsJsonUtils::isNumber(req[1]) || !QSgsJsonUtils::isBool(req[2]) || !QSgsJsonUtils::isBool(req[3])
        || !QSgsJsonUtils::isString(req[4]) || !QSgsJsonUtils::isString(req[5]))
        return;

    discard_num = req[0].toInt();
    min_num = req[1].toInt();
    m_isDiscardActionRefusable = req[2].toBool();
    m_canDiscardEquip = req[3].toBool();
    QString prompt = req[4].toString();
    highlight_skill_name = req[5].toString();
    if (prompt.isEmpty()) {
        if (m_canDiscardEquip)
            prompt = tr("Please discard %1 card(s), include equip").arg(discard_num);
        else
            prompt = tr("Please discard %1 card(s), only hand cards is allowed").arg(discard_num);
        if (min_num < discard_num) {
            prompt.append(QStringLiteral("<br/>"));
            prompt.append(tr("%1 %2 cards(s) are required at least").arg(QString::number(min_num), m_canDiscardEquip ? QString() : tr("hand")));
        }
        prompt_doc->setHtml(prompt);
    } else {
        QStringList texts = prompt.split(QStringLiteral(":"));
        if (texts.length() < 4) {
            while (texts.length() < 3)
                texts.append(QString());
            texts.append(QString::number(discard_num));
        }
        setPromptList(texts);
    }

    setStatus(Discarding);
}

void Client::askForExchange(const QJsonValue &exchange)
{
    QJsonArray args = exchange.toArray();
    if (args.size() != 6 || !QSgsJsonUtils::isNumber(args[0]) || !QSgsJsonUtils::isNumber(args[1]) || !QSgsJsonUtils::isBool(args[2]) || !QSgsJsonUtils::isString(args[3])
        || !QSgsJsonUtils::isBool(args[4]) || !QSgsJsonUtils::isString(args[5]))
        return;

    discard_num = args[0].toInt();
    min_num = args[1].toInt();
    m_canDiscardEquip = args[2].toBool();
    QString prompt = args[3].toString();
    m_isDiscardActionRefusable = args[4].toBool();

    highlight_skill_name = args[5].toString();

    if (prompt.isEmpty()) {
        if (discard_num == min_num)
            prompt = tr("Please give %1 cards to exchange").arg(discard_num);
        else
            prompt = tr("Please give at most %1 cards to exchange.<br />You can give %2 cards at least").arg(discard_num, min_num);
        prompt_doc->setHtml(prompt);
    } else {
        QStringList texts = prompt.split(QStringLiteral(":"));
        if (texts.length() < 4) {
            while (texts.length() < 3)
                texts.append(QString());
            texts.append(QString::number(discard_num));
        }
        setPromptList(texts);
    }
    setStatus(Exchanging);
}

void Client::gameOver(const QJsonValue &arg)
{
    disconnectFromHost();
    m_isGameOver = true;
    setStatus(Client::NotActive);

    QJsonArray args = arg.toArray();
    if (args.size() < 2)
        return;

    QString winner = args[0].toString();
    QStringList roles;
    for (const QJsonValue &role : args[1].toArray())
        roles << role.toString();

    Q_ASSERT(roles.length() == players().length());

    for (int i = 0; i < roles.length(); i++) {
        QString name = players().at(i)->objectName();
        findPlayer(name)->setRole(roles.at(i));
    }

    if (winner == QStringLiteral(".")) {
        emit standoff();
        return;
    }

    QStringList winnersList = winner.split(QStringLiteral("+"));

    QSet<QString> winners = QSet<QString>(winnersList.begin(), winnersList.end());
    foreach (Player *player, players()) {
        QString role = player->roleString();
        bool win = winners.contains(player->objectName()) || winners.contains(role);
        player->setProperty("win", win);
    }

    emit game_over();
}

void Client::killPlayer(const QJsonValue &player_name)
{
    if (!QSgsJsonUtils::isString(player_name))
        return;
    QString name = player_name.toString();

    alive_count--;
    Player *player = findPlayer(name);
    if (player == Self) {
        if (isHegemonyGameMode(ServerInfo.GameModeStr)) {
            foreach (const Skill *skill, Self->getHeadSkillList(true, true))
                emit skill_detached(skill->name(), true);
            foreach (const Skill *skill, Self->getDeputySkillList(true, true))
                emit skill_detached(skill->name(), false);
        } else {
            foreach (const Skill *skill, Self->skills(false))
                emit skill_detached(skill->name());
        }
    }
    player->detachAllSkills();

    if (!Self->hasFlag(QStringLiteral("marshalling"))) {
        QString general_name = player->generalName();
        QString last_word = Sanguosha->translate(QStringLiteral("~%1").arg(general_name));
        if (last_word.startsWith(QStringLiteral("~"))) {
            QStringList origin_generals = general_name.split(QStringLiteral("_"));
            if (origin_generals.length() > 1)
                last_word = Sanguosha->translate(QStringLiteral("~") + origin_generals.at(1));
        }

        if (last_word.startsWith(QStringLiteral("~")) && general_name.endsWith(QStringLiteral("f"))) {
            QString origin_general = general_name;
            origin_general.chop(1);
            if (Sanguosha->general(origin_general) != nullptr)
                last_word = Sanguosha->translate(QStringLiteral("~") + origin_general);
        }
        updatePileNum();
    }

    emit player_killed(name);
}

void Client::setDashboardShadow(const QJsonValue &player_arg)
{
    if (!QSgsJsonUtils::isString(player_arg))
        return;
    QString name = player_arg.toString();

    emit dashboard_death(name);
}

void Client::revivePlayer(const QJsonValue &player_arg)
{
    if (!QSgsJsonUtils::isString(player_arg))
        return;

    QString player_name = player_arg.toString();

    alive_count++;
    updatePileNum();
    emit player_revived(player_name);
}

void Client::warn(const QJsonValue &reason_var)
{
    QString reason = reason_var.toString();
    QString msg;
    if (reason == QStringLiteral("GAME_OVER"))
        msg = tr("Game is over now");
    else if (reason == QStringLiteral("INVALID_FORMAT"))
        msg = tr("Invalid signup string");
    else if (reason == QStringLiteral("INVALID_OPERATION"))
        msg = tr("Invalid operation");
    else if (reason == QStringLiteral("OPERATION_NOT_IMPLEMENTED"))
        msg = tr("Operation not implemented");
    else if (reason == QStringLiteral("USERNAME_INCORRECT"))
        msg = tr("Username is incorrect");
    else
        msg = tr("Unknown warning: %1").arg(reason);

    disconnectFromHost();
    QMessageBox::warning(nullptr, tr("Warning"), msg);
}

void Client::askForGeneral(const QJsonValue &arg)
{
    QJsonArray args = arg.toArray();
    QStringList generals;
    bool single_result = false;
    bool can_convert = false;

    if (!isHegemonyGameMode(ServerInfo.GameModeStr) || Self->hasFlag(QStringLiteral("Pingyi_Choose"))) {
        if (!tryParse(arg, generals))
            return;
    } else {
        if (!tryParse(args[0], generals))
            return;
        single_result = args[1].toBool();
        can_convert = args[2].toBool();
    }

    if (isHegemonyGameMode(ServerInfo.GameModeStr) && ServerInfo.isMultiGeneralEnabled() && !Self->hasFlag(QStringLiteral("Pingyi_Choose"))) {
        emit generals_got(generals, single_result, can_convert);
        setStatus(AskForGeneralTaken);
    } else {
        emit generals_got(generals, single_result, can_convert);
        setStatus(ExecDialog);
    }
}

void Client::askForSuit(const QJsonValue &arg)
{
    QJsonArray arr = arg.toArray();
    if (arr.count() != 2)
        return;
    highlight_skill_name = arr.first().toString();
    QStringList suits;
    suits << QStringLiteral("spade") << QStringLiteral("club") << QStringLiteral("heart") << QStringLiteral("diamond");
    emit suits_got(suits);
    setStatus(ExecDialog);
}

void Client::askForKingdom(const QJsonValue & /*unused*/)
{
    QStringList kingdoms = Sanguosha->kingdoms().values();

    if (kingdoms.contains(QStringLiteral("zhu")))
        kingdoms.removeOne(QStringLiteral("zhu"));
    if (kingdoms.contains(QStringLiteral("touhougod")))
        kingdoms.removeOne(QStringLiteral("touhougod"));

    emit kingdoms_got(kingdoms);
    setStatus(ExecDialog);
}

void Client::askForChoice(const QJsonValue &ask_str)
{
    QJsonArray ask = ask_str.toArray();
    if (!QSgsJsonUtils::isStringArray(ask, 0, 1))
        return;
    QString skill_name = ask[0].toString();
    highlight_skill_name = skill_name;
    QStringList options = ask[1].toString().split(QStringLiteral("+"));
    emit options_got(skill_name, options);
    setStatus(AskForChoice);
}

void Client::askForCardChosen(const QJsonValue &ask_str)
{
    QJsonArray ask = ask_str.toArray();
    if (ask.size() != 7 || !QSgsJsonUtils::isStringArray(ask, 0, 2) || !QSgsJsonUtils::isBool(ask[3]) || !QSgsJsonUtils::isNumber(ask[4]))
        return;
    QString player_name = ask[0].toString();
    QString flags = ask[1].toString();
    QString reason = ask[2].toString();
    highlight_skill_name = reason;
    bool handcard_visible = ask[3].toBool();
    QSanguosha::HandlingMethod method = (QSanguosha::HandlingMethod)ask[4].toInt();
    Player *player = findPlayer(player_name);
    if (player == nullptr)
        return;
    QList<int> disabled_ids;
    QSgsJsonUtils::tryParse(ask[5], disabled_ids);
    bool enableEmptyCard = ask[6].toBool();
    emit cards_got(player, flags, reason, handcard_visible, method, disabled_ids, enableEmptyCard);
    setStatus(AskForCardChosen);
}

void Client::askForOrder(const QJsonValue &arg)
{
    if (!QSgsJsonUtils::isNumber(arg))
        return;
    Game3v3ChooseOrderCommand reason = (Game3v3ChooseOrderCommand)arg.toInt();
    emit orders_got(reason);
    setStatus(ExecDialog);
}

void Client::askForRole3v3(const QJsonValue &arg)
{
    QJsonArray ask = arg.toArray();
    if (ask.count() != 2 || !QSgsJsonUtils::isString(ask[0]) || !QSgsJsonUtils::isStringArray(ask[1], 0, ask[1].toArray().count() - 1))
        return;

    QStringList roles;
    if (!QSgsJsonUtils::tryParse(ask[1], roles))
        return;
    QString scheme = ask[0].toString();
    emit roles_got(scheme, roles);
    setStatus(ExecDialog);
}

void Client::askForDirection(const QJsonValue & /*unused*/)
{
    emit directions_got();
    setStatus(ExecDialog);
}

void Client::askForTriggerOrder(const QJsonValue &ask_str)
{
    QJsonArray ask = ask_str.toArray();
    if (ask.size() != 2 || !ask[0].isArray() || !QSgsJsonUtils::isBool(ask[1]))
        return;
    QVariantList l = ask[0].toArray().toVariantList();
    bool optional = ask[1].toBool();

    emit triggers_got(l, optional);
    setStatus(AskForTriggerOrder);
}

void Client::setMark(const QJsonValue &mark_var)
{
    QJsonArray mark_str = mark_var.toArray();
    if (mark_str.size() != 3)
        return;
    if (!QSgsJsonUtils::isString(mark_str[0]) || !QSgsJsonUtils::isString(mark_str[1]) || !QSgsJsonUtils::isNumber(mark_str[2]))
        return;

    QString who = mark_str[0].toString();
    QString mark = mark_str[1].toString();
    int value = mark_str[2].toInt();

    Player *player = findPlayer(who);
    player->setMark(mark, value);
}

void Client::onPlayerChooseSuit()
{
    replyToServer(S_COMMAND_CHOOSE_SUIT, sender()->objectName());
    setStatus(NotActive);
}

void Client::onPlayerChooseKingdom()
{
    replyToServer(S_COMMAND_CHOOSE_KINGDOM, sender()->objectName());
    setStatus(NotActive);
}

void Client::onPlayerDiscardCards(const Card *cards)
{
    if (cards != nullptr) {
        QJsonArray arr;
        foreach (int card_id, cards->subcards())
            arr << card_id;
        if (cards->isVirtualCard()) // TODO: Delete card
            cardDeleting(cards);
        replyToServer(S_COMMAND_DISCARD_CARD, arr);
    } else {
        replyToServer(S_COMMAND_DISCARD_CARD);
    }

    setStatus(NotActive);
}

void Client::fillAG(const QJsonValue &cards_str)
{
    QJsonArray cards = cards_str.toArray();
    if (cards.size() != 3)
        return;
    QList<int> card_ids;
    QList<int> disabled_ids;
    QList<int> shownHandcard_ids;
    QSgsJsonUtils::tryParse(cards[0], card_ids);
    QSgsJsonUtils::tryParse(cards[1], disabled_ids);
    QSgsJsonUtils::tryParse(cards[2], shownHandcard_ids);
    emit ag_filled(card_ids, disabled_ids, shownHandcard_ids);
}

void Client::takeAG(const QJsonValue &take_var)
{
    QJsonArray take = take_var.toArray();
    if (take.size() != 3)
        return;
    if (!QSgsJsonUtils::isNumber(take[1]) || !QSgsJsonUtils::isBool(take[2]))
        return;

    int card_id = take[1].toInt();
    bool move_cards = take[2].toBool();
    const Card *card_ = card(card_id);

    if (take[0].isNull()) {
        if (move_cards) {
            discardPile().prepend(card_id);
            updatePileNum();
        }
        emit ag_taken(nullptr, card_id, move_cards);
    } else {
        Player *taker = findPlayer(take[0].toString());
        if (move_cards)
            taker->addCard(card_, QSanguosha::PlaceHand);
        emit ag_taken(taker, card_id, move_cards);
    }
}

void Client::clearAG(const QJsonValue & /*unused*/)
{
    emit ag_cleared();
}

void Client::askForSinglePeach(const QJsonValue &arg)
{
    QJsonArray args = arg.toArray();
    if (args.size() != 2 || !QSgsJsonUtils::isString(args[0]) || !QSgsJsonUtils::isNumber(args[1]))
        return;

    Player *dying = findPlayer(args[0].toString());
    int peaches = args[1].toInt();

    // @todo: anti-cheating of askForSinglePeach is not done yet!!!
    QStringList pattern;
    pattern << QStringLiteral("peach");
    if (dying == Self) {
        prompt_doc->setHtml(tr("You are dying, please provide %1 peach(es)(or analeptic) to save yourself").arg(peaches));
        pattern << QStringLiteral("analeptic");

    } else {
        QString dying_general = getPlayerName(dying->objectName());

        prompt_doc->setHtml(tr("%1 is dying, please provide %2 peach(es) to save him").arg(dying_general, peaches));
    }

    Card *temp_peach = cloneCard(QStringLiteral("Peach"));
    if (Self->mark(QStringLiteral("Global_PreventPeach")) > 0 || Self->isProhibited(dying, temp_peach)) {
        bool has_skill = false;
        foreach (const Skill *skill, Self->skills(true)) {
            const ViewAsSkill *view_as_skill = dynamic_cast<const ViewAsSkill *>(skill);
            if ((view_as_skill != nullptr) && view_as_skill->isAvailable(Self, QSanguosha::CardUseReasonResponseUse, pattern.join(QStringLiteral("+")))) {
                has_skill = true;
                break;
            }
        }
        if (!has_skill) {
            pattern.removeOne(QStringLiteral("peach"));
            if (pattern.isEmpty()) {
                onPlayerResponseCard(nullptr);
                return;
            }
        } else {
            Self->setFlag(QStringLiteral("Client_PreventPeach"));
            Self->setCardLimitation(QStringLiteral("use"), QStringLiteral("Peach"), QStringLiteral("Global_PreventPeach"));
        }
    }
    cardDeleting(temp_peach); // Delete of card peach.

    setCurrentCardUsePattern(pattern.join(QStringLiteral("+")));
    m_isDiscardActionRefusable = true;
    setStatus(RespondingUse);
}

void Client::askForCardShow(const QJsonValue &requestor)
{
    if (!QSgsJsonUtils::isString(requestor))
        return;
    QString name = Sanguosha->translate(requestor.toString());
    prompt_doc->setHtml(tr("%1 request you to show one hand card").arg(name));

    setCurrentCardUsePattern(QStringLiteral("."));
    setStatus(AskForShowOrPindian);
}

void Client::askForAG(const QJsonValue &arg)
{
    QJsonArray arr = arg.toArray();
    if (arr.count() != 2 || !QSgsJsonUtils::isBool(arr.first()) || !QSgsJsonUtils::isString(arr.at(1)))
        return;

    m_isDiscardActionRefusable = arr.first().toBool();
    highlight_skill_name = arr.at(1).toString();

    setStatus(AskForAG);
}

void Client::onPlayerChooseAG(int card_id)
{
    replyToServer(S_COMMAND_AMAZING_GRACE, card_id);
    setStatus(NotActive);
}

void Client::alertFocus()
{
    if (Self->phase() == QSanguosha::PhasePlay)
        QApplication::alert(QApplication::focusWidget());
}

void Client::showCard(const QJsonValue &show_str)
{
    QJsonArray show = show_str.toArray();
    if (show.size() != 2 || !QSgsJsonUtils::isString(show[0]) || !QSgsJsonUtils::isNumber(show[1]))
        return;

    QString player_name = show[0].toString();
    int card_id = show[1].toInt();

    Player *player = findPlayer(player_name);
    if (player != Self) {
        IdSet s = player->handcards();
        s << card_id;
        player->setHandCards(s);
    }

    emit card_shown(player_name, card_id);
}

void Client::attachSkill(const QJsonValue &skill)
{
    if (!QSgsJsonUtils::isString(skill))
        return;

    QString skill_name = skill.toString();
    const Skill *s = Sanguosha->skill(skill_name);
    loadSkill(s);
    Self->acquireSkill(skill_name);
    emit skill_attached(skill_name, true);
}

void Client::askForAssign(const QJsonValue & /*unused*/)
{
    emit assign_asked();
}

void Client::onPlayerAssignRole(const QStringList &names, const QStringList &roles)
{
    Q_ASSERT(names.size() == roles.size());

    QJsonArray reply;
    reply << QSgsJsonUtils::toJsonArray(names) << QSgsJsonUtils::toJsonArray(roles);

    replyToServer(S_COMMAND_CHOOSE_ROLE, reply);
}

void Client::askForGuanxing(const QJsonValue &arg)
{
    QJsonArray args = arg.toArray();
    if (args.isEmpty())
        return;

    QJsonArray deck = args[0].toArray();
    bool single_side = args[1].toBool();
    highlight_skill_name = args[2].toString();
    QList<int> card_ids;
    QSgsJsonUtils::tryParse(deck, card_ids);

    emit guanxing(card_ids, single_side);
    setStatus(AskForGuanxing);
}

void Client::showAllCards(const QJsonValue &arg)
{
    QJsonArray args = arg.toArray();
    if (args.size() != 3 || !QSgsJsonUtils::isString(args[0]) || !QSgsJsonUtils::isBool(args[1]))
        return;

    Player *who = findPlayer(args[0].toString());
    QList<int> card_ids;
    if (!QSgsJsonUtils::tryParse(args[2], card_ids))
        return;

    if (who != nullptr)
        who->setHandCards(List2Set(card_ids));

    emit gongxin(card_ids, false, IdSet(), who != nullptr ? who->shownHandcards() : IdSet());
}

void Client::askForGongxin(const QJsonValue &args)
{
    QJsonArray arg = args.toArray();
    if (arg.size() != 6 || !QSgsJsonUtils::isString(arg[0]) || !QSgsJsonUtils::isBool(arg[1]))
        return;

    Player *who = findPlayer(arg[0].toString());

    bool enable_heart = arg[1].toBool();
    QList<int> card_ids;
    if (!QSgsJsonUtils::tryParse(arg[2], card_ids))
        return;
    QList<int> enabled_ids;
    if (!QSgsJsonUtils::tryParse(arg[3], enabled_ids))
        return;
    highlight_skill_name = arg[4].toString();
    m_isDiscardActionRefusable = arg[5].toBool();

    who->setHandCards(List2Set(card_ids));

    emit gongxin(card_ids, enable_heart, IdSet(enabled_ids.begin(), enabled_ids.end()), who->shownHandcards());
    setStatus(AskForGongxin);
}

void Client::onPlayerReplyGongxin(int card_id)
{
    QJsonValue reply;
    if (card_id != -1)
        reply = card_id;
    replyToServer(S_COMMAND_SKILL_GONGXIN, reply);
    setStatus(NotActive);
}

void Client::askForPindian(const QJsonValue &ask_str)
{
    QJsonArray ask = ask_str.toArray();
    if (!QSgsJsonUtils::isStringArray(ask, 0, 1))
        return;
    QString from = ask[0].toString();
    if (from == Self->objectName())
        prompt_doc->setHtml(tr("Please play a card for pindian"));
    else {
        QString requestor = getPlayerName(from);
        prompt_doc->setHtml(tr("%1 ask for you to play a card to pindian").arg(requestor));
    }
    setCurrentCardUsePattern(QStringLiteral("."));
    setStatus(AskForShowOrPindian);
}

void Client::askForYiji(const QJsonValue &ask_str)
{
    QJsonArray ask = ask_str.toArray();
    if (ask.size() != 6)
        return;
    QJsonArray card_list = ask[0].toArray();
    int count = ask[2].toInt();
    m_isDiscardActionRefusable = ask[1].toBool();
    QString prompt = ask[4].toString();
    highlight_skill_name = ask[5].toString();

    if (!prompt.isEmpty()) {
        QStringList texts = prompt.split(QStringLiteral(":"));
        if (texts.length() < 4) {
            while (texts.length() < 3)
                texts.append(QString());
            texts.append(QString::number(count));
        }
        setPromptList(texts);
    } else {
        prompt_doc->setHtml(tr("Please distribute %1 cards %2 as you wish").arg(QString::number(count), m_isDiscardActionRefusable ? QString() : tr("to another player")));
    }

    //@todo: use cards directly rather than the QString
    QStringList card_str;
    for (const QJsonValue &card : card_list)
        card_str << QString::number(card.toInt());

    QJsonArray players = ask[3].toArray();
    QStringList names;
    QSgsJsonUtils::tryParse(players, names);

    setCurrentCardUsePattern(QStringLiteral("%1=%2=%3").arg(QString::number(count), card_str.join(QStringLiteral("+")), names.join(QStringLiteral("+"))));
    setStatus(AskForYiji);
}

void Client::askForPlayerChosen(const QJsonValue &players)
{
    QJsonArray args = players.toArray();
    if (args.size() != 4)
        return;
    if (!QSgsJsonUtils::isString(args[1]) || !args[0].isArray() || !QSgsJsonUtils::isBool(args[3]))
        return;
    QJsonArray choices = args[0].toArray();
    if (choices.empty())
        return;

    skill_name = args[1].toString();
    highlight_skill_name = args[1].toString();
    players_to_choose.clear();
    for (int i = 0; i < choices.size(); i++)
        players_to_choose.push_back(choices[i].toString());
    m_isDiscardActionRefusable = args[3].toBool();

    QString text;
    QString description = Sanguosha->translate(skill_name);
    QString prompt = args[2].toString();
    if (!prompt.isEmpty()) {
        QStringList texts = prompt.split(QStringLiteral(":"));
        text = setPromptList(texts);
        if (prompt.startsWith(QStringLiteral("@")) && !description.isEmpty() && description != skill_name)
            text.append(tr("<br/> <b>Source</b>: %1<br/>").arg(description));
    } else {
        text = tr("Please choose a player");
        if (!description.isEmpty() && description != skill_name)
            text.append(tr("<br/> <b>Source</b>: %1<br/>").arg(description));
    }
    prompt_doc->setHtml(text);

    setStatus(AskForPlayerChoose);
}

void Client::onPlayerReplyYiji(const Card *card, const Player *to)
{
    if (card == nullptr)
        replyToServer(S_COMMAND_SKILL_YIJI);
    else {
        QJsonArray req;
        req << QSgsJsonUtils::toJsonArray(card->subcards().values());
        req << to->objectName();
        replyToServer(S_COMMAND_SKILL_YIJI, req);
    }

    setStatus(NotActive);
}

void Client::onPlayerReplyGuanxing(const QList<int> &up_cards, const QList<int> &down_cards)
{
    QJsonArray decks;
    decks << QSgsJsonUtils::toJsonArray(up_cards);
    decks << QSgsJsonUtils::toJsonArray(down_cards);

    replyToServer(S_COMMAND_SKILL_GUANXING, decks);

    setStatus(NotActive);
}

void Client::log(const QJsonValue &log_str)
{
    QStringList log;

    if (!QSgsJsonUtils::tryParse(log_str, log) || log.size() != 6)
        emit log_received(QStringList() << QString());
    else {
        if (log.first().contains(QStringLiteral("#HegemonyReveal")))
            Audio::playSystemAudioEffect(QStringLiteral("choose-item"));

        emit log_received(log);
    }
}

void Client::speak(const QJsonValue &speak)
{
    if (!speak.isArray()) {
        qDebug() << speak;
        return;
    }

    QJsonArray args = speak.toArray();
    QString who = args[0].toString();
    QString text = QString::fromUtf8(QByteArray::fromBase64(args[1].toString().toLatin1()));
    emit text_spoken(text);

    if (who == QStringLiteral(".")) {
        QString line = tr("<font color='red'>System: %1</font>").arg(text);
        emit line_spoken(QStringLiteral("<p style=\"margin:3px 2px;\">%1</p>").arg(line));
        return;
    }

    emit player_spoken(who, QStringLiteral("<p style=\"margin:3px 2px;\">%1</p>").arg(text));

    const Player *from = findPlayer(who);

    QString title;
    if (from != nullptr) {
        title = from->generalName();
        title = Sanguosha->translate(title);
        title.append(QStringLiteral("(%1)").arg(from->screenName()));
    }

    title = QStringLiteral("<b>%1</b>").arg(title);

    QString line = tr("<font color='%1'>[%2] said: %3 </font>").arg(Config.TextEditColor.name(), title, text);

    emit line_spoken(QStringLiteral("<p style=\"margin:3px 2px;\">%1</p>").arg(line));
}

void Client::heartbeat(const QJsonValue & /*unused*/)
{
}

void Client::moveFocus(const QJsonValue &focus)
{
    Countdown countdown;

    QJsonArray args = focus.toArray();
    Q_ASSERT(!args.isEmpty());

    QStringList playersx;
    QJsonArray json_players = args[0].toArray();
    if (!json_players.isEmpty()) {
        QSgsJsonUtils::tryParse(json_players, playersx);
    } else {
        foreach (Player *player, players()) {
            if (player->isAlive())
                playersx << player->objectName();
        }
    }

    if (args.size() == 1) { //default countdown
        countdown.type = Countdown::S_COUNTDOWN_USE_SPECIFIED;
        countdown.current = 0;
        countdown.max = ServerInfo.getCommandTimeout(S_COMMAND_UNKNOWN, S_CLIENT_INSTANCE);
    } else // focus[1] is the moveFocus reason, which is unused for now.
        countdown.tryParse(args[2]);
    emit focus_moved(playersx, countdown);
}

void Client::setEmotion(const QJsonValue &set_str)
{
    QJsonArray set = set_str.toArray();
    if (set.size() != 2)
        return;
    if (!QSgsJsonUtils::isStringArray(set, 0, 1))
        return;

    QString target_name = set[0].toString();
    QString emotion = set[1].toString();

    emit emotion_set(target_name, emotion);
}

void Client::skillInvoked(const QJsonValue &arg)
{
    QJsonArray args = arg.toArray();
    if (!QSgsJsonUtils::isStringArray(args, 0, 1))
        return;
    emit skill_invoked(args[1].toString(), args[0].toString());
}

void Client::animate(const QJsonValue &animate_str)
{
    QJsonArray animate = animate_str.toArray();
    if (animate.size() != 3 || !QSgsJsonUtils::isNumber(animate[0]) || !QSgsJsonUtils::isString(animate[1]) || !QSgsJsonUtils::isString(animate[2]))
        return;

    QStringList args;
    args << animate[1].toString() << animate[2].toString();
    int name = animate[0].toInt();
    emit animated(name, args);
}

void Client::setFixedDistance(const QJsonValue &set_str)
{
    QJsonArray set = set_str.toArray();
    if (set.size() != 3 || !QSgsJsonUtils::isString(set[0]) || !QSgsJsonUtils::isString(set[1]) || !QSgsJsonUtils::isNumber(set[2]))
        return;

    Player *from = findPlayer(set[0].toString());
    Player *to = findPlayer(set[1].toString());
    int distance = set[2].toInt();

    if ((from != nullptr) && (to != nullptr))
        from->setFixedDistance(to, distance);
}

void Client::fillGenerals(const QJsonValue &generals)
{
    if (!generals.isArray())
        return;

    QStringList filled;
    QSgsJsonUtils::tryParse(generals, filled);
    emit generals_filled(filled);
}

void Client::askForGeneral3v3(const QJsonValue & /*unused*/)
{
    emit general_asked();
    setStatus(AskForGeneralTaken);
}

void Client::takeGeneral(const QJsonValue &take)
{
    QJsonArray take_array = take.toArray();
    if (!QSgsJsonUtils::isStringArray(take_array, 0, 2))
        return;
    QString who = take_array[0].toString();
    QString name = take_array[1].toString();
    QString rule = take_array[2].toString();

    emit general_taken(who, name, rule);
}

void Client::startArrange(const QJsonValue &to_arrange)
{
    if (to_arrange.isNull()) {
        emit arrange_started(QString());
    } else {
        if (!to_arrange.isArray())
            return;
        QStringList arrangelist;
        QSgsJsonUtils::tryParse(to_arrange, arrangelist);
        emit arrange_started(arrangelist.join(QStringLiteral("+")));
    }
    setStatus(AskForArrangement);
}

void Client::onPlayerChooseRole3v3()
{
    replyToServer(S_COMMAND_CHOOSE_ROLE_3V3, sender()->objectName());
    setStatus(NotActive);
}

void Client::recoverGeneral(const QJsonValue &recover)
{
    QJsonArray args = recover.toArray();
    if (args.size() != 2 || !QSgsJsonUtils::isNumber(args[0]) || !QSgsJsonUtils::isString(args[1]))
        return;
    int index = args[0].toInt();
    QString name = args[1].toString();

    emit general_recovered(index, name);
}

void Client::revealGeneral(const QJsonValue &reveal)
{
    QJsonArray args = reveal.toArray();
    if (args.size() != 2 || !QSgsJsonUtils::isString(args[0]) || !QSgsJsonUtils::isString(args[1]))
        return;
    bool self = (args[0].toString() == Self->objectName());
    QString general = args[1].toString();

    emit general_revealed(self, general);
}

void Client::onPlayerChooseOrder()
{
    OptionButton *button = qobject_cast<OptionButton *>(sender());
    QString order;
    if (button != nullptr) {
        order = button->objectName();
    } else {
        if (QRandomGenerator::global()->generate() % 2 == 0)
            order = QStringLiteral("warm");
        else
            order = QStringLiteral("cool");
    }
    int req = (int)S_CAMP_COOL;
    if (order == QStringLiteral("warm"))
        req = (int)S_CAMP_WARM;

    replyToServer(S_COMMAND_CHOOSE_ORDER, req);
    setStatus(NotActive);
}

void Client::updateStateItem(const QJsonValue &state)
{
    if (!QSgsJsonUtils::isString(state))
        return;
    emit role_state_changed(state.toString());
}

void Client::setAvailableCards(const QJsonValue &pile)
{
    QList<int> drawPile;
    if (QSgsJsonUtils::tryParse(pile, drawPile))
        available_cards = drawPile;
}

void Client::clearHighlightSkillName()
{
    highlight_skill_name = QString();
}

QString Client::getSkillDescription(QString skillname) const
{
    bool normal_game = ServerInfo.parsed() && isRoleGameMode(ServerInfo.GameModeStr);
    QString name = QStringLiteral("%1%2").arg(skillname, normal_game ? QStringLiteral("_p") : QString());
    QString des_src = Sanguosha->translate(QStringLiteral(":") + name);
    if (normal_game && des_src.startsWith(QStringLiteral(":")))
        des_src = Sanguosha->translate(QStringLiteral(":") + skillname);
    if (des_src.startsWith(QStringLiteral(":")))
        return QString();
    QString desc = QStringLiteral("<font color=%1>%2</font>").arg(QStringLiteral("#FF0080"), des_src);
    return desc;
}

QString Client::getSkillNotice(QString skillname, int index) const
{
    if (index == -1)
        return Sanguosha->translate(QStringLiteral("~") + skillname);

    return Sanguosha->translate(QStringLiteral("~%1%2").arg(skillname, index));
}

QString Client::getGeneralSkillDescription(QString generalname, bool include_name, bool yellow) const
{
    const General *g = Sanguosha->general(generalname);
    if (g == nullptr)
        return QString();

    QString description;
    foreach (const Skill *skill, g->skills()) {
        QString skill_name = Sanguosha->translate(skill->name());
        QString desc = getSkillDescription(skill->name());
        desc.replace(QStringLiteral("\n"), QStringLiteral("<br/>"));
        description.append(QStringLiteral("<font color=%1><b>%2</b>:</font> %3 <br/> <br/>").arg(yellow ? QStringLiteral("#FFFF33") : QStringLiteral("#FF0080"), skill_name, desc));
    }

    if (include_name) {
        QString color_str = QSanUiUtils::getKingdomColor(g->kingdom()).name();
        QString g_name = Sanguosha->translate(QStringLiteral("!") + generalname);
        if (g_name.startsWith(QStringLiteral("!")))
            g_name = Sanguosha->translate(generalname);
        QString name = QStringLiteral("<font color=%1><b>%2</b></font>     ").arg(color_str, g_name);
        name.prepend(QStringLiteral("<img src='image/kingdom/icon/%1.png'/>    ").arg(g->kingdom()));
        for (int i = 0; i < g->maxHp(); i++)
            name.append(QStringLiteral("<img src='image/system/magatamas/5.png' height = 12/>"));
        if (g->hasSkill(QStringLiteral("banling"))) {
            for (int i = 0; i < g->maxHp(); i++)
                name.append(QStringLiteral("<img src='image/system/magatamas/1.png' height = 12/>"));
        }
        name.append(QStringLiteral("<br/> <br/>"));
        description.prepend(name);
    }

    return description;
}

QString Client::getPlayerFootNoteName(const Player *player) const
{
    QStringList generalNames = player->generalNames();
    generalNames.removeAll(QString());
    generalNames.removeAll(QStringLiteral("anjiang"));

    if (generalNames.isEmpty()) {
        return Sanguosha->translate(QStringLiteral("SEAT(%1)").arg(QString::number(player->seat())));
    } else {
        QStringList translatedGeneralNames;
        foreach (const QString &name, generalNames)
            translatedGeneralNames << Sanguosha->translate(name);

        return translatedGeneralNames.join(QStringLiteral("/"));
    }
}

void Client::changeSkin(const QString &name, int index)
{
    QJsonArray skinInfo;
    skinInfo << name << index;

    notifyServer(S_COMMAND_SKIN_CHANGE, skinInfo);
}
