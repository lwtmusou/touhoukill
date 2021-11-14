#include "package.h"
#include "card.h"

#include <QFile>

using namespace QSanguosha;

class PackagePrivate
{
public:
    QString name;
    PackageType type;

    QList<const General *> generals;
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

PackageType Package::type() const
{
    return d->type;
}

const QList<CardDescriptor> &Package::cards() const
{
    return d->all_cards;
}

const QList<const General *> &Package::generals() const
{
    return d->generals;
}

Package &Package::operator<<(const General *general)
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
