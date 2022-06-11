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
    : m_command(command)
    , m_packetDescription(packetDescription)
{
}

void Packet::setMessageBody(const QJsonValue &value)
{
    m_messageBody = value;
}

const QJsonValue &Packet::messageBody() const
{
    return m_messageBody;
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

    m_packetDescription = static_cast<PacketDescription>(result[2].toInt());
    m_command = (CommandType)result[3].toInt();

    if (result.size() == 5)
        m_messageBody = result[4];
    return true;
}

QByteArray QSanProtocol::Packet::toJson() const
{
    QJsonArray result;
    result << 0;
    result << 0;
    result << (int)(m_packetDescription);
    result << m_command;
    if (!m_messageBody.isNull())
        result << m_messageBody;

    QJsonDocument doc(result);
    const QByteArray &msg = doc.toJson(QJsonDocument::Compact);

    //return an empty string here, for Packet::parse won't parse it (line 92)
    if (msg.length() > S_MAX_PACKET_SIZE)
        return QByteArray();

    return msg;
}

PacketDescriptionFlag Packet::destination() const
{
    return (m_packetDescription & S_DEST_MASK);
}

PacketDescriptionFlag Packet::source() const
{
    return (m_packetDescription & S_SRC_MASK);
}

PacketDescriptionFlag Packet::type() const
{
    return (m_packetDescription & S_TYPE_MASK);
}

PacketDescriptionFlag Packet::description() const
{
    return m_packetDescription;
}

CommandType Packet::commandType() const
{
    return m_command;
}

QString QSanProtocol::Packet::toString() const
{
    return QString::fromUtf8(toJson());
}
