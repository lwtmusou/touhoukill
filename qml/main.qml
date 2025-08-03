import QtQuick 6.5

Item {
    anchors.fill: parent

    Item {
        visible: true

        height: 1440
        width: Math.max(1440, parent.width * 1440 / Math.max(0.1, parent.height))

        transformOrigin: Item.TopLeft
        scale: parent.height / 1440

        RootItem {
            // This item has:
            // fixed height 1440
            // width scaled with height, with 1440 as minimum

            // All visual children can assume the height is never changed
            // only horizontal position / width may be considered when resizing
        }
    }
}
