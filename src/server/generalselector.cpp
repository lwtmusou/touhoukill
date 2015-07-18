#include "generalselector.h"
#include "engine.h"
#include "serverplayer.h"

#include <QFile>
#include <QTextStream>
#include <qmath.h>

static GeneralSelector *Selector;

GeneralSelector *GeneralSelector::getInstance()
{
    if (Selector == NULL) {
        Selector = new GeneralSelector;
        //@todo: this setParent is illegitimate in QT and is equivalent to calling
        // setParent(NULL). So taking it off at the moment until we figure out
        // a way to do it.
        //Selector->setParent(Sanguosha);
    }

    return Selector;
}

GeneralSelector::GeneralSelector()
{
    loadFirstGeneralTable();
    loadSecondGeneralTable();
    load3v3Table();
    load1v1Table();
}

//GeneralSelect score
QString GeneralSelector::selectFirst(ServerPlayer *player, const QStringList &candidates)
{
    QMap<QString, qreal> values;
    QString role = player->getRole();
    ServerPlayer *lord = player->getRoom()->getLord();

    QString seat_place;
    if (player->getSeat() - 1 <= 2)
        seat_place = "1";
    else if (player->getRoom()->alivePlayerCount() - player->getSeat() <= 2)
        seat_place = "3";
    else
        seat_place = "2";

    foreach (QString candidate, candidates) {
        qreal value = 5.0;
        const General *general = Sanguosha->getGeneral(candidate);
        if (role == "loyalist" &&
            (general->getKingdom() == lord->getKingdom() || general->getKingdom() == "god"
            || general->getKingdom() == "zhu" || general->getKingdom() == "touhougod"))
            value *= 1.04;
        if (role == "rebel" && lord->getGeneral() && lord->getGeneral()->hasSkill("xueyi")
            && general->getKingdom() == "qun")
            value *= 0.8;
        if (role != "loyalist" && lord->getGeneral() && lord->getGeneral()->hasSkill("shichou")
            && general->getKingdom() == "shu")
            value *= 0.1;
        QString key = QString("_:%1:%2").arg(candidate).arg(role);
        value *= qPow(1.1, first_general_table.value(key, 0.0));
        QString key2 = QString("%1:%2:%3").arg(lord->getGeneralName()).arg(candidate).arg(role);
        value *= qPow(1.1, first_general_table.value(key2, 0.0));

        //considering seat
        QString seat_key = QString("_:%1:%2").arg(candidate).arg(role);
        QString seat_str = general_seat_table.value(seat_key, "_");
        if (seat_str != "_") {
            QStringList seat_strs = seat_str.split(":");
            if (seat_strs.contains(seat_place + "0"))
                value *= 1.2;
            else if (seat_strs.contains(seat_place + "1"))
                value *= 0.8;
        }
        QString seat_key2 = QString("%1:%2:%3").arg(lord->getGeneralName()).arg(candidate).arg(role);
        QString seat_str2 = general_seat_table.value(seat_key2, "_");
        if (seat_str2 != "_") {
            QStringList seat_strs2 = seat_str2.split(":");
            if (seat_strs2.contains(seat_place + "0"))
                value *= 1.2;
            else if (seat_strs2.contains(seat_place + "1"))
                value *= 0.8;
        }



        values.insert(candidate, value);
    }
    QStringList _candidates = candidates;
    QStringList choice_list;
    while (!_candidates.isEmpty() && choice_list.length() < 6) {
        qreal max = -1;
        QString choice = QString();
        foreach (QString candidate, _candidates) {
            qreal value = values.value(candidate, 5.0);
            if (value > max) {
                max = value;
                choice = candidate;
            }
        }
        choice_list << choice;
        _candidates.removeOne(choice);
    }
    QString max_general;

    int rnd = qrand() % 100;
    int total = choice_list.length();
    int prob[6] = { 70, 85, 92, 95, 97, 99 };
    for (int i = 0; i < 6; i++) {
        if (rnd <= prob[i] || total <= i + 1) {
            max_general = choice_list.at(i).split(":").at(0);
            break;
        }
    }

    Q_ASSERT(!max_general.isEmpty());

    return max_general;
}

QString GeneralSelector::selectSecond(ServerPlayer *player, const QStringList &candidates)
{
    QString first = player->getGeneralName();

    int max = -1;
    QString max_general;

    foreach (QString candidate, candidates) {
        QString key = QString("%1+%2").arg(first).arg(candidate);
        int value = second_general_table.value(key, 0);
        if (value == 0) {
            key = QString("%1+%2").arg(candidate).arg(first);
            value = second_general_table.value(key, 3);
        }

        if (value > max) {
            max = value;
            max_general = candidate;
        }
    }

    Q_ASSERT(!max_general.isEmpty());

    return max_general;
}

QString GeneralSelector::select3v3(ServerPlayer *, const QStringList &candidates)
{
    return selectHighest(priority_3v3_table, candidates, 5);
}

