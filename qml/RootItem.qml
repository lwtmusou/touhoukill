import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: rootItem

    property Item currentScene: null

    Component.onCompleted: {
        currentScene = startSceneComponent.createObject(rootItem, {});
    }

    Component {
        id: startSceneComponent

        StartScene {
            anchors.fill: parent
        }
    }
    Component {
        id: roomSceneComponent

        RoomScene {
            anchors.fill: parent
        }
    }
    Connections {
        function onQml_switchToRoomScene() {
            currentScene.destroy();
            currentScene = roomSceneComponent.createObject(rootItem, {});
        }
        function onQml_switchToStartScene() {
            currentScene.destroy();
            currentScene = startSceneComponent.createObject(rootItem, {});
        }

        target: MainWindowInstance
    }
}
