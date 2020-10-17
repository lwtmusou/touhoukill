#ifndef _CLIENT_H
#define _CLIENT_H

#include "RoomObject.h"
#include "card.h"
#include "clientplayer.h"
#include "clientstruct.h"
#include "protocol.h"
#include "skill.h"
#include "socket.h"

class Recorder;
class Replayer;
class QTextDocument;

class Client : public RoomObject
{
    Q_OBJECT
    Q_PROPERTY(Client::Status status READ getStatus WRITE setStatus)
    Q_ENUMS(Status)

public:
    enum Status
    {
        NotActive = 0x0000,
        Responding = 0x0001,
        Playing = 0x0002,
        Discarding = 0x0003,
        Exchanging = 0x0004,
        ExecDialog = 0x0005,
        AskForSkillInvoke = 0x0006,
        AskForAG = 0x0007,
        AskForPlayerChoose = 0x0008,
        AskForYiji = 0x0009,
        AskForGuanxing = 0x000A,
        AskForGongxin = 0x000B,
        AskForShowOrPindian = 0x000C,
        AskForGeneralTaken = 0x000D,
        AskForArrangement = 0x000E,
        AskForChoice = 0x000F,
        AskForTriggerOrder = 0x0010,
        AskForCardChosen = 0x0011,

        RespondingUse = 0x0101,
        RespondingForDiscard = 0x0201,
        RespondingNonTrigger = 0x0301,

        ClientStatusBasicMask = 0x00FF
    };

    explicit Client(QObject *parent, const QString &filename = QString());
    ~Client();

    // cheat functions
    void requestCheatGetOneCard(int card_id);
    void requestCheatChangeGeneral(const QString &name, bool isSecondaryHero = false);
    void requestCheatKill(const QString &killer, const QString &victim);
    void requestCheatDamage(const QString &source, const QString &target, DamageStruct::Nature nature, int points);
    void requestCheatRevive(const QString &name);
    void requestCheatRunScript(const QString &script);

    // other client requests
    void requestSurrender();

    void disconnectFromHost();
    void replyToServer(QSanProtocol::CommandType command, const QVariant &arg = QVariant());
    void requestServer(QSanProtocol::CommandType command, const QVariant &arg = QVariant());
    void notifyServer(QSanProtocol::CommandType command, const QVariant &arg = QVariant());
    void onPlayerResponseCard(const Card *card, const QList<const Player *> &targets = QList<const Player *>());
    void setStatus(Status status);
    Status getStatus() const;
    int alivePlayerCount() const;
    void onPlayerInvokeSkill(bool invoke);
    void onPlayerDiscardCards(const Card *card);
    void onPlayerReplyYiji(const Card *card, const Player *to);
    void onPlayerReplyGuanxing(const QList<int> &up_cards, const QList<int> &down_cards);
    void onPlayerAssignRole(const QList<QString> &names, const QList<QString> &roles);
    QList<const ClientPlayer *> getPlayers() const;
    void speakToServer(const QString &text);
    ClientPlayer *getPlayer(const QString &name);
    bool save(const QString &filename) const;
    QList<QByteArray> getRecords() const;
    QString getReplayPath() const;
    void setLines(const QString &skill_name);
    Replayer *getReplayer() const;
    QString getPlayerName(const QString &str);
    QString getSkillNameToInvoke() const;

    QTextDocument *getLinesDoc() const;
    QTextDocument *getPromptDoc() const;

    typedef void (Client::*Callback)(const QVariant &);

    void checkVersion(const QVariant &server_version);
    void setup(const QVariant &setup_str);
    void networkDelayTest(const QVariant &);
    void addPlayer(const QVariant &player_info);
    void removePlayer(const QVariant &player_name);
    void startInXs(const QVariant &);
    void arrangeSeats(const QVariant &seats);
    void activate(const QVariant &playerId);
    void startGame(const QVariant &);
    void hpChange(const QVariant &change_str);
    void maxhpChange(const QVariant &change_str);
    void resetPiles(const QVariant &);
    void setPileNumber(const QVariant &pile_str);
    void synchronizeDiscardPile(const QVariant &discard_pile);
    void gameOver(const QVariant &);
    void loseCards(const QVariant &);
    void getCards(const QVariant &);
    void updateProperty(const QVariant &);
    void killPlayer(const QVariant &player_arg);
    void revivePlayer(const QVariant &player_arg);
    void setDashboardShadow(const QVariant &player_arg);
    void warn(const QVariant &);
    void setMark(const QVariant &mark_str);
    void showCard(const QVariant &show_str);
    void log(const QVariant &log_str);
    void speak(const QVariant &speak_data);
    void heartbeat(const QVariant &);
    void addHistory(const QVariant &history);
    void moveFocus(const QVariant &focus);
    void setEmotion(const QVariant &set_str);
    void skillInvoked(const QVariant &invoke_str);
    void animate(const QVariant &animate_str);
    void cardLimitation(const QVariant &limit);
    void disableShow(const QVariant &args);
    void setNullification(const QVariant &str);
    void enableSurrender(const QVariant &enabled);
    void exchangeKnownCards(const QVariant &players);
    void setKnownCards(const QVariant &set_str);
    void viewGenerals(const QVariant &str);
    void setFixedDistance(const QVariant &set_str);
    void updateStateItem(const QVariant &state_str);
    void setAvailableCards(const QVariant &pile);
    void setCardFlag(const QVariant &pattern_str);
    void updateCard(const QVariant &arg);
    void setPlayerSkillInvalidity(const QVariant &arg);
    void setShownHandCards(const QVariant &card_str);
    void setBrokenEquips(const QVariant &card_str);
    void setHiddenGenerals(const QVariant &arg);
    void setShownHiddenGeneral(const QVariant &arg);

