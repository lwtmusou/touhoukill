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
    // TODO: consider how to get this after Client is no longer global singleton.
    // This RoomScene class is created by QML and can only have default constructor.
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
