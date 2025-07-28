import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: rootItem

    anchors.fill: parent

    StartScene {
        id: startScene
        anchors.fill: parent
    }

    RoomScene {
        id: roomScene

        anchors.fill: parent
        visible: false
    }

    Connections {
        target: MainWindowInstance
        function onQml_switchToRoomScene() {
            startScene.visible = false;
            roomScene.visible = true;
        }
        function onQml_switchToStartScene() {
            roomScene.visible = false;
            startScene.visible = true;
        }
    }
}
