#include "roomscene.h"

#include "client.h"
#include "clientplayer.h"

#include <QApplication>
#include <QtQml>

RoomScene::RoomScene(QQuickItem *parent)
    : QQuickItem(parent)
    , gameStarted(false)
    , gameOver(false)
{
}

RoomScene::~RoomScene() = default;

QObject *RoomScene::selfHelper() const
{
    return Self;
}

QObject *RoomScene::clientHelper() const
{
    return ClientInstance;
}

namespace {
void registerRoomScene()
{
    int ret = qmlRegisterType<RoomScene>("rocks.touhousatsu", 1, 0, "CppRoomScene");

    if (ret == -1)
        qDebug() << "Failed to register RoomScene to Qml";
}
} // namespace

// NOLINTNEXTLINE
Q_COREAPP_STARTUP_FUNCTION(registerRoomScene);
