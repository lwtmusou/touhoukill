#ifndef TOUHOUKILL_PACKAGE_H
#define TOUHOUKILL_PACKAGE_H

// BE WARE! THIS FILE IS USED IN BOTH SWIG AND C++.
// MAKE SURE THE GRAMMAR IS COMPATIBLE BETWEEN 2 LANGUAGES.

#ifndef SWIG
#include "global.h"

#include <QCryptographicHash>
#include <QList>
#include <QString>

class Skill;
class Player;
class Card;
class CardFace;
struct CardDescriptor;
class General;
#endif

#ifndef SWIG
class PackagePrivate;
#endif

class QSGS_CORE_EXPORT Package final
{
public:
#ifndef SWIG
    explicit Package(const QString &name, QSanguosha::PackageType pack_type = QSanguosha::GeneralPack);
#endif
    ~Package();

    const QString &name() const;

    QSanguosha::PackageType type() const;
    QSanguosha::ModeCategory availableModeCategory() const;

    const QList<CardDescriptor> &cards() const;
    const QList<const General *> &generals() const;

#ifndef SWIG
    Package &operator<<(General *general);
    Package &operator<<(CardDescriptor &card);
#endif

private:
    Package() = delete;
    Q_DISABLE_COPY_MOVE(Package)
    PackagePrivate *d;
};

#ifndef SWIG
namespace BuiltinExtension {
QSGS_CORE_EXPORT bool VerifyChecksum(const QString &path, const QString &hash, QCryptographicHash::Algorithm algorithm);
} // namespace BuiltinExtension
#endif

#endif
