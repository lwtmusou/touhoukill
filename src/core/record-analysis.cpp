#include "record-analysis.h"
#include "engine.h"
#include "recorder.h"
#include "settings.h"

#include "json.h"
#include <QFile>
#include <QMessageBox>

using namespace QSanProtocol;

RecAnalysis::RecAnalysis(QString dir)
    : m_recordPlayers(0)
    , m_currentPlayer(NULL)
{
    initialize(dir);
}

void RecAnalysis::initialize(QString dir)
{
    QList<QByteArray> records_line;
    if (dir.isEmpty()) {
        records_line = ClientInstance->getRecords();
    } else {
        QFile file(dir);
        if (file.open(QIODevice::ReadOnly)) {
            char header;
            file.getChar(&header);
            if (header == 0) {
                QByteArray lines = file.readAll();
                lines = qUncompress(lines);
                records_line = lines.split('\n');
            } else {
                file.ungetChar(header);
                while (!file.atEnd())
                    records_line << file.readLine();
            }
        }
    }
    records_line.removeAll(QByteArray());

    QStringList role_list;
    foreach (const QByteArray &_line, records_line) {
        QByteArray line = _line;
        line.remove(0, line.indexOf(' '));

        Packet packet;
        if (!packet.parse(line))
            continue;

        if (packet.getCommandType() == S_COMMAND_SETUP) {
            const QVariant &body = packet.getMessageBody();
            if (JsonUtils::isString(body)) {
                QString l = body.toString();
                QRegExp rx("(.*):(@?\\w+):(\\d+):(\\d+):([+\\w-]*):([RCFSTBHAMN123a-r]*)(\\s+)?");
                if (!rx.exactMatch(l))
                    continue;

                QStringList texts = rx.capturedTexts();
                m_recordGameMode = texts.at(2);
                m_recordPlayers = texts.at(2).split("_").first().remove(QRegExp("[^0-9]")).toInt();
                QStringList ban_packages = texts.at(5).split("+");
                foreach (const Package *package, Sanguosha->getPackages()) {
                    if (!ban_packages.contains(package->objectName()) && Sanguosha->getScenario(package->objectName()) == NULL)
                        m_recordPackages << Sanguosha->translate(package->objectName());
                }

                QString flags = texts.at(6);
                if (flags.contains("R"))
                    m_recordServerOptions << tr("RandomSeats");
                if (flags.contains("C"))
                    m_recordServerOptions << tr("EnableCheat");
                if (flags.contains("F"))
                    m_recordServerOptions << tr("FreeChoose");
                if (flags.contains("S"))
                    m_recordServerOptions << tr("Enable2ndGeneral");
                if (flags.contains("T"))
                    m_recordServerOptions << tr("EnableSame");
                if (flags.contains("B"))
                    m_recordServerOptions << tr("EnableBasara");
                if (flags.contains("H"))
                    m_recordServerOptions << tr("EnableHegemony");
                if (flags.contains("A"))
                    m_recordServerOptions << tr("EnableAI");

                continue;
            }
        }

        if (packet.getCommandType() == S_COMMAND_ARRANGE_SEATS) {
            role_list.clear();
            JsonUtils::tryParse(packet.getMessageBody(), role_list);
            continue;
        }

        if (packet.getCommandType() == S_COMMAND_ADD_PLAYER) {
            JsonArray body = packet.getMessageBody().value<JsonArray>();
            if (body.size() >= 2) {
                getPlayer(body[0].toString())->m_screenName = body[1].toString();
            }
            continue;
        }

        if (packet.getCommandType() == S_COMMAND_REMOVE_PLAYER) {
            QString name = packet.getMessageBody().toString();
            m_recordMap.remove(name);
            continue;
        }

        if (packet.getCommandType() == S_COMMAND_SET_PROPERTY) {
            QStringList self_info;
            if (!JsonUtils::tryParse(packet.getMessageBody(), self_info) || self_info.size() < 3)
                continue;

            const QString &who = self_info.at(0);
            const QString &property = self_info.at(1);
            const QString &value = self_info.at(2);

            if (who == S_PLAYER_SELF_REFERENCE_ID) {
                if (property == "objectName") {
                    getPlayer(value, S_PLAYER_SELF_REFERENCE_ID)->m_screenName = Config.UserName;
                } else if (property == "role") {
                    getPlayer(S_PLAYER_SELF_REFERENCE_ID)->m_role = value;
                } else if (property == "general") {
                    getPlayer(S_PLAYER_SELF_REFERENCE_ID)->m_generalName = value;
                } else if (property == "general2") {
                    getPlayer(S_PLAYER_SELF_REFERENCE_ID)->m_general2Name = value;
                }
            } else {
                PlayerRecordStruct *record = getPlayer(who);
                if (record == NULL)
                    continue;

                if (self_info.at(1) == "general") {
                    record->m_generalName = value;
                } else if (self_info.at(1) == "general2") {
                    record->m_general2Name = value;
                } else if (self_info.at(1) == "state" && value == "robot") {
                    record->m_statue = "robot";
                }
            }

            continue;
        }

        if (packet.getCommandType() == S_COMMAND_SET_MARK) {
            JsonArray args = packet.getMessageBody().value<JsonArray>();
            if (args.size() != 3)
                continue;

            QString who = args.at(0).toString();
            QString mark = args.at(1).toString();
            int num = args.at(2).toInt();
            if (mark == "Global_TurnCount") {
                PlayerRecordStruct *rec = getPlayer(who);
                if (rec) {
                    rec->m_turnCount = num;
                    m_currentPlayer = rec;
                }
            }

            continue;
        }

        if (packet.getCommandType() == S_COMMAND_SPEAK) {
            JsonArray body = packet.getMessageBody().value<JsonArray>();
            if (body.size() < 2) {
                continue;
            }

            QString speaker = body[0].toString();
            QString words = body[1].toString();
            m_recordChat += getPlayer(speaker)->m_screenName + ": " + words;
            m_recordChat.append("<br/>");

            continue;
        }

        if (packet.getCommandType() == S_COMMAND_CHANGE_HP) {
            JsonArray change = packet.getMessageBody().value<JsonArray>();
            if (change.size() != 3 || !JsonUtils::isString(change[0]) || !JsonUtils::isNumber(change[1]) || !JsonUtils::isNumber(change[2]))
                continue;

            QString name = change[0].toString();
            int hp_change = change[1].toInt();

            /*int nature_index = change[2].toInt();
            DamageStruct::Nature nature = DamageStruct::Normal;
            if (nature_index > 0) nature = (DamageStruct::Nature)nature_index;*/

            if (hp_change > 0)
                getPlayer(name)->m_recover += hp_change;

            continue;
        }

        if (packet.getCommandType() == S_COMMAND_LOG_SKILL) {
            QStringList log;
            if (!JsonUtils::tryParse(packet.getMessageBody(), log) || log.size() != 6)
                continue;

            const QString &type = log.at(0);
            const QString &from = log.at(1);
            QStringList tos = log.at(2).split('+');
            //const QString &card_str = log.at(3);
            const QString arg = log.at(4);
            //const QString arg2 = log.at(5);

            if (type.startsWith("#Damage")) {
                int damage = arg.toInt();

                if (!from.isEmpty())
                    getPlayer(from)->m_damage += damage;
                getPlayer(tos.first())->m_damaged += damage;
                continue;
            }

            if (type == "#Murder" || type == "#Suicide") {
                getPlayer(from)->m_kill++;
                getPlayer(tos.first())->m_isAlive = false;
                continue;
            }

            if (type == "#Contingency") {
                getPlayer(tos.first())->m_isAlive = false;
                continue;
            }
        }
    }

    QByteArray last_line = records_line.last();
    last_line.remove(0, last_line.indexOf(' '));

    Packet gameover_packet;
    gameover_packet.parse(last_line);
    if (gameover_packet.getCommandType() == S_COMMAND_GAME_OVER) {
        JsonArray args = gameover_packet.getMessageBody().value<JsonArray>();
        if (args.size() == 2) {
            QString winners = args.at(0).toString();
            m_recordWinners = winners.split("+");

            QStringList roles_order;
            JsonUtils::tryParse(args.at(1), roles_order);
            for (int i = 0; i < role_list.length(); i++)
                getPlayer(role_list.at(i))->m_role = roles_order.at(i);
        }
    }
}

