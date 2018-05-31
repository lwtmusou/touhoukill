import QtQuick 2.6

Rectangle {
    id: startScene
    color: "green"
    anchors.fill: parent

    Grid {
        anchors.horizontalCenter: startScene.horizontalCenter
        anchors.top: startScene.top
        anchors.topMargin: startScene.height / 3

        width: parent.width / 2
        height: parent.height / 2

        columns: 2
        QSanButton {
            width: parent.width / 2
            height: parent.height / 4

            text: "Start game"
            onClicked: mainWindow.startGame();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: parent.width / 2
            height: parent.height / 4

            text: "General overview"
            onClicked: mainWindow.generalOverview();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: parent.width / 2
            height: parent.height / 4

            text: "Start server"
            onClicked: mainWindow.startServer();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: parent.width / 2
            height: parent.height / 4

            text: "Card overview"
            onClicked: mainWindow.cardOverview();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: parent.width / 2
            height: parent.height / 4

            text: "PC console start"
            onClicked: mainWindow.pcConsoleStart();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: parent.width / 2
            height: parent.height / 4

            text: "Configure"
            onClicked: mainWindow.configure();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: parent.width / 2
            height: parent.height / 4

            text: "Replay"
            onClicked: mainWindow.replay();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: parent.width / 2
            height: parent.height / 4

            text: "About us"
            onClicked: mainWindow.aboutUs();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }
    }


}
