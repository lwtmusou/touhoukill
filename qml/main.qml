import QtQuick 6.5

Item {
    anchors.fill: parent

    Item {
        height: 1440
        scale: parent.height / 1440
        transformOrigin: Item.TopLeft
        visible: true
        width: Math.max(1920, parent.width * 1440 / Math.max(0.1, parent.height))

        RootItem {
            anchors.fill: parent

            // This item has:
            // fixed height 1440
            // width scaled with height, with 1920 as minimum

            // All visual children can assume the height is never changed
            // only horizontal position / width may be considered when resizing
        }
    }
}
