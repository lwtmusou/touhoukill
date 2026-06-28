import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: photo

    property bool banling: false
    property int dyingThreshold: 0
    property bool gameStarted: false
    property string general
    property string general2
    property int hp: 5
    property string huashenGeneral
    property string huashenGeneral2
    property string huashenSkillName
    property string huashenSkillName2
    property string kingdom
    property int linghp: 5
    property int maxhp: 5
    property real originalX
    property real originalY
    property int phase: Player.NotActive
    property string playerName
    property var privatePile: ({})
    property string role
    property bool roleShown: false
    property string screenName
    required property int seat
    property bool selfPhoto: false

    height: 407
    width: 336

    Component.onCompleted: {
        if (G.isHegemonyGameMode(ServerInfo.GameMode)) {
            hegRoleComboBox.visible = true;
            seatNumber.visible = true;
        } else if (G.isNormalGameMode(ServerInfo.GameMode)) {
            roleComboBox.visible = true;
            kingdomImage.visible = true;
        }

        // patch for specifying general2 initially
        if (general2 != "") {
            generalImage.width = Qt.binding(function () {
                return photo.width / 2;
            });
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
    onGeneralChanged: {
        if (general === "") {
            general = "yingyingguai";
            return;
        }

        generalImage.source = G.getUrl("image/fullskin/generals/full/" + general + ".png");
    }
    onHuashenGeneral2Changed: {
        if (huashenGeneral2 === "") {
            huashen2Image.visible = false;
        } else {
            huashen2Image.source = G.getUrl("image/fullskin/generals/full/" + huashenGeneral2 + ".png");
            huashen2Image.visible = true;
        }
    }
    onHuashenGeneralChanged: {
        if (huashenGeneral === "") {
            huashenImage.visible = false;
        } else {
            huashenImage.source = G.getUrl("image/fullskin/generals/full/" + huashenGeneral + ".png");
            huashenImage.visible = true;
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

    Image {
        id: generalImage

        anchors.left: photo.left
        anchors.top: photo.top
        cache: false
        clip: true
        fillMode: Image.PreserveAspectCrop
        height: photo.height
        source: G.getUrl("image/fullskin/generals/full/yingyingguai.png")
        width: photo.width

        Image {
            id: huashenImage

            anchors.fill: parent
            cache: false
            clip: true
            fillMode: Image.PreserveAspectCrop
            opacity: 0
            visible: false

            onVisibleChanged: {
                if (visible)
                    huashenImageAnimation.start();
                else
                    huashenImageAnimation.stop();
            }

            SequentialAnimation {
                id: huashenImageAnimation

                loops: Animation.Infinite

                PropertyAnimation {
                    duration: 500
                    from: 0
                    property: "opacity"
                    target: huashenImage
                    to: 1
                }

                PauseAnimation {
                    duration: 4000
                }

                PropertyAnimation {
                    duration: 500
                    from: 1
                    property: "opacity"
                    target: huashenImage
                    to: 0
                }

                PauseAnimation {
                    duration: 1000
                }
            }
        }

        Image {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: banner.height - 8
            height: 196
            source: G.getUrl("image/kingdom/frame/" + photo.kingdom + ".png")
            visible: photo.gameStarted
            width: 72

            VerticalText {
                anchors.centerIn: parent
                color: "#000000"
                font.family: G.GameFontFace
                font.pixelSize: 72
                fontSizeMode: Text.Fit
                height: 140
                horizontalAlignment: Text.AlignHCenter
                minimumPixelSize: 1
                style: Text.Outline
                styleColor: "#BBBBBB"
                verticalAlignment: Text.AlignVCenter
                verticalText: {
                    var toTranslate = "&" + photo.general;
                    var r = Sanguosha.translate(toTranslate);

                    if (r == toTranslate)
                        r = Sanguosha.translate(photo.general);

                    return r;
                }
                width: 45
            }
        }
    }

    Image {
        id: general2Image

        anchors.right: photo.right
        anchors.top: photo.top
        cache: false
        clip: true
        fillMode: Image.PreserveAspectCrop
        height: photo.height
        visible: false
        width: photo.width / 2.

        Image {
            id: huashen2Image

            anchors.fill: parent
            cache: false
            clip: true
            fillMode: Image.PreserveAspectCrop
            opacity: 0
            visible: false

            onVisibleChanged: {
                if (visible)
                    huashen2ImageAnimation.start();
                else
                    huashen2ImageAnimation.stop();
            }

            SequentialAnimation {
                id: huashen2ImageAnimation

                loops: Animation.Infinite

                PropertyAnimation {
                    duration: 500
                    from: 0
                    property: "opacity"
                    target: huashen2Image
                    to: 1
                }

                PauseAnimation {
                    duration: 4000
                }

                PropertyAnimation {
                    duration: 500
                    from: 1
                    property: "opacity"
                    target: huashen2Image
                    to: 0
                }

                PauseAnimation {
                    duration: 1000
                }
            }
        }
    }

    Rectangle {
        id: banner

        anchors.left: parent.left
        anchors.right: rightRect.left
        anchors.top: parent.top
        color: Qt.rgba(0, 0, 0, 0.5)
        height: photo.height / 12

        Text {
            anchors.centerIn: parent
            font.pixelSize: 50
            fontSizeMode: Text.Fit
            height: parent.height / 1.2
            horizontalAlignment: Qt.AlignHCenter
            text: photo.screenName
            verticalAlignment: Qt.AlignVCenter
        }

        Item {
            id: seatNumberOrKingdomImageItem

            anchors.fill: parent
            visible: photo.gameStarted

            SeatNumberItem {
                id: seatNumber

                anchors.left: parent.left
                anchors.top: parent.top
                height: parent.height
                seat: photo.seat
                visible: false
            }

            KingdomImage {
                id: kingdomImage

                anchors.left: parent.left
                anchors.top: parent.top
                kingdom: photo.kingdom
                visible: false
            }
        }
    }

    Rectangle {
        id: rightRect

        anchors.right: photo.right
        anchors.top: photo.top
        color: Qt.rgba(0.1, 0.1, 0.1, 0.8)
        height: photo.height
        width: photo.width / 6.34

        Magatamas {
            id: magatamas

            anchors.fill: parent
            banling: photo.banling
            dyingThreshold: photo.dyingThreshold
            hp: photo.hp
            linghp: photo.linghp
            maxhp: photo.maxhp
            visible: photo.gameStarted
            width: parent.width
        }

        HandcardNum {
            id: handcardNum

            anchors.bottom: parent.bottom
            anchors.right: parent.right
            kingdom: photo.kingdom
            visible: photo.gameStarted
        }

        Item {
            id: roleComboBoxItem

            anchors.fill: parent
            visible: photo.gameStarted

            RoleComboBox {
                id: roleComboBox

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                roleShown: photo.roleShown
                visible: false
            }

            HegRoleComboBox {
                id: hegRoleComboBox

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: parent.top
                roleShown: photo.roleShown
                visible: false
            }
        }
    }

    PhaseItem {
        anchors.horizontalCenter: photo.horizontalCenter
        anchors.top: photo.bottom
        phase: photo.phase
        visible: photo.gameStarted
    }
}
