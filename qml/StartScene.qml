import QtQuick 6.5

import rocks.touhousatsu 1.0

Image {
    id: startScene

    anchors.fill: parent
    fillMode: Image.PreserveAspectCrop
    source: G.getUrl(Config.BackgroundImage)

    Grid {
        id: btnGrid

        anchors.horizontalCenter: startScene.horizontalCenter
        anchors.top: startScene.top
        anchors.topMargin: (startScene.height * 5 - height * 4) / 8
        columns: 2
        height: parent.height / 2
        spacing: {
            var values = Array(height / 100, width / 100, 5);
            var min = values[0];

            for (var i = 1; i < values.length; ++i)
                if (values[i] < min)
                    min = values[i];

            return min;
        }
        width: parent.width / 2

        QSanButton {
            font.pixelSize: 53
            height: (parent.height - parent.spacing * 3) / 4
            source: G.getUrl("image/system/button/button.png")
            text: qsTr("Start game")
            width: (parent.width - parent.spacing) / 2

            onClicked: MainWindowInstance.on_actionStart_Game_triggered()
        }

        QSanButton {
            font.pixelSize: 53
            height: (parent.height - parent.spacing * 3) / 4
            source: G.getUrl("image/system/button/button.png")
            text: qsTr("General overview")
            width: (parent.width - parent.spacing) / 2

            onClicked: MainWindowInstance.on_actionGeneral_Overview_triggered()
        }

        QSanButton {
            font.pixelSize: 53
            height: (parent.height - parent.spacing * 3) / 4
            source: G.getUrl("image/system/button/button.png")
            text: qsTr("Start server")
            width: (parent.width - parent.spacing) / 2

            onClicked: MainWindowInstance.on_actionStart_Server_triggered()
        }

        QSanButton {
            font.pixelSize: 53
            height: (parent.height - parent.spacing * 3) / 4
            source: G.getUrl("image/system/button/button.png")
            text: qsTr("Card overview")
            width: (parent.width - parent.spacing) / 2

            onClicked: MainWindowInstance.on_actionCard_Overview_triggered()
        }

        QSanButton {
            font.pixelSize: 53
            height: (parent.height - parent.spacing * 3) / 4
            source: G.getUrl("image/system/button/button.png")
            text: qsTr("PC Console Start")
            width: (parent.width - parent.spacing) / 2

            onClicked: MainWindowInstance.on_actionPC_Console_Start_triggered()
        }

        QSanButton {
            font.pixelSize: 53
            height: (parent.height - parent.spacing * 3) / 4
            source: G.getUrl("image/system/button/button.png")
            text: qsTr("Configure")
            width: (parent.width - parent.spacing) / 2

            onClicked: MainWindowInstance.on_actionConfigure_triggered()
        }

        QSanButton {
            font.pixelSize: 53
            height: (parent.height - parent.spacing * 3) / 4
            source: G.getUrl("image/system/button/button.png")
            text: qsTr("Replay")
            width: (parent.width - parent.spacing) / 2

            onClicked: MainWindowInstance.on_actionReplay_triggered()
        }

        QSanButton {
            font.pixelSize: 53
            height: (parent.height - parent.spacing * 3) / 4
            source: G.getUrl("image/system/button/button.png")
            text: qsTr("About Us")
            width: (parent.width - parent.spacing) / 2

            onClicked: MainWindowInstance.on_actionAbout_Us_triggered()
        }
    }

    Image {
        id: logo

        clip: true
        fillMode: Image.PreserveAspectFit
        source: G.getUrl("image/logo/logo.png")
        x: 0

        Binding on height {
            id: logoHeightBinding

            value: btnGrid.y / 2
            when: true
        }
        Binding on width {
            id: logoWidthBinding

            value: parent.width
            when: true
        }
        Binding on y {
            id: logoYBinding

            value: btnGrid.y / 4
            when: true
        }
    }

    Text {
        id: groupText

        anchors.horizontalCenter: btnGrid.right
        anchors.top: btnGrid.bottom
        color: "white"
        font.pixelSize: 26
        text: qsTr("TouhouSatsu QQ Qun: 384318315")
    }

    Rectangle {
        id: serverTextBorder

        color: Qt.rgba(0.1, 0.1, 0.1, 0.5)
        visible: false

        Binding on height {
            id: serverTextHightBinding

            value: (parent.height - 144) * 0.8
            when: false
        }
        Binding on width {
            id: serverTextWidthBinding

            value: parent.width * 0.8
            when: false
        }
        Binding on x {
            id: serverTextXBinding

            value: parent.width * 0.1
            when: false
        }
        Binding on y {
            id: serverTextYBinding

            value: 144 + (parent.height - 144) * 0.1
            when: false
        }

        Flickable {
            id: flickable

            anchors.fill: parent
            clip: true
            contentHeight: serverText.contentHeight
            contentWidth: width
            contentY: {
                if (contentHeight < height)
                    return 0;
                else
                    return contentHeight - height;
            }

            TextEdit {
                id: serverText

                color: "white"
                enabled: false
                font.pixelSize: 20
                readOnly: true
                visible: true
                width: flickable.width
                wrapMode: TextEdit.Wrap
            }
        }
    }

    ParallelAnimation {
        id: serverTextAnimation

        running: false

        onFinished: {
            serverTextXBinding.when = true;
            serverTextWidthBinding.when = true;
            serverTextHightBinding.when = true;
        }

        PropertyAnimation {
            id: logoYAnimation

            duration: 400
            property: "y"
            target: logo
            to: 0
        }

        PropertyAnimation {
            duration: 400
            from: btnGrid.y / 2
            property: "height"
            target: logo
            to: 144
        }

        PropertyAnimation {
            duration: 400
            from: startScene.width
            property: "width"
            target: logo
            to: 360
        }

        PropertyAnimation {
            duration: 400
            from: startScene.width / 2
            property: "x"
            target: serverTextBorder
            to: startScene.width * 0.1
        }

        PropertyAnimation {
            duration: 400
            from: (startScene.height + 144) / 2
            property: "y"
            target: serverTextBorder
            to: 144 + (startScene.height - 144) * 0.1
        }

        PropertyAnimation {
            duration: 400
            from: 0
            property: "width"
            target: serverTextBorder
            to: startScene.width * 0.8
        }

        PropertyAnimation {
            duration: 400
            from: 0
            property: "height"
            target: serverTextBorder
            to: (startScene.height - 144) * 0.8
        }

        PropertyAnimation {
            duration: 400
            from: 0
            property: "opacity"
            target: serverTextBorder
            to: 1.0
        }
    }

    Connections {
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

        target: MainWindowInstance
    }
}
