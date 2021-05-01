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

class Package : public QObject
{
    Q_OBJECT
    Q_ENUMS(Type)

public:
    enum Type
    {
        GeneralPack,
        CardPack,
        MixedPack,
        SpecialPack
    };

    explicit Package(const QString &name, Type pack_type = GeneralPack)
    {
        setObjectName(name);
        type = pack_type;
    }

    QList<const QMetaObject *> getMetaObjects() const
    {
        return metaobjects;
    }

    const QList<const Skill *> &getSkills() const
    {
        return skills;
    }

    const QMap<QString, const CardPattern *> &getPatterns() const
    {
        return patterns;
    }

    const QMultiMap<QString, QString> &getRelatedSkills() const
    {
        return related_skills;
    }

    Type getType() const
    {
        return type;
    }

    template <typename T> void addMetaObject()
    {
        metaobjects << &T::staticMetaObject;
    }

    inline void insertRelatedSkills(const QString &main_skill, const QString &related_skill)
    {
        related_skills.insert(main_skill, related_skill);
    }

    const QList<const CardFace *> &cardFaces() const
    {
        return this->faces;
    }

    const QMultiMap<const CardFace *, QPair<Card::Suit, Card::Number> > cards() const
    {
        return this->all_cards;
    }

protected:
    QList<const QMetaObject *> metaobjects;
    QList<const Skill *> skills;
    QList<const CardFace *> faces;
    QMap<QString, const CardPattern *> patterns;
    QMultiMap<QString, QString> related_skills;
    QMultiMap<const CardFace *, QPair<Card::Suit, Card::Number> > all_cards;
    Type type;
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
