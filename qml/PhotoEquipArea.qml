import QtQuick 6.5

// Non-self equip area (Photo bottom overlay). EquipAreaBase with photo layout
// (image/fullskin/small-equips, 140x19, suitArea [117,2,21,17], pointArea
// [106,-4,25,25]). Display only -- no click routing / valid state / pending.
EquipAreaBase {
    equipIconDir: "image/fullskin/small-equips/"
    equipIconHeight: 19
    equipIconWidth: 140
    pointArea: [106, -4, 25, 25]
    slotHeight: 19
    slotStep: 33
    slotWidth: 140
    suitArea: [117, 2, 21, 17]
}
