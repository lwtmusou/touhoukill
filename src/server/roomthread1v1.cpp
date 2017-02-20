#include "roomthread1v1.h"
#include "engine.h"
#include "generalselector.h"
#include "room.h"
#include "settings.h"

#include <QDateTime>

using namespace QSanProtocol;
using namespace JsonUtils;

RoomThread1v1::RoomThread1v1(Room *room)
    : room(room)
{
    room->getRoomState()->reset();
}

void RoomThread1v1::run()
{
    // initialize the random seed for this thread
    qsrand(QTime(0, 0, 0).secsTo(QTime::currentTime()));
    QString rule = Config.value("1v1/Rule", "2013").toString();
    int total_num = rule != "Classical" ? 12 : 10;

    //note: for touhouKill 1v1 mode,we always use ExtensionPackage (touhou Package)
    /*if (!Config.value("1v1/UsingExtension", false).toBool()) {
        const Package *stdpack = Sanguosha->findChild<const Package *>("standard");
        const Package *windpack = Sanguosha->findChild<const Package *>("wind");

        QStringList candidates;
        if (rule == "Classical") {
        foreach(const General *general, stdpack->findChildren<const General *>())
        candidates << general->objectName();
        foreach(const General *general, windpack->findChildren<const General *>())
        candidates << general->objectName();
        }
        else {
        candidates << "caocao" << "simayi" << "xiahoudun" << "kof_zhangliao"
        << "kof_xuchu" << "guojia" << "kof_zhenji" << "kof_xiahouyuan"
        << "nos_caoren" << "dianwei" << "kof_guanyu" << "zhangfei"
        << "zhugeliang" << "zhaoyun" << "kof_machao" << "kof_nos_huangyueying"
        << "kof_huangzhong" << "kof_jiangwei" << "kof_menghuo" << "kof_zhurong"
        << "sunquan" << "ganning" << "huanggai" << "zhouyu"
        << "luxun" << "kof_sunshangxiang" << "sunjian" << "xiaoqiao"
        << "lvbu" << "kof_nos_diaochan" << "yanliangwenchou" << "hejin"
        << "niujin" << "hansui";
        if (rule == "OL") {
        candidates << "kof_liubei" << "kof_weiyan" << "kof_lvmeng" << "kof_daqiao"
        << "nos_zhoutai" << "kof_huatuo" << "nos_zhangjiao" << "kof_pangde";
        }
        }
        qShuffle(candidates);
        general_names = candidates.mid(0, total_num);
        }
        else {
        QSet<QString> banset = Config.value("Banlist/1v1").toStringList().toSet();
        general_names = Sanguosha->getRandomGenerals(total_num, banset);
        }*/
    QSet<QString> banset = Config.value("Banlist/1v1").toStringList().toSet();
    general_names = Sanguosha->getRandomGenerals(total_num, banset);

    if (rule == "Classical") {
        QStringList known_list = general_names.mid(0, 6);
        unknown_list = general_names.mid(6, 4);

        for (int i = 0; i < 4; i++)
            general_names[i + 6] = QString("x%1").arg(QString::number(i));

        room->doBroadcastNotify(S_COMMAND_FILL_GENERAL, toJsonArray(known_list << "x0"
                                                                               << "x1"
                                                                               << "x2"
                                                                               << "x3"));
    } else if (rule == "2013") {
        room->doBroadcastNotify(S_COMMAND_FILL_GENERAL, toJsonArray(general_names));
    } else if (rule == "OL") {
        QStringList known_list = general_names.mid(0, 6);
        unknown_list = general_names.mid(6, 6);

        for (int i = 0; i < 6; i++)
            general_names[i + 6] = QString("x%1").arg(QString::number(i));

        room->doBroadcastNotify(S_COMMAND_FILL_GENERAL, toJsonArray(known_list << "x0"
                                                                               << "x1"
                                                                               << "x2"
                                                                               << "x3"
                                                                               << "x4"
                                                                               << "x5"));
    }

    int index = qrand() % 2;
    ServerPlayer *first = room->getPlayers().at(index), *next = room->getPlayers().at(1 - index);
    QString order = room->askForOrder(first, "warm");
    if (order == "cool")
        qSwap(first, next);
    first->setRole("lord");
    next->setRole("renegade");

    room->broadcastProperty(first, "role");
    room->setPlayerProperty(first, "role_shown", true);
    room->broadcastProperty(next, "role");
    room->setPlayerProperty(next, "role_shown", true);
    room->adjustSeats();

    if (rule == "OL") {
        takeGeneral(first, "x0");
        takeGeneral(first, "x2");
        takeGeneral(first, "x4");
        takeGeneral(next, "x1");
        takeGeneral(next, "x3");
        takeGeneral(next, "x5");
    }

    askForTakeGeneral(first);

    while (general_names.length() > 1) {
        qSwap(first, next);

        askForTakeGeneral(first);
        askForTakeGeneral(first);
    }
    askForTakeGeneral(next);

    if (rule == "OL")
        askForFirstGeneral(QList<ServerPlayer *>() << first << next);
    else
        startArrange(QList<ServerPlayer *>() << first << next);
}

