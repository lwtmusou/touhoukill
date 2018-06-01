import QtQuick 2.6
import QtQuick.Window 2.2

Rectangle {
    id: startScene
    color: "green"
    anchors.fill: parent

    Grid {
        anchors.horizontalCenter: startScene.horizontalCenter
        anchors.top: startScene.top
        anchors.topMargin: (startScene.height * 5 - height * 4) / 8

        width: {
            if (parent.width / 2 < Screen.desktopAvailableWidth / 3.5)
                return parent.width / 2;
            else
                return Screen.desktopAvailableWidth / 3.5
        }
        height: {
            if (parent.height / 2 < Screen.desktopAvailableHeight / 3)
                return parent.height / 2
            else
                return Screen.desktopAvailableHeight / 3
        }

        columns: 2
        spacing: {
            var values = Array(height / 100, width / 100, 5);
            var min = values[0];

            for (var i = 1; i < values.length; ++i)
                if (values[i] < min)
                    min = values[i];

            return min;
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: "Start game"
            onClicked: mainWindow.startGame();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: "General overview"
            onClicked: mainWindow.generalOverview();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: "Start server"
            onClicked: mainWindow.startServer();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: "Card overview"
            onClicked: mainWindow.cardOverview();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: "PC console start"
            onClicked: mainWindow.pcConsoleStart();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: "Configure"
            onClicked: mainWindow.configure();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: "Replay"
            onClicked: mainWindow.replay();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: "About us"
            onClicked: mainWindow.aboutUs();

            upSource: "../image/system/button/button.png"
            upHoveredSource: "../image/system/state.png"
            downSource: "../image/system/state.png"
            downHoveredSource: "../image/system/state.png"
        }
    }


}
