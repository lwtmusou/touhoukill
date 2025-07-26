// SPDX-License-Identifier: AGPL-3.0-or-later

import QtQuick 6.5

Image {
    id: rootItem

    anchors.fill: parent

    // source: "../assets/1.jpg"

    StartScene {
        id: startScene
        anchors.fill: parent
    }

    Item {
        id: roomScene
        anchors.fill: parent
        visible: false
    }
}
