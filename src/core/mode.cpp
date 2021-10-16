#include "mode.h"
#include "RoomObject.h"

using namespace QSanguosha;

class ModePrivate
{
public:
    QString name;
    ModeCategory category;

    ModePrivate(const QString &name, ModeCategory category)
        : name(name)
        , category(category)
    {
    }
};

Mode::Mode(const QString &name, QSanguosha::ModeCategory category)
    : d(new ModePrivate(name, category))
{
}

Mode::~Mode()
{
    delete d;
}

const QString &Mode::name() const
{
    return d->name;
}

ModeCategory Mode::category() const
{
    return d->category;
}

class GenericRoleModePrivate
{
public:
    int loyalistCount;
    int rebelCount;
    int renegadeCount;
};

GenericRoleMode::GenericRoleMode(int loyalistCount, int rebelCount, int renegadeCount)
    : Mode(QString(QStringLiteral("role_%1,%2,%3")).arg(QString::number(loyalistCount), QString::number(rebelCount), QString::number(renegadeCount)), ModeRole)
    , d(new GenericRoleModePrivate)
{
    d->loyalistCount = loyalistCount;
    d->rebelCount = rebelCount;
    d->renegadeCount = renegadeCount;
}

GenericRoleMode::~GenericRoleMode()
{
    delete d;
}

int GenericRoleMode::playersCount() const
{
    return 1 + d->loyalistCount + d->rebelCount + d->renegadeCount;
}

Rule *GenericRoleMode::rule() const
{
    // TODO
    return nullptr;
}

IDSet GenericRoleMode::availableCards() const
{
    // TODO
    return {};
}

QSet<const Package *> GenericRoleMode::availablePackages() const
{
    // TODO
    return {};
}

void GenericRoleMode::gameProcess(RoomObject *room) const
{
    // TODO
}

class GenericHegemonyModePrivate
{
public:
    int players;
};

GenericHegemonyMode::GenericHegemonyMode(int players)
    : Mode(QString(QStringLiteral("hegemony_%1")).arg(players), ModeHegemony)
    , d(new GenericHegemonyModePrivate)
{
}

GenericHegemonyMode::~GenericHegemonyMode()
{
    delete d;
}

int GenericHegemonyMode::playersCount() const
{
    return d->players;
}

Rule *GenericHegemonyMode::rule() const
{
    // TODO
    return nullptr;
}

IDSet GenericHegemonyMode::availableCards() const
{
    // TODO
    return {};
}

QSet<const Package *> GenericHegemonyMode::availablePackages() const
{
    // TODO
    return {};
}

void GenericHegemonyMode::gameProcess(RoomObject *room) const
{
    // TODO
}
