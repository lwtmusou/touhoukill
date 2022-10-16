#include "legacyutil.h"

#include "engine.h"
#include "mode.h"
#include "qsgscore.h"

bool isRoleGameMode(const QString &name)
{
    const Mode *mode = Sanguosha->gameMode(name);
    return mode->category() == QSanguosha::ModeRole;
}

bool isHegemonyGameMode(const QString &name)
{
    const Mode *mode = Sanguosha->gameMode(name);
    return mode->category() == QSanguosha::ModeHegemony;
}
