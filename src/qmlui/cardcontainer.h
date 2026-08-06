#ifndef TOUHOUKILL_CARDCONTAINER_H_
#define TOUHOUKILL_CARDCONTAINER_H_

#include <QPointer>
#include <QQuickItem>

// Bridge class for CardContainer.qml. Holds a mirror of the QML cardItems list
// (maintained via registerCardItem/unregisterCardItem) so C++ can drive card
// selection state and enablement without QML logic. CardItem's cardId/selected
// are QML Q_PROPERTY accessed via QObject::property/setProperty.
// Layout (lay) and the QML cardItems list property stay in QML (visual).
// Registered as QML type "CppCardContainer".
class CardContainer : public QQuickItem
{
    Q_OBJECT

public:
    explicit CardContainer(QQuickItem *parent = nullptr);

    // QML createItem/removeItem call these to keep the C++ mirror in sync.
    Q_INVOKABLE void registerCardItem(QQuickItem *item);
    Q_INVOKABLE void unregisterCardItem(QQuickItem *item);

    // Collect cardIds of currently-selected CardItems.
    [[nodiscard]] Q_INVOKABLE QVariantList selectedCardIds() const;
    // All cardIds in this container.
    [[nodiscard]] Q_INVOKABLE QVariantList cardItemIds() const;
    // Toggle the selected property of the CardItem with the given cardId.
    Q_INVOKABLE void toggleCardSelected(int cardId);
    // Set selected = false on all CardItems.
    Q_INVOKABLE void unselectAll();
    // Set enabled on the CardItem with the given cardId (for isAvailable gating).
    Q_INVOKABLE void setCardEnabled(int cardId, bool enabled);
    // Set the footnote text on the CardItem with the given cardId (e.g. "shown_card"
    // marker for shown handcards). Empty string clears it.
    Q_INVOKABLE void setCardFootnote(int cardId, const QString &footnote);

private:
    [[nodiscard]] QQuickItem *findCard(int cardId) const;

    QList<QPointer<QQuickItem>> m_cardItems;
};

#endif
