import QtQuick 6.5
import rocks.touhousatsu 1.0

Item {
    id: dashboard

    height: 360

    Image {
        anchors.left: parent.left
        anchors.top: parent.top
        height: parent.height
        source: G.getUrl("image/system/dashboard-equip.png")
        width: 347

        // TODO: Switch to QSanSkillButton!
        Image {
            id: weaponArea

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 72
            height: 51
            visible: false
            width: 307
        }

        Image {
            id: armorArea

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 129
            height: 51
            visible: false
            width: 307
        }

        Image {
            id: dhorseArea

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 186
            height: 51
            visible: false
            width: 307
        }

        Image {
            id: ohorseArea

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 243
            height: 51
            visible: false
            width: 307
        }

        Image {
            id: treasureArea

            anchors.left: parent.left
            anchors.leftMargin: 12
            anchors.top: parent.top
            anchors.topMargin: 300
            height: 51
            visible: false
            width: 307
        }
    }

    Image {
        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height
        source: G.getUrl("image/system/dashboard-hand.png")
        width: parent.width - 347

        CardContainer {
            id: cardArea

            anchors.fill: parent
        }
    }
}
