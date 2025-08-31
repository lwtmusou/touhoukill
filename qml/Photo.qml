import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: photo

    width: 336
    height: 407

    property string kingdom
    property string general: ""
    property string huashenGeneral: ""
    property string general2: ""
    property string huashenGeneral2: ""
    property string role: ""
    property bool roleShown: false
    property int hp: 5
    property int maxhp: 5
    property int dyingThreshold: 0
    property int phase: Player.NotActive

    property string screenName: "东方杀"

    Image {
        id: generalImage
        cache: false
        fillMode: Image.PreserveAspectCrop
        clip: true

        anchors.top: photo.top
        anchors.left: photo.left

        height: photo.height
        width: photo.width

        source: G.getUrl("image/fullskin/generals/full/yingyingguai.png")

        Image {
            id: huashenImage
            cache: false
            fillMode: Image.PreserveAspectCrop
            clip: true

            anchors.fill: parent
            visible: false
            opacity: 0

            SequentialAnimation {
                id: huashenImageAnimation
                loops: Animation.Infinite

                PropertyAnimation {
                    target: huashenImage
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 500
                }

                PauseAnimation {
                    duration: 4000
                }

                PropertyAnimation {
                    target: huashenImage
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 500
                }

                PauseAnimation {
                    duration: 1000
                }
            }

            onVisibleChanged: {
                if (visible)
                    huashenImageAnimation.start();
                else
                    huashenImageAnimation.stop();
            }
        }
    }

    Image {
        id: general2Image
        visible: false
        cache: false
        fillMode: Image.PreserveAspectCrop
        clip: true

        anchors.top: photo.top
        anchors.right: photo.right
        height: photo.height
        width: photo.width / 2.

        Image {
            id: huashen2Image
            cache: false
            fillMode: Image.PreserveAspectCrop
            clip: true

            anchors.fill: parent
            visible: false
            opacity: 0

            SequentialAnimation {
                id: huashen2ImageAnimation
                loops: Animation.Infinite

                PropertyAnimation {
                    target: huashen2Image
                    property: "opacity"
                    from: 0
                    to: 1
                    duration: 500
                }

                PauseAnimation {
                    duration: 4000
                }

                PropertyAnimation {
                    target: huashen2Image
                    property: "opacity"
                    from: 1
                    to: 0
                    duration: 500
                }

                PauseAnimation {
                    duration: 1000
                }
            }

            onVisibleChanged: {
                if (visible)
                    huashen2ImageAnimation.start();
                else
                    huashen2ImageAnimation.stop();
            }
        }
    }

    Rectangle {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: rightRect.left
        height: photo.height / 12

        color: Qt.rgba(0, 0, 0, 0.5)

        Text {
            anchors.centerIn: parent
            height: parent.height / 1.2
            font.pixelSize: 50
            fontSizeMode: Text.Fit
            horizontalAlignment: Qt.AlignHCenter
            verticalAlignment: Qt.AlignVCenter
            text: screenName
        }
    }

    KingdomImage {
        id: kingdomImage

        visible: false

        anchors.horizontalCenter: photo.left
        anchors.verticalCenter: photo.top

        kingdom: photo.kingdom
    }

    Rectangle {
        id: rightRect

        color: Qt.rgba(0.1, 0.1, 0.1, 0.8)
        width: photo.width / 6.34
        height: photo.height
        anchors.right: photo.right
        anchors.top: photo.top

        Magatamas {
            id: magatamas
            width: parent.width

            visible: false

            anchors.fill: parent
            hp: photo.hp
            maxhp: photo.maxhp
            dyingThreshold: photo.dyingThreshold
        }

        HandcardNum {
            id: handcardNum

            anchors.bottom: parent.bottom
            anchors.right: parent.right

            // visible: false
            kingdom: photo.kingdom
        }

        RoleComboBox {
            id: roleComboBox

            visible: false

            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter
            roleShown: photo.roleShown
        }

        HegRoleComboBox {
            id: hegRoleComboBox

            visible: false

            anchors.top: parent.top
            anchors.horizontalCenter: parent.horizontalCenter

            roleShown: photo.roleShown
        }
    }

    PhaseItem {
        phase: photo.phase

        anchors.top: photo.bottom
        anchors.horizontalCenter: photo.horizontalCenter
    }

    onGeneralChanged: {
        if (general === "") {
            general = "yingyingguai";
            return;
        }

        generalImage.source = G.getUrl("image/fullskin/generals/full/" + general + ".png");
    }

    onHuashenGeneralChanged: {
        if (huashenGeneral === "") {
            huashenImage.visible = false;
        } else {
            huashenImage.source = G.getUrl("image/fullskin/generals/full/" + huashenGeneral + ".png");
            huashenImage.visible = true;
        }
    }

    onGeneral2Changed: {
        if (general2 === "") {
            general2Image.visible = false;
            generalImage.width = Qt.binding(function () {
                return photo.width;
            });
            return;
        }
        generalImage.width = Qt.binding(function () {
            return photo.width / 2;
        });
        general2Image.visible = true;
        general2Image.source = G.getUrl("image/fullskin/generals/full/" + general2 + ".png");
    }

    onHuashenGeneral2Changed: {
        if (huashenGeneral2 === "") {
            huashen2Image.visible = false;
        } else {
            huashen2Image.source = G.getUrl("image/fullskin/generals/full/" + huashenGeneral2 + ".png");
            huashen2Image.visible = true;
        }
    }

    onRoleChanged: {
        if (role !== "") {
            roleComboBox.role = role;
            roleComboBox.fixed = true;
            hegRoleComboBox.role = role;
            hegRoleComboBox.fixed = true;
        }
    }

    Component.onCompleted: {
        if (G.isHegemonyGameMode(ServerInfo.GameMode)) {
            hegRoleComboBox.visible = true;
        } else if (G.isNormalGameMode(ServerInfo.GameMode)) {
            roleComboBox.visible = true;
            kingdomImage.visible = true;
        }
    }
}