QString GeneralSelector::select1v1(const QStringList &candidates)
{
    return selectHighest(priority_1v1_table, candidates, 5);
}

QString GeneralSelector::selectHighest(const QHash<QString, int> &table, const QStringList &candidates, int default_value)
{
    int max = -1;
    QString max_general;

    foreach (QString candidate, candidates) {
        int value = table.value(candidate, default_value);

        if (value > max) {
            max = value;
            max_general = candidate;
        }
    }

    Q_ASSERT(!max_general.isEmpty());

    return max_general;
}

static bool CompareByMaxHp(const QString &a, const QString &b)
{
    const General *g1 = Sanguosha->getGeneral(a);
    const General *g2 = Sanguosha->getGeneral(b);

    return g1->getMaxHp() < g2->getMaxHp();
}

QStringList GeneralSelector::arrange3v3(ServerPlayer *player)
{
    QStringList arranged = player->getSelected();
    qShuffle(arranged);
    arranged = arranged.mid(0, 3);

    qSort(arranged.begin(), arranged.end(), CompareByMaxHp);
    arranged.swap(0, 1);

    return arranged;
}

static bool CompareFunction(const QString &first, const QString &second)
{
    return Selector->get1v1ArrangeValue(first) > Selector->get1v1ArrangeValue(second);
}

int GeneralSelector::get1v1ArrangeValue(const QString &name)
{
    int value = priority_1v1_table.value(name, 5);
    if (sacrifice.contains(name))
        value += 1000;
    return value;
}

QStringList GeneralSelector::arrange1v1(ServerPlayer *player)
{
    QStringList arranged = player->getSelected();
    qSort(arranged.begin(), arranged.end(), CompareFunction);

    QStringList result;
    int i;
    for (i = 0; i < 3; i++) {
        if (get1v1ArrangeValue(arranged[i]) > 1000) {
            result << arranged[i];
            break;
        }
    }
    if (!result.isEmpty()) {
        int strong = (i == 0) ? 1 : 0;
        int weak = (i == 2) ? 1 : 2;
        result << arranged[weak] << arranged[strong];
    } else {
        result << arranged[1] << arranged[2] << arranged[0];
    }

    return result;
}

void GeneralSelector::loadFirstGeneralTable()
{
    loadFirstGeneralTable("loyalist");
    loadFirstGeneralTable("rebel");
    loadFirstGeneralTable("renegade");
    loadGeneralSeatTable();
}

void GeneralSelector::loadFirstGeneralTable(const QString &role)
{
    QFile file(QString("etc/%1.txt").arg(role));
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString lord_name, name;
            stream >> lord_name >> name;
            qreal value;
            stream >> value;
            QString key = QString("%1:%2:%3").arg(lord_name).arg(name).arg(role);
            first_general_table.insert(key, value);
        }
        file.close();
    }
}




void GeneralSelector::loadSecondGeneralTable()
{
    QRegExp rx("(\\w+)\\s+(\\w+)\\s+(\\d+)");
    QFile file("etc/double-generals.txt");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (!rx.exactMatch(line))
                continue;

            QStringList texts = rx.capturedTexts();
            QString first = texts.at(1);
            QString second = texts.at(2);
            int value = texts.at(3).toInt();

            QString key = QString("%1+%2").arg(first).arg(second);
            second_general_table.insert(key, value);
        }

        file.close();
    }
}

void GeneralSelector::load3v3Table()
{
    QFile file("etc/3v3-priority.txt");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString name;
            int priority;

            stream >> name >> priority;

            priority_3v3_table.insert(name, priority);
        }

        file.close();
    }
}

void GeneralSelector::load1v1Table()
{
    QRegExp rx("(\\w+)\\s+(\\d+)\\s*(\\*)?");
    QFile file("etc/1v1-priority.txt");
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            if (!rx.exactMatch(line))
                continue;

            QStringList texts = rx.capturedTexts();
            QString name = texts.at(1);
            int priority = texts.at(2).toInt();

            priority_1v1_table.insert(name, priority);

            if (!texts.at(3).isEmpty())
                sacrifice << name;
        }

        file.close();
    }
}

void GeneralSelector::loadGeneralSeatTable()
{
    QFile file(QString("etc/seat.txt"));
    if (file.open(QIODevice::ReadOnly)) {
        QTextStream stream(&file);
        while (!stream.atEnd()) {
            QString lord_name, name, loyalist_seat, rebel_seat, renegade_seat;
            stream >> lord_name >> name >> loyalist_seat >> rebel_seat >> renegade_seat;
            QString key1 = QString("%1:%2:%3").arg(lord_name).arg(name).arg("loyalist");
            general_seat_table.insert(key1, loyalist_seat);
            QString key2 = QString("%1:%2:%3").arg(lord_name).arg(name).arg("rebel");
            general_seat_table.insert(key2, rebel_seat);
            QString key3 = QString("%1:%2:%3").arg(lord_name).arg(name).arg("renegade");
            general_seat_table.insert(key3, renegade_seat);
        }
        file.close();
    }
}