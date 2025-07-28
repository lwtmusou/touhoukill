import QtQuick 6.5

Image {
    id: startScene
    source: "../" + Config.BackgroundImage
    fillMode: Image.PreserveAspectCrop
    anchors.fill: parent

    Grid {
        id: btnGrid

        anchors.horizontalCenter: startScene.horizontalCenter
        anchors.top: startScene.top
        anchors.topMargin: (startScene.height * 5 - height * 4) / 8

        width: parent.width / 2
        height: parent.height / 2

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
            onClicked: MainWindowInstance.on_actionStart_Game_triggered()
            font.pixelSize: 40

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("General overview")
            onClicked: MainWindowInstance.on_actionGeneral_Overview_triggered()
            font.pixelSize: 40

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Start server")
            onClicked: MainWindowInstance.on_actionStart_Server_triggered()
            font.pixelSize: 40

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Card overview")
            onClicked: MainWindowInstance.on_actionCard_Overview_triggered()
            font.pixelSize: 40

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("PC Console Start")
            onClicked: MainWindowInstance.on_actionPC_Console_Start_triggered()
            font.pixelSize: 40

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Configure")
            onClicked: MainWindowInstance.on_actionConfigure_triggered()
            font.pixelSize: 40

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Replay")
            onClicked: MainWindowInstance.on_actionReplay_triggered()
            font.pixelSize: 40

            source: "../image/system/button/button.png"
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("About Us")
            onClicked: MainWindowInstance.on_actionAbout_Us_triggered()
            font.pixelSize: 40

            source: "../image/system/button/button.png"
        }
    }

    Image {
        id: logo

        anchors.horizontalCenter: startScene.horizontalCenter
        anchors.bottom: btnGrid.top
        anchors.bottomMargin: btnGrid.y / 2 - height / 2

        clip: false
        fillMode: Image.PreserveAspectFit

        source: "../image/logo/logo.png"
    }

    Text {
        id: groupText

        anchors.top: btnGrid.bottom
        anchors.horizontalCenter: btnGrid.right

        font.pixelSize: 20
        color: "white"

        text: qsTr("TouhouSatsu QQ Qun: 384318315")
    }

    Rectangle {
        id: serverTextBorder
        visible: false
        color: Qt.rgba(0.1, 0.1, 0.1, 0.5)

        TextEdit {
            id: serverText
            anchors.fill: parent

            readOnly: true

            enabled: false
            visible: true

            text: "this is the prefilled text"
        }

        Binding on width {
            id: serverTextWidthBinding
            when: false
            value: parent.width
        }
        Binding on height {
            id: serverTextHightBinding
            when: false
            value: parent.height - 108
        }
    }

    ParallelAnimation {
        id: serverTextAnimation

        running: false
        PropertyAnimation {
            target: logo
            property: "x"
            to: 0
            duration: 1000
        }
        PropertyAnimation {
            target: logo
            property: "y"
            to: 0
            duration: 1000
        }
        PropertyAnimation {
            target: logo
            property: "height"
            to: 108
            duration: 1000
        }
        PropertyAnimation {
            target: logo
            property: "width"
            to: 270
            duration: 1000
        }

        PropertyAnimation {
            target: serverTextBorder
            property: "x"
            from: startScene.width / 2
            to: 0
            duration: 1000
        }
        PropertyAnimation {
            target: serverTextBorder
            property: "y"
            from: (startScene.height + 108) / 2
            to: 108
            duration: 1000
        }

        PropertyAnimation {
            target: serverTextBorder
            property: "width"
            from: 0
            to: startScene.width
            duration: 1000
        }
        PropertyAnimation {
            target: serverTextBorder
            property: "height"
            from: 0
            to: startScene.height - 108
            duration: 1000
        }
        PropertyAnimation {
            target: serverTextBorder
            property: "opacity"
            from: 0
            to: 1.0
            duration: 1000
        }

        onFinished: {
            serverTextWidthBinding.when = true;
            serverTextHightBinding.when = true;
        }
    }

    Connections {
        target: MainWindowInstance
        function onQml_switchToServerScene(server) {
            btnGrid.visible = false;
            groupText.visible = false;
            logo.anchors.horizontalCenter = undefined;
            logo.anchors.bottom = undefined;
            logo.anchors.bottomMargin = undefined;

            serverTextBorder.visible = true;
            serverText.enabled = true;

            serverTextAnimation.start();

            MainWindowInstance.configureServerText(server, serverText);
        }
    }
}
