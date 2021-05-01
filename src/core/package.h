#ifndef _PACKAGE_H
#define _PACKAGE_H

class Skill;
class Card;
class Player;

#include <QHash>
#include <QMap>
#include <QObject>
#include <QPair>
#include <QStringList>

#include "json.h"

class CardPattern
{
public:
    virtual bool match(const Player *player, const Card *card) const = 0;
    virtual bool willThrow() const
    {
        return true;
    }
};

class CardFace;
class CardDescriptor;
class General;

class Package
{
public:
    enum Type
    {
        GeneralPack,
        CardPack,
        MixedPack,
        SpecialPack
    };

    explicit Package(const QString &name, Type pack_type = GeneralPack);

    const QString &getName() const;

    const QList<const Skill *> &getSkills() const;
    const QMap<QString, const CardPattern *> &getPatterns() const;
    const QMultiMap<QString, QString> &getRelatedSkills() const;

    Type getType() const;

    void insertRelatedSkills(const QString &main_skill, const QString &related_skill);

    const QList<const CardFace *> &getCardFaces() const;
    const QMultiMap<const CardFace *, CardDescriptor> &getCards() const;
    const QList<General *> &getGeneral() const;

    Package &operator<<(const CardFace *face); // register face.
    Package &operator<<(const Skill *skill);
    Package &operator<<(General *general);

protected:
    QString name;
    Package::Type type;

    QList<General *> generals;

    QList<const Skill *> skills;
    QList<const CardFace *> faces;
    QMap<QString, const CardPattern *> patterns;
    QMultiMap<QString, QString> related_skills;
    QMultiMap<const CardFace *, CardDescriptor> all_cards;
};

typedef QHash<QString, Package *> PackageHash;

class PackageAdder
{
public:
    PackageAdder(const QString &name, Package *pack)
    {
        packages()[name] = pack;
    }

    static PackageHash &packages();
};

#define ADD_PACKAGE(name) static PackageAdder name##PackageAdder(#name, new name##Package);

#endif
