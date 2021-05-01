#include "package.h"

Package::Package(const QString &name, Type pack_type)
    : name(name)
    , type(pack_type)
{
}

const QString &Package::getName() const
{
    return name;
}

const QList<const Skill *> &Package::getSkills() const
{
    return skills;
}

const QMap<QString, const CardPattern *> &Package::getPatterns() const
{
    return patterns;
}

const QMultiMap<QString, QString> &Package::getRelatedSkills() const
{
    return related_skills;
}

Package::Type Package::getType() const
{
    return type;
}

void Package::insertRelatedSkills(const QString &main_skill, const QString &related_skill)
{
    related_skills.insert(main_skill, related_skill);
}

const QList<const CardFace *> &Package::getCardFaces() const
{
    return faces;
}

const QMultiMap<const CardFace *, CardDescriptor> &Package::getCards() const
{
    return all_cards;
}

const QList<General *> &Package::getGeneral() const
{
    return generals;
}

Package &Package::operator<<(const CardFace *face)
{
    faces << face;
    return *this;
}

Package &Package::operator<<(const Skill *skill)
{
    skills << skill;
    return *this;
}

Package &Package::operator<<(General *general)
{
    generals << general;
    return *this;
}

Q_GLOBAL_STATIC(PackageHash, Packages)
PackageHash &PackageAdder::packages()
{
    return *(::Packages());
}
