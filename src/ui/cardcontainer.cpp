#include "cardcontainer.h"
#include "carditem.h"
#include "client.h"
#include "engine.h"
#include "roomscene.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

CardContainer::CardContainer()
    : scene_width(0)
    , itemCount(0)
    , _m_background(QStringLiteral("image/system/card-container.png"))
{
    setTransform(QTransform::fromTranslate(-_m_background.width() / 2, -_m_background.height() / 2), true);
    _m_boundingRect = QRectF(QPoint(0, 0), _m_background.size());
    setFlag(ItemIsFocusable);
    setFlag(ItemIsMovable);
    close_button = new SanCloseButton;
    close_button->setParentItem(this);
    close_button->setPos(517, 21);
    close_button->hide();
    connect(close_button, &SanCloseButton::clicked, this, &CardContainer::clear);
}

void CardContainer::paint(QPainter *painter, const QStyleOptionGraphicsItem * /*option*/, QWidget * /*widget*/)
{
    painter->drawPixmap(0, 0, _m_background);
}

QRectF CardContainer::boundingRect() const
{
    return _m_boundingRect;
}

void CardContainer::fillCards(const QList<int> &card_ids, const QList<int> &disabled_ids, const QList<int> &shownHandcard_ids)
{
    QList<CardItem *> card_items;
    if (card_ids.isEmpty() && items.isEmpty())
        return;
    else if (card_ids.isEmpty() && !items.isEmpty()) {
        card_items = items;
        items.clear();
    } else if (!items.isEmpty()) {
        retained_stack.push(retained());
        items_stack.push(items);
        foreach (CardItem *item, items)
            item->hide();
        items.clear();
    }

    close_button->hide();
    if (card_items.isEmpty())
        card_items = _createCards(card_ids);

    int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    QPointF pos1(30 + card_width / 2, 40 + G_COMMON_LAYOUT.m_cardNormalHeight / 2);
    QPointF pos2(30 + card_width / 2, 184 + G_COMMON_LAYOUT.m_cardNormalHeight / 2);
    int skip = 102;
    qreal whole_width = skip * 4;
    items.append(card_items);
    int n = items.length();

    for (int i = 0; i < n; i++) {
        QPointF pos;
        if (n <= 10) {
            if (i < 5) {
                pos = pos1;
                pos.setX(pos.x() + i * skip);
            } else {
                pos = pos2;
                pos.setX(pos.x() + (i - 5) * skip);
            }
        } else {
            int half = (n + 1) / 2;
            qreal real_skip = whole_width / (half - 1);

            if (i < half) {
                pos = pos1;
                pos.setX(pos.x() + i * real_skip);
            } else {
                pos = pos2;
                pos.setX(pos.x() + (i - half) * real_skip);
            }
        }
        CardItem *item = items[i];
        item->setPos(pos);
        item->setHomePos(pos);
        item->setOpacity(1.0);
        item->setHomeOpacity(1.0);
        item->setFlag(QGraphicsItem::ItemIsFocusable);

        item->setAcceptedMouseButtons(Qt::LeftButton);
        if (disabled_ids.contains(item->getCard()->effectiveID()))
            item->setEnabled(false);

        if (shownHandcard_ids.contains(item->getId())) {
            item->setFootnote(Sanguosha->translate(QStringLiteral("shown_card")));
            item->showFootnote();
        }

        item->show();
    }
}

