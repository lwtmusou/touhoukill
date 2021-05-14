#ifndef _GENERAL_SELECTOR_H
#define _GENERAL_SELECTOR_H

#include "lua-wrapper.h"

#include <QObject>

class ServerPlayer;
class Room;

class GeneralSelector : public QObject
{
    Q_OBJECT

public:
    explicit GeneralSelector(Room *room);

    QString selectFirst(ServerPlayer *player, const QStringList &candidates);
    QString selectSecond(ServerPlayer *player, const QStringList &candidates);
    QString selectPair(ServerPlayer *player, const QStringList &candidates);
    QString select3v3(ServerPlayer *player, const QStringList &candidates);
    QString select1v1(const QStringList &candidates);
    QStringList arrange3v3(ServerPlayer *player);
    QStringList arrange1v1(ServerPlayer *player);

private:
    void initialize();
    void callLuaInitialize();
};

#endif