    void fillAG(const QVariant &cards_str);
    void takeAG(const QVariant &take_str);
    void clearAG(const QVariant &);

    //interactive server callbacks
    void askForCardOrUseCard(const QVariant &);
    void askForAG(const QVariant &);
    void askForSinglePeach(const QVariant &);
    void askForCardShow(const QVariant &);
    void askForSkillInvoke(const QVariant &);
    void askForChoice(const QVariant &);
    void askForDiscard(const QVariant &);
    void askForExchange(const QVariant &);
    void askForSuit(const QVariant &);
    void askForKingdom(const QVariant &arg = QVariant());
    void askForNullification(const QVariant &);
    void askForPindian(const QVariant &);
    void askForCardChosen(const QVariant &);
    void askForPlayerChosen(const QVariant &);
    void askForGeneral(const QVariant &);
    void askForYiji(const QVariant &);
    void askForGuanxing(const QVariant &);
    void showAllCards(const QVariant &);
    void askForGongxin(const QVariant &);
    void askForAssign(const QVariant &); // Assign roles at the beginning of game
    void askForSurrender(const QVariant &);
    void askForLuckCard(const QVariant &);
    void askForTriggerOrder(const QVariant &);
    void handleGameEvent(const QVariant &);
    //3v3 & 1v1
    void askForOrder(const QVariant &);
    void askForRole3v3(const QVariant &);
    void askForDirection(const QVariant &);

    // 3v3 & 1v1 methods
    void fillGenerals(const QVariant &generals);
    void askForGeneral3v3(const QVariant &);
    void takeGeneral(const QVariant &take_str);
    void startArrange(const QVariant &to_arrange);

    void recoverGeneral(const QVariant &);
    void revealGeneral(const QVariant &);

    void attachSkill(const QVariant &skill);

    inline void setCountdown(QSanProtocol::Countdown countdown)
    {
        m_mutexCountdown.lock();
        m_countdown = countdown;
        m_mutexCountdown.unlock();
    }

    inline QSanProtocol::Countdown getCountdown()
    {
        m_mutexCountdown.lock();
        QSanProtocol::Countdown countdown = m_countdown;
        m_mutexCountdown.unlock();
        return countdown;
    }

    inline QList<int> getAvailableCards() const
    {
        return available_cards;
    }
    void clearHighlightSkillName();
    // public fields
    bool m_isDiscardActionRefusable;
    bool m_canDiscardEquip;
    bool m_noNullificationThisTime;
    QString m_noNullificationTrickName;
    int discard_num;
    int min_num;
    QString skill_name;
    QString highlight_skill_name; //for highlighting skill button when client is asked to use skill
    QString lord_name;

    QList<const Card *> discarded_list;
    QStringList players_to_choose;

public slots:
    void signup();
    void onPlayerChooseGeneral(const QString &_name);
    void onPlayerMakeChoice();
    void onPlayerChooseCard(int card_id = -2);
    void onPlayerChooseAG(int card_id);
    void onPlayerChoosePlayer(const Player *player);
    void onPlayerChooseOption(const QString &choice);
    void onPlayerChooseTriggerOrder(const QString &choice);
    void trust();
    void addRobot();
    void fillRobots();

    void onPlayerReplyGongxin(int card_id = -1);
    void changeSkin(const QString &name, int index);
    void preshow(const QString &skill_name, const bool isPreshowed); //, bool head

protected:
    // operation countdown
    QSanProtocol::Countdown m_countdown;
    // sync objects
    QMutex m_mutexCountdown;
    Status status;
    int alive_count;
    int swap_pile;

private:
    ClientSocket *socket;
    bool m_isGameOver;
    QHash<QSanProtocol::CommandType, Callback> m_interactions;
    QHash<QSanProtocol::CommandType, Callback> m_callbacks;
    QList<const ClientPlayer *> players;
    QStringList ban_packages;
    Recorder *recorder;
    Replayer *replayer;
    QTimer *heartbeatTimer;
    QTextDocument *lines_doc, *prompt_doc;
    int pile_num;
    QString skill_to_invoke;
    QList<int> available_cards;

