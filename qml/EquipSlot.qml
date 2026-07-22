import QtQuick 6.5
import rocks.touhousatsu 1.0

// Single equip slot. Mirrors old _getEquipPixmap with skin layout. The icon image
// itself contains the equip name (skin equipTextArea is [0,0,0,0]), so no
// equip-name text and no distance text. Configurable for Dashboard (image/equips,
// 149x25, dashboard layout) vs Photo (image/fullskin/small-equips, 140x19, photo
// layout). Broken placeholder / distance / click routing belong to the equip-area
// task; subtask A covers sync only.
Item {
    id: slot

    property int cardId: -1
    property string equipIconDir: "image/equips/"
    property int equipIconHeight: 25
    property int equipIconWidth: 149
    property string equipNumberString: ""
    property string equipObjectName: ""
    property var pointArea: [117, -5, 30, 30]
    property bool red: false
    property string suit: ""
    property var suitArea: [128, 5, 21, 17]

    height: 51
    visible: cardId !== -1
    width: 307

    onCardIdChanged: {
        if (cardId === -1) {
            equipObjectName = "";
            return;
        }

        var card = Sanguosha.getEngineCard(cardId);
        equipObjectName = card.objectName;
        suit = card.suit;
        equipNumberString = card.number_string;
        red = card.red;
    }

    // equipImageArea: icon image already contains the equip name.
    Image {
        id: equipIcon

        anchors.left: parent.left
        anchors.top: parent.top
        height: slot.equipIconHeight
        source: slot.equipObjectName !== "" ? G.getAssetUrl(slot.equipIconDir + slot.equipObjectName + ".png") : ""
        width: slot.equipIconWidth

        Image {
            fillMode: Image.PreserveAspectFit
            height: slot.suitArea[3]
            source: slot.suit !== "" ? G.getAssetUrl("image/system/cardsuit/" + slot.suit + ".png") : ""
            visible: slot.suit !== ""
            width: slot.suitArea[2]
            x: slot.suitArea[0]
            y: slot.suitArea[1]
        }

        Text {
            color: slot.red ? "#CC0000" : "#000000"
            font.pixelSize: 20
            height: slot.pointArea[3]
            horizontalAlignment: Text.AlignHCenter
            text: slot.equipNumberString
            verticalAlignment: Text.AlignVCenter
            width: slot.pointArea[2]
            x: slot.pointArea[0]
            y: slot.pointArea[1]
        }
    }
}
