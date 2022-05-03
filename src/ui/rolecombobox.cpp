#include "rolecombobox.h"
#include "engine.h"
#include "photo.h"
#include "util.h"

#include <QGraphicsScene>

RoleComboBoxItem::RoleComboBoxItem(const QString &role, int number, QSize size)
    : m_role(role)
    , m_number(number)
    , m_size(size)
{
    setRole(role);
    setFlag(QGraphicsItem::ItemIgnoresParentOpacity);
}

QString RoleComboBoxItem::getRole() const
{
    return m_role;
}

void RoleComboBoxItem::setRole(const QString &role)
{
    m_role = role;
    if (m_number != 0 && role != QStringLiteral("unknown"))
        load(QStringLiteral("image/system/roles/%1-%2.png").arg(m_role, m_number), m_size, false);
    else
        load(QStringLiteral("image/system/roles/%1.png").arg(m_role), m_size, false);
}

void RoleComboBoxItem::mousePressEvent(QGraphicsSceneMouseEvent * /*event*/)
{
    emit clicked();
}

RoleComboBox::RoleComboBox(QGraphicsItem *parent)
    : QGraphicsObject(parent)
{
    int index = getRoleIndex();
    QSize size(S_ROLE_COMBO_BOX_WIDTH, S_ROLE_COMBO_BOX_HEIGHT);
    m_currentRole = new RoleComboBoxItem(QStringLiteral("unknown"), index, size);
    m_currentRole->setParentItem(this);
    connect(m_currentRole, &RoleComboBoxItem::clicked, this, &RoleComboBox::expand);

    items << new RoleComboBoxItem(QStringLiteral("loyalist"), index, size) << new RoleComboBoxItem(QStringLiteral("rebel"), index, size)
          << new RoleComboBoxItem(QStringLiteral("renegade"), index, size);
    for (int i = 0; i < items.length(); i++) {
        RoleComboBoxItem *item = items.at(i);
        item->setPos(0, (i + 1) * (S_ROLE_COMBO_BOX_HEIGHT + S_ROLE_COMBO_BOX_GAP));
        item->setZValue(1.0);
    }
    foreach (RoleComboBoxItem *item, items) {
        item->setParentItem(this);
        item->hide();
        connect(item, &RoleComboBoxItem::clicked, this, &RoleComboBox::collapse);
    }
}

void RoleComboBox::paint(QPainter * /*painter*/, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
}

QRectF RoleComboBox::boundingRect() const
{
    if (items.isEmpty())
        return QRect(0, 0, 0, 0);
    else
        return items[0]->boundingRect();
}

void RoleComboBox::collapse()
{
    disconnect(m_currentRole, &RoleComboBoxItem::clicked, this, &RoleComboBox::collapse);
    connect(m_currentRole, &RoleComboBoxItem::clicked, this, &RoleComboBox::expand);
    RoleComboBoxItem *clicked_item = qobject_cast<RoleComboBoxItem *>(sender());
    foreach (RoleComboBoxItem *item, items)
        item->hide();
    m_currentRole->setRole(clicked_item->getRole());
}

void RoleComboBox::expand()
{
    foreach (RoleComboBoxItem *item, items)
        item->show();
    m_currentRole->setRole(QStringLiteral("unknown"));
    connect(m_currentRole, &RoleComboBoxItem::clicked, this, &RoleComboBox::collapse);
}

void RoleComboBox::toggle()
{
    Q_ASSERT(!_m_fixedRole.isNull());
    if (!isEnabled())
        return;
    QString displayed = m_currentRole->getRole();

    if (displayed == QStringLiteral("unknown"))
        m_currentRole->setRole(_m_fixedRole);
    else
        m_currentRole->setRole(QStringLiteral("unknown"));
}

void RoleComboBox::fix(const QString &role)
{
    if (_m_fixedRole.isNull()) {
        disconnect(m_currentRole, &RoleComboBoxItem::clicked, this, &RoleComboBox::expand);
        connect(m_currentRole, &RoleComboBoxItem::clicked, this, &RoleComboBox::toggle);
    }

    m_currentRole->setRole(role);
    if (role != QStringLiteral("unknown"))
        _m_fixedRole = role;
    // delete all
    foreach (RoleComboBoxItem *item, items)
        delete item;
    items.clear();
}

int RoleComboBox::getRoleIndex()
{
    if (ServerInfo.GameModeStr == QStringLiteral("3v3") || ServerInfo.GameModeStr == QStringLiteral("XMode"))
        return 4;
    else if (isHegemonyGameMode(ServerInfo.GameModeStr))
        return 5;
    else
        return 1;
}
