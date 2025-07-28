import QtQuick 6.5

Item {
    anchors.fill: parent

    Item {
        visible: true

        height: 1080
        width: Math.max(1080, parent.width * 1080 / Math.max(0.1, parent.height))

        transformOrigin: Item.TopLeft
        scale: parent.height / 1080

        RootItem {
            // This item has:
            // fixed height 1080
            // width scaled with height, with 1080 as minimum

            // All visual children can assume the height is never changed
            // only horizontal position / width may be considered when resizing
        }
    }
}
