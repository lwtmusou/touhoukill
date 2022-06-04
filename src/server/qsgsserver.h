#ifndef TOUHOUKILL_QSGSSERVER_H
#define TOUHOUKILL_QSGSSERVER_H

#include "qsgscore.h"

#include <QAbstractSocket>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QGlobalStatic>
#include <QHostAddress>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QLocalServer>
#include <QLocalSocket>
#include <QString>
#include <QTcpServer>
#include <QTcpSocket>

#if !((defined(Q_OS_DARWIN) && !defined(Q_OS_MACOS)) || defined(Q_OS_ANDROID))
#include <QCommandLineOption>
#include <QCommandLineParser>
#endif

extern int startServer();

#endif