    unsigned int _m_lastServerSerial;
    bool m_isObjectNameRecorded;

    void updatePileNum();
    QString setPromptList(const QStringList &text);
    QString _processCardPattern(const QString &pattern);
    void commandFormatWarning(const QString &str, const QRegExp &rx, const char *command);

    bool _loseSingleCard(int card_id, CardsMoveStruct move);
    bool _getSingleCard(int card_id, CardsMoveStruct move);

private slots:
    void processServerPacket(const QString &cmd);
    void processServerPacket(const char *cmd);
    bool processServerRequest(const QSanProtocol::Packet &packet);
    void processShowGeneral(const QSanProtocol::Packet &packet);
    void notifyRoleChange(const QString &new_role);
    void onPlayerChooseSuit();
    void onPlayerChooseKingdom();
    void alertFocus();
    void onPlayerChooseOrder();
    void onPlayerChooseRole3v3();

signals:
    void version_checked(const QString &version_number, const QString &mod_name);
    void server_connected();
    void error_message(const QString &msg);
    void player_added(ClientPlayer *new_player);
    void player_removed(const QString &player_name);
    // choice signal
    //void generals_got(const QStringList &generals);
    void generals_got(const QStringList &generals, const bool single_result, const bool can_convert);
    void kingdoms_got(const QStringList &kingdoms);
    void suits_got(const QStringList &suits);
    void options_got(const QString &skillName, const QStringList &options);
    void cards_got(const ClientPlayer *player, const QString &flags, const QString &reason, bool handcard_visible, Card::HandlingMethod method, QList<int> disabled_ids,
                   bool enableEmptyCard);
    void roles_got(const QString &scheme, const QStringList &roles);
    void directions_got();
    void orders_got(QSanProtocol::Game3v3ChooseOrderCommand reason);
    void triggers_got(const QVariantList &options, bool optional);

    void seats_arranged(const QList<const ClientPlayer *> &seats);
    void hp_changed(const QString &who, int delta, DamageStruct::Nature nature, bool losthp);
    void maxhp_changed(const QString &who, int delta);
    void status_changed(Client::Status oldStatus, Client::Status newStatus);
    void avatars_hiden();
    void pile_reset();
    void player_killed(const QString &who);
    void player_revived(const QString &who);
    void dashboard_death(const QString &who);
    void card_shown(const QString &player_name, int card_id);
    void log_received(const QStringList &log_str);
    void guanxing(const QList<int> &card_ids, bool single_side);
    void gongxin(const QList<int> &card_ids, bool enable_heart, QList<int> enabled_ids, QList<int> shownHandcard_ids);
    void focus_moved(const QStringList &focus, QSanProtocol::Countdown countdown);
    void emotion_set(const QString &target, const QString &emotion);
    void skill_invoked(const QString &who, const QString &skill_name);
    void skill_acquired(const ClientPlayer *player, const QString &skill_name, const bool &head);
    void animated(int name, const QStringList &args);
    void text_spoken(const QString &text);
    void line_spoken(const QString &line);
    void player_spoken(const QString &who, const QString &line);
    void skill_invalidity_changed(ClientPlayer *player);

    void card_used();

    void game_started();
    void game_over();
    void standoff();
    void event_received(const QVariant &);

    void move_cards_lost(int moveId, QList<CardsMoveStruct> moves);
    void move_cards_got(int moveId, QList<CardsMoveStruct> moves);

    void skill_attached(const QString &skill_name, bool from_left);
    void skill_detached(const QString &skill_name, bool head = true);
    void do_filter();

    void nullification_asked(bool asked);
    void surrender_enabled(bool enabled);

    void ag_filled(const QList<int> &card_ids, const QList<int> &disabled_ids, const QList<int> &shownHandcard_ids);
    void ag_taken(ClientPlayer *taker, int card_id, bool move_cards);
    void ag_cleared();

    void generals_filled(const QStringList &general_names);
    void general_taken(const QString &who, const QString &name, const QString &rule);
    void general_asked();
    void arrange_started(const QString &to_arrange);
    void general_recovered(int index, const QString &name);
    void general_revealed(bool self, const QString &general);

    void role_state_changed(const QString &state_str);
    void generals_viewed(const QString &reason, const QStringList &names);

    void assign_asked();
    void start_in_xs();

    void head_preshowed();
    void deputy_preshowed(); //hegemony
};

extern Client *ClientInstance;

#endif
