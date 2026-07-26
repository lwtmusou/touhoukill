#include "photo.h"

Photo::Photo(QQuickItem *parent)
    : QQuickItem(parent)
{
}

bool Photo::targetable() const
{
    return m_targetable;
}

void Photo::setTargetable(bool v)
{
    if (m_targetable != v) {
        m_targetable = v;
        emit targetableChanged();
    }
}

bool Photo::targetSelected() const
{
    return m_targetSelected;
}

void Photo::setTargetSelected(bool v)
{
    if (m_targetSelected != v) {
        m_targetSelected = v;
        emit targetSelectedChanged();
    }
}

QString Photo::playerName() const
{
    return m_playerName;
}

void Photo::setPlayerName(const QString &v)
{
    if (m_playerName != v) {
        m_playerName = v;
        emit playerNameChanged();
    }
}
