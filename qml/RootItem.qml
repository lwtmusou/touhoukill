import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: rootItem

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

    property Item currentScene: null

    Connections {
        target: MainWindowInstance
        function onQml_switchToRoomScene() {
            currentScene.destroy();
            currentScene = roomSceneComponent.createObject(rootItem, {});
        }
        function onQml_switchToStartScene() {
            currentScene.destroy();
            currentScene = startSceneComponent.createObject(rootItem, {});
        }
    }

    Component.onCompleted: {
        currentScene = startSceneComponent.createObject(rootItem, {});
    }
}
