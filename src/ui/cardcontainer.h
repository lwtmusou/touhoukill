#ifndef _CARD_CONTAINER_H
#define _CARD_CONTAINER_H

class CardItem;
class Player;

#include "GenericCardContainerUI.h"
#include "QSanSelectableItem.h"
#include "carditem.h"

#include <QStack>

class SanCloseButton : public QSanSelectableItem
{
    Q_OBJECT

public:
    SanCloseButton();

protected:
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override;
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override;

signals:
    void clicked();
};

class CardContainer : public GenericCardContainer
{
    Q_OBJECT

public:
    explicit CardContainer();
    QList<CardItem *> removeCardItems(const QList<int> &card_ids, QSanguosha::Place place) override;
    int getFirstEnabled() const;
    void startChoose();
    void startGongxin(const QList<int> &enabled_ids);
    void addCloseButton();
    void view(const Player *player);
    QRectF boundingRect() const override;
    Player *m_currentPlayer;
    void paint(QPainter *, const QStyleOptionGraphicsItem *, QWidget *) override;
    bool retained();

public slots:
    void fillCards(const QList<int> &card_ids = QList<int>(), const QList<int> &disabled_ids = QList<int>(), const QList<int> &showHandcard_ids = QList<int>());
    void clear();
    void freezeCards(bool is_disable);
    void fillGeneralCards(const QList<CardItem *> &card_items = QList<CardItem *>(), const QList<CardItem *> &disabled_item = QList<CardItem *>());

protected:
    QRectF _m_boundingRect;
    bool _addCardItems(QList<CardItem *> &card_items, const LegacyCardsMoveStruct &moveInfo) override;
    int scene_width;
    int itemCount;
    static const int cardInterval = 3;

private:
    QList<CardItem *> items;
    SanCloseButton *close_button;
    QPixmap _m_background;
    QStack<QList<CardItem *>> items_stack;
    QStack<bool> retained_stack;

    void _addCardItem(int card_id, const QPointF &pos);

private slots:
    void grabItem();
    void chooseItem();
    void gongxinItem();

signals:
    void item_chosen(int card_id);
    void item_gongxined(int card_id);
};

class GuanxingBox : public QSanSelectableItem
{
    Q_OBJECT

public:
    GuanxingBox();
    void clear();
    void reply();

public slots:
    void doGuanxing(const QList<int> &card_ids, bool up_only);
    void adjust();

private:
    QList<CardItem *> up_items, down_items;
    bool up_only;

    static const int start_x = 76;
    static const int start_y1 = 105;
    static const int start_y2 = 249;
    static const int middle_y = 173;
    static const int skip = 102;
};

#endif
