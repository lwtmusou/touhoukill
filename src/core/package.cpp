#include "package.h"
#include "card.h"

using namespace QSanguosha;

class PackagePrivate
{
public:
    QString name;
    PackageType type;

    QList<General *> generals;

    QList<const Skill *> skills;
    QList<const CardFace *> faces;
    QMap<QString, const CardPattern *> patterns;
    QMultiMap<QString, QString> related_skills;
    QList<CardDescriptor> all_cards;
};

Package::Package(const QString &name, PackageType pack_type)
    : d(new PackagePrivate)
{
    d->name = (name);
    d->type = (pack_type);
}

Package::~Package()
{
    delete d;
}

const QString &Package::name() const
{
    return d->name;
}

const QList<const Skill *> &Package::skills() const
{
    return d->skills;
}

const QMap<QString, const CardPattern *> &Package::patterns() const
{
    return d->patterns;
}

const QMultiMap<QString, QString> &Package::relatedSkills() const
{
    return d->related_skills;
}

PackageType Package::type() const
{
    return d->type;
}

void Package::insertRelatedSkills(const QString &main_skill, const QString &related_skill)
{
    d->related_skills.insert(main_skill, related_skill);
}

void Package::insertPattern(const QString &name, const CardPattern *cardPattern)
{
    d->patterns.insert(name, cardPattern);
}

const QList<const CardFace *> &Package::cardFaces() const
{
    return d->faces;
}

const QList<CardDescriptor> &Package::cards() const
{
    return d->all_cards;
}

const QList<General *> &Package::generals() const
{
    return d->generals;
}

Package &Package::operator<<(const CardFace *face)
{
    d->faces << face;
    return *this;
}

Package &Package::operator<<(const Skill *skill)
{
    d->skills << skill;
    return *this;
}

Package &Package::operator<<(General *general)
{
    d->generals << general;
    return *this;
}

Package &Package::operator<<(const CardDescriptor &card)
{
    d->all_cards << card;
    return *this;
}

namespace BuiltinExtension {

// checksum should be in SHA256 and keccak256 algorithms
// Qt provides QCryptographicHash for this job, which we used to verify the auto-update in legacy TouhouSatsu
// Todo: find a way for trusted download of packages (PGP?)

bool VerifyChecksum(const QString &path, const QString &hash, QCryptographicHash::Algorithm algorithm)
{
    QFile f(path);
    if (!f.open(QFile::ReadOnly))
        return false;

    QByteArray arr = f.readAll();
    f.close();

    QByteArray hashed = QCryptographicHash::hash(arr, algorithm);
    return QString::fromLatin1(hashed.toHex()).toUpper() == hash.toUpper();
}
} // namespace BuiltinExtension
