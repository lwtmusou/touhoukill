#include "generalselector.h"

#include "room.h"

#include <QApplication>
#include <QMessageBox>

GeneralSelector::GeneralSelector(Room *room)
    : QObject(room)
{
}

QString GeneralSelector::selectFirst(ServerPlayer *player, const QStringList &candidates)
{
    return QString();
}

QString GeneralSelector::selectSecond(ServerPlayer *player, const QStringList &candidates)
{
    return QString();
}

QString GeneralSelector::selectPair(ServerPlayer *player, const QStringList &candidates)
{
    return QString();
}

QString GeneralSelector::select3v3(ServerPlayer *player, const QStringList &candidates)
{
    return QString();
}

QString GeneralSelector::select1v1(const QStringList &candidates)
{
    return QString();
}

QStringList GeneralSelector::arrange3v3(ServerPlayer *player)
{
    return QStringList();
}

QStringList GeneralSelector::arrange1v1(ServerPlayer *player)
{
    return QStringList();
}
