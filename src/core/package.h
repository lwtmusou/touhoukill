#ifndef _PACKAGE_H
#define _PACKAGE_H

#include "global.h"

#include <QCoreApplication>
#include <QHash>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QStringList>

class Skill;
class Player;
class Card;
class CardFace;
struct CardDescriptor;
class General;

class CardPattern
{
public:
    virtual bool match(const Player *player, const Card *card) const = 0;
    virtual bool willThrow() const
    {
        return true;
    }
};

class PackagePrivate;

class Package final
{
public:
    explicit Package(const QString &name, QSanguosha::PackageType pack_type = QSanguosha::GeneralPack);
    ~Package();

    const QString &name() const;

    const QList<const Skill *> &skills() const;
    const QMap<QString, const CardPattern *> &patterns() const;
    const QMultiMap<QString, QString> &relatedSkills() const;

    QSanguosha::PackageType type() const;

    void insertRelatedSkills(const QString &main_skill, const QString &related_skill);
    void insertPattern(const QString &name, const CardPattern *cardPattern);

    const QList<const CardFace *> &cardFaces() const;
    const QList<CardDescriptor> &cards() const;
    const QList<General *> &generals() const;

    Package &operator<<(const CardFace *face); // register face.
    Package &operator<<(const Skill *skill);
    Package &operator<<(General *general);
    Package &operator<<(const CardDescriptor &card);

private:
    PackagePrivate *d;
};

namespace PackageAdder {
typedef Package *(*PackageInitializer)();
void registerPackageInitializer(const QString &name, PackageInitializer initializer);
const QHash<QString, Package *> &packages();
} // namespace PackageAdder

#define ADD_PACKAGE(name)                                                                   \
    namespace {                                                                             \
    void name##initializeFunc()                                                             \
    {                                                                                       \
        PackageAdder::registerPackageInitializer(QStringLiteral(#name), name##Initializer); \
    }                                                                                       \
    }                                                                                       \
    Q_COREAPP_STARTUP_FUNCTION(name##initializeFunc)

#endif
