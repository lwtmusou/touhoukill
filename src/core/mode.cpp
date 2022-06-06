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

QSet<QString> Mode::availableModes()
{
    return {
        // Role
        QStringLiteral("role_0,1,0"), //  2 players
        QStringLiteral("role_0,1,1"), //  3 players
        QStringLiteral("role_0,2,1"), //  4 players
        QStringLiteral("role_1,2,1"), //  5 players
        QStringLiteral("role_1,3,1"), //  6 players
        QStringLiteral("role_1,2,2"), //  6 players, dual renegades
        QStringLiteral("role_2,3,1"), //  7 players
        QStringLiteral("role_2,4,1"), //  8 players
        QStringLiteral("role_2,3,2"), //  8 players, dual renegades
        QStringLiteral("role_3,4,0"), //  8 players, no renegades
        QStringLiteral("role_3,4,1"), //  9 players
        QStringLiteral("role_3,4,2"), //  10players
        QStringLiteral("role_3,5,1"), //  10players, single renegade
        QStringLiteral("role_4,5,0"), //  10players, no renegades
        // TODO: Shall we support player number > 10 in Role mode?
        // TODO: Shall we support customized Role mode?

        // Hegemony
        QStringLiteral("hegemony_2"), //  2 players
        QStringLiteral("hegemony_3"), //  3 players
        QStringLiteral("hegemony_4"), //  4 players
        QStringLiteral("hegemony_5"), //  5 players
        QStringLiteral("hegemony_6"), //  6 players
        QStringLiteral("hegemony_7"), //  7 players
        QStringLiteral("hegemony_8"), //  8 players
        QStringLiteral("hegemony_9"), //  9 players
        QStringLiteral("hegemony_10"), // 10players
        QStringLiteral("hegemony_11"), // 11players
        QStringLiteral("hegemony_12"), // 12players

        // Others
        // QStringLiteral("1v1"), //      official 1v1 mode
        // QStringLiteral("1v3"), //      official 1v3 mode
        // QStringLiteral("3v3"), //      official 3v3 mode
        // QStringLiteral("3v3x"), //     official 3v3 mode, extreme
        // QStringLiteral("jiange"), //   official 4v4 mode, JianGe Defense
    };
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

        QRegularExpression regexp(QRegularExpression::anchoredPattern(QStringLiteral(R"re(role_(\d+),(\d+),(\d+))re")));
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

void GenericRoleMode::startGame(RoomObject *room) const
{
    // TODO
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

        QRegularExpression regexp(QRegularExpression::anchoredPattern(QStringLiteral(R"re(hegemony_(\d+))re")));
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

void GenericHegemonyMode::startGame(RoomObject *room) const
{
    // TODO
}

// This function must be put at end of file!!
const Mode *Mode::findMode(const QString &name)
{
    static QHash<QString, Mode *> modes;

    if (modes.contains(name))
        return modes.value(name);

    Mode *ret = nullptr;

    if (name == QStringLiteral("1v1")) {
        // TODO: create 1v1 mode
    } else if (name == QStringLiteral("1v3")) {
        // TODO: create 1v3 mode
    } else if (name == QStringLiteral("3v3")) {
        // TODO: create 3v3 mode
    } else if (name.startsWith(QStringLiteral("hegemony_"))) {
        if (GenericHegemonyModePrivate::parsePlayers(name, nullptr))
            ret = new GenericHegemonyMode(name);
    } else if (name.startsWith(QStringLiteral("role_"))) {
        if (GenericRoleModePrivate::parseRoleCount(name, nullptr))
            ret = new GenericRoleMode(name);
    }

    if (ret != nullptr)
        modes[name] = ret;

    return ret;
}
