#include "clientplayer.h"
#include "client.h"
#include "engine.h"
#include "skill.h"

#include <QTextDocument>
#include <QTextOption>

ClientPlayer::ClientPlayer(Client *client)
    : Player(client)
{
    mark_doc = new QTextDocument(this);
}

QTextDocument *ClientPlayer::getMarkDoc() const
{
    return mark_doc;
}

QString ClientPlayer::getDeathPixmapPath() const
{
    QString basename;
    if (ServerInfo.GameMode == QStringLiteral("06_3v3") || ServerInfo.GameMode == QStringLiteral("06_XMode")) {
        if (getRoleString() == QStringLiteral("lord") || getRoleString() == QStringLiteral("renegade"))
            basename = QStringLiteral("marshal");
        else
            basename = QStringLiteral("guard");
    } else
        basename = getRoleString();

    if (basename.isEmpty())
        basename = QStringLiteral("unknown");

    return QStringLiteral("image/system/death/%1.png").arg(basename);
}

Client *ClientPlayer::getClient() const
{
    return qobject_cast<Client *>(parent());
}
