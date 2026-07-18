import QtQuick 6.5

Image {
    id: backgroundImage

    anchors.fill: parent
    cache: false
    fillMode: Image.PreserveAspectCrop

    // left source property set by RootItem

    Item {
        id: scalableRoot

        // for screens with abnormal borders
        property real bottomMargin: 0
        property real leftMargin: MainWindowInstance.leftMargin
        property real rightMargin: 0
        property real topMargin: 0

        height: 1440
        scale: (parent.height - topMargin - bottomMargin) / 1440
        transformOrigin: Item.TopLeft
        visible: true
        width: Math.max(1920, (parent.width - leftMargin - rightMargin) * 1440 / Math.max(0.1, (parent.height - topMargin - bottomMargin)))
        x: leftMargin
        y: topMargin

        RootItem {
            anchors.fill: parent

            // This item has:
            // fixed height 1440
            // width scaled with height, with 1920 as minimum

            // All visual children can assume the height is never changed
            // only horizontal position / width may be considered when resizing
        }
    }

    // TODO:
    // Another item for notifying if the width is too small
}