void CardContainer::fillGeneralCards(const QList<CardItem *> &card_item, const QList<CardItem *> &disabled_item)
{
    if (card_item == items)
        return;

    QList<CardItem *> card_items = card_item;
    if (card_items.isEmpty() && items.isEmpty())
        return;
    else if (card_item.isEmpty() && !items.isEmpty()) {
        card_items = items;
        items.clear();
    } else if (!items.isEmpty()) {
        retained_stack.push(retained());
        items_stack.push(items);
        foreach (CardItem *item, items)
            item->hide();
        items.clear();
    }

    scene_width = RoomSceneInstance->sceneRect().width();

    items.append(card_items);
    itemCount = items.length();
    prepareGeometryChange();

    int card_width = G_COMMON_LAYOUT.m_cardNormalWidth;
    int card_height = G_COMMON_LAYOUT.m_cardNormalHeight;
    bool one_row = true;
    int width = (card_width + cardInterval) * itemCount - cardInterval + 50;
    if (width * 1.5 > scene_width) {
        one_row = false;
    }
    int first_row = one_row ? itemCount : (itemCount + 1) / 2;

    for (int i = 0; i < itemCount; i++) {
        QPointF pos;
        if (i < first_row) {
            pos.setX(25 + (card_width + cardInterval) * i);
            pos.setY(45);
        } else {
            if (itemCount % 2 == 1)
                pos.setX(25 + card_width / 2 + cardInterval / 2 + (card_width + cardInterval) * (i - first_row));
            else
                pos.setX(25 + (card_width + cardInterval) * (i - first_row));
            pos.setY(45 + card_height + cardInterval);
        }
        CardItem *item = items[i];
        item->resetTransform();
        item->setPos(pos);
        item->setHomePos(pos);
        item->setOpacity(1.0);
        item->setHomeOpacity(1.0);
        item->setFlag(QGraphicsItem::ItemIsFocusable);
        if (disabled_item.contains(item))
            item->setEnabled(false);
        item->show();
    }

    close_button->setPos(boundingRect().topRight().x() - boundingRect().width() / 10, boundingRect().topRight().y() + boundingRect().height() / 10);
    close_button->show();
}

bool CardContainer::_addCardItems(QList<CardItem *> & /*card_items*/, const LegacyCardsMoveStruct & /*moveInfo*/)
{
    return true;
}

bool CardContainer::retained()
{
    return close_button != nullptr && close_button->isVisible();
}

void CardContainer::clear()
{
    foreach (CardItem *item, items) {
        item->hide();
        item = nullptr;
        delete item;
    }

    items.clear();
    if (!items_stack.isEmpty()) {
        items = items_stack.pop();
        bool retained = retained_stack.pop();
        fillCards();
        if (retained && (close_button != nullptr))
            close_button->show();
    } else {
        close_button->hide();
        hide();
    }
}

void CardContainer::freezeCards(bool is_frozen)
{
    foreach (CardItem *item, items)
        item->setFrozen(is_frozen);
}

QList<CardItem *> CardContainer::removeCardItems(const QList<int> &card_ids, QSanguosha::Place /*place*/)
{
    QList<CardItem *> result;
    foreach (int card_id, card_ids) {
        CardItem *to_take = nullptr;
        foreach (CardItem *item, items) {
            if (item->getCard()->id() == card_id) {
                to_take = item;
                break;
            }
        }
        if (to_take == nullptr)
            continue;

        to_take->setEnabled(false);

        CardItem *copy = new CardItem(to_take->getCard());
        copy->setPos(mapToScene(to_take->pos()));
        copy->setEnabled(false);
        result.append(copy);

        copy->setAcceptedMouseButtons(Qt::MouseButtons());

        if (m_currentPlayer != nullptr)
            to_take->showAvatar(m_currentPlayer->general());
    }
    return result;
}

int CardContainer::getFirstEnabled() const
{
    foreach (CardItem *card, items) {
        if (card->isEnabled())
            return card->getCard()->id();
    }
    return -1;
}

void CardContainer::startChoose()
{
    close_button->hide();
    foreach (CardItem *item, items) {
        connect(item, &CardItem::leave_hover, this, &CardContainer::grabItem);
        connect(item, &CardItem::double_clicked, this, &CardContainer::chooseItem);
    }
}

void CardContainer::startGongxin(const QList<int> &enabled_ids)
{
    if (enabled_ids.isEmpty())
        return;

    foreach (CardItem *item, items) {
        const Card *card = item->getCard();
        if ((card != nullptr) && enabled_ids.contains(card->effectiveID())) {
            connect(item, &CardItem::double_clicked, this, &CardContainer::gongxinItem);
        } else
            item->setEnabled(false);
    }
}

void CardContainer::addCloseButton()
{
    close_button->show();
}

