import QtQuick 6.5

import rocks.touhousatsu 1.0

CppRoomScene {
    id: roomScene

    Image {
        source: "../" + Config.TableBgImage
        fillMode: Image.PreserveAspectCrop

        anchors.fill: parent

        QSanButton {
            anchors.centerIn: parent
            width: parent.width / 4
            height: parent.height / 4

            text: "return to start scene"
            font.pixelSize: 50

            source: "../image/system/button/button.png"

            onClicked: MainWindowInstance.gotoStartScene()
        }
    }
}
