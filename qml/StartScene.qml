import QtQuick 6.5

import rocks.touhousatsu 1.0

Image {
    id: startScene
    source: G.getUrl(Config.BackgroundImage)
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
            font.pixelSize: 53

            source: G.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("General overview")
            onClicked: MainWindowInstance.on_actionGeneral_Overview_triggered()
            font.pixelSize: 53

            source: G.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Start server")
            onClicked: MainWindowInstance.on_actionStart_Server_triggered()
            font.pixelSize: 53

            source: G.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Card overview")
            onClicked: MainWindowInstance.on_actionCard_Overview_triggered()
            font.pixelSize: 53

            source: G.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("PC Console Start")
            onClicked: MainWindowInstance.on_actionPC_Console_Start_triggered()
            font.pixelSize: 53

            source: G.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Configure")
            onClicked: MainWindowInstance.on_actionConfigure_triggered()
            font.pixelSize: 53

            source: G.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("Replay")
            onClicked: MainWindowInstance.on_actionReplay_triggered()
            font.pixelSize: 53

            source: G.getUrl("image/system/button/button.png")
        }

        QSanButton {
            width: (parent.width - parent.spacing) / 2
            height: (parent.height - parent.spacing * 3) / 4

            text: qsTr("About Us")
            onClicked: MainWindowInstance.on_actionAbout_Us_triggered()
            font.pixelSize: 53

            source: G.getUrl("image/system/button/button.png")
        }
    }

    Image {
        id: logo
        clip: true
        fillMode: Image.PreserveAspectFit
        x: 0

        source: G.getUrl("image/logo/logo.png")

        Binding on width {
            id: logoWidthBinding
            when: true
            value: parent.width
        }

        Binding on height {
            id: logoHeightBinding
            when: true
            value: btnGrid.y / 2
        }

        Binding on y {
            id: logoYBinding
            when: true
            value: btnGrid.y / 4
        }
    }

    Text {
        id: groupText

        anchors.top: btnGrid.bottom
        anchors.horizontalCenter: btnGrid.right

        font.pixelSize: 26
        color: "white"

        text: qsTr("TouhouSatsu QQ Qun: 384318315")
    }

    Rectangle {
        id: serverTextBorder
        visible: false
        color: Qt.rgba(0.1, 0.1, 0.1, 0.5)

        Flickable {
            id: flickable
            anchors.fill: parent

            clip: true
            contentWidth: width
            contentHeight: serverText.contentHeight

            contentY: {
                if (contentHeight < height)
                    return 0;
                else
                    return contentHeight - height;
            }

            TextEdit {
                id: serverText
                width: flickable.width

                readOnly: true

                enabled: false
                visible: true

                font.pixelSize: 20
                color: "white"
                wrapMode: TextEdit.Wrap
            }
        }

        Binding on x {
            id: serverTextXBinding
            when: false
            value: parent.width * 0.1
        }

        Binding on width {
            id: serverTextWidthBinding
            when: false
            value: parent.width * 0.8
        }

        Binding on y {
            id: serverTextYBinding
            when: false
            value: 144 + (parent.height - 144) * 0.1
        }

        Binding on height {
            id: serverTextHightBinding
            when: false
            value: (parent.height - 144) * 0.8
        }
    }

    ParallelAnimation {
        id: serverTextAnimation
        running: false

        PropertyAnimation {
            id: logoYAnimation
            target: logo
            property: "y"
            to: 0
            duration: 400
        }
        PropertyAnimation {
            target: logo
            property: "height"
            from: btnGrid.y / 2
            to: 144
            duration: 400
        }
        PropertyAnimation {
            target: logo
            property: "width"
            from: startScene.width
            to: 360
            duration: 400
        }
        PropertyAnimation {
            target: serverTextBorder
            property: "x"
            from: startScene.width / 2
            to: startScene.width * 0.1
            duration: 400
        }
        PropertyAnimation {
            target: serverTextBorder
            property: "y"
            from: (startScene.height + 144) / 2
            to: 144 + (startScene.height - 144) * 0.1
            duration: 400
        }
        PropertyAnimation {
            target: serverTextBorder
            property: "width"
            from: 0
            to: startScene.width * 0.8
            duration: 400
        }
        PropertyAnimation {
            target: serverTextBorder
            property: "height"
            from: 0
            to: (startScene.height - 144) * 0.8
            duration: 400
        }
        PropertyAnimation {
            target: serverTextBorder
            property: "opacity"
            from: 0
            to: 1.0
            duration: 400
        }

        onFinished: {
            serverTextXBinding.when = true;
            serverTextWidthBinding.when = true;
            serverTextHightBinding.when = true;
        }
    }

    Connections {
        target: MainWindowInstance
        function onQml_switchToServerScene(server: QtObject) {
            btnGrid.visible = false;
            groupText.visible = false;

            logoWidthBinding.when = false;
            logoHeightBinding.when = false;
            logoYBinding.when = false;

            serverTextBorder.visible = true;
            serverText.enabled = true;

            logoYAnimation.from = btnGrid.y / 4;

            serverTextAnimation.start();

            MainWindowInstance.configureServerText(server, serverText);
        }
    }
}
