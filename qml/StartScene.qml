import QtQuick 2.6

Rectangle {
    color: "green"
    anchors.fill: parent

    QSanButton {
        anchors.centerIn: parent
        text: "start game"
        onClicked: mainWindow.startGame();
        width: parent.width / 5
        height: parent.height / 5

        upSource: "../image/system/button/button.png"
        upHoveredSource: "../image/system/state.png"
        downSource: "../image/system/state.png"
        downHoveredSource: "../image/system/state.png"
    }

}
