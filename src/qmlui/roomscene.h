#ifndef TOUHOUKILL_ROOMSCENE_H_
#define TOUHOUKILL_ROOMSCENE_H_

#include <QQuickItem>

class RoomScene : public QQuickItem
{
    Q_OBJECT

public:
    using QQuickItem::QQuickItem;
    ~RoomScene() override;

    Q_DISABLE_COPY_MOVE(RoomScene)
};

#endif