void RoomThread1v1::askForTakeGeneral(ServerPlayer *player)
{
    room->tryPause();

    QString name;
    if (general_names.length() == 1)
        name = general_names.first();
    else if (player->getState() != "online")
        name = room->generalSelector()->select1v1(general_names);

    if (name.isNull()) {
        bool success = room->doRequest(player, S_COMMAND_ASK_GENERAL, QVariant(), true);
        QVariant clientReply = player->getClientReply();
        if (success && isString(clientReply)) {
            name = clientReply.toString();
            takeGeneral(player, name);
        } else {
            GeneralSelector *selector = room->generalSelector();
            name = selector->select1v1(general_names);
            takeGeneral(player, name);
        }
    } else {
        msleep(Config.AIDelay);
        takeGeneral(player, name);
    }
}

void RoomThread1v1::takeGeneral(ServerPlayer *player, const QString &name)
{
    QString rule = Config.value("1v1/Rule", "2013").toString();
    QString group = player->isLord() ? "warm" : "cool";
    room->doBroadcastNotify(room->getOtherPlayers(player, true), S_COMMAND_TAKE_GENERAL, JsonArray() << group << name << rule);

    QRegExp unknown_rx("x(\\d)");
    QString general_name = name;
    if (unknown_rx.exactMatch(name)) {
        int index = unknown_rx.capturedTexts().at(1).toInt();
        general_name = unknown_list.at(index);

        JsonArray arg;
        arg << index;
        arg << general_name;
        room->doNotify(player, S_COMMAND_RECOVER_GENERAL, arg);
    }

    room->doNotify(player, S_COMMAND_TAKE_GENERAL, JsonArray() << group << general_name << rule);

    QString namearg = unknown_rx.exactMatch(name) ? "anjiang" : name;
    foreach (ServerPlayer *p, room->getPlayers()) {
        LogMessage log;
        log.type = "#VsTakeGeneral";
        log.arg = group;
        log.arg2 = (p == player) ? general_name : namearg;
        room->doNotify(p, S_COMMAND_LOG_SKILL, log.toJsonValue());
    }

    general_names.removeOne(name);
    player->addToSelected(general_name);
}

void RoomThread1v1::startArrange(QList<ServerPlayer *> players)
{
    room->tryPause();
    QList<ServerPlayer *> online = players;
    foreach (ServerPlayer *player, players) {
        if (!player->isOnline()) {
            GeneralSelector *selector = room->generalSelector();
            arrange(player, selector->arrange1v1(player));
            online.removeOne(player);
        }
    }
    if (online.isEmpty())
        return;

    foreach (ServerPlayer *player, online)
        player->m_commandArgs = QVariant();

    room->doBroadcastRequest(online, S_COMMAND_ARRANGE_GENERAL);

    foreach (ServerPlayer *player, online) {
        QVariant clientReply = player->getClientReply();
        JsonArray arr = clientReply.value<JsonArray>();
        if (player->m_isClientResponseReady && arr.size() == 3) {
            QStringList arranged;
            tryParse(clientReply, arranged);
            arrange(player, arranged);
        } else {
            GeneralSelector *selector = room->generalSelector();
            arrange(player, selector->arrange1v1(player));
        }
    }
}

void RoomThread1v1::askForFirstGeneral(QList<ServerPlayer *> players)
{
    room->tryPause();
    QList<ServerPlayer *> online = players;
    foreach (ServerPlayer *player, players) {
        if (!player->isOnline()) {
            GeneralSelector *selector = room->generalSelector();
            QStringList arranged = player->getSelected();
            QStringList selected = selector->arrange1v1(player);
            selected.append(arranged);
            selected.removeDuplicates();
            arrange(player, selected);
            online.removeOne(player);
        }
    }
    if (online.isEmpty())
        return;

    foreach (ServerPlayer *player, online)
        player->m_commandArgs = toJsonArray(player->getSelected());

    room->doBroadcastRequest(online, S_COMMAND_CHOOSE_GENERAL);

    foreach (ServerPlayer *player, online) {
        QVariant clientReply = player->getClientReply();
        if (player->m_isClientResponseReady && isString(clientReply) && player->getSelected().contains(clientReply.toString())) {
            QStringList arranged = player->getSelected();
            QString first_gen = clientReply.toString();
            arranged.removeOne(first_gen);
            arranged.prepend(first_gen);
            arrange(player, arranged);
        } else {
            GeneralSelector *selector = room->generalSelector();
            QStringList arranged = player->getSelected();
            QStringList selected = selector->arrange1v1(player);
            selected.append(arranged);
            selected.removeDuplicates();
            arrange(player, selected);
        }
    }
}

void RoomThread1v1::arrange(ServerPlayer *player, const QStringList &arranged)
{
    QString rule = Config.value("1v1/Rule", "2013").toString();
    Q_ASSERT(arranged.length() == ((rule == "OL") ? 6 : 3));

    QStringList left = arranged.mid(1);
    player->tag["1v1Arrange"] = QVariant::fromValue(left);
    player->setGeneralName(arranged.first());

    foreach (QString general, arranged) {
        room->doNotify(player, S_COMMAND_REVEAL_GENERAL, JsonArray() << player->objectName() << general);
        if (rule != "Classical")
            break;
    }
}