RecAnalysis::~RecAnalysis()
{
    foreach (PlayerRecordStruct *s, m_recordMap)
        delete s;
}

PlayerRecordStruct *RecAnalysis::getPlayerRecord(const Player *player) const
{
    if (m_recordMap.keys().contains(player->objectName()))
        return m_recordMap[player->objectName()];
    else
        return NULL;
}

QMap<QString, PlayerRecordStruct *> RecAnalysis::getRecordMap() const
{
    return m_recordMap;
}

QStringList RecAnalysis::getRecordPackages() const
{
    return m_recordPackages;
}

QStringList RecAnalysis::getRecordWinners() const
{
    return m_recordWinners;
}

QString RecAnalysis::getRecordGameMode() const
{
    return m_recordGameMode;
}

QStringList RecAnalysis::getRecordServerOptions() const
{
    return m_recordServerOptions;
}

QString RecAnalysis::getRecordChat() const
{
    return m_recordChat;
}

PlayerRecordStruct *RecAnalysis::getPlayer(QString object_name, const QString &addition_name)
{
    if (m_recordMap.keys().contains(addition_name)) {
        m_recordMap[object_name] = m_recordMap[addition_name];
        m_recordMap[object_name]->m_additionName = addition_name;
        m_recordMap.remove(addition_name);
    } else if (!m_recordMap.keys().contains(addition_name) && !addition_name.isEmpty()) {
        m_recordMap[object_name] = new PlayerRecordStruct;
        m_recordMap[object_name]->m_additionName = addition_name;
    } else if (!m_recordMap.keys().contains(object_name)) {
        bool inQueue = false;
        foreach (QString name, m_recordMap.keys()) {
            if (m_recordMap[name]->m_additionName == object_name) {
                object_name = name;
                inQueue = true;
                break;
            }
        }

        if (!inQueue)
            m_recordMap[object_name] = new PlayerRecordStruct;
    }

    return m_recordMap[object_name];
}

