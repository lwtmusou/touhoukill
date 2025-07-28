#include "roomscene.h"

#include <QApplication>
#include <QtQml>

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
