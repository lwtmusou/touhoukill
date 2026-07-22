import QtQuick 6.5

import rocks.touhousatsu 1.0

Item {
    id: photo

    property bool banling: player.linghp !== -2147483647 - 1
    property int dyingThreshold: player.dyingFactor
    // Equip slot cardIds for non-self photos (5 fixed slots, -1 = empty).
    // Driven by PlaceEquip moves (subtask A sync). self uses Dashboard's equip area.
    property var equipCardIds: [-1, -1, -1, -1, -1]
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
    property int phase: player.phaseValue

    // PhaseItem: selfPhoto does NOT construct (uses Dashboard's PhaseItem);
    // non-selfPhoto dynamically constructs via createObject on gameStarted.
    // Same pattern will apply to equip area etc. -- selfPhoto doesn't construct,
    // non-selfPhoto uses dynamic construction.
    property var phaseItemInstance: null
    property ClientPlayer player
    property var privatePile: ({})
    property string role: player.role
    property bool roleShown: player.role_shown
    property string screenName: player.screenname
    required property int seat
    required property bool selfPhoto

    // Equip area sync (subtask A): route PlaceEquip moves to the 5 fixed slots.
    function addEquip(location: int, cardId: int) {
        var a = equipCardIds.slice();
        a[location] = cardId;
        equipCardIds = a;
    }

    function createPhaseItem() {
        photo.phaseItemInstance = phaseItemComponent.createObject(photo);
        photo.phaseItemInstance.anchors.horizontalCenter = Qt.binding(() => photo.horizontalCenter);
        photo.phaseItemInstance.anchors.top = Qt.binding(() => photo.bottom);
        photo.phaseItemInstance.phase = Qt.binding(() => photo.phase);
        photo.phaseItemInstance.visible = Qt.binding(() => photo.gameStarted);
    }

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

    function removeEquip(location: int) {
        var a = equipCardIds.slice();
        a[location] = -1;
        equipCardIds = a;
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

        if (!selfPhoto)
            photo.createPhaseItem();

        // patch for specifying general2 initially during test.
        // Remove after RoomScene.testItemToBeRemovedAfterTest is removed
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

    // Non-self equip area (subtask A sync). Overlays Photo bottom; uses small-equip
    // icons (image/fullskin/small-equips, 140x19) per photo layout. self uses
    // Dashboard's equip area. TODO (equip-area task): split into PhotoEquipArea +
    // dynamic createObject. Repeater index 0..4 == EquipCard.Location enum value.
    Item {
        id: equipArea

        anchors.bottom: photo.bottom
        anchors.left: photo.left
        height: 5 * 33 - 14
        visible: !selfPhoto
        width: 140

        Repeater {
            model: 5

            EquipSlot {
                cardId: photo.equipCardIds[index]
                equipIconDir: "image/fullskin/small-equips/"
                equipIconHeight: 19
                equipIconWidth: 140
                height: 19
                pointArea: [106, -4, 25, 25]
                suitArea: [117, 2, 21, 17]
                width: 140
                y: index * 33
            }
        }
    }

    Component {
        id: phaseItemComponent

        PhaseItem {
        }
    }
}
