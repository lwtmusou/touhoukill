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
        source: G.getAssetUrl("image/system/dashboard-equip.png")
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
        source: G.getAssetUrl("image/system/dashboard-hand.png")
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
                source: G.getAssetUrl("image/system/button/button.png")
                text: qsTr("Trust")
                width: 268

                onClicked: dashboard.clientInstance.trust()
            }

            QSanButton {
                // Enabled in Playing phase (used to end the play / skip). Discarding-phase
                // discard submits via OK after card selection (TODO: wire onPlayerDiscardCards
                // once CardItem selection lands).
                enabled: dashboard.clientInstance != null && dashboard.clientInstance.status === Client.Playing
                font.pixelSize: 50
                height: 133
                source: G.getAssetUrl("image/system/button/button.png")
                text: qsTr("Discard")
                width: 268

                // TODO: dashboard.clientInstance.onPlayerDiscardCards(selectedCard)
            }

            QSanButton {
                // Enabled in ExecDialog / AskForSkillInvoke, or in Responding/Discarding/Exchanging
                // when the discard action is refusable (Client::discardActionRefusable).
                enabled: dashboard.clientInstance != null
                         && (dashboard.clientInstance.status === Client.ExecDialog
                             || dashboard.clientInstance.status === Client.AskForSkillInvoke
                             || (dashboard.clientInstance.discardActionRefusable
                                 && ((dashboard.clientInstance.status & Client.ClientStatusBasicMask) === Client.Responding
                                     || dashboard.clientInstance.status === Client.Discarding
                                     || dashboard.clientInstance.status === Client.Exchanging)))
                font.pixelSize: 50
                height: 133
                source: G.getAssetUrl("image/system/button/button.png")
                text: qsTr("Cancel")
                width: 268

                // TODO: doCancelButton based on ClientInstance.status
            }

            QSanButton {
                // OK: confirms ChooseGeneralBox selection, or acknowledges AskForSkillInvoke.
                // Other response statuses enable OK after card selection lands (CardItem.selected TODO).
                enabled: dashboard.clientInstance != null
                         && ((dashboard.parent && dashboard.parent.activeChooseGeneralBox !== null && dashboard.parent.activeChooseGeneralBox.canAccept)
                             || dashboard.clientInstance.status === Client.AskForSkillInvoke)
                font.pixelSize: 50
                height: 133
                source: G.getAssetUrl("image/system/button/button.png")
                text: qsTr("OK")
                width: 268

                onClicked: {
                    if (dashboard.parent && dashboard.parent.activeChooseGeneralBox !== null)
                        dashboard.parent.activeChooseGeneralBox.accept();
                    // TODO: handle AskForSkillInvoke / card-response OK once wired
                }
            }
        }
    }
}