void CardContainer::grabItem()
{
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if ((card_item != nullptr) && !collidesWithItem(card_item)) {
        card_item->disconnect(this);
        emit item_chosen(card_item->getCard()->id());
    }
}

void CardContainer::chooseItem()
{
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item != nullptr) {
        card_item->disconnect(this);
        emit item_chosen(card_item->getCard()->id());
    }
}

void CardContainer::gongxinItem()
{
    CardItem *card_item = qobject_cast<CardItem *>(sender());
    if (card_item != nullptr) {
        emit item_gongxined(card_item->getCard()->id());
        clear();
    }
}

SanCloseButton::SanCloseButton()
    : QSanSelectableItem(QStringLiteral("image/system/close.png"), false)
{
    setFlag(ItemIsFocusable);
    setAcceptedMouseButtons(Qt::LeftButton);
}

void SanCloseButton::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    event->accept();
}

void SanCloseButton::mouseReleaseEvent(QGraphicsSceneMouseEvent * /*event*/)
{
    emit clicked();
}

void CardContainer::view(const Player *player)
{
    QList<int> card_ids;
    QList<const Card *> cards = player->handCards();
    foreach (const Card *card, cards)
        card_ids << card->effectiveID();

    fillCards(card_ids);
}

GuanxingBox::GuanxingBox()
    : QSanSelectableItem(QStringLiteral("image/system/guanxing-box.png"), true)
{
    setFlag(ItemIsFocusable);
    setFlag(ItemIsMovable);
}

void GuanxingBox::doGuanxing(const QList<int> &card_ids, bool up_only)
{
    if (card_ids.isEmpty()) {
        clear();
        return;
    }

    this->up_only = up_only;
    up_items.clear();

    foreach (int card_id, card_ids) {
        CardItem *card_item = new CardItem(ClientInstance->getCard(card_id));
        card_item->setAutoBack(false);
        card_item->setFlag(QGraphicsItem::ItemIsFocusable);
        connect(card_item, &CardItem::released, this, &GuanxingBox::adjust);

        up_items << card_item;
        card_item->setParentItem(this);
        card_item->setAcceptedMouseButtons(Qt::LeftButton);
    }

    show();

    QPointF source(start_x, start_y1);
    for (int i = 0; i < up_items.length(); i++) {
        CardItem *card_item = up_items.at(i);
        QPointF pos(start_x + i * skip, start_y1);
        card_item->setPos(source);
        card_item->setHomePos(pos);
        card_item->goBack(true);
    }
}

void GuanxingBox::adjust()
{
    CardItem *item = qobject_cast<CardItem *>(sender());
    if (item == nullptr)
        return;

    up_items.removeOne(item);
    down_items.removeOne(item);

    QList<CardItem *> *items = (up_only || item->y() <= middle_y) ? &up_items : &down_items;
    int c = (item->x() + item->boundingRect().width() / 2 - start_x) / G_COMMON_LAYOUT.m_cardNormalWidth;
    c = qBound(0, c, items->length());
    items->insert(c, item);

    for (int i = 0; i < up_items.length(); i++) {
        QPointF pos(start_x + i * skip, start_y1);
        up_items.at(i)->setHomePos(pos);
        up_items.at(i)->goBack(true);
    }

    for (int i = 0; i < down_items.length(); i++) {
        QPointF pos(start_x + i * skip, start_y2);
        down_items.at(i)->setHomePos(pos);
        down_items.at(i)->goBack(true);
    }
}

void GuanxingBox::clear()
{
    foreach (CardItem *card_item, up_items)
        card_item->deleteLater();
    foreach (CardItem *card_item, down_items)
        card_item->deleteLater();

    up_items.clear();
    down_items.clear();

    hide();
}

void GuanxingBox::reply()
{
    QList<int> up_cards;
    QList<int> down_cards;
    foreach (CardItem *card_item, up_items)
        up_cards << card_item->getCard()->id();

    foreach (CardItem *card_item, down_items)
        down_cards << card_item->getCard()->id();

    ClientInstance->onPlayerReplyGuanxing(up_cards, down_cards);
    clear();
}
