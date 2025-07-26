import QtQuick 6.2
import QtQuick.Window 6.2

Image {
    id: startScene
    source: "../backdrop/hall/gensoukyou_1.jpg"
    fillMode: Image.PreserveAspectCrop
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
            onClicked: MainWindowInstance.on_actionStart_Game_triggered();

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("General overview")
            onClicked: MainWindowInstance.on_actionGeneral_Overview_triggered();

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Start server")
            onClicked: MainWindowInstance.on_actionStart_Server_triggered();

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Card overview")
            onClicked: MainWindowInstance.on_actionCard_Overview_triggered();

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("PC console start")
            onClicked: MainWindowInstance.on_actionPC_Console_Start_triggered();

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Configure")
            onClicked: MainWindowInstance.on_actionConfigure_triggered();

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Replay")
            onClicked: MainWindowInstance.on_actionReplay_triggered();

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("About us")
            onClicked: MainWindowInstance.on_actionAbout_Us_triggered();

            source: "../image/system/button/button.png"
        }
    }

    Image {
        anchors.horizontalCenter: startScene.horizontalCenter
        anchors.bottom: btnGrid.top
        anchors.bottomMargin: btnGrid.y / 2 - height / 2

        clip: false
        fillMode: Image.PreserveAspectCrop

        source: "../image/logo/logo.png"
    }

    Text {
        anchors.top: btnGrid.bottom
        anchors.horizontalCenter: btnGrid.right

        font.pixelSize: 20
        color: "white"

        text: qsTr("TouhouSatsu QQ qun: 384318315")
    }
}
