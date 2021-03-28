#ifndef _TABLE_PILE_H
#define _TABLE_PILE_H

#include "GenericCardContainerUI.h"
#include "QSanSelectableItem.h"
#include "carditem.h"
#include "player.h"
#include "protocol.h"

#include <QGraphicsObject>
#include <QPixmap>

class TablePile : public GenericCardContainer
{
    Q_OBJECT

public:
    inline TablePile()
        : GenericCardContainer()
        , m_currentTime(0)
    {
        m_timer = startTimer(S_CLEARANCE_UPDATE_INTERVAL_MSEC);
    }
    QList<CardItem *> removeCardItems(const QList<int> &card_ids, Player::Place place) override;
    inline void setSize(QSize newSize)
    {
        setSize(newSize.width(), newSize.height());
    }
    void setSize(double width, double height);
    inline void setNumCardsVisible(int num)
    {
        m_numCardsVisible = num;
    }
    inline int getNumCardsVisible()
    {
        return m_numCardsVisible;
    }
    inline void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override
    {
    }
    void adjustCards();
    QRectF boundingRect() const override;
    void showJudgeResult(int cardId, bool takeEffect);

public slots:
    void clear(bool delayRequest = true);

protected:
    // This function must be called with mutex_pileCards locked.
    void _fadeOutCardsLocked(const QList<CardItem *> &cards);
    static const int S_CLEARANCE_UPDATE_INTERVAL_MSEC = 1000;
    static const int S_CLEARANCE_DELAY_BUCKETS = 3;
    void timerEvent(QTimerEvent *) override;
    bool _addCardItems(QList<CardItem *> &card_items, const CardsMoveStruct &moveInfo) override;
    void _markClearance(CardItem *item);
    QList<CardItem *> m_visibleCards;
    QMutex _m_mutex_pileCards;
    int m_numCardsVisible;
    QRect m_cardsDisplayRegion;
    int m_timer;
    int m_currentTime;
};

#endif
