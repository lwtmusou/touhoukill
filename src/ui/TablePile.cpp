#include "TablePile.h"
#include "SkinBank.h"
#include "pixmapanimation.h"

#include <QParallelAnimationGroup>
#include <QTimer>

QList<CardItem *> TablePile::removeCardItems(const QList<int> &card_ids, Player::Place)
{
    QList<CardItem *> result;
    _m_mutex_pileCards.lock();
    result = _createCards(card_ids);
    _disperseCards(result, m_cardsDisplayRegion, Qt::AlignCenter, false, true);
    foreach (CardItem *card, result) {
        for (int i = m_visibleCards.size() - 1; i >= 0; i--) {
            if (m_visibleCards[i]->getCard() && m_visibleCards[i]->getCard()->getId() == card->getId()) {
                card->setPos(m_visibleCards[i]->pos());
                break;
            }
        }
    }
    _m_mutex_pileCards.unlock();
    return result;
}

QRectF TablePile::boundingRect() const
{
    return m_cardsDisplayRegion;
}

void TablePile::setSize(double width, double height)
{
    m_cardsDisplayRegion = QRect(0, 0, width, height);
    m_numCardsVisible = width / G_COMMON_LAYOUT.m_cardNormalHeight + 1;
    resetTransform();
    setTransform(QTransform::fromTranslate(-width / 2, -height / 2), true);
}

void TablePile::timerEvent(QTimerEvent *)
{
    QList<CardItem *> oldCards;
    _m_mutex_pileCards.lock();
    m_currentTime++;
    foreach (CardItem *toRemove, m_visibleCards) {
        if (m_currentTime - toRemove->m_uiHelper.tablePileClearTimeStamp > S_CLEARANCE_DELAY_BUCKETS) {
            oldCards.append(toRemove);
            m_visibleCards.removeOne(toRemove);
        } else if (m_currentTime > toRemove->m_uiHelper.tablePileClearTimeStamp)
            toRemove->setEnabled(false); // @todo: this is a dirty trick. Use another property in the future
    }

    if (oldCards.isEmpty()) {
        _m_mutex_pileCards.unlock();
        return;
    }

    _fadeOutCardsLocked(oldCards);
    _m_mutex_pileCards.unlock();

    adjustCards();
}

void TablePile::_markClearance(CardItem *item)
{
    if (item->m_uiHelper.tablePileClearTimeStamp > m_currentTime)
        item->m_uiHelper.tablePileClearTimeStamp = m_currentTime;
}

void TablePile::clear(bool delayRequest)
{
    if (m_visibleCards.isEmpty())
        return;
    _m_mutex_pileCards.lock();
    // check again since we just gain the lock.
    if (m_visibleCards.isEmpty()) {
        _m_mutex_pileCards.unlock();
        return;
    }

    if (delayRequest) {
        foreach (CardItem *toRemove, m_visibleCards)
            _markClearance(toRemove);
    } else {
        _fadeOutCardsLocked(m_visibleCards);
        m_visibleCards.clear();
    }

    _m_mutex_pileCards.unlock();
}

void TablePile::_fadeOutCardsLocked(const QList<CardItem *> &cards)
{
    QParallelAnimationGroup *group = new QParallelAnimationGroup;
    foreach (CardItem *toRemove, cards) {
        toRemove->setZValue(0.0);
        toRemove->setHomeOpacity(0.0);
        toRemove->setHomePos(QPointF(toRemove->homePos().x(), toRemove->homePos().y()));
        toRemove->deleteLater();
        group->addAnimation(toRemove->getGoBackAnimation(true, false, 1000));
    }
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void TablePile::showJudgeResult(int cardId, bool takeEffect)
{
    _m_mutex_pileCards.lock();
    CardItem *judgeCard = nullptr;
    QList<CardItem *> cardsToClear;
    for (int i = m_visibleCards.size() - 1; i >= 0; i--) {
        CardItem *item = m_visibleCards[i];
        if (judgeCard == nullptr && item->getCard() && item->getCard()->getId() == cardId)
            judgeCard = m_visibleCards[i];
        else
            cardsToClear.append(item);
    }
    if (judgeCard == nullptr)
        judgeCard = _createCard(cardId);
    m_visibleCards.clear();
    m_visibleCards.append(judgeCard);
    _fadeOutCardsLocked(cardsToClear);
    PixmapAnimation::GetPixmapAnimation(judgeCard, takeEffect ? "judgegood" : "judgebad");
    _m_mutex_pileCards.unlock();

    adjustCards();
}

bool TablePile::_addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo)
{
    if (card_items.isEmpty())
        return false;
    else if (moveInfo.from_place == Player::PlaceDelayedTrick && moveInfo.reason.m_reason == CardMoveReason::S_REASON_NATURAL_ENTER) {
        foreach (CardItem *item, card_items) {
            item->deleteLater();
            card_items.clear();
        }
        return false;
    }

    _m_mutex_pileCards.lock();

    QPointF rightMostPos = m_cardsDisplayRegion.center();
    if (m_visibleCards.length() > 0) {
        rightMostPos = m_visibleCards.last()->homePos();
        rightMostPos += QPointF(G_COMMON_LAYOUT.m_cardNormalWidth, 0);
    }

    m_visibleCards.append(card_items);
    int numAdded = card_items.length();
    int numRemoved = m_visibleCards.size() - qMax(m_numCardsVisible, numAdded + 1);

    for (int i = 0; i < numRemoved; i++) {
        CardItem *toRemove = m_visibleCards[i];
        _markClearance(toRemove);
    }

    foreach (CardItem *card_item, card_items) {
        card_item->setHomeOpacity(1.0);
        card_item->showFootnote();
        if (moveInfo.from_place == Player::DrawPile || moveInfo.from_place == Player::PlaceJudge || moveInfo.from_place == Player::PlaceTable) {
            card_item->setOpacity(0.0);
            card_item->setPos(rightMostPos);
            rightMostPos += QPointF(G_COMMON_LAYOUT.m_cardNormalWidth, 0);
        }
        card_item->m_uiHelper.tablePileClearTimeStamp = INT_MAX;
    }

    _m_mutex_pileCards.unlock();
    adjustCards();
    return false;
}

void TablePile::adjustCards()
{
    if (m_visibleCards.length() == 0)
        return;
    _disperseCards(m_visibleCards, m_cardsDisplayRegion, Qt::AlignCenter, true, true);
    QParallelAnimationGroup *animation = new QParallelAnimationGroup(this);
    foreach (CardItem *card_item, m_visibleCards)
        animation->addAnimation(card_item->getGoBackAnimation(true));
    //connect(animation, SIGNAL(finished()), this, SLOT(onAnimationFinished()));
    animation->start(QAbstractAnimation::DeleteWhenStopped);
}
