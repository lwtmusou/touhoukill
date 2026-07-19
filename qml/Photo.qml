import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: photo

    property bool banling: player.linghp !== -2147483647 - 1
    property int dyingThreshold: player.dyingFactor
    property bool gameStarted: false
    property string general: (gameStarted ? (player.general) : (player != null ? player.avatar : "anjiang"))
    property string general2: (gameStarted ? (player.general2) : "")
    property int hp: (banling ? player.renhp : player.hp)
    property string huashenGeneral
    property string huashenGeneral2
    property string huashenSkillName
    property string huashenSkillName2
    property string kingdom: player.kingdom
    property int linghp: player.linghp
    property int maxhp: player.maxhp
    property real originalX
    property real originalY
    property int phase: Player.NotActive
    property ClientPlayer player
    property var privatePile: ({})
    property string role: player.role
    property bool roleShown: player.role_shown
    property string screenName: player.screenname
    required property int seat
    property bool selfPhoto: false

    function getGeneralName(g: string): string {
        if (g === "")
            return "";

        var toTranslate = "&" + g;
        var r = Sanguosha.translate(toTranslate);

        if (r == toTranslate)
            r = Sanguosha.translate(g);

        if (r == toTranslate && g.endsWith("_hegemony")) {
            toTranslate = "&" + g.substring(0, g.length - 9);
            r = Sanguosha.translate(toTranslate);
            if (r == toTranslate)
                r = Sanguosha.translate(g.substring(0, g.length - 9));
        }

        return r;
    }

    function getImageSourceUrl(g: string): url {
        if (g === "")
            return null;

        var generalImageFileName = g;
        if (g.endsWith("_hegemony") && !G.assetExists("image/fullskin/generals/full/" + g + ".png"))
            generalImageFileName = g.substring(0, g.length - 9);

        return G.getAssetUrl("image/fullskin/generals/full/" + generalImageFileName + ".png");
    }

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
            var general2Temp = general2;
            general2 = "";
            general2 = general2Temp;
        }
    }
    onGeneral2Changed: {
        if (general2 === "") {
            generalImage.width = Qt.binding(function () {
                return photo.width;
            });
        } else {
            generalImage.width = Qt.binding(function () {
                return photo.width / 2;
            });
        }
    }
    onHuashenGeneral2Changed: {
        if (huashenGeneral2 === "") {
            huashen2Image.visible = false;
        } else {
            var huashen2ImageFileName = huashenGeneral2;
            if (huashenGeneral2.endsWith("_hegemony"))
                huashen2ImageFileName = huashenGeneral2.substring(0, huashenGeneral2.length - 9);

            huashen2Image.source = G.getAssetUrl("image/fullskin/generals/full/" + huashen2ImageFileName + ".png");
            huashen2Image.visible = true;
        }
    }
    onHuashenGeneralChanged: {
        if (huashenGeneral === "") {
            huashenImage.visible = false;
        } else {
            var huashenImageFileName = huashenGeneral;
            if (huashenGeneral.endsWith("_hegemony"))
                huashenImageFileName = huashenGeneral.substring(0, huashenGeneral.length - 9);

            huashenImage.source = G.getAssetUrl("image/fullskin/generals/full/" + huashenImageFileName + ".png");
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
        source: (player != null && player.duozhi) ? getImageSourceUrl("yingyingguai") : getImageSourceUrl(general)
        width: photo.width

        Image {
            id: huashenImage

            anchors.fill: parent
            cache: false
            clip: true
            fillMode: Image.PreserveAspectCrop
            opacity: 0
            source: getImageSourceUrl(huashenGeneral)
            visible: huashenGeneral != ""

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
            source: G.getAssetUrl("image/kingdom/frame/" + photo.kingdom + ".png")
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
                verticalText: getGeneralName(general)
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
        source: (player != null && player.duozhi) ? getImageSourceUrl("yingyingguai") : getImageSourceUrl(general2)
        visible: general2 != ""
        width: photo.width / 2.

        Image {
            id: huashen2Image

            anchors.fill: parent
            cache: false
            clip: true
            fillMode: Image.PreserveAspectCrop
            opacity: 0
            source: getImageSourceUrl(huashenGeneral2)
            visible: huashenGeneral2 != ""

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

        Image {
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: banner.height - 8
            height: 196
            source: G.getAssetUrl("image/kingdom/frame/" + photo.kingdom + ".png")
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
                verticalText: getGeneralName(general2)
                width: 45
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
            color: Qt.rgba(255, 255, 255, 1)
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
