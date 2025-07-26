// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick 6.5

Item {
    anchors.fill: parent

    Item {
        visible: true

        height: 1024
        width: Math.max(800, parent.width * 1024 / Math.max(0.1, parent.height))

        transformOrigin: Item.TopLeft
        scale: parent.height / 1024

        RootItem {
            // This item has:
            // fixed height 1024
            // width scaled with height, with 800 as minimum

            // All visual children can assume the height is never changed
            // only horizontal position / width may be considered when resizing
        }
    }
}
