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

#include "card.h"
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
struct CardDescriptor;
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

    const QString &name() const;

    const QList<const Skill *> &skills() const;
    const QMap<QString, const CardPattern *> &patterns() const;
    const QMultiMap<QString, QString> &relatedSkills() const;

    Type type() const;

    void insertRelatedSkills(const QString &main_skill, const QString &related_skill);

    const QList<const CardFace *> &cardFaces() const;
    const QList<CardDescriptor> &cards() const;
    const QList<General *> &generals() const;

    Package &operator<<(const CardFace *face); // register face.
    Package &operator<<(const Skill *skill);
    Package &operator<<(General *general);

protected:
    QString m_name;
    Package::Type m_type;

    QList<General *> m_generals;

    QList<const Skill *> m_skills;
    QList<const CardFace *> m_faces;
    QMap<QString, const CardPattern *> m_patterns;
    QMultiMap<QString, QString> m_related_skills;
    // QMultiMap<const CardFace *, CardDescriptor> m_all_cards;
    QList<CardDescriptor> m_all_cards;
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
