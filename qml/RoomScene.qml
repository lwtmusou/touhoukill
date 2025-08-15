import QtQuick 6.5

import rocks.touhousatsu 1.0

CppRoomScene {
    id: roomScene

    signal spaceClicked

    Image {
        source: G.getUrl(Config.TableBgImage)
        fillMode: Image.PreserveAspectCrop

        anchors.fill: parent

        MouseArea {
            anchors.fill: parent
            onClicked: roomScene.spaceClicked()
        }

        QSanButton {
            id: startSceneButton

            anchors.centerIn: parent
            width: parent.width / 4
            height: parent.height / 4

            text: "return to start scene"
            font.pixelSize: 50

            source: G.getUrl("image/system/button/button.png")

            onClicked: MainWindowInstance.gotoStartScene()
        }
    }
}
