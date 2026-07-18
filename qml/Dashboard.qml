import QtQuick 6.5
import rocks.touhousatsu 1.0

Item {
    id: dashboard

    property var clientInstance: parent ? parent.ClientInstance : null

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

        // Operation buttons floating above the hand area (negative y).
        // Trust is fully functional (Client::trust slot); OK/Cancel/Discard depend on CardItem
        // selection & target choosing (CardItem.selected is currently commented out), so they are
        // UI stubs with TODO until card selection lands.
        Row {
            anchors.bottom: cardArea.top
            anchors.bottomMargin: 8
            anchors.horizontalCenter: cardArea.horizontalCenter
            spacing: 8

            QSanButton {
                enabled: dashboard.clientInstance != null
                font.pixelSize: 50
                height: 133
                source: G.getUrl("image/system/button/button.png")
                text: qsTr("Trust")
                width: 268

                onClicked: dashboard.clientInstance.trust()
            }

            QSanButton {
                // TODO: enable on Discarding status with cards selected
                enabled: false
                font.pixelSize: 50
                height: 133
                source: G.getUrl("image/system/button/button.png")
                text: qsTr("Discard")
                width: 268

                // TODO: dashboard.clientInstance.onPlayerDiscardCards(selectedCard)
            }

            QSanButton {
                // TODO: enable based on ClientInstance.status (cancel is context-dependent)
                enabled: false
                font.pixelSize: 50
                height: 133
                source: G.getUrl("image/system/button/button.png")
                text: qsTr("Cancel")
                width: 268

                // TODO: doCancelButton based on ClientInstance.status
            }

            QSanButton {
                // OK: confirms the active ChooseGeneralBox selection (single-general case for now).
                enabled: dashboard.parent && dashboard.parent.activeChooseGeneralBox !== null && dashboard.parent.activeChooseGeneralBox.selectedGenerals.length > 0
                font.pixelSize: 50
                height: 133
                source: G.getUrl("image/system/button/button.png")
                text: qsTr("OK")
                width: 268

                onClicked: dashboard.parent.activeChooseGeneralBox.accept()
            }
        }
    }
}