unsigned int RecAnalysis::findPlayerOfDamage(int n) const
{
    int result = 0;
    foreach (PlayerRecordStruct *s, m_recordMap.values()) {
        if (s->m_damage >= n)
            result++;
        result *= 2;
    }
    return result / 2;
}

unsigned int RecAnalysis::findPlayerOfDamaged(int n) const
{
    int result = 0;
    foreach (PlayerRecordStruct *s, m_recordMap.values()) {
        if (s->m_damaged >= n)
            result++;
        result *= 2;
    }
    return result / 2;
}

unsigned int RecAnalysis::findPlayerOfKills(int n) const
{
    int result = 0;
    foreach (PlayerRecordStruct *s, m_recordMap.values()) {
        if (s->m_kill >= n)
            result++;
        result *= 2;
    }
    return result / 2;
}

unsigned int RecAnalysis::findPlayerOfRecover(int n) const
{
    int result = 0;
    foreach (PlayerRecordStruct *s, m_recordMap.values()) {
        if (s->m_recover >= n)
            result++;
        result *= 2;
    }
    return result / 2;
}

unsigned int RecAnalysis::findPlayerOfDamage(int upper, int lower) const
{
    int result = 0;
    foreach (PlayerRecordStruct *s, m_recordMap.values()) {
        if (s->m_damage >= upper && s->m_damage <= lower)
            result++;
        result *= 2;
    }
    return result / 2;
}

unsigned int RecAnalysis::findPlayerOfDamaged(int upper, int lower) const
{
    int result = 0;
    foreach (PlayerRecordStruct *s, m_recordMap.values()) {
        if (s->m_damaged >= upper && s->m_damaged <= lower)
            result++;
        result *= 2;
    }
    return result / 2;
}

unsigned int RecAnalysis::findPlayerOfRecover(int upper, int lower) const
{
    int result = 0;
    foreach (PlayerRecordStruct *s, m_recordMap.values()) {
        if (s->m_recover >= upper && s->m_recover <= lower)
            result++;
        result *= 2;
    }
    return result / 2;
}

unsigned int RecAnalysis::findPlayerOfKills(int upper, int lower) const
{
    int result = 0;
    foreach (PlayerRecordStruct *s, m_recordMap.values()) {
        if (s->m_kill >= upper && s->m_kill <= lower)
            result++;
        result *= 2;
    }
    return result / 2;
}

PlayerRecordStruct::PlayerRecordStruct()
    : m_statue("online")
    , m_turnCount(0)
    , m_recover(0)
    , m_damage(0)
    , m_damaged(0)
    , m_kill(0)
    , m_isAlive(true)
{
}

bool PlayerRecordStruct::isNull()
{
    return m_screenName.isEmpty() || m_generalName.isEmpty();
}
