#include "package.h"

Package::Package(const QString &name, Type pack_type)
    : m_name(name)
    , m_type(pack_type)
{
}

const QString &Package::name() const
{
    return m_name;
}

const QList<const Skill *> &Package::skills() const
{
    return m_skills;
}

const QMap<QString, const CardPattern *> &Package::patterns() const
{
    return m_patterns;
}

const QMultiMap<QString, QString> &Package::relatedSkills() const
{
    return m_related_skills;
}

Package::Type Package::type() const
{
    return m_type;
}

void Package::insertRelatedSkills(const QString &main_skill, const QString &related_skill)
{
    m_related_skills.insert(main_skill, related_skill);
}

const QList<const CardFace *> &Package::cardFaces() const
{
    return m_faces;
}

const QList<CardDescriptor> &Package::cards() const
{
    return m_all_cards;
}

const QList<General *> &Package::generals() const
{
    return m_generals;
}

Package &Package::operator<<(const CardFace *face)
{
    m_faces << face;
    return *this;
}

Package &Package::operator<<(const Skill *skill)
{
    m_skills << skill;
    return *this;
}

Package &Package::operator<<(General *general)
{
    m_generals << general;
    return *this;
}

Q_GLOBAL_STATIC(PackageHash, Packages)
PackageHash &PackageAdder::packages()
{
    return *(::Packages());
}
