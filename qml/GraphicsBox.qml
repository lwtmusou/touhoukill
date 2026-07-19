import QtQuick 6.5

import rocks.touhousatsu 1.0

// Generic draggable container with an image background, base for popups/boxes
// (QML counterpart of the old QGraphics GraphicsBox). No title, no action buttons;
// subclasses populate content via the default `content` slot. Set `source` to the
// background image url (e.g. G.getAssetUrl("image/system/guanxing-box.png")).

Image {
    id: graphicsBox

    // Children declared inside are placed over the background.
    default property alias content: contentItem.children

    height: 360
    width: 720

    // Centered initially; position via x/y (not anchors) so dragging works.
    Component.onCompleted: {
        if (parent) {
            graphicsBox.x = (parent.width - graphicsBox.width) / 2;
            graphicsBox.y = (parent.height - graphicsBox.height) / 2;
        }
    }

    // Drag handle covering the background; child MouseAreas declared on top take precedence,
    // so interactive content (buttons/cards) still receives clicks while empty areas drag the box.
    MouseArea {
        anchors.fill: parent
        drag.axis: Drag.XAndYAxis
        drag.target: graphicsBox
    }

    Item {
        id: contentItem

        anchors.fill: parent
    }
}
