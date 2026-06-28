#ifndef TOUHOUKILL_ROOMSCENE_H_
#define TOUHOUKILL_ROOMSCENE_H_

#include <QQuickItem>

class RoomScene : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool gameStarted MEMBER gameStarted NOTIFY gameStartedChanged)

public:
    explicit RoomScene(QQuickItem *parent = nullptr);
    ~RoomScene() override;

    Q_DISABLE_COPY_MOVE(RoomScene)

signals:
    void gameStartedChanged(bool newGameStarted);

private:
    bool gameStarted;
};

#endif
