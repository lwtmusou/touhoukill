import QtQuick 6.5
import rocks.touhousatsu 1.0

// Base for equip area: 5 fixed slots (weapon/armor/defensive-horse/offensive-horse/
// treasure, index = EquipCard.Location). Holds equipCardIds + addEquip/removeEquip.
// Configurable icon dir/size/suitArea/pointArea/slot layout so SelfEquipArea and
// PhotoEquipArea derive (set properties) for Dashboard vs Photo layouts.
// TODO (equip-area task): click routing / valid state / broken placeholder /
// distance text / equip skill button. TODO (animation): Repeater itemAt(location)
// yields the EquipSlot instance for card-fly target positioning.
Item {
    id: equipAreaBase

    property var equipCardIds: [-1, -1, -1, -1, -1]
    property string equipIconDir: "image/equips/"
    property int equipIconHeight: 25
    property int equipIconWidth: 149
    property var pointArea: [117, -5, 30, 30]
    property int slotHeight: 51
    property int slotStep: 57
    property int slotWidth: 307
    property var suitArea: [128, 5, 21, 17]

    function addEquip(location: int, cardId: int) {
        var a = equipCardIds.slice();
        a[location] = cardId;
        equipCardIds = a;
    }

    function removeEquip(location: int) {
        var a = equipCardIds.slice();
        a[location] = -1;
        equipCardIds = a;
    }

    height: 4 * slotStep + slotHeight
    width: slotWidth

    Repeater {
        model: 5

        EquipSlot {
            cardId: equipAreaBase.equipCardIds[index]
            equipIconDir: equipAreaBase.equipIconDir
            equipIconHeight: equipAreaBase.equipIconHeight
            equipIconWidth: equipAreaBase.equipIconWidth
            height: equipAreaBase.slotHeight
            pointArea: equipAreaBase.pointArea
            suitArea: equipAreaBase.suitArea
            width: equipAreaBase.slotWidth
            y: index * equipAreaBase.slotStep
        }
    }
}
