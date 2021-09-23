#ifndef _PACKAGE_H
#define _PACKAGE_H

#include "global.h"

#include <QCoreApplication>
#include <QCryptographicHash>
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

    QSanguosha::PackageType type() const;

    const QList<CardDescriptor> &cards() const;
    const QList<const General *> &generals() const;

    Package &operator<<(const General *general);
    Package &operator<<(const CardDescriptor &card);

private:
    Q_DISABLE_COPY_MOVE(Package)
    PackagePrivate *d;
};

namespace BuiltinExtension {
bool VerifyChecksum(const QString &path, const QString &hash, QCryptographicHash::Algorithm algorithm);
} // namespace BuiltinExtension

#endif
