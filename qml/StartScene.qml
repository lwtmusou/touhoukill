import QtQuick 2.6
import QtQuick.Window 2.2

Image {
    id: startScene
    source: Sanguosha.getUrl(Config.jsValue("BackgroundImage", ""))
    anchors.fill: parent

    Grid {
        id: btnGrid

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

            text: qsTr("Start game")
            onClicked: mainWindow.startGame();

            source: Sanguosha.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("General overview")
            onClicked: mainWindow.generalOverview();

            source: Sanguosha.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Start server")
            onClicked: mainWindow.startServer();

            source: Sanguosha.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Card overview")
            onClicked: mainWindow.cardOverview();

            source: Sanguosha.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("PC console start")
            onClicked: mainWindow.pcConsoleStart();

            source: Sanguosha.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Configure")
            onClicked: mainWindow.configure();

            source: Sanguosha.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Replay")
            onClicked: mainWindow.replay();

            source: Sanguosha.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("About us")
            onClicked: mainWindow.aboutUs();

            source: Sanguosha.getUrl("image/system/button/button.png")
        }
    }

    Image {
        anchors.horizontalCenter: startScene.horizontalCenter
        anchors.bottom: btnGrid.top
        anchors.bottomMargin: btnGrid.y / 2 - height / 2

        source: "../image/logo/logo.png"
    }

    Text {
        anchors.top: btnGrid.bottom
        anchors.horizontalCenter: btnGrid.right

        font.pixelSize: 20
        color: "white"

        text: qsTr("QQ qun: 384318315")
    }
}
