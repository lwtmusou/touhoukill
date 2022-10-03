#ifndef TOUHOUKILL_QSGSCORE_H
#define TOUHOUKILL_QSGSCORE_H

#ifndef SWIG
#include <QCoreApplication>
#include <QCryptographicHash>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QList>
#include <QMap>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QRandomGenerator>
#include <QRect>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QRegularExpressionMatchIterator>
#include <QSet>
#include <QSharedPointer>
#include <QString>
#include <QStringList>
#include <QTextStream>
#include <QThreadStorage>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QVersionNumber>
#include <QtGlobal>

// Check Qt version here
// We support Qt 5 only 5.15 and Qt 6 after 6.2
#if QT_VERSION_MAJOR <= 4
#error "no support for Qt 4 anymore."
#elif QT_VERSION_MAJOR == 5
#if QT_VERSION_MINOR < 15
#error "no support for Qt 5 before 5.15."
#endif
#elif QT_VERSION_MAJOR == 6
#if QT_VERSION_MINOR < 2
#error "no support for Qt 6 before 6.2."
#endif
#else
#pragma message "QSanguosha is not ported to Qt after Qt 6."
#endif

#include <algorithm>
#include <functional>
#include <optional>
#include <random>
#include <sstream>
#include <type_traits>
#endif

#include <lua.hpp>

#if !defined(QSGS_STATIC)
#ifdef BUILD_QSGSCORE
#define QSGS_CORE_EXPORT Q_DECL_EXPORT
#else
#define QSGS_CORE_EXPORT Q_DECL_IMPORT
#endif
#else
#define QSGS_CORE_EXPORT
#endif

#endif
