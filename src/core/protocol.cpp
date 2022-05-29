#include "protocol.h"
#include "jsonutils.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <algorithm>
#include <sstream>

using namespace std;
using namespace QSanProtocol;

const int QSanProtocol::Packet::S_MAX_PACKET_SIZE = 65535;

QSanProtocol::Packet::Packet(PacketDescriptionFlag packetDescription, CommandType command)
    : command(command)
    , packetDescription(packetDescription)
{
}

bool QSanProtocol::Packet::parse(const QByteArray &raw)
{
    if (raw.length() > S_MAX_PACKET_SIZE) {
        return false;
    }

    QJsonDocument doc = QJsonDocument::fromJson(raw);
    QJsonArray result = doc.array();

    if (!QSgsJsonUtils::isNumberArray(result, 0, 3) || result.size() > 5)
        return false;

    packetDescription = static_cast<PacketDescription>(result[2].toInt());
    command = (CommandType)result[3].toInt();

    if (result.size() == 5)
        messageBody = result[4];
    return true;
}

QByteArray QSanProtocol::Packet::toJson() const
{
    QJsonArray result;
    result << 0;
    result << 0;
    result << (int)(packetDescription);
    result << command;
    if (!messageBody.isNull())
        result << messageBody;

    QJsonDocument doc(result);
    const QByteArray &msg = doc.toJson(QJsonDocument::Compact);

    //return an empty string here, for Packet::parse won't parse it (line 92)
    if (msg.length() > S_MAX_PACKET_SIZE)
        return QByteArray();

    return msg;
}

QString QSanProtocol::Packet::toString() const
{
    return QString::fromUtf8(toJson());
}
