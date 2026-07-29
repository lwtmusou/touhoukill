#include "cardcontainer.h"

CardContainer::CardContainer(QQuickItem *parent)
    : QQuickItem(parent)
{
}

void CardContainer::registerCardItem(QQuickItem *item)
{
    if (item != nullptr && !m_cardItems.contains(item))
        m_cardItems.append(item);
}

void CardContainer::unregisterCardItem(QQuickItem *item)
{
    m_cardItems.removeAll(item);
}

QVariantList CardContainer::selectedCardIds() const
{
    QVariantList result;
    for (const QPointer<QQuickItem> &item : m_cardItems) {
        if (item == nullptr)
            continue;
        if (item->property("selected").toBool())
            result.append(item->property("cardId").toInt());
    }
    return result;
}

QVariantList CardContainer::cardItemIds() const
{
    QVariantList result;
    for (const QPointer<QQuickItem> &item : m_cardItems) {
        if (item != nullptr)
            result.append(item->property("cardId").toInt());
    }
    return result;
}

void CardContainer::toggleCardSelected(int cardId)
{
    QQuickItem *item = findCard(cardId);
    if (item != nullptr)
        item->setProperty("selected", !item->property("selected").toBool());
}

void CardContainer::unselectAll()
{
    for (const QPointer<QQuickItem> &item : m_cardItems) {
        if (item != nullptr)
            item->setProperty("selected", false);
    }
}

void CardContainer::setCardEnabled(int cardId, bool enabled)
{
    QQuickItem *item = findCard(cardId);
    if (item != nullptr)
        item->setEnabled(enabled);
}

QQuickItem *CardContainer::findCard(int cardId) const
{
    for (const QPointer<QQuickItem> &item : m_cardItems) {
        if (item != nullptr && item->property("cardId").toInt() == cardId)
            return item;
    }
    return nullptr;
}
