#ifndef QSANGUOSHA_QSGSCORE_H
#define QSANGUOSHA_QSGSCORE_H

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

#include <algorithm>
#include <functional>
#include <optional>
#include <random>
#include <sstream>
#include <type_traits>
#endif

#include <lua.hpp>

#if !defined(SWIG) && !defined(QSGS_STATIC)
#ifdef BUILD_QSGSCORE
#define QSGS_CORE_EXPORT Q_DECL_EXPORT
#else
#define QSGS_CORE_EXPORT Q_DECL_IMPORT
#endif
#else
#define QSGS_CORE_EXPORT
#endif

#endif
