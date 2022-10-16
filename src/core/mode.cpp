#include "mode.h"
#include "RoomObject.h"
#include "card.h"
#include "engine.h"
#include "general.h"
#include "package.h"
#include "serverinfostruct.h"

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

Mode::Mode(const QString &name, ModeCategory category)
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

IdSet Mode::availableCards() const
{
    IdSet ret;
    int count = Sanguosha->cardCount();
    for (int i = 0; i <= count; ++i) {
        const CardDescriptor &desc = Sanguosha->cardDescriptor(i);
        if (desc.package == nullptr)
            continue;
        if (desc.package->availableModeCategory() != d->category)
            continue;

        ret << i;
    }

    // for hegemony: knownboth and doublesword are substituted in their own card package and no need to substitute them here
    return ret;
}

QSet<const General *> Mode::availableGenerals() const
{
    QSet<QString> names = Sanguosha->generalNames();
    QSet<const General *> ret;
    foreach (const QString &name, names) {
        const General *general = Sanguosha->general(name);
        if (general == nullptr)
            continue;
        if (general->isHidden())
            continue;
        if (general->package() == nullptr)
            continue;
        if (general->package()->availableModeCategory() != d->category)
            continue;

        ret << general;
    }

    // why consider ban in original Engine::availableGeneralCount?
    // ban should be considered only when selecting generals
    return ret;
}

class GenericRoleModePrivate
{
public:
    int loyalistCount;
    int rebelCount;
    int renegadeCount;

    void parseRoleCount(const QString &name)
    {
        parseRoleCount(name, &loyalistCount, &rebelCount, &renegadeCount);
    }

    // make this function can be called with only one nullptr parameter
    static bool parseRoleCount(const QString &name, int *loyalistCount, int *rebelCount = nullptr, int *renegadeCount = nullptr)
    {
        {
            static int _players;
            if (loyalistCount == nullptr)
                loyalistCount = &_players;
            if (rebelCount == nullptr)
                rebelCount = &_players;
            if (renegadeCount == nullptr)
                renegadeCount = &_players;
        }

        static QRegularExpression regexp(QRegularExpression::anchoredPattern(QStringLiteral(R"re(role_(\d+),(\d+),(\d+))re")));
        QRegularExpressionMatch match;
        if ((match = regexp.match(name)).hasMatch()) {
            bool ok1 = false;
            *loyalistCount = match.capturedTexts().value(1).toInt(&ok1);
            bool ok2 = false;
            *rebelCount = match.capturedTexts().value(2).toInt(&ok2);
            bool ok3 = false;
            *renegadeCount = match.capturedTexts().value(3).toInt(&ok3);
            return ok1 && ok2 && ok3;
        }

        return false;
    }
};

GenericRoleMode::GenericRoleMode(const QString &name)
    : Mode(name, ModeRole)
    , d(new GenericRoleModePrivate)
{
    d->parseRoleCount(name);
}

GenericRoleMode::~GenericRoleMode()
{
    delete d;
}

int GenericRoleMode::playersCount() const
{
    return 1 + d->loyalistCount + d->rebelCount + d->renegadeCount;
}

int GenericRoleMode::generalsPerPlayer() const
{
    return 1;
}

QString GenericRoleMode::roles() const
{
    QString output = QStringLiteral("Z");
    for (int i = 0; i < d->loyalistCount; ++i)
        output.append(QStringLiteral("C"));
    for (int i = 0; i < d->rebelCount; ++i)
        output.append(QStringLiteral("F"));
    for (int i = 0; i < d->renegadeCount; ++i)
        output.append(QStringLiteral("N"));

    return output;
}

Rule *GenericRoleMode::rule() const
{
    // TODO
    return nullptr;
}

void GenericRoleMode::startGame(GameLogic *logic, RoomObject *room) const
{
    // TODO
}

bool GenericRoleMode::nameMatched(const QString &name)
{
    return GenericRoleModePrivate::parseRoleCount(name, nullptr);
}

class GenericHegemonyModePrivate
{
public:
    int players;

    void parsePlayers(const QString &name)
    {
        parsePlayers(name, &players);
    }

    static bool parsePlayers(const QString &name, int *players)
    {
        {
            static int _players;
            if (players == nullptr)
                players = &_players;
        }

        static QRegularExpression regexp(QRegularExpression::anchoredPattern(QStringLiteral(R"re(hegemony_(\d+))re")));
        QRegularExpressionMatch match;
        if ((match = regexp.match(name)).hasMatch()) {
            bool ok1 = false;
            *players = match.capturedTexts().value(1).toInt(&ok1);
            return ok1;
        }

        return false;
    }
};

GenericHegemonyMode::GenericHegemonyMode(const QString &name)
    : Mode(name, ModeHegemony)
    , d(new GenericHegemonyModePrivate)
{
    d->parsePlayers(name);
}

GenericHegemonyMode::~GenericHegemonyMode()
{
    delete d;
}

int GenericHegemonyMode::playersCount() const
{
    return d->players;
}

int GenericHegemonyMode::generalsPerPlayer() const
{
    return 2;
}

QString GenericHegemonyMode::roles() const
{
    QString output;
    for (int i = 0; i < d->players; ++i)
        output.append(QStringLiteral("Q"));

    return output;
}

Rule *GenericHegemonyMode::rule() const
{
    // TODO
    return nullptr;
}

void GenericHegemonyMode::startGame(GameLogic *logic, RoomObject *room) const
{
    // TODO
}

bool GenericHegemonyMode::nameMatched(const QString &name)
{
    return GenericHegemonyModePrivate::parsePlayers(name, nullptr);
}
