#ifndef TOUHOUKILL_ROOMSCENE_H_
#define TOUHOUKILL_ROOMSCENE_H_

#include <QQuickItem>

class RoomScene : public QQuickItem
{
    Q_OBJECT
    Q_PROPERTY(bool gameStarted MEMBER gameStarted NOTIFY gameStartedChanged)
    Q_PROPERTY(bool gameOver MEMBER gameOver NOTIFY gameOverChanged)
    Q_PROPERTY(QObject *Self READ selfHelper STORED false)
    Q_PROPERTY(QObject *ClientInstance READ clientHelper STORED false)

public:
    explicit RoomScene(QQuickItem *parent = nullptr);
    ~RoomScene() override;

    Q_DISABLE_COPY_MOVE(RoomScene)

    QObject *selfHelper() const;
    QObject *clientHelper() const;

signals:
    void gameStartedChanged(bool newGameStarted);
    void gameOverChanged(bool newGameOver);

private:
    bool gameStarted;
    bool gameOver;
};

#endif
