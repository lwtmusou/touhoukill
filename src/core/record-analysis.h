#ifndef _RECORD_ANALYSIS_H
#define _RECORD_ANALYSIS_H

#include "client.h"
#include "engine.h"
#include "serverplayer.h"

#include <QObject>

struct PlayerRecordStruct;

class RecAnalysis : public QObject
{
    Q_OBJECT

public:
    explicit RecAnalysis(QString dir = QString());
    ~RecAnalysis();

    static const unsigned int M_ALL_PLAYER = 0xFFFF;
    void initialize(QString dir = QString());
    PlayerRecordStruct *getPlayerRecord(const Player *player) const;
    QMap<QString, PlayerRecordStruct *> getRecordMap() const;
    QStringList getRecordPackages() const;
    QStringList getRecordWinners() const;
    QString getRecordGameMode() const;
    QStringList getRecordServerOptions() const;
    QString getRecordChat() const;

private:
    PlayerRecordStruct *getPlayer(QString object_name, const QString &addition_name = QString());
    unsigned int findPlayerOfDamage(int n) const;
    unsigned int findPlayerOfDamaged(int n) const;
    unsigned int findPlayerOfKills(int n) const;
    unsigned int findPlayerOfRecover(int n) const;
    unsigned int findPlayerOfDamage(int upper, int lower) const;
    unsigned int findPlayerOfDamaged(int upper, int lower) const;
    unsigned int findPlayerOfKills(int upper, int lower) const;
    unsigned int findPlayerOfRecover(int upper, int lower) const;

    QMap<QString, PlayerRecordStruct *> m_recordMap;
    QStringList m_recordPackages, m_recordWinners;
    QString m_recordGameMode;
    QStringList m_recordServerOptions;
    QString m_recordChat;
    int m_recordPlayers;
    PlayerRecordStruct *m_currentPlayer;

    mutable QStringList m_tempSatisfiedObject;
};

struct PlayerRecordStruct
{
    PlayerRecordStruct();

    bool isNull();

    QString m_additionName;
    QString m_generalName, m_general2Name;
    QString m_screenName;
    QString m_statue;
    QString m_role;
    int m_turnCount;
    int m_recover;
    int m_damage;
    int m_damaged;
    int m_kill;
    bool m_isAlive;
};

#endif
